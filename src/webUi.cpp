/*** Last Changed: 2026-05-13 - 12:05 ***/
#include "webUi.h"
#include "profileManager.h"
#include "settingsStore.h"
#include "timerEngine.h"
#include "WiFiManagerExt.h"
#include "colorSettings.h"
#include "displayDriver.h"
#include "uiMenu.h"
#include "appConfig.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_log.h>

//--- Logging tag
static const char* logTag = "webUi";

//--- Web server instance
static WebServer server(80);

//--- HTML content
static const char* indexHtml = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Universal Timer</title>
<style>
:root {
  --bgTop: #f7efe5;
  --bgBottom: #dbe7f5;
  --panel: rgba(255, 255, 255, 0.88);
  --panelStrong: #ffffff;
  --line: rgba(28, 45, 66, 0.2);
  --ink: #1d2733;
  --inkMuted: #5b6877;
  --themeAccent: #2b6ea3;
  --themeAccentStrong: #245d89;
  --themeTint: #e8f2fb;
  --themeGlow: rgba(43, 110, 163, 0.25);
  --shadow: 0 12px 35px rgba(15, 31, 49, 0.12);
}

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  min-height: 100vh;
  color: var(--ink);
  font-family: "Avenir Next", "Futura", "Gill Sans", "Trebuchet MS", sans-serif;
  background:
    radial-gradient(1200px 700px at 10% -10%, rgba(249, 190, 124, 0.28), transparent 65%),
    radial-gradient(900px 600px at 90% 0%, rgba(125, 170, 221, 0.32), transparent 70%),
    linear-gradient(170deg, var(--bgTop), var(--bgBottom));
}

.menuBar {
  position: sticky;
  top: 0;
  z-index: 10;
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 14px;
  padding: 10px 16px;
  border-bottom: 1px solid var(--line);
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.88), rgba(255, 255, 255, 0.72));
  backdrop-filter: blur(12px);
}

.menuLeft {
  display: flex;
  align-items: center;
  gap: 4px;
  flex-wrap: wrap;
}

.menuGroup {
  position: relative;
}

.subMenu {
  position: absolute;
  left: 0;
  top: calc(100% + 6px);
  min-width: 170px;
  border: 1px solid var(--line);
  border-radius: 10px;
  background: rgba(255, 255, 255, 0.96);
  box-shadow: var(--shadow);
  padding: 6px;
  display: none;
  z-index: 30;
}

.subMenu.open {
  display: block;
}

.menuSubButton {
  width: 100%;
  text-align: left;
}

.menuButton {
  border: none;
  background: transparent;
  color: var(--ink);
  font-size: 14px;
  font-weight: 600;
  letter-spacing: 0.02em;
  border-radius: 6px;
  padding: 6px 10px;
  cursor: pointer;
  transition: background-color 0.16s ease, color 0.16s ease;
}

.menuButton:hover {
  background: rgba(20, 35, 54, 0.09);
}

.menuButton:disabled {
  opacity: 0.5;
  cursor: not-allowed;
  background: transparent;
}

.menuButton:disabled:hover {
  background: transparent;
}

.menuButton.active {
  color: #ffffff;
  background: var(--themeAccentStrong);
}

.menuRight {
  display: flex;
  align-items: center;
  gap: 12px;
  color: var(--inkMuted);
  font-size: 13px;
}

.headerValue {
  color: var(--ink);
  font-weight: 700;
  letter-spacing: 0.04em;
}

.clockValue {
  min-width: 74px;
  text-align: right;
  font-variant-numeric: tabular-nums;
}

.appShell {
  max-width: 1100px;
  margin: 0 auto;
  padding: 16px 14px 26px;
}

.tile {
  border-radius: 14px;
  border: 1px solid var(--line);
  background: var(--panel);
  box-shadow: var(--shadow);
  margin-bottom: 14px;
  overflow: hidden;
  animation: tileEnter 260ms ease;
}

.tileHeader {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 10px;
  border-bottom: 1px solid var(--line);
  padding: 12px 14px;
  background: linear-gradient(180deg, rgba(248, 250, 253, 0.96), rgba(241, 246, 251, 0.9));
}

.tileTitle {
  font-size: 16px;
  font-weight: 700;
  letter-spacing: 0.02em;
}

.tileBody {
  padding: 14px;
}

.statusGrid {
  display: grid;
  grid-template-columns: repeat(4, minmax(130px, 1fr));
  gap: 10px;
}

.statusItem {
  border: 1px solid var(--line);
  border-radius: 10px;
  padding: 10px;
  background: var(--panelStrong);
}

.statusLabel {
  font-size: 12px;
  color: var(--inkMuted);
  text-transform: uppercase;
  letter-spacing: 0.08em;
}

.statusValue {
  margin-top: 6px;
  font-size: 20px;
  font-weight: 700;
  letter-spacing: 0.03em;
}

.statusWide {
  grid-column: span 2;
}

.statusThreeWide {
  grid-column: span 3;
}

.statusOutputValue {
  font-size: 14px;
  line-height: 1.25;
}

.actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 12px;
}

.actionButton {
  border-radius: 9px;
  border: 1px solid transparent;
  padding: 9px 16px;
  font-weight: 700;
  cursor: pointer;
  transition: transform 0.12s ease, box-shadow 0.12s ease;
}

.actionButton:hover {
  transform: translateY(-1px);
  box-shadow: 0 6px 14px rgba(0, 0, 0, 0.12);
}

.actionStart {
  background: var(--themeTint);
  border-color: var(--themeAccent);
  color: var(--themeAccentStrong);
}

.actionStop {
  background: var(--themeTint);
  border-color: var(--themeAccent);
  color: var(--themeAccentStrong);
}

.actionReset {
  background: var(--themeTint);
  border-color: var(--themeAccent);
  color: var(--themeAccentStrong);
}

.menuTile {
  display: none;
}

.menuTile.active {
  display: block;
}

.cardDisabled {
  background: rgba(255, 255, 255, 0.68);
}

.cardDisabled input,
.cardDisabled select,
.cardDisabled button {
  opacity: 0.56;
}



.formGrid {
  display: grid;
  grid-template-columns: repeat(2, minmax(200px, 1fr));
  gap: 10px 16px;
}

.fieldRow {
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.fieldRow label {
  font-size: 12px;
  color: var(--inkMuted);
  letter-spacing: 0.06em;
  text-transform: uppercase;
}

.roField label::after {
  content: "  RO";
  font-size: 10px;
  color: var(--themeAccentStrong);
  letter-spacing: 0.08em;
}

.fieldRow input,
.fieldRow select {
  width: 100%;
  border: 1px solid #bac8d7;
  border-radius: 8px;
  background: #f9fbfd;
  padding: 9px 10px;
  color: var(--ink);
  font-size: 14px;
}

.fieldRow input[readonly] {
  border-color: #8ea3bb;
  border-style: dashed;
  background: repeating-linear-gradient(
    135deg,
    #edf3fa,
    #edf3fa 8px,
    #e2ebf6 8px,
    #e2ebf6 16px
  );
  color: #223246;
  font-weight: 600;
}

.footerActions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 12px;
}

.primaryButton,
.secondaryButton,
.dangerButton {
  border-radius: 8px;
  border: 1px solid transparent;
  padding: 8px 14px;
  font-weight: 700;
  cursor: pointer;
}

.primaryButton {
  background: var(--themeTint);
  color: var(--themeAccentStrong);
  border-color: var(--themeAccent);
}

.secondaryButton {
  background: var(--themeTint);
  color: var(--themeAccentStrong);
  border-color: var(--themeAccent);
}

.dangerButton {
  background: var(--themeTint);
  color: var(--themeAccentStrong);
  border-color: var(--themeAccent);
}

.metaCard {
  border: 1px solid var(--line);
  border-radius: 10px;
  background: var(--panelStrong);
  padding: 10px;
}

