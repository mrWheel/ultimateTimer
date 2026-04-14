#include "timerEngine.h"
#include "appConfig.h"

#include <esp_log.h>

//--- Logging tag
static const char *logTag = "timerEngine";

//--- Internal settings
static AppSettings appSettings;

//--- Internal runtime status
static RuntimeStatus runtimeStatus;

//--- Internal timing
static uint32_t phaseStartMs = 0;

//--- Start on phase
static void startOnPhase();

//--- Start off phase
static void startOffPhase();

//--- Initialize timer engine
void timerInit()
{
  timerLoadDefaultSettings(appSettings);

  runtimeStatus.state = TIMER_STATE_IDLE;
  runtimeStatus.outputActive = false;
  runtimeStatus.currentCycle = 0;
  runtimeStatus.totalCycles = appSettings.repeatCount;
  runtimeStatus.currentPhaseDurationMs = 0;
  runtimeStatus.currentPhaseElapsedMs = 0;
  runtimeStatus.inOnPhase = true;
  phaseStartMs = 0;

  ESP_LOGI(logTag, "Timer engine initialized");

}   //   timerInit()

//--- Update timer engine
void timerUpdate()
{
  uint32_t nowMs = millis();

  if (runtimeStatus.state != TIMER_STATE_RUNNING)
  {
    return;
  }

  runtimeStatus.currentPhaseElapsedMs = nowMs - phaseStartMs;

  if (runtimeStatus.currentPhaseElapsedMs < runtimeStatus.currentPhaseDurationMs)
  {
    return;
  }

  if (runtimeStatus.inOnPhase)
  {
    startOffPhase();
  }
  else
  {
    runtimeStatus.currentCycle++;

    if (runtimeStatus.totalCycles > 0 && runtimeStatus.currentCycle >= runtimeStatus.totalCycles)
    {
      runtimeStatus.state = TIMER_STATE_FINISHED;
      runtimeStatus.outputActive = false;
      runtimeStatus.currentPhaseElapsedMs = runtimeStatus.currentPhaseDurationMs;

      ESP_LOGI(logTag, "Timer finished");

      return;
    }

    startOnPhase();
  }

}   //   timerUpdate()

//--- Start timer cycle
void timerStart()
{
  runtimeStatus.totalCycles = appSettings.repeatCount;
  runtimeStatus.currentCycle = 0;
  runtimeStatus.state = TIMER_STATE_RUNNING;

  startOnPhase();

  ESP_LOGI(logTag, "Timer started");

}   //   timerStart()

//--- Stop timer cycle
void timerStop()
{
  runtimeStatus.state = TIMER_STATE_IDLE;
  runtimeStatus.outputActive = false;
  runtimeStatus.currentPhaseDurationMs = 0;
  runtimeStatus.currentPhaseElapsedMs = 0;
  runtimeStatus.inOnPhase = true;

  ESP_LOGI(logTag, "Timer stopped");

}   //   timerStop()

//--- Pause timer cycle
void timerPause()
{
  if (runtimeStatus.state != TIMER_STATE_RUNNING)
  {
    return;
  }

  runtimeStatus.state = TIMER_STATE_PAUSED;
  runtimeStatus.outputActive = false;

  ESP_LOGI(logTag, "Timer paused");

}   //   timerPause()

//--- Resume timer cycle
void timerResume()
{
  if (runtimeStatus.state != TIMER_STATE_PAUSED)
  {
    return;
  }

  runtimeStatus.state = TIMER_STATE_RUNNING;
  phaseStartMs = millis() - runtimeStatus.currentPhaseElapsedMs;
  runtimeStatus.outputActive = runtimeStatus.inOnPhase;

  ESP_LOGI(logTag, "Timer resumed");

}   //   timerResume()

//--- Reset timer cycle
void timerReset()
{
  runtimeStatus.state = TIMER_STATE_IDLE;
  runtimeStatus.outputActive = false;
  runtimeStatus.currentCycle = 0;
  runtimeStatus.totalCycles = appSettings.repeatCount;
  runtimeStatus.currentPhaseDurationMs = 0;
  runtimeStatus.currentPhaseElapsedMs = 0;
  runtimeStatus.inOnPhase = true;

  ESP_LOGI(logTag, "Timer reset");

}   //   timerReset()

//--- Request external trigger
void timerHandleExternalTrigger()
{
  if (appSettings.triggerMode != TRIGGER_MODE_EXTERNAL)
  {
    return;
  }

  if (timerIsBusy())
  {
    return;
  }

  timerStart();

  ESP_LOGI(logTag, "External trigger accepted");

}   //   timerHandleExternalTrigger()

//--- Request external reset
void timerHandleExternalReset()
{
  timerReset();

  ESP_LOGI(logTag, "External reset accepted");

}   //   timerHandleExternalReset()

//--- Set settings
void timerSetSettings(const AppSettings &settings)
{
  appSettings = settings;
  runtimeStatus.totalCycles = appSettings.repeatCount;

  if (!timerIsBusy())
  {
    runtimeStatus.currentPhaseDurationMs = 0;
    runtimeStatus.currentPhaseElapsedMs = 0;
    runtimeStatus.inOnPhase = true;
  }

  ESP_LOGI(logTag, "Settings updated");

}   //   timerSetSettings()

//--- Get settings
const AppSettings &timerGetSettings()
{
  return appSettings;

}   //   timerGetSettings()

//--- Get runtime status
RuntimeStatus timerGetRuntimeStatus()
{
  return runtimeStatus;

}   //   timerGetRuntimeStatus()

//--- Check whether timer is busy
bool timerIsBusy()
{
  return runtimeStatus.state == TIMER_STATE_RUNNING || runtimeStatus.state == TIMER_STATE_PAUSED;

}   //   timerIsBusy()

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

}   //   timerConvertToMs()

//--- Copy defaults into settings
void timerLoadDefaultSettings(AppSettings &settings)
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

}   //   timerLoadDefaultSettings()

//--- Get time unit label
const char *timerGetTimeUnitLabel(TimeUnit unit)
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

}   //   timerGetTimeUnitLabel()

//--- Get trigger mode label
const char *timerGetTriggerModeLabel(TriggerMode mode)
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

}   //   timerGetTriggerModeLabel()

//--- Get trigger edge label
const char *timerGetTriggerEdgeLabel(TriggerEdge edge)
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

}   //   timerGetTriggerEdgeLabel()

//--- Get timer state label
const char *timerGetStateLabel(TimerState state)
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

}   //   timerGetStateLabel()

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

  ESP_LOGI(logTag, "ON phase started for %lu ms", runtimeStatus.currentPhaseDurationMs);

}   //   startOnPhase()

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

  ESP_LOGI(logTag, "OFF phase started for %lu ms", runtimeStatus.currentPhaseDurationMs);

}   //   startOffPhase()
