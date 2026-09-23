# Ultimate Timer Project Prompt

## Source Of Truth

The C/C++ implementation and `platformio.ini` are the only source of truth for project behavior, configuration, APIs, pin assignments, timing values, persistence, and version information.

Documentation files describe intent only. When documentation conflicts with code or `platformio.ini`, inspect the implementation and build configuration, then update the documentation instead of changing working code to match documentation.

Do not infer behavior from stale comments, old version notes, or generated files. Verify behavior in the owning implementation before making changes.

## Project

- Platform: PlatformIO
- Board: `esp32dev`
- Framework: Arduino on ESP32
- Filesystem: LittleFS
- Main environment: `TFT_LCD_EC11_esp32`
- Firmware version is defined by `PROG_VERSION` in `src/main.cpp`.
- Build configuration, dependency versions, pins, and `build_flags` are defined in `platformio.ini`.

Normal build command:

```text
pio run -e TFT_LCD_EC11_esp32
```

Never run upload, flash, erase-flash, or equivalent hardware-write commands. The user performs those operations manually.

## Architecture

- `src/main.cpp` initializes the application and runs the timer and input services in separate FreeRTOS tasks.
- The timer task calls `timerUpdate()` and then `ioUpdate()`.
- The input task calls `input.update()`, backlight handling, external-input handling, and `uiMenuUpdate()`.
- `src/timerEngine.cpp` owns timer settings, runtime state, cyclic timing, 24-hour timing, phase transitions, output overrides, and timer synchronization.
- `src/ioControl.cpp` owns the physical output pin and translates runtime output state using the configured output polarity.
- `src/uiMenu.cpp` owns the local TFT menu and field-input flow.
- `src/webUi.cpp` owns the embedded web page, JavaScript controls, HTTP routes, and status JSON.
- `src/profileManager.cpp` owns JSON profile files in LittleFS.
- `src/settingsStore.cpp` owns system-level settings and last-profile state in Preferences/NVS.
- `src/InputClass.cpp` owns encoder quadrature decoding, encoder-button events, auxiliary-button events, and raw activity reporting.
- `src/DisplayDriver.cpp` owns the ST7789 display abstraction and display rendering helpers.

Preserve these ownership boundaries. A caller may request behavior through a public module API, but it must not duplicate the module's internal state logic.

## Timer Types

The firmware supports:

- Cyclic timers, using ON time, OFF time, units, cycle count, trigger mode, and trigger edge.
- 24-hour timers, using 96 quarter-hour states as the source of truth.

24-hour quarter-hour states are:

- `TIMER_24H_QUARTER_OFF` (`-`)
- `TIMER_24H_QUARTER_ON` (`+`)
- `TIMER_24H_QUARTER_RANDOM_ON` (`R`)
- `TIMER_24H_QUARTER_RANDOM_OFF` (`r`)

Mixed quarter-hour values are displayed as `S`; `S` is derived and is never stored.

24-hour runtime evaluation follows local wall-clock time. Random runs may combine consecutive matching random quarters into one transition span. The runtime engine, not the UI, determines the current state and transition timing.

24-hour profiles use the `-24h` filename suffix. Built-in default profiles are protected.

## Output Control

The physical output is controlled only through `ioUpdate()` using `RuntimeStatus.outputActive` and `AppSettings.outputPolarityHigh`.

The web Timer Screen exposes `ON` and `OFF` actions for both timer types. These actions call the timer-engine output override API. The override is temporary:

- ON forces the runtime output active.
- OFF forces the runtime output inactive.
- The next normal cyclic phase transition clears the override.
- The next normal 24-hour runtime segment transition clears the override.
- Reset, stop, and settings changes clear the override.
- The output polarity setting still applies at the physical output layer.

Do not implement a second output path in the web UI, local UI, or GPIO code. Keep override state in the timer engine so status data and physical output remain consistent.

## Persistence

Profile-scoped timer data is stored as JSON in LittleFS. This includes timer type, cyclic fields, trigger fields, and the 96 24-hour quarter-hour states.

System-scoped values are stored separately through `settingsStore`, including output polarity, input locking, auto-save behavior, encoder direction, theme color, display rotation, WiFi state, and last profile name.

Do not put system settings into profile JSON unless the existing implementation is intentionally changed as a complete persistence design.

## Input And Local UI

- Rotary encoder short press selects the current item.
- Encoder medium or long press commits or exits according to the active screen.
- `PIN_KEY0` medium or long press acts as back/commit according to the active screen.
- The local menu uses clamped navigation and skips disabled items.
- `InputClass::consumeActivity()` reports raw encoder or button activity and clears the activity flag.
- The TFT backlight is controlled by `BACKLIGHT_OFF_TIMEOUT` and wakes on input activity.

Keep event interpretation in `InputClass` and screen-specific behavior in `uiMenu.cpp`.

## Web UI And API

The web UI is embedded in `src/webUi.cpp` as the `indexHtml` string. When adding a control, update all of the following together:

1. HTML markup.
2. CSS when visual styling is needed.
3. JavaScript event binding.
4. HTTP route registration.
5. The owning C++ module API and runtime state when behavior is not purely presentational.
6. Status JSON when the new state must be displayed.

Existing timer endpoints include `/api/start`, `/api/stop`, `/api/reset`, `/api/output/on`, and `/api/output/off`. Keep route behavior aligned with the timer-engine API.

Web UI settings and profile actions must preserve the existing auto-save, active-profile, timer-type, and menu open/close behavior.

## Coding Rules

- Preserve the existing architecture and public APIs unless the requested behavior requires an API change.
- Make the smallest root-cause change that satisfies the request.
- Use two spaces for indentation and Allman braces in C/C++/JavaScript/CSS.
- Use descriptive lowerCamelCase names for new variables and functions.
- Do not use one-letter variable names.
- Keep comments short and place them above the block they describe. Match the existing `//---` comment style in source files.
- Do not reformat unrelated code.
- Do not remove existing functionality.
- Do not invent libraries, APIs, settings, or hardware behavior.
- Protect shared timer state with the existing timer mutex where required.
- Avoid blocking work in the 2 ms service tasks.
- Use the existing logging and module patterns.

## Validation

After source changes, run:

```text
pio run -e TFT_LCD_EC11_esp32
```

For behavior changes, also inspect the relevant owning code path and nearby status/API call sites. Do not claim hardware behavior was tested unless the user has tested it on the device.

Do not commit changes automatically. At completion, report changed files, validation result, and any untested hardware behavior.
