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
- Status header uses a dark gray background.
- Last status line shows active profile name.
- Status screen redraws only changed lines for smooth runtime updates.

## TFT wiring/configuration hints
- This project currently targets an ST7789-based 2.4 inch SPI display module.
- Required signals include power, `CS`, `DC`, `RST`, `BL`, plus SPI `SCK` and `MOSI`.
- Rotation is configurable via `TFT_ROTATION` in `platformio.ini`.
