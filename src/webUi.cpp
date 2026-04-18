/*** Last Changed: 2026-04-18 - 15:49 ***/
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
}
</style>
</head>
<body>
<header class="menuBar">
  <nav class="menuLeft" aria-label="Main menu">
    <button class="menuButton" data-menu="systemCard">System</button>
    <button class="menuButton" data-menu="timerSettingsCard">Timer Settings</button>
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
    <div>Profile: <span id="headerProfileName" class="headerValue">-</span></div>
    <div class="clockValue"><span id="headerClock" class="headerValue">00:00:00</span></div>
  </div>
</header>

<main class="appShell">
  <section class="tile">
    <div class="tileHeader">
      <div class="tileTitle">Timer Screen</div>
      <div class="metaLine" id="networkStatus">Network: Loading...</div>
    </div>
    <div class="tileBody">
      <div class="statusGrid">
        <div class="statusItem">
          <div class="statusLabel">State</div>
          <div id="statusState" class="statusValue">-</div>
        </div>
        <div class="statusItem">
          <div class="statusLabel">On Time</div>
          <div id="statusOn" class="statusValue">-</div>
        </div>
        <div class="statusItem">
          <div class="statusLabel">Off Time</div>
          <div id="statusOff" class="statusValue">-</div>
        </div>
        <div class="statusItem statusWide">
          <div class="statusLabel">Output</div>
          <div id="statusOutput" class="statusValue statusOutputValue">-</div>
        </div>
        <div class="statusItem">
          <div class="statusLabel">Cycles</div>
          <div id="statusCycles" class="statusValue">-</div>
        </div>
      </div>

      <div class="actions">
        <button id="startButton" class="actionButton actionStart" type="button">Start</button>
        <button id="stopButton" class="actionButton actionStop" type="button">Stop</button>
        <button id="resetButton" class="actionButton actionReset" type="button">Reset</button>
      </div>
    </div>
  </section>

  <section id="timerSettingsCard" class="tile menuTile" aria-hidden="true">
    <div class="tileHeader">
      <div class="tileTitle">Timer Settings</div>
    </div>
    <div class="tileBody">
      <p class="tileNotice">Only mutable when Timer is Stopped.</p>
      <div class="formGrid">
        <div class="fieldRow"><label for="onTimeValue">On Time</label><input id="onTimeValue" type="number" min="0"></div>
        <div class="fieldRow"><label for="onTimeUnit">On Time Unit</label><select id="onTimeUnit"><option value="0">ms</option><option value="1">s</option><option value="2">min</option></select></div>
        <div class="fieldRow"><label for="offTimeValue">Off Time</label><input id="offTimeValue" type="number" min="0"></div>
        <div class="fieldRow"><label for="offTimeUnit">Off Time Unit</label><select id="offTimeUnit"><option value="0">ms</option><option value="1">s</option><option value="2">min</option></select></div>
        <div class="fieldRow"><label for="repeatCount">Cycles</label><input id="repeatCount" type="number" min="0"></div>
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
        <div class="fieldRow"><label for="systemRestart">Restart Ultimate Timer</label><select id="systemRestart"><option value="0">No</option><option value="1">Yes</option></select></div>
      </div>
      <div class="metaCard">
        <div class="metaCardLabel">Theme</div>
        <div class="metaCardValue" id="systemThemeName">-</div>
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
      <div class="formGrid">
        <div class="fieldRow"><label for="profileName">Profile Name</label><input id="profileName" type="text" maxlength="16"></div>
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

