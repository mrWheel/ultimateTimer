# 24h Timer Specification

## Purpose
The 24h timer is a second timer type alongside the existing cyclic timer.

It models a mechanical 24-hour timer such as those used for holiday lighting:

- the full day is divided into 24 hours
- each hour is divided into 4 quarter-hour blocks
- each quarter-hour can be individually configured
- the current output state is derived from the current time and the configured quarter-hour pattern

The cyclic timer remains the default timer type.

## Timer Types
The application must support these timer types:

- `cyclic`
- `24h`

The selected timer type must be stored in `AppSettings` and must be visible in both the local TFT UI and the Web UI.

The main menu must offer two separate entries:

- `Cyclic Timer Settings`
- `24h Timer Settings`

## Data Model
The 24h timer must use quarter-hour values as the source of truth.

Do not store a separate hour array as the authoritative source.
An hour label is derived from the 4 quarter-hour values belonging to that hour.

Suggested storage layout:

- 24 hours × 4 quarter-hour blocks = 96 values
- index formula: `hourIndex * 4 + quarterIndex`

Quarter-hour values:

- `-` = OFF for the whole quarter-hour
- `+` = ON for the whole quarter-hour
- `R` = random ON transition inside the quarter-hour
- `r` = random OFF transition inside the quarter-hour

Hour label derivation:

- if all 4 quarter-hour values are the same, show that value for the hour
- if the 4 quarter-hour values differ, show `S`

`S` means the hour is split and the quarter-hour blocks must be shown and editable.

## Runtime Behaviour
The 24h timer must evaluate the current output state from the current wall clock time.

Requirements:

- the timer must follow local time
- output changes must be based on the configured quarter-hour pattern
- random transitions must be determined when the timer reaches the random block
- random transitions must not remain fixed across days unless that is intentionally implemented through a day-specific seed
- every day may produce a different random pattern

When a random block is active, the actual transition moment must be computed for that block when it becomes relevant.

Examples:

- if quarter 1 is `-` and quarter 2 is `R`, the output turns ON somewhere between `hh:16` and `hh:30`
- if quarter 1 is `-` and quarters 2, 3, and 4 are all `R`, the output turns ON somewhere between `hh:16` and `hh+1:00`
- if quarter 1 is `-`, quarter 2 is `R`, and quarter 3 is `r`, the output turns ON somewhere between `hh:16` and `hh:30`, then turns OFF somewhere between `hh:31` and `hh:45`

## Local TFT Editor Behaviour
The local editor presents one hour at a time using a **cursor / locked** display convention:

- `>XX<` — value is under the cursor; rotating the encoder changes it
- `[XX]` — value is locked (confirmed by a short press)

On entering the 24h timer menu, the cursor is placed at the **current clock hour**.

### Focus levels
The editor uses four sequential focus levels:

| Level | Focus | Encoder rotates | Short press |
|---|---|---|---|
| 0 | **Hour** `>HH<` with derived **Type** `[X]` | Hour 00–23 (wraps) | Lock hour → move to Type |
| 1 | **Type** `>X<` | Type: `+`, `-`, `R`, `r`, `S` (wraps) | If `S` → move to Quarter Slot; else apply to all 4 quarters and return to Hour |
| 2 | **Quarter Slot** `>00-15<` | Slot: 00-15, 16-30, 31-45, 46-59 (wraps) | Lock slot → move to Quarter Type |
| 3 | **Quarter Type** `>X<` | Type: `+`, `-`, `R`, `r` (wraps) | Apply to this quarter; return to Quarter Slot |

**Hour focus behavior:**
- The hour type is always shown as `[type]` (locked, not under cursor).
- The type is derived from the current quarter-hour values.
- If type is `+`, `-`, `R`, or `r`: only the hour is displayed, quarters are hidden.
- If type is `S`: the quarter-hour values are displayed below the hour/type, allowing immediate visibility without advancing to focus level 2.

Medium press or long press at **any** focus level commits all changes and exits the editor.  
`PIN_KEY0` medium or long press has the same effect.

### Display layout

Hour focus (type derived from quarters and displayed, quarters shown if type is `S`):
```
24h Timer
┌──────────────────────────────────┐
│ Hour              │ Type         │
│ >05<              │ [R]          │
└──────────────────────────────────┘
Turn=Select hour  Short=Lock
Hold=Save+Back  K0=Back
```

