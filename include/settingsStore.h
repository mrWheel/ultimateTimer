/*** Last Changed: 2026-04-15 - 13:12 ***/
#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include <Arduino.h>
#include "timerTypes.h"

//--- Initialize settings storage
void settingsStoreInit();

//--- Load WiFi settings
void settingsStoreLoadWifiSettings(WifiSettings& wifiSettings);

//--- Save WiFi settings
void settingsStoreSaveWifiSettings(const WifiSettings& wifiSettings);

//--- Erase stored station WiFi credentials
void settingsStoreEraseWifiCredentials();

//--- Load last profile name
String settingsStoreLoadLastProfileName();

//--- Save last profile name
void settingsStoreSaveLastProfileName(const String& profileName);

//--- Load encoder direction reversal state
bool settingsStoreLoadEncoderDirectionReversed();

//--- Save encoder direction reversal state
void settingsStoreSaveEncoderDirectionReversed(bool reversed);

//--- Load output polarity default state
bool settingsStoreLoadOutputPolarityHigh();

//--- Save output polarity default state
void settingsStoreSaveOutputPolarityHigh(bool activeHigh);

#endif
