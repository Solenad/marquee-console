## Why

The Windows Console API provides low-level access to the console buffer, making it ideal for real-time text rendering applications like marquees. However, naively combining rendering and input polling leads to screen tearing (when refresh outpaces vsync) or perceptible input lag (when polling starves the render loop). This change establishes a proper immediate-mode console engine in C++ that scientifically determines the optimal balance between refresh rate and polling rate for the current hardware, then validates those limits empirically.

## What Changes

- **New C++ project scaffold**: CMake-based build with MSVC toolchain targeting the Windows Console API
- **Immediate-mode console engine**: Double-buffered console rendering with a tunable refresh loop
- **Input polling subsystem**: Non-blocking `_kbhit`/`_getch` based input with precise timing instrumentation
- **Rate calibration utility**: Automated sweep across refresh (1&ndash;360 Hz) and polling (1&ndash;1000 Hz) rates to identify the tearing floor and input latency ceiling
- **Hardware fingerprint report**: Detects monitor refresh cap (via `QueryPerformanceFrequency`) and logs recommended rates
- **Marquee animation stub**: Placeholder rendering hook for the future marquee text animation

## Capabilities

### New Capabilities
- `console-engine`: Immediate-mode rendering engine with double-buffered console output, tunable refresh loop, and non-blocking input polling via the Windows Console API
- `rate-calibration`: Automated calibration utility that sweeps refresh/polling rate combinations, measures screen tearing and input latency, and outputs recommended values for the current hardware
- `marquee-animation`: Scrolling text marquee renderer that consumes the console engine&rsquo;s frame loop (implementation deferred; skeleton hook only in this change)

### Modified Capabilities
<!-- No existing specs to modify -->

## Impact

- **New directory**: `src/` containing C++ sources, headers, and CMakeLists.txt
- **New dependencies**: Windows SDK (built-in on MSVC), CMake 3.20+, no external libraries
- **Toolchain**: MSVC via `cl.exe` or MSBuild; console application subsystem (`/SUBSYSTEM:CONSOLE`)
- **No production service impact**: Standalone console application, no networking or persistent state
