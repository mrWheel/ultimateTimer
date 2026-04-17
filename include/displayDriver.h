/*** Last Changed: 2026-04-17 - 10:25 ***/
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <string>
#include "timerTypes.h"
#include "colorSettings.h"

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
void displayDrawTextInput(const char* title, const std::string& textValue, const std::string& currentToken);

//--- Draw generic field input screen
void displayDrawFieldInput(const char* title, const char* fieldName, const std::string& fieldValue, int cursorIndex, const std::string& prevToken, const std::string& currentToken, const std::string& nextToken);

//--- Draw message screen
void displayDrawMessage(const char* title, const char* message);

//--- Draw centered WiFi portal info screen
void displayDrawWifiPortalScreen(const std::string& line1, const std::string& line2, const std::string& line3, const std::string& line4);

//--- Draw centered startup connection screen
void displayDrawStartupConnectionScreen(const std::string& line1, const std::string& line2);

//--- Draw color palette test pattern
void displayDrawTestColorPattern();

//--- Draw test palette with dark bars and cursor
void displayDrawTestColorPalette(int selectedIndex);

//--- Draw color fade test for selected color
void displayDrawTestColorFade(const char* colorName, uint16_t darkColorVisual, uint16_t lightColorVisual, VisualTextColor darkLabelColor, VisualTextColor lightLabelColor);

//--- Set display backlight state
void displaySetBacklight(bool enabled);

#endif
