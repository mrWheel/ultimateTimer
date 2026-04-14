#include "settingsStore.h"
#include "appConfig.h"

#include <Preferences.h>
#include <esp_log.h>

//--- Logging tag
static const char *logTag = "settingsStore";

//--- Preferences instance
static Preferences preferences;

//--- Preferences namespace
static const char *preferencesNamespace = "univtimer";

//--- Initialize settings storage
void settingsStoreInit()
{
  preferences.begin(preferencesNamespace, false);

  ESP_LOGI(logTag, "Settings storage initialized");

}   //   settingsStoreInit()

//--- Load WiFi settings
void settingsStoreLoadWifiSettings(WifiSettings &wifiSettings)
{
  wifiSettings.staSsid = preferences.getString("staSsid", "");
  wifiSettings.staPassword = preferences.getString("staPass", "");
  wifiSettings.apSsid = preferences.getString("apSsid", DEFAULT_AP_SSID);
  wifiSettings.apPassword = preferences.getString("apPass", DEFAULT_AP_PASSWORD);
  wifiSettings.hostName = preferences.getString("hostName", DEFAULT_WIFI_HOSTNAME);

}   //   settingsStoreLoadWifiSettings()

//--- Save WiFi settings
void settingsStoreSaveWifiSettings(const WifiSettings &wifiSettings)
{
  preferences.putString("staSsid", wifiSettings.staSsid);
  preferences.putString("staPass", wifiSettings.staPassword);
  preferences.putString("apSsid", wifiSettings.apSsid);
  preferences.putString("apPass", wifiSettings.apPassword);
  preferences.putString("hostName", wifiSettings.hostName);

  ESP_LOGI(logTag, "WiFi settings saved");

}   //   settingsStoreSaveWifiSettings()

//--- Erase stored station WiFi credentials
void settingsStoreEraseWifiCredentials()
{
  preferences.putString("staSsid", "");
  preferences.putString("staPass", "");

  ESP_LOGI(logTag, "Stored STA WiFi credentials erased");

}   //   settingsStoreEraseWifiCredentials()

//--- Load last profile name
String settingsStoreLoadLastProfileName()
{
  return preferences.getString("lastProf", "");

}   //   settingsStoreLoadLastProfileName()

//--- Save last profile name
void settingsStoreSaveLastProfileName(const String &profileName)
{
  preferences.putString("lastProf", profileName);

  ESP_LOGI(logTag, "Last profile saved: %s", profileName.c_str());

}   //   settingsStoreSaveLastProfileName()
