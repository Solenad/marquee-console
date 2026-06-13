// test_marquee.cpp — automated verification harness
// Uses AllocConsole to satisfy Windows Console API, then writes results
// to the original stdout pipe so node can capture it.
#include "engine.h"
#include "calibrate.h"
#include "marquee.h"
#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <fcntl.h>

int main()
{
    // Save the original stdout FD (the pipe from node) before touching the console
    int orig_stdout_fd = _dup(1);          // dup original stdout
    FILE* orig_stdout = _fdopen(orig_stdout_fd, "w");
    if (!orig_stdout)
    {
        // fallback: just use stdout directly
        orig_stdout = stdout;
    }

    // Allocate a console for the Windows Console API
    FreeConsole();
    if (!AllocConsole())
    {
        fprintf(orig_stdout, "FAIL: AllocConsole (GLE=%lu)\n", GetLastError());
        return 1;
    }

    // Get the console output handle
    HANDLE hCon = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hCon == INVALID_HANDLE_VALUE)
    {
        fprintf(orig_stdout, "FAIL: CreateFile CONOUT$ (GLE=%lu)\n", GetLastError());
        return 1;
    }

    // Temporarily set the console handle so engine_init works
    HANDLE old_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetStdHandle(STD_OUTPUT_HANDLE, hCon);

    // Verify console works
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hCon, &csbi))
    {
        SetStdHandle(STD_OUTPUT_HANDLE, old_handle);
        fprintf(orig_stdout, "FAIL: GetConsoleScreenBufferInfo (GLE=%lu)\n", GetLastError());
        CloseHandle(hCon);
        return 1;
    }

    // ---- Helper: report results through original stdout ----
    int passed = 0;
    int failed = 0;
    auto report = [&](const char* label, bool ok, const char* detail = "") {
        fprintf(orig_stdout, "[%s] %s %s\n", ok ? "PASS" : "FAIL", label, detail);
        if (ok) passed++; else failed++;
    };

    fprintf(orig_stdout, "=== Marquee Console Test Harness ===\n\n");
    fprintf(orig_stdout, "Console OK: %d x %d\n\n", csbi.dwSize.X, csbi.dwSize.Y);

    // ---- Test 1: engine_init ----
    {
        ConsoleEngine eng;
        int rc = engine_init(&eng);

        // Restore original handle for stdout
        SetStdHandle(STD_OUTPUT_HANDLE, old_handle);

        report("engine_init", rc == 0);
        if (rc != 0)
        {
            CloseHandle(hCon);
            fprintf(orig_stdout, "\n=== Results: %d passed, %d failed ===\n", passed, failed);
            return 1;
        }

        // Test 2: buffers
        bool bufs_ok = eng.front_buffer && eng.back_buffer && eng.buffer_size.X > 0;
        char buf_sz[64];
        snprintf(buf_sz, sizeof(buf_sz), "(%d x %d)", eng.buffer_size.X, eng.buffer_size.Y);
        report("buffers allocated", bufs_ok, buf_sz);

        // Test 3: QPC
        report("QPC initialized", eng.qpc_frequency.QuadPart > 0);

        // Test 4: default rates
        {
            double r_hz = (double)eng.qpc_frequency.QuadPart / eng.refresh_interval_ticks;
            double p_hz = (double)eng.qpc_frequency.QuadPart / eng.poll_interval_ticks;
            char buf[64];
            snprintf(buf, sizeof(buf), "(%.0f Hz refresh, %.0f Hz poll)", r_hz, p_hz);
            report("default rates", r_hz == 324.0 && p_hz == 2000.0, buf);
        }

        // Test 5: set_pixel
        {
            engine_set_pixel(&eng, 0, 0, L'X', FOREGROUND_RED);
            bool ok = (eng.back_buffer[0].Char.UnicodeChar == L'X' &&
                       eng.back_buffer[0].Attributes == FOREGROUND_RED);
            report("set_pixel", ok);
        }

        // Test 6: clear
        {
            engine_clear(&eng);
            bool ok = (eng.back_buffer[0].Char.UnicodeChar == L' ');
            report("clear", ok);
        }

        // Test 7: render (buffer swap)
        {
            engine_set_pixel(&eng, 0, 0, L'Y', FOREGROUND_GREEN);
            engine_render(&eng);
            bool ok = (eng.back_buffer[0].Char.UnicodeChar == L' ');
            report("render (buffer swap)", ok);
        }

        // Test 8: ring buffer empty
        {
            engine_poll_input(&eng);
            bool ok = (engine_read_char(&eng) == -1);
            report("ring buffer empty returns -1", ok);
        }

        // Test 9: rate changes
        {
            engine_set_refresh_rate(&eng, 120);
            bool r1 = (eng.refresh_interval_ticks == (double)eng.qpc_frequency.QuadPart / 120.0);
            report("set_refresh_rate(120)", r1);

            engine_set_poll_rate(&eng, 500);
            bool r2 = (eng.poll_interval_ticks == (double)eng.qpc_frequency.QuadPart / 500.0);
            report("set_poll_rate(500)", r2);
        }

        // Test 10: marquee registration
        {
            engine_set_frame_callback(&eng, nullptr, nullptr);
            marquee_init(&eng);
            report("marquee_init registers callback", eng.frame_callback == marquee_on_frame);
        }

        // Test 11: Nyan Cat rainbow wave trail leaves scattered colors
        {
            marquee_on_frame(eng.back_buffer, 16.0, eng.buffer_size);
            int W = eng.buffer_size.X;
            int H = eng.buffer_size.Y;
            (void)H;
            int cat_x = (W - 30) / 2;
            int cat_top = (H - 11) / 2;
            int center_y = cat_top + 11 / 2;

            // Scan first few columns near the wave trail center
            bool found = false;
            for (int x = 0; x < 10 && x < cat_x; ++x)
            {
                for (int dy = -3; dy <= 3; ++dy)
                {
                    int y = center_y + dy;
                    if (y < 0 || y >= H) continue;
                    size_t idx = (size_t)y * (size_t)W + (size_t)x;
                    WORD bg = eng.back_buffer[idx].Attributes
                              & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
                    if (bg != 0) found = true;
                }
            }
            report("nyan cat rainbow wave trail leaves scattered colors", found);
        }

        // Test 12: cat area has no rainbow (clear background)
        {
            int W = eng.buffer_size.X;
            int H = eng.buffer_size.Y;
            (void)H;
            int cat_x = (W - 30) / 2;
            int cat_top = (H - 11) / 2;
            bool clear = true;
            if (cat_x < W)
            {
                size_t idx = (size_t)cat_top * (size_t)W + (size_t)cat_x;
                WORD bg = eng.back_buffer[idx].Attributes
                          & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
                clear = (bg == 0);
            }
            report("cat area has no rainbow overlay", clear);
        }

        // Test 13: hardware fingerprint (no crash)
        {
            calibrate_hardware_fingerprint(&eng);
            report("calibrate_hardware_fingerprint", true);
        }

        // Test 14: out-of-bounds set_pixel (no crash)
        {
            engine_set_pixel(&eng, -1, 0, L'X', 0);
            engine_set_pixel(&eng, 9999, 9999, L'X', 0);
            report("out-of-bounds set_pixel no crash", true);
        }

        // Test 15: rate clamping
        {
            engine_set_refresh_rate(&eng, 0);
            double min_ticks = (double)eng.qpc_frequency.QuadPart / 1.0;
            bool clamped_min = (eng.refresh_interval_ticks == min_ticks);
            report("refresh rate clamped to min (1 Hz)", clamped_min);

            engine_set_refresh_rate(&eng, 20000);
            double max_ticks = (double)eng.qpc_frequency.QuadPart / 10000.0;
            bool clamped_max = (eng.refresh_interval_ticks == max_ticks);
            report("refresh rate clamped to max (10000 Hz)", clamped_max);
        }

        engine_cleanup(&eng);
    }

    fprintf(orig_stdout, "\n=== Results: %d passed, %d failed ===\n", passed, failed);
    CloseHandle(hCon);
    fclose(orig_stdout);
    return failed > 0 ? 1 : 0;
}
