# console-engine Specification

## Purpose
TBD - created by archiving change marquee-console-app. Update Purpose after archive.
## Requirements
### Requirement: Console buffer initialization
The system SHALL initialize a console screen buffer with a double-buffered `CHAR_INFO` array matching the current console window dimensions.

#### Scenario: Buffer created with correct dimensions
- **WHEN** the engine initializes
- **THEN** the back buffer SHALL contain a `CHAR_INFO` array sized to `dwSize.X * dwSize.Y` where `dwSize` is obtained from `GetConsoleScreenBufferInfo`
- **AND** the front buffer SHALL be an identical array
- **AND** all cells SHALL default to `' '` (space) with white-on-black attributes

### Requirement: Frame rendering
The system SHALL render the back buffer to the visible console via a single `WriteConsoleOutputW` call per frame.

#### Scenario: Full buffer blit
- **WHEN** the render loop triggers a frame
- **THEN** the system SHALL call `WriteConsoleOutputW` with the back buffer `CHAR_INFO` array and the console output handle
- **AND** the system SHALL swap front/back buffer pointers after the blit completes

#### Scenario: Partial region blit
- **WHEN** only a sub-region of the buffer has changed
- **THEN** the system MAY call `WriteConsoleOutputW` with a clipped `SMALL_RECT` to improve throughput
- **AND** the clipped region SHALL still produce correct visual output

### Requirement: Tunable refresh loop
The engine SHALL run a main loop that iterates at a user-configurable refresh rate measured in Hz.

#### Scenario: Refresh rate honored
- **WHEN** refresh rate is set to `N` Hz
- **THEN** the render loop SHALL produce no more than `N` frames per second
- **AND** the actual frame rate SHALL be within 5% of `N` (measured over a 5-second window)

#### Scenario: Refresh rate reconfiguration
- **WHEN** the refresh rate is changed at runtime via `set_refresh_rate(rate)`
- **THEN** the next frame SHALL use the new rate
- **AND** the change SHALL take effect within one frame boundary

### Requirement: Non-blocking input polling
The engine SHALL check for keyboard input without blocking the render loop.

#### Scenario: Character available
- **WHEN** a key is pressed and `_kbhit()` returns non-zero
- **THEN** the system SHALL call `_getch()` to consume the character
- **AND** the character SHALL be enqueued in an input ring buffer

#### Scenario: No input available
- **WHEN** `_kbhit()` returns zero
- **THEN** the system SHALL NOT call `_getch()`
- **AND** the system SHALL return immediately without blocking

### Requirement: Tunable polling rate
The input poller SHALL execute at a user-configurable polling rate measured in Hz, independent of the refresh rate.

#### Scenario: Polling rate honored
- **WHEN** polling rate is set to `M` Hz and refresh rate is `N` Hz (M != N)
- **THEN** the poller SHALL check for input at approximately `M` checks per second
- **AND** the render loop SHALL produce `N` frames per second
- **AND** both rates SHALL be independently measurable via `QueryPerformanceCounter`

### Requirement: Input ring buffer
The system SHALL buffer all consumed input characters in a fixed-size ring buffer.

#### Scenario: Character buffered
- **WHEN** `_getch()` returns a character
- **THEN** the character SHALL be appended to the ring buffer
- **AND** the read cursor SHALL advance

#### Scenario: Buffer overflow
- **WHEN** the ring buffer is full and a new character arrives
- **THEN** the oldest unread character SHALL be discarded
- **AND** the new character SHALL be appended

### Requirement: Frame timing via QueryPerformanceCounter
The render loop SHALL use `QueryPerformanceCounter` / `QueryPerformanceFrequency` for sub-millisecond frame pacing.

#### Scenario: Accurate frame timing
- **WHEN** the render loop waits for the next frame interval
- **THEN** it SHALL compute the target timestamp as `last_frame_timestamp + (1.0 / refresh_rate)` using QPC ticks
- **AND** it SHALL busy-wait until QPC reaches the target, calling `Sleep(0)` each iteration to yield the remainder of the timeslice

