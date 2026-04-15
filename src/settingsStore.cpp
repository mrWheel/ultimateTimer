/*** Last Changed: 2026-04-15 - 14:23 ***/
#include "settingsStore.h"
#include "appConfig.h"

#include <Preferences.h>
#include <esp_log.h>

//--- Logging tag
static const char* logTag = "settingsStore";

//--- Preferences instance
static Preferences preferences;

//--- Preferences namespace
static const char* preferencesNamespace = "univtimer";

//--- Initialize settings storage
void settingsStoreInit()
{
  preferences.begin(preferencesNamespace, false);

  ESP_LOGI(logTag, "Settings storage initialized");

} //   settingsStoreInit()

//--- Load WiFi settings
void settingsStoreLoadWifiSettings(WifiSettings& wifiSettings)
{
  wifiSettings.staSsid = preferences.getString("staSsid", "");
  wifiSettings.staPassword = preferences.getString("staPass", "");
  wifiSettings.apSsid = preferences.getString("apSsid", DEFAULT_AP_SSID);
  wifiSettings.apPassword = preferences.getString("apPass", DEFAULT_AP_PASSWORD);
  wifiSettings.hostName = preferences.getString("hostName", DEFAULT_WIFI_HOSTNAME);

} //   settingsStoreLoadWifiSettings()

//--- Save WiFi settings
void settingsStoreSaveWifiSettings(const WifiSettings& wifiSettings)
{
  preferences.putString("staSsid", wifiSettings.staSsid);
  preferences.putString("staPass", wifiSettings.staPassword);
  preferences.putString("apSsid", wifiSettings.apSsid);
  preferences.putString("apPass", wifiSettings.apPassword);
  preferences.putString("hostName", wifiSettings.hostName);

  ESP_LOGI(logTag, "WiFi settings saved");

} //   settingsStoreSaveWifiSettings()

//--- Erase stored station WiFi credentials
void settingsStoreEraseWifiCredentials()
{
  preferences.putString("staSsid", "");
  preferences.putString("staPass", "");

  ESP_LOGI(logTag, "Stored STA WiFi credentials erased");

} //   settingsStoreEraseWifiCredentials()

//--- Load WiFi disabled indicator
bool settingsStoreLoadWifiDisabled()
{
  return preferences.getBool("wifiOff", false);

} //   settingsStoreLoadWifiDisabled()

//--- Save WiFi disabled indicator
void settingsStoreSaveWifiDisabled(bool disabled)
{
  preferences.putBool("wifiOff", disabled);

  ESP_LOGI(logTag, "WiFi disabled saved: %s", disabled ? "true" : "false");

} //   settingsStoreSaveWifiDisabled()

//--- Load last profile name
String settingsStoreLoadLastProfileName()
{
  return preferences.getString("lastProf", "");

} //   settingsStoreLoadLastProfileName()

//--- Save last profile name
void settingsStoreSaveLastProfileName(const String& profileName)
{
  preferences.putString("lastProf", profileName);

  ESP_LOGI(logTag, "Last profile saved: %s", profileName.c_str());

} //   settingsStoreSaveLastProfileName()

//--- Load encoder direction reversal state
bool settingsStoreLoadEncoderDirectionReversed()
{
  return preferences.getBool("encRev", false);

} //   settingsStoreLoadEncoderDirectionReversed()

//--- Save encoder direction reversal state
void settingsStoreSaveEncoderDirectionReversed(bool reversed)
{
  preferences.putBool("encRev", reversed);

  ESP_LOGI(logTag, "Encoder direction reversed saved: %s", reversed ? "true" : "false");

} //   settingsStoreSaveEncoderDirectionReversed()

//--- Load output polarity default state
bool settingsStoreLoadOutputPolarityHigh()
{
  return preferences.getBool("outHigh", true);

} //   settingsStoreLoadOutputPolarityHigh()

//--- Save output polarity default state
void settingsStoreSaveOutputPolarityHigh(bool activeHigh)
{
  preferences.putBool("outHigh", activeHigh);

  ESP_LOGI(logTag, "Output polarity activeHigh saved: %s", activeHigh ? "true" : "false");

} //   settingsStoreSaveOutputPolarityHigh()
