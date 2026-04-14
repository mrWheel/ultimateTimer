#include "webUi.h"
#include "profileManager.h"
#include "settingsStore.h"
#include "timerEngine.h"
#include "WiFiManagerExt.h"

#include <ArduinoJson.h>
#include <WebServer.h>
#include <esp_log.h>

//--- Logging tag
static const char *logTag = "webUi";

//--- Web server instance
static WebServer server(80);

//--- HTML content
static const char *indexHtml = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Universal Timer</title>
<style>
body { font-family: Arial, sans-serif; max-width: 860px; margin: 20px auto; padding: 0 12px; }
.card { border: 1px solid #bbb; border-radius: 8px; padding: 16px; margin-bottom: 16px; }
.cardDisabled { background: #e6e6e6; }
label { display: inline-block; width: 140px; margin-bottom: 6px; }
input, select, button { margin-bottom: 8px; }
.row { margin-bottom: 8px; }
.status { font-family: monospace; white-space: pre-wrap; }
</style>
</head>
<body>
<h1>Universal Timer</h1>

<div class="card">
  <h2>Status</h2>
  <div id="status" class="status">Loading...</div>
  <button onclick="callPost('/api/start')">Start</button>
  <button onclick="callPost('/api/stop')">Stop</button>
  <button onclick="callPost('/api/reset')">Reset</button>
</div>

<div id="timerSettingsCard" class="card">
  <h2>Timer Settings</h2>
  <div class="row"><label>On time</label><input id="onTimeValue" type="number" min="0"></div>
  <div class="row"><label>On unit</label><select id="onTimeUnit"><option value="0">ms</option><option value="1">s</option><option value="2">min</option></select></div>
  <div class="row"><label>Off time</label><input id="offTimeValue" type="number" min="0"></div>
  <div class="row"><label>Off unit</label><select id="offTimeUnit"><option value="0">ms</option><option value="1">s</option><option value="2">min</option></select></div>
  <div class="row"><label>Repeat count</label><input id="repeatCount" type="number" min="0"></div>
  <div class="row"><label>Trigger mode</label><select id="triggerMode"><option value="0">Manual</option><option value="1">External</option></select></div>
  <div class="row"><label>Trigger edge</label><select id="triggerEdge"><option value="0">Falling</option><option value="1">Rising</option></select></div>
  <div class="row"><label>Output polarity</label><select id="outputPolarityHigh"><option value="1">Active high</option><option value="0">Active low</option></select></div>
  <div class="row"><label>Lock input running</label><select id="lockInputDuringRun"><option value="0">No</option><option value="1">Yes</option></select></div>
  <div class="row"><label>Auto save profile</label><select id="autoSaveLastProfile"><option value="1">Yes</option><option value="0">No</option></select></div>
  <button onclick="saveSettings()">Save Settings</button>
</div>

<div id="profilesCard" class="card">
  <h2>Profiles</h2>
  <div class="row"><label>Profile name</label><input id="profileName" type="text"></div>
  <button onclick="saveProfile()">Save Profile</button>
  <button onclick="loadSelectedProfile()">Load Profile</button>
  <button onclick="deleteSelectedProfile()">Delete Profile</button>
  <div class="row"><label>Available</label><select id="profiles"></select></div>
</div>

<script>
let statusRefreshTimer = null;

function setStatusAutoRefresh(enabled)
{
  if (enabled)
  {
    if (statusRefreshTimer === null)
    {
      statusRefreshTimer = setInterval(refreshStatus, 1000);
    }

    return;
  }

  if (statusRefreshTimer !== null)
  {
    clearInterval(statusRefreshTimer);
    statusRefreshTimer = null;
  }
}

function setEditControlsEnabled(enabled)
{
  const cardIds = ['timerSettingsCard', 'profilesCard'];

  for (const cardId of cardIds)
  {
    const card = document.getElementById(cardId);

    if (!card)
    {
      continue;
    }

    const controls = card.querySelectorAll('input, select, button');

    for (const control of controls)
    {
      control.disabled = !enabled;
    }

    card.classList.toggle('cardDisabled', !enabled);
  }
}

async function refreshStatus()
{
  const response = await fetch('/api/status');
  const data = await response.json();

  setStatusAutoRefresh(data.runtime.state !== 0);
  setEditControlsEnabled(data.runtime.state === 0);

  document.getElementById('status').textContent =
    'State: ' + data.runtime.stateLabel + '\n' +
    'On: ' + data.settings.onTimeValue + ' ' + data.settings.onTimeUnitLabel + '\n' +
    'Off: ' + data.settings.offTimeValue + ' ' + data.settings.offTimeUnitLabel + '\n' +
    'Cycles: ' + data.settings.repeatCount + '\n' +
    'Done: ' + data.runtime.currentCycle + '/' + (data.runtime.totalCycles === 0 ? 'INF' : data.runtime.totalCycles) + '\n' +
    'Output: ' + (data.runtime.outputActive ? 'ON' : 'OFF') + '\n' +
    'Profile: ' + data.settings.profileName + '\n' +
    'IP: ' + data.network.address;

  document.getElementById('onTimeValue').value = data.settings.onTimeValue;
  document.getElementById('offTimeValue').value = data.settings.offTimeValue;
  document.getElementById('onTimeUnit').value = data.settings.onTimeUnit;
  document.getElementById('offTimeUnit').value = data.settings.offTimeUnit;
  document.getElementById('repeatCount').value = data.settings.repeatCount;
  document.getElementById('triggerMode').value = data.settings.triggerMode;
  document.getElementById('triggerEdge').value = data.settings.triggerEdge;
  document.getElementById('outputPolarityHigh').value = data.settings.outputPolarityHigh ? '1' : '0';
  document.getElementById('lockInputDuringRun').value = data.settings.lockInputDuringRun ? '1' : '0';
  document.getElementById('autoSaveLastProfile').value = data.settings.autoSaveLastProfile ? '1' : '0';
  document.getElementById('profileName').value = data.settings.profileName;
}

async function refreshProfiles()
{
  const response = await fetch('/api/profiles');
  const data = await response.json();
  const select = document.getElementById('profiles');
  const selectedProfileName = select.value;
  select.innerHTML = '';

  for (const name of data.profiles)
  {
    const option = document.createElement('option');
    option.value = name;
    option.textContent = name;
    select.appendChild(option);
  }

  if (selectedProfileName)
  {
    select.value = selectedProfileName;
  }
}

async function callPost(url, body)
{
  await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: body ? JSON.stringify(body) : '{}'
  });

  await refreshStatus();
  await refreshProfiles();
}

