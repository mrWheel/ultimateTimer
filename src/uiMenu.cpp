#include "uiMenu.h"
#include "buttonInput.h"
#include "displayDriver.h"
#include "encoderInput.h"
#include "profileManager.h"
#include "settingsStore.h"
#include "timerEngine.h"
#include "WiFiManagerExt.h"

#include <esp_log.h>

//--- Logging tag
static const char *logTag = "uiMenu";

//--- Main menu item identifiers
enum MainMenuItem
{
  MENU_ITEM_START_STOP = 0,
  MENU_ITEM_RESET = 1,
  MENU_ITEM_ON_TIME = 2,
  MENU_ITEM_ON_UNIT = 3,
  MENU_ITEM_OFF_TIME = 4,
  MENU_ITEM_OFF_UNIT = 5,
  MENU_ITEM_REPEAT_COUNT = 6,
  MENU_ITEM_TRIGGER_MODE = 7,
  MENU_ITEM_TRIGGER_EDGE = 8,
  MENU_ITEM_OUTPUT_POLARITY = 9,
  MENU_ITEM_LOCK_INPUT = 10,
  MENU_ITEM_AUTO_SAVE = 11,
  MENU_ITEM_SAVE_PROFILE = 12,
  MENU_ITEM_LOAD_PROFILE = 13,
  MENU_ITEM_DELETE_PROFILE = 14,
  MENU_ITEM_WIFI_SETUP = 15,
  MENU_ITEM_WIFI_INFO = 16
};

//--- Numeric editor targets
enum NumberEditTarget
{
  NUMBER_EDIT_NONE = 0,
  NUMBER_EDIT_ON_TIME = 1,
  NUMBER_EDIT_OFF_TIME = 2,
  NUMBER_EDIT_REPEAT_COUNT = 3
};

//--- Enum editor targets
enum EnumEditTarget
{
  ENUM_EDIT_NONE = 0,
  ENUM_EDIT_ON_UNIT = 1,
  ENUM_EDIT_OFF_UNIT = 2,
  ENUM_EDIT_TRIGGER_MODE = 3,
  ENUM_EDIT_TRIGGER_EDGE = 4,
  ENUM_EDIT_OUTPUT_POLARITY = 5,
  ENUM_EDIT_LOCK_INPUT = 6,
  ENUM_EDIT_AUTO_SAVE = 7
};

//--- Profile list modes
enum ProfileListMode
{
  PROFILE_LIST_LOAD = 0,
  PROFILE_LIST_DELETE = 1,
  PROFILE_LIST_WIFI_AP = 2
};

//--- Text input modes
enum TextInputMode
{
  TEXT_INPUT_MODE_PROFILE_NAME = 0,
  TEXT_INPUT_MODE_WIFI_PASSWORD = 1
};

//--- Main menu items
static const String mainMenuItems[] =
{
  "Start/Stop",
  "Reset",
  "On Time",
  "On Unit",
  "Off Time",
  "Off Unit",
  "Repeat Count",
  "Trigger Mode",
  "Trigger Edge",
  "Output Polarity",
  "Lock Input Run",
  "Auto Save Last",
  "Save Profile",
  "Load Profile",
  "Delete Profile",
  "WiFi Setup",
  "WiFi Info"
};

//--- Text input tokens
static const String textTokens[] =
{
  "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
  "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
  "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
  "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
  "_", "-", ".", " ", "!", "@", "#", "$", "%", "&", "*", "(", ")", "/", "?", "+", "=", ":", "BACK"
};

//--- Maximum number of WiFi APs shown in local UI
static const size_t wifiApListMaxCount = 20;

//--- Current screen state
static UiScreen currentScreen = UI_SCREEN_STATUS;

//--- Current indexes and scroll positions
static int menuIndex = 0;
static int mainMenuFirstVisibleIndex = 0;
static int profileIndex = 0;
static int profileFirstVisibleIndex = 0;
static size_t profileCount = 0;
static String profileNames[profileManagerMaxProfiles];

