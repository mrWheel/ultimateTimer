/*** Last Changed: 2026-04-15 - 14:23 ***/
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

//--- Check whether timer is busy
bool timerIsBusy();

//--- Convert value and unit to milliseconds
uint32_t timerConvertToMs(uint32_t value, TimeUnit unit);

//--- Copy defaults into settings
void timerLoadDefaultSettings(AppSettings& settings);

//--- Get time unit label
const char* timerGetTimeUnitLabel(TimeUnit unit);

//--- Get trigger mode label
const char* timerGetTriggerModeLabel(TriggerMode mode);

//--- Get trigger edge label
const char* timerGetTriggerEdgeLabel(TriggerEdge edge);

//--- Get timer state label
const char* timerGetStateLabel(TimerState state);

#endif
