#include "ioControl.h"
#include "appConfig.h"

#include <esp_log.h>

//--- Logging tag
static const char *logTag = "ioControl";

//--- Input edge tracking
static bool lastTriggerState = false;
static bool lastResetState = false;

//--- Initialize I/O control
void ioInit()
{
  pinMode(PIN_OUTPUT, OUTPUT);
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  pinMode(PIN_RESET, INPUT_PULLUP);

  digitalWrite(PIN_OUTPUT, LOW);

  lastTriggerState = ioGetRawTriggerState();
  lastResetState = ioGetRawResetState();

  ESP_LOGI(logTag, "I/O initialized");

}   //   ioInit()

//--- Update output according to runtime status and settings
void ioUpdate(const RuntimeStatus &runtimeStatus, const AppSettings &settings)
{
  bool effectiveState = runtimeStatus.outputActive;

  if (!settings.outputPolarityHigh)
  {
    effectiveState = !effectiveState;
  }

  digitalWrite(PIN_OUTPUT, effectiveState ? HIGH : LOW);

}   //   ioUpdate()

//--- Detect trigger edge
bool ioTriggerActivated(TriggerEdge edge)
{
  bool currentState = ioGetRawTriggerState();
  bool activated = false;

  if (edge == TRIGGER_EDGE_RISING)
  {
    activated = !lastTriggerState && currentState;
  }
  else
  {
    activated = lastTriggerState && !currentState;
  }

  lastTriggerState = currentState;

  return activated;

}   //   ioTriggerActivated()

//--- Detect reset input edge
bool ioResetActivated()
{
  bool currentState = ioGetRawResetState();
  bool activated = lastResetState && !currentState;

  lastResetState = currentState;

  return activated;

}   //   ioResetActivated()

//--- Get raw trigger input state
bool ioGetRawTriggerState()
{
  return digitalRead(PIN_TRIGGER) == HIGH;

}   //   ioGetRawTriggerState()

//--- Get raw reset input state
bool ioGetRawResetState()
{
  return digitalRead(PIN_RESET) == HIGH;

}   //   ioGetRawResetState()