//--- Current editors
static NumberEditTarget numberEditTarget = NUMBER_EDIT_NONE;
static EnumEditTarget enumEditTarget = ENUM_EDIT_NONE;
static ProfileListMode profileListMode = PROFILE_LIST_LOAD;
static String textInputValue = "";
static int textTokenIndex = 0;
static TextInputMode textInputMode = TEXT_INPUT_MODE_PROFILE_NAME;
static String textInputTitle = "Save Profile";

//--- WiFi AP list state
static String wifiApNames[wifiApListMaxCount];
static size_t wifiApCount = 0;
static int wifiApIndex = 0;
static int wifiApFirstVisibleIndex = 0;
static WifiSettings wifiEditSettings;

//--- Transient message
static String transientMessage = "";
static uint32_t transientMessageUntilMs = 0;

//--- Refresh profile list
static void refreshProfileList();

//--- Refresh scanned WiFi AP list
static void refreshWifiApList();

//--- Draw current screen
static void drawCurrentScreen();

//--- Handle status screen event
static void handleStatusScreen(EncoderEvent encoderEvent, ButtonEvent buttonEvent);

//--- Handle main menu event
static void handleMainMenu(EncoderEvent encoderEvent, ButtonEvent buttonEvent);

//--- Handle number editor event
static void handleNumberEditor(EncoderEvent encoderEvent, ButtonEvent buttonEvent);

//--- Handle enum editor event
static void handleEnumEditor(EncoderEvent encoderEvent, ButtonEvent buttonEvent);

//--- Handle text input event
static void handleTextInput(EncoderEvent encoderEvent, ButtonEvent buttonEvent);

//--- Handle profile list event
static void handleProfileList(EncoderEvent encoderEvent, ButtonEvent buttonEvent);

//--- Open main menu
static void openMainMenu();

//--- Return to status screen
static void openStatusScreen();

//--- Apply current settings back into timer
static void commitSettings(const AppSettings &settings);

//--- Initialize UI menu
void uiMenuInit()
{
  refreshProfileList();
  currentScreen = UI_SCREEN_STATUS;
  drawCurrentScreen();

  ESP_LOGI(logTag, "UI menu initialized");

}   //   uiMenuInit()

//--- Update UI menu
void uiMenuUpdate()
{
  EncoderEvent encoderEvent = encoderGetEvent();
  ButtonEvent buttonEvent = buttonGetEvent();

  if (transientMessageUntilMs != 0 && millis() > transientMessageUntilMs)
  {
    transientMessage = "";
    transientMessageUntilMs = 0;
    drawCurrentScreen();
  }

  switch (currentScreen)
  {
    case UI_SCREEN_STATUS:
      handleStatusScreen(encoderEvent, buttonEvent);
      break;

    case UI_SCREEN_MAIN_MENU:
      handleMainMenu(encoderEvent, buttonEvent);
      break;

    case UI_SCREEN_EDIT_NUMBER:
      handleNumberEditor(encoderEvent, buttonEvent);
      break;

    case UI_SCREEN_EDIT_ENUM:
      handleEnumEditor(encoderEvent, buttonEvent);
      break;

    case UI_SCREEN_TEXT_INPUT:
      handleTextInput(encoderEvent, buttonEvent);
      break;

    case UI_SCREEN_PROFILE_LIST:
      handleProfileList(encoderEvent, buttonEvent);
      break;
  }

}   //   uiMenuUpdate()

//--- Request short status message
void uiMenuShowTransientMessage(const String &message)
{
  transientMessage = message;
  transientMessageUntilMs = millis() + 1800UL;
  displayDrawMessage("Message", message.c_str());

}   //   uiMenuShowTransientMessage()

//--- Refresh profile list
static void refreshProfileList()
{
  profileCount = profileManagerListProfiles(profileNames, profileManagerMaxProfiles);

  if (profileIndex >= static_cast<int>(profileCount))
  {
    profileIndex = max(0, static_cast<int>(profileCount) - 1);
  }

}   //   refreshProfileList()

