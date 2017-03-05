#include <xtl.h>
#include <stdio.h>

#include "XBInput.h"
#include "SysInputXbox.h"

// Deadzone for the gamepad inputs
const SHORT XINPUT_DEADZONE = (SHORT)( 0.24f * FLOAT(0x7FFF) );

class cSysInputXbox
{
public:
	cSysInputXbox();
	~cSysInputXbox();
	
	bool UpdateGamepad(XBGamepadInfo_t *pGamepadInfo);
	bool GetButtons(XBGamepadInfo_t *pGamepadInfo);
private:
    // Members to init the XINPUT devices.
    XDEVICE_PREALLOC_TYPE* m_InputDeviceTypes;
    DWORD                  m_dwNumInputDeviceTypes;
    XBGAMEPAD*             m_Gamepad;
    XBGAMEPAD              m_DefaultGamepad;

	int m_ilastButton;
	int m_iKeyCount;
};

//======================================

cSysInputXbox g_SysInputXbox;

cSysInputXbox::cSysInputXbox()
{
	XInitDevices( m_dwNumInputDeviceTypes, m_InputDeviceTypes );
	XBInput_CreateGamepads( &m_Gamepad );

	m_ilastButton = 0;
	m_iKeyCount = 0;
}

cSysInputXbox::~cSysInputXbox()
{
	m_ilastButton = 0;
	m_iKeyCount = 0;
}

bool cSysInputXbox::UpdateGamepad(XBGamepadInfo_t *pGamepadInfo)
{
	// Read the input for all connected gamepads
	XBInput_GetInput( m_Gamepad );
	
	// Lump inputs of all connected gamepads into one common structure.
	// This is done so apps that need only one gamepad can function with
	// any gamepad.
    
	ZeroMemory( &m_DefaultGamepad, sizeof(m_DefaultGamepad) );
	INT nThumbLX = 0;
	INT nThumbLY = 0;
	INT nThumbRX = 0;
	INT nThumbRY = 0;

	for( DWORD i=0; i<4; i++ )
	{
		if( m_Gamepad[i].hDevice )
		{
			// Only account for thumbstick info beyond the deadzone
            if( m_Gamepad[i].sThumbLX > XINPUT_DEADZONE ||
                m_Gamepad[i].sThumbLX < -XINPUT_DEADZONE )
                nThumbLX += m_Gamepad[i].sThumbLX;
			if( m_Gamepad[i].sThumbLY > XINPUT_DEADZONE ||
                m_Gamepad[i].sThumbLY < -XINPUT_DEADZONE )
                nThumbLY += m_Gamepad[i].sThumbLY;
            if( m_Gamepad[i].sThumbRX > XINPUT_DEADZONE ||
                m_Gamepad[i].sThumbRX < -XINPUT_DEADZONE )
                nThumbRX += m_Gamepad[i].sThumbRX;
            if( m_Gamepad[i].sThumbRY > XINPUT_DEADZONE ||
                m_Gamepad[i].sThumbRY < -XINPUT_DEADZONE )
                nThumbRY += m_Gamepad[i].sThumbRY;

			m_DefaultGamepad.fX1 += m_Gamepad[i].fX1;
			m_DefaultGamepad.fY1 += m_Gamepad[i].fY1;
			m_DefaultGamepad.fX2 += m_Gamepad[i].fX2;
			m_DefaultGamepad.fY2 += m_Gamepad[i].fY2;
			m_DefaultGamepad.wButtons        |= m_Gamepad[i].wButtons;
			m_DefaultGamepad.wPressedButtons |= m_Gamepad[i].wPressedButtons;
			m_DefaultGamepad.wLastButtons    |= m_Gamepad[i].wLastButtons;

			if(/*!*/m_Gamepad[i].Event)
			{
			//return false;
			//} else {
				m_DefaultGamepad.Event = m_Gamepad[i].Event;
			}

			for( DWORD b=0; b<8; b++ )
			{
				m_DefaultGamepad.bAnalogButtons[b]        |= m_Gamepad[i].bAnalogButtons[b];
				m_DefaultGamepad.bPressedAnalogButtons[b] |= m_Gamepad[i].bPressedAnalogButtons[b];
				m_DefaultGamepad.bLastAnalogButtons[b]    |= m_Gamepad[i].bLastAnalogButtons[b];
			}
		}
	}

	// Clamp summed thumbstick values to proper range
	pGamepadInfo->fThumbLX = m_DefaultGamepad.sThumbLX = (SHORT)nThumbLX;
	pGamepadInfo->fThumbLY = m_DefaultGamepad.sThumbLY = (SHORT)nThumbLY;
	pGamepadInfo->fThumbRX = m_DefaultGamepad.sThumbRX = (SHORT)nThumbRX;
	pGamepadInfo->fThumbRY = m_DefaultGamepad.sThumbRY = (SHORT)nThumbRY;

	GetButtons(pGamepadInfo);

	return true;
}

