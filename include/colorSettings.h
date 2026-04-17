/*** Last Changed: 2026-04-17 - 10:25 ***/
#ifndef COLOR_SETTINGS_H
#define COLOR_SETTINGS_H

#include <Arduino.h>

//--- Visual text color selection
//--- Visual white maps to ST77XX_BLACK on this inverted panel.
//--- Visual black maps to ST77XX_WHITE on this inverted panel.
enum VisualTextColor
{
  VISUAL_TEXT_WHITE = 0,
  VISUAL_TEXT_BLACK = 1
};

//--- Color profile settings for test palette and UI theming rules
struct ColorProfile
{
  const char* colorName;
  uint16_t darkVisualColor;
  uint16_t lightVisualColor;
  uint8_t darkLevel;
  VisualTextColor darkLabelColor;
  uint8_t lightLevel;
  VisualTextColor lightLabelColor;
  bool usedInPalette;
};

//--- Shared color profiles
//--- Rule mapping format: ColorName: darkLevel-darkLabelColor, lightLevel-lightLabelColor
static const ColorProfile colorProfiles[] =
    {
        {"Red", 0xF800, 0xFBEF, 1, VISUAL_TEXT_WHITE, 6, VISUAL_TEXT_WHITE, true},
        {"Green", 0x07E0, 0x87F0, 1, VISUAL_TEXT_BLACK, 5, VISUAL_TEXT_BLACK, true},
        //  {"Blue", 0x001F, 0x7DFF, 1, VISUAL_TEXT_WHITE, 6, VISUAL_TEXT_WHITE, true},
        {"Blue", 0x21BF, 0x7DFF, 3, VISUAL_TEXT_WHITE, 6, VISUAL_TEXT_WHITE, true},
        {"Indigo", 0x4810, 0x8A3F, 1, VISUAL_TEXT_WHITE, 6, VISUAL_TEXT_WHITE, true},
        {"Violet", 0x780F, 0xC99F, 1, VISUAL_TEXT_WHITE, 7, VISUAL_TEXT_WHITE, true},
        {"Yellow", 0xFFE0, 0xFFF0, 1, VISUAL_TEXT_BLACK, 5, VISUAL_TEXT_BLACK, true},
        {"Orange", 0xFD20, 0xFEC0, 0, VISUAL_TEXT_WHITE, 0, VISUAL_TEXT_WHITE, false}};

static const int colorProfileCount = static_cast<int>(sizeof(colorProfiles) / sizeof(colorProfiles[0]));

#endif
