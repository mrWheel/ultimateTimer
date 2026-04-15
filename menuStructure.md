Selection rule:
- Selection of every menu item is always done with a SHORT press on the Rotary Encoder.

Menu hierarchy:
```
[Start]
|
[Trying to connect to]
[<SSID>] (centered)
|
V
[Timer Screen]
- Start (SHORT press when selected)
- Stop (SHORT press when selected)
- Reset (SHORT press when selected)
|
"Encoder Long Press"
|
V
[Edit Timer Menu]
- Timer Settings -> [Timer Settings Menu]
- Save Profile -> [Save Profile Menu]
- Load Profile -> [Load Profile Menu]
- New Profile -> [New Profile Menu]
- Delete Profile -> [Delete Profile Menu]
- Show System Settings -> [Show System Settings Menu]
- Exit -> return to [Timer Screen]

[Timer Settings Menu]
- On Time
- On Time Unit
- Off Time
- Off Time Unit
- Number of Cycles
- Trigger (Rise/Fall)
- Exit -> return to [Edit Timer Menu]

[Save Profile Menu]
- Profile field input (alphanumeric, fixed positions)
- Auto return to [Edit Timer Menu] after last position is confirmed

[Load Profile Menu]
- Profile list
- Exit (last item) -> return to [Edit Timer Menu]

[New Profile Menu]
- Profile field input (alphanumeric, fixed positions)
- Auto return to [Edit Timer Menu] after last position is confirmed

[Delete Profile Menu]
- Profile list
- Exit (last item) -> return to [Edit Timer Menu]

[Show System Settings Menu]
- WiFi SSID (RO, only if WiFi enabled)
- IP Address (RO, only if WiFi enabled)
- MAC Address (RO)
- WiFi Disabled: Yes (RO, only if WiFi disabled)
- Encoder Order (A-B / B-A)
- Erase WiFi credentials (Are you sure (Y/N))
- Start WiFi Manager (clears WiFi Disabled indicator and restarts)
- Output Polarity (Active High / Active Low)
- Restart ultimateTimer
- Exit -> return to [Edit Timer Menu]

[WiFi Manager Started]
- Connect to AP (centered, wrapped if needed)
- AP name (centered, wrapped if needed)
- [Cancel WiFi Manager]
- Cancel WiFi Manager -> sets WiFi Disabled/Ignore flag and returns to [Timer Screen] without restart
```
======= Field Input =======

Field input behavior:
- Start at the left-most position.
- Rotating the encoder changes the token at the current cursor position.
- SHORT press confirms the current position and moves to the next position.
- SHORT press at the last position finalizes input and automatically returns to the previous menu.
- MEDIUM press shift 1 to the left (if possible)

Generic field input parameters:
- fieldName
- positionCount
- tokenList

Token lists:
- Numeric: 1,2,3,4,5,6,7,8,9,0
- Alphanumeric: A,a,B,b,C,c,...,Y,y,Z,z,-,1,2,3,4,5,6,7,8,9,0
- Special: ms, s, Min

Status output countdown:
- Format is MMM:SS while running/paused.
- Idle placeholder is ---:--.
