/*** Last Changed: 2026-04-18 - 15:49 ***/
#ifndef UI_MENU_H
#define UI_MENU_H

#include <Arduino.h>
#include <string>

//--- Initialize UI menu
void uiMenuInit();

//--- Update UI menu
void uiMenuUpdate();

//--- Request short status message
void uiMenuShowTransientMessage(const std::string& message);

//--- Force switch to Timer Screen and redraw immediately
void uiMenuForceTimerScreen();

#endif
