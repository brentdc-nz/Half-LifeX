/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

#ifndef KEYDEFS_H
#define KEYDEFS_H

#ifdef _XBOX // Xbox button struct
typedef struct XBGamepadButtons_s
{
	int iKey;
	int iButtonDown;
} XBGamepadButtons_t;
#endif

//
// these are the key numbers that should be passed to Key_Event
//
#define K_TAB		9
#define K_ENTER		13
#define K_ESCAPE		27
#define K_SPACE		32
#define K_SCROLLOCK		70

// normal keys should be passed as lowercased ascii

#define K_BACKSPACE		127
#define K_UPARROW		128
#define K_DOWNARROW		129
#define K_LEFTARROW		130
#define K_RIGHTARROW	131

#define K_ALT		132
#define K_CTRL		133
#define K_SHIFT		134
#define K_F1		135
#define K_F2		136
#define K_F3		137
#define K_F4		138
#define K_F5		139
#define K_F6		140
#define K_F7		141
#define K_F8		142
#define K_F9		143
#define K_F10		144
#define K_F11		145
#define K_F12		146
#define K_INS		147
#define K_DEL		148
#define K_PGDN		149
#define K_PGUP		150
#define K_HOME		151
#define K_END		152

#define K_KP_HOME		160
#define K_KP_UPARROW	161
#define K_KP_PGUP		162
#define K_KP_LEFTARROW	163
#define K_KP_5		164
#define K_KP_RIGHTARROW	165
#define K_KP_END		166
#define K_KP_DOWNARROW	167
#define K_KP_PGDN		168
#define K_KP_ENTER		169
#define K_KP_INS   		170
#define K_KP_DEL		171
#define K_KP_SLASH		172
#define K_KP_MINUS		173
#define K_KP_PLUS		174
#define K_CAPSLOCK		175
#define K_KP_NUMLOCK	176
	
//
// joystick buttons
//
#define K_JOY1		203
#define K_JOY2		204
#define K_JOY3		205
#define K_JOY4		206

//
// aux keys are for multi-buttoned joysticks to generate so they can use
// the normal binding process
//
#define K_AUX1		207
#define K_AUX2		208
#define K_AUX3		209
#define K_AUX4		210
#define K_AUX5		211
#define K_AUX6		212
#define K_AUX7		213
#define K_AUX8		214
#define K_AUX9		215
#define K_AUX10		216
#define K_AUX11		217
#define K_AUX12		218
#define K_AUX13		219
#define K_AUX14		220
#define K_AUX15		221
#define K_AUX16		222
#define K_AUX17		223
#define K_AUX18		224
#define K_AUX19		225
#define K_AUX20		226
#define K_AUX21		227
#define K_AUX22		228
#define K_AUX23		229
#define K_AUX24		230
#define K_AUX25		231
#define K_AUX26		232
#define K_AUX27		233
#define K_AUX28		234
#define K_AUX29		235
#define K_AUX30		236
#define K_AUX31		237
#define K_AUX32		238
#define K_MWHEELDOWN	239
#define K_MWHEELUP		240

#define K_PAUSE		255

//
// mouse buttons generate virtual keys
//
#define K_MOUSE1		241
#define K_MOUSE2		242
#define K_MOUSE3		243
#define K_MOUSE4		244
#define K_MOUSE5		245

#ifdef _XBOX

//
// Xbox Gamepad Buttons
//
	//Analog Buttons
#define K_ANALOG        246 //Gbrownie - Should equal first analog button
#define K_XBOX_A		246
#define K_XBOX_B		247
#define K_XBOX_X		248
#define K_XBOX_Y		249
#define K_XBOX_BLACK	250
#define K_XBOX_WHITE	251
#define K_XBOX_LTRIG	252
#define K_XBOX_RTRIG	253
	//Digital Buttons
#define K_DIGITAL       254 //Gbrownie - Should equal first digital button
#define K_DPAD_UP		254 //MARTY - Break for K_PAUSE!
#define K_DPAD_DOWN		256
#define K_DPAD_LEFT		257
#define K_DPAD_RIGHT	258
#define K_XBOX_START	259
#define K_XBOX_BACK		260 
#define K_XBOX_LSTICK	261
#define K_XBOX_RSTICK	262
	//IGR
#define K_XBOX_QCOMBO	263

#endif

#endif // KEYDEFS_H