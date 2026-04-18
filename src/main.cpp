/*** Last Changed: 2026-04-18 - 15:49 ***/
#include <Arduino.h>

#include "buttonInput.h"
#include "displayDriver.h"
#include "encoderInput.h"
#include "ioControl.h"
#include "profileManager.h"
#include "settingsStore.h"
#include "timerEngine.h"
#include "colorSettings.h"
#include "uiMenu.h"
#include "webUi.h"
#include "WiFiManagerExt.h"
#include "appConfig.h"

#include <WiFi.h>
#include <cstdio>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include <time.h>

const char* PROG_VERSION = "v1.3.0";

//--- Logging tag
static const char* logTag = "main";

//--- Active application settings
static AppSettings activeSettings;

//--- WiFi disable indicator
static bool wifiDisabled = false;

//--- Track WiFi link transitions for NTP sync
static bool wifiWasConnected = false;

//--- Track whether NTP has already been configured in current session
static bool ntpConfigured = false;

//--- Task handles
static TaskHandle_t timerTaskHandle = nullptr;
static TaskHandle_t inputTaskHandle = nullptr;

//--- Task periods
static const TickType_t timerTaskPeriodTicks = pdMS_TO_TICKS(2);
static const TickType_t inputTaskPeriodTicks = pdMS_TO_TICKS(2);

//--- GPIO00 hold-to-reset tracking
static bool wifiResetPressed = false;
static bool wifiResetTriggered = false;
static uint32_t wifiResetPressStartMs = 0;
static uint32_t wifiResetLastUiUpdateMs = 0;

//--- NTP configuration
static const char* ntpTimeZone = "CET-1CEST,M3.5.0/2,M10.5.0/3";
static const char* ntpServer1 = "pool.ntp.org";
static const char* ntpServer2 = "time.nist.gov";

//--- Load initial settings and profiles
static void loadStartupSettings();

//--- Update external trigger and reset inputs
static void updateExternalInputs();

//--- Update all service modules
static void timerTask(void* taskParameter);

//--- Update all input-related modules
static void inputTask(void* taskParameter);

//--- Handle GPIO00 hold action for clearing WiFi credentials
static bool handleWifiCredentialResetHold();

//--- Show startup WiFi connection message
static void showStartupWifiConnectionMessage();

//--- Configure NTP when WiFi STA becomes connected
static void syncNtpTimeIfNeeded();

#ifdef TEST_COLOR_PATERN
//--- Test pattern screens
enum TestColorScreen
{
  TEST_COLOR_SCREEN_PALETTE = 0,
  TEST_COLOR_SCREEN_FADE = 1
};

static int testColorSelectedIndex = 0;
static TestColorScreen testColorScreen = TEST_COLOR_SCREEN_PALETTE;

//--- Get number of enabled palette colors
static int getUsedTestColorCount();

//--- Map enabled color index to profile index
static int mapUsedIndexToProfileIndex(int usedIndex);

//--- Blend two RGB565 colors using the same formula as displayDrawTestColorFade
static uint16_t blendRgb565ForFadeDump(uint16_t darkColor, uint16_t lightColor, uint8_t blendFactor);

//--- Print generated fade shades and lookup shades for all colors
static void printColorShadeDumpToSerial();

//--- Draw active test color screen
static void drawTestColorScreen();

//--- Handle interactive test color input
static void handleTestColorPatternInput();
#endif

//--- Load initial settings and profiles
static void loadStartupSettings()
{
  timerLoadDefaultSettings(activeSettings);

  String lastProfileName = settingsStoreLoadLastProfileName();

  if (!lastProfileName.isEmpty())
  {
    if (profileManagerLoadProfile(lastProfileName, activeSettings))
    {
      ESP_LOGI(logTag, "Loaded last profile: %s", lastProfileName.c_str());
    }
    else
    {
      ESP_LOGW(logTag, "Warning: Failed to load last profile: %s", lastProfileName.c_str());
    }
  }

  settingsStoreLoadSystemSettings(activeSettings);
  encoderSetDirectionReversed(settingsStoreLoadEncoderDirectionReversed());
  displaySetThemeColorIndex(settingsStoreLoadThemeColorIndex());

  timerSetSettings(activeSettings);

} //   loadStartupSettings()

//--- Update external trigger and reset inputs
static void updateExternalInputs()
{
  AppSettings settings = timerGetSettings();

  if (ioTriggerActivated(settings.triggerEdge))
  {
    timerHandleExternalTrigger();
  }

  if (ioResetActivated())
  {
    timerHandleExternalReset();
  }

} //   updateExternalInputs()

//--- Update all timer-related modules
static void timerTask(void* taskParameter)
{
  (void)taskParameter;

  for (;;)
  {
    timerUpdate();

    RuntimeStatus runtimeStatus = timerGetRuntimeStatus();
    AppSettings settings = timerGetSettings();
    ioUpdate(runtimeStatus, settings);

    vTaskDelay(timerTaskPeriodTicks);
  }

} //   timerTask()

