/*** Last Changed: 2026-05-11 - 16:24 ***/
#include "timerEngine.h"
#include "appConfig.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <time.h>

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

//--- Update 24h runtime snapshot
static void update24hRuntimeState();

//--- Get 24h quarter-hour index in settings storage
static int get24hQuarterIndex(uint8_t hourIndex, uint8_t quarterIndex);

//--- Get 24h quarter-hour transition seed
static uint32_t get24hQuarterTransitionOffset(const struct tm& timeInfo, int quarterIndex, Timer24hQuarterState state);

//--- Build 24h runtime segments for current day
static void build24hRuntimeSegments(const struct tm& timeInfo, uint32_t nowSeconds, bool& outputActive, uint32_t& phaseStartSeconds, uint32_t& phaseEndSeconds, uint32_t& cycleIndex);

//--- Build 24h day transitions (seconds-of-day where output changes)
static void build24hDayTransitions(const struct tm& dayInfo, uint32_t transitionSeconds[96], bool transitionStates[96], size_t& transitionCount);

//--- Apply common settings constraints
static void sanitizeSettings(AppSettings& settings, bool enforceMsMinimum);

//--- Check whether timer is busy while timer mutex is held
static bool timerIsBusyLocked();

//--- Find last transition at or before second-of-day
static bool findLastTransition(const uint32_t transitionSeconds[], const bool transitionStates[], size_t transitionCount, uint32_t nowSeconds, bool targetState, uint32_t& foundSeconds);

//--- Find next transition after second-of-day
static bool findNextTransition(const uint32_t transitionSeconds[], const bool transitionStates[], size_t transitionCount, uint32_t nowSeconds, bool targetStateOnly, bool targetState, uint32_t& foundSeconds);

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

  if (appSettings.timerType == TIMER_TYPE_24H)
  {
    if (runtimeStatus.state != TIMER_STATE_RUNNING)
    {
      unlockTimerState();

      return;
    }

    update24hRuntimeState();
    unlockTimerState();

    return;
  }

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

  if (appSettings.timerType == TIMER_TYPE_24H)
  {
    runtimeStatus.state = TIMER_STATE_RUNNING;
    update24hRuntimeState();
    unlockTimerState();
    ESP_LOGI(logTag, "24h timer started");

    return;
  }

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

  if (appSettings.timerType == TIMER_TYPE_24H)
  {
    runtimeStatus.state = TIMER_STATE_RUNNING;
    update24hRuntimeState();
  }

  unlockTimerState();
  ESP_LOGI(logTag, "Timer reset");

} //   timerReset()

//--- Request external trigger
void timerHandleExternalTrigger()
{
  if (appSettings.triggerMode != TRIGGER_MODE_EXTERNAL)
  {
    return;
  }

  timerReset();
  timerStart();
  ESP_LOGI(logTag, "External trigger accepted");

} //   timerHandleExternalTrigger()

