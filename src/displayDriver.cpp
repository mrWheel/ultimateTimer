/*** Last Changed: 2026-04-17 - 14:28 ***/
#include "displayDriver.h"
#include "appConfig.h"
#include "colorSettings.h"
#include "timerEngine.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <array>
#include <esp_log.h>
#include <string>
#include <string.h>

//--- TFT instance
static Adafruit_ST7789 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

//--- Panel color correction
//--- Based on hardware test pattern results, this panel maps colors inverted.
#define PANEL_COLOR(colorValue) static_cast<uint16_t>((colorValue) ^ 0xFFFFU)

//--- Active UI color index (0-based index into colorProfiles[])
static int activeUiColorIndex = 2;

//--- Status screen tile count (4 data tiles + 1 action row)
static const int statusLineCount = 6;

//--- Minimum phase duration to show countdown in status output
static const uint32_t minimumCountdownDisplayMs = 2000UL;

//--- Status screen cache
static bool statusScreenPrepared = false;
static std::array<std::string, statusLineCount> cachedStatusLines;

//--- Draw a status tile
static void drawStatusTile(int x, int y, int w, int h, const char* label, const std::string& value);

//--- Draw status action buttons
static void drawStatusActionButtons(int statusActionIndex);

//--- Format remaining duration text from milliseconds
static std::string formatRemainingMs(uint32_t remainingMs);

//--- Invalidate status screen cache
static void invalidateStatusScreenCache();

//--- Draw screen header
static void drawHeader(const char* title);

//--- Draw centered single line text
static void drawCenteredLine(const std::string& lineText, int y, uint16_t textColor, uint16_t backgroundColor);

//--- Blend two RGB565 colors by 8-bit factor
static uint16_t blendRgb565(uint16_t darkColor, uint16_t lightColor, uint8_t blendFactor);

//--- Resolve visual text color to panel text value
static uint16_t mapVisualTextColorToPanelColor(VisualTextColor visualTextColor);

//--- Get active UI color profile
static const ColorProfile& getActiveUiColorProfile();

//--- Get selected fill color for UI elements
static uint16_t getUiSelectedFillColor();

//--- Get inactive fill color for UI elements
static uint16_t getUiInactiveFillColor();

//--- Get selected text color for UI elements
static uint16_t getUiSelectedTextColor();

//--- Get inactive text color for UI elements
static uint16_t getUiInactiveTextColor();

//--- Get selected border color for UI elements
static uint16_t getUiSelectedBorderColor();

//--- Get inactive border color for UI elements
static uint16_t getUiInactiveBorderColor();

//--- Get accent color for UI elements
static uint16_t getUiAccentColor();

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

//--- Set active UI theme color index
void displaySetThemeColorIndex(int index)
{
  if (index >= 0 && index < colorProfileCount)
  {
    activeUiColorIndex = index;
  }

} //   displaySetThemeColorIndex()

