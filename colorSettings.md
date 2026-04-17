# Color Settings — ultimateTimer Display

## Hardware background

This project uses an ST7789-based 2.4-inch SPI TFT panel.
This specific panel has **hardware color inversion active (INVON)**.
Every pixel value you write is bit-complemented by the panel before it is shown.

## The PANEL_COLOR macro

```cpp
#define PANEL_COLOR(colorValue) static_cast<uint16_t>((colorValue) ^ 0xFFFFU)
```

Use this macro whenever you want a **colored fill or colored text** to appear in the color
you specify.

- Pass the **desired visual color** (RGB565 value you want to SEE on screen).
- The macro pre-inverts it.
- The panel inverts it back, so the visual result matches what you passed in.

**Example:**
```cpp
// Header background that actually looks blue on screen.
static const uint16_t headerBackgroundColor = PANEL_COLOR(0x001F);  // 0x001F = blue (RGB565)
```

## Text colors — NEVER use PANEL_COLOR for text

Because the panel inverts all values, the two Adafruit constants behave counter-intuitively:

| Constant | Written value | Panel inverts to | Appears on screen as |
|---|---|---|---|
| `ST77XX_BLACK` (0x0000) | 0x0000 | 0xFFFF | **WHITE** (use for legible text) |
| `ST77XX_WHITE` (0xFFFF) | 0xFFFF | 0x0000 | **BLACK** (invisible on dark backgrounds) |

**Rule:** Always use `ST77XX_BLACK` for text that must be readable.
Never use `ST77XX_WHITE` for text — it renders black and disappears on dark backgrounds.

## Screen background

`tft.fillScreen(ST77XX_BLACK)` produces a **black screen**.
All UI elements are drawn on top of this black background.

## Proven color constants (do not change without hardware test)

```cpp
// Header and selected item fill — visually blue
static const uint16_t selectionFillColor    = PANEL_COLOR(0x001F);

// Selected item border — visually light blue/teal
static const uint16_t selectionBorderColor  = PANEL_COLOR(0x7DFF);

// Selected item text — visually white (high-contrast on blue)
static const uint16_t selectionTextColor    = ST77XX_BLACK;

// Idle (unselected) button fill — visually light gray
static const uint16_t idleButtonFillColor   = PANEL_COLOR(0xD69A);

// Idle button border — visually lighter gray
static const uint16_t idleButtonBorderColor = PANEL_COLOR(0xBDF7);

// All button text — visually white on both blue and gray backgrounds
static const uint16_t selectedButtonTextColor = ST77XX_BLACK;
static const uint16_t idleButtonTextColor     = ST77XX_BLACK;
```

## How to add a new colored UI element

1. Pick the **desired RGB565 color** (use any RGB565 color picker tool).
2. Wrap it with `PANEL_COLOR(your_value)` for fills, borders, and colored text.
3. Use `ST77XX_BLACK` for text that must appear white/light on a colored background.
4. Never use `ST77XX_WHITE` for text — it will be invisible on dark backgrounds.
5. Test with the `TEST_COLOR_PATERN` build flag if in doubt.

## Common mistakes

| Mistake | Symptom | Fix |
|---|---|---|
| `tileValueColor = ST77XX_WHITE` | Text invisible on dark tile | Change to `ST77XX_BLACK` |
| Fill color `PANEL_COLOR(0x2104)` (near-black) | Tile invisible on black background | Use `selectionFillColor` or a clearly different hue |
| Using raw 0xRRGG for fills without `PANEL_COLOR` | Color looks wrong/inverted | Wrap with `PANEL_COLOR()` |

## Two button contexts — CRITICAL distinction

There are two distinct button contexts in this project with **opposite** fill conventions:

### 1. Navigation action buttons (Timer Screen: Start / Stop / Reset)
- These are action triggers arranged as a navigation strip.
- **Active / focused:** `getUiSelectedFillColor()` = `PANEL_COLOR(getDarkColor())` → dark shade visible on screen.
- **Inactive:** `getUiInactiveFillColor()` = `PANEL_COLOR(getLightColor())` → light shade.
- Active also gets a **double inner border** (`ST77XX_BLACK` → appears WHITE on inverted panel).

### 2. Choice / toggle buttons (Field Input: Yes/No, High/Low, color picker, etc.)
- These represent mutually exclusive options (like a hardware selector switch).
- **Selected option (current value):** `getUiInactiveFillColor()` = **LIGHT shade** — appears "lit up" / glowing.
- **Unselected options:** `getUiSelectedFillColor()` = **DARK shade** — appears dimmed / off.
- Selected also gets a **double inner border** (`tileBorderColor`) to reinforce the selection.
- Text colors are swapped accordingly: selected uses `getUiInactiveTextColor()`, unselected uses `getUiSelectedTextColor()`.

> **Why the inversion?** A physical selector switch lights up the active position. The bright/light shade visually reads as "this is the current choice". The dark shade reads as "this option is off / not chosen". If you use dark=active for choice buttons they look identically dull against each other — the light shade creates the needed "glow" distinction.

> **ST77XX note:** `ST77XX_BLACK` (0x0000) drawn without PANEL_COLOR → panel inverts → appears **WHITE**. Use it for visible inner borders on any fill.
> `ST77XX_WHITE` (0xFFFF) drawn without PANEL_COLOR → panel inverts → appears **BLACK**. Do NOT use for borders on dark fills — it is invisible.

## Button rendering — code reference

```cpp
// Choice buttons (Field Input) — light=selected, dark=inactive
uint16_t selectedFillColor   = getUiInactiveFillColor();   // LIGHT → appears "lit/active"
uint16_t selectedTextColor   = getUiInactiveTextColor();
uint16_t unselectedFillColor = getUiSelectedFillColor();   // DARK → appears "dimmed"
uint16_t unselectedTextColor = getUiSelectedTextColor();
// + double inner border on selected button for clarity

// Navigation buttons (Timer Screen) — dark=active, light=inactive
uint16_t fillActive   = getUiSelectedFillColor();           // DARK
uint16_t fillInactive = getUiInactiveFillColor();           // LIGHT
// + double inner border (ST77XX_BLACK → white) on active button
```

## Color profile rules (active project convention)

Color profiles are defined in `include/colorSettings.h` and use this format:

`ColorName: darkLevel-darkLabelColor, lightLevel-lightLabelColor`

- The first number is the dark/active variant index for that color.
- The second number is the light/inactive/background variant index for that color.
- Label color after each number defines the visual text color for that variant (`WHITE` or `BLACK`).
- `Orange` is currently marked as not used in palette workflows.

Current configured profiles:

- `Red: 1-WHITE, 6-WHITE`
- `Green: 1-BLACK, 5-BLACK`
- `Blue: 1-WHITE, 6-WHITE`
- `Indigo: 1-WHITE, 6-WHITE`
- `Violet: 1-WHITE, 7-WHITE`
- `Yellow: 1-BLACK, 5-BLACK`
- `Orange: Not used`

## Request interpretation defaults (from now on)

- If asked: `text must be red`, use the **first** (dark) variant of `Red`.
- If asked: `tile must be yellow`, use the **second** (light/background) variant of `Yellow`.
- For **navigation** action buttons: dark shade = focused/active, light shade = idle.
- For **choice** toggle buttons: light shade = current selection ("lit"), dark shade = not chosen ("off").

## Visual text mapping for this inverted panel

- Visual `WHITE` text -> use `ST77XX_BLACK`.
- Visual `BLACK` text -> use `ST77XX_WHITE`.
