#include "calibrate.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static double compute_variance(const double* samples, size_t count, double mean)
{
    if (count < 2) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < count; ++i)
    {
        double d = samples[i] - mean;
        sum += d * d;
    }
    return sum / (double)(count - 1);
}

// Inject a keystroke into the console input buffer via WriteFile.
static void inject_keystroke(HANDLE hConIn, char ch)
{
    INPUT_RECORD ir[2];
    // Key down
    ir[0].EventType = KEY_EVENT;
    ir[0].Event.KeyEvent.bKeyDown = TRUE;
    ir[0].Event.KeyEvent.wRepeatCount = 1;
    ir[0].Event.KeyEvent.wVirtualKeyCode = 0x41; // 'A' virtual key code
    ir[0].Event.KeyEvent.wVirtualScanCode = 0;
    ir[0].Event.KeyEvent.uChar.AsciiChar = ch;
    ir[0].Event.KeyEvent.dwControlKeyState = 0;
    // Key up
    ir[1] = ir[0];
    ir[1].Event.KeyEvent.bKeyDown = FALSE;

    DWORD written = 0;
    WriteConsoleInput(hConIn, ir, 2, &written);
}

// ---------------------------------------------------------------------------
// Refresh rate sweep
// ---------------------------------------------------------------------------

void calibrate_refresh_sweep(ConsoleEngine* eng,
                              int* out_max_stable_refresh,
                              int* out_first_tearing_rate)
{
    const int STEP        = 10;
    const int MAX_HZ      = 360;
    const int MIN_HZ      = 10;
    const int DURATION_MS = 2000;   // measurement window per step

    int  max_stable      = MIN_HZ;
    int  first_tearing   = 0;
    bool tearing_found   = false;

    // Fixed poll rate for the sweep – keep it comfortable
    engine_set_poll_rate(eng, 250);

    printf("\n--- Refresh rate sweep (%d Hz to %d Hz, step %d Hz) ---\n",
           MIN_HZ, MAX_HZ, STEP);

    for (int hz = MIN_HZ; hz <= MAX_HZ; hz += STEP)
    {
        engine_set_refresh_rate(eng, hz);

        // Collect frame deltas over DURATION_MS
        const size_t MAX_SAMPLES = 4096;
        double       deltas[MAX_SAMPLES];
        size_t       sample_count = 0;
        LARGE_INTEGER prev, now;
        QueryPerformanceCounter(&prev);

        double elapsed = 0.0;
        while (elapsed < (double)DURATION_MS)
        {
            // Busy-wait until next frame
            LARGE_INTEGER target;
            target.QuadPart = prev.QuadPart + (LONGLONG)eng->refresh_interval_ticks;
            do {
                QueryPerformanceCounter(&now);
            } while (now.QuadPart < target.QuadPart);

            double delta_ms = qpc_elapsed_ms(eng->qpc_frequency, prev, now);
            if (sample_count < MAX_SAMPLES)
                deltas[sample_count++] = delta_ms;

            prev = now;
            elapsed += delta_ms;

            // Poll input so we remain responsive
            engine_poll_input(eng);
        }

        // Compute stats
        double sum = 0.0;
        for (size_t i = 0; i < sample_count; ++i) sum += deltas[i];
        double mean   = sum / (double)sample_count;
        double var    = compute_variance(deltas, sample_count, mean);
        double stddev = std::sqrt(var);

        int measured_hz = (mean > 0.0) ? (int)(1000.0 / mean + 0.5) : 0;

        printf("  %3d Hz target -> %3d Hz measured | mean %.3f ms | stddev %.3f ms | var %.6f\n",
               hz, measured_hz, mean, stddev, var);

        // Tearing detection: variance > 2 ms²
        if (!tearing_found && var > 2.0)
        {
            first_tearing = hz;
            tearing_found = true;
            printf("  *** Tearing detected at %d Hz (var=%.4f > 2.0) ***\n", hz, var);
        }

        if (!tearing_found)
            max_stable = hz;
    }

    *out_max_stable_refresh = max_stable;
    *out_first_tearing_rate = tearing_found ? first_tearing : 0;

    printf("  => Max stable refresh: %d Hz\n", max_stable);
    if (tearing_found)
        printf("  => Tearing floor:      %d Hz\n", first_tearing);
    else
        printf("  => No tearing detected up to %d Hz\n", MAX_HZ);
}