.metaCardLabel {
  font-size: 12px;
  color: var(--inkMuted);
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.metaCardValue {
  margin-top: 6px;
  font-size: 18px;
  font-weight: 700;
}

.activeProfileBanner {
  border-left: 4px solid var(--themeAccent);
  border-radius: 10px;
  padding: 10px;
  background: var(--themeTint);
  margin-bottom: 10px;
}

.activeProfileTitle {
  font-size: 12px;
  color: var(--inkMuted);
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.activeProfileName {
  margin-top: 4px;
  font-size: 20px;
  font-weight: 700;
}

.selectHint {
  margin-top: 4px;
  font-size: 12px;
  color: var(--inkMuted);
}

.timer24hEditorWrap {
  margin-top: 8px;
  border: 1px solid var(--line);
  border-radius: 10px;
  background: var(--panelStrong);
  overflow: auto;
  max-height: 380px;
}

.timer24hEditorTable {
  width: 100%;
  border-collapse: collapse;
  min-width: 620px;
}

.timer24hEditorTable th,
.timer24hEditorTable td {
  border-bottom: 1px solid var(--line);
  padding: 6px 8px;
  text-align: left;
}

.timer24hEditorTable th {
  position: sticky;
  top: 0;
  background: #f4f8fc;
  z-index: 1;
  font-size: 12px;
  color: var(--inkMuted);
  letter-spacing: 0.05em;
  text-transform: uppercase;
}

.timer24hEditorHour {
  font-variant-numeric: tabular-nums;
  font-weight: 700;
}

.timer24hQuarterSelect {
  width: 100%;
  border: 1px solid #bac8d7;
  border-radius: 6px;
  background: #f9fbfd;
  padding: 6px 8px;
  color: var(--ink);
  font-size: 14px;
}

.menuDivider {
  width: 1px;
  height: 18px;
  background: var(--line);
  margin: 0 4px;
}

.primaryButton:hover,
.secondaryButton:hover,
.dangerButton:hover,
.actionButton:hover {
  box-shadow: 0 0 0 3px var(--themeGlow);
}

.primaryButton,
.secondaryButton,
.dangerButton,
.actionButton,
.menuButton {
  transition: background-color 0.16s ease, color 0.16s ease, border-color 0.16s ease, box-shadow 0.16s ease, transform 0.12s ease;
}

.primaryButton:active,
.secondaryButton:active,
.dangerButton:active,
.actionButton:active {
  transform: translateY(1px);
}

.menuButton.themePulse {
  box-shadow: 0 0 0 3px var(--themeGlow);
}

.primaryButton {
  color: #ffffff;
  color: var(--themeAccentStrong);
}

.metaLine {
  margin-top: 10px;
  color: var(--inkMuted);
  font-size: 13px;
}

.profileNameBold {
  font-weight: 700;
  color: var(--ink);
}

.tileNotice {
  display: none;
  margin: 0 0 12px 0;
  padding: 8px 10px;
  border-left: 3px solid var(--themeAccent);
  background: var(--themeTint);
  color: var(--ink);
  font-size: 14px;
  font-weight: 700;
  border-radius: 8px;
}

.cardDisabled .tileNotice {
  display: block;
  opacity: 1;
  color: var(--ink);
}

.saveNotice {
  position: fixed;
  z-index: 40;
  max-width: min(92vw, 460px);
  border: 1px solid var(--themeAccent);
  border-radius: 10px;
  padding: 10px 12px;
  font-size: 13px;
  font-weight: 700;
  background: var(--themeTint);
  color: var(--themeAccentStrong);
  box-shadow: var(--shadow);
  opacity: 0;
  pointer-events: none;
  transition: opacity 0.2s ease, transform 0.2s ease;
}

.saveNoticeContent {
  display: flex;
  align-items: center;
  gap: 10px;
}

.saveNoticeMessage {
  flex: 1;
}

.saveNoticeOkButton {
  border: 1px solid var(--themeAccent);
  border-radius: 8px;
  background: #ffffff;
  color: var(--themeAccentStrong);
  font-size: 12px;
  font-weight: 700;
  padding: 6px 12px;
  cursor: pointer;
}

.saveNotice.defaultPosition {
  right: 14px;
  bottom: 14px;
  transform: translateY(8px);
}

.saveNotice.anchored {
  right: var(--saveNoticeRight, 14px);
  top: var(--saveNoticeTop, 50vh);
  bottom: auto;
  transform: translateY(calc(-50% + 8px));
}

.saveNotice.show {
  opacity: 1;
  pointer-events: auto;
}

.saveNotice.defaultPosition.show {
  transform: translateY(0);
}

.saveNotice.anchored.show {
  transform: translateY(-50%);
}

@keyframes tileEnter {
  from {
    opacity: 0;
    transform: translateY(8px);
  }

  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 860px) {
  .statusGrid {
    grid-template-columns: repeat(2, minmax(130px, 1fr));
  }

  .statusWide {
    grid-column: span 2;
  }

  .statusThreeWide {
    grid-column: span 2;
  }

  .formGrid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 520px) {
  .menuBar {
    align-items: flex-start;
    flex-direction: column;
  }

  .menuRight {
    width: 100%;
    justify-content: space-between;
  }

  .statusGrid {
    grid-template-columns: 1fr;
  }

  .statusThreeWide,
  .statusWide {
    grid-column: span 1;
  }
}
</style>
</head>
<body>
<header class="menuBar">
  <nav class="menuLeft" aria-label="Main menu">
    <button class="menuButton" data-menu="systemCard">System</button>
    <div class="menuGroup" id="timerSettingsMenuGroup">
      <button class="menuButton" id="timerSettingsMenuToggle" type="button">Timer Settings</button>
      <div class="subMenu" id="timerSettingsSubMenu">
        <button class="menuButton menuSubButton" data-menu="cyclicTimerSettingsCard" type="button">Cyclic Timer Settings</button>
        <button class="menuButton menuSubButton" data-menu="twentyFourHTimerSettingsCard" type="button">24h Timer Settings</button>
      </div>
    </div>
    <div class="menuDivider" aria-hidden="true"></div>
    <div class="menuGroup" id="profileMenuGroup">
      <button class="menuButton" id="profileMenuToggle" type="button">Profiles</button>
      <div class="subMenu" id="profileSubMenu">
        <button class="menuButton menuSubButton" data-menu="loadProfileCard" type="button">Load Profile</button>
        <button class="menuButton menuSubButton" data-menu="saveProfileCard" type="button">Save Profile</button>
        <button class="menuButton menuSubButton" data-menu="newProfileCard" type="button">New Profile</button>
        <button class="menuButton menuSubButton" data-menu="deleteProfileCard" type="button">Delete Profile</button>
      </div>
    </div>
  </nav>
  <div class="menuRight">
    <div id="headerNetworkInfo">Network: Loading...</div>
    <div class="clockValue"><span id="headerClock" class="headerValue">00:00:00</span></div>
  </div>
</header>

<div id="saveNotice" class="saveNotice defaultPosition" role="status" aria-live="polite"></div>

<main class="appShell">
  <section class="tile">
    <div class="tileHeader">
      <div class="tileTitle">Timer Screen</div>
      <div class="metaLine">Profile: <span id="headerProfileName" class="profileNameBold">-</span></div>
    </div>
    <div class="tileBody">
      <div class="statusGrid">
        <div class="statusItem">
          <div class="statusLabel">State</div>
          <div id="statusState" class="statusValue">-</div>
        </div>
        <div class="statusItem">
          <div id="statusOnLabel" class="statusLabel">On Time</div>
          <div id="statusOn" class="statusValue">-</div>
        </div>
        <div class="statusItem">
          <div id="statusOffLabel" class="statusLabel">Off Time</div>
          <div id="statusOff" class="statusValue">-</div>
        </div>
        <div class="statusItem statusThreeWide" id="statusNextRangeItem" style="display:none;">
          <div class="statusLabel">Next Change Between</div>
          <div id="statusNextRange" class="statusValue statusOutputValue">--:-- - --:--</div>
        </div>
        <div class="statusItem statusWide">
          <div class="statusLabel">Output</div>
          <div id="statusOutput" class="statusValue statusOutputValue">-</div>
        </div>
        <div class="statusItem" id="statusCyclesItem">
          <div class="statusLabel">Cycles</div>
          <div id="statusCycles" class="statusValue">-</div>
        </div>
        <div class="statusItem" id="statusWarpModeItem" style="display:none;">
          <div class="statusLabel">Warp Mode</div>
          <div id="statusWarpMode" class="statusValue">-</div>
        </div>
      </div>

      <div class="actions">
        <button id="startButton" class="actionButton actionStart" type="button">Start</button>
        <button id="stopButton" class="actionButton actionStop" type="button">Stop</button>
        <button id="resetButton" class="actionButton actionReset" type="button">Reset</button>
      </div>
    </div>
  </section>

  <section id="cyclicTimerSettingsCard" class="tile menuTile" aria-hidden="true">
    <div class="tileHeader">
      <div class="tileTitle">Cyclic Timer Settings</div>
    </div>
    <div class="tileBody">
      <p class="tileNotice">Only mutable when Timer is Stopped.</p>
      <div class="formGrid">
        <div class="fieldRow"><label for="onTimeValue">On Time</label><input id="onTimeValue" type="number" min="0"></div>
        <div class="fieldRow"><label for="onTimeUnit">On Time Unit</label><select id="onTimeUnit"><option value="0">ms</option><option value="1">s</option><option value="2">min</option></select></div>
        <div class="fieldRow"><label for="offTimeValue">Off Time</label><input id="offTimeValue" type="number" min="0"></div>
        <div class="fieldRow"><label for="offTimeUnit">Off Time Unit</label><select id="offTimeUnit"><option value="0">ms</option><option value="1">s</option><option value="2">min</option></select></div>
        <div class="fieldRow"><label for="repeatCount">Cycles</label><input id="repeatCount" type="number" min="0"></div>
        <div class="fieldRow"><label for="timerType">Timer Type</label><select id="timerType"><option value="0">Cyclic</option><option value="1">24h</option></select></div>
        <div class="fieldRow"><label for="triggerMode">Trigger Mode</label><select id="triggerMode"><option value="0">Manual</option><option value="1">External</option></select></div>
        <div class="fieldRow"><label for="triggerEdge">Trigger Edge</label><select id="triggerEdge"><option value="0">Falling</option><option value="1">Rising</option></select></div>
        <div class="fieldRow"><label for="lockInputDuringRun">Lock Input While Running</label><select id="lockInputDuringRun"><option value="0">No</option><option value="1">Yes</option></select></div>
      </div>

      <div class="footerActions">
        <button id="cancelSettingsButton" class="secondaryButton" type="button">Cancel</button>
        <button id="saveSettingsButton" class="primaryButton" type="button">Save Settings</button>
      </div>
    </div>
  </section>

  <section id="twentyFourHTimerSettingsCard" class="tile menuTile" aria-hidden="true">
    <div class="tileHeader">
      <div class="tileTitle">24h Timer Settings</div>
    </div>
    <div class="tileBody">
      <p class="tileNotice">Only mutable when Timer is Stopped.</p>
      <div class="formGrid">
        <div class="fieldRow"><label for="timerType24h">Timer Type</label><select id="timerType24h"><option value="1">24h</option></select></div>
        <div class="fieldRow"><label for="triggerMode24h">Trigger Mode</label><select id="triggerMode24h"><option value="0">Manual</option><option value="1">External</option></select></div>
        <div class="fieldRow"><label for="triggerEdge24h">Trigger Edge</label><select id="triggerEdge24h"><option value="0">Falling</option><option value="1">Rising</option></select></div>
        <div class="fieldRow"><label for="lockInputDuringRun24h">Lock Input While Running</label><select id="lockInputDuringRun24h"><option value="0">No</option><option value="1">Yes</option></select></div>
      </div>

      <div class="timer24hEditorWrap">
        <table class="timer24hEditorTable" aria-label="24h quarter-hour editor">
          <thead>
            <tr>
              <th>Hour</th>
              <th>Type</th>
              <th>00-15</th>
              <th>16-30</th>
              <th>31-45</th>
              <th>46-59</th>
            </tr>
          </thead>
          <tbody id="timer24hEditorBody"></tbody>
        </table>
      </div>

      <div class="footerActions">
        <button id="cancel24hSettingsButton" class="secondaryButton" type="button">Cancel</button>
        <button id="save24hSettingsButton" class="primaryButton" type="button">Save Settings</button>
      </div>
    </div>
  </section>

  <section id="systemCard" class="tile menuTile" aria-hidden="true">
    <div class="tileHeader">
      <div class="tileTitle">System</div>
    </div>
    <div class="tileBody">
      <p class="tileNotice">Only mutable when Timer is Stopped.</p>
      <div class="formGrid">
        <div class="fieldRow roField"><label for="systemSsid">SSID</label><input id="systemSsid" type="text" readonly></div>
        <div class="fieldRow roField"><label for="systemIp">IP</label><input id="systemIp" type="text" readonly></div>
        <div class="fieldRow roField"><label for="systemMac">MAC</label><input id="systemMac" type="text" readonly></div>
        <div class="fieldRow roField"><label for="systemEncoderOrder">Encoder X-Y</label><input id="systemEncoderOrder" type="text" readonly></div>
        <div class="fieldRow"><label for="systemOutputPolarity">Output</label><select id="systemOutputPolarity"><option value="1">Active HIGH</option><option value="0">Active LOW</option></select></div>
        <div class="fieldRow"><label for="systemAutoSaveLastProfile">Auto Save Profile</label><select id="systemAutoSaveLastProfile"><option value="1">Yes</option><option value="0">No</option></select></div>
        <div class="fieldRow"><label for="systemThemeIndex">Theme</label><select id="systemThemeIndex"><option value="0">Red</option><option value="1">Green</option><option value="2">Blue</option><option value="3">Indigo</option><option value="4">Violet</option><option value="5">Yellow</option></select></div>
        <div class="fieldRow"><label for="systemWarpSpeed">Warp Speed</label><select id="systemWarpSpeed"><option value="0">Disabled</option><option value="1">Enabled</option></select></div>
        <div class="fieldRow"><label for="systemRestart">Restart Ultimate Timer</label><select id="systemRestart"><option value="0">No</option><option value="1">Yes</option></select></div>
      </div>
      <div class="metaCard">
        <div class="metaCardLabel">Network</div>
        <div class="metaCardValue" id="systemNetworkState">-</div>
      </div>
      <div class="metaCard">
        <div class="metaCardLabel">Profiles</div>
        <div class="metaCardValue" id="profileCountValue">0</div>
      </div>
      <div class="footerActions">
        <button id="systemCancelButton" class="secondaryButton" type="button">Cancel</button>
        <button id="systemSaveButton" class="primaryButton" type="button">Save</button>
      </div>
    </div>
  </section>

  <section id="saveProfileCard" class="tile menuTile" aria-hidden="true">
    <div class="tileHeader">
      <div class="tileTitle">Save Profile</div>
    </div>
    <div class="tileBody">
      <p class="tileNotice">Only mutable when Timer is Stopped.</p>
      <div class="activeProfileBanner">
        <div class="activeProfileTitle">Active Profile</div>
        <div id="activeProfileInSave" class="activeProfileName">-</div>
      </div>
      <div class="footerActions">
        <button id="cancelSaveProfileButton" class="secondaryButton" type="button">Cancel</button>
        <button id="saveProfileButton" class="primaryButton" type="button">Save Profile</button>
      </div>
    </div>
  </section>

  <section id="loadProfileCard" class="tile menuTile" aria-hidden="true">
    <div class="tileHeader">
      <div class="tileTitle">Load Profile</div>
    </div>
    <div class="tileBody">
      <p class="tileNotice">Only mutable when Timer is Stopped.</p>
      <div class="activeProfileBanner">
        <div class="activeProfileTitle">Active Profile</div>
        <div id="activeProfileInLoad" class="activeProfileName">-</div>
      </div>
      <div class="formGrid">
        <div class="fieldRow"><label for="profilesForLoad">Profile Selection</label><select id="profilesForLoad"></select><div class="selectHint">Select from available profiles, then click Load.</div></div>
      </div>
      <div class="footerActions">
        <button id="cancelLoadProfileButton" class="secondaryButton" type="button">Cancel</button>
        <button id="loadProfileButton" class="secondaryButton" type="button">Load Profile</button>
      </div>
    </div>
  </section>

  <section id="deleteProfileCard" class="tile menuTile" aria-hidden="true">
    <div class="tileHeader">
      <div class="tileTitle">Delete Profile</div>
    </div>
    <div class="tileBody">
      <p class="tileNotice">Only mutable when Timer is Stopped.</p>
      <div class="activeProfileBanner">
        <div class="activeProfileTitle">Active Profile</div>
        <div id="activeProfileInDelete" class="activeProfileName">-</div>
      </div>
      <div class="formGrid">
        <div class="fieldRow"><label for="profilesForDelete">Profile Selection</label><select id="profilesForDelete"></select><div class="selectHint">Active profile and selection are shown separately.</div></div>
      </div>
      <div class="footerActions">
        <button id="cancelDeleteProfileButton" class="secondaryButton" type="button">Cancel</button>
        <button id="deleteProfileButton" class="dangerButton" type="button">Delete Profile</button>
      </div>
    </div>
  </section>

  <section id="newProfileCard" class="tile menuTile" aria-hidden="true">
    <div class="tileHeader">
      <div class="tileTitle">New Profile</div>
    </div>
    <div class="tileBody">
      <p class="tileNotice">Only mutable when Timer is Stopped.</p>
      <div class="activeProfileBanner">
        <div class="activeProfileTitle">Active Profile</div>
        <div id="activeProfileInNew" class="activeProfileName">-</div>
      </div>
      <div class="formGrid">
        <div class="fieldRow"><label for="newProfileName">New Profile Name</label><input id="newProfileName" type="text" maxlength="16"></div>
      </div>
      <div class="footerActions">
        <button id="cancelNewProfileButton" class="secondaryButton" type="button">Cancel</button>
        <button id="newProfileButton" class="primaryButton" type="button">Create Profile</button>
      </div>
    </div>
  </section>
</main>

<script>
let statusRefreshTimer = null;
let applySettingsTimer = null;
let savedProfileName = '';
let savedProfileSignature = '';
let profileDirtyResetPending = false;
let activeMenuId = '';
let saveNoticeResolve = null;

async function syncTimerForMenuState(previousMenuId, nextMenuId)
{
  const hadOpenMenu = previousMenuId !== '';
  const hasOpenMenu = nextMenuId !== '';

  if (hadOpenMenu === hasOpenMenu)
  {
    return;
  }

  await callPost(hasOpenMenu ? '/api/stop' : '/api/start');
}

function showSaveNotice(message, anchorButtonId)
{
  const saveNotice = document.getElementById('saveNotice');

  if (!saveNotice)
  {
    return Promise.resolve();
  }

  if (saveNoticeResolve !== null)
  {
    saveNoticeResolve();
    saveNoticeResolve = null;
  }

  const anchorButton = anchorButtonId ? document.getElementById(anchorButtonId) : null;
  const saveNoticeContent = document.createElement('div');
  const saveNoticeMessage = document.createElement('span');
  const saveNoticeOkButton = document.createElement('button');

  saveNoticeContent.className = 'saveNoticeContent';
  saveNoticeMessage.className = 'saveNoticeMessage';
  saveNoticeMessage.textContent = message;
  saveNoticeOkButton.className = 'saveNoticeOkButton';
  saveNoticeOkButton.type = 'button';
  saveNoticeOkButton.textContent = 'OK';
  saveNoticeContent.appendChild(saveNoticeMessage);
  saveNoticeContent.appendChild(saveNoticeOkButton);
  saveNotice.innerHTML = '';
  saveNotice.appendChild(saveNoticeContent);

  if (anchorButton)
  {
    const buttonRect = anchorButton.getBoundingClientRect();
    const popupTop = Math.max(14, Math.min(window.innerHeight - 14, buttonRect.top + (buttonRect.height / 2)));
    const popupRight = Math.max(14, window.innerWidth - buttonRect.right);

    saveNotice.style.setProperty('--saveNoticeTop', String(popupTop) + 'px');
    saveNotice.style.setProperty('--saveNoticeRight', String(popupRight) + 'px');
    saveNotice.classList.remove('defaultPosition');
    saveNotice.classList.add('anchored');
  }
  else
  {
    saveNotice.classList.remove('anchored');
    saveNotice.classList.add('defaultPosition');
  }

  saveNotice.classList.add('show');

  return new Promise((resolve) =>
  {
    saveNoticeResolve = resolve;
    saveNoticeOkButton.addEventListener('click', () =>
    {
      saveNotice.classList.remove('show');
      if (saveNoticeResolve !== null)
      {
        const pendingResolve = saveNoticeResolve;

        saveNoticeResolve = null;
        pendingResolve();
      }
    }, { once: true });
  });
}

function shouldWarnAutoSaveDisabled()
{
  const autoSaveControl = document.getElementById('systemAutoSaveLastProfile');

  return !!autoSaveControl && autoSaveControl.value === '0';
}

function buildProfileSettingsSignature(settings)
{
  return [
    String(settings.onTimeValue),
    String(settings.offTimeValue),
    String(settings.onTimeUnit),
    String(settings.offTimeUnit),
    String(settings.repeatCount),
    String(settings.timerType),
    String(settings.triggerMode),
    String(settings.triggerEdge),
    settings.lockInputDuringRun ? '1' : '0',
    Array.from(settings.timer24hQuarterStates || []).join(',')
  ].join('|');
}

function updateProfileHeaderDisplay(settings)
{
  const profileName = settings.profileName || '-';
  const currentSignature = buildProfileSettingsSignature(settings);

  if (savedProfileName === '' || profileName !== savedProfileName || profileDirtyResetPending)
  {
    savedProfileName = profileName;
    savedProfileSignature = currentSignature;
    profileDirtyResetPending = false;
  }

  const profileIsDirty = currentSignature !== savedProfileSignature;

  const profileDisplay = (profileIsDirty && profileName !== '-')
      ? profileName + ' (not saved)'
      : profileName;
  document.getElementById('headerProfileName').textContent = profileDisplay;

  return profileName;
}

function updateHeaderClock()
{
  const now = new Date();
  const hours = String(now.getHours()).padStart(2, '0');
  const minutes = String(now.getMinutes()).padStart(2, '0');
  const seconds = String(now.getSeconds()).padStart(2, '0');
  document.getElementById('headerClock').textContent = hours + ':' + minutes + ':' + seconds;
}

function formatChangeWindowLabel(startSeconds, endSeconds)
{
  let displayStartSeconds = Number(startSeconds || 0);
  let displayEndSeconds = Number(endSeconds || 0);

  if (displayStartSeconds !== displayEndSeconds)
  {
    if ((displayStartSeconds % 60) === 0)
    {
      displayStartSeconds += 60;
    }
    else
    {
      displayStartSeconds = Math.ceil(displayStartSeconds / 60) * 60;
    }

    if ((displayEndSeconds % 60) !== 0)
    {
      displayEndSeconds = Math.ceil(displayEndSeconds / 60) * 60;
    }
  }

  return formatHhMmFromSeconds(displayStartSeconds) + ' - ' + formatHhMmFromSeconds(displayEndSeconds);
}

function applyTheme(themeName)
{
  const palettes = {
    Red: { accent: '#b44343', strong: '#933939', tint: '#faecec', glow: 'rgba(180, 67, 67, 0.25)' },
    Green: { accent: '#2b8b67', strong: '#206e51', tint: '#e8f7f0', glow: 'rgba(43, 139, 103, 0.25)' },
    Blue: { accent: '#2b6ea3', strong: '#245d89', tint: '#e8f2fb', glow: 'rgba(43, 110, 163, 0.25)' },
    Indigo: { accent: '#4d5cb8', strong: '#3e4b98', tint: '#eceefd', glow: 'rgba(77, 92, 184, 0.25)' },
    Violet: { accent: '#8a4db3', strong: '#703c94', tint: '#f4ebfb', glow: 'rgba(138, 77, 179, 0.25)' },
    Yellow: { accent: '#b99325', strong: '#997818', tint: '#fdf7e2', glow: 'rgba(185, 147, 37, 0.25)' },
    Orange: { accent: '#c06b2e', strong: '#9b5522', tint: '#fdf0e5', glow: 'rgba(192, 107, 46, 0.25)' }
  };

  const fallback = palettes.Blue;
  const palette = palettes[themeName] || fallback;
  const root = document.documentElement;

  root.style.setProperty('--themeAccent', palette.accent);
  root.style.setProperty('--themeAccentStrong', palette.strong);
  root.style.setProperty('--themeTint', palette.tint);
  root.style.setProperty('--themeGlow', palette.glow);
}

function formatCountdown(durationMs, elapsedMs)
{
  if (durationMs <= 0 || elapsedMs < 0)
  {
    return '---:--';
  }

  const remainingMs = Math.max(0, durationMs - elapsedMs);
  const totalSeconds = Math.floor(remainingMs / 1000);
  const minutes = String(Math.floor(totalSeconds / 60)).padStart(3, '0');
  const seconds = String(totalSeconds % 60).padStart(2, '0');

  return minutes + ':' + seconds;
}

function formatHhMmSsFromSeconds(totalSeconds)
{
  if (!Number.isFinite(totalSeconds) || totalSeconds < 0)
  {
    return '--:--:--';
  }

  const normalized = Math.floor(totalSeconds) % 86400;
  const hours = String(Math.floor(normalized / 3600)).padStart(2, '0');
  const minutes = String(Math.floor((normalized % 3600) / 60)).padStart(2, '0');
  const seconds = String(normalized % 60).padStart(2, '0');

  return hours + ':' + minutes + ':' + seconds;
}

function formatHhMmFromSeconds(totalSeconds)
{
  if (!Number.isFinite(totalSeconds) || totalSeconds < 0)
  {
    return '--:--';
  }

  const normalized = Math.floor(totalSeconds) % 86400;
  const hours = String(Math.floor(normalized / 3600)).padStart(2, '0');
  const minutes = String(Math.floor((normalized % 3600) / 60)).padStart(2, '0');

  return hours + ':' + minutes;
}

function get24hQuarterStateLabel(state)
{
  switch (Number(state))
  {
    case 0: return '-';
    case 1: return '+';
    case 2: return 'R';
    case 3: return 'r';
    default: return '-';
  }
}

function get24hHourTypeLabel(type)
{
  if (Number(type) === 4)
  {
    return 'S';
  }

  return get24hQuarterStateLabel(type);
}

function get24hHourTypeFromQuarterStates(quarterValues)
{
  const first = Number(quarterValues[0]);

  for (let quarter = 1; quarter < 4; quarter++)
  {
    if (Number(quarterValues[quarter]) !== first)
    {
      return 4;
    }
  }

  return (Number.isFinite(first) && first >= 0 && first <= 3) ? first : 0;
}

function build24hHourTypeSelectHtml(hourIndex, selectedType)
{
  const selected = Number(selectedType);
  let html = '<select class="timer24hHourTypeSelect" data-hour="' + String(hourIndex) + '" onchange="handle24hHourTypeChange(' + String(hourIndex) + ')">';

  for (let type = 0; type <= 4; type++)
  {
    const isSelected = type === selected ? ' selected' : '';
    html += '<option value="' + String(type) + '"' + isSelected + '>' + get24hHourTypeLabel(type) + '</option>';
  }

  html += '</select>';

  return html;
}

function build24hQuarterSelectHtml(hourIndex, quarterIndex, selectedState)
{
  const selected = Number(selectedState);
  let html = '<select class="timer24hQuarterSelect" data-hour="' + String(hourIndex) + '" data-quarter="' + String(quarterIndex) + '">';

  for (let state = 0; state <= 3; state++)
  {
    const isSelected = state === selected ? ' selected' : '';
    html += '<option value="' + String(state) + '"' + isSelected + '>' + get24hQuarterStateLabel(state) + '</option>';
  }

  html += '</select>';

  return html;
}

function set24hHourQuarterMutability(hourIndex, splitEnabled)
{
  const quarterSelects = document.querySelectorAll('#timer24hEditorBody select.timer24hQuarterSelect[data-hour="' + String(hourIndex) + '"]');

  for (const quarterSelect of quarterSelects)
  {
    quarterSelect.disabled = !splitEnabled;
  }
}

function apply24hHourTypeToRow(hourIndex)
{
  const hourTypeSelect = document.querySelector('#timer24hEditorBody select.timer24hHourTypeSelect[data-hour="' + String(hourIndex) + '"]');

  if (!hourTypeSelect)
  {
    return;
  }

  const hourType = Number(hourTypeSelect.value);
  const splitEnabled = hourType === 4;
  const quarterSelects = document.querySelectorAll('#timer24hEditorBody select.timer24hQuarterSelect[data-hour="' + String(hourIndex) + '"]');

  if (!splitEnabled && hourType >= 0 && hourType <= 3)
  {
    for (const quarterSelect of quarterSelects)
    {
      quarterSelect.value = String(hourType);
    }
  }

  set24hHourQuarterMutability(hourIndex, splitEnabled);
}

function handle24hHourTypeChange(hourIndex)
{
  apply24hHourTypeToRow(hourIndex);
}

function render24hEditorFromArray(quarterStates)
{
  const body = document.getElementById('timer24hEditorBody');

  if (!body)
  {
    return;
  }

  const states = Array.isArray(quarterStates) ? quarterStates : [];
  let rowsHtml = '';

  for (let hour = 0; hour < 24; hour++)
  {
    const hourLabel = String(hour).padStart(2, '0');
    const rowQuarterStates = [0, 0, 0, 0];

    rowsHtml += '<tr>';
    rowsHtml += '<td class="timer24hEditorHour">' + hourLabel + '</td>';

    for (let quarter = 0; quarter < 4; quarter++)
    {
      const index = (hour * 4) + quarter;
      const value = Number(states[index] ?? 0);
      const normalized = (Number.isFinite(value) && value >= 0 && value <= 3) ? value : 0;

      rowQuarterStates[quarter] = normalized;
    }

    rowsHtml += '<td>' + build24hHourTypeSelectHtml(hour, get24hHourTypeFromQuarterStates(rowQuarterStates)) + '</td>';

    for (let quarter = 0; quarter < 4; quarter++)
    {
      rowsHtml += '<td>' + build24hQuarterSelectHtml(hour, quarter, rowQuarterStates[quarter]) + '</td>';
    }

    rowsHtml += '</tr>';
  }

  body.innerHTML = rowsHtml;

  for (let hour = 0; hour < 24; hour++)
  {
    apply24hHourTypeToRow(hour);
  }

  // Add change event listeners to quarter selects for reverse sync
  const quarterSelects = document.querySelectorAll('#timer24hEditorBody select.timer24hQuarterSelect');
  for (const select of quarterSelects)
  {
    select.addEventListener('change', function() {
      const hourIndex = Number(this.dataset.hour);
      // Get all quarters for this hour
      const hourQuarterSelects = document.querySelectorAll('#timer24hEditorBody select.timer24hQuarterSelect[data-hour="' + String(hourIndex) + '"]');
      const quarterStates = [];
      for (const q of hourQuarterSelects)
      {
        quarterStates.push(Number(q.value));
      }
      // Derive new hour type from quarters
      const newHourType = get24hHourTypeFromQuarterStates(quarterStates);
      // Update hour type select
      const hourTypeSelect = document.querySelector('#timer24hEditorBody select.timer24hHourTypeSelect[data-hour="' + String(hourIndex) + '"]');
      if (hourTypeSelect)
      {
        hourTypeSelect.value = String(newHourType);
      }
      // Apply the new hour type to update disabled states
      apply24hHourTypeToRow(hourIndex);
    });
  }
}

function read24hQuarterStatesFromEditor()
{
  const values = new Array(96).fill(0);
  const selects = document.querySelectorAll('#timer24hEditorBody select.timer24hQuarterSelect');

  for (const select of selects)
  {
    const hour = Number(select.dataset.hour);
    const quarter = Number(select.dataset.quarter);

    if (!Number.isInteger(hour) || !Number.isInteger(quarter) || hour < 0 || hour > 23 || quarter < 0 || quarter > 3)
    {
      continue;
    }

    const index = (hour * 4) + quarter;
    const rawValue = Number(select.value);
    values[index] = (Number.isFinite(rawValue) && rawValue >= 0 && rawValue <= 3) ? rawValue : 0;
  }

  return values;
}

function setActiveMenu(menuId)
{
  const previousMenuId = activeMenuId;
  activeMenuId = menuId;

  const menuButtons = document.querySelectorAll('.menuButton');
  const menuTiles = document.querySelectorAll('.menuTile');
  const profileMenuIds = ['loadProfileCard', 'saveProfileCard', 'newProfileCard', 'deleteProfileCard'];
  const timerSettingsMenuIds = ['cyclicTimerSettingsCard', 'twentyFourHTimerSettingsCard'];

  for (const button of menuButtons)
  {
    const isActive = button.dataset.menu === menuId;
    button.classList.toggle('active', isActive);
  }

  const profileMenuToggle = document.getElementById('profileMenuToggle');
  const timerSettingsMenuToggle = document.getElementById('timerSettingsMenuToggle');

  if (profileMenuToggle)
  {
    profileMenuToggle.classList.toggle('active', profileMenuIds.includes(menuId));
  }

  if (timerSettingsMenuToggle)
  {
    timerSettingsMenuToggle.classList.toggle('active', timerSettingsMenuIds.includes(menuId));
  }

  for (const tile of menuTiles)
  {
    const isActive = tile.id === menuId;
    tile.classList.toggle('active', isActive);
    tile.setAttribute('aria-hidden', isActive ? 'false' : 'true');
  }

  void syncTimerForMenuState(previousMenuId, menuId);
}

function closeProfileSubMenu()
{
  const profileSubMenu = document.getElementById('profileSubMenu');

  if (profileSubMenu)
  {
    profileSubMenu.classList.remove('open');
  }
}

function closeTimerSettingsSubMenu()
{
  const timerSettingsSubMenu = document.getElementById('timerSettingsSubMenu');

  if (timerSettingsSubMenu)
  {
    timerSettingsSubMenu.classList.remove('open');
  }
}

function bindMenu()
{
  const menuButtons = document.querySelectorAll('.menuButton');
  const profileMenuToggle = document.getElementById('profileMenuToggle');
  const timerSettingsMenuToggle = document.getElementById('timerSettingsMenuToggle');
  const profileMenuGroup = document.getElementById('profileMenuGroup');
  const timerSettingsMenuGroup = document.getElementById('timerSettingsMenuGroup');

  for (const button of menuButtons)
  {
    if (!button.dataset.menu)
    {
      continue;
    }

    button.addEventListener('click', () =>
    {
      const activeButton = document.querySelector('.menuButton.active');
      const activeMenuId = activeButton ? activeButton.dataset.menu : '';
      const targetMenuId = button.dataset.menu;

      if (activeMenuId === targetMenuId)
      {
        setActiveMenu('');
        closeProfileSubMenu();
        closeTimerSettingsSubMenu();

        return;
      }

      setActiveMenu(targetMenuId);
      closeProfileSubMenu();
      closeTimerSettingsSubMenu();
    });
  }

  if (profileMenuToggle)
  {
    profileMenuToggle.addEventListener('click', (event) =>
    {
      event.stopPropagation();
      const profileSubMenu = document.getElementById('profileSubMenu');

      if (!profileSubMenu)
      {
        return;
      }

      profileSubMenu.classList.toggle('open');
      closeTimerSettingsSubMenu();
    });
  }

  if (timerSettingsMenuToggle)
  {
    timerSettingsMenuToggle.addEventListener('click', (event) =>
    {
      event.stopPropagation();
      const timerSettingsSubMenu = document.getElementById('timerSettingsSubMenu');

      if (!timerSettingsSubMenu)
      {
        return;
      }

      timerSettingsSubMenu.classList.toggle('open');
      closeProfileSubMenu();
    });
  }

  document.addEventListener('click', (event) =>
  {
    if (profileMenuGroup && !profileMenuGroup.contains(event.target))
    {
      closeProfileSubMenu();
    }

    if (timerSettingsMenuGroup && !timerSettingsMenuGroup.contains(event.target))
    {
      closeTimerSettingsSubMenu();
    }
  });
}

function setStatusAutoRefresh(enabled)
{
  if (statusRefreshTimer !== null)
  {
    return;
  }

  statusRefreshTimer = setInterval(refreshStatus, 1000);
}

function setEditControlsEnabled(enabled)
{
  const cardIds = ['cyclicTimerSettingsCard', 'twentyFourHTimerSettingsCard', 'systemCard', 'saveProfileCard', 'loadProfileCard', 'deleteProfileCard', 'newProfileCard'];
  const cancelButtonIds = new Set([
    'cancelSettingsButton',
    'systemCancelButton',
    'cancelSaveProfileButton',
    'cancelLoadProfileButton',
    'cancelDeleteProfileButton',
    'cancelNewProfileButton'
  ]);
  const alwaysEnabledButtonIds = new Set([
    'saveProfileButton'
  ]);

  for (const cardId of cardIds)
  {
    const card = document.getElementById(cardId);

    if (!card)
    {
      continue;
    }

    const controls = card.querySelectorAll('input, select, button');

    for (const control of controls)
    {
      if (cancelButtonIds.has(control.id) || alwaysEnabledButtonIds.has(control.id))
      {
        control.disabled = false;
        continue;
      }

      control.disabled = !enabled;
    }

    card.classList.toggle('cardDisabled', !enabled);
  }
}

function applyTimeInputConstraints()
{
  const onUnitIsMs = document.getElementById('onTimeUnit').value === '0';
  const offUnitIsMs = document.getElementById('offTimeUnit').value === '0';
  const onTimeInput = document.getElementById('onTimeValue');
  const offTimeInput = document.getElementById('offTimeValue');

  onTimeInput.min = onUnitIsMs ? '900' : '0';
  offTimeInput.min = offUnitIsMs ? '900' : '0';
}

async function refreshStatus()
{
  let data = null;

  try
  {
    const response = await fetch('/api/status');
    data = await response.json();
  }
  catch (_error)
  {
    return;
  }

  setStatusAutoRefresh(data.runtime.state !== 0);
  setEditControlsEnabled(data.runtime.state === 0);

  const is24hTimer = data.settings.timerType === 1;
  const countdown = data.runtime.state === 0
    ? '---:--'
    : formatCountdown(data.runtime.currentPhaseDurationMs, data.runtime.currentPhaseElapsedMs);
  const nextPhase = data.runtime.inOnPhase ? 'OFF' : 'ON';
  const outputLabelCyclic = data.runtime.state === 0
    ? 'OFF [---:--]'
    : (data.runtime.outputActive ? 'ON' : 'OFF') + ' [' + countdown + ' to ' + nextPhase + ']';

  let statusOnLabelText = 'On Time';
  let statusOffLabelText = 'Off Time';
  let statusOnText = data.settings.onTimeValue + ' ' + data.settings.onTimeUnitLabel;
  let statusOffText = data.settings.offTimeValue + ' ' + data.settings.offTimeUnitLabel;
  let statusOutputText = outputLabelCyclic;

  if (is24hTimer)
  {
    const status24h = data.runtime.twentyFourH || {};
    const outputActive24h = !!status24h.outputActive;
    const hasNextSwitch = !!status24h.hasNextSwitch;
    const hasNextOff = !!status24h.hasNextOff;
    const hasLastOn = !!status24h.hasLastOn;
    const hasLastOff = !!status24h.hasLastOff;
    const nextSwitchClock = hasNextSwitch
      ? formatHhMmFromSeconds(Number(status24h.nextSwitchSeconds || 0))
      : '--:--';
    const nextOffClock = hasNextOff
      ? formatHhMmFromSeconds(Number(status24h.nextOffSeconds || 0))
      : '--:--';
    const lastOnClock = hasLastOn
      ? formatHhMmFromSeconds(Number(status24h.lastOnSeconds || 0))
      : '--:--';
    const lastOffClock = hasLastOff
      ? formatHhMmFromSeconds(Number(status24h.lastOffSeconds || 0))
      : '--:--';
    const nextSwitchIn = hasNextSwitch
      ? formatHhMmSsFromSeconds(Number(status24h.nextSwitchInSeconds || 0))
      : '--:--:--';
    const nextChangeWindowText = hasNextSwitch
      ? formatChangeWindowLabel(Number(status24h.nextSwitchWindowStartSeconds || 0), Number(status24h.nextSwitchWindowEndSeconds || 0))
      : '--:-- - --:--';

    statusOnLabelText = outputActive24h ? 'Last State Change ON' : 'Last State Change OFF';
    statusOffLabelText = outputActive24h ? 'Next State Change OFF' : 'Next State Change ON';
    statusOnText = outputActive24h ? lastOnClock : lastOffClock;
    statusOffText = outputActive24h ? nextOffClock : nextSwitchClock;
    statusOutputText = (outputActive24h ? 'ON' : 'OFF') + ' [' + nextSwitchIn + ']';
    document.getElementById('statusNextRange').textContent = nextChangeWindowText;
  }

  let displayCycle = data.runtime.currentCycle;

  if (data.runtime.state === 1 || data.runtime.state === 2)
  {
    if (data.runtime.totalCycles === 0 || displayCycle < data.runtime.totalCycles)
    {
      displayCycle += 1;
    }
  }
  else if (data.runtime.state === 3)
  {
    if (data.runtime.totalCycles > 0)
    {
      displayCycle = data.runtime.totalCycles;
    }
  }

  // === Always update Timer Screen display ===
  document.getElementById('statusOnLabel').textContent = statusOnLabelText;
  document.getElementById('statusOffLabel').textContent = statusOffLabelText;
  document.getElementById('statusState').textContent = data.runtime.stateLabel;
  document.getElementById('statusOn').textContent = statusOnText;
  document.getElementById('statusOff').textContent = statusOffText;
  document.getElementById('statusOutput').textContent = statusOutputText;
  const statusNextRangeItem = document.getElementById('statusNextRangeItem');
  if (statusNextRangeItem)
  {
    statusNextRangeItem.style.display = is24hTimer ? '' : 'none';
  }
  const statusCyclesItem = document.getElementById('statusCyclesItem');
  if (statusCyclesItem)
  {
    statusCyclesItem.style.display = is24hTimer ? 'none' : '';
  }

  if (!is24hTimer)
  {
    document.getElementById('statusCycles').textContent =
      data.runtime.totalCycles === 0
        ? String(displayCycle) + '/INF'
        : String(displayCycle) + '/' + String(data.runtime.totalCycles);
  }

  //-- Update Warp Mode status (tile visible only when enabled)
  const statusWarpModeItem = document.getElementById('statusWarpModeItem');
  if (statusWarpModeItem)
  {
    const warpEnabled = !!data.settings.warpSpeedEnabled;

    statusWarpModeItem.style.display = warpEnabled ? '' : 'none';

    if (warpEnabled)
    {
      document.getElementById('statusWarpMode').textContent = 'Enabled';
    }
  }

  document.getElementById('headerNetworkInfo').textContent =
    'Network: ' + (data.network.connected ? 'Connected' : 'Not connected') + ' | IP: ' + data.network.address;

  //-- Hide action buttons for 24h timer (timerType === 1)
  const startButton = document.getElementById('startButton');
  const stopButton = document.getElementById('stopButton');
  const resetButton = document.getElementById('resetButton');
  if (is24hTimer)
  {
    startButton.style.display = 'none';
    stopButton.style.display = 'none';
    resetButton.style.display = 'none';
  }
  else
  {
    startButton.style.display = '';
    stopButton.style.display = '';
    resetButton.style.display = '';
  }

  //-- Disable Timer Settings submenu buttons based on active timer type
  const cyclicTimerButton = document.querySelector('[data-menu="cyclicTimerSettingsCard"]');
  const twentyFourHTimerButton = document.querySelector('[data-menu="twentyFourHTimerSettingsCard"]');
  
  if (cyclicTimerButton)
  {
    cyclicTimerButton.disabled = is24hTimer;
  }
  if (twentyFourHTimerButton)
  {
    twentyFourHTimerButton.disabled = !is24hTimer;
  }

  // === Update System read-only fields (always) ===
  document.getElementById('systemNetworkState').textContent = data.network.connected ? 'Connected' : 'Disconnected';
  document.getElementById('systemSsid').value = data.network.ssid || '(not connected)';
  document.getElementById('systemIp').value = data.network.address || '(none)';
  document.getElementById('systemMac').value = data.network.macAddress || '(unknown)';
  document.getElementById('systemEncoderOrder').value = data.settings.encoderOrderLabel || '(unknown)';

  // === Update profile info (always) ===
  const activeProfileName = updateProfileHeaderDisplay(data.settings);
  document.getElementById('activeProfileInSave').textContent = activeProfileName;
  document.getElementById('activeProfileInLoad').textContent = activeProfileName;
  document.getElementById('activeProfileInDelete').textContent = activeProfileName;
  document.getElementById('activeProfileInNew').textContent = activeProfileName;

  // === Update theme (always) ===
  applyTheme(data.settings.themeColorName || 'Blue');

  // === Only update editable form fields when their menus are CLOSED ===
  // This prevents form values from being reset while user is editing

  // Update Cyclic Timer Settings fields only if menu is NOT open
  const cyclicTimerSettingsCard = document.getElementById('cyclicTimerSettingsCard');
  if (cyclicTimerSettingsCard && cyclicTimerSettingsCard.getAttribute('aria-hidden') === 'true')
  {
    document.getElementById('onTimeValue').value = data.settings.onTimeValue;
    document.getElementById('offTimeValue').value = data.settings.offTimeValue;
    document.getElementById('onTimeUnit').value = data.settings.onTimeUnit;
    document.getElementById('offTimeUnit').value = data.settings.offTimeUnit;
    document.getElementById('repeatCount').value = data.settings.repeatCount;
    document.getElementById('timerType').value = data.settings.timerType;
    document.getElementById('triggerMode').value = data.settings.triggerMode;
    document.getElementById('triggerEdge').value = data.settings.triggerEdge;
    document.getElementById('lockInputDuringRun').value = data.settings.lockInputDuringRun ? '1' : '0';
  }

  // Update 24h Timer Settings fields only if menu is NOT open
  const twentyFourHTimerSettingsCard = document.getElementById('twentyFourHTimerSettingsCard');
  if (twentyFourHTimerSettingsCard && twentyFourHTimerSettingsCard.getAttribute('aria-hidden') === 'true')
  {
    document.getElementById('triggerMode24h').value = data.settings.triggerMode;
    document.getElementById('triggerEdge24h').value = data.settings.triggerEdge;
    document.getElementById('lockInputDuringRun24h').value = data.settings.lockInputDuringRun ? '1' : '0';
    render24hEditorFromArray(data.settings.timer24hQuarterStates || []);
  }

  // Update System Settings fields only if menu is NOT open
  const systemCard = document.getElementById('systemCard');
  if (systemCard && systemCard.getAttribute('aria-hidden') === 'true')
  {
    document.getElementById('systemOutputPolarity').value = data.settings.outputPolarityHigh ? '1' : '0';
    document.getElementById('systemAutoSaveLastProfile').value = data.settings.autoSaveLastProfile ? '1' : '0';
    document.getElementById('systemThemeIndex').value = String(data.settings.themeColorIndex || 0);
    document.getElementById('systemWarpSpeed').value = data.settings.warpSpeedEnabled ? '1' : '0';
  }

  applyTimeInputConstraints();
}

async function refreshProfiles()
{
  let data = null;

  try
  {
    const response = await fetch('/api/profiles');
    data = await response.json();
  }
  catch (_error)
  {
    return;
  }

  const loadSelect = document.getElementById('profilesForLoad');
  const deleteSelect = document.getElementById('profilesForDelete');
  const selectedLoadValue = loadSelect ? loadSelect.value : '';
  const selectedDeleteValue = deleteSelect ? deleteSelect.value : '';

  if (loadSelect)
  {
    loadSelect.innerHTML = '';
    for (const name of data.profiles)
    {
      const option = document.createElement('option');
      option.value = name;
      option.textContent = name;
      loadSelect.appendChild(option);
    }
    if (selectedLoadValue)
    {
      loadSelect.value = selectedLoadValue;
    }
  }

  if (deleteSelect)
  {
    deleteSelect.innerHTML = '';
    //--- Use deletable profiles list if available, otherwise filter default profiles
    const deletableList = data.deletableProfiles || data.profiles.filter(name => name !== 'default' && name !== 'default-24h');
    for (const name of deletableList)
    {
      const option = document.createElement('option');
      option.value = name;
      option.textContent = name;
      deleteSelect.appendChild(option);
    }
    if (selectedDeleteValue && deletableList.includes(selectedDeleteValue))
    {
      deleteSelect.value = selectedDeleteValue;
    }
  }

  document.getElementById('profileCountValue').textContent = String(data.profiles.length);
}

async function callPost(url, body)
{
  try
  {
    await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: body ? JSON.stringify(body) : '{}'
    });
  }
  catch (_error)
  {
    return;
  }

  await refreshStatus();
  await refreshProfiles();
}

