/*** Last Changed: 2026-04-15 - 13:12 ***/
#include "displayDriver.h"
#include "appConfig.h"
#include "timerEngine.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <esp_log.h>

//--- TFT instance
static Adafruit_ST7789 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

//--- Header colors
static const uint16_t headerBackgroundColor = 0x4208;
static const uint16_t headerTextColor = ST77XX_WHITE;

//--- Status line layout
static const int statusLineHeight = 22;
static const int statusLineTopY = 30;
static const int statusLineCount = 6;

//--- Status screen cache
static bool statusScreenPrepared = false;
static String cachedStatusLines[statusLineCount];

//--- Draw one status line
static void drawStatusLine(int lineIndex, const String& lineText);

//--- Format remaining duration text from milliseconds
static String formatRemainingMs(uint32_t remainingMs);

//--- Invalidate status screen cache
static void invalidateStatusScreenCache();

//--- Draw screen header
static void drawHeader(const char* title);

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

  String actionLine = "";

  if (statusActionIndex == 0)
  {
    actionLine += "[Start] ";
  }
  else
  {
    actionLine += " Start  ";
  }

  if (statusActionIndex == 1)
  {
    actionLine += "[Stop] ";
  }
  else
  {
    actionLine += " Stop  ";
  }

  if (statusActionIndex == 2)
  {
    actionLine += "[Reset]";
  }
  else
  {
    actionLine += " Reset";
  }

  nextStatusLines[5] = actionLine;

  for (int lineIndex = 0; lineIndex < statusLineCount; lineIndex++)
  {
    if (cachedStatusLines[lineIndex] != nextStatusLines[lineIndex])
    {
      drawStatusLine(lineIndex, nextStatusLines[lineIndex]);
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

    if (itemIndex == selectedIndex)
    {
      tft.setTextColor(ST77XX_WHITE, 0x7BEF);
    }
    else
    {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    }

    tft.setCursor(6, 30 + (line * 22));
    tft.print(items[itemIndex]);
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

  tft.setTextColor(ST77XX_CYAN);
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

  tft.setTextColor(ST77XX_CYAN);
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
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(cursorX, 98);
    tft.print("^");
  }

  tft.setTextColor(ST77XX_CYAN);
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

} //   drawHeader()

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