//--- Request external reset
void timerHandleExternalReset()
{
  if (appSettings.triggerMode != TRIGGER_MODE_EXTERNAL)
  {
    return;
  }

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

  if (appSettings.timerType == TIMER_TYPE_24H)
  {
    runtimeStatus.state = TIMER_STATE_RUNNING;
    update24hRuntimeState();
  }

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

//--- Get derived 24h status timing info
Timer24hStatusInfo timerGet24hStatusInfo()
{
  Timer24hStatusInfo info;
  time_t now = time(nullptr);
  struct tm localTimeInfo;
  struct tm tomorrowInfo;
  struct tm yesterdayInfo;
  uint32_t transitionsTodaySeconds[96];
  bool transitionsTodayStates[96];
  size_t transitionsTodayCount = 0;
  uint32_t transitionsTomorrowSeconds[96];
  bool transitionsTomorrowStates[96];
  size_t transitionsTomorrowCount = 0;
  uint32_t transitionsYesterdaySeconds[96];
  bool transitionsYesterdayStates[96];
  size_t transitionsYesterdayCount = 0;
  uint32_t nowSeconds = 0;
  uint32_t transitionSeconds = 0;

  info.timeValid = false;
  info.outputActive = false;
  info.hasLastOn = false;
  info.hasLastOff = false;
  info.hasNextSwitch = false;
  info.hasNextOff = false;
  info.lastOnSecondsOfDay = 0;
  info.lastOffSecondsOfDay = 0;
  info.nextSwitchSecondsOfDay = 0;
  info.nextOffSecondsOfDay = 0;
  info.nextSwitchInSeconds = 0;

  lockTimerState();

  if (appSettings.timerType != TIMER_TYPE_24H)
  {
    unlockTimerState();

    return info;
  }

  if (now <= 0 || localtime_r(&now, &localTimeInfo) == nullptr)
  {
    unlockTimerState();

    return info;
  }

  nowSeconds = static_cast<uint32_t>(localTimeInfo.tm_hour) * 3600UL + static_cast<uint32_t>(localTimeInfo.tm_min) * 60UL + static_cast<uint32_t>(localTimeInfo.tm_sec);

  tomorrowInfo = localTimeInfo;
  tomorrowInfo.tm_mday += 1;
  mktime(&tomorrowInfo);

  yesterdayInfo = localTimeInfo;
  yesterdayInfo.tm_mday -= 1;
  mktime(&yesterdayInfo);

  build24hDayTransitions(localTimeInfo, transitionsTodaySeconds, transitionsTodayStates, transitionsTodayCount);
  build24hDayTransitions(tomorrowInfo, transitionsTomorrowSeconds, transitionsTomorrowStates, transitionsTomorrowCount);
  build24hDayTransitions(yesterdayInfo, transitionsYesterdaySeconds, transitionsYesterdayStates, transitionsYesterdayCount);

  for (size_t transitionIndex = 0; transitionIndex < transitionsTodayCount; transitionIndex++)
  {
    if (transitionsTodaySeconds[transitionIndex] <= nowSeconds)
    {
      info.outputActive = transitionsTodayStates[transitionIndex];
    }
    else
    {
      break;
    }
  }

  if (findLastTransition(transitionsTodaySeconds, transitionsTodayStates, transitionsTodayCount, nowSeconds, true, transitionSeconds))
  {
    info.hasLastOn = true;
    info.lastOnSecondsOfDay = transitionSeconds;
  }
  else if (findLastTransition(transitionsYesterdaySeconds, transitionsYesterdayStates, transitionsYesterdayCount, 86400UL, true, transitionSeconds))
  {
    info.hasLastOn = true;
    info.lastOnSecondsOfDay = transitionSeconds;
  }

  if (findLastTransition(transitionsTodaySeconds, transitionsTodayStates, transitionsTodayCount, nowSeconds, false, transitionSeconds))
  {
    info.hasLastOff = true;
    info.lastOffSecondsOfDay = transitionSeconds;
  }
  else if (findLastTransition(transitionsYesterdaySeconds, transitionsYesterdayStates, transitionsYesterdayCount, 86400UL, false, transitionSeconds))
  {
    info.hasLastOff = true;
    info.lastOffSecondsOfDay = transitionSeconds;
  }

  if (findNextTransition(transitionsTodaySeconds, transitionsTodayStates, transitionsTodayCount, nowSeconds, false, false, transitionSeconds))
  {
    info.hasNextSwitch = true;
    info.nextSwitchSecondsOfDay = transitionSeconds;
    info.nextSwitchInSeconds = transitionSeconds - nowSeconds;
  }
  else if (findNextTransition(transitionsTomorrowSeconds, transitionsTomorrowStates, transitionsTomorrowCount, 0, false, false, transitionSeconds))
  {
    info.hasNextSwitch = true;
    info.nextSwitchSecondsOfDay = transitionSeconds;
    info.nextSwitchInSeconds = (86400UL - nowSeconds) + transitionSeconds;
  }

  if (findNextTransition(transitionsTodaySeconds, transitionsTodayStates, transitionsTodayCount, nowSeconds, true, false, transitionSeconds))
  {
    info.hasNextOff = true;
    info.nextOffSecondsOfDay = transitionSeconds;
  }
  else if (findNextTransition(transitionsTomorrowSeconds, transitionsTomorrowStates, transitionsTomorrowCount, 0, true, false, transitionSeconds))
  {
    info.hasNextOff = true;
    info.nextOffSecondsOfDay = transitionSeconds;
  }

  info.timeValid = true;

  unlockTimerState();

  return info;

} //   timerGet24hStatusInfo()

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
  settings.timerType = TIMER_TYPE_CYCLIC;
  settings.onTimeValue = DEFAULT_ON_TIME;
  settings.offTimeValue = DEFAULT_OFF_TIME;
  settings.onTimeUnit = static_cast<TimeUnit>(DEFAULT_ON_UNIT);
  settings.offTimeUnit = static_cast<TimeUnit>(DEFAULT_OFF_UNIT);
  settings.repeatCount = DEFAULT_REPEAT_COUNT;
  settings.triggerMode = static_cast<TriggerMode>(DEFAULT_TRIGGER_MODE);
  settings.triggerEdge = static_cast<TriggerEdge>(DEFAULT_TRIGGER_EDGE);
  for (size_t index = 0; index < sizeof(settings.timer24hQuarterStates); index++)
  {
    settings.timer24hQuarterStates[index] = static_cast<uint8_t>(TIMER_24H_QUARTER_OFF);
  }
  settings.outputPolarityHigh = DEFAULT_OUTPUT_POLARITY != 0;
  settings.lockInputDuringRun = DEFAULT_LOCK_INPUT_DURING_RUN != 0;
  settings.autoSaveLastProfile = DEFAULT_AUTO_SAVE_LAST_PROFILE != 0;
  settings.profileName = "default";

} //   timerLoadDefaultSettings()

