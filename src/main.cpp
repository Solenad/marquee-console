#include "engine.h"
#include "calibrate.h"
#include "marquee.h"
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Usage
// ---------------------------------------------------------------------------

static void print_usage()
{
    printf("Marquee Console v1.0.0\n");
    printf("Usage: marquee-console [options]\n\n");
    printf("Options:\n");
    printf("  --refresh <Hz>     Refresh rate (default: 60, range: 1-10000)\n");
    printf("  --poll <Hz>        Input poll rate (default: 250, range: 1-10000)\n");
    printf("  --calibrate        Run hardware calibration and exit\n");
    printf("  --help             Show this message\n\n");
    printf("Controls:\n");
    printf("  q                  Quit\n");
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char** argv)
{
    int  refresh_rate  = 60;
    int  poll_rate     = 250;
    bool run_calibrate = false;

    // Parse CLI args
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_usage();
            return 0;
        }
        else if (strcmp(argv[i], "--calibrate") == 0)
        {
            run_calibrate = true;
        }
        else if (strcmp(argv[i], "--refresh") == 0 && i + 1 < argc)
        {
            refresh_rate = atoi(argv[++i]);
            if (refresh_rate < 1)   refresh_rate = 1;
            if (refresh_rate > 10000) refresh_rate = 10000;
        }
        else if (strcmp(argv[i], "--poll") == 0 && i + 1 < argc)
        {
            poll_rate = atoi(argv[++i]);
            if (poll_rate < 1)   poll_rate = 1;
            if (poll_rate > 10000) poll_rate = 10000;
        }
        else
        {
            printf("Unknown option: %s\n\n", argv[i]);
            print_usage();
            return 1;
        }
    }

    // Initialise engine
    ConsoleEngine engine;
    int err = engine_init(&engine);
    if (err != 0)
    {
        fprintf(stderr, "Engine initialisation failed (code %d)\n", err);
        return 1;
    }

    engine_set_refresh_rate(&engine, refresh_rate);
    engine_set_poll_rate(&engine, poll_rate);

    if (run_calibrate)
    {
        calibrate_run(&engine);
        engine_cleanup(&engine);
        return 0;
    }

    // Normal mode: initialise marquee and run loop
    marquee_init(&engine);

    int  frame_count  = 0;
    double        fps_elapsed  = 0.0;
    int           fps_display  = 0;

    printf("Marquee Console running (refresh=%d Hz, poll=%d Hz)\n",
           refresh_rate, poll_rate);
    printf("Press 'q' to quit.\n");

    // Main loop: iterate at the higher of refresh_rate and poll_rate
    double max_rate_hz       = (double)(refresh_rate > poll_rate ? refresh_rate : poll_rate);
    double loop_interval_ticks = (double)engine.qpc_frequency.QuadPart / max_rate_hz;

    QueryPerformanceCounter(&engine.last_frame_time);
    engine.last_poll_time = engine.last_frame_time;

    bool running = true;
    while (running)
    {
        LARGE_INTEGER loop_start;
        QueryPerformanceCounter(&loop_start);

        // ---------- Poll cycle ----------
        LARGE_INTEGER poll_elapsed;
        QueryPerformanceCounter(&poll_elapsed);
        double poll_delta_ticks = (double)(poll_elapsed.QuadPart - engine.last_poll_time.QuadPart);
        if (poll_delta_ticks >= engine.poll_interval_ticks)
        {
            engine_poll_input(&engine);
            engine.last_poll_time = poll_elapsed;

            // Check for quit
            int ch;
            while ((ch = engine_read_char(&engine)) != -1)
            {
                if (ch == 'q' || ch == 'Q')
                {
                    running = false;
                    break;
                }
            }
        }

        // ---------- Render cycle ----------
        LARGE_INTEGER render_elapsed;
        QueryPerformanceCounter(&render_elapsed);
        double render_delta_ticks = (double)(render_elapsed.QuadPart - engine.last_frame_time.QuadPart);
        if (render_delta_ticks >= engine.refresh_interval_ticks)
        {
            LARGE_INTEGER render_delta_li;
            render_delta_li.QuadPart = render_elapsed.QuadPart - engine.last_frame_time.QuadPart;
            double delta_ms = qpc_ticks_to_ms(engine.qpc_frequency, render_delta_li);

            engine_clear(&engine);

            // Demo: frame counter at top-left
            {
                char counter[64];
                snprintf(counter, sizeof(counter), "FPS: %d | Refresh: %d Hz | Poll: %d Hz",
                         fps_display, refresh_rate, poll_rate);
                for (int i = 0; counter[i] != '\0' && i < engine.buffer_size.X; ++i)
                    engine_set_pixel(&engine, i, 0, counter[i],
                                     FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }

            // Marquee callback (if registered)
            if (engine.frame_callback)
                engine.frame_callback(engine.back_buffer, delta_ms, engine.buffer_size);

            engine_render(&engine);
            engine.last_frame_time = render_elapsed;
            ++frame_count;

            // FPS counter update every ~1 second
            fps_elapsed += delta_ms;
            if (fps_elapsed >= 1000.0)
            {
                fps_display = frame_count;
                frame_count = 0;
                fps_elapsed -= 1000.0;
            }
        }

        // ---------- Frame pacing ----------
        LARGE_INTEGER after;
        QueryPerformanceCounter(&after);
        double elapsed_loop_ticks = (double)(after.QuadPart - loop_start.QuadPart);
        if (elapsed_loop_ticks < loop_interval_ticks)
        {
            // Busy-wait with yield
            LONGLONG target = loop_start.QuadPart + (LONGLONG)loop_interval_ticks;
            do {
                Sleep(0);
                QueryPerformanceCounter(&after);
            } while (after.QuadPart < target);
        }
    }

    engine_cleanup(&engine);
    printf("\nGoodbye.\n");
    return 0;
}
