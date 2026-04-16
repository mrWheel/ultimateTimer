/*** Last Changed: 2026-04-16 - 14:54 ***/
#include "displayDriver.h"
#include "appConfig.h"
#include "timerEngine.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <esp_log.h>

//--- TFT instance
static Adafruit_ST7789 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

//--- Panel color correction
//--- Based on hardware test pattern results, this panel maps colors inverted.
#define PANEL_COLOR(colorValue) static_cast<uint16_t>((colorValue) ^ 0xFFFFU)

//--- Header colors
static const uint16_t headerBackgroundColor = PANEL_COLOR(0x001F);
static const uint16_t headerTextColor = ST77XX_BLACK;

//--- UI accent colors
//--- Target visual result:
//--- Selected = blue, Not selected = light gray.
static const uint16_t selectionFillColor = PANEL_COLOR(0x001F);
static const uint16_t selectionBorderColor = PANEL_COLOR(0x7DFF);
static const uint16_t selectionTextColor = ST77XX_BLACK;
static const uint16_t selectionAccentColor = PANEL_COLOR(0x7DFF);
static const uint16_t idleButtonFillColor = PANEL_COLOR(0xD69A);
static const uint16_t idleButtonBorderColor = PANEL_COLOR(0xBDF7);

//--- Keep button text high-contrast regardless of color preset
static const uint16_t selectedButtonTextColor = ST77XX_BLACK;
static const uint16_t idleButtonTextColor = ST77XX_BLACK;

//--- Status tile colors
//--- tileFillColor: same blue as header/selection (proven on this panel).
//--- tileValueColor: ST77XX_BLACK renders as WHITE on this inverted panel.
static const uint16_t tileFillColor = PANEL_COLOR(0x001F);
static const uint16_t tileBorderColor = PANEL_COLOR(0x7DFF);
static const uint16_t tileLabelColor = PANEL_COLOR(0xBDF7);
static const uint16_t tileValueColor = ST77XX_BLACK;

//--- Status screen tile count (4 data tiles + 1 action row)
static const int statusLineCount = 6;

//--- Status screen cache
static bool statusScreenPrepared = false;
static String cachedStatusLines[statusLineCount];

//--- Draw a status tile
static void drawStatusTile(int x, int y, int w, int h, const char* label, const String& value);

//--- Draw status action buttons
static void drawStatusActionButtons(int statusActionIndex);

//--- Format remaining duration text from milliseconds
static String formatRemainingMs(uint32_t remainingMs);

//--- Invalidate status screen cache
static void invalidateStatusScreenCache();

//--- Draw screen header
static void drawHeader(const char* title);

//--- Draw centered single line text
static void drawCenteredLine(const String& lineText, int y, uint16_t textColor, uint16_t backgroundColor);

//--- Initialize display
void displayInit()
{
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);

  SPI.begin(PIN_TFT_SCL, -1, PIN_TFT_SDA, PIN_TFT_CS);

  tft.init(TFT_HEIGHT, TFT_WIDTH);
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setFont(nullptr);
  invalidateStatusScreenCache();

  ESP_LOGI("displayDriver", "Display initialized");

} //   displayInit()