//--- Get timer type label
const char* timerGetTimerTypeLabel(TimerType type)
{
  switch (type)
  {
  case TIMER_TYPE_CYCLIC:
    return "Cyclic";

  case TIMER_TYPE_24H:
    return "24h";

  default:
    return "?";
  }

} //   timerGetTimerTypeLabel()

//--- Get 24h quarter-hour state label
const char* timerGet24hQuarterStateLabel(Timer24hQuarterState state)
{
  switch (state)
  {
  case TIMER_24H_QUARTER_OFF:
    return "-";

  case TIMER_24H_QUARTER_ON:
    return "+";

  case TIMER_24H_QUARTER_RANDOM_ON:
    return "R";

  case TIMER_24H_QUARTER_RANDOM_OFF:
    return "r";

  default:
    return "?";
  }

} //   timerGet24hQuarterStateLabel()

//--- Get 24h quarter-hour state
Timer24hQuarterState timerGet24hQuarterState(const AppSettings& settings, uint8_t hourIndex, uint8_t quarterIndex)
{
  int storageIndex = get24hQuarterIndex(hourIndex, quarterIndex);

  if (storageIndex < 0 || storageIndex >= static_cast<int>(sizeof(settings.timer24hQuarterStates)))
  {
    return TIMER_24H_QUARTER_OFF;
  }

  return static_cast<Timer24hQuarterState>(settings.timer24hQuarterStates[storageIndex]);

} //   timerGet24hQuarterState()

//--- Set 24h quarter-hour state
void timerSet24hQuarterState(AppSettings& settings, uint8_t hourIndex, uint8_t quarterIndex, Timer24hQuarterState state)
{
  int storageIndex = get24hQuarterIndex(hourIndex, quarterIndex);

  if (storageIndex < 0 || storageIndex >= static_cast<int>(sizeof(settings.timer24hQuarterStates)))
  {
    return;
  }

  settings.timer24hQuarterStates[storageIndex] = static_cast<uint8_t>(state);

} //   timerSet24hQuarterState()

