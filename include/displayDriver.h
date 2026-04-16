/*** Last Changed: 2026-04-16 - 14:10 ***/
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include "timerTypes.h"

//--- Initialize display
void displayInit();

//--- Update status screen
void displayDrawStatusScreen(const AppSettings& settings, const RuntimeStatus& runtimeStatus, bool wifiConnected, int statusActionIndex);

//--- Draw text list
void displayDrawListScreen(const char* title, const String items[], size_t itemCount, int selectedIndex, int firstVisibleIndex);

//--- Draw numeric editor
void displayDrawNumberEditor(const char* label, uint32_t value, const char* unitLabel);

//--- Draw enum editor
void displayDrawEnumEditor(const char* label, const char* valueLabel);

//--- Draw text input editor
void displayDrawTextInput(const char* title, const String& textValue, const String& currentToken);

//--- Draw generic field input screen
void displayDrawFieldInput(const char* title, const char* fieldName, const String& fieldValue, int cursorIndex, const String& prevToken, const String& currentToken, const String& nextToken);

//--- Draw message screen
void displayDrawMessage(const char* title, const char* message);

//--- Draw centered WiFi portal info screen
void displayDrawWifiPortalScreen(const String& line1, const String& line2, const String& line3, const String& line4);

//--- Draw centered startup connection screen
void displayDrawStartupConnectionScreen(const String& line1, const String& line2);

//--- Draw color palette test pattern
void displayDrawTestColorPattern();

//--- Set display backlight state
void displaySetBacklight(bool enabled);

#endif
