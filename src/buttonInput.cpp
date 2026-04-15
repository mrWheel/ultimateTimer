/*** Last Changed: 2026-04-15 - 13:12 ***/
#include "buttonInput.h"
#include "appConfig.h"

#include <esp_log.h>

//--- Logging tag
static const char* logTag = "buttonInput";

//--- Button state
static ButtonEvent pendingEvent = BUTTON_EVENT_NONE;
static bool buttonPressed = false;
static bool longPressReported = false;
static uint32_t buttonPressStartMs = 0;

//--- Initialize auxiliary button input
void buttonInit()
{
  pinMode(PIN_KEY0, INPUT_PULLUP);

  ESP_LOGI(logTag, "Auxiliary button initialized");

} //   buttonInit()

//--- Update auxiliary button input
void buttonUpdate()
{
  bool rawPressed = digitalRead(PIN_KEY0) == LOW;

  if (rawPressed && !buttonPressed)
  {
    buttonPressed = true;
    longPressReported = false;
    buttonPressStartMs = millis();
  }
  else if (rawPressed && buttonPressed && !longPressReported)
  {
    if (millis() - buttonPressStartMs >= BUTTON_LONG_PRESS_MS)
    {
      pendingEvent = BUTTON_EVENT_LONG_PRESS;
      longPressReported = true;
    }
  }
  else if (!rawPressed && buttonPressed)
  {
    uint32_t pressDurationMs = millis() - buttonPressStartMs;

    if (!longPressReported)
    {
      if (pressDurationMs >= BUTTON_MEDIUM_PRESS_MS)
      {
        pendingEvent = BUTTON_EVENT_MEDIUM_PRESS;
      }
      else if (pressDurationMs >= BUTTON_SHORT_PRESS_MS)
      {
        pendingEvent = BUTTON_EVENT_SHORT_PRESS;
      }
    }

    buttonPressed = false;
    longPressReported = false;
  }

} //   buttonUpdate()

//--- Get next auxiliary button event
ButtonEvent buttonGetEvent()
{
  ButtonEvent event = pendingEvent;
  pendingEvent = BUTTON_EVENT_NONE;

  return event;

} //   buttonGetEvent()

//--- Clear pending auxiliary button event
void buttonClearEvent()
{
  pendingEvent = BUTTON_EVENT_NONE;

} //   buttonClearEvent()
