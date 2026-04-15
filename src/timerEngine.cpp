/*** Last Changed: 2026-04-15 - 14:23 ***/
#include "timerEngine.h"
#include "appConfig.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

//--- Logging tag
static const char* logTag = "timerEngine";

//--- Internal settings
static AppSettings appSettings;

//--- Internal runtime status
static RuntimeStatus runtimeStatus;

//--- Internal timing
static uint32_t phaseStartMs = 0;

//--- Shared state protection
static SemaphoreHandle_t timerMutex = nullptr;

//--- Start on phase
static void startOnPhase();

//--- Start off phase
static void startOffPhase();

//--- Apply common settings constraints
static void sanitizeSettings(AppSettings& settings, bool enforceMsMinimum);

//--- Check whether timer is busy while timer mutex is held
static bool timerIsBusyLocked();

//--- Lock timer state
static inline void lockTimerState()
{
  if (timerMutex != nullptr)
  {
    xSemaphoreTake(timerMutex, portMAX_DELAY);
  }

} //   lockTimerState()

//--- Unlock timer state
static inline void unlockTimerState()
{
  if (timerMutex != nullptr)
  {
    xSemaphoreGive(timerMutex);
  }

} //   unlockTimerState()

//--- Initialize timer engine
void timerInit()
{
  if (timerMutex == nullptr)
  {
    timerMutex = xSemaphoreCreateMutex();
  }

  timerLoadDefaultSettings(appSettings);

  lockTimerState();

  runtimeStatus.state = TIMER_STATE_IDLE;
  runtimeStatus.outputActive = false;
  runtimeStatus.currentCycle = 0;
  runtimeStatus.totalCycles = appSettings.repeatCount;
  runtimeStatus.currentPhaseDurationMs = 0;
  runtimeStatus.currentPhaseElapsedMs = 0;
  runtimeStatus.inOnPhase = true;
  phaseStartMs = 0;

  unlockTimerState();

  ESP_LOGI(logTag, "Timer engine initialized");

} //   timerInit()

//--- Update timer engine
void timerUpdate()
{
  lockTimerState();

  uint32_t nowMs = millis();

  if (runtimeStatus.state != TIMER_STATE_RUNNING)
  {
    unlockTimerState();

    return;
  }

  runtimeStatus.currentPhaseElapsedMs = nowMs - phaseStartMs;

  if (runtimeStatus.currentPhaseElapsedMs < runtimeStatus.currentPhaseDurationMs)
  {
    unlockTimerState();

    return;
  }

  if (runtimeStatus.inOnPhase)
  {
    startOffPhase();
    unlockTimerState();

    return;
  }

  runtimeStatus.currentCycle++;

  if (runtimeStatus.totalCycles > 0 && runtimeStatus.currentCycle >= runtimeStatus.totalCycles)
  {
    runtimeStatus.state = TIMER_STATE_FINISHED;
    runtimeStatus.outputActive = false;
    runtimeStatus.currentPhaseElapsedMs = runtimeStatus.currentPhaseDurationMs;

    unlockTimerState();

    ESP_LOGI(logTag, "Timer finished");

    return;
  }

  startOnPhase();
  unlockTimerState();

} //   timerUpdate()

//--- Start timer cycle
void timerStart()
{
  lockTimerState();

  if (runtimeStatus.state == TIMER_STATE_FINISHED)
  {
    unlockTimerState();
    ESP_LOGD(logTag, "Start ignored: timer finished, reset required");

    return;
  }

  if (runtimeStatus.state == TIMER_STATE_RUNNING || runtimeStatus.state == TIMER_STATE_PAUSED)
  {
    unlockTimerState();
    ESP_LOGD(logTag, "Start ignored: timer already active");

    return;
  }

  sanitizeSettings(appSettings, true);
  runtimeStatus.totalCycles = appSettings.repeatCount;

  if (runtimeStatus.totalCycles > 0 && runtimeStatus.currentCycle >= runtimeStatus.totalCycles)
  {
    runtimeStatus.state = TIMER_STATE_FINISHED;
    runtimeStatus.outputActive = false;

    unlockTimerState();
    ESP_LOGD(logTag, "Start ignored: cycles exhausted, reset required");

    return;
  }

  runtimeStatus.state = TIMER_STATE_RUNNING;
  startOnPhase();

  unlockTimerState();
  ESP_LOGI(logTag, "Timer started");

} //   timerStart()