async function saveSettings()
{
  await callPost('/api/settings', readSettingsFromForm());

  if (shouldWarnAutoSaveDisabled())
  {
    await showSaveNotice('Auto Save Profile is NO: changes are active, but not saved to the profile file.', 'saveSettingsButton');
  }

  setActiveMenu('');
}

function readSettingsFromForm()
{
  applyTimeInputConstraints();

  return {
    onTimeValue: Number(document.getElementById('onTimeValue').value),
    offTimeValue: Number(document.getElementById('offTimeValue').value),
    onTimeUnit: Number(document.getElementById('onTimeUnit').value),
    offTimeUnit: Number(document.getElementById('offTimeUnit').value),
    repeatCount: Number(document.getElementById('repeatCount').value),
    timerType: Number(document.getElementById('timerType').value),
    triggerMode: Number(document.getElementById('triggerMode').value),
    triggerEdge: Number(document.getElementById('triggerEdge').value),
    lockInputDuringRun: document.getElementById('lockInputDuringRun').value === '1',
  };
}

function readSystemFromForm()
{
  return {
    outputPolarityHigh: document.getElementById('systemOutputPolarity').value === '1',
    autoSaveLastProfile: document.getElementById('systemAutoSaveLastProfile').value === '1',
    themeColorIndex: Number(document.getElementById('systemThemeIndex').value),
    warpSpeedEnabled: document.getElementById('systemWarpSpeed').value === '1',
    restart: document.getElementById('systemRestart').value === '1'
  };
}

