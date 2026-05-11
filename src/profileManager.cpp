/*** Last Changed: 2026-05-11 - 16:24 ***/
#include "profileManager.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <esp_log.h>

//--- Logging tag
static const char* logTag = "profileManager";
const char* const profileManagerDefaultProfileName = "default";

//--- Build file path for profile
static String buildProfilePath(const String& profileName);

//--- Sanitize profile name
static String sanitizeProfileName(const String& profileName);

//--- Read settings from JSON document
static void loadSettingsFromJson(const JsonDocument& doc, const String& profileName, AppSettings& settings);

//--- Write settings into JSON document
static void saveSettingsToJson(JsonDocument& doc, const AppSettings& settings);

//--- Initialize profile manager
bool profileManagerInit()
{
  bool ok = LittleFS.begin(true);

  if (!ok)
  {
    ESP_LOGE(logTag, "Error: LittleFS mount failed");
  }
  else
  {
    ESP_LOGI(logTag, "Profile manager initialized");
  }

  return ok;

} //   profileManagerInit()

//--- Save profile
bool profileManagerSaveProfile(const String& profileName, const AppSettings& settings)
{
  String safeName = sanitizeProfileName(profileName);

  if (safeName.isEmpty())
  {
    ESP_LOGE(logTag, "Error: Empty profile name");

    return false;
  }

  //-- Add "24h" suffix for 24h timer profiles
  if (settings.timerType == TIMER_TYPE_24H && !safeName.endsWith("24h"))
  {
    safeName += "24h";
  }

  String path = buildProfilePath(safeName);
  File file = LittleFS.open(path, "w");

  if (!file)
  {
    ESP_LOGE(logTag, "Error: Failed to open profile for writing: %s", path.c_str());

    return false;
  }

  JsonDocument doc;
  saveSettingsToJson(doc, settings);

  if (serializeJsonPretty(doc, file) == 0)
  {
    file.close();
    ESP_LOGE(logTag, "Error: Failed to serialize JSON");

    return false;
  }

  file.close();

  ESP_LOGI(logTag, "Profile saved: %s", safeName.c_str());

  return true;

} //   profileManagerSaveProfile()

//--- Load profile
bool profileManagerLoadProfile(const String& profileName, AppSettings& settings)
{
  String safeName = sanitizeProfileName(profileName);
  String path = buildProfilePath(safeName);
  File file = LittleFS.open(path, "r");

  if (!file)
  {
    ESP_LOGE(logTag, "Error: Failed to open profile for reading: %s", path.c_str());

    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error)
  {
    ESP_LOGE(logTag, "Error: Failed to parse JSON: %s", error.c_str());

    return false;
  }

  loadSettingsFromJson(doc, safeName, settings);
  settings.profileName = safeName;

  ESP_LOGI(logTag, "Profile loaded: %s", safeName.c_str());

  return true;

} //   profileManagerLoadProfile()

//--- Delete profile
bool profileManagerDeleteProfile(const String& profileName)
{
  String safeName = sanitizeProfileName(profileName);

  if (safeName.equalsIgnoreCase(profileManagerDefaultProfileName) || safeName.equalsIgnoreCase("default24h"))
  {
    ESP_LOGW(logTag, "Refused to delete default profile: %s", safeName.c_str());

    return false;
  }

  String path = buildProfilePath(safeName);

  if (!LittleFS.exists(path))
  {
    return false;
  }

  bool ok = LittleFS.remove(path);

  if (ok)
  {
    ESP_LOGI(logTag, "Profile deleted: %s", safeName.c_str());
  }
  else
  {
    ESP_LOGE(logTag, "Error: Failed to delete profile: %s", safeName.c_str());
  }

  return ok;

} //   profileManagerDeleteProfile()

