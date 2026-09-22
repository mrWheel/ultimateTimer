# Ultimate Timer

Ultimate Timer is an ESP32-based timer controller for projects that need reliable, repeatable control of an external output. The firmware is designed for an ESP32 Dev Module connected to a 2.4 inch SPI TFT display and an EC11 rotary encoder module. A local interface is available on the display, while a browser-based Web UI provides a convenient way to configure the timer over WiFi.

## Timer modes

The firmware supports two different timer models. A cyclic timer alternates between configurable ON and OFF phases and can run for a fixed number of cycles or indefinitely. The phase duration can be expressed in milliseconds, seconds, or minutes. Trigger mode, trigger edge, output polarity, and input locking can be configured for the application.

The 24-hour timer is intended for schedules that repeat every day. The day is divided into 96 quarter-hour blocks. Each block can be OFF, ON, or use a random transition into ON or OFF. The runtime evaluates the schedule against the current local wall-clock time, including the transition window shown by the status screen. A 24-hour profile is only allowed to run when the system clock contains a valid date and time; after time synchronisation it starts automatically at the correct position in the schedule.

## User interfaces

The TFT interface is operated with the rotary encoder and its push button. A long press opens the local menu and a short press selects an item. The menu includes separate editors for cyclic and 24-hour timers, profile management, system settings, WiFi setup, and display settings. The status screen shows the active profile, timer state, output state, phase or schedule information, and the remaining time where applicable.

The Web UI exposes the same core settings through a browser. It includes dedicated cyclic and 24-hour timer editors, a complete 24 by 4 quarter-hour editor for daily schedules, profile load/save/delete operations, system settings, and timer status. Settings can be applied immediately while profile persistence remains an explicit action. The UI also indicates when the active settings differ from the saved profile.

## Profiles and persistence

Timer profiles are stored as JSON files in LittleFS. Cyclic profiles use the profile name as their filename; 24-hour profiles use the `-24h` suffix. The active profile name is stored in ESP32 Preferences (NVS), so the selected profile can be restored after a restart. On startup the firmware loads that profile, restores system-level settings, and places the timer in the appropriate state: cyclic timers remain stopped, while a 24-hour timer starts when valid time is available.

Profile data contains the timer type, cyclic timing values, repeat count, trigger settings, and all 96 quarter-hour values for a 24-hour schedule. System settings such as output polarity, encoder direction, theme, display rotation, WiFi state, and auto-save behavior are stored separately. The built-in default profiles are protected from deletion, and deleting the active profile causes the matching default profile to be loaded.

## Hardware and software structure

The project is built with PlatformIO, the Arduino framework for ESP32, LittleFS, Preferences, ArduinoJson, WiFiManager, and the Adafruit graphics/display libraries. The source is separated into timer engine, profile manager, settings storage, input handling, display driver, I/O control, WiFi management, local menu, and Web UI modules. Build-time options in `platformio.ini` define the GPIO assignments, display dimensions, press durations, defaults, logging level, and optional color test mode.

## Safety

This firmware can control hardware connected to dangerous voltages, high temperatures, motors, heaters, lamps, or other hazardous equipment. The repository does not provide electrical safety certification or a guarantee that a particular circuit is safe. Use suitable isolation, fusing, enclosures, grounding, strain relief, and independent hardware safety limits. Only qualified users should build or connect the hardware, and the firmware must not be treated as the sole safety mechanism.