//--- Get active UI theme color index
int displayGetThemeColorIndex()
{
  return activeUiColorIndex;

} //   displayGetThemeColorIndex()

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

  char buffer[32];
  std::string stateValue = timerGetStateLabel(runtimeStatus.state);
  std::string profileValue = settings.profileName.c_str();
  std::array<std::string, statusLineCount> nextStatusLines;

  if (profileValue.empty())
  {
    profileValue = "-";
  }

  nextStatusLines[0] = stateValue + "|" + profileValue;
  snprintf(buffer, sizeof(buffer), "%lu %s", static_cast<unsigned long>(settings.onTimeValue), timerGetTimeUnitLabel(settings.onTimeUnit));
  nextStatusLines[1] = buffer;
  snprintf(buffer, sizeof(buffer), "%lu %s", static_cast<unsigned long>(settings.offTimeValue), timerGetTimeUnitLabel(settings.offTimeUnit));
  nextStatusLines[2] = buffer;

  uint32_t remainingMs = 0;

  if (runtimeStatus.currentPhaseDurationMs > runtimeStatus.currentPhaseElapsedMs)
  {
    remainingMs = runtimeStatus.currentPhaseDurationMs - runtimeStatus.currentPhaseElapsedMs;
  }

  std::string outputValue = runtimeStatus.outputActive ? "ON" : "OFF";

  if (runtimeStatus.state == TIMER_STATE_RUNNING || runtimeStatus.state == TIMER_STATE_PAUSED)
  {
    if (runtimeStatus.currentPhaseDurationMs >= minimumCountdownDisplayMs)
    {
      outputValue += "  ";
      outputValue += formatRemainingMs(remainingMs);
    }
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
    snprintf(buffer, sizeof(buffer), "%lu / Inv.", static_cast<unsigned long>(displayCycle));
    nextStatusLines[4] = buffer;
  }
  else
  {
    snprintf(buffer, sizeof(buffer), "%lu/%lu", static_cast<unsigned long>(displayCycle), static_cast<unsigned long>(runtimeStatus.totalCycles));
    nextStatusLines[4] = buffer;
  }

  snprintf(buffer, sizeof(buffer), "A:%d", statusActionIndex);
  nextStatusLines[5] = buffer;

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
      tft.getTextBounds(profileValue.c_str(), 0, 0, &textX1, &textY1, &textWidth, &textHeight);
      profileX = col1X + fullW - static_cast<int>(textWidth) - 6;

      if (profileX < col1X + 100)
      {
        profileX = col1X + 100;
      }

      tft.setTextColor(getUiSelectedTextColor(), getUiSelectedFillColor());
      tft.setCursor(profileX, tileStartY + 14);
      tft.print(profileValue.c_str());
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
      uint16_t selectedFillColor = getUiSelectedFillColor();

      tft.fillRect(0, y, tft.width(), 22, selectedFillColor);
      tft.setTextColor(getUiSelectedTextColor(), selectedFillColor);
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

  tft.setTextColor(getUiAccentColor());
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
void displayDrawTextInput(const char* title, const std::string& textValue, const std::string& currentToken)
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
  tft.print(textValue.c_str());

  tft.setTextColor(getUiAccentColor());
  tft.setTextSize(3);
  tft.setCursor(6, 140);
  tft.print("[");
  tft.print(currentToken.c_str());
  tft.print("]");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(6, 205);
  tft.print("Hold=Save  K0=Back");

} //   displayDrawTextInput()

