#ifndef WIFI_MANAGER_EXT_H
#define WIFI_MANAGER_EXT_H

#include <Arduino.h>
#include "timerTypes.h"

//--- Initialize WiFi manager service
void wifiManagerInit();

//--- Update WiFi manager service
void wifiManagerUpdate();

//--- Apply stored WiFi settings
void wifiManagerApplySettings(const WifiSettings &wifiSettings);

//--- Get current WiFi settings
const WifiSettings &wifiManagerGetSettings();

//--- Save and apply WiFi settings
void wifiManagerSaveAndApplySettings(const WifiSettings &wifiSettings);

//--- Scan available access points and return unique SSIDs
size_t wifiManagerScanNetworks(String ssids[], size_t maxCount);

//--- Check whether station is connected
bool wifiManagerIsStaConnected();

//--- Check whether WiFi setup portal should be shown
bool wifiManagerShouldOpenPortal();

//--- Get local URL string
String wifiManagerGetAddressString();

//--- Initialize WiFiManager extension
void wifiManagerExtInit(const WifiSettings &wifiSettings);

//--- Update WiFiManager extension
void wifiManagerExtUpdate();

//--- Apply WiFi settings and reconnect flow
void wifiManagerExtApplySettings(const WifiSettings &wifiSettings);

//--- Check whether STA is connected
bool wifiManagerExtIsStaConnected();

//--- Check whether config portal is active
bool wifiManagerExtIsPortalActive();

//--- Get current address string (STA or AP)
String wifiManagerExtGetAddressString();

//--- Consume newly configured STA credentials from portal
bool wifiManagerExtConsumeNewStaCredentials(WifiSettings &wifiSettings);

#endif
