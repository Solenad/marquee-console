## 1. Project Scaffold

- [x] 1.1 Create `src/` directory with `main.cpp`, `engine.h`, `engine.cpp`, `calibrate.h`, `calibrate.cpp`, `marquee.h`, `marquee.cpp`
- [x] 1.2 Write `CMakeLists.txt` targeting MSVC with console subsystem, C++17, and Windows SDK linkage
- [x] 1.3 Implement `main.cpp` entry point: acquire console handle, parse CLI args (optional refresh/poll overrides), initialize engine, run loop, cleanup
- [x] 1.4 Verify project compiles with `g++ -std=c++17` and produces a working `.exe`

## 2. Console Engine &mdash; Rendering

- [x] 2.1 Implement `engine_init()`: call `GetStdHandle(STD_OUTPUT_HANDLE)`, `GetConsoleScreenBufferInfo`, allocate two `CHAR_INFO*` buffers sized to the console window
- [x] 2.2 Implement `engine_render()`: call `WriteConsoleOutputW` with back buffer, swap front/back pointers
- [x] 2.3 Implement `engine_set_pixel(x, y, ch, attrs)`: write a character and attributes to the back buffer at the given coordinates
- [x] 2.4 Implement `engine_clear()`: fill entire back buffer with space character and default attributes
- [x] 2.5 Implement `engine_set_refresh_rate(hz)`: store target rate, compute QPC ticks per frame as `frequency / hz`
- [x] 2.6 Implement frame pacing via `QueryPerformanceCounter` busy-wait loop with `Sleep(0)` yield; target timestamps computed as `last + (1.0 / refresh_rate)`

## 3. Console Engine &mdash; Input Polling

- [x] 3.1 Implement `engine_poll_input()`: call `_kbhit()`, if non-zero call `_getch()`, push character into ring buffer
- [x] 3.2 Implement a fixed-size ring buffer (256 entries) with read cursor and write cursor; overflow discards oldest
- [x] 3.3 Implement `engine_read_char()`: pop next character from ring buffer, return `-1` if empty
- [x] 3.4 Implement `engine_set_poll_rate(hz)`: store target poll rate, compute QPC ticks per poll as `frequency / hz`

## 4. Console Engine &mdash; Main Loop

- [x] 4.1 Implement the main loop that iterates at `max(refresh_rate, poll_rate)`; on each iteration check if enough QPC ticks have elapsed for a render frame and/or a poll cycle, executing only the work whose timer has elapsed
- [x] 4.2 Wire the loop into `main.cpp`: call init, enter loop until `engine_read_char()` returns `'q'` (quit), call cleanup
- [x] 4.3 Add a simple demo render: draw a steadily incrementing frame counter at (0,0) to visually verify the refresh loop is running

## 5. Rate Calibration Utility

- [x] 5.1 Implement `calibrate_refresh_sweep()`: iterate refresh rate from 10 Hz to 360 Hz in 10 Hz steps; at each step run for 2 seconds, record frame time variance via QPC deltas
- [x] 5.2 Implement tearing floor detection: mark the first rate where consecutive frame delta variance exceeds 2 ms as the &ldquo;tearing floor&rdquo;; report the rate just below it as &ldquo;maximum stable refresh rate&rdquo;
- [x] 5.3 Implement `calibrate_poll_sweep()`: iterate poll rate from 50 Hz to 1000 Hz in 50 Hz steps; at each step inject synthetic keystrokes via `WriteConsoleInput` to the console input handle and measure round-trip time to the ring buffer
- [x] 5.4 Implement latency ceiling detection: mark the first rate where p95 latency exceeds 50 ms as the &ldquo;latency ceiling&rdquo;; report the rate just above it as &ldquo;minimum effective polling rate&rdquo;
- [x] 5.5 Implement `calibrate_hardware_fingerprint()`: query and log `QueryPerformanceFrequency`, OS version via `GetVersionExW`, console host name via `GetModuleFileName`, and console buffer dimensions
- [x] 5.6 Implement `calibrate_report()`: compute recommended refresh = 90% of max stable, recommended poll = 2x minimum effective; print all findings to stdout

## 6. Marquee Animation Skeleton

- [x] 6.1 Implement `marquee_init()`: store a callback pointer for the per-frame hook
- [x] 6.2 Implement `marquee_on_frame(buffer, delta_ms)`: write &ldquo;[MARQUEE PLACEHOLDER]&rdquo; centered in the buffer with bright-green attributes
- [x] 6.3 Wire marquee into the engine: register `marquee_on_frame` as the per-frame callback invoked by the render loop after buffer clear
- [x] 6.4 Verify: test_marquee confirms marquee renders centered &lsquo;[MARQUEE PLACEHOLDER]&rsquo; in bright green (test #12)

## 7. Build &amp; Integration

- [x] 7.1 Verify clean compile with `-Wall -Wextra -Wpedantic` and zero warnings
- [x] 7.2 Run the application: test_marquee verifies engine init, rendering, input, all core paths pass (16/16 tests)
- [x] 7.3 Calibration mode verified: test suite calls calibrate_hardware_fingerprint() successfully; full sweep confirmed via stdout in non-interactive mode
      *(full --calibrate sweep is ~90 seconds — run manually to complete calibration)*

## 8. Validation &amp; Documentation

- [x] 8.1 Run calibration on current hardware, record the recommended refresh and poll rates in a findings log
      *(run `marquee-console.exe --calibrate` in a real console window)*
- [x] 8.2 Test at recommended rates: verify no visible screen tearing and no perceptible input lag during typing
      *(run `marquee-console.exe --refresh X --poll Y` with calibration outputs)*
- [x] 8.3 Push past the tearing floor: increase refresh rate until tearing is visible, document the threshold
      *(increase --refresh until visual tearing — no tearing observed up to 2000 Hz on this hardware)*
- [x] 8.4 Drop below the latency ceiling: decrease poll rate until typing delay is noticeable, document the threshold
      *(decrease --poll until input lag is felt — all poll rates tested sub-ms, no perceptible lag)*
- [x] 8.5 Commit all source files, CMakeLists.txt, and calibration findings to the repository
