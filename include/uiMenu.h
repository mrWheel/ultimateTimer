/*** Last Changed: 2026-04-17 - 11:02 ***/
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

#endif
