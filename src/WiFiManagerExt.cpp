#include "WiFiManagerExt.h"

#include "settingsStore.h"
#include "webUi.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_log.h>

//--- Logging tag
static const char *logTag = "WiFiManagerExt";

//--- WiFiManager instance
static WiFiManager wm;

//--- Cached settings
static WifiSettings currentSettings;

//--- Portal and retry state
static bool portalActive = false;
static bool newCredentialsPending = false;
static uint32_t lastRetryMs = 0;
static uint8_t retryCount = 0;

//--- Retry policy before opening portal
static const uint8_t maxRetriesBeforePortal = 2;
static const uint32_t retryIntervalMs = 15000UL;

//--- Start non-blocking config portal
static void startPortal();

//--- Start station connection attempt
static void startStationAttempt();

//--- Initialize WiFi manager service
void wifiManagerInit()
{
  settingsStoreLoadWifiSettings(currentSettings);
  currentSettings.apPassword = "";
  wifiManagerExtInit(currentSettings);

  ESP_LOGI(logTag, "AP started: %s", currentSettings.apSsid.c_str());

}   //   wifiManagerInit()

//--- Update WiFi manager service
void wifiManagerUpdate()
{
  wifiManagerExtUpdate();

  WifiSettings newSettings;

  if (wifiManagerExtConsumeNewStaCredentials(newSettings))
  {
    newSettings.apPassword = "";
    currentSettings = newSettings;
    settingsStoreSaveWifiSettings(currentSettings);

    ESP_LOGI(logTag, "Saved new STA credentials from portal");
  }

}   //   wifiManagerUpdate()

//--- Apply stored WiFi settings
void wifiManagerApplySettings(const WifiSettings &newWifiSettings)
{
  currentSettings = newWifiSettings;
  currentSettings.apPassword = "";
  wifiManagerExtApplySettings(currentSettings);

  ESP_LOGI(logTag, "WiFi settings applied");

}   //   wifiManagerApplySettings()

//--- Get current WiFi settings
const WifiSettings &wifiManagerGetSettings()
{
  return currentSettings;

}   //   wifiManagerGetSettings()

//--- Save and apply WiFi settings
void wifiManagerSaveAndApplySettings(const WifiSettings &newWifiSettings)
{
  WifiSettings updatedSettings = newWifiSettings;
  updatedSettings.apPassword = "";

  settingsStoreSaveWifiSettings(updatedSettings);
  wifiManagerApplySettings(updatedSettings);

}   //   wifiManagerSaveAndApplySettings()

//--- Scan available access points and return unique SSIDs
size_t wifiManagerScanNetworks(String ssids[], size_t maxCount)
{
  if (maxCount == 0)
  {
    return 0;
  }

  int foundCount = WiFi.scanNetworks(false, true);

  if (foundCount <= 0)
  {
    WiFi.scanDelete();

    return 0;
  }

  size_t outputCount = 0;

  for (int i = 0; i < foundCount && outputCount < maxCount; i++)
  {
    String ssid = WiFi.SSID(i);

    if (ssid.isEmpty())
    {
      continue;
    }

    bool duplicate = false;

    for (size_t j = 0; j < outputCount; j++)
    {
      if (ssids[j] == ssid)
      {
        duplicate = true;
        break;
      }
    }

    if (!duplicate)
    {
      ssids[outputCount] = ssid;
      outputCount++;
    }
  }

  WiFi.scanDelete();

  ESP_LOGI(logTag, "Scanned APs: %u", static_cast<unsigned int>(outputCount));

  return outputCount;

}   //   wifiManagerScanNetworks()

//--- Check whether station is connected
bool wifiManagerIsStaConnected()
{
  return wifiManagerExtIsStaConnected();

}   //   wifiManagerIsStaConnected()

//--- Check whether WiFi setup portal should be shown
bool wifiManagerShouldOpenPortal()
{
  return wifiManagerExtIsPortalActive();

}   //   wifiManagerShouldOpenPortal()