//--- Stop timer cycle
void timerStop()
{
  lockTimerState();

  runtimeStatus.state = TIMER_STATE_IDLE;
  runtimeStatus.outputActive = false;
  runtimeStatus.currentPhaseDurationMs = 0;
  runtimeStatus.currentPhaseElapsedMs = 0;
  runtimeStatus.inOnPhase = true;

  unlockTimerState();
  ESP_LOGI(logTag, "Timer stopped");

} //   timerStop()

//--- Pause timer cycle
void timerPause()
{
  lockTimerState();

  if (runtimeStatus.state != TIMER_STATE_RUNNING)
  {
    unlockTimerState();

    return;
  }

  runtimeStatus.state = TIMER_STATE_PAUSED;
  runtimeStatus.outputActive = false;

  unlockTimerState();
  ESP_LOGI(logTag, "Timer paused");

} //   timerPause()

//--- Resume timer cycle
void timerResume()
{
  lockTimerState();

  if (runtimeStatus.state != TIMER_STATE_PAUSED)
  {
    unlockTimerState();

    return;
  }

  runtimeStatus.state = TIMER_STATE_RUNNING;
  phaseStartMs = millis() - runtimeStatus.currentPhaseElapsedMs;
  runtimeStatus.outputActive = runtimeStatus.inOnPhase;

  unlockTimerState();
  ESP_LOGI(logTag, "Timer resumed");

} //   timerResume()

//--- Reset timer cycle
void timerReset()
{
  lockTimerState();

  runtimeStatus.state = TIMER_STATE_IDLE;
  runtimeStatus.outputActive = false;
  runtimeStatus.currentCycle = 0;
  runtimeStatus.totalCycles = appSettings.repeatCount;
  runtimeStatus.currentPhaseDurationMs = 0;
  runtimeStatus.currentPhaseElapsedMs = 0;
  runtimeStatus.inOnPhase = true;

  unlockTimerState();
  ESP_LOGI(logTag, "Timer reset");

} //   timerReset()

//--- Request external trigger
void timerHandleExternalTrigger()
{
  bool accepted = false;

  lockTimerState();

  if (appSettings.triggerMode == TRIGGER_MODE_EXTERNAL && !timerIsBusyLocked())
  {
    accepted = true;
  }

  unlockTimerState();

  if (!accepted)
  {
    return;
  }

  timerStart();
  ESP_LOGI(logTag, "External trigger accepted");

} //   timerHandleExternalTrigger()

//--- Request external reset
void timerHandleExternalReset()
{
  timerReset();
  ESP_LOGI(logTag, "External reset accepted");

} //   timerHandleExternalReset()

//--- Set settings
void timerSetSettings(const AppSettings& settings)
{
  lockTimerState();

  AppSettings sanitizedSettings = settings;
  sanitizeSettings(sanitizedSettings, timerIsBusyLocked());

  appSettings = sanitizedSettings;
  runtimeStatus.totalCycles = appSettings.repeatCount;

  if (!timerIsBusyLocked())
  {
    runtimeStatus.currentPhaseDurationMs = 0;
    runtimeStatus.currentPhaseElapsedMs = 0;
    runtimeStatus.inOnPhase = true;
  }

  unlockTimerState();
  ESP_LOGI(logTag, "Settings updated");

} //   timerSetSettings()

//--- Apply common settings constraints
static void sanitizeSettings(AppSettings& settings, bool enforceMsMinimum)
{
  if (enforceMsMinimum && settings.onTimeUnit == TIME_UNIT_MS && settings.onTimeValue < 900)
  {
    settings.onTimeValue = 900;
  }

  if (enforceMsMinimum && settings.offTimeUnit == TIME_UNIT_MS && settings.offTimeValue < 900)
  {
    settings.offTimeValue = 900;
  }

} //   sanitizeSettings()

//--- Get settings
AppSettings timerGetSettings()
{
  AppSettings settingsSnapshot;

  lockTimerState();
  settingsSnapshot = appSettings;
  unlockTimerState();

  return settingsSnapshot;

} //   timerGetSettings()

//--- Get runtime status
RuntimeStatus timerGetRuntimeStatus()
{
  RuntimeStatus statusSnapshot;

  lockTimerState();
  statusSnapshot = runtimeStatus;
  unlockTimerState();

  return statusSnapshot;

} //   timerGetRuntimeStatus()

//--- Check whether timer is busy
bool timerIsBusy()
{
  bool busy;

  lockTimerState();
  busy = timerIsBusyLocked();
  unlockTimerState();

  return busy;

} //   timerIsBusy()

