//-----------------------------------------------------------------------------
// File: XBInput.h
//
// Desc: Input helper functions for the XBox samples
//
// Hist: 12.15.00 - Separated from XBUtil.h for December XDK release
//       01.03.00 - Made changes for real Xbox controller
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef XBINPUT_H
#define XBINPUT_H




//-----------------------------------------------------------------------------
// Name: enum XBGAMEPAD_EVENT
// Desc: Cooked event codes for gamepad button presses
//-----------------------------------------------------------------------------
enum XBGAMEPAD_EVENT
{
    XBGAMEPAD_NONE = 0,
    XBGAMEPAD_START,
    XBGAMEPAD_BACK,
    XBGAMEPAD_A,
    XBGAMEPAD_B,
    XBGAMEPAD_X,
    XBGAMEPAD_Y,
    XBGAMEPAD_BLACK,
    XBGAMEPAD_WHITE,
    XBGAMEPAD_DPAD_UP,
    XBGAMEPAD_DPAD_DOWN,
    XBGAMEPAD_DPAD_LEFT,
    XBGAMEPAD_DPAD_RIGHT,
    XBGAMEPAD_LEFT_TRIGGER,
    XBGAMEPAD_RIGHT_TRIGGER,
};




//-----------------------------------------------------------------------------
// Name: struct XBGAMEPAD
// Desc: structure for holding Game pad data
//-----------------------------------------------------------------------------
struct XBGAMEPAD : public XINPUT_GAMEPAD
{
    // The following members are inherited from XINPUT_GAMEPAD:
    //    WORD      wButtons;
    //    BYTE      bAnalogButtons[8];
    //    SHORT     sThumbLX;
    //    SHORT     sThumbLY;
    //    SHORT     sThumbRX;
    //    SHORT     sThumbRY;

    // Thumb stick values converted to range [-1,+1]
    FLOAT           fX1;
    FLOAT           fY1;
    FLOAT           fX2;
    FLOAT           fY2;
    
    // State of buttons tracked since last poll
    WORD            wLastButtons;
    BOOL            bLastAnalogButtons[8];
    WORD            wPressedButtons;
    BOOL            bPressedAnalogButtons[8];

    // A cooked event, useful for UI navigation
    XBGAMEPAD_EVENT Event;

    // Rumble properties
    XINPUT_RUMBLE   Rumble;
    XINPUT_FEEDBACK Feedback;

    // Device properties
    XINPUT_CAPABILITIES caps;
    HANDLE          hDevice;

    // Flags for whether game pad was just inserted or removed
    BOOL            bInserted;
    BOOL            bRemoved;
};




//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
// Global access to polling parameters
extern XINPUT_POLLING_PARAMETERS g_PollingParameters;

// Global access to input states
extern XINPUT_STATE g_InputStates[4];

// Global access to gamepad devices
extern XBGAMEPAD g_Gamepads[4];




//-----------------------------------------------------------------------------
// Name: XBInput_CreateGamepads()
// Desc: Creates the game pad devices
//-----------------------------------------------------------------------------
HRESULT XBInput_CreateGamepads( XBGAMEPAD** ppGamepads = NULL );




//-----------------------------------------------------------------------------
// Name: XBInput_RefreshDeviceList()
// Desc: Check for device removals and insertions
//-----------------------------------------------------------------------------
VOID XBInput_RefreshDeviceList( XBGAMEPAD* pGamepads = NULL );




//-----------------------------------------------------------------------------
// Name: XBInput_GetInput()
// Desc: Processes input from the game pad
//-----------------------------------------------------------------------------
VOID XBInput_GetInput( XBGAMEPAD* pGamepads = NULL );




#endif // XBINPUT_H