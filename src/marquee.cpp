#include "marquee.h"
#include <cmath>
#include <cwchar>

// ---------------------------------------------------------------------------
// Rainbow colour palette (background attributes)
// ---------------------------------------------------------------------------

static const WORD RAINBOW_COLORS[] = {
    BACKGROUND_RED,
    BACKGROUND_RED | BACKGROUND_GREEN,
    BACKGROUND_GREEN,
    BACKGROUND_GREEN | BACKGROUND_BLUE,
    BACKGROUND_BLUE,
    BACKGROUND_RED | BACKGROUND_BLUE,
};
static const int NUM_RAINBOW = 6;

// ---------------------------------------------------------------------------
// Nyan Cat braille sprite  30 x 11
// ---------------------------------------------------------------------------

static const wchar_t* CAT_LINES[] = {
    L"\u2800\u2800\u2800\u2800\u2800\u2800\u2880\u28C0\u28C0\u28C0\u28E4\u28E4\u28E4\u28E4\u28E4\u28E4\u28E4\u28E4\u28E4\u28E4\u28E4\u28E4\u28E4\u28C0\u28C0\u2800\u2800\u2800\u2800\u2800",
    L"\u2800\u2800\u2800\u2800\u2800\u2800\u28FE\u2809\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u28C0\u2800\u2800\u2800\u2800\u2880\u2800\u2808\u2847\u2800\u2800\u2800\u2800",
    L"\u2800\u2800\u2800\u2800\u2800\u2800\u28FF\u2800\u2801\u2800\u2818\u2801\u2800\u2800\u2800\u2800\u2800\u28C0\u2840\u2800\u2800\u2800\u2808\u2800\u2800\u2847\u2800\u2800\u2800\u2800",
    L"\u28C0\u28C0\u28C0\u2800\u2800\u2800\u28FF\u2800\u2800\u2800\u2800\u2800\u2804\u2800\u2800\u2838\u28B0\u284F\u2809\u2833\u28C4\u2830\u2800\u2800\u28B0\u28F7\u2836\u281B\u28E7\u2800",
    L"\u28BB\u2840\u2808\u2819\u2832\u2844\u28FF\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2820\u2800\u28B8\u2800\u2800\u2800\u2808\u2813\u2812\u2812\u281B\u2801\u2800\u2800\u28FF\u2800",
    L"\u2800\u283B\u28C4\u2800\u2800\u2819\u28FF\u2800\u2800\u2800\u2808\u2801\u2800\u28A0\u2804\u28F0\u281F\u2800\u2880\u2854\u28A0\u2800\u2800\u2800\u2800\u28E0\u2820\u2844\u2818\u28A7",
    L"\u2800\u2800\u2808\u281B\u28A6\u28C0\u28FF\u2800\u2800\u28A0\u2846\u2800\u2800\u2808\u2800\u28EF\u2800\u2800\u2808\u281B\u281B\u2800\u2820\u28A6\u2804\u2819\u281B\u2803\u2800\u28B8",
    L"\u2800\u2800\u2800\u2800\u2800\u2809\u28FF\u2800\u2800\u2800\u28A0\u2800\u2800\u28A0\u2800\u2839\u28C6\u2800\u2800\u2800\u2822\u28A4\u2820\u281E\u2824\u2860\u2804\u2800\u2880\u287E",
    L"\u2800\u2800\u2800\u2800\u2800\u2880\u287F\u2826\u28A4\u28E4\u28E4\u28E4\u28E4\u28E4\u28E4\u28E4\u287C\u28F7\u2836\u2824\u28A4\u28E4\u28E4\u2864\u28A4\u2864\u2836\u2816\u280B\u2800",
    L"\u2800\u2800\u2800\u2800\u2800\u2838\u28E4\u2874\u280B\u2838\u28C7\u28E0\u283C\u2801\u2800\u2800\u2800\u2839\u28C4\u28E0\u281E\u2800\u28BE\u2840\u28E0\u2803\u2800\u2800\u2800\u2800",
    L"\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2800\u2809\u2801\u2800\u2800\u2800\u2800",
};
static const int CAT_HEIGHT = 11;
static const int CAT_WIDTH = 30;

// ---------------------------------------------------------------------------
// Marquee state (static, single-instance)
// ---------------------------------------------------------------------------