async function saveSystem()
{
  await callPost('/api/system/save', readSystemFromForm());
  document.getElementById('systemRestart').value = '0';
  setActiveMenu('');
}

function scheduleApplySettings()
{
  if (applySettingsTimer !== null)
  {
    clearTimeout(applySettingsTimer);
  }

  applySettingsTimer = setTimeout(async () =>
  {
    applySettingsTimer = null;

    try
    {
      await fetch('/api/settings/apply', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(readSettingsFromForm())
      });

      await refreshStatus();
    }
    catch (_error)
    {
    }
  }, 250);
}

function bindLiveApplySettings()
{
  const timerSettingIds = [
    'onTimeValue',
    'offTimeValue',
    'onTimeUnit',
    'offTimeUnit',
    'repeatCount',
    'timerType',
    'triggerMode',
    'triggerEdge',
    'lockInputDuringRun',
  ];

  for (const id of timerSettingIds)
  {
    const element = document.getElementById(id);

    if (!element)
    {
      continue;
    }

    element.addEventListener('input', scheduleApplySettings);
    element.addEventListener('change', scheduleApplySettings);
  }
}

async function saveProfile()
{
  const currentProfileName = document.getElementById('activeProfileInSave').textContent;

  try
  {
    const response = await fetch('/api/profile/save', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ profileName: currentProfileName })
    });

    if (response.ok)
    {
      profileDirtyResetPending = true;
    }
  }
  catch (_error)
  {
    return;
  }

  await refreshStatus();
  await refreshProfiles();
  setActiveMenu('');
}

