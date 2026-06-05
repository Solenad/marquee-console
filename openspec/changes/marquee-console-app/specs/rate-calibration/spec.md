## ADDED Requirements

### Requirement: Refresh rate sweep
The calibration utility SHALL sweep the refresh rate across a configurable range to measure rendering stability.

#### Scenario: Full sweep execution
- **WHEN** calibration starts with refresh range 1&ndash;360 Hz and a fixed polling rate
- **THEN** the utility SHALL iterate through each refresh rate step (step size default 10 Hz)
- **AND** at each step it SHALL run a measurement window of at least 2 seconds
- **AND** it SHALL record the measured frame rate and frame time variance

#### Scenario: Tearing detection
- **WHEN** frame time variance exceeds 2 ms between consecutive frames
- **THEN** the utility SHALL flag the current refresh rate as above the &ldquo;tearing floor&rdquo;
- **AND** the last stable rate SHALL be recorded as the &ldquo;maximum stable refresh rate&rdquo;

### Requirement: Polling rate sweep
The calibration utility SHALL sweep the polling rate independently to measure input latency.

#### Scenario: Full polling sweep
- **WHEN** calibration starts with polling range 1&ndash;1000 Hz and a fixed refresh rate
- **THEN** the utility SHALL iterate through each polling rate step (step size default 50 Hz)
- **AND** at each step it SHALL simulate synthetic keystrokes (via `WriteFile` to console input handle) at random intervals
- **AND** it SHALL measure the time from keystroke injection to the character appearing in the engine&rsquo;s input ring buffer

#### Scenario: Latency ceiling detection
- **WHEN** measured input latency exceeds 50 ms at the 95th percentile
- **THEN** the utility SHALL flag the current polling rate as below the &ldquo;minimum acceptable polling rate&rdquo;
- **AND** the last responsive rate SHALL be recorded as the &ldquo;minimum effective polling rate&rdquo;

### Requirement: Hardware fingerprint
The calibration utility SHALL record hardware characteristics relevant to rendering and input performance.

#### Scenario: Hardware report generation
- **WHEN** calibration completes
- **THEN** the utility SHALL output the following in a human-readable report:
  - `QueryPerformanceFrequency` value
  - OS version (from `GetVersionExW`)
  - Console host executable name (from `GetModuleFileName` on the parent process)
  - Console window buffer dimensions
  - Measured maximum stable refresh rate
  - Measured minimum effective polling rate
  - Recommended refresh rate (90% of maximum stable)
  - Recommended polling rate (2x the minimum effective)

### Requirement: Recommended values output
The calibration utility SHALL compute and display recommended settings for the current hardware.

#### Scenario: Recommendation printed to stdout
- **WHEN** calibration finishes
- **THEN** the utility SHALL print a summary to stdout
- **AND** the summary SHALL include: `Recommended Refresh: X Hz`, `Recommended Poll: Y Hz`, `Tearing Floor: Z Hz`, `Latency Ceiling: W Hz`
