//-----------------------------------------------------------------------------
// File: XBInput.cpp
//
// Desc: Input helper functions for the XBox samples
//
// Hist: 12.15.00 - Separated from XBUtil.cpp for December XDK release
//       01.03.01 - Made changes for real Xbox controller
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <xtl.h>
#include "XBInput.h"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

// Deadzone for thumbsticks
#define XBINPUT_DEADZONE 0.24f

// Global instance of XInput polling parameters
XINPUT_POLLING_PARAMETERS g_PollingParameters = 
{
    TRUE,
    TRUE,
    0,
    8,
    8,
    0,
};

// Global instance of input states
XINPUT_STATE g_InputStates[4];

// Global instance of custom gamepad devices
XBGAMEPAD g_Gamepads[4];

//-----------------------------------------------------------------------------
// Name: XBInput_CreateGamepads()
// Desc: Creates the gamepad devices
//-----------------------------------------------------------------------------
HRESULT XBInput_CreateGamepads( XBGAMEPAD** ppGamepads )
{
    // Get a mask of all currently available devices
    DWORD dwDeviceMask = XGetDevices( XDEVICE_TYPE_GAMEPAD );

    // Open the devices
    for( DWORD i=0; i < XGetPortCount(); i++ )
    {
        ZeroMemory( &g_InputStates[i], sizeof(XINPUT_STATE) );
        ZeroMemory( &g_Gamepads[i], sizeof(XBGAMEPAD) );
        if( dwDeviceMask & (1<<i) ) 
        {
            // Get a handle to the device
            g_Gamepads[i].hDevice = XInputOpen( XDEVICE_TYPE_GAMEPAD, i, 
                                                XDEVICE_NO_SLOT, &g_PollingParameters );

            // Store capabilities of the device
            XInputGetCapabilities( g_Gamepads[i].hDevice, &g_Gamepads[i].caps );

            // Initialize last pressed buttons
            XInputGetState( g_Gamepads[i].hDevice, &g_InputStates[i] );

            g_Gamepads[i].wLastButtons = g_InputStates[i].Gamepad.wButtons;

            for( DWORD b=0; b<8; b++ )
            {
                g_Gamepads[i].bLastAnalogButtons[b] =
                    // Turn the 8-bit polled value into a boolean value
                    ( g_InputStates[i].Gamepad.bAnalogButtons[b] > XINPUT_GAMEPAD_MAX_CROSSTALK );
            }
        }
    }

    // Created devices are kept global, but for those who prefer member
    // variables, they can get a pointer to the gamepads returned.
    if( ppGamepads )
        (*ppGamepads) = g_Gamepads;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: XBInput_RefreshDeviceList()
// Desc: Check for device removals and insertions
//-----------------------------------------------------------------------------
VOID XBInput_RefreshDeviceList( XBGAMEPAD* pGamepads )
{
    // If the user did not specify a list of gamepads, use the global list
    if( NULL == pGamepads )
        pGamepads = g_Gamepads;

    // TCR Controller Discovery
    // Get status about gamepad insertions and removals. Note that, in order to
    // not miss devices, we will check for removed device BEFORE checking for
    // insertions
    DWORD dwInsertions, dwRemovals;
    XGetDeviceChanges( XDEVICE_TYPE_GAMEPAD, &dwInsertions, &dwRemovals );

    // Loop through all gamepads
    for( DWORD i=0; i < XGetPortCount(); i++ )
    {
        // Handle removed devices.
        pGamepads[i].bRemoved = ( dwRemovals & (1<<i) ) ? TRUE : FALSE;
        if( pGamepads[i].bRemoved )
        {
            // If the controller was removed after XGetDeviceChanges but before
            // XInputOpen, the device handle will be NULL
            if( pGamepads[i].hDevice )
                XInputClose( pGamepads[i].hDevice );
            pGamepads[i].hDevice = NULL;
            pGamepads[i].Feedback.Rumble.wLeftMotorSpeed  = 0;
            pGamepads[i].Feedback.Rumble.wRightMotorSpeed = 0;
        }

        // Handle inserted devices
        pGamepads[i].bInserted = ( dwInsertions & (1<<i) ) ? TRUE : FALSE;
        if( pGamepads[i].bInserted ) 
        {
            // TCR Device Types
            pGamepads[i].hDevice = XInputOpen( XDEVICE_TYPE_GAMEPAD, i, 
                                               XDEVICE_NO_SLOT, &g_PollingParameters );

            // if the controller is removed after XGetDeviceChanges but before
            // XInputOpen, the device handle will be NULL
            if( pGamepads[i].hDevice )
            {
                XInputGetCapabilities( pGamepads[i].hDevice, &pGamepads[i].caps );

                // Initialize last pressed buttons
                XInputGetState( g_Gamepads[i].hDevice, &g_InputStates[i] );

                g_Gamepads[i].wLastButtons = g_InputStates[i].Gamepad.wButtons;

                for( DWORD b=0; b<8; b++ )
                {
                    g_Gamepads[i].bLastAnalogButtons[b] =
                        // Turn the 8-bit polled value into a boolean value
                        ( g_InputStates[i].Gamepad.bAnalogButtons[b] > XINPUT_GAMEPAD_MAX_CROSSTALK );
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Name: XBInput_GetInput()
// Desc: Processes input from the gamepads
//-----------------------------------------------------------------------------
VOID XBInput_GetInput( XBGAMEPAD* pGamepads )
{
    // If the user did not specify a list of gamepads, use the global list
    if( NULL == pGamepads )
        pGamepads = g_Gamepads;

    // TCR Controller Discovery
    // Get status about gamepad insertions and removals.
    XBInput_RefreshDeviceList( pGamepads );
    
    // Loop through all gamepads
    for( DWORD i=0; i < XGetPortCount(); i++ )
    {
        // If we have a valid device, poll it's state and track button changes
        if( pGamepads[i].hDevice )
        {
            // Read the input state
            XInputGetState( pGamepads[i].hDevice, &g_InputStates[i] );

            // Copy gamepad to local structure
            memcpy( &pGamepads[i], &g_InputStates[i].Gamepad, sizeof(XINPUT_GAMEPAD) );

            // Put Xbox device input for the gamepad into our custom format
            FLOAT fX1 = (pGamepads[i].sThumbLX+0.5f)/32767.5f;
            pGamepads[i].fX1 = ( fX1 >= 0.0f ? 1.0f : -1.0f ) *
                               max( 0.0f, (fabsf(fX1)-XBINPUT_DEADZONE)/(1.0f-XBINPUT_DEADZONE) );

            FLOAT fY1 = (pGamepads[i].sThumbLY+0.5f)/32767.5f;
            pGamepads[i].fY1 = ( fY1 >= 0.0f ? 1.0f : -1.0f ) *
                               max( 0.0f, (fabsf(fY1)-XBINPUT_DEADZONE)/(1.0f-XBINPUT_DEADZONE) );

            FLOAT fX2 = (pGamepads[i].sThumbRX+0.5f)/32767.5f;
            pGamepads[i].fX2 = ( fX2 >= 0.0f ? 1.0f : -1.0f ) *
                               max( 0.0f, (fabsf(fX2)-XBINPUT_DEADZONE)/(1.0f-XBINPUT_DEADZONE) );

            FLOAT fY2 = (pGamepads[i].sThumbRY+0.5f)/32767.5f;
            pGamepads[i].fY2 = ( fY2 >= 0.0f ? 1.0f : -1.0f ) *
                               max( 0.0f, (fabsf(fY2)-XBINPUT_DEADZONE)/(1.0f-XBINPUT_DEADZONE) );

            // Get the boolean buttons that have been pressed since the last
            // call. Each button is represented by one bit.
            pGamepads[i].wPressedButtons = ( pGamepads[i].wLastButtons ^ pGamepads[i].wButtons ) & pGamepads[i].wButtons;
            pGamepads[i].wLastButtons    = pGamepads[i].wButtons;

            BOOL bAnyButtonPressed = pGamepads[i].wPressedButtons != 0 ? TRUE : FALSE;

            // Get the analog buttons that have been pressed or released since
            // the last call.
            for( DWORD b=0; b<8; b++ )
            {
                // Turn the 8-bit polled value into a boolean value
                BOOL bPressed = ( pGamepads[i].bAnalogButtons[b] > XINPUT_GAMEPAD_MAX_CROSSTALK );

                if( bPressed )
                    pGamepads[i].bPressedAnalogButtons[b] = !pGamepads[i].bLastAnalogButtons[b];
                else
                    pGamepads[i].bPressedAnalogButtons[b] = FALSE;
                
                // Store the current state for the next time
                pGamepads[i].bLastAnalogButtons[b] = bPressed;

                bAnyButtonPressed |= bPressed;
            }

            // Cook the event
            pGamepads[i].Event = XBGAMEPAD_NONE;

            if( bAnyButtonPressed )
            {
                if( pGamepads[i].wPressedButtons & XINPUT_GAMEPAD_START )
                    pGamepads[i].Event = XBGAMEPAD_START;
                else if( pGamepads[i].wPressedButtons & XINPUT_GAMEPAD_BACK )
                    pGamepads[i].Event = XBGAMEPAD_BACK;
                else if( pGamepads[i].bPressedAnalogButtons[XINPUT_GAMEPAD_A] )
                    pGamepads[i].Event = XBGAMEPAD_A;
                else if( pGamepads[i].bPressedAnalogButtons[XINPUT_GAMEPAD_B] )
                    pGamepads[i].Event = XBGAMEPAD_B;
                else if( pGamepads[i].bPressedAnalogButtons[XINPUT_GAMEPAD_X] )
                    pGamepads[i].Event = XBGAMEPAD_X;
                else if( pGamepads[i].bPressedAnalogButtons[XINPUT_GAMEPAD_Y] )
                    pGamepads[i].Event = XBGAMEPAD_Y;
                else if( pGamepads[i].bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK] )
                    pGamepads[i].Event = XBGAMEPAD_BLACK;
                else if( pGamepads[i].bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE] )
                    pGamepads[i].Event = XBGAMEPAD_WHITE;
                else if( pGamepads[i].wPressedButtons & XINPUT_GAMEPAD_DPAD_UP )
                    pGamepads[i].Event = XBGAMEPAD_DPAD_UP;
                else if( pGamepads[i].wPressedButtons & XINPUT_GAMEPAD_DPAD_DOWN )
                    pGamepads[i].Event = XBGAMEPAD_DPAD_DOWN;
                else if( pGamepads[i].wPressedButtons & XINPUT_GAMEPAD_DPAD_LEFT )
                    pGamepads[i].Event = XBGAMEPAD_DPAD_LEFT;
                else if( pGamepads[i].wPressedButtons & XINPUT_GAMEPAD_DPAD_RIGHT )
                    pGamepads[i].Event = XBGAMEPAD_DPAD_RIGHT;
                else if( pGamepads[i].bPressedAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] )
                    pGamepads[i].Event = XBGAMEPAD_LEFT_TRIGGER;
                else if( pGamepads[i].bPressedAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] )
                    pGamepads[i].Event = XBGAMEPAD_RIGHT_TRIGGER;
            }
        }
    }
}