//--- Update all input-related modules
static void inputTask(void* taskParameter)
{
  (void)taskParameter;

  for (;;)
  {
    encoderUpdate();
    buttonUpdate();
    updateExternalInputs();
    uiMenuUpdate();
    handleWifiCredentialResetHold();

    vTaskDelay(inputTaskPeriodTicks);
  }

} //   inputTask()

//--- Handle GPIO00 hold action for clearing WiFi credentials
static bool handleWifiCredentialResetHold()
{
  bool rawPressed = digitalRead(PIN_WIFI_ERASE) == LOW;

  if (rawPressed && !wifiResetPressed)
  {
    wifiResetPressed = true;
    wifiResetTriggered = false;
    wifiResetPressStartMs = millis();
    wifiResetLastUiUpdateMs = 0;

    return true;
  }

  if (!rawPressed)
  {
    wifiResetPressed = false;
    wifiResetTriggered = false;
    wifiResetLastUiUpdateMs = 0;

    return false;
  }

  if (wifiResetTriggered)
  {
    return true;
  }

  uint32_t elapsedMs = millis() - wifiResetPressStartMs;

  if (wifiResetLastUiUpdateMs == 0 || millis() - wifiResetLastUiUpdateMs >= 250)
  {
    uint32_t remainingMs = (elapsedMs < WIFI_CREDENTIAL_RESET_HOLD_MS) ? (WIFI_CREDENTIAL_RESET_HOLD_MS - elapsedMs) : 0;
    uint32_t remainingSeconds = (remainingMs + 999UL) / 1000UL;
    char messageBuffer[48];

    snprintf(messageBuffer, sizeof(messageBuffer), "Hold %lus to erase WiFi", static_cast<unsigned long>(remainingSeconds));

    displayDrawMessage("WiFi Reset", messageBuffer);
    wifiResetLastUiUpdateMs = millis();
  }

  if (elapsedMs < WIFI_CREDENTIAL_RESET_HOLD_MS)
  {
    return true;
  }

  wifiResetTriggered = true;

  ESP_LOGW(logTag, "GPIO00 hold detected, erasing WiFi credentials and restarting");
  displayDrawMessage("WiFi Reset", "Erasing creds...");
  settingsStoreEraseWifiCredentials();
  settingsStoreSaveWifiDisabled(true);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  ESP.restart();

  return true;

} //   handleWifiCredentialResetHold()

//--- Show startup WiFi connection message
static void showStartupWifiConnectionMessage()
{
  WifiSettings wifiSettings;
  std::string ssid;

  settingsStoreLoadWifiSettings(wifiSettings);
  ssid = wifiSettings.staSsid.c_str();

  if (ssid.empty())
  {
    ssid = "<SSID>";
  }

  displayDrawStartupConnectionScreen("Trying to connect to", ssid);

} //   showStartupWifiConnectionMessage()

//--- Configure NTP when WiFi STA becomes connected
static void syncNtpTimeIfNeeded()
{
  if (wifiDisabled)
  {
    return;
  }

  bool wifiConnected = wifiManagerIsStaConnected();

  if (!wifiConnected)
  {
    wifiWasConnected = false;

    return;
  }

  if (wifiWasConnected && ntpConfigured)
  {
    return;
  }

  configTzTime(ntpTimeZone, ntpServer1, ntpServer2);
  ntpConfigured = true;
  wifiWasConnected = true;
  ESP_LOGI(logTag, "NTP sync configured (servers: %s, %s)", ntpServer1, ntpServer2);

} //   syncNtpTimeIfNeeded()

#ifdef TEST_COLOR_PATERN
//--- Get number of enabled palette colors
static int getUsedTestColorCount()
{
  int usedCount = 0;

  for (int index = 0; index < colorProfileCount; index++)
  {
    if (colorProfiles[index].usedInPalette)
    {
      usedCount++;
    }
  }

  return usedCount;

} //   getUsedTestColorCount()

//--- Map enabled color index to profile index
static int mapUsedIndexToProfileIndex(int usedIndex)
{
  int usedCounter = 0;

  for (int profileIndex = 0; profileIndex < colorProfileCount; profileIndex++)
  {
    if (!colorProfiles[profileIndex].usedInPalette)
    {
      continue;
    }

    if (usedCounter == usedIndex)
    {
      return profileIndex;
    }

    usedCounter++;
  }

  return -1;

} //   mapUsedIndexToProfileIndex()

//--- Blend two RGB565 colors using the same formula as displayDrawTestColorFade
static uint16_t blendRgb565ForFadeDump(uint16_t darkColor, uint16_t lightColor, uint8_t blendFactor)
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

} //   blendRgb565ForFadeDump()