//--- Draw current screen
static void drawCurrentScreen()
{
  if (!transientMessage.isEmpty())
  {
    displayDrawMessage("Message", transientMessage.c_str());

    return;
  }

  const AppSettings &settings = timerGetSettings();
  RuntimeStatus runtimeStatus = timerGetRuntimeStatus();

  switch (currentScreen)
  {
    case UI_SCREEN_STATUS:
      displayDrawStatusScreen(settings, runtimeStatus, wifiManagerIsStaConnected());
      break;

    case UI_SCREEN_MAIN_MENU:
      displayDrawListScreen("Main Menu", mainMenuItems, sizeof(mainMenuItems) / sizeof(mainMenuItems[0]), menuIndex, mainMenuFirstVisibleIndex);
      break;

    case UI_SCREEN_EDIT_NUMBER:
      if (numberEditTarget == NUMBER_EDIT_ON_TIME)
      {
        displayDrawNumberEditor("On Time", settings.onTimeValue, timerGetTimeUnitLabel(settings.onTimeUnit));
      }
      else if (numberEditTarget == NUMBER_EDIT_OFF_TIME)
      {
        displayDrawNumberEditor("Off Time", settings.offTimeValue, timerGetTimeUnitLabel(settings.offTimeUnit));
      }
      else if (numberEditTarget == NUMBER_EDIT_REPEAT_COUNT)
      {
        displayDrawNumberEditor("Repeat Count", settings.repeatCount, "cycles");
      }
      break;

    case UI_SCREEN_EDIT_ENUM:
      if (enumEditTarget == ENUM_EDIT_ON_UNIT)
      {
        displayDrawEnumEditor("On Unit", timerGetTimeUnitLabel(settings.onTimeUnit));
      }
      else if (enumEditTarget == ENUM_EDIT_OFF_UNIT)
      {
        displayDrawEnumEditor("Off Unit", timerGetTimeUnitLabel(settings.offTimeUnit));
      }
      else if (enumEditTarget == ENUM_EDIT_TRIGGER_MODE)
      {
        displayDrawEnumEditor("Trigger Mode", timerGetTriggerModeLabel(settings.triggerMode));
      }
      else if (enumEditTarget == ENUM_EDIT_TRIGGER_EDGE)
      {
        displayDrawEnumEditor("Trigger Edge", timerGetTriggerEdgeLabel(settings.triggerEdge));
      }
      else if (enumEditTarget == ENUM_EDIT_OUTPUT_POLARITY)
      {
        displayDrawEnumEditor("Output Polarity", settings.outputPolarityHigh ? "Active High" : "Active Low");
      }
      else if (enumEditTarget == ENUM_EDIT_LOCK_INPUT)
      {
        displayDrawEnumEditor("Lock Input Run", settings.lockInputDuringRun ? "Yes" : "No");
      }
      else if (enumEditTarget == ENUM_EDIT_AUTO_SAVE)
      {
        displayDrawEnumEditor("Auto Save Last", settings.autoSaveLastProfile ? "Yes" : "No");
      }
      break;

    case UI_SCREEN_TEXT_INPUT:
      displayDrawTextInput(textInputTitle.c_str(), textInputValue, textTokens[textTokenIndex]);
      break;

    case UI_SCREEN_PROFILE_LIST:
      if (profileListMode == PROFILE_LIST_LOAD)
      {
        displayDrawListScreen("Load Profile", profileNames, profileCount, profileIndex, profileFirstVisibleIndex);
      }
      else if (profileListMode == PROFILE_LIST_DELETE)
      {
        displayDrawListScreen("Delete Profile", profileNames, profileCount, profileIndex, profileFirstVisibleIndex);
      }
      else
      {
        displayDrawListScreen("Select WiFi AP", wifiApNames, wifiApCount, wifiApIndex, wifiApFirstVisibleIndex);
      }
      break;
  }

}   //   drawCurrentScreen()

