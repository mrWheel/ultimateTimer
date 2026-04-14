# Ultimate Timer

ESP32 cyclic timer for a 2.4 inch SPI TFT + EC11 rotary encoder module.

## Confirm to .codingRules
Read .codingfRules.md

## Implemented structure
- Local TFT menu
- Rotary encoder editing
- Long press profile save with character-by-character name entry
- JSON profiles in LittleFS
- Trigger and reset inputs
- Output control with selectable polarity
- Web UI with matching timer settings
- Build-time configuration through `build_flags` in `platformio.ini`

## Notes
- Profile files are stored as `/<profileName>.json`
- The fallback access point is open (no password)
- If no WiFi credentials are found (or STA connection fails), use the local `WiFi Setup` menu: scan nearby APs, select SSID with the rotary encoder, then enter the WiFi password via rotary text input and save/apply.
- The local UI currently includes timer and profile management
- Local TFT WiFi credential editing is available through the rotary menu
