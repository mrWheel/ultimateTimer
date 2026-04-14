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

const char* PROG_VERSION = "0.1.0";

//--- Logging tag
static const char *logTag = "main";

//--- Active application settings
static AppSettings activeSettings;

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
static void updateServices();

//--- Handle GPIO00 hold action for clearing WiFi credentials
static bool handleWifiCredentialResetHold();

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

  timerSetSettings(activeSettings);

}   //   loadStartupSettings()

//--- Update external trigger and reset inputs
static void updateExternalInputs()
{
  const AppSettings &settings = timerGetSettings();

  if (ioTriggerActivated(settings.triggerEdge))
  {
    timerHandleExternalTrigger();
  }

  if (ioResetActivated())
  {
    timerHandleExternalReset();
  }

}   //   updateExternalInputs()

//--- Update all service modules
static void updateServices()
{
  encoderUpdate();
  buttonUpdate();
  wifiManagerUpdate();
  webUiUpdate();
  updateExternalInputs();
  timerUpdate();
  ioUpdate(timerGetRuntimeStatus(), timerGetSettings());
  uiMenuUpdate();
  handleWifiCredentialResetHold();

}   //   updateServices()

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
  WiFi.disconnect(true, true);
  delay(100);
  ESP.restart();

  return true;

}   //   handleWifiCredentialResetHold()

//--- Arduino setup
void setup()
{
  Serial.begin(115200);
  delay(100);

  settingsStoreInit();
  profileManagerInit();
  timerInit();
  loadStartupSettings();
  encoderInit();
  buttonInit();
  pinMode(PIN_WIFI_ERASE, INPUT_PULLUP);
  ioInit();
  displayInit();
  wifiManagerInit();
  webUiInit();
  uiMenuInit();

  ESP_LOGI(logTag, "Setup complete");
  ESP_LOGI(logTag, "Open web UI at: http://%s", wifiManagerGetAddressString().c_str());

}   //   setup()

//--- Arduino loop
void loop()
{
  updateServices();

}   //   loop()