//--- Draw generic field input screen
void displayDrawFieldInput(const char* title, const char* fieldName, const std::string& fieldValue, int cursorIndex, const std::string& prevToken, const std::string& currentToken, const std::string& nextToken,
                           const char* tokenOptions[], int tokenOptionCount, int selectedOptionIndex)
{
  //--- Per colorSettings.md: PANEL_COLOR() for fills/borders, ST77XX_BLACK for readable text.
  uint16_t tokenCurrentColor = ST77XX_BLACK;
  uint16_t tokenNeighborColor = getUiInactiveFillColor();
  uint16_t tileFillColor = getUiSelectedFillColor();
  uint16_t tileBorderColor = getUiSelectedBorderColor();
  uint16_t tileLabelColor = getUiAccentColor();
  uint16_t tileValueColor = getUiSelectedTextColor();

  const int tileX = 3;
  const int tileW = 314;
  bool useButtonOptions = (cursorIndex == 0) && (tokenOptions != nullptr) && (tokenOptionCount >= 2) && (tokenOptionCount <= 6);
  bool useTwoRowButtons = useButtonOptions && (tokenOptionCount > 4);

  //--- Tile 1: field name + value, cursor arrows (higher and roomier)
  const int tile1Y = 30;
  const int tile1H = 84;

  //--- Tile 2: token selector fixed near bottom
  const int tile2H = useButtonOptions ? (useTwoRowButtons ? 104 : 70) : 44;
  const int tile2Y = useButtonOptions ? (useTwoRowButtons ? 116 : 130) : 156;

  std::string valueText = "[" + fieldValue + "]";
  std::string leftTokenText = prevToken + " < ";
  std::string rightTokenText = " > " + nextToken;
  int16_t textX1;
  int16_t textY1;
  uint16_t textW;
  uint16_t textH;
  uint16_t leftTokenW;
  uint16_t currentTokenW;
  uint16_t rightTokenW;
  int valueX;
  int tokenX;
  int cursorBaseX;
  int currentTokenX;
  int currentTokenY;

  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader(title);

  //--- Tile 1: value input
  tft.fillRoundRect(tileX, tile1Y, tileW, tile1H, 5, tileFillColor);
  tft.drawRoundRect(tileX, tile1Y, tileW, tile1H, 5, tileBorderColor);

  tft.setTextSize(2);
  tft.setTextColor(tileLabelColor, tileFillColor);
  tft.setCursor(tileX + 6, tile1Y + 5);
  tft.print(fieldName);

  tft.setTextSize(2);
  tft.getTextBounds(valueText.c_str(), 0, 0, &textX1, &textY1, &textW, &textH);
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
    tft.setCursor(cursorX, tile1Y + 24);
    tft.print("v");
  }

  tft.setTextColor(tileValueColor, tileFillColor);
  tft.setCursor(valueX, tile1Y + 41);
  tft.print(valueText.c_str());

  if (cursorIndex >= 0)
  {
    int cursorX = cursorBaseX + (cursorIndex * 12);
    tft.setTextColor(ST77XX_BLACK, tileFillColor);
    tft.setCursor(cursorX, tile1Y + 62);
    tft.print("^");
  }

  //--- Tile 2: token selector
  tft.fillRoundRect(tileX, tile2Y, tileW, tile2H, 5, tileFillColor);
  tft.drawRoundRect(tileX, tile2Y, tileW, tile2H, 5, tileBorderColor);

  tft.setTextSize(2);
  tft.setTextColor(tileLabelColor, tileFillColor);
  tft.setCursor(tileX + 6, tile2Y + 5);
  tft.print("SELECT");

  if (useButtonOptions)
  {
    const int buttonRowX = tileX + 8;
    const int buttonRowW = tileW - 16;
    const int buttonH = 30;
    const int buttonGap = 6;
    int itemsPerRow = useTwoRowButtons ? 3 : tokenOptionCount;
    int rowCount = useTwoRowButtons ? 2 : 1;
    int buttonW = (buttonRowW - (buttonGap * (itemsPerRow - 1))) / itemsPerRow;
    //--- Choice buttons: selected = LIGHT (glowing/lit), unselected = DARK (dimmed)
    //--- This is intentionally the inverse of list-row selection, which is dark=selected.
    //--- A lit-up button visually reads as "this is the current choice".
    uint16_t selectedFillColor = getUiInactiveFillColor();
    uint16_t selectedTextColor = getUiInactiveTextColor();
    uint16_t unselectedFillColor = getUiSelectedFillColor();
    uint16_t unselectedTextColor = getUiSelectedTextColor();

    tft.setTextSize(2);

    for (int row = 0; row < rowCount; row++)
    {
      int buttonRowY = tile2Y + 26 + (row * (buttonH + buttonGap));

      for (int col = 0; col < itemsPerRow; col++)
      {
        int optionIndex = row * itemsPerRow + col;

        if (optionIndex >= tokenOptionCount)
        {
          break;
        }

        int buttonX = buttonRowX + (col * (buttonW + buttonGap));
        bool isSelected = (optionIndex == selectedOptionIndex);
        uint16_t buttonFill = isSelected ? selectedFillColor : unselectedFillColor;
        uint16_t buttonText = isSelected ? selectedTextColor : unselectedTextColor;
        const char* optionLabel = tokenOptions[optionIndex];
        int16_t buttonTextX;
        int16_t buttonTextY;
        uint16_t buttonTextW;
        uint16_t buttonTextH;
        int labelX;
        int labelY;

        tft.fillRoundRect(buttonX, buttonRowY, buttonW, buttonH, 5, buttonFill);
        tft.drawRoundRect(buttonX, buttonRowY, buttonW, buttonH, 5, tileBorderColor);

        if (isSelected)
        {
          //--- Extra inner border so the lit button is unambiguous
          tft.drawRoundRect(buttonX + 1, buttonRowY + 1, buttonW - 2, buttonH - 2, 4, tileBorderColor);
        }

        tft.getTextBounds(optionLabel, 0, 0, &buttonTextX, &buttonTextY, &buttonTextW, &buttonTextH);
        labelX = buttonX + ((buttonW - static_cast<int>(buttonTextW)) / 2);
        labelY = buttonRowY + ((buttonH - static_cast<int>(buttonTextH)) / 2);

        if (labelX < (buttonX + 2))
        {
          labelX = buttonX + 2;
        }

        tft.setTextColor(buttonText, buttonFill);
        tft.setCursor(labelX, labelY);
        tft.print(optionLabel);
      }
    }
  }
  else
  {
    tft.setTextSize(2);
    tft.getTextBounds(leftTokenText.c_str(), 0, 0, &textX1, &textY1, &leftTokenW, &textH);
    tft.getTextBounds(currentToken.c_str(), 0, 0, &textX1, &textY1, &currentTokenW, &textH);
    tft.getTextBounds(rightTokenText.c_str(), 0, 0, &textX1, &textY1, &rightTokenW, &textH);

    textW = leftTokenW + currentTokenW + rightTokenW;
    tokenX = tileX + ((tileW - static_cast<int>(textW)) / 2);

    if (tokenX < tileX + 4)
    {
      tokenX = tileX + 4;
    }

    currentTokenY = tile2Y + 24;
    tft.setCursor(tokenX, currentTokenY);
    tft.setTextColor(tokenNeighborColor, tileFillColor);
    tft.print(leftTokenText.c_str());
    currentTokenX = tokenX + static_cast<int>(leftTokenW);

    tft.setTextColor(tokenCurrentColor, tileFillColor);
    tft.setCursor(currentTokenX, currentTokenY);
    tft.print(currentToken.c_str());
    //--- Pseudo-bold effect for current token on bitmap font.
    tft.setCursor(currentTokenX + 1, currentTokenY);
    tft.print(currentToken.c_str());

    tft.setTextColor(tokenNeighborColor, tileFillColor);
    tft.setCursor(currentTokenX + static_cast<int>(currentTokenW), currentTokenY);
    tft.print(rightTokenText.c_str());
  }

  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(6, tile2Y + tile2H + 4);
  if (useButtonOptions)
  {
    tft.print("Turn=Select  Short=Save");
  }
  else
  {
    tft.print("Short=Next  Turn=Change");
  }

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
void displayDrawWifiPortalScreen(const std::string& line1, const std::string& line2, const std::string& line3, const std::string& line4)
{
  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader("WiFi Manager Started");

  tft.setTextSize(2);

  drawCenteredLine(line1, 62, ST77XX_WHITE, ST77XX_BLACK);
  drawCenteredLine(line2, 92, ST77XX_GREEN, ST77XX_BLACK);
  drawCenteredLine(line3, 122, ST77XX_GREEN, ST77XX_BLACK);
  drawCenteredLine(line4, 186, getUiAccentColor(), ST77XX_BLACK);

} //   displayDrawWifiPortalScreen()

