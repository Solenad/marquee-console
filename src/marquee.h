#pragma once
#include "engine.h"

// Nyan Cat rainbow marquee state
struct MarqueeState {
    double  scroll_offset;    // continuous scroll offset (chars)
    double  speed_cps;        // scroll speed in characters per second
    double  bounce_time;      // elapsed time for vertical bounce (seconds)
    double  dot_timer;        // elapsed time for dot animation (seconds)
    bool    initialized;
};

// Initialize marquee animation and register frame callback
void marquee_init(ConsoleEngine* eng);

// Set rainbow scroll speed in characters per second (default: 10)
void marquee_set_speed(ConsoleEngine* eng, double chars_per_sec);

// Per-frame callback registered with the engine
void marquee_on_frame(CHAR_INFO* buffer, double delta_ms, COORD buffer_size);