//--- Update status screen
void displayDrawStatusScreen(const AppSettings& settings, const RuntimeStatus& runtimeStatus, bool wifiConnected, int statusActionIndex)
{
  (void)wifiConnected;

  const int tileGap = 3;
  const int tileH = 36;
  const int tileStartY = 27;
  const int col1X = 3;
  const int col1W = 155;
  const int col2X = 161;
  const int col2W = 156;
  const int fullW = 314;

  if (!statusScreenPrepared)
  {
    tft.fillScreen(ST77XX_BLACK);
    drawHeader("Universal Timer");

    for (int lineIndex = 0; lineIndex < statusLineCount; lineIndex++)
    {
      cachedStatusLines[lineIndex] = "";
    }

    statusScreenPrepared = true;
  }

  String stateValue = timerGetStateLabel(runtimeStatus.state);
  String profileValue = settings.profileName;
  String nextStatusLines[statusLineCount];

  if (profileValue.isEmpty())
  {
    profileValue = "-";
  }

  nextStatusLines[0] = stateValue + String("|") + profileValue;
  nextStatusLines[1] = String(settings.onTimeValue) + " " + String(timerGetTimeUnitLabel(settings.onTimeUnit));
  nextStatusLines[2] = String(settings.offTimeValue) + " " + String(timerGetTimeUnitLabel(settings.offTimeUnit));

  uint32_t remainingMs = 0;

  if (runtimeStatus.currentPhaseDurationMs > runtimeStatus.currentPhaseElapsedMs)
  {
    remainingMs = runtimeStatus.currentPhaseDurationMs - runtimeStatus.currentPhaseElapsedMs;
  }

  String outputValue = runtimeStatus.outputActive ? "ON" : "OFF";

  if (runtimeStatus.state == TIMER_STATE_RUNNING || runtimeStatus.state == TIMER_STATE_PAUSED)
  {
    outputValue += "  ";
    outputValue += formatRemainingMs(remainingMs);
  }
  else
  {
    outputValue += "  ---:--";
  }

  nextStatusLines[3] = outputValue;

  uint32_t displayCycle = runtimeStatus.currentCycle;

  if (runtimeStatus.state == TIMER_STATE_RUNNING || runtimeStatus.state == TIMER_STATE_PAUSED)
  {
    if (runtimeStatus.totalCycles == 0 || displayCycle < runtimeStatus.totalCycles)
    {
      displayCycle++;
    }
  }
  else if (runtimeStatus.state == TIMER_STATE_FINISHED)
  {
    if (runtimeStatus.totalCycles > 0)
    {
      displayCycle = runtimeStatus.totalCycles;
    }
  }

  if (runtimeStatus.totalCycles == 0)
  {
    nextStatusLines[4] = String(displayCycle) + " / Inv.";
  }
  else
  {
    nextStatusLines[4] = String(displayCycle) + "/" + String(runtimeStatus.totalCycles);
  }

  nextStatusLines[5] = String("A:") + String(statusActionIndex);

  for (int lineIndex = 0; lineIndex < statusLineCount; lineIndex++)
  {
    if (cachedStatusLines[lineIndex] == nextStatusLines[lineIndex])
    {
      continue;
    }

    switch (lineIndex)
    {
    case 0:
    {
      int16_t textX1;
      int16_t textY1;
      uint16_t textWidth;
      uint16_t textHeight;
      int profileX;

      drawStatusTile(col1X, tileStartY, fullW, tileH, "STATE", stateValue);

      tft.setTextSize(2);
      tft.getTextBounds(profileValue, 0, 0, &textX1, &textY1, &textWidth, &textHeight);
      profileX = col1X + fullW - static_cast<int>(textWidth) - 6;

      if (profileX < col1X + 100)
      {
        profileX = col1X + 100;
      }

      tft.setTextColor(ST77XX_BLACK, tileFillColor);
      tft.setCursor(profileX, tileStartY + 14);
      tft.print(profileValue);
      break;
    }

    case 1:
      drawStatusTile(col1X, tileStartY + tileH + tileGap, col1W, tileH, "ON TIME", nextStatusLines[1]);
      break;

    case 2:
      drawStatusTile(col2X, tileStartY + tileH + tileGap, col2W, tileH, "OFF TIME", nextStatusLines[2]);
      break;

    case 3:
      drawStatusTile(col1X, tileStartY + 2 * (tileH + tileGap), fullW, tileH, "OUTPUT", nextStatusLines[3]);
      break;

    case 4:
      drawStatusTile(col1X, tileStartY + 3 * (tileH + tileGap), fullW, tileH, "CYCLES", nextStatusLines[4]);
      break;

    case 5:
      drawStatusActionButtons(statusActionIndex);
      break;
    }

    cachedStatusLines[lineIndex] = nextStatusLines[lineIndex];
  }

} //   displayDrawStatusScreen()

