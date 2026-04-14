#include "displayDriver.h"
#include "appConfig.h"
#include "timerEngine.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <esp_log.h>

//--- Logging tag
static const char *logTag = "displayDriver";

//--- TFT instance
static Adafruit_ST7789 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

//--- Header colors
static const uint16_t headerBackgroundColor = 0x4208;
static const uint16_t headerTextColor = ST77XX_WHITE;

//--- Status line layout
static const int statusLineHeight = 22;
static const int statusLineTopY = 30;
static const int statusLineCount = 8;

//--- Status screen cache
static bool statusScreenPrepared = false;
static String cachedStatusLines[statusLineCount];

//--- Draw one status line
static void drawStatusLine(int lineIndex, const String &lineText);

//--- Invalidate status screen cache
static void invalidateStatusScreenCache();

//--- Draw screen header
static void drawHeader(const char *title);

//--- Initialize display
void displayInit()
{
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);

  tft.init(TFT_HEIGHT, TFT_WIDTH);
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  invalidateStatusScreenCache();

  ESP_LOGI(logTag, "Display initialized");

}   //   displayInit()

//--- Update status screen
void displayDrawStatusScreen(const AppSettings &settings, const RuntimeStatus &runtimeStatus, bool wifiConnected)
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
  nextStatusLines[1] = String("On: ") + String(settings.onTimeValue) + String(" ") + String(timerGetTimeUnitLabel(settings.onTimeUnit));
  nextStatusLines[2] = String("Off: ") + String(settings.offTimeValue) + String(" ") + String(timerGetTimeUnitLabel(settings.offTimeUnit));
  nextStatusLines[3] = String("Cycles: ") + String(settings.repeatCount);
  nextStatusLines[4] = String("Done: ") + String(runtimeStatus.currentCycle) + String("/") + (runtimeStatus.totalCycles == 0 ? String("INF") : String(runtimeStatus.totalCycles));
  nextStatusLines[5] = String("Output: ") + String(runtimeStatus.outputActive ? "ON" : "OFF");
  nextStatusLines[6] = String("Trigger: ") + String(timerGetTriggerModeLabel(settings.triggerMode));
  nextStatusLines[7] = String("Profile: ") + settings.profileName;

  for (int lineIndex = 0; lineIndex < statusLineCount; lineIndex++)
  {
    if (cachedStatusLines[lineIndex] != nextStatusLines[lineIndex])
    {
      drawStatusLine(lineIndex, nextStatusLines[lineIndex]);
      cachedStatusLines[lineIndex] = nextStatusLines[lineIndex];
    }
  }

}   //   displayDrawStatusScreen()

//--- Draw text list
void displayDrawListScreen(const char *title, const String items[], size_t itemCount, int selectedIndex, int firstVisibleIndex)
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
      tft.setTextColor(ST77XX_BLACK, ST77XX_YELLOW);
    }
    else
    {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    }

    tft.setCursor(6, 30 + (line * 22));
    tft.print(items[itemIndex]);
  }

}   //   displayDrawListScreen()

//--- Draw numeric editor
void displayDrawNumberEditor(const char *label, uint32_t value, const char *unitLabel)
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
  tft.printf("%lu", value);

  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.setCursor(6, 170);
  tft.print(unitLabel);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(6, 205);
  tft.print("Press=OK  K0=Back");

}   //   displayDrawNumberEditor()

//--- Draw enum editor
void displayDrawEnumEditor(const char *label, const char *valueLabel)
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

}   //   displayDrawEnumEditor()

//--- Draw text input editor
void displayDrawTextInput(const char *title, const String &textValue, const String &currentToken)
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

}   //   displayDrawTextInput()

//--- Draw message screen
void displayDrawMessage(const char *title, const char *message)
{
  invalidateStatusScreenCache();
  tft.fillScreen(ST77XX_BLACK);
  drawHeader(title);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(6, 60);
  tft.print(message);

}   //   displayDrawMessage()

//--- Set display backlight state
void displaySetBacklight(bool enabled)
{
  digitalWrite(PIN_TFT_BL, enabled ? HIGH : LOW);

}   //   displaySetBacklight()

//--- Draw screen header
static void drawHeader(const char *title)
{
  tft.fillRect(0, 0, tft.width(), 24, headerBackgroundColor);
  tft.setCursor(6, 4);
  tft.setTextColor(headerTextColor, headerBackgroundColor);
  tft.setTextSize(2);
  tft.print(title);

}   //   drawHeader()

//--- Draw one status line
static void drawStatusLine(int lineIndex, const String &lineText)
{
  int y = statusLineTopY + (lineIndex * statusLineHeight);

  tft.fillRect(0, y, tft.width(), statusLineHeight, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(6, y);
  tft.print(lineText);

}   //   drawStatusLine()

//--- Invalidate status screen cache
static void invalidateStatusScreenCache()
{
  statusScreenPrepared = false;

}   //   invalidateStatusScreenCache()
