[← Back to README](README.md)

# Web UI

## Overview
- The Web UI is a single-page application (SPA) served by the firmware from `src/webUi.cpp`.
- The page is delivered from `/` and communicates with firmware through JSON API endpoints.
- The top bar shows the active profile and a live clock (`HH:mm:ss`).
- If Timer Settings differ from the last saved state of the active profile, the header shows ` (not saved)` directly after the active profile name.

## Top Menu Structure
- Top-level order:
  - `System`
  - `Timer Settings` (submenu)
  - `Profiles` (submenu)
- `Timer Settings` opens a submenu with:
  - `Cyclic Timer Settings`
  - `24h Timer Settings`
- `Profiles` opens a submenu with:
  - `Load Profile`
  - `Save Profile`
  - `New Profile`
  - `Delete Profile`

## Always-Visible Tile
- The `Timer Screen` tile is always visible.
- It shows:
  - `State`
  - `On Time` and `Off Time` for cyclic timers
  - `Last State Change (ON/OFF)` and `Next State Change (OFF/ON)` for 24h timers
  - `Output` (including countdown)
  - `Cycles` (`current/INF` when repeat count is `0`) for cyclic timers only
- Action buttons:
  - `Start`
  - `Stop`
  - `Reset`
  - only visible for cyclic timers (hidden for 24h timers)

## System Tile
- Read-only fields:
  - `SSID`
  - `IP`
  - `MAC`
  - `Encoder X-Y`
- Info cards:
  - `Network`
  - `Profiles`
- Mutable fields:
  - `Output` (`Active HIGH` / `Active LOW`)
  - `Auto Save Profile` (`Yes` / `No`)
  - `Theme` (`Red`, `Green`, `Blue`, `Indigo`, `Violet`, `Yellow`)
  - `Warp Speed` (`Disabled` / `Enabled`)
  - `Restart Ultimate Timer` (`No` / `Yes`)
- Buttons:
  - `Cancel`
  - `Save`

Warp Speed behavior:
- `Warp Mode` tile in Timer Screen is only visible when Warp Speed is enabled.
- On every device startup/reboot, Warp Speed is forced to `Disabled`.

## Cyclic Timer Settings Tile
- Fields:
  - `On Time`
  - `On Time Unit`
  - `Off Time`
  - `Off Time Unit`
  - `Cycles`
  - `Timer Type`
  - `Trigger Mode`
  - `Trigger Edge`
  - `Lock Input While Running`
- Buttons:
  - `Cancel`
  - `Save Settings`

## 24h Timer Settings Tile
- Fields:
  - `Timer Type` (display only, set to 24h)
  - `Trigger Mode`
  - `Trigger Edge`
  - `Lock Input While Running`
  - quarter-hour editor table: 24 rows (`00`..`23`) × 4 quarter slots (`00-15`, `16-30`, `31-45`, `46-59`)
  - quarter state values: `-`, `+`, `R`, `r`
- Buttons:
  - `Cancel`
  - `Save Settings`
## Profile Tiles
### Save Profile
- Shows active profile.
- No editable fields.
- Buttons:
  - `Cancel`
  - `Save Profile`
- `Save Profile` saves the currently active profile name shown in the tile.
- `Save Profile` remains enabled while timer state is `Running`.

### Load Profile
- Shows active profile.
- Field:
  - `Profile Selection`
- Buttons:
  - `Cancel`
  - `Load Profile`

### New Profile
- Shows active profile.
- Field:
  - `New Profile Name`
- Buttons:
  - `Cancel`
  - `Create Profile`

### Delete Profile
- Shows active profile.
- Field:
  - `Profile Selection`
- Buttons:
  - `Cancel`
  - `Delete Profile`

## Global Interaction Rules
- Only one menu tile is visible at a time below the Timer Screen tile.
- Every popup/menu tile can show the note:
  - `Only mutable when Timer is Stopped.`
