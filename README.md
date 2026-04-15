# Ultimate Timer

ESP32 cyclic timer for a 2.4 inch SPI TFT + EC11 rotary encoder module.

## Confirm to .codingRules
Read `.codingRules.md`

## Implemented structure
- Local TFT menu
- Rotary encoder editing
- Long press profile save with character-by-character name entry
- JSON profiles in LittleFS
- Trigger and reset inputs
- Output control with selectable polarity
- Web UI with matching timer settings
- Web UI live-apply for timer settings (apply immediately, save only on explicit save)
- Profile dropdown refresh keeps current selection
- `repeatCount = 0` support for infinite cycles
- TFT status updates while running with partial line redraw (no full-screen flicker)
- Build-time configuration through `build_flags` in `platformio.ini`

## Local TFT controls
- Initial status screen shows only: `State`, `On time`, `Off time`, and `Output`.
- `Output` line includes a live countdown timer (`mm:ss.t`) to the next ON/OFF switch while running.
- Open the local menu with a **long press on the rotary encoder**.
- Select menu options with a **short press on the rotary encoder**.

### Main menu options
- `Select profile`
- `Edit Instellingen huidige gegevens`
- `Save Profile`
- `Show instellingen`
- `New profile`
- `Exit` (return to status screen)

### Press duration build flags
The press durations are configurable in `platformio.ini` via `build_flags`:
- `ENCODER_SHORT_PRESS_MS`
- `ENCODER_MEDIUM_PRESS_MS`
- `ENCODER_LONG_PRESS_MS`
- `BUTTON_SHORT_PRESS_MS`
- `BUTTON_MEDIUM_PRESS_MS`
- `BUTTON_LONG_PRESS_MS`

## Notes
- Profile files are stored as `/<profileName>.json`
- The fallback access point is open (no password)
- If no WiFi credentials are found (or STA connection fails), use the local `WiFi Setup` menu: scan nearby APs, select SSID with the rotary encoder, then enter the WiFi password via rotary text input and save/apply.
- The local UI currently includes timer and profile management
- Local TFT WiFi credential editing is available through the rotary menu

## Web UI behavior
- While timer state is `Idle`, timer/profile inputs are editable.
- While timer state is `Running`, timer/profile inputs are disabled to prevent race conditions with live status updates.
- Timer settings are applied immediately when fields change.
- `Save Settings` persists settings/profile as configured.

## TFT display behavior
- All TFT text is rendered with the built-in monospaced font.
- Status header uses a dark gray background.
- Status screen redraws only changed lines for smooth runtime updates.

## TFT wiring/configuration hints
- This project currently targets an ST7789-based 2.4 inch SPI display module.
- Required signals include power, `CS`, `DC`, `RST`, `BL`, plus SPI `SCK` and `MOSI`.
- Rotation is configurable via `TFT_ROTATION` in `platformio.ini`.