async function loadSelectedProfile()
{
  await callPost('/api/profile/load', {
    profileName: document.getElementById('profilesForLoad').value
  });
  setActiveMenu('');
}

async function deleteSelectedProfile()
{
  await callPost('/api/profile/delete', {
    profileName: document.getElementById('profilesForDelete').value
  });
  setActiveMenu('');
}

async function createNewProfile()
{
  const newProfileName = document.getElementById('newProfileName').value;

  if (!newProfileName)
  {
    return;
  }

  await callPost('/api/profile/save', {
    profileName: newProfileName
  });
  document.getElementById('newProfileName').value = '';
  setActiveMenu('');
}

function bindActionButtons()
{
  document.getElementById('startButton').addEventListener('click', async () =>
  {
    await callPost('/api/start');
  });

  document.getElementById('stopButton').addEventListener('click', async () =>
  {
    await callPost('/api/stop');
  });

  document.getElementById('resetButton').addEventListener('click', async () =>
  {
    await callPost('/api/reset');
  });

  document.getElementById('cancelSettingsButton').addEventListener('click', () =>
  {
    setActiveMenu('');
  });
  document.getElementById('saveSettingsButton').addEventListener('click', saveSettings);
  document.getElementById('cancel24hSettingsButton').addEventListener('click', () =>
  {
    setActiveMenu('');
  });
  document.getElementById('save24hSettingsButton').addEventListener('click', async () =>
  {
    const settings = {
      timerType: 1,
      triggerMode: Number(document.getElementById('triggerMode24h').value),
      triggerEdge: Number(document.getElementById('triggerEdge24h').value),
      lockInputDuringRun: document.getElementById('lockInputDuringRun24h').value === '1',
      timer24hQuarterStates: read24hQuarterStatesFromEditor()
    };

    await callPost('/api/settings', settings);

    if (shouldWarnAutoSaveDisabled())
    {
      await showSaveNotice('Auto Save Profile is NO: changes are active, but not saved to the profile file.', 'save24hSettingsButton');
    }

    setActiveMenu('');
  });
  document.getElementById('systemCancelButton').addEventListener('click', () =>
  {
    setActiveMenu('');
  });
  document.getElementById('systemSaveButton').addEventListener('click', saveSystem);
  document.getElementById('saveProfileButton').addEventListener('click', saveProfile);
  document.getElementById('loadProfileButton').addEventListener('click', loadSelectedProfile);
  document.getElementById('deleteProfileButton').addEventListener('click', deleteSelectedProfile);
  document.getElementById('newProfileButton').addEventListener('click', createNewProfile);
  document.getElementById('cancelSaveProfileButton').addEventListener('click', () =>
  {
    setActiveMenu('');
  });
  document.getElementById('cancelLoadProfileButton').addEventListener('click', () =>
  {
    setActiveMenu('');
  });
  document.getElementById('cancelDeleteProfileButton').addEventListener('click', () =>
  {
    setActiveMenu('');
  });
  document.getElementById('cancelNewProfileButton').addEventListener('click', () =>
  {
    setActiveMenu('');
  });
}

