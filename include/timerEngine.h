/*** Last Changed: 2026-05-12 - 11:43 ***/
#ifndef TIMER_ENGINE_H
#define TIMER_ENGINE_H

#include <Arduino.h>
#include "timerTypes.h"

//--- Initialize timer engine
void timerInit();

//--- Update timer engine
void timerUpdate();

//--- Start timer cycle
void timerStart();

//--- Stop timer cycle
void timerStop();

//--- Pause timer cycle
void timerPause();

//--- Resume timer cycle
void timerResume();

//--- Reset timer cycle
void timerReset();

//--- Request external trigger
void timerHandleExternalTrigger();

//--- Request external reset
void timerHandleExternalReset();

//--- Set settings
void timerSetSettings(const AppSettings& settings);

//--- Get settings
AppSettings timerGetSettings();

//--- Get runtime status
RuntimeStatus timerGetRuntimeStatus();

//--- Get derived 24h status timing info
Timer24hStatusInfo timerGet24hStatusInfo();

//--- Check whether timer is busy
bool timerIsBusy();

//--- Convert value and unit to milliseconds
uint32_t timerConvertToMs(uint32_t value, TimeUnit unit);

//--- Copy defaults into settings
void timerLoadDefaultSettings(AppSettings& settings);

//--- Get timer type label
const char* timerGetTimerTypeLabel(TimerType type);

//--- Get 24h quarter-hour state label
const char* timerGet24hQuarterStateLabel(Timer24hQuarterState state);

//--- Get 24h hour label derived from quarter-hour states
const char* timerGet24hHourLabel(const AppSettings& settings, uint8_t hourIndex);

//--- Get 24h quarter-hour state
Timer24hQuarterState timerGet24hQuarterState(const AppSettings& settings, uint8_t hourIndex, uint8_t quarterIndex);

//--- Set 24h quarter-hour state
void timerSet24hQuarterState(AppSettings& settings, uint8_t hourIndex, uint8_t quarterIndex, Timer24hQuarterState state);

//--- Set all 24h quarter-hour states for one hour
void timerSet24hHourState(AppSettings& settings, uint8_t hourIndex, Timer24hQuarterState state);

//--- Get time unit label
const char* timerGetTimeUnitLabel(TimeUnit unit);

//--- Get trigger mode label
const char* timerGetTriggerModeLabel(TriggerMode mode);

//--- Get trigger edge label
const char* timerGetTriggerEdgeLabel(TriggerEdge edge);

//--- Get timer state label
const char* timerGetStateLabel(TimerState state);

#endif