//--- Set all 24h quarter-hour states for one hour
void timerSet24hHourState(AppSettings& settings, uint8_t hourIndex, Timer24hQuarterState state)
{
  for (uint8_t quarterIndex = 0; quarterIndex < 4; quarterIndex++)
  {
    timerSet24hQuarterState(settings, hourIndex, quarterIndex, state);
  }

} //   timerSet24hHourState()

//--- Get 24h hour label derived from quarter-hour states
const char* timerGet24hHourLabel(const AppSettings& settings, uint8_t hourIndex)
{
  Timer24hQuarterState firstState = timerGet24hQuarterState(settings, hourIndex, 0);

  for (uint8_t quarterIndex = 1; quarterIndex < 4; quarterIndex++)
  {
    if (timerGet24hQuarterState(settings, hourIndex, quarterIndex) != firstState)
    {
      return "S";
    }
  }

  return timerGet24hQuarterStateLabel(firstState);

} //   timerGet24hHourLabel()

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

//--- Get 24h quarter-hour state index
static int get24hQuarterIndex(uint8_t hourIndex, uint8_t quarterIndex)
{
  if (hourIndex >= 24 || quarterIndex >= 4)
  {
    return -1;
  }

  return static_cast<int>(hourIndex) * 4 + static_cast<int>(quarterIndex);

} //   get24hQuarterIndex()

//--- Get 24h quarter-hour random transition offset in seconds
static uint32_t get24hQuarterTransitionOffset(const struct tm& timeInfo, int quarterIndex, Timer24hQuarterState state)
{
  uint32_t seed = static_cast<uint32_t>(timeInfo.tm_year + 1900) * 1000UL;
  seed += static_cast<uint32_t>(timeInfo.tm_yday) * 97UL;
  seed += static_cast<uint32_t>(quarterIndex) * 31UL;
  seed += static_cast<uint32_t>(state) * 17UL;
  seed ^= 0xA5A5A5A5UL;
  seed = seed * 1664525UL + 1013904223UL;

  return seed % 900UL;

} //   get24hQuarterTransitionOffset()

//--- Build 24h day transitions (seconds-of-day where output changes)
static void build24hDayTransitions(const struct tm& dayInfo, uint32_t transitionSeconds[96], bool transitionStates[96], size_t& transitionCount)
{
  bool currentState = false;

  transitionCount = 0;

  for (uint32_t currentQuarter = 0; currentQuarter < 96UL; currentQuarter++)
  {
    uint32_t quarterStartSeconds = currentQuarter * 900UL;
    uint32_t quarterEndSeconds = quarterStartSeconds + 900UL;

    if (quarterEndSeconds > 86400UL)
    {
      quarterEndSeconds = 86400UL;
    }

    Timer24hQuarterState quarterState = timerGet24hQuarterState(appSettings, static_cast<uint8_t>(currentQuarter / 4UL), static_cast<uint8_t>(currentQuarter % 4UL));

    if (quarterState == TIMER_24H_QUARTER_OFF)
    {
      if (currentState && transitionCount < 96)
      {
        transitionSeconds[transitionCount] = quarterStartSeconds;
        transitionStates[transitionCount] = false;
        transitionCount++;
      }

      currentState = false;
      continue;
    }

    if (quarterState == TIMER_24H_QUARTER_ON)
    {
      if (!currentState && transitionCount < 96)
      {
        transitionSeconds[transitionCount] = quarterStartSeconds;
        transitionStates[transitionCount] = true;
        transitionCount++;
      }

      currentState = true;
      continue;
    }

    bool desiredState = (quarterState == TIMER_24H_QUARTER_RANDOM_ON);

    if (currentState != desiredState)
    {
      uint32_t transitionSecondsOfDay = quarterStartSeconds + get24hQuarterTransitionOffset(dayInfo, static_cast<int>(currentQuarter), quarterState);

      if (transitionSecondsOfDay > quarterEndSeconds)
      {
        transitionSecondsOfDay = quarterEndSeconds;
      }

      if (transitionCount < 96)
      {
        transitionSeconds[transitionCount] = transitionSecondsOfDay;
        transitionStates[transitionCount] = desiredState;
        transitionCount++;
      }

      currentState = desiredState;
    }
  }

} //   build24hDayTransitions()

