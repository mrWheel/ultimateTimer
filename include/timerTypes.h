/*** Last Changed: 2026-05-13 - 10:34 ***/
#ifndef TIMER_TYPES_H
#define TIMER_TYPES_H

#include <Arduino.h>

//--- Time units
enum TimeUnit
{
  TIME_UNIT_MS = 0,
  TIME_UNIT_SECONDS = 1,
  TIME_UNIT_MINUTES = 2
};

//--- Timer types
enum TimerType
{
  TIMER_TYPE_CYCLIC = 0,
  TIMER_TYPE_24H = 1
};

//--- 24h quarter-hour states
enum Timer24hQuarterState
{
  TIMER_24H_QUARTER_OFF = 0,
  TIMER_24H_QUARTER_ON = 1,
  TIMER_24H_QUARTER_RANDOM_ON = 2,
  TIMER_24H_QUARTER_RANDOM_OFF = 3
};

//--- Trigger modes
enum TriggerMode
{
  TRIGGER_MODE_MANUAL = 0,
  TRIGGER_MODE_EXTERNAL = 1
};

//--- Trigger edges
enum TriggerEdge
{
  TRIGGER_EDGE_FALLING = 0,
  TRIGGER_EDGE_RISING = 1
};

//--- Timer states
enum TimerState
{
  TIMER_STATE_IDLE = 0,
  TIMER_STATE_RUNNING = 1,
  TIMER_STATE_PAUSED = 2,
  TIMER_STATE_FINISHED = 3
};

//--- UI screens
enum UiScreen
{
  UI_SCREEN_STATUS = 0,
  UI_SCREEN_MAIN_MENU = 1,
  UI_SCREEN_TIMER_SETTINGS_MENU = 2,
  UI_SCREEN_SYSTEM_SETTINGS_MENU = 3,
  UI_SCREEN_PROFILE_LIST = 4,
  UI_SCREEN_FIELD_INPUT = 5,
  UI_SCREEN_WIFI_MANAGER_PORTAL = 6,
  UI_SCREEN_24H_TIMER_MENU = 7
};

//--- Application settings
struct AppSettings
{
  TimerType timerType;
  uint32_t onTimeValue;
  uint32_t offTimeValue;
  TimeUnit onTimeUnit;
  TimeUnit offTimeUnit;
  uint32_t repeatCount;
  TriggerMode triggerMode;
  TriggerEdge triggerEdge;
  uint8_t timer24hQuarterStates[96];
  bool outputPolarityHigh;
  bool lockInputDuringRun;
  bool autoSaveLastProfile;
  String profileName;
};

//--- Runtime status
struct RuntimeStatus
{
  TimerState state;
  bool outputActive;
  uint32_t currentCycle;
  uint32_t totalCycles;
  uint32_t currentPhaseDurationMs;
  uint32_t currentPhaseElapsedMs;
  bool inOnPhase;
};

//--- Derived 24h status timing info for Universal Timer views
struct Timer24hStatusInfo
{
  bool timeValid;
  bool outputActive;
  bool hasLastOn;
  bool hasLastOff;
  bool hasNextSwitch;
  bool hasNextOff;
  uint32_t lastOnSecondsOfDay;
  uint32_t lastOffSecondsOfDay;
  uint32_t nextSwitchSecondsOfDay;
  uint32_t nextOffSecondsOfDay;
  uint32_t nextSwitchWindowStartSecondsOfDay;
  uint32_t nextSwitchWindowEndSecondsOfDay;
  uint32_t nextSwitchInSeconds;
};

//--- WiFi settings
struct WifiSettings
{
  String staSsid;
  String staPassword;
  String apSsid;
  String apPassword;
  String hostName;
};

#endif
