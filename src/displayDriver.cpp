/*** Last Changed: 2026-04-15 - 16:11 ***/
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

//--- Status line layout
static const int statusLineHeight = 22;
static const int statusLineTopY = 30;
static const int statusLineCount = 6;

//--- Status screen cache
static bool statusScreenPrepared = false;
static String cachedStatusLines[statusLineCount];

//--- Draw one status line
static void drawStatusLine(int lineIndex, const String& lineText);

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

  String nextStatusLines[statusLineCount];
  nextStatusLines[0] = String("State: ") + String(timerGetStateLabel(runtimeStatus.state));
  nextStatusLines[1] = String("On time: ") + String(settings.onTimeValue) + String(" ") + String(timerGetTimeUnitLabel(settings.onTimeUnit));
  nextStatusLines[2] = String("Off time: ") + String(settings.offTimeValue) + String(" ") + String(timerGetTimeUnitLabel(settings.offTimeUnit));

  uint32_t remainingMs = 0;

  if (runtimeStatus.currentPhaseDurationMs > runtimeStatus.currentPhaseElapsedMs)
  {
    remainingMs = runtimeStatus.currentPhaseDurationMs - runtimeStatus.currentPhaseElapsedMs;
  }

  String outputLine = String("Output: ") + String(runtimeStatus.outputActive ? "ON " : "OFF ");

  if (runtimeStatus.state == TIMER_STATE_RUNNING || runtimeStatus.state == TIMER_STATE_PAUSED)
  {
    outputLine += formatRemainingMs(remainingMs);
  }
  else
  {
    outputLine += String("---:--");
  }

  nextStatusLines[3] = outputLine;

  if (runtimeStatus.totalCycles == 0)
  {
    nextStatusLines[4] = "Cycles: Inv.";
  }
  else
  {
    uint32_t displayCycle = runtimeStatus.currentCycle;

    if (runtimeStatus.state == TIMER_STATE_RUNNING || runtimeStatus.state == TIMER_STATE_PAUSED)
    {
      if (displayCycle < runtimeStatus.totalCycles)
      {
        displayCycle++;
      }
    }
    else if (runtimeStatus.state == TIMER_STATE_FINISHED)
    {
      displayCycle = runtimeStatus.totalCycles;
    }

    nextStatusLines[4] = String("Cycles: ") + String(displayCycle) + String("/") + String(runtimeStatus.totalCycles);
  }

  nextStatusLines[5] = String("ACTION:") + String(statusActionIndex);

  for (int lineIndex = 0; lineIndex < statusLineCount; lineIndex++)
  {
    if (cachedStatusLines[lineIndex] != nextStatusLines[lineIndex])
    {
      if (lineIndex == statusLineCount - 1)
      {
        drawStatusActionButtons(statusActionIndex);
      }
      else
      {
        drawStatusLine(lineIndex, nextStatusLines[lineIndex]);
      }

      cachedStatusLines[lineIndex] = nextStatusLines[lineIndex];
    }
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
  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader(title);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(6, 44);
  tft.print(fieldName);

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(6, 76);
  tft.print("[");
  tft.print(fieldValue);
  tft.print("]");

  if (cursorIndex >= 0)
  {
    int cursorX = 18 + (cursorIndex * 12);
    tft.setTextColor(selectionAccentColor);
    tft.setCursor(cursorX, 98);
    tft.print("^");
  }

  tft.setTextColor(selectionAccentColor);
  tft.setCursor(6, 130);
  tft.print(prevToken);
  tft.print(" < ");
  tft.print(currentToken);
  tft.print(" > ");
  tft.print(nextToken);

  tft.setTextColor(ST77XX_WHITE);
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

//--- Draw one status line
static void drawStatusLine(int lineIndex, const String& lineText)
{
  int y = statusLineTopY + (lineIndex * statusLineHeight);

  tft.fillRect(0, y, tft.width(), statusLineHeight, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(6, y);
  tft.print(lineText);

} //   drawStatusLine()

//--- Draw status action buttons
static void drawStatusActionButtons(int statusActionIndex)
{
  static const char* labels[] =
      {
          "Start",
          "Stop",
          "Reset"};

  const int buttonY = 188;
  const int buttonHeight = 38;
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
    textY = buttonY + ((buttonHeight - static_cast<int>(textHeight)) / 2) + 5;

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
