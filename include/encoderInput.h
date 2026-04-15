/*** Last Changed: 2026-04-15 - 13:12 ***/
#ifndef ENCODER_INPUT_H
#define ENCODER_INPUT_H

#include <Arduino.h>

//--- Encoder events
enum EncoderEvent
{
  ENCODER_EVENT_NONE = 0,
  ENCODER_EVENT_LEFT = 1,
  ENCODER_EVENT_RIGHT = 2,
  ENCODER_EVENT_SHORT_PRESS = 3,
  ENCODER_EVENT_LONG_PRESS = 4,
  ENCODER_EVENT_MEDIUM_PRESS = 5
};

//--- Initialize encoder input
void encoderInit();

//--- Update encoder input
void encoderUpdate();

//--- Get next encoder event
EncoderEvent encoderGetEvent();

//--- Clear pending encoder event
void encoderClearEvent();

//--- Set encoder direction reversal state
void encoderSetDirectionReversed(bool reversed);

//--- Get encoder direction reversal state
bool encoderGetDirectionReversed();

#endif
