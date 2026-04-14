#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include <Arduino.h>
#include "timerTypes.h"

//--- Initialize settings storage
void settingsStoreInit();

//--- Load WiFi settings
void settingsStoreLoadWifiSettings(WifiSettings &wifiSettings);

//--- Save WiFi settings
void settingsStoreSaveWifiSettings(const WifiSettings &wifiSettings);

//--- Erase stored station WiFi credentials
void settingsStoreEraseWifiCredentials();

//--- Load last profile name
String settingsStoreLoadLastProfileName();

//--- Save last profile name
void settingsStoreSaveLastProfileName(const String &profileName);

#endif
