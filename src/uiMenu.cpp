/*** Last Changed: 2026-04-15 - 16:11 ***/
#include "uiMenu.h"
#include "buttonInput.h"
#include "displayDriver.h"
#include "encoderInput.h"
#include "profileManager.h"
#include "settingsStore.h"
#include "timerEngine.h"
#include "WiFiManagerExt.h"

#include <WiFi.h>
#include <esp_log.h>

//--- Main menu item identifiers
enum MainMenuItem
{
  MENU_ITEM_TIMER_SETTINGS = 0,
  MENU_ITEM_SAVE_PROFILE = 1,
  MENU_ITEM_LOAD_PROFILE = 2,
  MENU_ITEM_NEW_PROFILE = 3,
  MENU_ITEM_DELETE_PROFILE = 4,
  MENU_ITEM_SHOW_SYSTEM_SETTINGS = 5,
  MENU_ITEM_EXIT = 6
};

//--- Timer settings menu item identifiers
enum TimerSettingsItem
{
  TIMER_SETTINGS_ITEM_ON_TIME = 0,
  TIMER_SETTINGS_ITEM_ON_UNIT = 1,
  TIMER_SETTINGS_ITEM_OFF_TIME = 2,
  TIMER_SETTINGS_ITEM_OFF_UNIT = 3,
  TIMER_SETTINGS_ITEM_REPEAT_COUNT = 4,
  TIMER_SETTINGS_ITEM_TRIGGER_EDGE = 5,
  TIMER_SETTINGS_ITEM_EXIT = 6
};

//--- System settings menu item identifiers
enum SystemSettingsItem
{
  SYSTEM_SETTINGS_ITEM_WIFI_SSID = 0,
  SYSTEM_SETTINGS_ITEM_IP_ADDRESS = 1,
  SYSTEM_SETTINGS_ITEM_MAC_ADDRESS = 2,
  SYSTEM_SETTINGS_ITEM_WIFI_DISABLED = 3,
  SYSTEM_SETTINGS_ITEM_ENCODER_ORDER = 4,
  SYSTEM_SETTINGS_ITEM_ERASE_WIFI = 5,
  SYSTEM_SETTINGS_ITEM_START_WIFI_MANAGER = 6,
  SYSTEM_SETTINGS_ITEM_OUTPUT_POLARITY = 7,
  SYSTEM_SETTINGS_ITEM_RESTART_ULTIMATE_TIMER = 8,
  SYSTEM_SETTINGS_ITEM_EXIT = 9
};

//--- Profile list modes
enum ProfileListMode
{
  PROFILE_LIST_LOAD = 0,
  PROFILE_LIST_DELETE = 1
};

//--- Field input targets
enum FieldInputTarget
{
  FIELD_INPUT_TARGET_NONE = 0,
  FIELD_INPUT_TARGET_SAVE_PROFILE = 1,
  FIELD_INPUT_TARGET_NEW_PROFILE = 2,
  FIELD_INPUT_TARGET_ON_TIME = 3,
  FIELD_INPUT_TARGET_OFF_TIME = 4,
  FIELD_INPUT_TARGET_REPEAT_COUNT = 5,
  FIELD_INPUT_TARGET_ON_UNIT = 6,
  FIELD_INPUT_TARGET_OFF_UNIT = 7,
  FIELD_INPUT_TARGET_TRIGGER_EDGE = 8,
  FIELD_INPUT_TARGET_ERASE_WIFI_CONFIRM = 9,
  FIELD_INPUT_TARGET_DELETE_PROFILE_CONFIRM = 10
};

//--- Main menu labels
static const String mainMenuItems[] =
    {
        "Timer Settings",
        "Save Profile",
        "Load Profile",
        "New Profile",
        "Delete Profile",
        "Show System Settings",
        "Exit"};

//--- Timer settings menu labels
static const String timerSettingsMenuItems[] =
    {
        "On Time",
        "On Time Unit",
        "Off Time",
        "Off Time Unit",
        "Number of Cycles",
        "Trigger (Rise/Fall)",
        "Exit"};

//--- System settings menu labels
static const String systemSettingsMenuItems[] =
    {
        "WiFi SSID",
        "IP Address",
        "MAC Address",
        "WiFi Disabled",
        "Encoder Order",
        "Erase WiFi credentials",
        "Start WiFi Manager",
        "Output Polarity",
        "Restart ultimateTimer",
        "Exit"};

//--- Numeric field tokens
static const String numericTokens[] =
    {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};

//--- Alphanumeric field tokens
static const String alphaNumericTokens[] =
    {
        "A", "a", "B", "b", "C", "c", "D", "d", "E", "e", "F", "f", "G", "g", "H", "h", "I", "i", "J", "j", "K", "k", "L", "l", "M", "m",
        "N", "n", "O", "o", "P", "p", "Q", "q", "R", "r", "S", "s", "T", "t", "U", "u", "V", "v", "W", "w", "X", "x", "Y", "y", "Z", "z",
        "-", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};

//--- Time unit field tokens
static const String timeUnitTokens[] =
    {
        "ms", "s", "Min"};

//--- Trigger edge field tokens
static const String triggerEdgeTokens[] =
    {
        "Falling", "Rising"};

//--- Yes/No field tokens
static const String yesNoTokens[] =
    {
        "Y", "N"};

//--- Maximum field input positions
static const int maxFieldPositions = 12;

//--- Current screen
static UiScreen currentScreen = UI_SCREEN_STATUS;

//--- Menu indexes and visible offsets
static int mainMenuIndex = 0;
static int mainMenuFirstVisibleIndex = 0;
static int timerSettingsIndex = 0;
static int timerSettingsFirstVisibleIndex = 0;
static int systemSettingsIndex = SYSTEM_SETTINGS_ITEM_ENCODER_ORDER;
static int statusActionIndex = 0;
static UiScreen screenBeforeWifiPortal = UI_SCREEN_STATUS;

