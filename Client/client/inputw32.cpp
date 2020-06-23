/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "hud.h"
#include "utils.h"
#include "..\common\cvardef.h"
#include "..\common\usercmd.h"
#include "..\common\const.h"
#include "..\common\kbutton.h"
#include "render\r_view.h"
#include "..\engine\keydefs.h"
#include "..\game_shared\mathlib.h"

#ifdef _XBOX //MARTY
#include <xtl.h>
#include ".\XBox\SysInputXbox.h"
#endif

extern kbutton_t in_forward; //MARTY
extern kbutton_t in_back; //MARTY

// in_win.c -- windows 95 mouse code

#define MOUSE_BUTTON_COUNT	5

extern kbutton_t	in_strafe;
extern kbutton_t	in_mlook;
extern kbutton_t	in_speed;
extern kbutton_t	in_jlook;

extern cvar_t	*m_pitch;
extern cvar_t	*m_yaw;
extern cvar_t	*m_forward;
extern cvar_t	*m_side;
extern cvar_t	*lookstrafe;
extern cvar_t	*lookspring;
extern cvar_t	*cl_pitchdown;
extern cvar_t	*cl_pitchup;
extern cvar_t	*cl_yawspeed;
extern cvar_t	*cl_sidespeed;
extern cvar_t	*cl_forwardspeed;
extern cvar_t	*cl_pitchspeed;
extern cvar_t	*cl_movespeedkey;

#ifdef _XBOX //MARTY
extern cvar_t	*cl_invert_look;
extern cvar_t	*cl_rthumb_sensitivity;
#endif

// mouse variables
cvar_t		*m_filter;
cvar_t		*sensitivity;

int		mouse_buttons;
int		mouse_oldbuttonstate;
POINT		current_pos;
int		mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;

static int	restore_spi;
static int	originalmouseparms[3], newmouseparms[3] = { 0, 0, 1 };
static int	mouseactive;
static int	mouseinitialized;
static int	mouseparmsvalid;

#ifdef _XBOX //MARTY
static XBGamepadInfo_t g_gamepadInfo;
#endif

/*
===========
Force_CenterView_f
===========
*/
void Force_CenterView_f( void )
{
	Vector viewangles;

	gEngfuncs.GetViewAngles( viewangles );
	viewangles[PITCH] = 0;

	float rgfl[3];
	viewangles.CopyToArray( rgfl );
	gEngfuncs.SetViewAngles( rgfl );
}

/*
===========
IN_ActivateMouse
===========
*/
void StaticIN_ActivateMouse( void )
{
	if( mouseinitialized )
	{
#ifdef _WIN32_
        if (mouseparmsvalid)
            restore_spi = SystemParametersInfo (SPI_SETMOUSE, 0, newmouseparms, 0);

#endif
        mouseactive = 1;
	}
}

/*
===========
IN_DeactivateMouse
===========
*/
void StaticIN_DeactivateMouse( void )
{
	if( mouseinitialized )
	{
#ifdef _WIN32_
        if (restore_spi)
            SystemParametersInfo (SPI_SETMOUSE, 0, originalmouseparms, 0);

#endif
		mouseactive = 0;
	}
}

