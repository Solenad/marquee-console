#pragma once
#include "engine.h"

// Initialize marquee animation and register frame callback with the engine.
void marquee_init(ConsoleEngine* eng);

// Per-frame callback: renders the marquee text.
// Registered via engine_set_frame_callback.
void marquee_on_frame(CHAR_INFO* buffer, double delta_ms, COORD buffer_size);