//--- Profile list state
static ProfileListMode profileListMode = PROFILE_LIST_LOAD;
static String profileNames[profileManagerMaxProfiles + 1];
static size_t profileCount = 0;
static int profileIndex = 0;
static int profileFirstVisibleIndex = 0;
static String pendingDeleteProfileName = "";

//--- Field input state
static String fieldInputTitle = "Field Input";
static String fieldInputName = "Field";
static const String* fieldInputTokenList = nullptr;
static int fieldInputTokenCount = 0;
static int fieldInputPositionCount = 0;
static int fieldInputCursorPosition = 0;
static int fieldInputTokenIndexes[maxFieldPositions];
static FieldInputTarget fieldInputTarget = FIELD_INPUT_TARGET_NONE;
static UiScreen fieldInputReturnScreen = UI_SCREEN_MAIN_MENU;

//--- Transient message
static String transientTitle = "Message";
static String transientMessage = "";
static uint32_t transientMessageUntilMs = 0;

//--- Status refresh timing
static const uint32_t statusRefreshIntervalMs = 100;
static uint32_t lastStatusRefreshMs = 0;

//--- Draw current screen
static void drawCurrentScreen();

//--- Open status screen
static void openStatusScreen();

//--- Open main menu
static void openMainMenu();

//--- Open timer settings menu
static void openTimerSettingsMenu();

//--- Open system settings menu
static void openSystemSettingsMenu();

//--- Open profile list screen
static void openProfileList(ProfileListMode mode);

//--- Handle status screen
static void handleStatusScreen(EncoderEvent encoderEvent);

//--- Handle main menu
static void handleMainMenu(EncoderEvent encoderEvent);

//--- Handle timer settings menu
static void handleTimerSettingsMenu(EncoderEvent encoderEvent);

//--- Handle system settings menu
static void handleSystemSettingsMenu(EncoderEvent encoderEvent);

//--- Handle WiFi portal info screen
static void handleWifiPortalScreen(EncoderEvent encoderEvent);

//--- Handle profile list
static void handleProfileList(EncoderEvent encoderEvent);

//--- Open generic field input
static void openFieldInput(const String& title, const String& fieldName, int positionCount, const String tokens[], int tokenCount, FieldInputTarget target, UiScreen returnScreen, const String& initialValue);

//--- Handle generic field input
static void handleFieldInput(EncoderEvent encoderEvent);

//--- Apply and store field input value
static void applyFieldInputAndReturn();

//--- Build current field input text
static String buildFieldInputValue();

//--- Find token index by value
static int findTokenIndex(const String tokens[], int tokenCount, const String& tokenValue);

//--- Build zero-padded numeric value with fixed width
static String buildFixedWidthNumber(uint32_t value, int width);

//--- Refresh profile list and append Exit entry
static void refreshProfileListWithExit();

//--- Apply current settings back into timer
static void commitSettings(const AppSettings& settings);

//--- Log active menu or screen
static void logActiveScreen(const char* screenName);

//--- Build readable AP SSID lines for portal info screen
static void buildPortalApLines(const String& apSsid, String& line1, String& line2);

//--- Convert encoder event to text
static const char* encoderEventToLabel(EncoderEvent event);

//--- Convert UI screen to text
static const char* uiScreenToLabel(UiScreen screen);

//--- Build current field input text
static String buildFieldInputValue()
{
  String fieldValue = "";

  for (int position = 0; position < fieldInputPositionCount; position++)
  {
    fieldValue += fieldInputTokenList[fieldInputTokenIndexes[position]];
  }

  return fieldValue;

} //   buildFieldInputValue()

//--- Find token index by value
static int findTokenIndex(const String tokens[], int tokenCount, const String& tokenValue)
{
  for (int tokenIndex = 0; tokenIndex < tokenCount; tokenIndex++)
  {
    if (tokens[tokenIndex] == tokenValue)
    {
      return tokenIndex;
    }
  }

  return 0;

} //   findTokenIndex()

//--- Build zero-padded numeric value with fixed width
static String buildFixedWidthNumber(uint32_t value, int width)
{
  char valueBuffer[20];
  char formatBuffer[10];
  String paddedValue;

  snprintf(formatBuffer, sizeof(formatBuffer), "%%0%du", width);
  snprintf(valueBuffer, sizeof(valueBuffer), formatBuffer, static_cast<unsigned long>(value));
  paddedValue = String(valueBuffer);

  if (static_cast<int>(paddedValue.length()) > width)
  {
    paddedValue = paddedValue.substring(paddedValue.length() - width);
  }

  return paddedValue;

} //   buildFixedWidthNumber()

//--- Open generic field input
static void openFieldInput(const String& title, const String& fieldName, int positionCount, const String tokens[], int tokenCount, FieldInputTarget target, UiScreen returnScreen, const String& initialValue)
{
  fieldInputTitle = title;
  fieldInputName = fieldName;
  fieldInputTokenList = tokens;
  fieldInputTokenCount = tokenCount;
  fieldInputPositionCount = positionCount;
  fieldInputCursorPosition = 0;
  fieldInputTarget = target;
  fieldInputReturnScreen = returnScreen;

  if (fieldInputPositionCount > maxFieldPositions)
  {
    fieldInputPositionCount = maxFieldPositions;
  }

  for (int position = 0; position < fieldInputPositionCount; position++)
  {
    fieldInputTokenIndexes[position] = 0;
  }

  if (fieldInputPositionCount > 0)
  {
    int tokenLength = static_cast<int>(tokens[0].length());

    if (tokenLength <= 0)
    {
      tokenLength = 1;
    }

    for (int position = 0; position < fieldInputPositionCount; position++)
    {
      int startIndex = position * tokenLength;
      String tokenValue;

      if (startIndex >= static_cast<int>(initialValue.length()))
      {
        break;
      }

      tokenValue = initialValue.substring(startIndex, startIndex + tokenLength);
      fieldInputTokenIndexes[position] = findTokenIndex(tokens, tokenCount, tokenValue);
    }
  }

  currentScreen = UI_SCREEN_FIELD_INPUT;
  logActiveScreen(title.c_str());
  drawCurrentScreen();

} //   openFieldInput()

