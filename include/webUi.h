#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

//--- Initialize web UI
void webUiInit();

//--- Update web UI
void webUiUpdate();

//--- Suspend web UI server (free port 80 for WiFiManager portal)
void webUiSuspend();

//--- Resume web UI server
void webUiResume();

#endif
