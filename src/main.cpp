/*** Last Changed: 2026-04-16 - 14:54 ***/
#include <Arduino.h>

#include "buttonInput.h"
#include "displayDriver.h"
#include "encoderInput.h"
#include "ioControl.h"
#include "profileManager.h"
#include "settingsStore.h"
#include "timerEngine.h"
#include "uiMenu.h"
#include "webUi.h"
#include "WiFiManagerExt.h"
#include "appConfig.h"

#include <WiFi.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

const char* PROG_VERSION = "v1.0.4";

//--- Logging tag
static const char* logTag = "main";

//--- Active application settings
static AppSettings activeSettings;

//--- WiFi disable indicator
static bool wifiDisabled = false;

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

  activeSettings.outputPolarityHigh = settingsStoreLoadOutputPolarityHigh();
  encoderSetDirectionReversed(settingsStoreLoadEncoderDirectionReversed());

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
    String message = String("Hold ") + String(remainingSeconds) + String("s to erase WiFi");

    displayDrawMessage("WiFi Reset", message.c_str());
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
  String ssid;

  settingsStoreLoadWifiSettings(wifiSettings);
  ssid = wifiSettings.staSsid;

  if (ssid.isEmpty())
  {
    ssid = "<SSID>";
  }

  displayDrawStartupConnectionScreen("Trying to connect to", ssid);

} //   showStartupWifiConnectionMessage()

//--- Arduino setup
void setup()
{
  Serial.begin(115200);
  delay(100);

#ifdef TEST_COLOR_PATERN
  displayInit();
  displayDrawTestColorPattern();
  ESP_LOGI(logTag, "TEST_COLOR_PATERN enabled: showing color pattern only");

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
  delay(250);

  return;
#endif

  if (!wifiDisabled)
  {
    wifiManagerUpdate();
    webUiUpdate();
  }

  delay(2);

} //   loop()