bool cSysInputXbox::GetButtons(XBGamepadInfo_t *pGamepadInfo)
{
	// Button combo to return to the Xbox Dashboard
	if( m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > 0 )
	{
		if( m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > 0 )
        {
			if( m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK] )
            {
				pGamepadInfo->iKey = m_ilastButton = K_XBOX_QCOMBO;
				pGamepadInfo->iButtonDown = 1;
				m_iKeyCount = 0;
				return true;
			}
		}
    }

	if(m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_A])
	{
		pGamepadInfo->iKey = m_ilastButton = K_XBOX_A;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}
	
	if(m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_B])
	{
		pGamepadInfo->iKey = m_ilastButton = K_XBOX_B;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}

	if(m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_X])
	{
		pGamepadInfo->iKey = m_ilastButton = K_XBOX_X;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}

	if(m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_Y])
	{
		pGamepadInfo->iKey = m_ilastButton = K_XBOX_Y;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}


	if(m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
	{
		pGamepadInfo->iKey = m_ilastButton = K_XBOX_BLACK;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}

	if(m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE])
	{
		pGamepadInfo->iKey = m_ilastButton = K_XBOX_WHITE;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}


	if(m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER])
	{
		pGamepadInfo->iKey = m_ilastButton = K_XBOX_RTRIG;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}

	if(m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER])
	{
		pGamepadInfo->iKey = m_ilastButton = K_XBOX_LTRIG;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}

	if(m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP )
	{
		pGamepadInfo->iKey = m_ilastButton = K_DPAD_UP;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}	
	  
	if(m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN )
	{
		pGamepadInfo->iKey = m_ilastButton = K_DPAD_DOWN;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}

	if(m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT )
	{
		pGamepadInfo->iKey = m_ilastButton = K_DPAD_RIGHT;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}

	if(m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT )
	{
		pGamepadInfo->iKey = m_ilastButton = K_DPAD_LEFT;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}

	if(m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_START )
	{
		pGamepadInfo->iKey = m_ilastButton = K_XBOX_START;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}

	if(m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_BACK )
	{
		pGamepadInfo->iKey = m_ilastButton = K_XBOX_BACK;
		pGamepadInfo->iButtonDown = 1;
		m_iKeyCount = 0;
		return true;
	}

	if(!(m_iKeyCount > 1))
	{
		pGamepadInfo->iKey = m_ilastButton;
		pGamepadInfo->iButtonDown = 0;
		
		m_iKeyCount++;
		return true;
	} 
	else
	{
		pGamepadInfo->iKey = 0;
		pGamepadInfo->iButtonDown = 0;
		return true;	
	}

	return false;
}

//======================================

int UpdateGamepad(XBGamepadInfo_t *pGamepadInfo)
{
	return g_SysInputXbox.UpdateGamepad(pGamepadInfo);
}