//--- Handle status screen event
static void handleStatusScreen(EncoderEvent encoderEvent, ButtonEvent buttonEvent)
{
  if (encoderEvent == ENCODER_EVENT_SHORT_PRESS || buttonEvent == BUTTON_EVENT_SHORT_PRESS)
  {
    openMainMenu();

    return;
  }

  if (encoderEvent == ENCODER_EVENT_LONG_PRESS)
  {
    if (timerGetRuntimeStatus().state == TIMER_STATE_RUNNING)
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

}   //   handleStatusScreen()

//--- Handle main menu event
static void handleMainMenu(EncoderEvent encoderEvent, ButtonEvent buttonEvent)
{
  const int itemCount = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);

  if (encoderEvent == ENCODER_EVENT_LEFT)
  {
    menuIndex--;

    if (menuIndex < 0)
    {
      menuIndex = itemCount - 1;
    }
  }
  else if (encoderEvent == ENCODER_EVENT_RIGHT)
  {
    menuIndex++;

    if (menuIndex >= itemCount)
    {
      menuIndex = 0;
    }
  }
  else if (buttonEvent == BUTTON_EVENT_SHORT_PRESS)
  {
    openStatusScreen();

    return;
  }
  else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    AppSettings settings = timerGetSettings();

    switch (menuIndex)
    {
      case MENU_ITEM_START_STOP:
        if (timerGetRuntimeStatus().state == TIMER_STATE_RUNNING)
        {
          timerStop();
        }
        else
        {
          timerStart();
        }
        openStatusScreen();
        return;

      case MENU_ITEM_RESET:
        timerReset();
        openStatusScreen();
        return;

      case MENU_ITEM_ON_TIME:
        numberEditTarget = NUMBER_EDIT_ON_TIME;
        currentScreen = UI_SCREEN_EDIT_NUMBER;
        break;

      case MENU_ITEM_ON_UNIT:
        enumEditTarget = ENUM_EDIT_ON_UNIT;
        currentScreen = UI_SCREEN_EDIT_ENUM;
        break;

      case MENU_ITEM_OFF_TIME:
        numberEditTarget = NUMBER_EDIT_OFF_TIME;
        currentScreen = UI_SCREEN_EDIT_NUMBER;
        break;

      case MENU_ITEM_OFF_UNIT:
        enumEditTarget = ENUM_EDIT_OFF_UNIT;
        currentScreen = UI_SCREEN_EDIT_ENUM;
        break;

      case MENU_ITEM_REPEAT_COUNT:
        numberEditTarget = NUMBER_EDIT_REPEAT_COUNT;
        currentScreen = UI_SCREEN_EDIT_NUMBER;
        break;

      case MENU_ITEM_TRIGGER_MODE:
        enumEditTarget = ENUM_EDIT_TRIGGER_MODE;
        currentScreen = UI_SCREEN_EDIT_ENUM;
        break;

      case MENU_ITEM_TRIGGER_EDGE:
        enumEditTarget = ENUM_EDIT_TRIGGER_EDGE;
        currentScreen = UI_SCREEN_EDIT_ENUM;
        break;

      case MENU_ITEM_OUTPUT_POLARITY:
        enumEditTarget = ENUM_EDIT_OUTPUT_POLARITY;
        currentScreen = UI_SCREEN_EDIT_ENUM;
        break;

      case MENU_ITEM_LOCK_INPUT:
        enumEditTarget = ENUM_EDIT_LOCK_INPUT;
        currentScreen = UI_SCREEN_EDIT_ENUM;
        break;

      case MENU_ITEM_AUTO_SAVE:
        enumEditTarget = ENUM_EDIT_AUTO_SAVE;
        currentScreen = UI_SCREEN_EDIT_ENUM;
        break;

      case MENU_ITEM_SAVE_PROFILE:
        textInputValue = settings.profileName;
        textTokenIndex = 0;
        textInputMode = TEXT_INPUT_MODE_PROFILE_NAME;
        textInputTitle = "Save Profile";
        currentScreen = UI_SCREEN_TEXT_INPUT;
        break;

      case MENU_ITEM_LOAD_PROFILE:
        refreshProfileList();
        profileListMode = PROFILE_LIST_LOAD;
        currentScreen = UI_SCREEN_PROFILE_LIST;
        break;

      case MENU_ITEM_DELETE_PROFILE:
        refreshProfileList();
        profileListMode = PROFILE_LIST_DELETE;
        currentScreen = UI_SCREEN_PROFILE_LIST;
        break;

      case MENU_ITEM_WIFI_SETUP:
        refreshWifiApList();

        if (wifiApCount == 0)
        {
          uiMenuShowTransientMessage("No WiFi AP found");

          return;
        }

        wifiApIndex = 0;
        wifiApFirstVisibleIndex = 0;
        profileListMode = PROFILE_LIST_WIFI_AP;
        currentScreen = UI_SCREEN_PROFILE_LIST;
        break;

      case MENU_ITEM_WIFI_INFO:
        uiMenuShowTransientMessage(wifiManagerGetAddressString());
        return;
    }
  }

  mainMenuFirstVisibleIndex = menuIndex - 3;

  if (mainMenuFirstVisibleIndex < 0)
  {
    mainMenuFirstVisibleIndex = 0;
  }

  if (mainMenuFirstVisibleIndex > itemCount - 9)
  {
    mainMenuFirstVisibleIndex = max(0, itemCount - 9);
  }

  drawCurrentScreen();

}   //   handleMainMenu()

