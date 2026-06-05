## Context

This change builds a C++ immediate-mode console engine for the Windows Console API (conhost/Windows Terminal). The console buffer model (`CHAR_INFO` via `WriteConsoleOutputW`) differs from modern framebuffers: there is no hardware vsync, no implicit double-buffering, and input arrives through synchronous CRT calls (`_kbhit`/`_getch`). The design must compensate for these limitations to achieve a tear-free, responsive rendering loop.

Current state: No existing C++ codebase. This is greenfield development with CMake + MSVC on Windows 10/11.

Stakeholders: End users interacting with the console application in real time. No server-side or multi-user concerns.

## Goals / Non-Goals

**Goals:**
- Establish a reusable immediate-mode console engine with a decoupled render loop and input poller
- Achieve a scientifically determined balance between refresh rate (visual smoothness) and polling rate (input responsiveness)
- Provide a calibration utility that fingerprints the current hardware and recommends rate values
- Document the measurable limits (tearing floor, input latency ceiling) for common Windows console hosts

**Non-Goals:**
- Marquee animation logic (deferred to a follow-up change; skeleton hook only here)
- Cross-platform support (Windows Console API only)
- GPU-accelerated rendering or DirectX interop
- Network distribution or multi-process communication

## Decisions

### Decision 1: Double-buffered `WriteConsoleOutputW` instead of per-character `SetConsoleCursorPosition`
- **Chosen**: Allocate two `CHAR_INFO` buffers; render off-screen, then blit via a single `WriteConsoleOutputW` call.
- **Alternatives considered**: Individual `SetConsoleCursorPosition` + `WriteConsoleOutputCharacter` calls per cell (causes visible flicker and is ~10x slower in benchmarking).
- **Rationale**: A single blit eliminates flicker and provides a natural frame boundary for the refresh loop.

### Decision 2: `QueryPerformanceCounter` (QPC) for frame timing instead of `Sleep` or `WaitForSingleObject`
- **Chosen**: Spin-wait with `QPC` busy-polling for precise frame pacing.
- **Alternatives considered**: `Sleep(1)` resolution is ~15 ms on Windows, far too coarse for sub-millisecond frame timing. `CreateWaitableTimer` has ~500 us resolution but adds kernel transition overhead.
- **Rationale**: QPC provides sub-microsecond resolution. A busy-wait loop that yields via `Sleep(0)` when ahead of schedule keeps CPU usage reasonable while maintaining precise frame pacing.

### Decision 3: `_kbhit`/`_getch` for input with a dedicated poll slot instead of `ReadConsoleInput` or `GetKeyState`
- **Chosen**: `_kbhit` (non-blocking check) + `_getch` (read if available) called once per poll cycle.
- **Alternatives considered**: `ReadConsoleInput` provides richer events but requires a handle and more ceremony. `GetKeyState` is prone to phantom key states and race conditions.
- **Rationale**: `_kbhit` is the simplest non-blocking check available in the CRT. Paired with `_getch`, it provides character-by-character input with minimal overhead. The poll rate is controlled by the polling loop iteration frequency.

### Decision 4: Separated refresh and polling rates instead of a unified tick rate
- **Chosen**: Two independent counters govern the render loop (refresh rate in Hz) and the input check loop (polling rate in Hz). The main loop iterates at the higher of the two rates and skips work when a counter has not elapsed.
- **Alternatives considered**: Single tick rate where input is checked every N frames (couples responsiveness to visual smoothness).
- **Rationale**: Independent rates allow the calibration utility to sweep each axis independently, finding the Pareto-optimal combination.

### Decision 5: Calibration via empirical sweep instead of static presets
- **Chosen**: The calibration utility iterates refresh rate from 1&ndash;360 Hz, polling rate from 1&ndash;1000 Hz, and measures:
  - **Tearing metric**: Delta between consecutive frame blit timestamps. High variance indicates tearing.
  - **Latency metric**: Time from keypress to character appearing in consumed input queue.
- **Alternatives considered**: Reading monitor EDID or `GetDeviceCaps` for fixed recommended rates (does not account for software rendering pipeline latency).
- **Rationale**: Empirical measurement accounts for the full software stack (CRT, conhost, terminal dispatch) and adapts to the specific hardware.

## Risks / Trade-offs

| Risk | Mitigation |
|------|-----------|
| Busy-wait loop consumes 100% CPU at high refresh rates | `Sleep(0)` yield when ahead of schedule; rate caps prevent runaway |
| `WriteConsoleOutputW` performance degrades with large console buffers | Limit rendered region to visible viewport; benchmark early |
| `_kbhit` may miss rapid key sequences at low poll rates | Independent poll rate can be set high even with low refresh rate |
| Different console hosts (conhost vs Windows Terminal) have different tearing characteristics | Calibration utility runs on the actual host; expose raw metrics for comparison |
| QPC frequency differs across hardware | `QueryPerformanceFrequency` result is logged in the hardware fingerprint |

## Migration Plan

No migration needed (greenfield project). All artifacts are new files.

## Open Questions

- What is the practical upper bound for `WriteConsoleOutputW` throughput on modern Windows 10/11 conhost? Initial estimate: ~200 Hz for an 80&times;24 buffer. Calibration will confirm.
- Should the calibration utility save results to a config file for reuse across sessions? Probably yes, but deferred to follow-up.
