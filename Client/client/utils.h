/*
utils.h - utility code
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef UTILS_H
#define UTILS_H

#include "..\common\cvardef.h"

#define EXPORT	_declspec( dllexport )

#ifdef _WIN32_
#define DLLEXPORT __declspec( dllexport )
#else
#define DLLEXPORT
#endif

typedef unsigned char byte;
typedef unsigned short word;

#ifdef _XBOX // MARTY - For Gamepad update call
#include "..\engine\keydefs.h"
#endif

//typedef int (*pfnUserMsgHook)( const char *pszName, int iSize, void *pbuf );
//typedef int (*cmpfunc)( const void *a, const void *b );

//extern int pause;
extern int developer_level;
extern int r_currentMessageNum;
extern float v_idlescale;

//extern int g_iXashEngineBuildNumber;
extern BOOL g_fRenderInitialized;

#ifndef _XBOX
typedef HMODULE dllhandle_t;

typedef struct dllfunc_s
{
	const char *name;
	void	**func;
} dllfunc_t;
#endif

client_sprite_t *GetSpriteList( client_sprite_t *pList, const char *psz, int iRes, int iCount );/*
extern void DrawQuad( float xmin, float ymin, float xmax, float ymax );*/
extern void ALERT( ALERT_TYPE level, char *szFmt, ... );/*

// dll managment
bool Sys_LoadLibrary( const char *dllname, dllhandle_t *handle, const dllfunc_t *fcts = NULL );
void *Sys_GetProcAddress( dllhandle_t handle, const char *name );
void Sys_FreeLibrary( dllhandle_t *handle );*/

// ScreenHeight returns the height of the screen, in pixels
#define ScreenHeight	(gHUD.m_scrinfo.iHeight)
// ScreenWidth returns the width of the screen, in pixels
#define ScreenWidth		(gHUD.m_scrinfo.iWidth)

// Use this to set any co-ords in 640x480 space
#define XRES( x )		((int)(float( x ) * ((float)ScreenWidth / 640.0f) + 0.5f))
#define YRES( y )		((int)(float( y ) * ((float)ScreenHeight / 480.0f) + 0.5f))

// use this to project world coordinates to screen coordinates
//#define XPROJECT( x )	(( 1.0f + (x)) * ScreenWidth * 0.5f )
//#define YPROJECT( y )	(( 1.0f - (y)) * ScreenHeight * 0.5f )

inline int ConsoleStringLen( const char *string )
{
	int _width, _height;
	GetConsoleStringSize( string, &_width, &_height );
	return _width;
}

void ScaleColors( int &r, int &g, int &b, int a );

inline void UnpackRGB(int &r, int &g, int &b, unsigned long ulRGB)\
{\
	r = (ulRGB & 0xFF0000) >>16;\
	g = (ulRGB & 0xFF00) >> 8;\
	b = ulRGB & 0xFF;\
}

HSPRITE LoadSprite( const char *pszName );

typedef struct leaflist_s
{
	int		count;
	int		maxcount;
	bool		overflowed;
	short		*list;
	Vector		mins, maxs;
	int		topnode;		// for overflows where each leaf can't be stored individually
} leaflist_t;

struct mleaf_s *Mod_PointInLeaf( Vector p, struct mnode_s *node );
bool Mod_BoxVisible( const Vector mins, const Vector maxs, const byte *visbits );
//bool Mod_CheckEntityPVS( cl_entity_t *ent );
//bool Mod_CheckTempEntityPVS( struct tempent_s *pTemp );
bool Mod_CheckEntityLeafPVS( const Vector &absmin, const Vector &absmax, struct mleaf_s *leaf );
//void Mod_GetFrames( int modelIndex, int &numFrames );
//int Mod_GetType( int modelIndex );

bool UTIL_IsPlayer( int idx );
bool UTIL_IsLocal( int idx );

extern "C" void StaticHUD_StudioEvent( const struct mstudioevent_s *event, const struct cl_entity_s *entity );
extern "C" void StaticHUD_TempEntUpdate( double frametime, double client_time, double cl_gravity, struct tempent_s **ppTempEntFree,
struct tempent_s **ppTempEntActive, int ( *Callback_AddVisibleEntity )( struct cl_entity_s *pEntity ),
void ( *Callback_TempEntPlaySound )( struct tempent_s *pTemp, float damp ));

extern "C" int StaticHUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname );
extern "C" void StaticHUD_TxferLocalOverrides( struct entity_state_s *state, const struct clientdata_s *client );
extern "C" void StaticHUD_ProcessPlayerState( struct entity_state_s *dst, const struct entity_state_s *src );
extern "C" void StaticHUD_TxferPredictionData( entity_state_t *ps, const entity_state_t *pps, clientdata_t *pcd, const clientdata_t *ppcd, weapon_data_t *wd, const weapon_data_t *pwd );
extern "C" void StaticHUD_StudioEvent( const struct mstudioevent_s *event, const struct cl_entity_s *entity );
extern "C" void StaticHUD_CreateEntities( void );

extern int CL_ButtonBits( int );
extern void CL_ResetButtonBits( int bits );
extern void InitInput( void );
extern void ShutdownInput( void );
extern int KB_ConvertString( char *in, char **ppout );
extern void IN_Init( void );
extern void IN_Move( float frametime, struct usercmd_s *cmd );
extern void IN_Shutdown( void );

extern "C" void StaticIN_ActivateMouse( void );
extern "C" void StaticIN_DeactivateMouse( void );
extern "C" void StaticIN_MouseEvent( int mstate );
extern "C" void StaticIN_Accumulate( void );
extern "C" void StaticIN_ClearStates( void );
extern "C" void *StaticKB_Find( const char *name );
extern "C" void StaticCL_CreateMove( float frametime, struct usercmd_s *cmd, int active );

#ifdef _XBOX // MARTY - Custom Xbox only call to pump gamepad key messages.
extern "C" int /*DLLEXPORT*/ StaticIN_XBoxGamepadButtons( XBGamepadButtons_t *pGamepadButtons );
#endif

//extern "C" int HUD_GetRenderInterface( int version, render_api_t *renderfuncs, render_interface_t *callback );
extern "C" int StaticHUD_GetStudioModelInterface( int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio );

extern void PM_Init( struct playermove_s *ppmove );
extern void PM_Move( struct playermove_s *ppmove, int server );
extern char PM_FindTextureType( char *name );
extern "C" void StaticV_CalcRefdef( struct ref_params_s *pparams );

//void UTIL_CreateAurora( cl_entity_t *ent, const char *file, int attachment, float lifetime = 0.0f );
//void UTIL_RemoveAurora( cl_entity_t *ent );
extern int PM_GetPhysEntInfo( int ent );

extern "C" void StaticCAM_Think( void );
extern "C" void StaticCL_CameraOffset( float *ofs );
extern "C" int StaticCL_IsThirdPerson( void );

// xxx need client dll function to get and clear impuse
//extern cvar_t *in_joystick;
extern int g_weaponselect;

//void ClearLink( link_t *l );
//void RemoveLink( link_t *l );
//void InsertLinkBefore( link_t *l, link_t *before );

//extern float noise( float vx, float vy, float vz );
//extern void init_noise( void );
   
#endif // UTILS_H