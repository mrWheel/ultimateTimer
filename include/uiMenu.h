/*** Last Changed: 2026-04-15 - 13:12 ***/
#ifndef UI_MENU_H
#define UI_MENU_H

#include <Arduino.h>

//--- Initialize UI menu
void uiMenuInit();

//--- Update UI menu
void uiMenuUpdate();

//--- Request short status message
void uiMenuShowTransientMessage(const String& message);

#endif
