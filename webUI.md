[← Back to README](README.md)

# Web UI

## Overview
- The Web UI is a single-page application (SPA) served by the firmware from `src/webUi.cpp`.
- The page is delivered from `/` and communicates with firmware through JSON API endpoints.
- The top bar shows the active profile and a live clock (`HH:mm:ss`).

## Top Menu Structure
- Top-level order:
  - `System`
  - `Timer Settings`
  - `Profiles`
- `Profiles` opens a submenu with:
  - `Load Profile`
  - `Save Profile`
  - `New Profile`
  - `Delete Profile`

## Always-Visible Tile
- The `Timer Screen` tile is always visible.
- It shows:
  - `State`
  - `On Time`
  - `Off Time`
  - `Output` (including countdown)
  - `Cycles` (`current/INF` when repeat count is `0`)
- Action buttons:
  - `Start`
  - `Stop`
  - `Reset`

## System Tile
- Read-only fields:
  - `SSID`
  - `IP`
  - `MAC`
  - `Encoder X-Y`
- Mutable fields:
  - `Output` (`Active HIGH` / `Active LOW`)
  - `Auto Save Profile` (`Yes` / `No`)
  - `Theme` (`Red`, `Green`, `Blue`, `Indigo`, `Violet`, `Yellow`)
  - `Restart Ultimate Timer` (`No` / `Yes`)
- Buttons:
  - `Cancel`
  - `Save`

## Timer Settings Tile
- Fields:
  - `On Time`
  - `On Time Unit`
  - `Off Time`
  - `Off Time Unit`
  - `Cycles`
  - `Trigger Mode`
  - `Trigger Edge`
  - `Lock Input While Running`
- Buttons:
  - `Cancel`
  - `Save Settings`

## Profile Tiles
### Save Profile
- Shows active profile.
- Field:
  - `Profile Name`
- Buttons:
  - `Cancel`
  - `Save Profile`

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
- While timer state is `Idle`, form controls are editable and the note is hidden.
- While timer state is `Running`, form controls are disabled and the note is shown in bold with a larger font.
- Clicking `Cancel` closes the current tile.
- After pressing `Save Profile`, `Load Profile`, `New Profile`, or `Delete Profile`, the tile closes automatically.
- After pressing `Save Settings`, the Timer Settings tile closes automatically.

## Theme Behavior
- Web UI accent colors follow the selected system theme.
- Theme names map to browser palettes (not required to match TFT inversion behavior exactly).
- The active theme is exposed by status JSON and applied dynamically in the browser.
- When theme color is changed through the Web UI, the local TFT display is forced back to `Timer Screen` and fully rebuilt, even if it was already showing `Timer Screen`.

## Refresh and Live Apply
- Status polling is always active and updates runtime information, including starts initiated from the device itself.
- Profile list polling keeps profile selections up to date.
- Timer setting fields use live-apply (`/api/settings/apply`) with debounce.
- Explicit Save uses `/api/settings`.
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
- Profile files store only profile-scoped timer fields:
  - `onTimeValue`
  - `offTimeValue`
  - `onTimeUnit`
  - `offTimeUnit`
  - `repeatCount`
- System-scoped fields are stored separately in Preferences/NVS:
  - `triggerMode`
  - `triggerEdge`
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