//--- Apply and store field input value
static void applyFieldInputAndReturn()
{
  AppSettings settings = timerGetSettings();
  String fieldValue = buildFieldInputValue();

  switch (fieldInputTarget)
  {
  case FIELD_INPUT_TARGET_SAVE_PROFILE:
    while (fieldValue.endsWith("-"))
    {
      fieldValue.remove(fieldValue.length() - 1);
    }

    if (fieldValue.isEmpty())
    {
      uiMenuShowTransientMessage("Empty profile name");

      return;
    }

    settings.profileName = fieldValue;

    if (profileManagerSaveProfile(settings.profileName, settings))
    {
      commitSettings(settings);
      settingsStoreSaveLastProfileName(settings.profileName);
      uiMenuShowTransientMessage("Profile saved");
    }
    else
    {
      uiMenuShowTransientMessage("Save failed");
    }
    break;

  case FIELD_INPUT_TARGET_NEW_PROFILE:
    while (fieldValue.endsWith("-"))
    {
      fieldValue.remove(fieldValue.length() - 1);
    }

    if (fieldValue.isEmpty())
    {
      uiMenuShowTransientMessage("Empty profile name");

      return;
    }

    settings.profileName = fieldValue;

    if (profileManagerSaveProfile(settings.profileName, settings))
    {
      commitSettings(settings);
      settingsStoreSaveLastProfileName(settings.profileName);
      uiMenuShowTransientMessage("New profile saved");
    }
    else
    {
      uiMenuShowTransientMessage("Save failed");
    }
    break;

  case FIELD_INPUT_TARGET_ON_TIME:
    settings.onTimeValue = static_cast<uint32_t>(fieldValue.toInt());
    commitSettings(settings);
    break;

  case FIELD_INPUT_TARGET_OFF_TIME:
    settings.offTimeValue = static_cast<uint32_t>(fieldValue.toInt());
    commitSettings(settings);
    break;

  case FIELD_INPUT_TARGET_REPEAT_COUNT:
    settings.repeatCount = static_cast<uint32_t>(fieldValue.toInt());
    commitSettings(settings);
    break;

  case FIELD_INPUT_TARGET_ON_UNIT:
    if (fieldValue == "ms")
    {
      settings.onTimeUnit = TIME_UNIT_MS;
    }
    else if (fieldValue == "s")
    {
      settings.onTimeUnit = TIME_UNIT_SECONDS;
    }
    else
    {
      settings.onTimeUnit = TIME_UNIT_MINUTES;
    }
    commitSettings(settings);
    break;

  case FIELD_INPUT_TARGET_OFF_UNIT:
    if (fieldValue == "ms")
    {
      settings.offTimeUnit = TIME_UNIT_MS;
    }
    else if (fieldValue == "s")
    {
      settings.offTimeUnit = TIME_UNIT_SECONDS;
    }
    else
    {
      settings.offTimeUnit = TIME_UNIT_MINUTES;
    }
    commitSettings(settings);
    break;

  case FIELD_INPUT_TARGET_TRIGGER_EDGE:
    settings.triggerEdge = (fieldValue == "Rising") ? TRIGGER_EDGE_RISING : TRIGGER_EDGE_FALLING;
    commitSettings(settings);
    break;

  case FIELD_INPUT_TARGET_ERASE_WIFI_CONFIRM:
    if (fieldValue == "Y")
    {
      settingsStoreEraseWifiCredentials();
      settingsStoreSaveWifiDisabled(true);
      displayDrawMessage("WiFi Disabled", "Restarting...");
      delay(250);
      ESP.restart();
    }
    else
    {
      uiMenuShowTransientMessage("Erase cancelled");
    }
    break;

  case FIELD_INPUT_TARGET_DELETE_PROFILE_CONFIRM:
    if (fieldValue == "Y")
    {
      if (profileManagerDeleteProfile(pendingDeleteProfileName))
      {
        uiMenuShowTransientMessage("Profile deleted");
      }
      else
      {
        uiMenuShowTransientMessage("Delete failed");
      }
    }
    else
    {
      uiMenuShowTransientMessage("Delete cancelled");
    }

    refreshProfileListWithExit();
    break;

  default:
    break;
  }

  currentScreen = fieldInputReturnScreen;
  drawCurrentScreen();

} //   applyFieldInputAndReturn()

//--- Handle generic field input
static void handleFieldInput(EncoderEvent encoderEvent)
{
  bool redrawRequired = false;

  if (encoderEvent == ENCODER_EVENT_LEFT)
  {
    fieldInputTokenIndexes[fieldInputCursorPosition]--;

    if (fieldInputTokenIndexes[fieldInputCursorPosition] < 0)
    {
      fieldInputTokenIndexes[fieldInputCursorPosition] = fieldInputTokenCount - 1;
    }

    redrawRequired = true;
  }
  else if (encoderEvent == ENCODER_EVENT_RIGHT)
  {
    fieldInputTokenIndexes[fieldInputCursorPosition]++;

    if (fieldInputTokenIndexes[fieldInputCursorPosition] >= fieldInputTokenCount)
    {
      fieldInputTokenIndexes[fieldInputCursorPosition] = 0;
    }

    redrawRequired = true;
  }
  else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    if (fieldInputCursorPosition >= fieldInputPositionCount - 1)
    {
      applyFieldInputAndReturn();

      return;
    }

    fieldInputCursorPosition++;
    redrawRequired = true;
  }

  if (redrawRequired)
  {
    drawCurrentScreen();
  }

} //   handleFieldInput()