/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse( void )
{
	if( gEngfuncs.CheckParm( "-nomouse", NULL )) 
		return; 

	mouseinitialized = 1;
	#ifdef _WIN32_
	mouseparmsvalid = SystemParametersInfo( SPI_GETMOUSE, 0, originalmouseparms, 0 );

	if( mouseparmsvalid )
	{
		if( gEngfuncs.CheckParm( "-noforcemspd", NULL ))
		{ 
			newmouseparms[2] = originalmouseparms[2];
                    }

		if( gEngfuncs.CheckParm( "-noforcemaccel", NULL )) 
		{
			newmouseparms[0] = originalmouseparms[0];
			newmouseparms[1] = originalmouseparms[1];
		}

		if( gEngfuncs.CheckParm( "-noforcemparms", NULL )) 
		{
			newmouseparms[0] = originalmouseparms[0];
			newmouseparms[1] = originalmouseparms[1];
			newmouseparms[2] = originalmouseparms[2];
		}
	}
#endif
	mouse_buttons = MOUSE_BUTTON_COUNT;
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown( void )
{
	StaticIN_DeactivateMouse();
}

/*
===========
IN_GetMousePos

Ask for mouse position from engine
===========
*/
void IN_GetMousePos( int *mx, int *my )
{
	gEngfuncs.GetMousePosition( mx, my );
}

/*
===========
IN_ResetMouse

Reset mouse position from engine
===========
*/
void IN_ResetMouse( void )
{
#ifdef _WIN32_
	SetCursorPos( gEngfuncs.GetWindowCenterX(), gEngfuncs.GetWindowCenterY() );	
 #endif
}

/*
===========
IN_MouseEvent
===========
*/
void StaticIN_MouseEvent( int mstate )
{
	// perform button actions
	for( int i = 0; i < mouse_buttons; i++ )
	{
		if(( mstate & BIT( i )) && !( mouse_oldbuttonstate & BIT( i )))
		{
			gEngfuncs.Key_Event( K_MOUSE1 + i, 1 );
		}

		if( !( mstate & BIT( i )) && ( mouse_oldbuttonstate & BIT( i )))
		{
			gEngfuncs.Key_Event( K_MOUSE1 + i, 0 );
		}
	}	
	mouse_oldbuttonstate = mstate;
}

//MARTY START
#ifdef _XBOX 
/*
===================
IN_UpdateThumbStickAxis
===================
*/
void IN_UpdateThumbStickAxes(float frametime, usercmd_t *cmd, float rawvalue, int iAxis) //MARTY
{
	float value;
	float svalue;
	float speed, aspeed;
	vec3_t viewangles;

	gEngfuncs.GetViewAngles( (float *)viewangles );

	if(iAxis && cl_invert_look->value > 0)
	rawvalue -= rawvalue *2;

	// Convert value
	value = (rawvalue / 128.0);
	svalue = value * cl_rthumb_sensitivity->value; //3.5;

	// Handle +speed
	if (in_speed.state & 1)
	{
		speed = cl_movespeedkey->value; //2.0;
	}
	else
	{
		speed = 1;
		aspeed = speed * frametime;
	}

	if(fabs(value) > 0.15)
	{
		if (cl_invert_look->value > 0)
		{
			viewangles[iAxis] += svalue * aspeed * 0.15;
		}
		else
		{
			viewangles[iAxis] -= svalue * aspeed * 0.15;
		}
	}
//	else if(lookspring->value == 0.0)
//	{
//		V_StopPitchDrift();
//	}

	// bounds check pitch
	if (viewangles[PITCH] > 80.0)
		viewangles[PITCH] = 80.0;

	if (viewangles[PITCH] < -70.0)
		viewangles[PITCH] = -70.0;

	gEngfuncs.SetViewAngles( (float *)viewangles );
}

/*
===========
IN_XBoxGamepadThumbs
===========
*/
void IN_XBoxGamepadThumbs ( float frametime, usercmd_t *cmd ) //MARTY
{
	//Left thumb stick
	if(g_gamepadInfo.fThumbLY > 0)
	{
		in_forward.state |= 1;
	}
	else if(g_gamepadInfo.fThumbLY < 0)
	{
		in_back.state |= 1;
	}
	else if (in_forward.state & 1)
		in_forward.state &= ~1;
	else if (in_back.state & 1)
		in_back.state &= ~1;

	cmd->forwardmove = g_gamepadInfo.fThumbLY;
	cmd->sidemove = g_gamepadInfo.fThumbLX;

	//Right thumb stick
	IN_UpdateThumbStickAxes(frametime, cmd, g_gamepadInfo.fThumbRY, PITCH);
	IN_UpdateThumbStickAxes(frametime, cmd, g_gamepadInfo.fThumbRX, YAW);
}

/*
===========
IN_XBoxGamepad
===========
*/
int StaticIN_XBoxGamepadButtons ( XBGamepadButtons_t *pGamepadButtons, int key )
{
	//XBGamepadInfo_t gamepadInfo; // MARTY - Made global
	int ret = UpdateGamepad(&g_gamepadInfo, key);
	
	// Put the button pointers into a button only struct
	if(pGamepadButtons)
	{
		pGamepadButtons->iKey = g_gamepadInfo.iKey;
		pGamepadButtons->iButtonDown = g_gamepadInfo.iButtonDown;
		return ret;
	}
	return ret;
}
#endif //_XBOX
//MARTY END

/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove( float frametime, usercmd_t *cmd )
{
#ifndef _XBOX //MARTY - No mouse support atm
	int mx, my;
	Vector viewangles;

	if( !mouseactive ) return;

	gEngfuncs.GetViewAngles( viewangles );

	if( in_mlook.state & 1 )
	{
		V_StopPitchDrift();
	}

	// jjb - this disbles normal mouse control if the user is trying to 
	// move the camera, or if the mouse cursor is visible or if we're in intermission
	if( !gHUD.m_iIntermission )
	{
#ifdef _WIN32_
		GetCursorPos( &current_pos );

        mx = current_pos.x - gEngfuncs.GetWindowCenterX() + mx_accum;
        my = current_pos.y - gEngfuncs.GetWindowCenterY() + my_accum;
#else
        int deltaX, deltaY;
		deltaX = deltaY = 0; //MARTY  
//      SDL_GetRelativeMouseState( &deltaX, &deltaY ); //MARTY
                   current_pos.x = deltaX;
                   current_pos.y = deltaY;

                   mx = current_pos.x + mx_accum;
                   my = current_pos.y + my_accum;

#endif


		mx_accum = 0;
		my_accum = 0;

		if( m_filter->value )
		{
			mouse_x = (mx + old_mouse_x) * 0.5f;
			mouse_y = (my + old_mouse_y) * 0.5f;
		}
		else
		{
			mouse_x = mx;
			mouse_y = my;
		}

		old_mouse_x = mx;
		old_mouse_y = my;

		if( gHUD.GetSensitivity() != 0 )
		{
			mouse_x *= gHUD.GetSensitivity();
			mouse_y *= gHUD.GetSensitivity();
		}
		else
		{
			mouse_x *= sensitivity->value;
			mouse_y *= sensitivity->value;
		}

		// add mouse X/Y movement to cmd
		if(( in_strafe.state & 1 ) || ( lookstrafe->value && ( in_mlook.state & 1 )))
			cmd->sidemove += m_side->value * mouse_x;
		else
			viewangles[YAW] -= m_yaw->value * mouse_x;

		if(( in_mlook.state & 1 ) && !( in_strafe.state & 1 ))
		{
			viewangles[PITCH] += m_pitch->value * mouse_y;
			viewangles[PITCH] = bound( -cl_pitchup->value, viewangles[PITCH], cl_pitchdown->value );
		}
		else
		{
			if(( in_strafe.state & 1 ) && gEngfuncs.IsNoClipping( ))
			{
				cmd->upmove -= m_forward->value * mouse_y;
			}
			else
			{
				cmd->forwardmove -= m_forward->value * mouse_y;
			}
		}

		// if the mouse has moved, force it to the center, so there's room to move
		if( mx || my )
		{
			IN_ResetMouse();
		}
	}

	float rgfl[3];
	viewangles.CopyToArray( rgfl );
	gEngfuncs.SetViewAngles( rgfl );
#endif
}

/*
===========
IN_Accumulate
===========
*/
void StaticIN_Accumulate( void ) // TODO
{
/*	if( mouseactive )
	{
#ifdef _WIN32
		GetCursorPos( &current_pos );

        mx_accum += current_pos.x - gEngfuncs.GetWindowCenterX();
        my_accum += current_pos.y - gEngfuncs.GetWindowCenterY();
#else
        int deltaX, deltaY;
        SDL_GetRelativeMouseState( &deltaX, &deltaY );

        mx_accum += deltaX;
        my_accum += deltaY;
#endif
		// force the mouse to the center, so there's room to move
		IN_ResetMouse();
	}*/
}

/*
===================
IN_ClearStates
===================
*/
void StaticIN_ClearStates( void )
{
	if( !mouseactive ) return;

	mx_accum = 0;
	my_accum = 0;
	mouse_oldbuttonstate = 0;
}

/*
===========
IN_Move
===========
*/
void IN_Move( float frametime, usercmd_t *cmd )
{
	//IN_MouseMove( frametime, cmd );

#ifdef _XBOX //MARTY
	IN_XBoxGamepadThumbs( frametime, cmd);
#endif
}

/*
===========
IN_Init
===========
*/
void IN_Init( void )
{
	m_filter	= CVAR_REGISTER ( "m_filter","0", FCVAR_ARCHIVE );
	sensitivity	= CVAR_REGISTER ( "sensitivity","3", FCVAR_ARCHIVE ); // user mouse sensitivity setting.

	ADD_COMMAND ("force_centerview", Force_CenterView_f);
	IN_StartupMouse ();
}
