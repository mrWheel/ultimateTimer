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

//--- Draw screen header
static void drawHeader(const char *title);

//--- Initialize display
void displayInit()
{
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);

  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);

  ESP_LOGI(logTag, "Display initialized");

}   //   displayInit()

//--- Update status screen
void displayDrawStatusScreen(const AppSettings &settings, const RuntimeStatus &runtimeStatus, bool wifiConnected)
{
  tft.fillScreen(ST77XX_BLACK);
  drawHeader("Universal Timer");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  tft.setCursor(6, 30);
  tft.printf("State: %s", timerGetStateLabel(runtimeStatus.state));

  tft.setCursor(6, 55);
  tft.printf("On: %lu %s", settings.onTimeValue, timerGetTimeUnitLabel(settings.onTimeUnit));

  tft.setCursor(6, 80);
  tft.printf("Off: %lu %s", settings.offTimeValue, timerGetTimeUnitLabel(settings.offTimeUnit));

  tft.setCursor(6, 105);
  tft.printf("Cycles: %lu", settings.repeatCount);

  tft.setCursor(6, 130);
  tft.printf("Done: %lu/%lu", runtimeStatus.currentCycle, runtimeStatus.totalCycles);

  tft.setCursor(6, 155);
  tft.printf("Output: %s", runtimeStatus.outputActive ? "ON" : "OFF");

  tft.setCursor(6, 180);
  tft.printf("Trigger: %s", timerGetTriggerModeLabel(settings.triggerMode));

  tft.setCursor(6, 205);
  tft.printf("WiFi: %s", wifiConnected ? "Connected" : "AP only");

}   //   displayDrawStatusScreen()

//--- Draw text list
void displayDrawListScreen(const char *title, const String items[], size_t itemCount, int selectedIndex, int firstVisibleIndex)
{
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
  tft.fillRect(0, 0, TFT_HEIGHT, 24, ST77XX_BLUE);
  tft.setCursor(6, 4);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
  tft.setTextSize(2);
  tft.print(title);

}   //   drawHeader()