setInterval(refreshProfiles, 3000);
setInterval(updateHeaderClock, 1000);
updateHeaderClock();
render24hEditorFromArray(new Array(96).fill(0));
bindMenu();
bindActionButtons();
bindLiveApplySettings();
refreshStatus();
refreshProfiles();
</script>
</body>
</html>
)HTML";

//--- Send JSON response with current status
static void handleStatus();

//--- Update settings from request body
static void handleSaveSettings();

//--- Apply settings from request body without persistence
static void handleApplySettings();

//--- Return profile list
static void handleProfiles();

//--- Save profile from current settings
static void handleSaveProfile();

//--- Load profile into current settings
static void handleLoadProfile();

//--- Delete selected profile
static void handleDeleteProfile();

//--- Save system settings and optional restart
static void handleSaveSystem();

//--- Parse JSON body
static bool parseJsonBody(JsonDocument& doc);

//--- Fill status document
static void fillStatusDocument(JsonDocument& doc);

//--- Enforce minimum ON/OFF value for ms units
static void enforceMsMinimum(AppSettings& settings);

//--- Format seconds-of-day as HH:MM:SS
static String formatHhMmSsFromSecondsOfDay(uint32_t secondsOfDay);

//--- Active state
static bool serverRunning = false;

//--- Initialize web UI
void webUiInit()
{
  server.on("/", HTTP_GET, []()
            { server.send(200, "text/html", indexHtml); });

  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/settings", HTTP_POST, handleSaveSettings);
  server.on("/api/settings/apply", HTTP_POST, handleApplySettings);
  server.on("/api/start", HTTP_POST, []()
            {
              timerStart();
              server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/stop", HTTP_POST, []()
            {
              timerStop();
              server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/reset", HTTP_POST, []()
            {
              timerReset();
              server.send(200, "application/json", "{\"ok\":true}"); });
  server.on("/api/profiles", HTTP_GET, handleProfiles);
  server.on("/api/profile/save", HTTP_POST, handleSaveProfile);
  server.on("/api/profile/load", HTTP_POST, handleLoadProfile);
  server.on("/api/profile/delete", HTTP_POST, handleDeleteProfile);
  server.on("/api/system/save", HTTP_POST, handleSaveSystem);

  server.begin();
  serverRunning = true;

  ESP_LOGI(logTag, "Web UI started");

} //   webUiInit()

//--- Update web UI
void webUiUpdate()
{
  if (!serverRunning)
  {
    return;
  }

  server.handleClient();

} //   webUiUpdate()

//--- Suspend web UI server
void webUiSuspend()
{
  if (!serverRunning)
  {
    return;
  }

  server.stop();
  serverRunning = false;

  ESP_LOGI(logTag, "Web UI suspended");

} //   webUiSuspend()

//--- Resume web UI server
void webUiResume()
{
  if (serverRunning)
  {
    return;
  }

  server.begin();
  serverRunning = true;

  ESP_LOGI(logTag, "Web UI resumed");

} //   webUiResume()

//--- Send JSON response with current status
static void handleStatus()
{
  JsonDocument doc;
  fillStatusDocument(doc);

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

} //   handleStatus()

//--- Update settings from request body
static void handleSaveSettings()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  AppSettings settings = timerGetSettings();
  settings.timerType = static_cast<TimerType>(doc["timerType"] | static_cast<int>(settings.timerType));
  settings.onTimeValue = doc["onTimeValue"] | settings.onTimeValue;
  settings.offTimeValue = doc["offTimeValue"] | settings.offTimeValue;
  settings.onTimeUnit = static_cast<TimeUnit>(doc["onTimeUnit"] | static_cast<int>(settings.onTimeUnit));
  settings.offTimeUnit = static_cast<TimeUnit>(doc["offTimeUnit"] | static_cast<int>(settings.offTimeUnit));
  settings.repeatCount = static_cast<uint32_t>(doc["repeatCount"] | settings.repeatCount);
  settings.triggerMode = static_cast<TriggerMode>(doc["triggerMode"] | static_cast<int>(settings.triggerMode));
  settings.triggerEdge = static_cast<TriggerEdge>(doc["triggerEdge"] | static_cast<int>(settings.triggerEdge));
  {
    JsonArrayConst quarterStates = doc["timer24hQuarterStates"].as<JsonArrayConst>();

    if (!quarterStates.isNull())
    {
      for (size_t stateIndex = 0; stateIndex < sizeof(settings.timer24hQuarterStates); stateIndex++)
      {
        if (stateIndex < quarterStates.size())
        {
          int rawState = quarterStates[stateIndex] | static_cast<int>(settings.timer24hQuarterStates[stateIndex]);

          if (rawState < static_cast<int>(TIMER_24H_QUARTER_OFF))
          {
            rawState = static_cast<int>(TIMER_24H_QUARTER_OFF);
          }
          else if (rawState > static_cast<int>(TIMER_24H_QUARTER_RANDOM_OFF))
          {
            rawState = static_cast<int>(TIMER_24H_QUARTER_RANDOM_OFF);
          }

          settings.timer24hQuarterStates[stateIndex] = static_cast<uint8_t>(rawState);
        }
      }
    }
  }
  settings.outputPolarityHigh = doc["outputPolarityHigh"] | settings.outputPolarityHigh;
  settings.lockInputDuringRun = doc["lockInputDuringRun"] | settings.lockInputDuringRun;
  settings.autoSaveLastProfile = doc["autoSaveLastProfile"] | settings.autoSaveLastProfile;
  settings.profileName = String(static_cast<const char*>(doc["profileName"] | settings.profileName.c_str()));
  enforceMsMinimum(settings);

  timerSetSettings(settings);
  settings = timerGetSettings();
  settingsStoreSaveSystemSettings(settings);

  if (DEFAULT_AUTO_SAVE_LAST_PROFILE != 0 && !settings.profileName.isEmpty())
  {
    profileManagerSaveProfile(settings.profileName, settings);
    settingsStoreSaveLastProfileName(settings.profileName);
  }

  handleStatus();

} //   handleSaveSettings()

//--- Apply settings from request body without persistence
static void handleApplySettings()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  AppSettings settings = timerGetSettings();
  settings.timerType = static_cast<TimerType>(doc["timerType"] | static_cast<int>(settings.timerType));
  settings.onTimeValue = doc["onTimeValue"] | settings.onTimeValue;
  settings.offTimeValue = doc["offTimeValue"] | settings.offTimeValue;
  settings.onTimeUnit = static_cast<TimeUnit>(doc["onTimeUnit"] | static_cast<int>(settings.onTimeUnit));
  settings.offTimeUnit = static_cast<TimeUnit>(doc["offTimeUnit"] | static_cast<int>(settings.offTimeUnit));
  settings.repeatCount = static_cast<uint32_t>(doc["repeatCount"] | settings.repeatCount);
  settings.triggerMode = static_cast<TriggerMode>(doc["triggerMode"] | static_cast<int>(settings.triggerMode));
  settings.triggerEdge = static_cast<TriggerEdge>(doc["triggerEdge"] | static_cast<int>(settings.triggerEdge));
  {
    JsonArrayConst quarterStates = doc["timer24hQuarterStates"].as<JsonArrayConst>();

    if (!quarterStates.isNull())
    {
      for (size_t stateIndex = 0; stateIndex < sizeof(settings.timer24hQuarterStates); stateIndex++)
      {
        if (stateIndex < quarterStates.size())
        {
          int rawState = quarterStates[stateIndex] | static_cast<int>(settings.timer24hQuarterStates[stateIndex]);

          if (rawState < static_cast<int>(TIMER_24H_QUARTER_OFF))
          {
            rawState = static_cast<int>(TIMER_24H_QUARTER_OFF);
          }
          else if (rawState > static_cast<int>(TIMER_24H_QUARTER_RANDOM_OFF))
          {
            rawState = static_cast<int>(TIMER_24H_QUARTER_RANDOM_OFF);
          }

          settings.timer24hQuarterStates[stateIndex] = static_cast<uint8_t>(rawState);
        }
      }
    }
  }
  settings.outputPolarityHigh = doc["outputPolarityHigh"] | settings.outputPolarityHigh;
  settings.lockInputDuringRun = doc["lockInputDuringRun"] | settings.lockInputDuringRun;
  settings.autoSaveLastProfile = doc["autoSaveLastProfile"] | settings.autoSaveLastProfile;
  settings.profileName = String(static_cast<const char*>(doc["profileName"] | settings.profileName.c_str()));

  timerSetSettings(settings);
  settings = timerGetSettings();

  if (DEFAULT_AUTO_SAVE_LAST_PROFILE != 0 && !settings.profileName.isEmpty())
  {
    profileManagerSaveProfile(settings.profileName, settings);
    settingsStoreSaveLastProfileName(settings.profileName);
  }

  handleStatus();

} //   handleApplySettings()

