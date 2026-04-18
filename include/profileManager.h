/*** Last Changed: 2026-04-18 - 15:49 ***/
#ifndef PROFILE_MANAGER_H
#define PROFILE_MANAGER_H

#include <Arduino.h>
#include "timerTypes.h"

//--- Maximum number of profiles returned in one list
static const size_t profileManagerMaxProfiles = 32;
//--- Built-in default profile name
extern const char* const profileManagerDefaultProfileName;

//--- Initialize profile manager
bool profileManagerInit();

//--- Save profile
bool profileManagerSaveProfile(const String& profileName, const AppSettings& settings);

//--- Load profile
bool profileManagerLoadProfile(const String& profileName, AppSettings& settings);

//--- Delete profile
bool profileManagerDeleteProfile(const String& profileName);

//--- Get profile names
size_t profileManagerListProfiles(String profileNames[], size_t maxProfiles);

//--- Check whether profile exists
bool profileManagerProfileExists(const String& profileName);

#endif