//--- Handle number editor event
static void handleNumberEditor(EncoderEvent encoderEvent, ButtonEvent buttonEvent)
{
  AppSettings settings = timerGetSettings();
  uint32_t *valuePtr = nullptr;

  if (numberEditTarget == NUMBER_EDIT_ON_TIME)
  {
    valuePtr = &settings.onTimeValue;
  }
  else if (numberEditTarget == NUMBER_EDIT_OFF_TIME)
  {
    valuePtr = &settings.offTimeValue;
  }
  else if (numberEditTarget == NUMBER_EDIT_REPEAT_COUNT)
  {
    valuePtr = &settings.repeatCount;
  }

  if (valuePtr == nullptr)
  {
    openMainMenu();

    return;
  }

  if (encoderEvent == ENCODER_EVENT_LEFT)
  {
    if (*valuePtr > 0)
    {
      (*valuePtr)--;
    }
  }
  else if (encoderEvent == ENCODER_EVENT_RIGHT)
  {
    (*valuePtr)++;
  }
  else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    commitSettings(settings);
    openMainMenu();

    return;
  }
  else if (buttonEvent == BUTTON_EVENT_SHORT_PRESS)
  {
    openMainMenu();

    return;
  }

  timerSetSettings(settings);
  drawCurrentScreen();

}   //   handleNumberEditor()

//--- Handle enum editor event
static void handleEnumEditor(EncoderEvent encoderEvent, ButtonEvent buttonEvent)
{
  AppSettings settings = timerGetSettings();

  if (encoderEvent == ENCODER_EVENT_LEFT || encoderEvent == ENCODER_EVENT_RIGHT)
  {
    bool stepForward = encoderEvent == ENCODER_EVENT_RIGHT;

    switch (enumEditTarget)
    {
      case ENUM_EDIT_ON_UNIT:
        settings.onTimeUnit = static_cast<TimeUnit>((static_cast<int>(settings.onTimeUnit) + (stepForward ? 1 : 2)) % 3);
        break;

      case ENUM_EDIT_OFF_UNIT:
        settings.offTimeUnit = static_cast<TimeUnit>((static_cast<int>(settings.offTimeUnit) + (stepForward ? 1 : 2)) % 3);
        break;

      case ENUM_EDIT_TRIGGER_MODE:
        settings.triggerMode = static_cast<TriggerMode>((static_cast<int>(settings.triggerMode) + 1) % 2);
        break;

      case ENUM_EDIT_TRIGGER_EDGE:
        settings.triggerEdge = static_cast<TriggerEdge>((static_cast<int>(settings.triggerEdge) + 1) % 2);
        break;

      case ENUM_EDIT_OUTPUT_POLARITY:
        settings.outputPolarityHigh = !settings.outputPolarityHigh;
        break;

      case ENUM_EDIT_LOCK_INPUT:
        settings.lockInputDuringRun = !settings.lockInputDuringRun;
        break;

      case ENUM_EDIT_AUTO_SAVE:
        settings.autoSaveLastProfile = !settings.autoSaveLastProfile;
        break;

      default:
        break;
    }

    timerSetSettings(settings);
    drawCurrentScreen();

    return;
  }

  if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    commitSettings(settings);
    openMainMenu();

    return;
  }

  if (buttonEvent == BUTTON_EVENT_SHORT_PRESS)
  {
    openMainMenu();

    return;
  }

}   //   handleEnumEditor()