//--- Find last transition at or before second-of-day
static bool findLastTransition(const uint32_t transitionSeconds[], const bool transitionStates[], size_t transitionCount, uint32_t nowSeconds, bool targetState, uint32_t& foundSeconds)
{
  if (nowSeconds > 86400UL)
  {
    nowSeconds = 86400UL;
  }

  for (size_t transitionIndex = transitionCount; transitionIndex > 0; transitionIndex--)
  {
    size_t index = transitionIndex - 1;

    if (transitionSeconds[index] <= nowSeconds && transitionStates[index] == targetState)
    {
      foundSeconds = transitionSeconds[index];

      return true;
    }
  }

  return false;

} //   findLastTransition()

//--- Find next transition after second-of-day
static bool findNextTransition(const uint32_t transitionSeconds[], const bool transitionStates[], size_t transitionCount, uint32_t nowSeconds, bool targetStateOnly, bool targetState, uint32_t& foundSeconds)
{
  for (size_t transitionIndex = 0; transitionIndex < transitionCount; transitionIndex++)
  {
    if (transitionSeconds[transitionIndex] <= nowSeconds)
    {
      continue;
    }

    if (targetStateOnly && transitionStates[transitionIndex] != targetState)
    {
      continue;
    }

    foundSeconds = transitionSeconds[transitionIndex];

    return true;
  }

  return false;

} //   findNextTransition()