//--- Check whether timer is busy while timer mutex is held
static bool timerIsBusyLocked()
{
  return runtimeStatus.state == TIMER_STATE_RUNNING || runtimeStatus.state == TIMER_STATE_PAUSED;

} //   timerIsBusyLocked()

//--- Convert value and unit to milliseconds
uint32_t timerConvertToMs(uint32_t value, TimeUnit unit)
{
  switch (unit)
  {
  case TIME_UNIT_MS:
    return value;

  case TIME_UNIT_SECONDS:
    return value * 1000UL;

  case TIME_UNIT_MINUTES:
    return value * 60000UL;

  default:
    return value;
  }

} //   timerConvertToMs()

//--- Copy defaults into settings
void timerLoadDefaultSettings(AppSettings& settings)
{
  settings.onTimeValue = DEFAULT_ON_TIME;
  settings.offTimeValue = DEFAULT_OFF_TIME;
  settings.onTimeUnit = static_cast<TimeUnit>(DEFAULT_ON_UNIT);
  settings.offTimeUnit = static_cast<TimeUnit>(DEFAULT_OFF_UNIT);
  settings.repeatCount = DEFAULT_REPEAT_COUNT;
  settings.triggerMode = static_cast<TriggerMode>(DEFAULT_TRIGGER_MODE);
  settings.triggerEdge = static_cast<TriggerEdge>(DEFAULT_TRIGGER_EDGE);
  settings.outputPolarityHigh = DEFAULT_OUTPUT_POLARITY != 0;
  settings.lockInputDuringRun = DEFAULT_LOCK_INPUT_DURING_RUN != 0;
  settings.autoSaveLastProfile = DEFAULT_AUTO_SAVE_LAST_PROFILE != 0;
  settings.profileName = "default";

} //   timerLoadDefaultSettings()

//--- Get time unit label
const char* timerGetTimeUnitLabel(TimeUnit unit)
{
  switch (unit)
  {
  case TIME_UNIT_MS:
    return "ms";

  case TIME_UNIT_SECONDS:
    return "s";

  case TIME_UNIT_MINUTES:
    return "min";

  default:
    return "?";
  }

} //   timerGetTimeUnitLabel()

//--- Get trigger mode label
const char* timerGetTriggerModeLabel(TriggerMode mode)
{
  switch (mode)
  {
  case TRIGGER_MODE_MANUAL:
    return "Manual";

  case TRIGGER_MODE_EXTERNAL:
    return "External";

  default:
    return "?";
  }

} //   timerGetTriggerModeLabel()

//--- Get trigger edge label
const char* timerGetTriggerEdgeLabel(TriggerEdge edge)
{
  switch (edge)
  {
  case TRIGGER_EDGE_FALLING:
    return "Falling";

  case TRIGGER_EDGE_RISING:
    return "Rising";

  default:
    return "?";
  }

} //   timerGetTriggerEdgeLabel()

//--- Get timer state label
const char* timerGetStateLabel(TimerState state)
{
  switch (state)
  {
  case TIMER_STATE_IDLE:
    return "Idle";

  case TIMER_STATE_RUNNING:
    return "Running";

  case TIMER_STATE_PAUSED:
    return "Paused";

  case TIMER_STATE_FINISHED:
    return "Finished";

  default:
    return "?";
  }

} //   timerGetStateLabel()

//--- Start on phase
static void startOnPhase()
{
  runtimeStatus.inOnPhase = true;
  runtimeStatus.outputActive = true;
  runtimeStatus.currentPhaseDurationMs = timerConvertToMs(appSettings.onTimeValue, appSettings.onTimeUnit);
  runtimeStatus.currentPhaseElapsedMs = 0;
  phaseStartMs = millis();

  if (runtimeStatus.currentPhaseDurationMs == 0)
  {
    runtimeStatus.currentPhaseDurationMs = 1;
  }

  ESP_LOGD(logTag, "ON phase started for %lu ms", runtimeStatus.currentPhaseDurationMs);

} //   startOnPhase()

//--- Start off phase
static void startOffPhase()
{
  runtimeStatus.inOnPhase = false;
  runtimeStatus.outputActive = false;
  runtimeStatus.currentPhaseDurationMs = timerConvertToMs(appSettings.offTimeValue, appSettings.offTimeUnit);
  runtimeStatus.currentPhaseElapsedMs = 0;
  phaseStartMs = millis();

  if (runtimeStatus.currentPhaseDurationMs == 0)
  {
    runtimeStatus.currentPhaseDurationMs = 1;
  }

  ESP_LOGD(logTag, "OFF phase started for %lu ms", runtimeStatus.currentPhaseDurationMs);

} //   startOffPhase()
