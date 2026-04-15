/*** Last Changed: 2026-04-15 - 13:12 ***/
#include "encoderInput.h"
#include "appConfig.h"

#include <esp_log.h>

//--- Logging tag
static const char* logTag = "encoderInput";

//--- Encoder state
static int32_t encoderDelta = 0;
static uint8_t lastState = 0;
static EncoderEvent pendingEvent = ENCODER_EVENT_NONE;
static bool directionReversed = false;

//--- Button timing state
static bool buttonPressed = false;
static bool longPressReported = false;
static uint32_t buttonPressStartMs = 0;

//--- Encode current quadrature state
static uint8_t readState();

//--- Initialize encoder input
void encoderInit()
{
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_BTN, INPUT_PULLUP);

  lastState = readState();
  pendingEvent = ENCODER_EVENT_NONE;

  ESP_LOGI(logTag, "Encoder initialized");

} //   encoderInit()

//--- Update encoder input
void encoderUpdate()
{
  uint8_t currentState = readState();

  if (currentState != lastState)
  {
    if ((lastState == 0b00 && currentState == 0b01) ||
        (lastState == 0b01 && currentState == 0b11) ||
        (lastState == 0b11 && currentState == 0b10) ||
        (lastState == 0b10 && currentState == 0b00))
    {
      encoderDelta++;
    }
    else
    {
      encoderDelta--;
    }

    if (encoderDelta >= 4)
    {
      if (pendingEvent == ENCODER_EVENT_NONE)
      {
        pendingEvent = directionReversed ? ENCODER_EVENT_LEFT : ENCODER_EVENT_RIGHT;
      }
      encoderDelta = 0;
    }
    else if (encoderDelta <= -4)
    {
      if (pendingEvent == ENCODER_EVENT_NONE)
      {
        pendingEvent = directionReversed ? ENCODER_EVENT_RIGHT : ENCODER_EVENT_LEFT;
      }
      encoderDelta = 0;
    }

    lastState = currentState;
  }

  bool rawPressed = digitalRead(PIN_ENC_BTN) == LOW;

  if (rawPressed && !buttonPressed)
  {
    buttonPressed = true;
    longPressReported = false;
    buttonPressStartMs = millis();
  }
  else if (rawPressed && buttonPressed && !longPressReported)
  {
    if (millis() - buttonPressStartMs >= ENCODER_LONG_PRESS_MS)
    {
      pendingEvent = ENCODER_EVENT_LONG_PRESS;
      longPressReported = true;
    }
  }
  else if (!rawPressed && buttonPressed)
  {
    uint32_t pressDurationMs = millis() - buttonPressStartMs;

    if (!longPressReported)
    {
      if (pressDurationMs >= ENCODER_MEDIUM_PRESS_MS)
      {
        if (pendingEvent == ENCODER_EVENT_NONE)
        {
          pendingEvent = ENCODER_EVENT_MEDIUM_PRESS;
        }
      }
      else if (pressDurationMs >= ENCODER_SHORT_PRESS_MS)
      {
        if (pendingEvent == ENCODER_EVENT_NONE)
        {
          pendingEvent = ENCODER_EVENT_SHORT_PRESS;
        }
      }
    }

    buttonPressed = false;
    longPressReported = false;
  }

} //   encoderUpdate()

//--- Get next encoder event
EncoderEvent encoderGetEvent()
{
  EncoderEvent event = pendingEvent;
  pendingEvent = ENCODER_EVENT_NONE;

  return event;

} //   encoderGetEvent()

//--- Clear pending encoder event
void encoderClearEvent()
{
  pendingEvent = ENCODER_EVENT_NONE;

} //   encoderClearEvent()

//--- Set encoder direction reversal state
void encoderSetDirectionReversed(bool reversed)
{
  directionReversed = reversed;

} //   encoderSetDirectionReversed()

//--- Get encoder direction reversal state
bool encoderGetDirectionReversed()
{
  return directionReversed;

} //   encoderGetDirectionReversed()

//--- Encode current quadrature state
static uint8_t readState()
{
  uint8_t a = static_cast<uint8_t>(digitalRead(PIN_ENC_A));
  uint8_t b = static_cast<uint8_t>(digitalRead(PIN_ENC_B));

  return static_cast<uint8_t>((a << 1) | b);

} //   readState()