function updateHeaderClock()
{
  const now = new Date();
  const hours = String(now.getHours()).padStart(2, '0');
  const minutes = String(now.getMinutes()).padStart(2, '0');
  const seconds = String(now.getSeconds()).padStart(2, '0');
  document.getElementById('headerClock').textContent = hours + ':' + minutes + ':' + seconds;
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

function setActiveMenu(menuId)
{
  const menuButtons = document.querySelectorAll('.menuButton');
  const menuTiles = document.querySelectorAll('.menuTile');
  const profileMenuIds = ['loadProfileCard', 'saveProfileCard', 'newProfileCard', 'deleteProfileCard'];

  for (const button of menuButtons)
  {
    const isActive = button.dataset.menu === menuId;
    button.classList.toggle('active', isActive);
  }

  const profileMenuToggle = document.getElementById('profileMenuToggle');

  if (profileMenuToggle)
  {
    profileMenuToggle.classList.toggle('active', profileMenuIds.includes(menuId));
  }

  for (const tile of menuTiles)
  {
    const isActive = tile.id === menuId;
    tile.classList.toggle('active', isActive);
    tile.setAttribute('aria-hidden', isActive ? 'false' : 'true');
  }
}

function closeProfileSubMenu()
{
  const profileSubMenu = document.getElementById('profileSubMenu');

  if (profileSubMenu)
  {
    profileSubMenu.classList.remove('open');
  }
}

function bindMenu()
{
  const menuButtons = document.querySelectorAll('.menuButton');
  const profileMenuToggle = document.getElementById('profileMenuToggle');
  const profileMenuGroup = document.getElementById('profileMenuGroup');

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

        return;
      }

      setActiveMenu(targetMenuId);
      closeProfileSubMenu();
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
    });
  }

  document.addEventListener('click', (event) =>
  {
    if (profileMenuGroup && !profileMenuGroup.contains(event.target))
    {
      closeProfileSubMenu();
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
  const cardIds = ['timerSettingsCard', 'systemCard', 'saveProfileCard', 'loadProfileCard', 'deleteProfileCard', 'newProfileCard'];

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

  const countdown = data.runtime.state === 0
    ? '---:--'
    : formatCountdown(data.runtime.currentPhaseDurationMs, data.runtime.currentPhaseElapsedMs);
  const nextPhase = data.runtime.inOnPhase ? 'OFF' : 'ON';
  const outputLabel = data.runtime.state === 0
    ? 'OFF [---:--]'
    : (data.runtime.outputActive ? 'ON' : 'OFF') + ' [' + countdown + ' to ' + nextPhase + ']';

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

  document.getElementById('statusState').textContent = data.runtime.stateLabel;
  document.getElementById('statusOn').textContent = data.settings.onTimeValue + ' ' + data.settings.onTimeUnitLabel;
  document.getElementById('statusOff').textContent = data.settings.offTimeValue + ' ' + data.settings.offTimeUnitLabel;
  document.getElementById('statusOutput').textContent = outputLabel;
  document.getElementById('statusCycles').textContent =
    data.runtime.totalCycles === 0
      ? String(displayCycle) + '/INF'
      : String(displayCycle) + '/' + String(data.runtime.totalCycles);
  document.getElementById('networkStatus').textContent =
    'Network: ' + (data.network.connected ? 'Connected' : 'Not connected') + ' | IP: ' + data.network.address;
  document.getElementById('systemThemeName').textContent = data.settings.themeColorName || '-';
  document.getElementById('systemNetworkState').textContent = data.network.connected ? 'Connected' : 'Disconnected';
  document.getElementById('systemSsid').value = data.network.ssid || '(not connected)';
  document.getElementById('systemIp').value = data.network.address || '(none)';
  document.getElementById('systemMac').value = data.network.macAddress || '(unknown)';
  document.getElementById('systemEncoderOrder').value = data.settings.encoderOrderLabel || '(unknown)';
  document.getElementById('systemOutputPolarity').value = data.settings.outputPolarityHigh ? '1' : '0';
  document.getElementById('systemAutoSaveLastProfile').value = data.settings.autoSaveLastProfile ? '1' : '0';
  document.getElementById('systemThemeIndex').value = String(data.settings.themeColorIndex || 0);

  document.getElementById('headerProfileName').textContent = data.settings.profileName || '-';
  document.getElementById('activeProfileInSave').textContent = data.settings.profileName || '-';
  document.getElementById('activeProfileInLoad').textContent = data.settings.profileName || '-';
  document.getElementById('activeProfileInDelete').textContent = data.settings.profileName || '-';
  document.getElementById('activeProfileInNew').textContent = data.settings.profileName || '-';
  applyTheme(data.settings.themeColorName || 'Blue');

  document.getElementById('onTimeValue').value = data.settings.onTimeValue;
  document.getElementById('offTimeValue').value = data.settings.offTimeValue;
  document.getElementById('onTimeUnit').value = data.settings.onTimeUnit;
  document.getElementById('offTimeUnit').value = data.settings.offTimeUnit;
  document.getElementById('repeatCount').value = data.settings.repeatCount;
  document.getElementById('triggerMode').value = data.settings.triggerMode;
  document.getElementById('triggerEdge').value = data.settings.triggerEdge;
  document.getElementById('lockInputDuringRun').value = data.settings.lockInputDuringRun ? '1' : '0';
  document.getElementById('profileName').value = data.settings.profileName;
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

  const selects = [
    document.getElementById('profilesForLoad'),
    document.getElementById('profilesForDelete')
  ];
  const selectedValues = selects.map((select) => select ? select.value : '');

  for (const select of selects)
  {
    if (!select)
    {
      continue;
    }

    select.innerHTML = '';
  }

  for (const name of data.profiles)
  {
    for (const select of selects)
    {
      if (!select)
      {
        continue;
      }

      const option = document.createElement('option');
      option.value = name;
      option.textContent = name;
      select.appendChild(option);
    }
  }

  selects.forEach((select, index) =>
  {
    if (select && selectedValues[index])
    {
      select.value = selectedValues[index];
    }
  });

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
    triggerMode: Number(document.getElementById('triggerMode').value),
    triggerEdge: Number(document.getElementById('triggerEdge').value),
    lockInputDuringRun: document.getElementById('lockInputDuringRun').value === '1',
    profileName: document.getElementById('profileName').value
  };
}

function readSystemFromForm()
{
  return {
    outputPolarityHigh: document.getElementById('systemOutputPolarity').value === '1',
    autoSaveLastProfile: document.getElementById('systemAutoSaveLastProfile').value === '1',
    themeColorIndex: Number(document.getElementById('systemThemeIndex').value),
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
    'triggerMode',
    'triggerEdge',
    'lockInputDuringRun',
    'profileName'
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
  await callPost('/api/profile/save', {
    profileName: document.getElementById('profileName').value
  });
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

  document.getElementById('profileName').value = newProfileName;
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
  settings.onTimeValue = doc["onTimeValue"] | settings.onTimeValue;
  settings.offTimeValue = doc["offTimeValue"] | settings.offTimeValue;
  settings.onTimeUnit = static_cast<TimeUnit>(doc["onTimeUnit"] | static_cast<int>(settings.onTimeUnit));
  settings.offTimeUnit = static_cast<TimeUnit>(doc["offTimeUnit"] | static_cast<int>(settings.offTimeUnit));
  settings.repeatCount = static_cast<uint32_t>(doc["repeatCount"] | settings.repeatCount);
  settings.triggerMode = static_cast<TriggerMode>(doc["triggerMode"] | static_cast<int>(settings.triggerMode));
  settings.triggerEdge = static_cast<TriggerEdge>(doc["triggerEdge"] | static_cast<int>(settings.triggerEdge));
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
  settings.onTimeValue = doc["onTimeValue"] | settings.onTimeValue;
  settings.offTimeValue = doc["offTimeValue"] | settings.offTimeValue;
  settings.onTimeUnit = static_cast<TimeUnit>(doc["onTimeUnit"] | static_cast<int>(settings.onTimeUnit));
  settings.offTimeUnit = static_cast<TimeUnit>(doc["offTimeUnit"] | static_cast<int>(settings.offTimeUnit));
  settings.repeatCount = static_cast<uint32_t>(doc["repeatCount"] | settings.repeatCount);
  settings.triggerMode = static_cast<TriggerMode>(doc["triggerMode"] | static_cast<int>(settings.triggerMode));
  settings.triggerEdge = static_cast<TriggerEdge>(doc["triggerEdge"] | static_cast<int>(settings.triggerEdge));
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

  for (size_t i = 0; i < count; i++)
  {
    array.add(names[i]);
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
  bool restartRequested = doc["restart"] | false;
  bool themeChanged = (displayGetThemeColorIndex() != static_cast<int>(themeColorIndex));

  AppSettings settings = timerGetSettings();
  settings.outputPolarityHigh = outputPolarityHigh;
  settings.autoSaveLastProfile = doc["autoSaveLastProfile"] | settings.autoSaveLastProfile;
  settingsStoreSaveSystemSettings(settings);
  timerSetSettings(settings);

  settingsStoreSaveThemeColorIndex(themeColorIndex);
  displaySetThemeColorIndex(themeColorIndex);

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
  doc["settings"]["triggerMode"] = static_cast<int>(settings.triggerMode);
  doc["settings"]["triggerEdge"] = static_cast<int>(settings.triggerEdge);
  doc["settings"]["outputPolarityHigh"] = settings.outputPolarityHigh;
  doc["settings"]["lockInputDuringRun"] = settings.lockInputDuringRun;
  doc["settings"]["autoSaveLastProfile"] = settings.autoSaveLastProfile;
  doc["settings"]["profileName"] = settings.profileName;
  doc["settings"]["themeColorIndex"] = themeColorIndex;
  doc["settings"]["themeColorName"] = colorProfiles[themeColorIndex].colorName;
  doc["settings"]["encoderOrderLabel"] = settingsStoreLoadEncoderDirectionReversed() ? "B-A" : "A-B";

  doc["runtime"]["state"] = static_cast<int>(runtimeStatus.state);
  doc["runtime"]["stateLabel"] = timerGetStateLabel(runtimeStatus.state);
  doc["runtime"]["outputActive"] = runtimeStatus.outputActive;
  doc["runtime"]["currentCycle"] = runtimeStatus.currentCycle;
  doc["runtime"]["totalCycles"] = runtimeStatus.totalCycles;
  doc["runtime"]["currentPhaseDurationMs"] = runtimeStatus.currentPhaseDurationMs;
  doc["runtime"]["currentPhaseElapsedMs"] = runtimeStatus.currentPhaseElapsedMs;
  doc["runtime"]["inOnPhase"] = runtimeStatus.inOnPhase;

  doc["network"]["connected"] = wifiManagerIsStaConnected();
  doc["network"]["address"] = wifiManagerGetAddressString();
  doc["network"]["ssid"] = WiFi.SSID();
  doc["network"]["macAddress"] = WiFi.macAddress();

} //   fillStatusDocument()

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