//--- Get profile names
size_t profileManagerListProfiles(String profileNames[], size_t maxProfiles)
{
  size_t count = 0;
  File root = LittleFS.open("/");

  if (!root || !root.isDirectory())
  {
    return 0;
  }

  File file = root.openNextFile();

  while (file && count < maxProfiles)
  {
    String name = file.name();

    if (name.endsWith(".json"))
    {
      name.replace("/", "");
      name.replace(".json", "");
      profileNames[count] = name;
      count++;
    }

    file = root.openNextFile();
  }

  return count;

} //   profileManagerListProfiles()

//--- Check whether profile exists
bool profileManagerProfileExists(const String& profileName)
{
  String path = buildProfilePath(profileName);

  return LittleFS.exists(path);

} //   profileManagerProfileExists()

//--- Build file path for profile
static String buildProfilePath(const String& profileName)
{
  String safeName = sanitizeProfileName(profileName);

  return "/" + safeName + ".json";

} //   buildProfilePath()

//--- Sanitize profile name
static String sanitizeProfileName(const String& profileName)
{
  String safeName = profileName;
  safeName.trim();

  String filtered = "";

  for (size_t i = 0; i < safeName.length(); i++)
  {
    char c = safeName.charAt(i);

    if (isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.')
    {
      filtered += c;
    }
  }

  return filtered;

} //   sanitizeProfileName()

//--- Read settings from JSON document
static void loadSettingsFromJson(const JsonDocument& doc, const String& profileName, AppSettings& settings)
{
  //-- Explicit timerType from JSON takes priority; otherwise infer from profile naming convention.
  if (!doc["timerType"].isNull())
  {
    settings.timerType = static_cast<TimerType>(doc["timerType"].as<int>());
  }
  else
  {
    settings.timerType = profileName.endsWith("24h") ? TIMER_TYPE_24H : TIMER_TYPE_CYCLIC;
  }

  settings.onTimeValue = doc["onTimeValue"] | settings.onTimeValue;
  settings.offTimeValue = doc["offTimeValue"] | settings.offTimeValue;
  settings.onTimeUnit = static_cast<TimeUnit>(doc["onTimeUnit"] | static_cast<int>(settings.onTimeUnit));
  settings.offTimeUnit = static_cast<TimeUnit>(doc["offTimeUnit"] | static_cast<int>(settings.offTimeUnit));
  settings.repeatCount = doc["repeatCount"] | settings.repeatCount;
  settings.triggerMode = static_cast<TriggerMode>(doc["triggerMode"] | static_cast<int>(settings.triggerMode));
  settings.triggerEdge = static_cast<TriggerEdge>(doc["triggerEdge"] | static_cast<int>(settings.triggerEdge));

  JsonArrayConst quarterStates = doc["timer24hQuarterStates"].as<JsonArrayConst>();

  if (!quarterStates.isNull())
  {
    for (size_t stateIndex = 0; stateIndex < quarterStates.size() && stateIndex < sizeof(settings.timer24hQuarterStates); stateIndex++)
    {
      settings.timer24hQuarterStates[stateIndex] = quarterStates[stateIndex] | static_cast<uint8_t>(TIMER_24H_QUARTER_OFF);
    }
  }

} //   loadSettingsFromJson()

//--- Write settings into JSON document
static void saveSettingsToJson(JsonDocument& doc, const AppSettings& settings)
{
  doc["timerType"] = static_cast<int>(settings.timerType);
  doc["onTimeValue"] = settings.onTimeValue;
  doc["offTimeValue"] = settings.offTimeValue;
  doc["onTimeUnit"] = static_cast<int>(settings.onTimeUnit);
  doc["offTimeUnit"] = static_cast<int>(settings.offTimeUnit);
  doc["repeatCount"] = settings.repeatCount;
  doc["triggerMode"] = static_cast<int>(settings.triggerMode);
  doc["triggerEdge"] = static_cast<int>(settings.triggerEdge);

  JsonArray quarterStates = doc["timer24hQuarterStates"].to<JsonArray>();

  for (size_t stateIndex = 0; stateIndex < sizeof(settings.timer24hQuarterStates); stateIndex++)
  {
    quarterStates.add(settings.timer24hQuarterStates[stateIndex]);
  }

} //   saveSettingsToJson()
