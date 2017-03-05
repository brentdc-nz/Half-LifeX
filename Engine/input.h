/*
input.h - win32 input devices
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef INPUT_H
#define INPUT_H

/*
==============================================================

INPUT

==============================================================
*/

#include "keydefs.h"

//MARTY FIXME WIP - Mouse is disabled atm, may re-enabled for XBox in future
#define WM_MOUSEWHEEL	( WM_MOUSELAST + 1 ) // message that will be supported by the OS
#define MK_XBUTTON1		0x0020
#define MK_XBUTTON2		0x0040
#define MK_XBUTTON3		0x0080
#define MK_XBUTTON4		0x0100
#define MK_XBUTTON5		0x0200
#define WM_XBUTTONUP	0x020C
#define WM_XBUTTONDOWN	0x020B

//
// input.c
//

// MARTY FIXME WIP
// IN_WndProc is used to send keyboard and mouse events.
// Not required atm as I'm doing this in the client lib.

void IN_Init( void );
void Host_InputFrame( void );
void IN_Shutdown( void );
void IN_MouseEvent( int mstate );
void IN_ActivateMouse( qboolean force );
void IN_DeactivateMouse( void );
void IN_ToggleClientMouse( int newstate, int oldstate );
long IN_WndProc( void *hWnd, uint uMsg, uint wParam, long lParam );
void IN_SetCursor( HICON hCursor );

#endif//INPUT_H