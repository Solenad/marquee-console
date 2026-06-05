#include "marquee.h"
#include <cwchar>

// Current marquee text
static const wchar_t* MARQUEE_TEXT = L"[MARQUEE PLACEHOLDER]";
static const WORD     MARQUEE_COLOR = FOREGROUND_GREEN | FOREGROUND_INTENSITY;

// ---------------------------------------------------------------------------
// Initialisation: register the frame callback
// ---------------------------------------------------------------------------

void marquee_init(ConsoleEngine* eng)
{
    engine_set_frame_callback(eng, marquee_on_frame, nullptr);
}

// ---------------------------------------------------------------------------
// Per-frame callback
// ---------------------------------------------------------------------------

void marquee_on_frame(CHAR_INFO* buffer, double /*delta_ms*/, COORD buffer_size)
{
    if (!buffer) return;

    size_t len = wcslen(MARQUEE_TEXT);
    if (len == 0) return;

    // Centre horizontally
    int start_x = (buffer_size.X - (SHORT)len) / 2;
    if (start_x < 0) start_x = 0;

    int y = buffer_size.Y / 2;

    // Draw each character
    for (size_t i = 0; i < len; ++i)
    {
        int x = start_x + (int)i;
        if (x >= buffer_size.X) break;

        size_t idx = (size_t)y * (size_t)buffer_size.X + (size_t)x;
        buffer[idx].Char.UnicodeChar = MARQUEE_TEXT[i];
        buffer[idx].Attributes       = MARQUEE_COLOR;
    }
}