- Opening any menu tile auto-stops the timer; closing all menu tiles auto-starts the timer again.
- While any menu tile is open, form controls are editable (timer is auto-stopped).
- All `Cancel` buttons remain enabled, also while timer state is `Running`.
- In `Save Profile`, the `Save Profile` button remains enabled, also while timer state is `Running`.
- Clicking `Cancel` always closes the current tile without saving changes.
- After pressing `Save Profile`, `Load Profile`, `New Profile`, or `Delete Profile`, the tile closes automatically.
- After pressing `Save Settings`, the Timer Settings tile closes automatically.
- If `Auto Save Profile` is `No`, pressing `Save Settings` in cyclic or 24h tile shows a warning toast that changes are active but not saved to profile file.

## Theme Behavior
- Web UI accent colors follow the selected system theme.
- Theme names map to browser palettes (not required to match TFT inversion behavior exactly).
- The active theme is exposed by status JSON and applied dynamically in the browser.
- When theme color is changed through the Web UI, the local TFT display is forced back to `Timer Screen` and fully rebuilt, even if it was already showing `Timer Screen`.

## Refresh and Live Apply
- Status polling is always active and updates runtime information, including starts initiated from the device itself.
- The always-visible `Timer Screen` is always refreshed from status data.
- Profile list polling keeps profile selections up to date.
- Timer setting fields use live-apply (`/api/settings/apply`) with debounce.
- Live-apply is only used for `Timer Settings` fields, not for `System` fields.
- Explicit Save uses `/api/settings`.
- `System` changes are only applied through explicit `Save` (`/api/system/save`).
- Editable fields in an open submenu are not overwritten by status polling while the user is editing.
- Header profile text includes ` (not saved)` when current Timer Settings differ from the saved baseline of the active profile, and this suffix is cleared after successful `Save Profile`.
- Web `Cycles` display follows the same cycle-progress logic as the TFT status screen.

## Web API Endpoints
### Page
- `GET /`
  - Returns the SPA HTML.

### Status and Settings
- `GET /api/status`
  - Returns current settings, runtime, and network/system info.
- `POST /api/settings`
  - Saves timer settings and persists system-scoped fields included in the Timer Settings form.
- `POST /api/settings/apply`
  - Applies timer settings without persistence.

### System
- `POST /api/system/save`
  - Saves system mutable values (`Output`, `Auto Save Profile`, `Theme`, optional `Restart`).

## Persistence Model
- Profile files store profile-scoped timer fields:
  - `onTimeValue`
  - `offTimeValue`
  - `onTimeUnit`
  - `offTimeUnit`
  - `repeatCount`
  - `triggerMode`
  - `triggerEdge`
  - `timerType`
  - `timer24hQuarterStates` (96 values)
- System-scoped fields are stored separately in Preferences/NVS:
  - `outputPolarityHigh`
  - `lockInputDuringRun`
  - `autoSaveLastProfile`
  - `themeColorIndex`
  - encoder direction and WiFi-related settings

### Timer Actions
- `POST /api/start`
- `POST /api/stop`
- `POST /api/reset`

### Profiles
- `GET /api/profiles`
- `POST /api/profile/save`
- `POST /api/profile/load`
- `POST /api/profile/delete`

## Status JSON Fields Used by Web UI
### settings
- `timerType`
- `timerTypeLabel`
- `onTimeValue`
- `offTimeValue`
- `onTimeUnit`
- `offTimeUnit`
- `onTimeUnitLabel`
- `offTimeUnitLabel`
- `repeatCount`
- `triggerMode`
- `triggerEdge`
- `outputPolarityHigh`
- `lockInputDuringRun`
- `autoSaveLastProfile`
- `profileName`
- `themeColorIndex`
- `themeColorName`
- `encoderOrderLabel`
- `timer24hQuarterStates`

### runtime
- `state`
- `stateLabel`
- `outputActive`
- `currentCycle`
- `totalCycles`
- `currentPhaseDurationMs`
- `currentPhaseElapsedMs`
- `inOnPhase`

### network
- `connected`
- `address`
- `ssid`
- `macAddress`

[← Back to README](README.md)
