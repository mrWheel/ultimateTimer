/*** Last Changed: 2026-05-12 - 11:43 ***/
#include "warpMachine.h"
#include "appConfig.h"
#include "settingsStore.h"

#include <Arduino.h>

//--- Startup time (captured once at first call)
static time_t startupTime = 0;
static uint32_t startupMs = 0;
static bool initialized = false;

//--- Get current time with optional warp speed multiplier
time_t warpMachineNow()
{
  if (!initialized)
  {
    startupTime = time(nullptr);
    startupMs = millis();
    initialized = true;

    return startupTime;
  }

  //-- Check runtime warp speed setting
  if (settingsStoreLoadWarpSpeedEnabled())
  {
    //-- Warp speed enabled: each real second counts as 60 seconds
    uint32_t elapsedMs = millis() - startupMs;
    uint32_t elapsedSeconds = elapsedMs / 1000UL;
    uint32_t warpedSeconds = elapsedSeconds * 60UL;

    return startupTime + warpedSeconds;
  }
  else
  {
    //-- Normal time mode
    return time(nullptr);
  }
}