async function saveSettings()
{
  await callPost('/api/settings', readSettingsFromForm());
}

function readSettingsFromForm()
{
  return {
    onTimeValue: Number(document.getElementById('onTimeValue').value),
    offTimeValue: Number(document.getElementById('offTimeValue').value),
    onTimeUnit: Number(document.getElementById('onTimeUnit').value),
    offTimeUnit: Number(document.getElementById('offTimeUnit').value),
    repeatCount: Number(document.getElementById('repeatCount').value),
    triggerMode: Number(document.getElementById('triggerMode').value),
    triggerEdge: Number(document.getElementById('triggerEdge').value),
    outputPolarityHigh: document.getElementById('outputPolarityHigh').value === '1',
    lockInputDuringRun: document.getElementById('lockInputDuringRun').value === '1',
    autoSaveLastProfile: document.getElementById('autoSaveLastProfile').value === '1',
    profileName: document.getElementById('profileName').value
  };
}

let applySettingsTimer = null;

function scheduleApplySettings()
{
  if (applySettingsTimer !== null)
  {
    clearTimeout(applySettingsTimer);
  }

  applySettingsTimer = setTimeout(async () =>
  {
    applySettingsTimer = null;

    try
    {
      await fetch('/api/settings/apply', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(readSettingsFromForm())
      });

      await refreshStatus();
    }
    catch (_error)
    {
    }
  }, 250);
}

function bindLiveApplySettings()
{
  const timerSettingIds = [
    'onTimeValue',
    'offTimeValue',
    'onTimeUnit',
    'offTimeUnit',
    'repeatCount',
    'triggerMode',
    'triggerEdge',
    'outputPolarityHigh',
    'lockInputDuringRun',
    'autoSaveLastProfile',
    'profileName'
  ];

  for (const id of timerSettingIds)
  {
    const element = document.getElementById(id);

    if (!element)
    {
      continue;
    }

    element.addEventListener('input', scheduleApplySettings);
    element.addEventListener('change', scheduleApplySettings);
  }
}

async function saveProfile()
{
  await callPost('/api/profile/save', {
    profileName: document.getElementById('profileName').value
  });
}

async function loadSelectedProfile()
{
  await callPost('/api/profile/load', {
    profileName: document.getElementById('profiles').value
  });
}

async function deleteSelectedProfile()
{
  await callPost('/api/profile/delete', {
    profileName: document.getElementById('profiles').value
  });
}

setInterval(refreshProfiles, 3000);
bindLiveApplySettings();
refreshStatus();
refreshProfiles();
</script>
</body>
</html>
)HTML";

//--- Send JSON response with current status
static void handleStatus();

//--- Update settings from request body
static void handleSaveSettings();

//--- Apply settings from request body without persistence
static void handleApplySettings();

//--- Return profile list
static void handleProfiles();

//--- Save profile from current settings
static void handleSaveProfile();

//--- Load profile into current settings
static void handleLoadProfile();

