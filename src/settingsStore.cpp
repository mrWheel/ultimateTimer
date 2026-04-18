/*** Last Changed: 2026-04-18 - 15:49 ***/
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

//--- Load system-level application settings
void settingsStoreLoadSystemSettings(AppSettings& settings)
{
  settings.triggerMode = static_cast<TriggerMode>(preferences.getUChar("trigMode", static_cast<uint8_t>(DEFAULT_TRIGGER_MODE)));
  settings.triggerEdge = static_cast<TriggerEdge>(preferences.getUChar("trigEdge", static_cast<uint8_t>(DEFAULT_TRIGGER_EDGE)));
  settings.outputPolarityHigh = preferences.getBool("outHigh", DEFAULT_OUTPUT_POLARITY != 0);
  settings.lockInputDuringRun = preferences.getBool("lockRun", DEFAULT_LOCK_INPUT_DURING_RUN != 0);
  settings.autoSaveLastProfile = preferences.getBool("autoSave", DEFAULT_AUTO_SAVE_LAST_PROFILE != 0);

} //   settingsStoreLoadSystemSettings()

//--- Save system-level application settings
void settingsStoreSaveSystemSettings(const AppSettings& settings)
{
  preferences.putUChar("trigMode", static_cast<uint8_t>(settings.triggerMode));
  preferences.putUChar("trigEdge", static_cast<uint8_t>(settings.triggerEdge));
  preferences.putBool("outHigh", settings.outputPolarityHigh);
  preferences.putBool("lockRun", settings.lockInputDuringRun);
  preferences.putBool("autoSave", settings.autoSaveLastProfile);

  ESP_LOGI(logTag, "System settings saved");

} //   settingsStoreSaveSystemSettings()

//--- Load theme color index (0-based, default from DEFAULT_THEME_COLOR build flag)
uint8_t settingsStoreLoadThemeColorIndex()
{
#ifdef DEFAULT_THEME_COLOR
  const uint8_t defaultIndex = static_cast<uint8_t>(DEFAULT_THEME_COLOR - 1);
#else
  const uint8_t defaultIndex = 2U;
#endif

  return preferences.getUChar("themeClr", defaultIndex);

} //   settingsStoreLoadThemeColorIndex()

//--- Save theme color index
void settingsStoreSaveThemeColorIndex(uint8_t index)
{
  preferences.putUChar("themeClr", index);

  ESP_LOGI(logTag, "Theme color index saved: %u", static_cast<unsigned>(index));

} //   settingsStoreSaveThemeColorIndex()