//--- Return profile list
static void handleProfiles()
{
  String names[profileManagerMaxProfiles];
  size_t count = profileManagerListProfiles(names, profileManagerMaxProfiles);

  JsonDocument doc;
  JsonArray array = doc["profiles"].to<JsonArray>();
  JsonArray deletableArray = doc["deletableProfiles"].to<JsonArray>();

  for (size_t i = 0; i < count; i++)
  {
    array.add(names[i]);

    //--- Only add to deletable list if it's not a default profile
    if (!names[i].equalsIgnoreCase("default") && !names[i].equalsIgnoreCase("default-24h"))
    {
      deletableArray.add(names[i]);
    }
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

} //   handleProfiles()

//--- Save profile from current settings
static void handleSaveProfile()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  AppSettings settings = timerGetSettings();
  String profileName = String(static_cast<const char*>(doc["profileName"] | settings.profileName.c_str()));
  settings.profileName = profileName;

  bool ok = profileManagerSaveProfile(profileName, settings);

  if (ok)
  {
    settingsStoreSaveLastProfileName(profileName);
    timerSetSettings(settings);
  }

  server.send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");

} //   handleSaveProfile()

//--- Load profile into current settings
static void handleLoadProfile()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  AppSettings settings = timerGetSettings();
  String profileName = String(static_cast<const char*>(doc["profileName"] | settings.profileName.c_str()));
  bool ok = profileManagerLoadProfile(profileName, settings);

  if (ok)
  {
    timerSetSettings(settings);
    timerReset();
    settingsStoreSaveLastProfileName(profileName);
  }

  server.send(ok ? 200 : 404, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");

} //   handleLoadProfile()