//--- Print generated fade shades and lookup shades for all colors
static void printColorShadeDumpToSerial()
{
  const int rowCount = 8;

  Serial.println();
  Serial.println("--- Color shade dump (TEST_COLOR_PATERN) ---");

  for (int profileIndex = 0; profileIndex < colorProfileCount; profileIndex++)
  {
    const ColorProfile& profile = colorProfiles[profileIndex];
    uint16_t darkColorVisual = profile.getDarkColor();
    uint16_t lightColorVisual = profile.getLightColor();

    Serial.printf("%s generated: {", profile.colorName);

    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
      uint8_t blendFactor = static_cast<uint8_t>((rowIndex * 255) / (rowCount - 1));
      uint16_t shadeVisual = blendRgb565ForFadeDump(darkColorVisual, lightColorVisual, blendFactor);

      Serial.printf("0x%04X", static_cast<unsigned int>(shadeVisual));

      if (rowIndex < (rowCount - 1))
      {
        Serial.print(", ");
      }
    }

    Serial.println("}");

    Serial.printf("%s lookup:    {", profile.colorName);

    for (int shadeIndex = 0; shadeIndex < rowCount; shadeIndex++)
    {
      Serial.printf("0x%04X", static_cast<unsigned int>(profile.shades[shadeIndex]));

      if (shadeIndex < (rowCount - 1))
      {
        Serial.print(", ");
      }
    }

    Serial.printf("}  (darkLevel=%d, lightLevel=%d)", profile.darkLevel, profile.lightLevel);
    Serial.println();
  }

  Serial.println("--- End color shade dump ---");
  Serial.println();

} //   printColorShadeDumpToSerial()

//--- Draw active test color screen
static void drawTestColorScreen()
{
  int profileIndex = mapUsedIndexToProfileIndex(testColorSelectedIndex);

  if (profileIndex < 0)
  {
    return;
  }

  const ColorProfile& selectedProfile = colorProfiles[profileIndex];

  if (testColorScreen == TEST_COLOR_SCREEN_PALETTE)
  {
    displayDrawTestColorPalette(testColorSelectedIndex);

    return;
  }

  displayDrawTestColorFade(selectedProfile.colorName, selectedProfile.getDarkColor(), selectedProfile.getLightColor(), selectedProfile.darkLabelColor, selectedProfile.lightLabelColor);

} //   drawTestColorScreen()

//--- Handle interactive test color input
static void handleTestColorPatternInput()
{
  EncoderEvent event;

  encoderUpdate();
  event = encoderGetEvent();

  if (event == ENCODER_EVENT_NONE)
  {
    return;
  }

  if (testColorScreen == TEST_COLOR_SCREEN_PALETTE)
  {
    int usedCount = getUsedTestColorCount();

    if (event == ENCODER_EVENT_LEFT)
    {
      if (testColorSelectedIndex > 0)
      {
        testColorSelectedIndex--;
        drawTestColorScreen();
      }
    }
    else if (event == ENCODER_EVENT_RIGHT)
    {
      if (testColorSelectedIndex < (usedCount - 1))
      {
        testColorSelectedIndex++;
        drawTestColorScreen();
      }
    }
    else if (event == ENCODER_EVENT_SHORT_PRESS)
    {
      testColorScreen = TEST_COLOR_SCREEN_FADE;
      drawTestColorScreen();
    }

    return;
  }

  if (event == ENCODER_EVENT_MEDIUM_PRESS)
  {
    testColorScreen = TEST_COLOR_SCREEN_PALETTE;
    drawTestColorScreen();
  }

} //   handleTestColorPatternInput()
#endif

//--- Arduino setup
void setup()
{
  Serial.begin(115200);
  delay(100);

#ifdef TEST_COLOR_PATERN
  encoderInit();
  displayInit();
  printColorShadeDumpToSerial();
  drawTestColorScreen();
  ESP_LOGI(logTag, "TEST_COLOR_PATERN enabled: interactive color test active");

  return;
#endif

  settingsStoreInit();
  wifiDisabled = settingsStoreLoadWifiDisabled();
  profileManagerInit();
  timerInit();
  loadStartupSettings();
  encoderInit();
  buttonInit();
  pinMode(PIN_WIFI_ERASE, INPUT_PULLUP);
  ioInit();
  displayInit();

  if (!wifiDisabled)
  {
    showStartupWifiConnectionMessage();
    delay(600);
    wifiManagerInit();
    webUiInit();
  }
  else
  {
    WiFi.mode(WIFI_OFF);
    ESP_LOGI(logTag, "WiFi stack disabled by user indicator");
  }

  uiMenuInit();

  xTaskCreatePinnedToCore(timerTask, "timerTask", 4096, nullptr, 3, &timerTaskHandle, 1);
  xTaskCreatePinnedToCore(inputTask, "inputTask", 6144, nullptr, 3, &inputTaskHandle, 1);

  ESP_LOGI(logTag, "Setup complete");

  if (!wifiDisabled)
  {
    ESP_LOGI(logTag, "Open web UI at: http://%s", wifiManagerGetAddressString().c_str());
  }

} //   setup()

//--- Arduino loop
void loop()
{
#ifdef TEST_COLOR_PATERN
  handleTestColorPatternInput();
  delay(2);

  return;
#endif

  if (!wifiDisabled)
  {
    wifiManagerUpdate();
    syncNtpTimeIfNeeded();
    webUiUpdate();
  }

  delay(2);

} //   loop()