//--- Refresh profile list and append Exit entry
static void refreshProfileListWithExit()
{
  size_t loadedProfiles = profileManagerListProfiles(profileNames, profileManagerMaxProfiles);

  profileCount = loadedProfiles + 1;
  profileNames[profileCount - 1] = "Exit";

  if (profileIndex >= static_cast<int>(profileCount))
  {
    profileIndex = static_cast<int>(profileCount) - 1;
  }

  if (profileIndex < 0)
  {
    profileIndex = 0;
  }

} //   refreshProfileListWithExit()

//--- Open status screen
static void openStatusScreen()
{
  currentScreen = UI_SCREEN_STATUS;
  logActiveScreen("Timer Screen");
  drawCurrentScreen();

} //   openStatusScreen()

//--- Open main menu
static void openMainMenu()
{
  currentScreen = UI_SCREEN_MAIN_MENU;
  logActiveScreen("Edit Timer Menu");
  drawCurrentScreen();

} //   openMainMenu()

//--- Open timer settings menu
static void openTimerSettingsMenu()
{
  currentScreen = UI_SCREEN_TIMER_SETTINGS_MENU;
  logActiveScreen("Timer Settings Menu");
  drawCurrentScreen();

} //   openTimerSettingsMenu()

//--- Open system settings menu
static void openSystemSettingsMenu()
{
  systemSettingsIndex = SYSTEM_SETTINGS_ITEM_ENCODER_ORDER;
  currentScreen = UI_SCREEN_SYSTEM_SETTINGS_MENU;
  logActiveScreen("Show System Settings Menu");
  drawCurrentScreen();

} //   openSystemSettingsMenu()

//--- Open profile list screen
static void openProfileList(ProfileListMode mode)
{
  profileListMode = mode;
  profileIndex = 0;
  profileFirstVisibleIndex = 0;
  refreshProfileListWithExit();

  currentScreen = UI_SCREEN_PROFILE_LIST;

  if (mode == PROFILE_LIST_LOAD)
  {
    logActiveScreen("Load Profile Menu");
  }
  else
  {
    logActiveScreen("Delete Profile Menu");
  }

  drawCurrentScreen();

} //   openProfileList()