//--- Build 24h runtime segments for current day
static void build24hRuntimeSegments(const struct tm& timeInfo, uint32_t nowSeconds, bool& outputActive, uint32_t& phaseStartSeconds, uint32_t& phaseEndSeconds, uint32_t& cycleIndex)
{
  uint32_t quarterIndex = nowSeconds / 900UL;
  uint32_t quarterStartSeconds = 0;
  uint32_t quarterEndSeconds = 0;
  bool currentState = false;
  uint32_t segmentStartSeconds = 0;
  uint32_t currentSegmentEndSeconds = 86400UL;
  bool foundCurrentSegment = false;

  outputActive = false;
  phaseStartSeconds = 0;
  phaseEndSeconds = 86400UL;
  cycleIndex = quarterIndex;

  for (uint32_t currentQuarter = 0; currentQuarter < 96UL; currentQuarter++)
  {
    quarterStartSeconds = currentQuarter * 900UL;
    quarterEndSeconds = quarterStartSeconds + 900UL;

    if (quarterEndSeconds > 86400UL)
    {
      quarterEndSeconds = 86400UL;
    }

    Timer24hQuarterState quarterState = timerGet24hQuarterState(appSettings, static_cast<uint8_t>(currentQuarter / 4UL), static_cast<uint8_t>(currentQuarter % 4UL));

    if (quarterState == TIMER_24H_QUARTER_OFF)
    {
      if (currentState)
      {
        if (segmentStartSeconds < quarterStartSeconds && nowSeconds >= segmentStartSeconds && nowSeconds < quarterStartSeconds)
        {
          outputActive = true;
          phaseStartSeconds = segmentStartSeconds;
          phaseEndSeconds = quarterStartSeconds;
          foundCurrentSegment = true;
        }

        currentState = false;
        segmentStartSeconds = quarterStartSeconds;
      }

      if (nowSeconds >= quarterStartSeconds && nowSeconds < quarterEndSeconds)
      {
        currentSegmentEndSeconds = quarterEndSeconds;
        foundCurrentSegment = true;
      }
    }
    else if (quarterState == TIMER_24H_QUARTER_ON)
    {
      if (!currentState)
      {
        if (segmentStartSeconds < quarterStartSeconds && nowSeconds >= segmentStartSeconds && nowSeconds < quarterStartSeconds)
        {
          outputActive = false;
          phaseStartSeconds = segmentStartSeconds;
          phaseEndSeconds = quarterStartSeconds;
          foundCurrentSegment = true;
        }

        currentState = true;
        segmentStartSeconds = quarterStartSeconds;
      }

      if (nowSeconds >= quarterStartSeconds && nowSeconds < quarterEndSeconds)
      {
        currentSegmentEndSeconds = quarterEndSeconds;
        foundCurrentSegment = true;
      }
    }
    else
    {
      bool desiredState = (quarterState == TIMER_24H_QUARTER_RANDOM_ON);
      uint32_t transitionOffsetSeconds = get24hQuarterTransitionOffset(timeInfo, static_cast<int>(currentQuarter), quarterState);
      uint32_t transitionSeconds = quarterStartSeconds + transitionOffsetSeconds;

      if (transitionSeconds > quarterEndSeconds)
      {
        transitionSeconds = quarterEndSeconds;
      }

      if (currentState != desiredState)
      {
        if (segmentStartSeconds < transitionSeconds)
        {
          if (nowSeconds >= segmentStartSeconds && nowSeconds < transitionSeconds)
          {
            outputActive = currentState;
            phaseStartSeconds = segmentStartSeconds;
            phaseEndSeconds = transitionSeconds;
            foundCurrentSegment = true;
          }
        }

        currentState = desiredState;
        segmentStartSeconds = transitionSeconds;
      }

      if (nowSeconds >= transitionSeconds && nowSeconds < quarterEndSeconds)
      {
        currentSegmentEndSeconds = quarterEndSeconds;
        foundCurrentSegment = true;
      }
    }

    if (foundCurrentSegment)
    {
      break;
    }
  }

  if (!foundCurrentSegment)
  {
    outputActive = currentState;
    phaseStartSeconds = segmentStartSeconds;
    phaseEndSeconds = currentSegmentEndSeconds;
  }

} //   build24hRuntimeSegments()

//--- Update 24h runtime snapshot
static void update24hRuntimeState()
{
  time_t now = time(nullptr);
  struct tm localTimeInfo;
  uint32_t nowSeconds;
  bool outputActive;
  uint32_t phaseStartSeconds;
  uint32_t phaseEndSeconds;
  uint32_t cycleIndex;

  if (now <= 0 || localtime_r(&now, &localTimeInfo) == nullptr)
  {
    runtimeStatus.outputActive = false;
    runtimeStatus.currentCycle = 0;
    runtimeStatus.totalCycles = 96;
    runtimeStatus.currentPhaseDurationMs = 0;
    runtimeStatus.currentPhaseElapsedMs = 0;
    runtimeStatus.inOnPhase = false;

    return;
  }

  nowSeconds = static_cast<uint32_t>(localTimeInfo.tm_hour) * 3600UL + static_cast<uint32_t>(localTimeInfo.tm_min) * 60UL + static_cast<uint32_t>(localTimeInfo.tm_sec);

  build24hRuntimeSegments(localTimeInfo, nowSeconds, outputActive, phaseStartSeconds, phaseEndSeconds, cycleIndex);

  runtimeStatus.outputActive = outputActive;
  runtimeStatus.currentCycle = cycleIndex;
  runtimeStatus.totalCycles = 96;
  runtimeStatus.currentPhaseDurationMs = (phaseEndSeconds > phaseStartSeconds) ? (phaseEndSeconds - phaseStartSeconds) * 1000UL : 0;
  runtimeStatus.currentPhaseElapsedMs = (nowSeconds > phaseStartSeconds) ? (nowSeconds - phaseStartSeconds) * 1000UL : 0;
  runtimeStatus.inOnPhase = outputActive;

} //   update24hRuntimeState()

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