//--- Draw centered startup connection screen
void displayDrawStartupConnectionScreen(const std::string& line1, const std::string& line2)
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
  displayDrawTestColorPalette(0);

} //   displayDrawTestColorPattern()

//--- Draw test palette with dark bars and cursor
void displayDrawTestColorPalette(int selectedIndex)
{
  int barCount = 0;
  const int barY0 = 24;
  int barH;

  for (int profileIndex = 0; profileIndex < colorProfileCount; profileIndex++)
  {
    if (colorProfiles[profileIndex].usedInPalette)
    {
      barCount++;
    }
  }

  if (barCount <= 0)
  {
    return;
  }

  barH = (tft.height() - barY0) / barCount;

  if (selectedIndex < 0)
  {
    selectedIndex = 0;
  }
  else if (selectedIndex >= barCount)
  {
    selectedIndex = barCount - 1;
  }

  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader("Dark Palette");
  tft.setTextSize(2);

  int drawIndex = 0;

  for (int profileIndex = 0; profileIndex < colorProfileCount; profileIndex++)
  {
    if (!colorProfiles[profileIndex].usedInPalette)
    {
      continue;
    }

    int y = barY0 + (drawIndex * barH);
    int drawH = barH;
    uint16_t panelColor = PANEL_COLOR(colorProfiles[profileIndex].getDarkColor());
    uint16_t labelColor = mapVisualTextColorToPanelColor(colorProfiles[profileIndex].darkLabelColor);

    if (drawIndex == (barCount - 1))
    {
      drawH = tft.height() - y;
    }

    tft.fillRect(0, y, tft.width(), drawH, panelColor);
    tft.setTextColor(labelColor, panelColor);
    tft.setCursor(26, y + ((drawH - 16) / 2));
    tft.print(colorProfiles[profileIndex].colorName);

    if (drawIndex == selectedIndex)
    {
      tft.setTextColor(labelColor, panelColor);
      tft.setCursor(6, y + ((drawH - 16) / 2));
      tft.print(">");
    }

    drawIndex++;
  }

} //   displayDrawTestColorPalette()