//--- Draw text list
void displayDrawListScreen(const char* title, const String items[], size_t itemCount, int selectedIndex, int firstVisibleIndex)
{
  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader(title);

  tft.setTextSize(2);

  const int visibleLines = 9;

  for (int line = 0; line < visibleLines; line++)
  {
    int itemIndex = firstVisibleIndex + line;

    if (itemIndex >= static_cast<int>(itemCount))
    {
      break;
    }

    int y = 30 + (line * 22);

    if (itemIndex == selectedIndex)
    {
      tft.fillRect(0, y, tft.width(), 22, selectionFillColor);
      tft.setTextColor(selectionTextColor, selectionFillColor);
      tft.setCursor(6, y);
      tft.print("> ");
      tft.print(items[itemIndex]);
    }
    else
    {
      tft.fillRect(0, y, tft.width(), 22, ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft.setCursor(6, y);
      tft.print(items[itemIndex]);
    }
  }

} //   displayDrawListScreen()

//--- Draw numeric editor
void displayDrawNumberEditor(const char* label, uint32_t value, const char* unitLabel)
{
  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader("Edit Value");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  tft.setCursor(6, 60);
  tft.print(label);

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(4);
  tft.setCursor(6, 105);
  tft.printf("%lu", static_cast<unsigned long>(value));

  tft.setTextColor(selectionAccentColor);
  tft.setTextSize(2);
  tft.setCursor(6, 170);
  tft.print(unitLabel);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(6, 205);
  tft.print("Press=OK  K0=Back");

} //   displayDrawNumberEditor()

//--- Draw enum editor
void displayDrawEnumEditor(const char* label, const char* valueLabel)
{
  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader("Edit Option");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  tft.setCursor(6, 60);
  tft.print(label);

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(4);
  tft.setCursor(6, 110);
  tft.print(valueLabel);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(6, 205);
  tft.print("Press=OK  K0=Back");

} //   displayDrawEnumEditor()

//--- Draw text input editor
void displayDrawTextInput(const char* title, const String& textValue, const String& currentToken)
{
  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader(title);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  tft.setCursor(6, 45);
  tft.print("Name:");

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(6, 75);
  tft.print(textValue);

  tft.setTextColor(selectionAccentColor);
  tft.setTextSize(3);
  tft.setCursor(6, 140);
  tft.print("[");
  tft.print(currentToken);
  tft.print("]");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(6, 205);
  tft.print("Hold=Save  K0=Back");

} //   displayDrawTextInput()

//--- Draw generic field input screen
void displayDrawFieldInput(const char* title, const char* fieldName, const String& fieldValue, int cursorIndex, const String& prevToken, const String& currentToken, const String& nextToken)
{
  //--- Per colorSettings.md: PANEL_COLOR() for fills/borders, ST77XX_BLACK for readable text.
  static const uint16_t tokenCurrentColor = PANEL_COLOR(0xF800);
  static const uint16_t tokenNeighborColor = PANEL_COLOR(0xC618);

  const int tileX = 3;
  const int tileW = 314;

  //--- Tile 1: field name + value, cursor arrows (higher and roomier)
  const int tile1Y = 30;
  const int tile1H = 74;

  //--- Tile 2: token selector fixed near bottom
  const int tile2H = 36;
  const int tile2Y = 164;

  String valueText = String("[") + fieldValue + String("]");
  String tokenText = prevToken + String(" < ") + currentToken + String(" > ") + nextToken;
  int16_t textX1;
  int16_t textY1;
  uint16_t textW;
  uint16_t textH;
  int valueX;
  int tokenX;
  int cursorBaseX;

  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader(title);

  //--- Tile 1: value input
  tft.fillRoundRect(tileX, tile1Y, tileW, tile1H, 5, tileFillColor);
  tft.drawRoundRect(tileX, tile1Y, tileW, tile1H, 5, tileBorderColor);

  tft.setTextSize(1);
  tft.setTextColor(tileLabelColor, tileFillColor);
  tft.setCursor(tileX + 4, tile1Y + 3);
  tft.print(fieldName);

  tft.setTextSize(2);
  tft.getTextBounds(valueText, 0, 0, &textX1, &textY1, &textW, &textH);
  valueX = tileX + ((tileW - static_cast<int>(textW)) / 2);

  if (valueX < tileX + 4)
  {
    valueX = tileX + 4;
  }

  cursorBaseX = valueX + 12;

  if (cursorIndex >= 0)
  {
    int cursorX = cursorBaseX + (cursorIndex * 12);
    tft.setTextColor(ST77XX_BLACK, tileFillColor);
    tft.setCursor(cursorX, tile1Y + 18);
    tft.print("v");
  }

  tft.setTextColor(ST77XX_BLACK, tileFillColor);
  tft.setCursor(valueX, tile1Y + 35);
  tft.print(valueText);

  if (cursorIndex >= 0)
  {
    int cursorX = cursorBaseX + (cursorIndex * 12);
    tft.setTextColor(ST77XX_BLACK, tileFillColor);
    tft.setCursor(cursorX, tile1Y + 54);
    tft.print("^");
  }

  //--- Tile 2: token selector
  tft.fillRoundRect(tileX, tile2Y, tileW, tile2H, 5, tileFillColor);
  tft.drawRoundRect(tileX, tile2Y, tileW, tile2H, 5, tileBorderColor);

  tft.setTextSize(1);
  tft.setTextColor(tileLabelColor, tileFillColor);
  tft.setCursor(tileX + 4, tile2Y + 3);
  tft.print("SELECT");

  tft.setTextSize(2);
  tft.getTextBounds(tokenText, 0, 0, &textX1, &textY1, &textW, &textH);
  tokenX = tileX + ((tileW - static_cast<int>(textW)) / 2);

  if (tokenX < tileX + 4)
  {
    tokenX = tileX + 4;
  }

  tft.setCursor(tokenX, tile2Y + 14);
  tft.setTextColor(tokenNeighborColor, tileFillColor);
  tft.print(prevToken + String(" < "));
  tft.setTextColor(tokenCurrentColor, tileFillColor);
  tft.print(currentToken);
  tft.setTextColor(tokenNeighborColor, tileFillColor);
  tft.print(String(" > ") + nextToken);

  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(6, 206);
  tft.print("Short=Next  Turn=Change");

} //   displayDrawFieldInput()

//--- Draw message screen
void displayDrawMessage(const char* title, const char* message)
{
  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader(title);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(6, 60);
  tft.print(message);

} //   displayDrawMessage()

//--- Draw centered WiFi portal info screen
void displayDrawWifiPortalScreen(const String& line1, const String& line2, const String& line3, const String& line4)
{
  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader("WiFi Manager Started");

  tft.setTextSize(2);

  drawCenteredLine(line1, 62, ST77XX_WHITE, ST77XX_BLACK);
  drawCenteredLine(line2, 92, ST77XX_GREEN, ST77XX_BLACK);
  drawCenteredLine(line3, 122, ST77XX_GREEN, ST77XX_BLACK);
  drawCenteredLine(line4, 186, selectionAccentColor, ST77XX_BLACK);

} //   displayDrawWifiPortalScreen()

//--- Draw centered startup connection screen
void displayDrawStartupConnectionScreen(const String& line1, const String& line2)
{
  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader("Universal Timer");

  tft.setTextSize(2);
  drawCenteredLine(line1, 86, ST77XX_WHITE, ST77XX_BLACK);
  drawCenteredLine(line2, 118, ST77XX_GREEN, ST77XX_BLACK);

} //   displayDrawStartupConnectionScreen()

//--- Draw color palette test pattern
void displayDrawTestColorPattern()
{
  struct PaletteCell
  {
    const char* label;
    uint16_t color;
  };

  const PaletteCell cells[] =
      {
          {"Sel Fill", selectionFillColor},
          {"Sel Border", selectionBorderColor},
          {"Idle Fill", idleButtonFillColor},
          {"Idle Border", idleButtonBorderColor},
          {"Blue", ST77XX_BLUE},
          {"Yellow", ST77XX_YELLOW},
          {"Gray", 0x8410},
          {"White", ST77XX_WHITE}};

  const int cols = 2;
  const int cellW = 152;
  const int cellH = 42;
  const int x0 = 8;
  const int y0 = 32;

  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader("Color Pattern");
  tft.setTextSize(2);

  for (int index = 0; index < 8; index++)
  {
    int col = index % cols;
    int row = index / cols;
    int x = x0 + (col * (cellW + 8));
    int y = y0 + (row * (cellH + 8));

    tft.fillRoundRect(x, y, cellW, cellH, 6, cells[index].color);
    tft.drawRoundRect(x, y, cellW, cellH, 6, ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK, cells[index].color);
    tft.setCursor(x + 8, y + 12);
    tft.print(cells[index].label);
  }

} //   displayDrawTestColorPattern()

//--- Set display backlight state
void displaySetBacklight(bool enabled)
{
  digitalWrite(PIN_TFT_BL, enabled ? HIGH : LOW);

} //   displaySetBacklight()

//--- Draw screen header
static void drawHeader(const char* title)
{
  tft.fillRect(0, 0, tft.width(), 24, headerBackgroundColor);
  tft.setCursor(6, 4);
  tft.setTextColor(headerTextColor, headerBackgroundColor);
  tft.setTextSize(2);
  tft.print(title);
  tft.setCursor(7, 4);
  tft.print(title);

} //   drawHeader()

//--- Draw centered single line text
static void drawCenteredLine(const String& lineText, int y, uint16_t textColor, uint16_t backgroundColor)
{
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
  int cursorX;

  tft.getTextBounds(lineText, 0, 0, &x1, &y1, &width, &height);
  cursorX = (tft.width() - static_cast<int>(width)) / 2;

  if (cursorX < 0)
  {
    cursorX = 0;
  }

  tft.setTextColor(textColor, backgroundColor);
  tft.setCursor(cursorX, y);
  tft.print(lineText);

} //   drawCenteredLine()

//--- Draw a status tile
static void drawStatusTile(int x, int y, int w, int h, const char* label, const String& value)
{
  tft.fillRoundRect(x, y, w, h, 5, tileFillColor);
  tft.drawRoundRect(x, y, w, h, 5, tileBorderColor);

  tft.setTextSize(1);
  tft.setTextColor(tileLabelColor, tileFillColor);
  tft.setCursor(x + 4, y + 3);
  tft.print(label);

  tft.setTextSize(2);
  tft.setTextColor(tileValueColor, tileFillColor);
  tft.setCursor(x + 4, y + 14);
  tft.print(value);

} //   drawStatusTile()

//--- Draw status action buttons
static void drawStatusActionButtons(int statusActionIndex)
{
  static const char* labels[] =
      {
          "Start",
          "Stop",
          "Reset"};

  const int buttonY = 194;
  const int buttonHeight = 30;
  const int buttonWidth = 92;
  const int buttonSpacing = 10;
  const int firstButtonX = 11;
  tft.fillRect(0, 182, tft.width(), 58, ST77XX_BLACK);
  tft.setTextSize(2);

  for (int buttonIndex = 0; buttonIndex < 3; buttonIndex++)
  {
    int buttonX = firstButtonX + (buttonIndex * (buttonWidth + buttonSpacing));
    bool isSelected = (buttonIndex == statusActionIndex);
    uint16_t fillColor = isSelected ? selectionFillColor : idleButtonFillColor;
    uint16_t borderColor = isSelected ? selectionBorderColor : idleButtonBorderColor;
    uint16_t textColor = isSelected ? selectedButtonTextColor : idleButtonTextColor;
    int16_t textX1;
    int16_t textY1;
    uint16_t textWidth;
    uint16_t textHeight;
    int textX;
    int textY;

    tft.fillRoundRect(buttonX, buttonY, buttonWidth, buttonHeight, 8, fillColor);
    tft.drawRoundRect(buttonX, buttonY, buttonWidth, buttonHeight, 8, borderColor);

    if (isSelected)
    {
      tft.drawRoundRect(buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2, 8, ST77XX_WHITE);
    }

    tft.getTextBounds(labels[buttonIndex], 0, 0, &textX1, &textY1, &textWidth, &textHeight);
    textX = buttonX + ((buttonWidth - static_cast<int>(textWidth)) / 2);
    textY = buttonY + ((buttonHeight - static_cast<int>(textHeight)) / 2);

    tft.setTextColor(textColor, fillColor);
    tft.setCursor(textX, textY);
    tft.print(labels[buttonIndex]);
  }

} //   drawStatusActionButtons()

//--- Invalidate status screen cache
static void invalidateStatusScreenCache()
{
  statusScreenPrepared = false;

} //   invalidateStatusScreenCache()

//--- Format remaining duration text from milliseconds
static String formatRemainingMs(uint32_t remainingMs)
{
  uint32_t totalSeconds = remainingMs / 1000UL;
  uint32_t minutes = totalSeconds / 60UL;
  uint32_t seconds = totalSeconds % 60UL;
  char buffer[16];

  if (minutes > 999UL)
  {
    minutes = 999UL;
  }

  snprintf(buffer, sizeof(buffer), "%03lu:%02lu", static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));

  return String(buffer);

} //   formatRemainingMs()
