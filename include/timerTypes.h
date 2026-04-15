/*** Last Changed: 2026-04-15 - 14:23 ***/
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
  UI_SCREEN_WIFI_MANAGER_PORTAL = 6
};

//--- Application settings
struct AppSettings
{
  uint32_t onTimeValue;
  uint32_t offTimeValue;
  TimeUnit onTimeUnit;
  TimeUnit offTimeUnit;
  uint32_t repeatCount;
  TriggerMode triggerMode;
  TriggerEdge triggerEdge;
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