static MarqueeState s_state;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void marquee_init(ConsoleEngine* eng)
{
    s_state.scroll_offset = 0.0;
    s_state.speed_cps = 10.0;
    s_state.bounce_time = 0.0;
    s_state.dot_timer = 0.0;
    s_state.initialized = true;

    engine_set_frame_callback(eng, marquee_on_frame, nullptr);
}

void marquee_set_speed(ConsoleEngine* eng, double chars_per_sec)
{
    (void)eng;
    s_state.speed_cps = chars_per_sec;
}

// ---------------------------------------------------------------------------
// Per-frame callback — Nyan Cat rainbow + exit prompt
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Helper: draw the cat sprite at a given screen position with a given color
// ---------------------------------------------------------------------------

static void draw_cat(CHAR_INFO* buffer, int W, int H, int screen_x, int screen_y, WORD fg)
{
    for (int row = 0; row < CAT_HEIGHT; ++row)
    {
        int y = screen_y + row;
        if (y < 0 || y >= H) continue;

        int line_len = (int)wcslen(CAT_LINES[row]);
        for (int cx = 0; cx < line_len; ++cx)
        {
            int wx = screen_x + cx;
            if (wx >= W) break;

            size_t idx = (size_t)y * (size_t)W + (size_t)wx;
            buffer[idx].Char.UnicodeChar = CAT_LINES[row][cx];
            buffer[idx].Attributes = fg;
        }
    }
}

void marquee_on_frame(CHAR_INFO* buffer, double delta_ms, COORD buffer_size)
{
    if (!buffer || !s_state.initialized) return;

    double dt_sec = delta_ms / 1000.0;
    s_state.scroll_offset += s_state.speed_cps * dt_sec;
    s_state.bounce_time += dt_sec;
    s_state.dot_timer += dt_sec;

    int W = buffer_size.X;
    int H = buffer_size.Y;

    if (W < CAT_WIDTH + 4 || H < CAT_HEIGHT + 4) return;

    // ---- Bounce ----
    int bounce = (int)round(sin(s_state.bounce_time * 2.0) * 3.0);

    int cat_x = (W - CAT_WIDTH) / 2;
    int cat_y = (H - CAT_HEIGHT) / 2 + bounce;

    WORD cat_fg = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

    // Rainbow wave trail centered on the cat
    int trail_center_y = cat_y + CAT_HEIGHT / 2;

    for (int x = 0; x < cat_x && x < W; ++x)
    {
        float wave = sinf(x * 0.25f + (float)s_state.scroll_offset * 0.3f);
        int ty = trail_center_y + (int)(wave * 2.5f);
        if (ty < 0 || ty >= H) continue;

        size_t idx = (size_t)ty * (size_t)W + (size_t)x;
        int c = (int)(x * 0.4f + s_state.scroll_offset * 0.2f) % NUM_RAINBOW;
        if (c < 0) c += NUM_RAINBOW;

        buffer[idx].Char.UnicodeChar = L' ';
        buffer[idx].Attributes = RAINBOW_COLORS[c];
    }

    // ---- Braille cat (single draw, no cross-fade) ----
    draw_cat(buffer, W, H, cat_x, cat_y, cat_fg);

    // ---- Exit prompt at bottom with cycling dots ----
    int dot_count = ((int)(s_state.dot_timer / 0.4)) % 4;
    if (dot_count == 3) dot_count = 0;

    wchar_t prompt[32];
    int pi = 0;
    const wchar_t* base = L"press a key to exit";
    while (base[pi])
    {
        prompt[pi] = base[pi];
        ++pi;
    }
    prompt[pi] = L'.';
    ++pi;
    for (int d = 0; d < dot_count; ++d)
    {
        prompt[pi] = L'.';
        ++pi;
    }
    prompt[pi] = L'\0';

    int prompt_y = H - 2;
    if (prompt_y < 0) prompt_y = 0;

    size_t plen = wcslen(prompt);
    int prompt_x = (W - (int)plen) / 2;
    if (prompt_x < 0) prompt_x = 0;

    for (size_t i = 0; i < plen; ++i)
    {
        int wx = prompt_x + (int)i;
        if (wx >= W) break;
        size_t idx = (size_t)prompt_y * (size_t)W + (size_t)wx;
        buffer[idx].Char.UnicodeChar = prompt[i];
        buffer[idx].Attributes = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    }
}
