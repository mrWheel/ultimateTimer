#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include "timerTypes.h"

//--- Initialize display
void displayInit();

//--- Update status screen
void displayDrawStatusScreen(const AppSettings &settings, const RuntimeStatus &runtimeStatus, bool wifiConnected);

//--- Draw text list
void displayDrawListScreen(const char *title, const String items[], size_t itemCount, int selectedIndex, int firstVisibleIndex);

//--- Draw numeric editor
void displayDrawNumberEditor(const char *label, uint32_t value, const char *unitLabel);

//--- Draw enum editor
void displayDrawEnumEditor(const char *label, const char *valueLabel);

//--- Draw text input editor
void displayDrawTextInput(const char *title, const String &textValue, const String &currentToken);

//--- Draw message screen
void displayDrawMessage(const char *title, const char *message);

//--- Set display backlight state
void displaySetBacklight(bool enabled);

#endif