//--- Delete selected profile
static void handleDeleteProfile()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  AppSettings settings = timerGetSettings();
  String profileName = String(static_cast<const char*>(doc["profileName"] | settings.profileName.c_str()));
  bool ok = profileManagerDeleteProfile(profileName);

  server.send(ok ? 200 : 404, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");

} //   handleDeleteProfile()

//--- Save system settings and optional restart
static void handleSaveSystem()
{
  JsonDocument doc;

  if (!parseJsonBody(doc))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");

    return;
  }

  uint8_t themeColorIndex = static_cast<uint8_t>(doc["themeColorIndex"] | settingsStoreLoadThemeColorIndex());

  if (themeColorIndex >= static_cast<uint8_t>(colorProfileCount))
  {
    themeColorIndex = 0;
  }

  bool outputPolarityHigh = doc["outputPolarityHigh"] | settingsStoreLoadOutputPolarityHigh();
  bool warpSpeedEnabled = doc["warpSpeedEnabled"] | settingsStoreLoadWarpSpeedEnabled();
  bool restartRequested = doc["restart"] | false;
  bool themeChanged = (displayGetThemeColorIndex() != static_cast<int>(themeColorIndex));

  AppSettings settings = timerGetSettings();
  settings.outputPolarityHigh = outputPolarityHigh;
  settings.autoSaveLastProfile = doc["autoSaveLastProfile"] | settings.autoSaveLastProfile;
  settingsStoreSaveSystemSettings(settings);
  timerSetSettings(settings);

  settingsStoreSaveThemeColorIndex(themeColorIndex);
  displaySetThemeColorIndex(themeColorIndex);
  settingsStoreSaveWarpSpeedEnabled(warpSpeedEnabled);

  if (themeChanged)
  {
    uiMenuForceTimerScreen();
  }

  server.send(200, "application/json", "{\"ok\":true}");

  if (restartRequested)
  {
    delay(100);
    ESP.restart();
  }

} //   handleSaveSystem()

//--- Parse JSON body
static bool parseJsonBody(JsonDocument& doc)
{
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  return !error;

} //   parseJsonBody()

//--- Fill status document
static void fillStatusDocument(JsonDocument& doc)
{
  const AppSettings& settings = timerGetSettings();
  RuntimeStatus runtimeStatus = timerGetRuntimeStatus();
  Timer24hStatusInfo status24h = timerGet24hStatusInfo();
  uint8_t themeColorIndex = settingsStoreLoadThemeColorIndex();

  if (themeColorIndex >= static_cast<uint8_t>(colorProfileCount))
  {
    themeColorIndex = 0;
  }

  doc["settings"]["onTimeValue"] = settings.onTimeValue;
  doc["settings"]["offTimeValue"] = settings.offTimeValue;
  doc["settings"]["onTimeUnit"] = static_cast<int>(settings.onTimeUnit);
  doc["settings"]["offTimeUnit"] = static_cast<int>(settings.offTimeUnit);
  doc["settings"]["onTimeUnitLabel"] = timerGetTimeUnitLabel(settings.onTimeUnit);
  doc["settings"]["offTimeUnitLabel"] = timerGetTimeUnitLabel(settings.offTimeUnit);
  doc["settings"]["repeatCount"] = settings.repeatCount;
  doc["settings"]["timerType"] = static_cast<int>(settings.timerType);
  doc["settings"]["timerTypeLabel"] = timerGetTimerTypeLabel(settings.timerType);
  doc["settings"]["triggerMode"] = static_cast<int>(settings.triggerMode);
  doc["settings"]["triggerEdge"] = static_cast<int>(settings.triggerEdge);
  doc["settings"]["outputPolarityHigh"] = settings.outputPolarityHigh;
  doc["settings"]["lockInputDuringRun"] = settings.lockInputDuringRun;
  doc["settings"]["autoSaveLastProfile"] = settings.autoSaveLastProfile;
  doc["settings"]["profileName"] = settings.profileName;
  {
    JsonArray quarterStates = doc["settings"]["timer24hQuarterStates"].to<JsonArray>();

    for (size_t stateIndex = 0; stateIndex < sizeof(settings.timer24hQuarterStates); stateIndex++)
    {
      quarterStates.add(settings.timer24hQuarterStates[stateIndex]);
    }
  }
  doc["settings"]["themeColorIndex"] = themeColorIndex;
  doc["settings"]["themeColorName"] = colorProfiles[themeColorIndex].colorName;
  doc["settings"]["encoderOrderLabel"] = settingsStoreLoadEncoderDirectionReversed() ? "B-A" : "A-B";
  doc["settings"]["warpSpeedEnabled"] = settingsStoreLoadWarpSpeedEnabled();

  doc["runtime"]["state"] = static_cast<int>(runtimeStatus.state);
  doc["runtime"]["stateLabel"] = timerGetStateLabel(runtimeStatus.state);
  doc["runtime"]["outputActive"] = runtimeStatus.outputActive;
  doc["runtime"]["currentCycle"] = runtimeStatus.currentCycle;
  doc["runtime"]["totalCycles"] = runtimeStatus.totalCycles;
  doc["runtime"]["currentPhaseDurationMs"] = runtimeStatus.currentPhaseDurationMs;
  doc["runtime"]["currentPhaseElapsedMs"] = runtimeStatus.currentPhaseElapsedMs;
  doc["runtime"]["inOnPhase"] = runtimeStatus.inOnPhase;
  doc["runtime"]["twentyFourH"]["timeValid"] = status24h.timeValid;
  doc["runtime"]["twentyFourH"]["outputActive"] = status24h.outputActive;
  doc["runtime"]["twentyFourH"]["hasLastOn"] = status24h.hasLastOn;
  doc["runtime"]["twentyFourH"]["hasLastOff"] = status24h.hasLastOff;
  doc["runtime"]["twentyFourH"]["hasNextSwitch"] = status24h.hasNextSwitch;
  doc["runtime"]["twentyFourH"]["hasNextOff"] = status24h.hasNextOff;
  doc["runtime"]["twentyFourH"]["lastOnSeconds"] = status24h.lastOnSecondsOfDay;
  doc["runtime"]["twentyFourH"]["lastOffSeconds"] = status24h.lastOffSecondsOfDay;
  doc["runtime"]["twentyFourH"]["nextSwitchSeconds"] = status24h.nextSwitchSecondsOfDay;
  doc["runtime"]["twentyFourH"]["nextOffSeconds"] = status24h.nextOffSecondsOfDay;
  doc["runtime"]["twentyFourH"]["nextSwitchWindowStartSeconds"] = status24h.nextSwitchWindowStartSecondsOfDay;
  doc["runtime"]["twentyFourH"]["nextSwitchWindowEndSeconds"] = status24h.nextSwitchWindowEndSecondsOfDay;
  doc["runtime"]["twentyFourH"]["nextSwitchInSeconds"] = status24h.nextSwitchInSeconds;
  doc["runtime"]["twentyFourH"]["lastOnLabel"] = status24h.hasLastOn ? formatHhMmSsFromSecondsOfDay(status24h.lastOnSecondsOfDay) : "--:--:--";
  doc["runtime"]["twentyFourH"]["lastOffLabel"] = status24h.hasLastOff ? formatHhMmSsFromSecondsOfDay(status24h.lastOffSecondsOfDay) : "--:--:--";
  doc["runtime"]["twentyFourH"]["nextSwitchLabel"] = status24h.hasNextSwitch ? formatHhMmSsFromSecondsOfDay(status24h.nextSwitchSecondsOfDay) : "--:--:--";
  doc["runtime"]["twentyFourH"]["nextOffLabel"] = status24h.hasNextOff ? formatHhMmSsFromSecondsOfDay(status24h.nextOffSecondsOfDay) : "--:--:--";

  doc["network"]["connected"] = wifiManagerIsStaConnected();
  doc["network"]["address"] = wifiManagerGetAddressString();
  doc["network"]["ssid"] = WiFi.SSID();
  doc["network"]["macAddress"] = WiFi.macAddress();

} //   fillStatusDocument()

//--- Format seconds-of-day as HH:MM:SS
static String formatHhMmSsFromSecondsOfDay(uint32_t secondsOfDay)
{
  uint32_t normalized = secondsOfDay % 86400UL;
  uint32_t hours = normalized / 3600UL;
  uint32_t minutes = (normalized % 3600UL) / 60UL;
  uint32_t seconds = normalized % 60UL;
  char buffer[16];

  snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", static_cast<unsigned long>(hours), static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));

  return String(buffer);

} //   formatHhMmSsFromSecondsOfDay()

//--- Enforce minimum ON/OFF value for ms units
static void enforceMsMinimum(AppSettings& settings)
{
  if (settings.onTimeUnit == TIME_UNIT_MS && settings.onTimeValue < 900)
  {
    settings.onTimeValue = 900;
  }

  if (settings.offTimeUnit == TIME_UNIT_MS && settings.offTimeValue < 900)
  {
    settings.offTimeValue = 900;
  }

} //   enforceMsMinimum()
