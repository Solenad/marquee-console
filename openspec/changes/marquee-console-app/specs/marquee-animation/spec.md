## ADDED Requirements

### Requirement: Animation hook (skeleton)
The marquee animation SHALL register a per-frame callback with the console engine that receives the back buffer and frame delta time.

#### Scenario: Callback registration
- **WHEN** the engine starts
- **THEN** the marquee animation SHALL register a render callback via `engine.on_frame(callback)`
- **AND** the callback SHALL be invoked once per frame with the back buffer `CHAR_INFO*` and `delta_time_ms`

### Requirement: Placeholder rendering
The skeleton marquee SHALL render static placeholder text in the center of the console buffer to verify the animation hook is operational.

#### Scenario: Static placeholder displayed
- **WHEN** the application launches with marquee animation enabled
- **THEN** the console SHALL display the text &ldquo;[MARQUEE PLACEHOLDER]&rdquo; centered horizontally and vertically
- **AND** the text SHALL use bright-green foreground (`FOREGROUND_GREEN | FOREGROUND_INTENSITY`) on black background