//--- Draw current screen
static void drawCurrentScreen()
{
  if (!transientMessage.isEmpty())
  {
    displayDrawMessage(transientTitle.c_str(), transientMessage.c_str());

    return;
  }

  const AppSettings& settings = timerGetSettings();
  RuntimeStatus runtimeStatus = timerGetRuntimeStatus();

  switch (currentScreen)
  {
  case UI_SCREEN_STATUS:
    displayDrawStatusScreen(settings, runtimeStatus, true, statusActionIndex);
    break;

  case UI_SCREEN_MAIN_MENU:
    displayDrawListScreen("Edit Timer Menu", mainMenuItems, sizeof(mainMenuItems) / sizeof(mainMenuItems[0]), mainMenuIndex, mainMenuFirstVisibleIndex);
    break;

  case UI_SCREEN_TIMER_SETTINGS_MENU:
    displayDrawListScreen("Timer Settings Menu", timerSettingsMenuItems, sizeof(timerSettingsMenuItems) / sizeof(timerSettingsMenuItems[0]), timerSettingsIndex, timerSettingsFirstVisibleIndex);
    break;

  case UI_SCREEN_SYSTEM_SETTINGS_MENU:
  {
    String dynamicSystemSettingsItems[10];
    int visibleItemLogicalIndexes[10];
    int visibleItemCount = 0;
    int selectedVisibleIndex = 0;
    int firstVisibleIndex = 0;
    String wifiSsid = wifiManagerGetSettings().staSsid;
    bool wifiDisabled = settingsStoreLoadWifiDisabled();

    if (wifiSsid.isEmpty())
    {
      wifiSsid = "<none>";
    }

    if (!wifiDisabled)
    {
      dynamicSystemSettingsItems[visibleItemCount] = String("SSID: ") + wifiSsid;
      visibleItemLogicalIndexes[visibleItemCount] = SYSTEM_SETTINGS_ITEM_WIFI_SSID;
      visibleItemCount++;

      dynamicSystemSettingsItems[visibleItemCount] = String("IP: ") + wifiManagerGetAddressString();
      visibleItemLogicalIndexes[visibleItemCount] = SYSTEM_SETTINGS_ITEM_IP_ADDRESS;
      visibleItemCount++;
    }

    dynamicSystemSettingsItems[visibleItemCount] = String("MAC: ") + WiFi.macAddress();
    visibleItemLogicalIndexes[visibleItemCount] = SYSTEM_SETTINGS_ITEM_MAC_ADDRESS;
    visibleItemCount++;

    if (wifiDisabled)
    {
      dynamicSystemSettingsItems[visibleItemCount] = "WiFi Disabled: Yes";
      visibleItemLogicalIndexes[visibleItemCount] = SYSTEM_SETTINGS_ITEM_WIFI_DISABLED;
      visibleItemCount++;
    }

    dynamicSystemSettingsItems[visibleItemCount] = String("Encoder Order: ") + String(encoderGetDirectionReversed() ? "B-A" : "A-B");
    visibleItemLogicalIndexes[visibleItemCount] = SYSTEM_SETTINGS_ITEM_ENCODER_ORDER;
    visibleItemCount++;

    dynamicSystemSettingsItems[visibleItemCount] = systemSettingsMenuItems[5];
    visibleItemLogicalIndexes[visibleItemCount] = SYSTEM_SETTINGS_ITEM_ERASE_WIFI;
    visibleItemCount++;

    dynamicSystemSettingsItems[visibleItemCount] = systemSettingsMenuItems[6];
    visibleItemLogicalIndexes[visibleItemCount] = SYSTEM_SETTINGS_ITEM_START_WIFI_MANAGER;
    visibleItemCount++;

    dynamicSystemSettingsItems[visibleItemCount] = String("Output: ") + String(settings.outputPolarityHigh ? "Active High" : "Active Low");
    visibleItemLogicalIndexes[visibleItemCount] = SYSTEM_SETTINGS_ITEM_OUTPUT_POLARITY;
    visibleItemCount++;

    dynamicSystemSettingsItems[visibleItemCount] = systemSettingsMenuItems[8];
    visibleItemLogicalIndexes[visibleItemCount] = SYSTEM_SETTINGS_ITEM_RESTART_ULTIMATE_TIMER;
    visibleItemCount++;

    dynamicSystemSettingsItems[visibleItemCount] = systemSettingsMenuItems[9];
    visibleItemLogicalIndexes[visibleItemCount] = SYSTEM_SETTINGS_ITEM_EXIT;
    visibleItemCount++;

    for (int itemIndex = 0; itemIndex < visibleItemCount; itemIndex++)
    {
      if (visibleItemLogicalIndexes[itemIndex] == systemSettingsIndex)
      {
        selectedVisibleIndex = itemIndex;
        break;
      }
    }

    if (selectedVisibleIndex > 8)
    {
      firstVisibleIndex = selectedVisibleIndex - 8;
    }

    displayDrawListScreen("Show System Settings", dynamicSystemSettingsItems, visibleItemCount, selectedVisibleIndex, firstVisibleIndex);
    break;
  }

  case UI_SCREEN_PROFILE_LIST:
    if (profileListMode == PROFILE_LIST_LOAD)
    {
      displayDrawListScreen("Load Profile Menu", profileNames, profileCount, profileIndex, profileFirstVisibleIndex);
    }
    else
    {
      displayDrawListScreen("Delete Profile Menu", profileNames, profileCount, profileIndex, profileFirstVisibleIndex);
    }
    break;

  case UI_SCREEN_FIELD_INPUT:
  {
    String fieldValue = buildFieldInputValue();
    int currentTokenIndex = fieldInputTokenIndexes[fieldInputCursorPosition];
    int prevTokenIndex = currentTokenIndex - 1;
    int nextTokenIndex = currentTokenIndex + 1;

    if (prevTokenIndex < 0)
    {
      prevTokenIndex = fieldInputTokenCount - 1;
    }

    if (nextTokenIndex >= fieldInputTokenCount)
    {
      nextTokenIndex = 0;
    }

    displayDrawFieldInput(fieldInputTitle.c_str(), fieldInputName.c_str(), fieldValue, fieldInputCursorPosition, fieldInputTokenList[prevTokenIndex], fieldInputTokenList[currentTokenIndex], fieldInputTokenList[nextTokenIndex]);
    break;
  }

  case UI_SCREEN_WIFI_MANAGER_PORTAL:
  {
    String line1;
    String line2;
    String line3;

    buildPortalApLines(wifiManagerGetPortalApSsid(), line1, line2);
    line3 = line2;

    if (line3.isEmpty())
    {
      line3 = " ";
    }

    displayDrawWifiPortalScreen("Connect to AP", line1, line3, "[Cancel WiFi Manager]");
    break;
  }
  }

} //   drawCurrentScreen()

//--- Handle status screen
static void handleStatusScreen(EncoderEvent encoderEvent)
{
  bool redrawRequired = false;

  if (encoderEvent == ENCODER_EVENT_LEFT)
  {
    if (statusActionIndex > 0)
    {
      statusActionIndex--;
      redrawRequired = true;
    }
  }
  else if (encoderEvent == ENCODER_EVENT_RIGHT)
  {
    if (statusActionIndex < 2)
    {
      statusActionIndex++;
      redrawRequired = true;
    }
  }
  else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    if (statusActionIndex == 0)
    {
      timerStart();
    }
    else if (statusActionIndex == 1)
    {
      timerStop();
    }
    else
    {
      timerReset();
    }

    drawCurrentScreen();

    return;
  }

  if (encoderEvent == ENCODER_EVENT_LONG_PRESS)
  {
    mainMenuIndex = 0;
    mainMenuFirstVisibleIndex = 0;
    openMainMenu();

    return;
  }

  if (redrawRequired)
  {
    drawCurrentScreen();
  }

} //   handleStatusScreen()