//--- Draw color fade test for selected color
void displayDrawTestColorFade(const char* colorName, uint16_t darkColorVisual, uint16_t lightColorVisual, VisualTextColor darkLabelColor, VisualTextColor lightLabelColor)
{
  const int rowCount = 8;
  const int rowY0 = 24;
  const int rowH = (tft.height() - rowY0) / rowCount;
  const int textHeight = 16;
  uint16_t darkLabelPanelColor = mapVisualTextColorToPanelColor(darkLabelColor);
  uint16_t lightLabelPanelColor = mapVisualTextColorToPanelColor(lightLabelColor);
  char rowText[8];
  char hexText[24];

  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader(colorName);
  tft.setTextSize(2);

  for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
  {
    int y = rowY0 + (rowIndex * rowH);
    int drawH = rowH;
    uint8_t blendFactor = static_cast<uint8_t>((rowIndex * 255) / (rowCount - 1));
    uint16_t shadeVisual = blendRgb565(darkColorVisual, lightColorVisual, blendFactor);
    uint16_t shadePanel = PANEL_COLOR(shadeVisual);

    if (rowIndex == (rowCount - 1))
    {
      drawH = tft.height() - y;
    }

    tft.fillRect(0, y, tft.width(), drawH, shadePanel);
    snprintf(rowText, sizeof(rowText), "%d", rowIndex);
    snprintf(hexText, sizeof(hexText), "0x%04X", static_cast<unsigned int>(shadeVisual));

    tft.setTextColor(darkLabelPanelColor, shadePanel);
    tft.setCursor(8, y + ((drawH - textHeight) / 2));
    tft.print(rowText);

    tft.setTextColor(lightLabelPanelColor, shadePanel);
    tft.setCursor(44, y + ((drawH - textHeight) / 2));
    tft.print(hexText);
  }

} //   displayDrawTestColorFade()

//--- Set display backlight state
void displaySetBacklight(bool enabled)
{
  digitalWrite(PIN_TFT_BL, enabled ? HIGH : LOW);

} //   displaySetBacklight()

//--- Draw screen header
static void drawHeader(const char* title)
{
  uint16_t headerBackgroundColor = getUiSelectedFillColor();
  uint16_t headerTextColor = getUiSelectedTextColor();

  tft.fillRect(0, 0, tft.width(), 24, headerBackgroundColor);
  tft.setCursor(6, 4);
  tft.setTextColor(headerTextColor, headerBackgroundColor);
  tft.setTextSize(2);
  tft.print(title);
  tft.setCursor(7, 4);
  tft.print(title);

} //   drawHeader()

//--- Draw centered single line text
static void drawCenteredLine(const std::string& lineText, int y, uint16_t textColor, uint16_t backgroundColor)
{
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
  int cursorX;

  tft.getTextBounds(lineText.c_str(), 0, 0, &x1, &y1, &width, &height);
  cursorX = (tft.width() - static_cast<int>(width)) / 2;

  if (cursorX < 0)
  {
    cursorX = 0;
  }

  tft.setTextColor(textColor, backgroundColor);
  tft.setCursor(cursorX, y);
  tft.print(lineText.c_str());

} //   drawCenteredLine()

//--- Draw a status tile
static void drawStatusTile(int x, int y, int w, int h, const char* label, const std::string& value)
{
  uint16_t tileFillColor = getUiSelectedFillColor();
  uint16_t tileBorderColor = getUiSelectedBorderColor();
  uint16_t tileLabelColor = getUiAccentColor();
  uint16_t tileValueColor = getUiSelectedTextColor();

  tft.fillRoundRect(x, y, w, h, 5, tileFillColor);
  tft.drawRoundRect(x, y, w, h, 5, tileBorderColor);

  tft.setTextSize(1);
  tft.setTextColor(tileLabelColor, tileFillColor);
  tft.setCursor(x + 4, y + 3);
  tft.print(label);

  tft.setTextSize(2);
  tft.setTextColor(tileValueColor, tileFillColor);
  tft.setCursor(x + 4, y + 14);
  tft.print(value.c_str());

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
    uint16_t fillColor = isSelected ? getUiSelectedFillColor() : getUiInactiveFillColor();
    uint16_t borderColor = isSelected ? getUiSelectedBorderColor() : getUiInactiveBorderColor();
    uint16_t textColor = isSelected ? getUiSelectedTextColor() : getUiInactiveTextColor();
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
      //--- Extra inner border: ST77XX_BLACK appears WHITE on this inverted panel
      tft.drawRoundRect(buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2, 8, ST77XX_BLACK);
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
static std::string formatRemainingMs(uint32_t remainingMs)
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

  return std::string(buffer);

} //   formatRemainingMs()

