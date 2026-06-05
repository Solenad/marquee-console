#include "engine.h"
#include <conio.h>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Engine lifecycle
// ---------------------------------------------------------------------------

int engine_init(ConsoleEngine* eng)
{
    // Zero-initialise everything
    std::memset(eng, 0, sizeof(*eng));

    eng->console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (eng->console_handle == INVALID_HANDLE_VALUE || eng->console_handle == NULL)
        return -1;

    if (!GetConsoleScreenBufferInfo(eng->console_handle, &eng->csbi))
        return -2;

    eng->buffer_size.X = eng->csbi.dwSize.X;
    eng->buffer_size.Y = eng->csbi.dwSize.Y;

    // Allocate double buffers
    size_t cell_count = (size_t)eng->buffer_size.X * (size_t)eng->buffer_size.Y;
    eng->front_buffer = new CHAR_INFO[cell_count];
    eng->back_buffer  = new CHAR_INFO[cell_count];

    if (!eng->front_buffer || !eng->back_buffer)
        return -3;

    // Default fill
    CHAR_INFO default_cell;
    default_cell.Char.UnicodeChar = L' ';
    default_cell.Attributes       = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    for (size_t i = 0; i < cell_count; ++i)
    {
        eng->front_buffer[i] = default_cell;
        eng->back_buffer[i]  = default_cell;
    }

    // QPC init
    QueryPerformanceFrequency(&eng->qpc_frequency);
    QueryPerformanceCounter(&eng->last_frame_time);
    eng->last_poll_time = eng->last_frame_time;

    // Default rates: 324 Hz refresh, 2000 Hz poll (calibrated values)
    engine_set_refresh_rate(eng, 324);
    engine_set_poll_rate(eng, 2000);

    // Ring buffer
    eng->ring_write = 0;
    eng->ring_read  = 0;

    return 0;
}

void engine_cleanup(ConsoleEngine* eng)
{
    delete[] eng->front_buffer;
    delete[] eng->back_buffer;
    eng->front_buffer = nullptr;
    eng->back_buffer  = nullptr;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void engine_render(ConsoleEngine* eng)
{
    SMALL_RECT write_region;
    write_region.Left   = 0;
    write_region.Top    = 0;
    write_region.Right  = eng->buffer_size.X - 1;
    write_region.Bottom = eng->buffer_size.Y - 1;

    WriteConsoleOutputW(
        eng->console_handle,
        eng->back_buffer,
        eng->buffer_size,
        { 0, 0 },
        &write_region
    );

    // Swap buffers
    CHAR_INFO* tmp = eng->front_buffer;
    eng->front_buffer = eng->back_buffer;
    eng->back_buffer  = tmp;
}

void engine_set_pixel(ConsoleEngine* eng, int x, int y, wchar_t ch, WORD attrs)
{
    if (x < 0 || x >= eng->buffer_size.X || y < 0 || y >= eng->buffer_size.Y)
        return;

    size_t idx = (size_t)y * (size_t)eng->buffer_size.X + (size_t)x;
    eng->back_buffer[idx].Char.UnicodeChar = ch;
    eng->back_buffer[idx].Attributes       = attrs;
}

void engine_clear(ConsoleEngine* eng)
{
    CHAR_INFO empty;
    empty.Char.UnicodeChar = L' ';
    empty.Attributes       = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    size_t cell_count = (size_t)eng->buffer_size.X * (size_t)eng->buffer_size.Y;
    for (size_t i = 0; i < cell_count; ++i)
        eng->back_buffer[i] = empty;
}

void engine_set_refresh_rate(ConsoleEngine* eng, int hz)
{
    if (hz < 1) hz = 1;
    if (hz > 10000) hz = 10000;
    eng->refresh_interval_ticks = (double)eng->qpc_frequency.QuadPart / (double)hz;
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void engine_poll_input(ConsoleEngine* eng)
{
    while (_kbhit())
    {
        int ch = _getch();
        if (ch == EOF) continue;

        size_t next = (eng->ring_write + 1) % RING_BUFFER_SIZE;
        if (next == eng->ring_read)
        {
            // Overflow: discard oldest
            eng->ring_read = (eng->ring_read + 1) % RING_BUFFER_SIZE;
        }
        eng->ring_buffer[eng->ring_write] = (char)ch;
        eng->ring_write = next;
    }
}

int engine_read_char(ConsoleEngine* eng)
{
    if (eng->ring_read == eng->ring_write)
        return -1; // empty

    char ch = eng->ring_buffer[eng->ring_read];
    eng->ring_read = (eng->ring_read + 1) % RING_BUFFER_SIZE;
    return ch;
}

void engine_set_poll_rate(ConsoleEngine* eng, int hz)
{
    if (hz < 1) hz = 1;
    if (hz > 10000) hz = 10000;
    eng->poll_interval_ticks = (double)eng->qpc_frequency.QuadPart / (double)hz;
}

// ---------------------------------------------------------------------------
// Frame callback
// ---------------------------------------------------------------------------

void engine_set_frame_callback(ConsoleEngine* eng,
                                void (*cb)(CHAR_INFO*, double, COORD),
                                void* ctx)
{
    eng->frame_callback      = cb;
    eng->frame_callback_ctx  = ctx;
}