//--- Handle main menu
static void handleMainMenu(EncoderEvent encoderEvent)
{
  const int itemCount = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
  bool redrawRequired = false;

  if (encoderEvent == ENCODER_EVENT_LEFT)
  {
    if (mainMenuIndex > 0)
    {
      mainMenuIndex--;
    }

    redrawRequired = true;
  }
  else if (encoderEvent == ENCODER_EVENT_RIGHT)
  {
    if (mainMenuIndex < itemCount - 1)
    {
      mainMenuIndex++;
    }

    redrawRequired = true;
  }
  else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    AppSettings settings = timerGetSettings();

    switch (mainMenuIndex)
    {
    case MENU_ITEM_TIMER_SETTINGS:
      openTimerSettingsMenu();
      return;

    case MENU_ITEM_SAVE_PROFILE:
      openFieldInput("Save Profile Menu", "Profile", 8, alphaNumericTokens, sizeof(alphaNumericTokens) / sizeof(alphaNumericTokens[0]), FIELD_INPUT_TARGET_SAVE_PROFILE, UI_SCREEN_MAIN_MENU, settings.profileName);
      return;

    case MENU_ITEM_LOAD_PROFILE:
      openProfileList(PROFILE_LIST_LOAD);
      return;

    case MENU_ITEM_NEW_PROFILE:
      openFieldInput("New Profile Menu", "Profile", 8, alphaNumericTokens, sizeof(alphaNumericTokens) / sizeof(alphaNumericTokens[0]), FIELD_INPUT_TARGET_NEW_PROFILE, UI_SCREEN_MAIN_MENU, "--------");
      return;

    case MENU_ITEM_DELETE_PROFILE:
      openProfileList(PROFILE_LIST_DELETE);
      return;

    case MENU_ITEM_SHOW_SYSTEM_SETTINGS:
      openSystemSettingsMenu();
      return;

    case MENU_ITEM_EXIT:
      openStatusScreen();
      return;
    }
  }

  mainMenuFirstVisibleIndex = mainMenuIndex - 3;

  if (mainMenuFirstVisibleIndex < 0)
  {
    mainMenuFirstVisibleIndex = 0;
  }

  if (mainMenuFirstVisibleIndex > itemCount - 9)
  {
    mainMenuFirstVisibleIndex = max(0, itemCount - 9);
  }

  if (redrawRequired)
  {
    drawCurrentScreen();
  }

} //   handleMainMenu()

//--- Handle timer settings menu
static void handleTimerSettingsMenu(EncoderEvent encoderEvent)
{
  const int itemCount = sizeof(timerSettingsMenuItems) / sizeof(timerSettingsMenuItems[0]);
  bool redrawRequired = false;
  AppSettings settings = timerGetSettings();

  if (encoderEvent == ENCODER_EVENT_LEFT)
  {
    if (timerSettingsIndex > 0)
    {
      timerSettingsIndex--;
    }

    redrawRequired = true;
  }
  else if (encoderEvent == ENCODER_EVENT_RIGHT)
  {
    if (timerSettingsIndex < itemCount - 1)
    {
      timerSettingsIndex++;
    }

    redrawRequired = true;
  }
  else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    switch (timerSettingsIndex)
    {
    case TIMER_SETTINGS_ITEM_ON_TIME:
      openFieldInput("Timer Settings Menu", "On Time", 3, numericTokens, sizeof(numericTokens) / sizeof(numericTokens[0]), FIELD_INPUT_TARGET_ON_TIME, UI_SCREEN_TIMER_SETTINGS_MENU, buildFixedWidthNumber(settings.onTimeValue, 3));
      return;

    case TIMER_SETTINGS_ITEM_ON_UNIT:
      openFieldInput("Timer Settings Menu", "On Unit", 1, timeUnitTokens, sizeof(timeUnitTokens) / sizeof(timeUnitTokens[0]), FIELD_INPUT_TARGET_ON_UNIT, UI_SCREEN_TIMER_SETTINGS_MENU, String(timerGetTimeUnitLabel(settings.onTimeUnit)));
      return;

    case TIMER_SETTINGS_ITEM_OFF_TIME:
      openFieldInput("Timer Settings Menu", "Off Time", 3, numericTokens, sizeof(numericTokens) / sizeof(numericTokens[0]), FIELD_INPUT_TARGET_OFF_TIME, UI_SCREEN_TIMER_SETTINGS_MENU, buildFixedWidthNumber(settings.offTimeValue, 3));
      return;

    case TIMER_SETTINGS_ITEM_OFF_UNIT:
      openFieldInput("Timer Settings Menu", "Off Unit", 1, timeUnitTokens, sizeof(timeUnitTokens) / sizeof(timeUnitTokens[0]), FIELD_INPUT_TARGET_OFF_UNIT, UI_SCREEN_TIMER_SETTINGS_MENU, String(timerGetTimeUnitLabel(settings.offTimeUnit)));
      return;

    case TIMER_SETTINGS_ITEM_REPEAT_COUNT:
      openFieldInput("Timer Settings Menu", "Cycles", 3, numericTokens, sizeof(numericTokens) / sizeof(numericTokens[0]), FIELD_INPUT_TARGET_REPEAT_COUNT, UI_SCREEN_TIMER_SETTINGS_MENU, buildFixedWidthNumber(settings.repeatCount, 3));
      return;

    case TIMER_SETTINGS_ITEM_TRIGGER_EDGE:
      openFieldInput("Timer Settings Menu", "Trigger", 1, triggerEdgeTokens, sizeof(triggerEdgeTokens) / sizeof(triggerEdgeTokens[0]), FIELD_INPUT_TARGET_TRIGGER_EDGE, UI_SCREEN_TIMER_SETTINGS_MENU, String(timerGetTriggerEdgeLabel(settings.triggerEdge)));
      return;

    case TIMER_SETTINGS_ITEM_EXIT:
      openMainMenu();
      return;
    }
  }

  timerSettingsFirstVisibleIndex = timerSettingsIndex - 3;

  if (timerSettingsFirstVisibleIndex < 0)
  {
    timerSettingsFirstVisibleIndex = 0;
  }

  if (timerSettingsFirstVisibleIndex > itemCount - 9)
  {
    timerSettingsFirstVisibleIndex = max(0, itemCount - 9);
  }

  if (redrawRequired)
  {
    drawCurrentScreen();
  }

} //   handleTimerSettingsMenu()