// ---------------------------------------------------------------------------
// Polling rate sweep
// ---------------------------------------------------------------------------

void calibrate_poll_sweep(ConsoleEngine* eng,
                           int* out_min_effective_poll,
                           int* out_first_latency_ceiling)
{
    const int STEP        = 50;
    const int MAX_POLL    = 1000;
    const int MIN_POLL    = 50;
    const int SAMPLES_PER_STEP = 20;

    int  min_effective  = MAX_POLL;
    int  first_ceiling  = 0;
    bool ceiling_found  = false;

    // Fixed refresh during poll sweep
    engine_set_refresh_rate(eng, 60);
    HANDLE hConIn = GetStdHandle(STD_INPUT_HANDLE);

    printf("\n--- Polling rate sweep (%d Hz to %d Hz, step %d Hz) ---\n",
           MIN_POLL, MAX_POLL, STEP);

    for (int hz = MIN_POLL; hz <= MAX_POLL; hz += STEP)
    {
        engine_set_poll_rate(eng, hz);

        double latencies[SAMPLES_PER_STEP];
        int    valid_samples = 0;

        for (int s = 0; s < SAMPLES_PER_STEP; ++s)
        {
            // Inject a keystroke
            char test_char = 'a' + (s % 26);
            inject_keystroke(hConIn, test_char);

            LARGE_INTEGER t0, t1;
            QueryPerformanceCounter(&t0);

            // Spin until we see the character or timeout
            int       got = -1;
            const int TIMEOUT_MS = 500;
            double    waited = 0.0;
            while (waited < (double)TIMEOUT_MS)
            {
                engine_poll_input(eng);
                got = engine_read_char(eng);
                if (got == test_char) break;

                LARGE_INTEGER now;
                QueryPerformanceCounter(&now);
                waited = qpc_elapsed_ms(eng->qpc_frequency, t0, now);

                // Small back-off to avoid 100% spin on missed chars
                Sleep(0);
            }

            QueryPerformanceCounter(&t1);
            double latency_ms = qpc_elapsed_ms(eng->qpc_frequency, t0, t1);

            if (got == test_char && valid_samples < SAMPLES_PER_STEP)
            {
                latencies[valid_samples++] = latency_ms;
            }
        }

        // Compute p95
        if (valid_samples > 0)
        {
            std::sort(latencies, latencies + valid_samples);
            int p95_idx = (int)(0.95 * (double)valid_samples);
            if (p95_idx >= valid_samples) p95_idx = valid_samples - 1;
            double p95 = latencies[p95_idx];

            double sum = 0.0;
            for (int i = 0; i < valid_samples; ++i) sum += latencies[i];
            double avg = sum / (double)valid_samples;

            printf("  %3d Hz poll -> p95 %.3f ms | avg %.3f ms (%d samples)\n",
                   hz, p95, avg, valid_samples);

            if (!ceiling_found && p95 > 50.0)
            {
                first_ceiling = hz;
                ceiling_found = true;
                printf("  *** Latency ceiling at %d Hz (p95=%.3f > 50 ms) ***\n", hz, p95);
            }
        }
        else
        {
            printf("  %3d Hz poll -> no valid samples\n", hz);
        }

        if (!ceiling_found)
            min_effective = hz;
    }

    *out_min_effective_poll  = min_effective;
    *out_first_latency_ceiling = ceiling_found ? first_ceiling : 0;

    printf("  => Min effective poll: %d Hz\n", min_effective);
    if (ceiling_found)
        printf("  => Latency ceiling:    %d Hz\n", first_ceiling);
    else
        printf("  => No latency issues up to %d Hz\n", MAX_POLL);
}