//--- Handle text input event
static void handleTextInput(EncoderEvent encoderEvent, ButtonEvent buttonEvent)
{
  const int tokenCount = sizeof(textTokens) / sizeof(textTokens[0]);

  if (encoderEvent == ENCODER_EVENT_LEFT)
  {
    textTokenIndex--;

    if (textTokenIndex < 0)
    {
      textTokenIndex = tokenCount - 1;
    }
  }
  else if (encoderEvent == ENCODER_EVENT_RIGHT)
  {
    textTokenIndex++;

    if (textTokenIndex >= tokenCount)
    {
      textTokenIndex = 0;
    }
  }
  else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    if (textTokens[textTokenIndex] == "BACK")
    {
      if (!textInputValue.isEmpty())
      {
        textInputValue.remove(textInputValue.length() - 1);
      }
    }
    else
    {
      textInputValue += textTokens[textTokenIndex];
    }
  }
  else if (encoderEvent == ENCODER_EVENT_LONG_PRESS)
  {
    if (textInputMode == TEXT_INPUT_MODE_PROFILE_NAME)
    {
      AppSettings settings = timerGetSettings();
      textInputValue.trim();

      if (textInputValue.isEmpty())
      {
        uiMenuShowTransientMessage("Empty profile name");

        return;
      }

      settings.profileName = textInputValue;

      if (profileManagerSaveProfile(settings.profileName, settings))
      {
        commitSettings(settings);
        settingsStoreSaveLastProfileName(settings.profileName);
        refreshProfileList();
        uiMenuShowTransientMessage("Profile saved");
      }
      else
      {
        uiMenuShowTransientMessage("Save failed");
      }
    }
    else
    {
      wifiEditSettings.staPassword = textInputValue;
      wifiManagerSaveAndApplySettings(wifiEditSettings);
      uiMenuShowTransientMessage("WiFi saved");
    }

    currentScreen = UI_SCREEN_MAIN_MENU;

    return;
  }
  else if (buttonEvent == BUTTON_EVENT_SHORT_PRESS)
  {
    openMainMenu();

    return;
  }

  drawCurrentScreen();

}   //   handleTextInput()