//--- Blend two RGB565 colors by 8-bit factor
static uint16_t blendRgb565(uint16_t darkColor, uint16_t lightColor, uint8_t blendFactor)
{
  uint8_t darkR = static_cast<uint8_t>((darkColor >> 11) & 0x1FU);
  uint8_t darkG = static_cast<uint8_t>((darkColor >> 5) & 0x3FU);
  uint8_t darkB = static_cast<uint8_t>(darkColor & 0x1FU);
  uint8_t lightR = static_cast<uint8_t>((lightColor >> 11) & 0x1FU);
  uint8_t lightG = static_cast<uint8_t>((lightColor >> 5) & 0x3FU);
  uint8_t lightB = static_cast<uint8_t>(lightColor & 0x1FU);
  uint16_t mixedR = static_cast<uint16_t>((static_cast<uint16_t>(darkR) * static_cast<uint16_t>(255U - blendFactor) + static_cast<uint16_t>(lightR) * static_cast<uint16_t>(blendFactor)) / 255U);
  uint16_t mixedG = static_cast<uint16_t>((static_cast<uint16_t>(darkG) * static_cast<uint16_t>(255U - blendFactor) + static_cast<uint16_t>(lightG) * static_cast<uint16_t>(blendFactor)) / 255U);
  uint16_t mixedB = static_cast<uint16_t>((static_cast<uint16_t>(darkB) * static_cast<uint16_t>(255U - blendFactor) + static_cast<uint16_t>(lightB) * static_cast<uint16_t>(blendFactor)) / 255U);

  return static_cast<uint16_t>((mixedR << 11) | (mixedG << 5) | mixedB);

} //   blendRgb565()

//--- Get active UI color profile
static const ColorProfile& getActiveUiColorProfile()
{
  if (activeUiColorIndex >= 0 && activeUiColorIndex < colorProfileCount)
  {
    return colorProfiles[activeUiColorIndex];
  }

  return colorProfiles[0];

} //   getActiveUiColorProfile()

//--- Get selected fill color for UI elements
static uint16_t getUiSelectedFillColor()
{
  return PANEL_COLOR(getActiveUiColorProfile().getDarkColor());

} //   getUiSelectedFillColor()

//--- Get inactive fill color for UI elements
static uint16_t getUiInactiveFillColor()
{
  return PANEL_COLOR(getActiveUiColorProfile().getLightColor());

} //   getUiInactiveFillColor()

//--- Get selected text color for UI elements
static uint16_t getUiSelectedTextColor()
{
  return mapVisualTextColorToPanelColor(getActiveUiColorProfile().darkLabelColor);

} //   getUiSelectedTextColor()

//--- Get inactive text color for UI elements
static uint16_t getUiInactiveTextColor()
{
  return mapVisualTextColorToPanelColor(getActiveUiColorProfile().lightLabelColor);

} //   getUiInactiveTextColor()

//--- Get selected border color for UI elements
static uint16_t getUiSelectedBorderColor()
{
  return getUiInactiveFillColor();

} //   getUiSelectedBorderColor()

//--- Get inactive border color for UI elements
static uint16_t getUiInactiveBorderColor()
{
  return PANEL_COLOR(blendRgb565(getActiveUiColorProfile().getLightColor(), 0xFFFF, 92));

} //   getUiInactiveBorderColor()

//--- Get accent color for UI elements
static uint16_t getUiAccentColor()
{
  return getUiInactiveFillColor();

} //   getUiAccentColor()

//--- Resolve visual text color to panel text value
static uint16_t mapVisualTextColorToPanelColor(VisualTextColor visualTextColor)
{
  if (visualTextColor == VISUAL_TEXT_WHITE)
  {
    return ST77XX_BLACK;
  }

  return ST77XX_WHITE;

} //   mapVisualTextColorToPanelColor()