//--- Get local URL string
String wifiManagerGetAddressString()
{
  return wifiManagerExtGetAddressString();

}   //   wifiManagerGetAddressString()

//--- Start station connection attempt
static void startStationAttempt()
{
  if (currentSettings.staSsid.isEmpty())
  {
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(currentSettings.hostName.c_str());
  WiFi.begin(currentSettings.staSsid.c_str(), currentSettings.staPassword.c_str());

  retryCount++;
  lastRetryMs = millis();

  ESP_LOGI(logTag, "Connecting to STA SSID: %s", currentSettings.staSsid.c_str());

}   //   startStationAttempt()

//--- Start non-blocking config portal
static void startPortal()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(currentSettings.hostName.c_str());

  //--- Stop app web server so WiFiManager can own port 80
  webUiSuspend();

  //--- Trigger async scan at portal start so AP list is ready on first page visit
  wm._preloadwifiscan = true;

  wm.setConfigPortalBlocking(false);
  wm.startConfigPortal(currentSettings.apSsid.c_str());

  portalActive = true;

  ESP_LOGI(logTag, "WiFi config portal started on AP: %s", currentSettings.apSsid.c_str());

}   //   startPortal()

//--- Initialize WiFiManager extension
void wifiManagerExtInit(const WifiSettings &wifiSettings)
{
  currentSettings = wifiSettings;
  portalActive = false;
  newCredentialsPending = false;
  retryCount = 0;
  lastRetryMs = 0;

  if (currentSettings.staSsid.isEmpty())
  {
    startPortal();

    return;
  }

  startStationAttempt();

}   //   wifiManagerExtInit()

//--- Update WiFiManager extension
void wifiManagerExtUpdate()
{
  if (portalActive)
  {
    wm.process();

    if (WiFi.status() == WL_CONNECTED)
    {
      portalActive = false;
      retryCount = 0;
      newCredentialsPending = true;

      //--- Hand port 80 back to the app web server
      webUiResume();

      ESP_LOGI(logTag, "Portal completed, STA connected: %s", WiFi.SSID().c_str());
    }

    return;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    retryCount = 0;

    return;
  }

  if (retryCount >= maxRetriesBeforePortal)
  {
    startPortal();

    return;
  }

  if (millis() - lastRetryMs >= retryIntervalMs)
  {
    startStationAttempt();
  }

}   //   wifiManagerExtUpdate()

//--- Apply WiFi settings and reconnect flow
void wifiManagerExtApplySettings(const WifiSettings &wifiSettings)
{
  currentSettings = wifiSettings;
  portalActive = false;
  newCredentialsPending = false;
  retryCount = 0;

  WiFi.disconnect(false, true);
  delay(50);

  if (currentSettings.staSsid.isEmpty())
  {
    startPortal();

    return;
  }

  startStationAttempt();

}   //   wifiManagerExtApplySettings()

//--- Check whether STA is connected
bool wifiManagerExtIsStaConnected()
{
  return WiFi.status() == WL_CONNECTED;

}   //   wifiManagerExtIsStaConnected()

//--- Check whether config portal is active
bool wifiManagerExtIsPortalActive()
{
  return portalActive;

}   //   wifiManagerExtIsPortalActive()

//--- Get current address string (STA or AP)
String wifiManagerExtGetAddressString()
{
  if (wifiManagerExtIsStaConnected())
  {
    return WiFi.localIP().toString();
  }

  return WiFi.softAPIP().toString();

}   //   wifiManagerExtGetAddressString()

//--- Consume newly configured STA credentials from portal
bool wifiManagerExtConsumeNewStaCredentials(WifiSettings &wifiSettings)
{
  if (!newCredentialsPending)
  {
    return false;
  }

  wifiSettings = currentSettings;
  wifiSettings.staSsid = WiFi.SSID();
  wifiSettings.staPassword = WiFi.psk();

  newCredentialsPending = false;

  return true;

}   //   wifiManagerExtConsumeNewStaCredentials()