//--- Handle profile list event
static void handleProfileList(EncoderEvent encoderEvent, ButtonEvent buttonEvent)
{
  if (profileListMode == PROFILE_LIST_WIFI_AP)
  {
    if (wifiApCount == 0)
    {
      if (buttonEvent == BUTTON_EVENT_SHORT_PRESS || encoderEvent == ENCODER_EVENT_SHORT_PRESS)
      {
        openMainMenu();

        return;
      }

      displayDrawMessage("WiFi Setup", "No AP found");

      return;
    }

    if (encoderEvent == ENCODER_EVENT_LEFT)
    {
      wifiApIndex--;

      if (wifiApIndex < 0)
      {
        wifiApIndex = wifiApCount - 1;
      }
    }
    else if (encoderEvent == ENCODER_EVENT_RIGHT)
    {
      wifiApIndex++;

      if (wifiApIndex >= static_cast<int>(wifiApCount))
      {
        wifiApIndex = 0;
      }
    }
    else if (buttonEvent == BUTTON_EVENT_SHORT_PRESS)
    {
      openMainMenu();

      return;
    }
    else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
    {
      wifiEditSettings = wifiManagerGetSettings();
      wifiEditSettings.staSsid = wifiApNames[wifiApIndex];
      textInputValue = wifiEditSettings.staPassword;
      textTokenIndex = 0;
      textInputMode = TEXT_INPUT_MODE_WIFI_PASSWORD;
      textInputTitle = "WiFi Password";
      currentScreen = UI_SCREEN_TEXT_INPUT;
      drawCurrentScreen();

      return;
    }

    wifiApFirstVisibleIndex = wifiApIndex - 3;

    if (wifiApFirstVisibleIndex < 0)
    {
      wifiApFirstVisibleIndex = 0;
    }

    if (wifiApFirstVisibleIndex > static_cast<int>(wifiApCount) - 9)
    {
      wifiApFirstVisibleIndex = max(0, static_cast<int>(wifiApCount) - 9);
    }

    drawCurrentScreen();

    return;
  }

  if (profileCount == 0)
  {
    if (buttonEvent == BUTTON_EVENT_SHORT_PRESS || encoderEvent == ENCODER_EVENT_SHORT_PRESS)
    {
      openMainMenu();

      return;
    }

    displayDrawMessage("Profiles", "No profiles available");

    return;
  }

  if (encoderEvent == ENCODER_EVENT_LEFT)
  {
    profileIndex--;

    if (profileIndex < 0)
    {
      profileIndex = profileCount - 1;
    }
  }
  else if (encoderEvent == ENCODER_EVENT_RIGHT)
  {
    profileIndex++;

    if (profileIndex >= static_cast<int>(profileCount))
    {
      profileIndex = 0;
    }
  }
  else if (buttonEvent == BUTTON_EVENT_SHORT_PRESS)
  {
    openMainMenu();

    return;
  }
  else if (encoderEvent == ENCODER_EVENT_SHORT_PRESS)
  {
    AppSettings settings = timerGetSettings();
    String selectedProfile = profileNames[profileIndex];

    if (profileListMode == PROFILE_LIST_LOAD)
    {
      if (profileManagerLoadProfile(selectedProfile, settings))
      {
        commitSettings(settings);
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
      if (profileManagerDeleteProfile(selectedProfile))
      {
        refreshProfileList();
        uiMenuShowTransientMessage("Profile deleted");
      }
      else
      {
        uiMenuShowTransientMessage("Delete failed");
      }
    }

    currentScreen = UI_SCREEN_MAIN_MENU;

    return;
  }

  profileFirstVisibleIndex = profileIndex - 3;

  if (profileFirstVisibleIndex < 0)
  {
    profileFirstVisibleIndex = 0;
  }

  if (profileFirstVisibleIndex > static_cast<int>(profileCount) - 9)
  {
    profileFirstVisibleIndex = max(0, static_cast<int>(profileCount) - 9);
  }

  drawCurrentScreen();

}   //   handleProfileList()

//--- Open main menu
static void openMainMenu()
{
  currentScreen = UI_SCREEN_MAIN_MENU;
  drawCurrentScreen();

}   //   openMainMenu()

//--- Return to status screen
static void openStatusScreen()
{
  currentScreen = UI_SCREEN_STATUS;
  drawCurrentScreen();

}   //   openStatusScreen()

//--- Apply current settings back into timer
static void commitSettings(const AppSettings &settings)
{
  timerSetSettings(settings);

  if (settings.autoSaveLastProfile && !settings.profileName.isEmpty())
  {
    profileManagerSaveProfile(settings.profileName, settings);
    settingsStoreSaveLastProfileName(settings.profileName);
  }

}   //   commitSettings()

//--- Refresh scanned WiFi AP list
static void refreshWifiApList()
{
  wifiApCount = wifiManagerScanNetworks(wifiApNames, wifiApListMaxCount);

  if (wifiApIndex >= static_cast<int>(wifiApCount))
  {
    wifiApIndex = max(0, static_cast<int>(wifiApCount) - 1);
  }

}   //   refreshWifiApList()
