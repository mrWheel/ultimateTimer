#ifndef IO_CONTROL_H
#define IO_CONTROL_H

#include <Arduino.h>
#include "timerTypes.h"

//--- Initialize I/O control
void ioInit();

//--- Update output according to runtime status and settings
void ioUpdate(const RuntimeStatus &runtimeStatus, const AppSettings &settings);

//--- Detect trigger edge
bool ioTriggerActivated(TriggerEdge edge);

//--- Detect reset input edge
bool ioResetActivated();

//--- Get raw trigger input state
bool ioGetRawTriggerState();

//--- Get raw reset input state
bool ioGetRawResetState();

#endif
