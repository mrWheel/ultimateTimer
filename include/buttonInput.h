/*** Last Changed: 2026-04-15 - 13:12 ***/
#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <Arduino.h>

//--- Button events
enum ButtonEvent
{
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_SHORT_PRESS = 1,
  BUTTON_EVENT_LONG_PRESS = 2,
  BUTTON_EVENT_MEDIUM_PRESS = 3
};

//--- Initialize auxiliary button input
void buttonInit();

//--- Update auxiliary button input
void buttonUpdate();

//--- Get next auxiliary button event
ButtonEvent buttonGetEvent();

//--- Clear pending auxiliary button event
void buttonClearEvent();

#endif
