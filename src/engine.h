#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstddef>
#include <cstdint>

constexpr size_t RING_BUFFER_SIZE = 256;

struct ConsoleEngine {
    HANDLE console_handle;
    CHAR_INFO* front_buffer;
    CHAR_INFO* back_buffer;
    COORD buffer_size;

    // Timing
    LARGE_INTEGER qpc_frequency;
    LARGE_INTEGER last_frame_time;
    LARGE_INTEGER last_poll_time;
    double refresh_interval_ticks;
    double poll_interval_ticks;

    // Input ring buffer
    char ring_buffer[RING_BUFFER_SIZE];
    size_t ring_write;
    size_t ring_read;

    // Frame callback hook (used by marquee animation)
    void (*frame_callback)(CHAR_INFO* buffer, double delta_ms, COORD buffer_size);
    void* frame_callback_ctx;

    // Cached console info
    CONSOLE_SCREEN_BUFFER_INFO csbi;
};

// Engine lifecycle
int  engine_init(ConsoleEngine* eng);
void engine_cleanup(ConsoleEngine* eng);

// Rendering
void engine_render(ConsoleEngine* eng);
void engine_set_pixel(ConsoleEngine* eng, int x, int y, wchar_t ch, WORD attrs);
void engine_clear(ConsoleEngine* eng);
void engine_set_refresh_rate(ConsoleEngine* eng, int hz);

// Input
void engine_poll_input(ConsoleEngine* eng);
int  engine_read_char(ConsoleEngine* eng);
void engine_set_poll_rate(ConsoleEngine* eng, int hz);

// Frame callback registration
void engine_set_frame_callback(ConsoleEngine* eng,
                                void (*cb)(CHAR_INFO*, double, COORD),
                                void* ctx);

// QPC helper
inline double qpc_ticks_to_ms(LARGE_INTEGER freq, LARGE_INTEGER ticks)
{
    return (double)ticks.QuadPart * 1000.0 / (double)freq.QuadPart;
}

inline LARGE_INTEGER qpc_diff(LARGE_INTEGER start, LARGE_INTEGER now)
{
    LARGE_INTEGER d;
    d.QuadPart = now.QuadPart - start.QuadPart;
    return d;
}

inline double qpc_elapsed_ms(LARGE_INTEGER freq, LARGE_INTEGER start, LARGE_INTEGER now)
{
    return qpc_ticks_to_ms(freq, qpc_diff(start, now));
}