//--- Delete selected profile
static void handleDeleteProfile();

//--- Parse JSON body
static bool parseJsonBody(JsonDocument &doc);

//--- Fill status document
static void fillStatusDocument(JsonDocument &doc);

//--- Active state
static bool serverRunning = false;

//--- Initialize web UI
void webUiInit()
{
  server.on("/", HTTP_GET, []()
  {
    server.send(200, "text/html", indexHtml);

  });

  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/settings", HTTP_POST, handleSaveSettings);
  server.on("/api/settings/apply", HTTP_POST, handleApplySettings);
  server.on("/api/start", HTTP_POST, []()
  {
    timerStart();
    server.send(200, "application/json", "{\"ok\":true}");

  });
  server.on("/api/stop", HTTP_POST, []()
  {
    timerStop();
    server.send(200, "application/json", "{\"ok\":true}");

  });
  server.on("/api/reset", HTTP_POST, []()
  {
    timerReset();
    server.send(200, "application/json", "{\"ok\":true}");

  });
  server.on("/api/profiles", HTTP_GET, handleProfiles);
  server.on("/api/profile/save", HTTP_POST, handleSaveProfile);
  server.on("/api/profile/load", HTTP_POST, handleLoadProfile);
  server.on("/api/profile/delete", HTTP_POST, handleDeleteProfile);

  server.begin();
  serverRunning = true;

  ESP_LOGI(logTag, "Web UI started");

}   //   webUiInit()

//--- Update web UI
void webUiUpdate()
{
  if (!serverRunning)
  {
    return;
  }

  server.handleClient();

}   //   webUiUpdate()

//--- Suspend web UI server
void webUiSuspend()
{
  if (!serverRunning)
  {
    return;
  }

  server.stop();
  serverRunning = false;

  ESP_LOGI(logTag, "Web UI suspended");

}   //   webUiSuspend()

//--- Resume web UI server
void webUiResume()
{
  if (serverRunning)
  {
    return;
  }

  server.begin();
  serverRunning = true;

  ESP_LOGI(logTag, "Web UI resumed");

}   //   webUiResume()

//--- Send JSON response with current status
static void handleStatus()
{
  JsonDocument doc;
  fillStatusDocument(doc);

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

}   //   handleStatus()

//--- Update settings from request body
static void handleSaveSettings()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  AppSettings settings = timerGetSettings();
  settings.onTimeValue = doc["onTimeValue"] | settings.onTimeValue;
  settings.offTimeValue = doc["offTimeValue"] | settings.offTimeValue;
  settings.onTimeUnit = static_cast<TimeUnit>(doc["onTimeUnit"] | static_cast<int>(settings.onTimeUnit));
  settings.offTimeUnit = static_cast<TimeUnit>(doc["offTimeUnit"] | static_cast<int>(settings.offTimeUnit));
  settings.repeatCount = static_cast<uint32_t>(doc["repeatCount"] | settings.repeatCount);
  settings.triggerMode = static_cast<TriggerMode>(doc["triggerMode"] | static_cast<int>(settings.triggerMode));
  settings.triggerEdge = static_cast<TriggerEdge>(doc["triggerEdge"] | static_cast<int>(settings.triggerEdge));
  settings.outputPolarityHigh = doc["outputPolarityHigh"] | settings.outputPolarityHigh;
  settings.lockInputDuringRun = doc["lockInputDuringRun"] | settings.lockInputDuringRun;
  settings.autoSaveLastProfile = doc["autoSaveLastProfile"] | settings.autoSaveLastProfile;
  settings.profileName = String(static_cast<const char *>(doc["profileName"] | settings.profileName.c_str()));

  timerSetSettings(settings);

  if (settings.autoSaveLastProfile && !settings.profileName.isEmpty())
  {
    profileManagerSaveProfile(settings.profileName, settings);
    settingsStoreSaveLastProfileName(settings.profileName);
  }

  handleStatus();

}   //   handleSaveSettings()

//--- Apply settings from request body without persistence
static void handleApplySettings()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  AppSettings settings = timerGetSettings();
  settings.onTimeValue = doc["onTimeValue"] | settings.onTimeValue;
  settings.offTimeValue = doc["offTimeValue"] | settings.offTimeValue;
  settings.onTimeUnit = static_cast<TimeUnit>(doc["onTimeUnit"] | static_cast<int>(settings.onTimeUnit));
  settings.offTimeUnit = static_cast<TimeUnit>(doc["offTimeUnit"] | static_cast<int>(settings.offTimeUnit));
  settings.repeatCount = static_cast<uint32_t>(doc["repeatCount"] | settings.repeatCount);
  settings.triggerMode = static_cast<TriggerMode>(doc["triggerMode"] | static_cast<int>(settings.triggerMode));
  settings.triggerEdge = static_cast<TriggerEdge>(doc["triggerEdge"] | static_cast<int>(settings.triggerEdge));
  settings.outputPolarityHigh = doc["outputPolarityHigh"] | settings.outputPolarityHigh;
  settings.lockInputDuringRun = doc["lockInputDuringRun"] | settings.lockInputDuringRun;
  settings.autoSaveLastProfile = doc["autoSaveLastProfile"] | settings.autoSaveLastProfile;
  settings.profileName = String(static_cast<const char *>(doc["profileName"] | settings.profileName.c_str()));

  timerSetSettings(settings);
  handleStatus();

}   //   handleApplySettings()