// ---------------------------------------------------------------------------
// Hardware fingerprint
// ---------------------------------------------------------------------------

void calibrate_hardware_fingerprint(ConsoleEngine* eng)
{
    printf("\n--- Hardware Fingerprint ---\n");

    // QPC frequency
    printf("  QPC Frequency: %lld ticks/sec\n", eng->qpc_frequency.QuadPart);

    // OS version
    OSVERSIONINFOW osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
    GetVersionExW(&osvi);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    printf("  OS Version:    %lu.%lu.%lu\n",
           osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

    // Console host module name
    wchar_t module_path[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, module_path, MAX_PATH);
    if (len > 0)
    {
        printf("  Host process:  %ls\n", module_path);
    }

    // Buffer dimensions
    printf("  Console size:  %d x %d\n",
           eng->buffer_size.X, eng->buffer_size.Y);

    // Current rates
    double refresh_hz = (eng->refresh_interval_ticks > 0.0)
        ? (double)eng->qpc_frequency.QuadPart / eng->refresh_interval_ticks
        : 0.0;
    double poll_hz = (eng->poll_interval_ticks > 0.0)
        ? (double)eng->qpc_frequency.QuadPart / eng->poll_interval_ticks
        : 0.0;
    printf("  Current refr:  %.0f Hz\n", refresh_hz);
    printf("  Current poll:  %.0f Hz\n", poll_hz);
}

// ---------------------------------------------------------------------------
// Report
// ---------------------------------------------------------------------------

void calibrate_report(int max_stable_refresh,
                       int tearing_floor,
                       int min_effective_poll,
                       int latency_ceiling)
{
    int recommended_refresh = (int)((double)max_stable_refresh * 0.9 + 0.5);
    int recommended_poll    = min_effective_poll * 2;

    printf("\n========================================\n");
    printf("        CALIBRATION RESULTS\n");
    printf("========================================\n");
    printf("  Max stable refresh:   %d Hz\n", max_stable_refresh);
    printf("  Tearing floor:        %d Hz\n", tearing_floor ? tearing_floor : max_stable_refresh);
    printf("  Min effective poll:   %d Hz\n", min_effective_poll);
    printf("  Latency ceiling:      %d Hz\n", latency_ceiling ? latency_ceiling : min_effective_poll);
    printf("  -----------------------------------\n");
    printf("  Recommended refresh:  %d Hz  (90%% of max stable)\n", recommended_refresh);
    printf("  Recommended poll:     %d Hz  (2x min effective)\n", recommended_poll);
    printf("========================================\n");
}

// ---------------------------------------------------------------------------
// Run all
// ---------------------------------------------------------------------------

void calibrate_run(ConsoleEngine* eng)
{
    printf("=== Marquee Console Calibration Utility ===\n");
    printf("This will sweep refresh and polling rates to find the optimal\n");
    printf("balance for your hardware. Press 'q' to abort at any time.\n\n");

    calibrate_hardware_fingerprint(eng);

    int max_stable_refresh  = 60;
    int tearing_floor       = 0;
    calibrate_refresh_sweep(eng, &max_stable_refresh, &tearing_floor);

    // Check for abort
    engine_poll_input(eng);
    if (engine_read_char(eng) == 'q')
    {
        printf("\nCalibration aborted by user.\n");
        return;
    }

    int min_effective_poll  = 250;
    int latency_ceiling     = 0;
    calibrate_poll_sweep(eng, &min_effective_poll, &latency_ceiling);

    calibrate_report(max_stable_refresh, tearing_floor,
                     min_effective_poll, latency_ceiling);

    // Restore defaults
    engine_set_refresh_rate(eng, 60);
    engine_set_poll_rate(eng, 250);
}