Hour focus with type `S` — quarters are visible below:
```
24h Timer
┌──────────────────────────────────┐
│ Hour              │ Type         │
│ >05<              │ [S]          │
└──────────────────────────────────┘
 [00-15] [16-30] [31-45] [46-59]
 [-]     [+]     [R]     [-]
Turn=Select hour  Short=Lock
Hold=Save+Back  K0=Back
```

Type focus (hour locked, type under cursor, quarters shown only if type is `S`):
```
24h Timer
┌──────────────────────────────────┐
│ Hour              │ Type         │
│ [05]              │ >R<          │
└──────────────────────────────────┘
Turn=Select type  Short=Set
Hold=Save+Back  K0=Back
```

Type = `S` — quarters become editable when type cursor shows `>S<`:
```
24h Timer
┌──────────────────────────────────┐
│ Hour              │ Type         │
│ [05]              │ >S<          │
└──────────────────────────────────┘
 [00-15] [16-30] [31-45] [46-59]
 [-]     [+]     [R]     [-]
Turn=Type  Short=Edit quarters
Hold=Save+Back  K0=Back
```

Quarter Slot focus (type locked as `[S]`, slot under cursor):
```
 >00-15< [16-30] [31-45] [46-59]
 [-]     [+]     [R]     [-]
Turn=Quarter  Short=Edit type
```

Quarter Type focus (slot locked, type under cursor):
```
 [00-15] [16-30] [31-45] [46-59]
 >-<     [+]     [R]     [-]
Turn=Type  Short=Set type
```

### Hour label derivation (display only)
- If all 4 quarter values are equal, show that value for the hour label.
- If the 4 quarter values differ, show `S`.

`S` is never stored directly; it is always derived from the quarter-hour values at render time.

## Persistence
The 24h timer settings must be persisted in profile JSON.

Profile data must include:

- `timerType`
- `timer24hQuarterStates` as a 96-element integer array (values 0–3)
- all existing cyclic timer fields must remain supported

### Profile Naming and File Storage
- 24h profiles are automatically stored with a "24h" suffix in the filename
- File naming: `<profileName>24h.json`
- The built-in default 24h profile is `default24h.json` (protected, cannot be deleted)

### Built-in Profile Protection
- The built-in profile `default24h` cannot be deleted from the local TFT menu or Web UI
- The built-in profile `default24h` is hidden from the Delete Profile list
- If a user deletes the active 24h profile, the firmware automatically loads `default24h`
System-level settings remain separate from profile-scoped timer data.

When a 24h profile is loaded, the runtime engine must **immediately seek to the current wall clock position** rather than starting evaluation from hour 0. This ensures the output state is correct the moment the profile is active.

## UI and API Expectations
### Local TFT UI
- The 24h Timer Settings menu is a separate menu item from Cyclic Timer Settings
- Users edit the 24-hour schedule directly on the TFT display

### Web UI
- Timer Settings menu splits into two submenu items: Cyclic Timer Settings and 24h Timer Settings
- Action buttons (Start/Stop/Reset) are hidden for 24h profiles
- The Web UI includes a full 24h quarter-hour editor (24x4 table) and can save all 96 quarter states
- Opening any Web UI menu tile auto-stops the timer; closing all menu tiles auto-starts the timer again
- If Auto Save Profile is `No`, saving settings shows a warning that runtime changes are active but not yet written to profile file
Both the local UI and Web UI must be able to distinguish timer types.

The following must be visible in status data and editable forms:

- timer type label
- current timer type
- 24h quarter-hour data

The Web UI transports and preserves the full 24h quarter-hour data in status and save/apply payloads.

## Implementation Guidance
For a correct implementation, the developer should ensure:

- the 24h timer type is a first-class setting, not a temporary UI flag
- the runtime engine can evaluate the current 24h state from local time
- the UI derives the hour label from quarter-hour values
- `S` is used only when the hour is mixed
- random transitions are computed per block and not hardcoded as static durations
- saving and loading preserve the complete 24h pattern

## Open Design Choice
The exact randomization algorithm is implementation-defined, but it must satisfy these requirements:

- deterministic for a given day if that is useful for debugging
- different enough across days to avoid repeating the exact same pattern every day
- transition moments must stay inside the quarter-hour block they belong to