//--- Return profile list
static void handleProfiles()
{
  String names[profileManagerMaxProfiles];
  size_t count = profileManagerListProfiles(names, profileManagerMaxProfiles);

  JsonDocument doc;
  JsonArray array = doc["profiles"].to<JsonArray>();

  for (size_t i = 0; i < count; i++)
  {
    array.add(names[i]);
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

}   //   handleProfiles()

//--- Save profile from current settings
static void handleSaveProfile()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  AppSettings settings = timerGetSettings();
  String profileName = String(static_cast<const char *>(doc["profileName"] | settings.profileName.c_str()));
  settings.profileName = profileName;

  bool ok = profileManagerSaveProfile(profileName, settings);

  if (ok)
  {
    settingsStoreSaveLastProfileName(profileName);
    timerSetSettings(settings);
  }

  server.send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");

}   //   handleSaveProfile()

//--- Load profile into current settings
static void handleLoadProfile()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  AppSettings settings = timerGetSettings();
  String profileName = String(static_cast<const char *>(doc["profileName"] | settings.profileName.c_str()));
  bool ok = profileManagerLoadProfile(profileName, settings);

  if (ok)
  {
    timerSetSettings(settings);
    settingsStoreSaveLastProfileName(profileName);
  }

  server.send(ok ? 200 : 404, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");

}   //   handleLoadProfile()

//--- Delete selected profile
static void handleDeleteProfile()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  AppSettings settings = timerGetSettings();
  String profileName = String(static_cast<const char *>(doc["profileName"] | settings.profileName.c_str()));
  bool ok = profileManagerDeleteProfile(profileName);

  server.send(ok ? 200 : 404, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");

}   //   handleDeleteProfile()

//--- Parse JSON body
static bool parseJsonBody(JsonDocument &doc)
{
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  return !error;

}   //   parseJsonBody()

//--- Fill status document
static void fillStatusDocument(JsonDocument &doc)
{
  const AppSettings &settings = timerGetSettings();
  RuntimeStatus runtimeStatus = timerGetRuntimeStatus();

  doc["settings"]["onTimeValue"] = settings.onTimeValue;
  doc["settings"]["offTimeValue"] = settings.offTimeValue;
  doc["settings"]["onTimeUnit"] = static_cast<int>(settings.onTimeUnit);
  doc["settings"]["offTimeUnit"] = static_cast<int>(settings.offTimeUnit);
  doc["settings"]["onTimeUnitLabel"] = timerGetTimeUnitLabel(settings.onTimeUnit);
  doc["settings"]["offTimeUnitLabel"] = timerGetTimeUnitLabel(settings.offTimeUnit);
  doc["settings"]["repeatCount"] = settings.repeatCount;
  doc["settings"]["triggerMode"] = static_cast<int>(settings.triggerMode);
  doc["settings"]["triggerEdge"] = static_cast<int>(settings.triggerEdge);
  doc["settings"]["outputPolarityHigh"] = settings.outputPolarityHigh;
  doc["settings"]["lockInputDuringRun"] = settings.lockInputDuringRun;
  doc["settings"]["autoSaveLastProfile"] = settings.autoSaveLastProfile;
  doc["settings"]["profileName"] = settings.profileName;

  doc["runtime"]["state"] = static_cast<int>(runtimeStatus.state);
  doc["runtime"]["stateLabel"] = timerGetStateLabel(runtimeStatus.state);
  doc["runtime"]["outputActive"] = runtimeStatus.outputActive;
  doc["runtime"]["currentCycle"] = runtimeStatus.currentCycle;
  doc["runtime"]["totalCycles"] = runtimeStatus.totalCycles;
  doc["runtime"]["currentPhaseDurationMs"] = runtimeStatus.currentPhaseDurationMs;
  doc["runtime"]["currentPhaseElapsedMs"] = runtimeStatus.currentPhaseElapsedMs;
  doc["runtime"]["inOnPhase"] = runtimeStatus.inOnPhase;

  doc["network"]["connected"] = wifiManagerIsStaConnected();
  doc["network"]["address"] = wifiManagerGetAddressString();

}   //   fillStatusDocument()