//--- Handle WiFi portal info screen
static void handleWifiPortalScreen(EncoderEvent encoderEvent)
{
  if (encoderEvent != ENCODER_EVENT_SHORT_PRESS)
  {
    return;
  }

  settingsStoreSaveWifiDisabled(true);
  wifiManagerSetDisabled(true);

  currentScreen = UI_SCREEN_STATUS;
  logActiveScreen("Timer Screen");
  drawCurrentScreen();

} //   handleWifiPortalScreen()

//--- Handle system settings menu
static void handleSystemSettingsMenu(EncoderEvent encoderEvent)
{
  bool redrawRequired = false;
  AppSettings settings = timerGetSettings();

  if (encoderEvent == ENCODER_EVENT_LEFT)
  {
    if (systemSettingsIndex > SYSTEM_SETTINGS_ITEM_ENCODER_ORDER)
    {
      systemSettingsIndex--;
    }

    redrawRequired = true;
  }
  else if (encoderEvent == ENCODER_EVENT_RIGHT)
  {
    if (systemSettingsIndex < SYSTEM_SETTINGS_ITEM_EXIT)
    {
      systemSettingsIndex++;
    }

    redrawRequired = true;
  }
  else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    if (systemSettingsIndex == SYSTEM_SETTINGS_ITEM_ENCODER_ORDER)
    {
      bool reversed = !encoderGetDirectionReversed();

      encoderSetDirectionReversed(reversed);
      settingsStoreSaveEncoderDirectionReversed(reversed);
      drawCurrentScreen();

      return;
    }

    if (systemSettingsIndex == SYSTEM_SETTINGS_ITEM_ERASE_WIFI)
    {
      openFieldInput("Erase WiFi", "Are you sure (Y/N)", 1, yesNoTokens, sizeof(yesNoTokens) / sizeof(yesNoTokens[0]), FIELD_INPUT_TARGET_ERASE_WIFI_CONFIRM, UI_SCREEN_SYSTEM_SETTINGS_MENU, "N");

      return;
    }

    if (systemSettingsIndex == SYSTEM_SETTINGS_ITEM_START_WIFI_MANAGER)
    {
      settingsStoreSaveWifiDisabled(false);
      displayDrawMessage("WiFi Manager", "Restarting...");
      delay(250);
      ESP.restart();

      return;
    }

    if (systemSettingsIndex == SYSTEM_SETTINGS_ITEM_RESTART_ULTIMATE_TIMER)
    {
      displayDrawMessage("System", "Restarting...");
      delay(250);
      ESP.restart();

      return;
    }

    if (systemSettingsIndex == SYSTEM_SETTINGS_ITEM_OUTPUT_POLARITY)
    {
      settings.outputPolarityHigh = !settings.outputPolarityHigh;
      settingsStoreSaveOutputPolarityHigh(settings.outputPolarityHigh);
      commitSettings(settings);
      drawCurrentScreen();

      return;
    }

    if (systemSettingsIndex == SYSTEM_SETTINGS_ITEM_EXIT)
    {
      openMainMenu();

      return;
    }
  }

  if (redrawRequired)
  {
    drawCurrentScreen();
  }

} //   handleSystemSettingsMenu()

//--- Handle profile list
static void handleProfileList(EncoderEvent encoderEvent)
{
  int itemCount = static_cast<int>(profileCount);
  bool redrawRequired = false;

  if (encoderEvent == ENCODER_EVENT_LEFT)
  {
    if (profileIndex > 0)
    {
      profileIndex--;
    }

    redrawRequired = true;
  }
  else if (encoderEvent == ENCODER_EVENT_RIGHT)
  {
    if (profileIndex < itemCount - 1)
    {
      profileIndex++;
    }

    redrawRequired = true;
  }
  else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    AppSettings settings = timerGetSettings();
    String selectedProfile = profileNames[profileIndex];

    if (profileIndex == itemCount - 1)
    {
      openMainMenu();

      return;
    }

    if (profileListMode == PROFILE_LIST_LOAD)
    {
      if (profileManagerLoadProfile(selectedProfile, settings))
      {
        commitSettings(settings);
        timerReset();
        settingsStoreSaveLastProfileName(selectedProfile);
        uiMenuShowTransientMessage("Profile loaded");
      }
      else
      {
        uiMenuShowTransientMessage("Load failed");
      }
    }
    else
    {
      pendingDeleteProfileName = selectedProfile;
      openFieldInput("Delete Profile", "Are you sure (Y/N)", 1, yesNoTokens, sizeof(yesNoTokens) / sizeof(yesNoTokens[0]), FIELD_INPUT_TARGET_DELETE_PROFILE_CONFIRM, UI_SCREEN_PROFILE_LIST, "N");

      return;
    }

    refreshProfileListWithExit();
    drawCurrentScreen();

    return;
  }

  profileFirstVisibleIndex = profileIndex - 3;

  if (profileFirstVisibleIndex < 0)
  {
    profileFirstVisibleIndex = 0;
  }

  if (profileFirstVisibleIndex > itemCount - 9)
  {
    profileFirstVisibleIndex = max(0, itemCount - 9);
  }

  if (redrawRequired)
  {
    drawCurrentScreen();
  }

} //   handleProfileList()

//--- Apply current settings back into timer
static void commitSettings(const AppSettings& settings)
{
  timerSetSettings(settings);

  if (settings.autoSaveLastProfile && !settings.profileName.isEmpty())
  {
    profileManagerSaveProfile(settings.profileName, settings);
    settingsStoreSaveLastProfileName(settings.profileName);
  }

} //   commitSettings()

//--- Log active menu or screen
static void logActiveScreen(const char* screenName)
{
  ESP_LOGI("uiMenu", "Active menu: %s", screenName);

} //   logActiveScreen()

//--- Build readable AP SSID lines for portal info screen
static void buildPortalApLines(const String& apSsid, String& line1, String& line2)
{
  const int maxLineLength = 22;

  if (static_cast<int>(apSsid.length()) <= maxLineLength)
  {
    line1 = apSsid;
    line2 = "";

    return;
  }

  int splitAt = -1;

  for (int index = maxLineLength; index >= 8; index--)
  {
    if (apSsid.charAt(index - 1) == '-')
    {
      splitAt = index;
      break;
    }
  }

  if (splitAt < 0)
  {
    splitAt = maxLineLength;
  }

  line1 = apSsid.substring(0, splitAt);
  line2 = apSsid.substring(splitAt);

  if (static_cast<int>(line2.length()) > maxLineLength)
  {
    line2 = line2.substring(0, maxLineLength - 3) + "...";
  }

} //   buildPortalApLines()

//--- Convert encoder event to text
static const char* encoderEventToLabel(EncoderEvent event)
{
  switch (event)
  {
  case ENCODER_EVENT_LEFT:
    return "Rotate Left";

  case ENCODER_EVENT_RIGHT:
    return "Rotate Right";

  case ENCODER_EVENT_SHORT_PRESS:
    return "Short Press";

  case ENCODER_EVENT_MEDIUM_PRESS:
    return "Medium Press";

  case ENCODER_EVENT_LONG_PRESS:
    return "Long Press";

  case ENCODER_EVENT_NONE:
  default:
    return "None";
  }

} //   encoderEventToLabel()

//--- Convert UI screen to text
static const char* uiScreenToLabel(UiScreen screen)
{
  switch (screen)
  {
  case UI_SCREEN_STATUS:
    return "Timer Screen";

  case UI_SCREEN_MAIN_MENU:
    return "Edit Timer Menu";

  case UI_SCREEN_TIMER_SETTINGS_MENU:
    return "Timer Settings Menu";

  case UI_SCREEN_SYSTEM_SETTINGS_MENU:
    return "Show System Settings";

  case UI_SCREEN_PROFILE_LIST:
    return (profileListMode == PROFILE_LIST_LOAD) ? "Load Profile Menu" : "Delete Profile Menu";

  case UI_SCREEN_FIELD_INPUT:
    return fieldInputTitle.c_str();

  case UI_SCREEN_WIFI_MANAGER_PORTAL:
    return "WiFi Manager Started";

  default:
    return "Unknown";
  }

} //   uiScreenToLabel()

//--- Initialize UI menu
void uiMenuInit()
{
  currentScreen = UI_SCREEN_STATUS;
  logActiveScreen("Timer Screen");
  drawCurrentScreen();

  ESP_LOGI("uiMenu", "UI menu initialized");

} //   uiMenuInit()

//--- Update UI menu
void uiMenuUpdate()
{
  EncoderEvent encoderEvent = encoderGetEvent();
  ButtonEvent buttonEvent = buttonGetEvent();
  uint32_t nowMs = millis();
  bool portalActive = wifiManagerShouldOpenPortal();

  if (currentScreen == UI_SCREEN_FIELD_INPUT && buttonEvent != BUTTON_EVENT_NONE)
  {
    if (fieldInputCursorPosition > 0)
    {
      fieldInputCursorPosition--;
      drawCurrentScreen();
    }
  }

  if (portalActive && currentScreen != UI_SCREEN_WIFI_MANAGER_PORTAL)
  {
    screenBeforeWifiPortal = currentScreen;
    currentScreen = UI_SCREEN_WIFI_MANAGER_PORTAL;
    transientMessage = "";
    transientMessageUntilMs = 0;
    drawCurrentScreen();
  }

  if (!portalActive && currentScreen == UI_SCREEN_WIFI_MANAGER_PORTAL)
  {
    currentScreen = screenBeforeWifiPortal;
    drawCurrentScreen();
  }

  if (encoderEvent != ENCODER_EVENT_NONE)
  {
    const char* eventLabel = encoderEventToLabel(encoderEvent);
    const char* screenLabel = uiScreenToLabel(currentScreen);

    (void)eventLabel;
    (void)screenLabel;
    ESP_LOGI("uiMenu", "Input event: %s in %s", eventLabel, screenLabel);
  }

  if (transientMessageUntilMs != 0 && nowMs > transientMessageUntilMs)
  {
    transientMessage = "";
    transientMessageUntilMs = 0;
    drawCurrentScreen();
  }

  switch (currentScreen)
  {
  case UI_SCREEN_STATUS:
    handleStatusScreen(encoderEvent);
    break;

  case UI_SCREEN_MAIN_MENU:
    handleMainMenu(encoderEvent);
    break;

  case UI_SCREEN_TIMER_SETTINGS_MENU:
    handleTimerSettingsMenu(encoderEvent);
    break;

  case UI_SCREEN_SYSTEM_SETTINGS_MENU:
    handleSystemSettingsMenu(encoderEvent);
    break;

  case UI_SCREEN_PROFILE_LIST:
    handleProfileList(encoderEvent);
    break;

  case UI_SCREEN_FIELD_INPUT:
    handleFieldInput(encoderEvent);
    break;

  case UI_SCREEN_WIFI_MANAGER_PORTAL:
    handleWifiPortalScreen(encoderEvent);
    break;
  }

  if (currentScreen == UI_SCREEN_STATUS && transientMessage.isEmpty() && nowMs - lastStatusRefreshMs >= statusRefreshIntervalMs)
  {
    drawCurrentScreen();
    lastStatusRefreshMs = nowMs;
  }

} //   uiMenuUpdate()

//--- Request short status message
void uiMenuShowTransientMessage(const String& message)
{
  transientTitle = "Message";
  transientMessage = message;
  transientMessageUntilMs = millis() + 1800UL;
  displayDrawMessage(transientTitle.c_str(), message.c_str());

} //   uiMenuShowTransientMessage()
