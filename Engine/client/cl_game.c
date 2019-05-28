/*
cl_game.c - client dll interaction
Copyright (C) 2008 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "client.h"
#include "const.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "demo_api.h"
#include "ivoicetweak.h"
#include "pm_local.h"
#include "cl_tent.h"
#include "input.h"
#include "shake.h"
#include "sprite.h"
#include "gl_local.h"
#include "library.h"
#include "vgui_draw.h"
#include "sound.h" // SND_STOP_LOOPING

#define MAX_LINELENGTH	80
#define MAX_TEXTCHANNELS	8		// must be power of two (GoldSrc uses 4 channels)

#ifdef _HARDLINKED

void /*DLLEXPORT*/ StaticIN_ClearStates (void);
int	/*DLLEXPORT*/ StaticHUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void /*DLLEXPORT*/ StaticV_CalcRefdef( struct ref_params_s *pparams );
void /*DLLEXPORT*/ StaticHUD_TempEntUpdate( double frametime, double client_time, double cl_gravity, struct tempent_s **ppTempEntFree, struct tempent_s **ppTempEntActive, int ( *Callback_AddVisibleEntity )( struct cl_entity_s *pEntity ), void ( *Callback_TempEntPlaySound )( struct tempent_s *pTemp, float damp ) );
int	/*DLLEXPORT*/ StaticHUD_VidInit( void );
int	/*DLLEXPORT*/ StaticHUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
int	/*DLLEXPORT*/ StaticHUD_UpdateClientData( client_data_t *cdata, float flTime );
int	/*DLLEXPORT*/ StaticHUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
struct cl_entity_s /*DLLEXPORT*/ *StaticHUD_GetUserEntity( int index );
int	/*DLLEXPORT*/ StaticHUD_Redraw( float flTime, int intermission ); 
int	/*DLLEXPORT*/ StaticInitialize( cl_enginefunc_t *pEnginefuncs, int iVersion );
void/*DLLEXPORT*/ StaticHUD_Init( void );
int /*DLLEXPORT*/ StaticHUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname );
void /*DLLEXPORT*/ StaticHUD_TxferPredictionData ( struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd, const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd );
void /*DLLEXPORT*/ StaticHUD_TxferLocalOverrides( struct entity_state_s *state, const struct clientdata_s *client );
void /*DLLEXPORT*/ StaticHUD_ProcessPlayerState( struct entity_state_s *dst, const struct entity_state_s *src );
void /*DLLEXPORT*/ StaticCAM_Think( void );
void /*DLLEXPORT*/ StaticHUD_CreateEntities( void );
void /*DLLEXPORT*/ StaticHUD_Frame( double time );
void /*DLLEXPORT*/ StaticIN_Accumulate (void);
char /*DLLEXPORT*/ StaticHUD_PlayerMoveTexture( char *name );
void /*DLLEXPORT*/ StaticHUD_StudioEvent( const struct mstudioevent_s *event, const struct cl_entity_s *entity );
void /*DLLEXPORT*/ StaticCL_CreateMove ( float frametime, struct usercmd_s *cmd, int active );
void /*DLLEXPORT*/ StaticHUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed );
void /*DLLEXPORT*/ StaticHUD_Reset ( void );
void /*DLLEXPORT*/ StaticHUD_Shutdown( void );
void /*DLLEXPORT*/ StaticHUD_DrawTransparentTriangles( void );
void /*DLLEXPORT*/ StaticHUD_DrawNormalTriangles( void );
int /*DLLEXPORT*/ StaticCL_IsThirdPerson( void );
void /*DLLEXPORT*/ StaticCL_CameraOffset( float *ofs );
void /*DLLEXPORT*/ StaticDemo_ReadBuffer( int size, unsigned char *buffer );
void /*DLLEXPORT*/ StaticHUD_PlayerMoveInit( struct playermove_s *ppmove );
void /*DLLEXPORT*/ StaticHUD_PlayerMove( struct playermove_s *ppmove, int server );
int /*DLLEXPORT*/ StaticHUD_Key_Event( int eventcode, int keynum, const char *pszCurrentBinding );
void /*DLLEXPORT*/ StaticIN_ActivateMouse( void ); 
void /*DLLEXPORT*/ StaticIN_DeactivateMouse( void );
void /*DLLEXPORT*/ StaticIN_MouseEvent( int mstate );
void /*DLLEXPORT*/ *StaticKB_Find( const char *name );

// The "new" export headers
int /*DLLEXPORT*/ StaticHUD_GetStudioModelInterface( int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio );
void /*DLLEXPORT*/ StaticHUD_VoiceStatus(int entindex, qboolean bTalking);
void /*DLLEXPORT*/ StaticHUD_DirectorMessage( int iSize, void *pbuf );

#ifdef _XBOX //MARTY - Custom XBox only call to pump gamepad button messages
int /*DLLEXPORT*/ StaticIN_XBoxGamepadButtons( XBGamepadButtons_t *pGamepadButtons );
#endif

#endif //_HARDLINKED

#define TEXT_MSGNAME	"TextMessage%i"

char			cl_textbuffer[MAX_TEXTCHANNELS][2048];
client_textmessage_t	cl_textmessage[MAX_TEXTCHANNELS];

static dllfunc_t cdll_exports[] =
{
{ "Initialize", (void **)&clgame.dllFuncs.pfnInitialize },
{ "HUD_VidInit", (void **)&clgame.dllFuncs.pfnVidInit },
{ "HUD_Init", (void **)&clgame.dllFuncs.pfnInit },
{ "HUD_Shutdown", (void **)&clgame.dllFuncs.pfnShutdown },
{ "HUD_Redraw", (void **)&clgame.dllFuncs.pfnRedraw },
{ "HUD_UpdateClientData", (void **)&clgame.dllFuncs.pfnUpdateClientData },
{ "HUD_Reset", (void **)&clgame.dllFuncs.pfnReset },
{ "HUD_PlayerMove", (void **)&clgame.dllFuncs.pfnPlayerMove },
{ "HUD_PlayerMoveInit", (void **)&clgame.dllFuncs.pfnPlayerMoveInit },
{ "HUD_PlayerMoveTexture", (void **)&clgame.dllFuncs.pfnPlayerMoveTexture },
{ "HUD_ConnectionlessPacket", (void **)&clgame.dllFuncs.pfnConnectionlessPacket },
{ "HUD_GetHullBounds", (void **)&clgame.dllFuncs.pfnGetHullBounds },
{ "HUD_Frame", (void **)&clgame.dllFuncs.pfnFrame },
{ "HUD_PostRunCmd", (void **)&clgame.dllFuncs.pfnPostRunCmd },
{ "HUD_Key_Event", (void **)&clgame.dllFuncs.pfnKey_Event },
{ "HUD_AddEntity", (void **)&clgame.dllFuncs.pfnAddEntity },
{ "HUD_CreateEntities", (void **)&clgame.dllFuncs.pfnCreateEntities },
{ "HUD_StudioEvent", (void **)&clgame.dllFuncs.pfnStudioEvent },
{ "HUD_TxferLocalOverrides", (void **)&clgame.dllFuncs.pfnTxferLocalOverrides },
{ "HUD_ProcessPlayerState", (void **)&clgame.dllFuncs.pfnProcessPlayerState },
{ "HUD_TxferPredictionData", (void **)&clgame.dllFuncs.pfnTxferPredictionData },
{ "HUD_TempEntUpdate", (void **)&clgame.dllFuncs.pfnTempEntUpdate },
{ "HUD_DrawNormalTriangles", (void **)&clgame.dllFuncs.pfnDrawNormalTriangles },
{ "HUD_DrawTransparentTriangles", (void **)&clgame.dllFuncs.pfnDrawTransparentTriangles },
{ "HUD_GetUserEntity", (void **)&clgame.dllFuncs.pfnGetUserEntity },
{ "Demo_ReadBuffer", (void **)&clgame.dllFuncs.pfnDemo_ReadBuffer },
{ "CAM_Think", (void **)&clgame.dllFuncs.CAM_Think },
{ "CL_IsThirdPerson", (void **)&clgame.dllFuncs.CL_IsThirdPerson },
{ "CL_CameraOffset", (void **)&clgame.dllFuncs.CL_CameraOffset },	// unused callback. Now camera code is completely moved to the user area
{ "CL_CreateMove", (void **)&clgame.dllFuncs.CL_CreateMove },
{ "IN_ActivateMouse", (void **)&clgame.dllFuncs.IN_ActivateMouse },
{ "IN_DeactivateMouse", (void **)&clgame.dllFuncs.IN_DeactivateMouse },
{ "IN_MouseEvent", (void **)&clgame.dllFuncs.IN_MouseEvent },
{ "IN_Accumulate", (void **)&clgame.dllFuncs.IN_Accumulate },
{ "IN_ClearStates", (void **)&clgame.dllFuncs.IN_ClearStates },
{ "V_CalcRefdef", (void **)&clgame.dllFuncs.pfnCalcRefdef },
{ "KB_Find", (void **)&clgame.dllFuncs.KB_Find },
{ NULL, NULL }
};

// optional exports
static dllfunc_t cdll_new_exports[] = 	// allowed only in SDK 2.3 and higher
{
{ "HUD_GetStudioModelInterface", (void **)&clgame.dllFuncs.pfnGetStudioModelInterface },
{ "HUD_DirectorMessage", (void **)&clgame.dllFuncs.pfnDirectorMessage },
{ "HUD_VoiceStatus", (void **)&clgame.dllFuncs.pfnVoiceStatus },
// MARTY - Extensions
{ "HUD_ChatInputPosition", (void **)&clgame.dllFuncs.pfnChatInputPosition },
{ "HUD_GetRenderInterface", (void **)&clgame.dllFuncs.pfnGetRenderInterface },	// Xash3D ext
{ "HUD_ClipMoveToEntity", (void **)&clgame.dllFuncs.pfnClipMoveToEntity },	// Xash3D ext
{ NULL, NULL }
};

static void pfnSPR_DrawHoles( int frame, int x, int y, const wrect_t *prc );

/*
====================
CL_GetEntityByIndex

Render callback for studio models
====================
*/
cl_entity_t *CL_GetEntityByIndex( int index )
{
	if( !clgame.entities ) // not in game yet
		return NULL;

	if( index < 0 || index >= clgame.maxEntities )
		return NULL;

	if( index == 0 )
		return clgame.entities;

	return CL_EDICT_NUM( index );
}

/*
================
CL_ModelHandle

get model handle by index
================
*/
model_t *CL_ModelHandle( int modelindex )
{
	if( modelindex < 0 || modelindex >= MAX_MODELS )
		return NULL;
	return cl.models[modelindex];
}

/*
====================
CL_IsThirdPerson

returns true if thirdperson is enabled
====================
*/
qboolean CL_IsThirdPerson( void )
{
	cl.local.thirdperson = clgame.dllFuncs.CL_IsThirdPerson();

	if( cl.local.thirdperson )
		return true;
	return false;
}

/*
====================
CL_GetPlayerInfo

get player info by render request
====================
*/
player_info_t *CL_GetPlayerInfo( int playerIndex )
{
	if( playerIndex < 0 || playerIndex >= cl.maxclients )
		return NULL;

	return &cl.players[playerIndex];
}

/*
====================
CL_CreatePlaylist

Create a default valve playlist
====================
*/
void CL_CreatePlaylist( const char *filename )
{
	file_t	*f;

	f = FS_Open( filename, "w", false );
	if( !f ) return;

	// make standard cdaudio playlist
	FS_Print( f, "blank\n" );		// #1
	FS_Print( f, "Half-Life01.mp3\n" );	// #2
	FS_Print( f, "Prospero01.mp3\n" );	// #3
	FS_Print( f, "Half-Life12.mp3\n" );	// #4
	FS_Print( f, "Half-Life07.mp3\n" );	// #5
	FS_Print( f, "Half-Life10.mp3\n" );	// #6
	FS_Print( f, "Suspense01.mp3\n" );	// #7
	FS_Print( f, "Suspense03.mp3\n" );	// #8
	FS_Print( f, "Half-Life09.mp3\n" );	// #9
	FS_Print( f, "Half-Life02.mp3\n" );	// #10
	FS_Print( f, "Half-Life13.mp3\n" );	// #11
	FS_Print( f, "Half-Life04.mp3\n" );	// #12
	FS_Print( f, "Half-Life15.mp3\n" );	// #13
	FS_Print( f, "Half-Life14.mp3\n" );	// #14
	FS_Print( f, "Half-Life16.mp3\n" );	// #15
	FS_Print( f, "Suspense02.mp3\n" );	// #16
	FS_Print( f, "Half-Life03.mp3\n" );	// #17
	FS_Print( f, "Half-Life08.mp3\n" );	// #18
	FS_Print( f, "Prospero02.mp3\n" );	// #19
	FS_Print( f, "Half-Life05.mp3\n" );	// #20
	FS_Print( f, "Prospero04.mp3\n" );	// #21
	FS_Print( f, "Half-Life11.mp3\n" );	// #22
	FS_Print( f, "Half-Life06.mp3\n" );	// #23
	FS_Print( f, "Prospero03.mp3\n" );	// #24
	FS_Print( f, "Half-Life17.mp3\n" );	// #25
	FS_Print( f, "Prospero05.mp3\n" );	// #26
	FS_Print( f, "Suspense05.mp3\n" );	// #27
	FS_Print( f, "Suspense07.mp3\n" );	// #28
	FS_Close( f );
}

/*
====================
CL_InitCDAudio

Initialize CD playlist
====================
*/
void CL_InitCDAudio( const char *filename )
{
	char	*afile, *pfile;
	string	token;
	int	c = 0;

	if( !FS_FileExists( filename, false ))
	{
		// create a default playlist
		CL_CreatePlaylist( filename );
	}

	afile = FS_LoadFile( filename, NULL, false );
	if( !afile ) return;

	pfile = afile;

	// format: trackname\n [num]
	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		if( !Q_stricmp( token, "blank" )) token[0] = '\0';
		Q_strncpy( clgame.cdtracks[c], token, sizeof( clgame.cdtracks[0] ));

		if( ++c > MAX_CDTRACKS - 1 )
		{
			Con_Reportf( S_WARN "CD_Init: too many tracks %i in %s\n", MAX_CDTRACKS, filename );
			break;
		}
	}

	Mem_Free( afile );
}

/*
====================
CL_PointContents

Return contents for point
====================
*/
int CL_PointContents( const vec3_t p )
{
	int cont = PM_PointContents( clgame.pmove, p );

	if( cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN )
		cont = CONTENTS_WATER;
	return cont;
}

/*
=============
CL_AdjustXPos

adjust text by x pos
=============
*/
static int CL_AdjustXPos( float x, int width, int totalWidth )
{
	int	xPos;

	if( x == -1 )
	{
		xPos = ( glState.width - width ) * 0.5f;
	}
	else
	{
		if ( x < 0 )
			xPos = (1.0f + x) * glState.width - totalWidth;	// Alight right
		else // align left
			xPos = x * glState.width;
	}

	if( xPos + width > glState.width )
		xPos = glState.width - width;
	else if( xPos < 0 )
		xPos = 0;

	return xPos;
}

/*
=============
CL_AdjustYPos

adjust text by y pos
=============
*/
static int CL_AdjustYPos( float y, int height )
{
	int	yPos;

	if( y == -1 ) // centered?
	{
		yPos = ( glState.height - height ) * 0.5f;
	}
	else
	{
		// Alight bottom?
		if( y < 0 )
			yPos = (1.0f + y) * glState.height - height; // Alight bottom
		else // align top
			yPos = y * glState.height;
	}

	if( yPos + height > glState.height )
		yPos = glState.height - height;
	else if( yPos < 0 )
		yPos = 0;

	return yPos;
}

/*
=============
CL_CenterPrint

print centerscreen message
=============
*/
void CL_CenterPrint( const char *text, float y )
{
	int	length = 0;
	int	width = 0;
	char	*s;

	if( !COM_CheckString( text ))
		return;

	clgame.centerPrint.lines = 1;
	clgame.centerPrint.totalWidth = 0;
	clgame.centerPrint.time = cl.mtime[0]; // allow pause for centerprint
	Q_strncpy( clgame.centerPrint.message, text, sizeof( clgame.centerPrint.message ));
	s = clgame.centerPrint.message;

	// count the number of lines for centering
	while( *s )
	{
		if( *s == '\n' )
		{
			clgame.centerPrint.lines++;
			if( width > clgame.centerPrint.totalWidth )
				clgame.centerPrint.totalWidth = width;
			width = 0;
		}
		else width += clgame.scrInfo.charWidths[*s];
		s++;
		length++;
	}

	clgame.centerPrint.totalHeight = ( clgame.centerPrint.lines * clgame.scrInfo.iCharHeight ); 
	clgame.centerPrint.y = CL_AdjustYPos( y, clgame.centerPrint.totalHeight );
}

/*
====================
SPR_AdjustSize

draw hudsprite routine
====================
*/
static void SPR_AdjustSize( float *x, float *y, float *w, float *h )
{
	float	xscale, yscale;

	if( !x && !y && !w && !h ) return;

	// scale for screen sizes
	xscale = glState.width / (float)clgame.scrInfo.iWidth;
	yscale = glState.height / (float)clgame.scrInfo.iHeight;

	if( x ) *x *= xscale;
	if( y ) *y *= yscale;
	if( w ) *w *= xscale;
	if( h ) *h *= yscale;
}

/*
====================
PictAdjustSize

draw hudsprite routine
====================
*/
void PicAdjustSize( float *x, float *y, float *w, float *h )
{
	float	xscale, yscale;

	if( !clgame.ds.adjust_size ) return;
	if( !x && !y && !w && !h ) return;

	// scale for screen sizes
	xscale = glState.width / (float)clgame.scrInfo.iWidth;
	yscale = glState.height / (float)clgame.scrInfo.iHeight;

	if( x ) *x *= xscale;
	if( y ) *y *= yscale;
	if( w ) *w *= xscale;
	if( h ) *h *= yscale;
}

static qboolean SPR_Scissor( float *x, float *y, float *width, float *height, float *u0, float *v0, float *u1, float *v1 )
{
	float	dudx, dvdy;

	// clip sub rect to sprite
	if(( width == 0 ) || ( height == 0 ))
		return false;

	if( *x + *width <= clgame.ds.scissor_x )
		return false;
	if( *x >= clgame.ds.scissor_x + clgame.ds.scissor_width )
		return false;
	if( *y + *height <= clgame.ds.scissor_y )
		return false;
	if( *y >= clgame.ds.scissor_y + clgame.ds.scissor_height )
		return false;

	dudx = (*u1 - *u0) / *width;
	dvdy = (*v1 - *v0) / *height;

	if( *x < clgame.ds.scissor_x )
	{
		*u0 += (clgame.ds.scissor_x - *x) * dudx;
		*width -= clgame.ds.scissor_x - *x;
		*x = clgame.ds.scissor_x;
	}

	if( *x + *width > clgame.ds.scissor_x + clgame.ds.scissor_width )
	{
		*u1 -= (*x + *width - (clgame.ds.scissor_x + clgame.ds.scissor_width)) * dudx;
		*width = clgame.ds.scissor_x + clgame.ds.scissor_width - *x;
	}

	if( *y < clgame.ds.scissor_y )
	{
		*v0 += (clgame.ds.scissor_y - *y) * dvdy;
		*height -= clgame.ds.scissor_y - *y;
		*y = clgame.ds.scissor_y;
	}

	if( *y + *height > clgame.ds.scissor_y + clgame.ds.scissor_height )
	{
		*v1 -= (*y + *height - (clgame.ds.scissor_y + clgame.ds.scissor_height)) * dvdy;
		*height = clgame.ds.scissor_y + clgame.ds.scissor_height - *y;
	}

	return true;
}

/*
====================
SPR_DrawGeneric

draw hudsprite routine
====================
*/
static void SPR_DrawGeneric( int frame, float x, float y, float width, float height, const wrect_t *prc )
{
	float	s1, s2, t1, t2;
	int	texnum;

	if( width == -1 && height == -1 )
	{
		int	w, h;

		// assume we get sizes from image
		R_GetSpriteParms( &w, &h, NULL, frame, clgame.ds.pSprite );

		width = w;
		height = h;
	}

	if( prc )
	{
		wrect_t	rc;

		rc = *prc;

		// Sigh! some stupid modmakers set wrong rectangles in hud.txt 
		if( rc.left <= 0 || rc.left >= width ) rc.left = 0;
		if( rc.top <= 0 || rc.top >= height ) rc.top = 0;
		if( rc.right <= 0 || rc.right > width ) rc.right = width;
		if( rc.bottom <= 0 || rc.bottom > height ) rc.bottom = height;

		// calc user-defined rectangle
		s1 = (float)rc.left / width;
		t1 = (float)rc.top / height;
		s2 = (float)rc.right / width;
		t2 = (float)rc.bottom / height;
		width = rc.right - rc.left;
		height = rc.bottom - rc.top;
	}
	else
	{
		s1 = t1 = 0.0f;
		s2 = t2 = 1.0f;
	}

	// pass scissor test if supposed
	if( clgame.ds.scissor_test && !SPR_Scissor( &x, &y, &width, &height, &s1, &t1, &s2, &t2 ))
		return;

	// scale for screen sizes
	SPR_AdjustSize( &x, &y, &width, &height );
	texnum = R_GetSpriteTexture( clgame.ds.pSprite, frame );
	/*p*/glColor4ubv( clgame.ds.spriteColor );
	/*p*/glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	R_DrawStretchPic( x, y, width, height, s1, t1, s2, t2, texnum );
}

/*
=============
CL_DrawCenterPrint

called each frame
=============
*/
void CL_DrawCenterPrint( void )
{
	char	*pText;
	int	i, j, x, y;
	int	width, lineLength;
	byte	*colorDefault, line[MAX_LINELENGTH];
	int	charWidth, charHeight;

	if( !clgame.centerPrint.time )
		return;

	if(( cl.time - clgame.centerPrint.time ) >= scr_centertime->value )
	{
		// time expired
		clgame.centerPrint.time = 0.0f;
		return;
	}

	y = clgame.centerPrint.y; // start y
	colorDefault = g_color_table[7];
	pText = clgame.centerPrint.message;
	Con_DrawCharacterLen( 0, NULL, &charHeight );
	
	for( i = 0; i < clgame.centerPrint.lines; i++ )
	{
		lineLength = 0;
		width = 0;

		while( *pText && *pText != '\n' && lineLength < MAX_LINELENGTH )
		{
			byte c = *pText;
			line[lineLength] = c;
			Con_DrawCharacterLen( c, &charWidth, NULL );
			width += charWidth;
			lineLength++;
			pText++;
		}

		if( lineLength == MAX_LINELENGTH )
			lineLength--;

		pText++; // Skip LineFeed
		line[lineLength] = 0;

		x = CL_AdjustXPos( -1, width, clgame.centerPrint.totalWidth );

		for( j = 0; j < lineLength; j++ )
		{
			if( x >= 0 && y >= 0 && x <= glState.width )
				x += Con_DrawCharacter( x, y, line[j], colorDefault );
		}
		y += charHeight;
	}
}

/*
=============
CL_DrawScreenFade

fill screen with specfied color
can be modulated
=============
*/
void CL_DrawScreenFade( void )
{
	screenfade_t	*sf = &clgame.fade;
	int		iFadeAlpha, testFlags;

	// keep pushing reset time out indefinitely
	if( sf->fadeFlags & FFADE_STAYOUT )
		sf->fadeReset = cl.time + 0.1f;
		
	if( sf->fadeReset == 0.0f && sf->fadeEnd == 0.0f )
		return;	// inactive

	// all done?
	if(( cl.time > sf->fadeReset ) && ( cl.time > sf->fadeEnd ))
	{
		memset( &clgame.fade, 0, sizeof( clgame.fade ));
		return;
	}

	testFlags = (sf->fadeFlags & ~FFADE_MODULATE);

	// fading...
	if( testFlags == FFADE_STAYOUT )
	{
		iFadeAlpha = sf->fadealpha;
	}
	else
	{
		iFadeAlpha = sf->fadeSpeed * ( sf->fadeEnd - cl.time );
		if( sf->fadeFlags & FFADE_OUT ) iFadeAlpha += sf->fadealpha;
		iFadeAlpha = bound( 0, iFadeAlpha, sf->fadealpha );
	}

	/*p*/glColor4ub( sf->fader, sf->fadeg, sf->fadeb, iFadeAlpha );

	if( sf->fadeFlags & FFADE_MODULATE )
		GL_SetRenderMode( kRenderTransAdd );
	else GL_SetRenderMode( kRenderTransTexture );
	R_DrawStretchPic( 0, 0, glState.width, glState.height, 0, 0, 1, 1, tr.whiteTexture );
	/*p*/glColor4ub( 255, 255, 255, 255 );
}

/*
====================
CL_InitTitles

parse all messages that declared in titles.txt
and hold them into permament memory pool 
====================
*/
static void CL_InitTitles( const char *filename )
{
	size_t	fileSize;
	byte	*pMemFile;
	int	i;

	// initialize text messages (game_text)
	for( i = 0; i < MAX_TEXTCHANNELS; i++ )
	{
		cl_textmessage[i].pName = _copystring( clgame.mempool, va( TEXT_MSGNAME, i ), __FILE__, __LINE__ );
		cl_textmessage[i].pMessage = cl_textbuffer[i];
	}

	// clear out any old data that's sitting around.
	if( clgame.titles ) Mem_Free( clgame.titles );

	clgame.titles = NULL;
	clgame.numTitles = 0;

	pMemFile = FS_LoadFile( filename, &fileSize, false );
	if( !pMemFile ) return;

	CL_TextMessageParse( pMemFile, fileSize );
	Mem_Free( pMemFile );
}

/*
====================
CL_HudMessage

Template to show hud messages
====================
*/
void CL_HudMessage( const char *pMessage )
{
	if( !COM_CheckString( pMessage )) return;
	CL_DispatchUserMessage( "HudText", Q_strlen( pMessage ), (void *)pMessage );
}

/*
====================
CL_ParseTextMessage

Parse TE_TEXTMESSAGE
====================
*/
void CL_ParseTextMessage( sizebuf_t *msg )
{
	static int		msgindex = 0;
	client_textmessage_t	*text;
	int			channel;

	// read channel ( 0 - auto)
	channel = MSG_ReadByte( msg );

	if( channel <= 0 || channel > ( MAX_TEXTCHANNELS - 1 ))
	{
		channel = msgindex;
		msgindex = (msgindex + 1) & (MAX_TEXTCHANNELS - 1);
	}	

	// grab message channel
	text = &cl_textmessage[channel];

	text->x = (float)(MSG_ReadShort( msg ) / 8192.0f);
	text->y = (float)(MSG_ReadShort( msg ) / 8192.0f);
	text->effect = MSG_ReadByte( msg );
	text->r1 = MSG_ReadByte( msg );
	text->g1 = MSG_ReadByte( msg );
	text->b1 = MSG_ReadByte( msg );
	text->a1 = MSG_ReadByte( msg );
	text->r2 = MSG_ReadByte( msg );
	text->g2 = MSG_ReadByte( msg );
	text->b2 = MSG_ReadByte( msg );
	text->a2 = MSG_ReadByte( msg );
	text->fadein = (float)(MSG_ReadShort( msg ) / 256.0f );
	text->fadeout = (float)(MSG_ReadShort( msg ) / 256.0f );
	text->holdtime = (float)(MSG_ReadShort( msg ) / 256.0f );

	if( text->effect == 2 )
		text->fxtime = (float)(MSG_ReadShort( msg ) / 256.0f );
	else text->fxtime = 0.0f;

	// to prevent grab too long messages
	Q_strncpy( (char *)text->pMessage, MSG_ReadString( msg ), 2048 ); 		

	CL_HudMessage( text->pName );
}

/*
================
CL_ParseFinaleCutscene

show display finale or cutscene message
================
*/
void CL_ParseFinaleCutscene( sizebuf_t *msg, int level )
{
	static int		msgindex = 0;
	client_textmessage_t	*text;
	int			channel;

	cl.intermission = level;

	channel = msgindex;
	msgindex = (msgindex + 1) & (MAX_TEXTCHANNELS - 1);

	// grab message channel
	text = &cl_textmessage[channel];

	// NOTE: svc_finale and svc_cutscene has a
	// predefined settings like Quake-style
	text->x = -1.0f;
	text->y = 0.15f;
	text->effect = 2;	// scan out effect
	text->r1 = 245;
	text->g1 = 245;
	text->b1 = 245;
	text->a1 = 0;	// unused
	text->r2 = 0;
	text->g2 = 0;
	text->b2 = 0;
	text->a2 = 0;
	text->fadein = 0.15f;
	text->fadeout = 0.0f;
	text->holdtime = 99999.0f;
	text->fxtime = 0.0f;

	// to prevent grab too long messages
	Q_strncpy( (char *)text->pMessage, MSG_ReadString( msg ), 2048 ); 		

	if( *text->pMessage == '\0' )
		return; // no real text

	CL_HudMessage( text->pName );
}

/*
====================
CL_GetLocalPlayer

Render callback for studio models
====================
*/
cl_entity_t *CL_GetLocalPlayer( void )
{
	cl_entity_t	*player;

	player = CL_EDICT_NUM( cl.playernum + 1 );
	Assert( player != NULL );

	return player;
}

/*
====================
CL_GetMaxlients

Render callback for studio models
====================
*/
int CL_GetMaxClients( void )
{
	return cl.maxclients;
}

/*
====================
CL_SoundFromIndex

return soundname from index
====================
*/
const char *CL_SoundFromIndex( int index )
{
	sfx_t	*sfx = NULL;
	int	hSound;

	// make sure what we in-bounds
	index = bound( 0, index, MAX_SOUNDS );
	hSound = cl.sound_index[index];

	if( !hSound )
	{
		Con_DPrintf( S_ERROR "CL_SoundFromIndex: invalid sound index %i\n", index );
		return NULL;
	}

	sfx = S_GetSfxByHandle( hSound );
	if( !sfx )
	{
		Con_DPrintf( S_ERROR "CL_SoundFromIndex: bad sfx for index %i\n", index );
		return NULL;
	}

	return sfx->name;
}

/*
=========
SPR_EnableScissor

=========
*/
static void SPR_EnableScissor( int x, int y, int width, int height )
{
	// check bounds
	x = bound( 0, x, clgame.scrInfo.iWidth );
	y = bound( 0, y, clgame.scrInfo.iHeight );
	width = bound( 0, width, clgame.scrInfo.iWidth - x );
	height = bound( 0, height, clgame.scrInfo.iHeight - y );

	clgame.ds.scissor_x = x;
	clgame.ds.scissor_width = width;
	clgame.ds.scissor_y = y;
	clgame.ds.scissor_height = height;
	clgame.ds.scissor_test = true;
}

/*
=========
SPR_DisableScissor

=========
*/
static void SPR_DisableScissor( void )
{
	clgame.ds.scissor_x = 0;
	clgame.ds.scissor_width = 0;
	clgame.ds.scissor_y = 0;
	clgame.ds.scissor_height = 0;
	clgame.ds.scissor_test = false;
}

/*
====================
CL_DrawCrosshair

Render crosshair
====================
*/
void CL_DrawCrosshair( void )
{
	int	x, y, width, height;

	if( !clgame.ds.pCrosshair || !cl_crosshair->value )
		return;

	// any camera on or client is died
	if( cl.local.health <= 0 || cl.viewentity != ( cl.playernum + 1 ))
		return;

	// get crosshair dimension
	width = clgame.ds.rcCrosshair.right - clgame.ds.rcCrosshair.left;
	height = clgame.ds.rcCrosshair.bottom - clgame.ds.rcCrosshair.top;

	x = clgame.viewport[0] + ( clgame.viewport[2] >> 1 ); 
	y = clgame.viewport[1] + ( clgame.viewport[3] >> 1 );

	// g-cont - cl.crosshairangle is the autoaim angle.
	// if we're not using autoaim, just draw in the middle of the screen
	if( !VectorIsNull( cl.crosshairangle ))
	{
		vec3_t	angles;
		vec3_t	forward;
		vec3_t	point, screen;

		VectorAdd( RI.viewangles, cl.crosshairangle, angles );
		AngleVectors( angles, forward, NULL, NULL );
		VectorAdd( RI.vieworg, forward, point );
		R_WorldToScreen( point, screen );

		x += ( clgame.viewport[2] >> 1 ) * screen[0] + 0.5f;
		y += ( clgame.viewport[3] >> 1 ) * screen[1] + 0.5f;
	}

	// move at center the screen
	x -= 0.5f * width;
	y -= 0.5f * height;

	clgame.ds.pSprite = clgame.ds.pCrosshair;
	*(int *)clgame.ds.spriteColor = *(int *)clgame.ds.rgbaCrosshair;

	SPR_EnableScissor( x, y, width, height );
	pfnSPR_DrawHoles( 0, x, y, &clgame.ds.rcCrosshair );
	SPR_DisableScissor();
}

/*
=============
CL_DrawLoading

draw loading progress bar
=============
*/
static void CL_DrawLoading( float percent )
{
	int	x, y, width, height, right;
	float	xscale, yscale, step, s2;

	R_GetTextureParms( &width, &height, cls.loadingBar );
	x = ( clgame.scrInfo.iWidth - width ) >> 1;
	y = ( clgame.scrInfo.iHeight - height) >> 1;

	xscale = glState.width / (float)clgame.scrInfo.iWidth;
	yscale = glState.height / (float)clgame.scrInfo.iHeight;

	x *= xscale;
	y *= yscale;
	width *= xscale;
	height *= yscale;

	if( cl_allow_levelshots->value )
  	{
		/*p*/glColor4ub( 128, 128, 128, 255 );
		GL_SetRenderMode( kRenderTransTexture );
		R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, cls.loadingBar );

		step = (float)width / 100.0f;
		right = (int)ceil( percent * step );
		s2 = (float)right / width;
		width = right;
	
		/*p*/glColor4ub( 208, 152, 0, 255 );
		GL_SetRenderMode( kRenderTransTexture );
		R_DrawStretchPic( x, y, width, height, 0, 0, s2, 1, cls.loadingBar );
		/*p*/glColor4ub( 255, 255, 255, 255 );
	}
	else
	{
		/*p*/glColor4ub( 255, 255, 255, 255 );
		GL_SetRenderMode( kRenderTransTexture );
		R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, cls.loadingBar );
	}
}

/*
=============
CL_DrawPause

draw pause sign
=============
*/
static void CL_DrawPause( void )
{
	int	x, y, width, height;
	float	xscale, yscale;

	R_GetTextureParms( &width, &height, cls.pauseIcon );
	x = ( clgame.scrInfo.iWidth - width ) >> 1;
	y = ( clgame.scrInfo.iHeight - height) >> 1;

	xscale = glState.width / (float)clgame.scrInfo.iWidth;
	yscale = glState.height / (float)clgame.scrInfo.iHeight;

	x *= xscale;
	y *= yscale;
	width *= xscale;
	height *= yscale;

	/*p*/glColor4ub( 255, 255, 255, 255 );
	GL_SetRenderMode( kRenderTransTexture );
	R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, cls.pauseIcon );
}

void CL_DrawHUD( int state )
{
	if( state == CL_ACTIVE && !cl.video_prepped )
		state = CL_LOADING;

	if( state == CL_ACTIVE && cl.paused )
		state = CL_PAUSED;

	switch( state )
	{
	case CL_ACTIVE:
		if( !cl.intermission )
			CL_DrawScreenFade ();
		CL_DrawCrosshair ();
		CL_DrawCenterPrint ();
		clgame.dllFuncs.pfnRedraw( cl.time, cl.intermission );
		if( cl.intermission ) CL_DrawScreenFade ();
		break;
	case CL_PAUSED:
		CL_DrawScreenFade ();
		CL_DrawCrosshair ();
		CL_DrawCenterPrint ();
		clgame.dllFuncs.pfnRedraw( cl.time, cl.intermission );
		CL_DrawPause();
		break;
	case CL_LOADING:
		CL_DrawLoading( scr_loading->value );
		break;
	case CL_CHANGELEVEL:
		if( cls.draw_changelevel )
		{
			CL_DrawLoading( 100.0f );
			cls.draw_changelevel = false;
		}
		break;
	}
}

void CL_LinkUserMessage( char *pszName, const int svc_num, int iSize )
{
	int	i;

	if( !pszName || !*pszName )
		Host_Error( "CL_LinkUserMessage: bad message name\n" );

	if( svc_num <= svc_lastmsg )
		Host_Error( "CL_LinkUserMessage: tried to hook a system message \"%s\"\n", svc_strings[svc_num] );	

	// see if already hooked
	for( i = 0; i < MAX_USER_MESSAGES && clgame.msg[i].name[0]; i++ )
	{
		// NOTE: no check for DispatchFunc, check only name
		if( !Q_stricmp( clgame.msg[i].name, pszName ))
		{
			clgame.msg[i].number = svc_num;
			clgame.msg[i].size = iSize;
			return;
		}
	}

	if( i == MAX_USER_MESSAGES ) 
	{
		Host_Error( "CL_LinkUserMessage: MAX_USER_MESSAGES hit!\n" );
		return;
	}

	// register new message without DispatchFunc, so we should parse it properly
	Q_strncpy( clgame.msg[i].name, pszName, sizeof( clgame.msg[i].name ));
	clgame.msg[i].number = svc_num;
	clgame.msg[i].size = iSize;
}

void CL_FreeEntity( cl_entity_t *pEdict )
{
	Assert( pEdict != NULL );
	R_RemoveEfrags( pEdict );
	CL_KillDeadBeams( pEdict );
}

void CL_ClearWorld( void )
{
	cl_entity_t	*world;

	world = clgame.entities;
	world->curstate.modelindex = 1;	// world model
	world->curstate.solid = SOLID_BSP;
	world->curstate.movetype = MOVETYPE_PUSH;
	world->model = cl.worldmodel;
	world->index = 0;

	clgame.ds.cullMode = GL_FRONT;
	clgame.numStatics = 0;
}

void CL_InitEdicts( void )
{
	Assert( clgame.entities == NULL );

	if( !clgame.mempool ) return; // Host_Error without client

	CL_UPDATE_BACKUP = ( cl.maxclients == 1 ) ? SINGLEPLAYER_BACKUP : MULTIPLAYER_BACKUP;
	cls.num_client_entities = CL_UPDATE_BACKUP * NUM_PACKET_ENTITIES;
	cls.packet_entities = Mem_Realloc( clgame.mempool, cls.packet_entities, sizeof( entity_state_t ) * cls.num_client_entities );
	clgame.entities = Mem_Calloc( clgame.mempool, sizeof( cl_entity_t ) * clgame.maxEntities );
	clgame.static_entities = Mem_Calloc( clgame.mempool, sizeof( cl_entity_t ) * MAX_STATIC_ENTITIES );
	clgame.numStatics = 0;

	if(( clgame.maxRemapInfos - 1 ) != clgame.maxEntities )
	{
		CL_ClearAllRemaps (); // purge old remap info
		clgame.maxRemapInfos = clgame.maxEntities + 1; 
		clgame.remap_info = (remap_info_t **)Mem_Calloc( clgame.mempool, sizeof( remap_info_t* ) * clgame.maxRemapInfos );
	}

	if( clgame.drawFuncs.R_ProcessEntData != NULL )
	{
		// let the client.dll free custom data
		clgame.drawFuncs.R_ProcessEntData( true );
	}
}

void CL_FreeEdicts( void )
{
	if( clgame.drawFuncs.R_ProcessEntData != NULL )
	{
		// let the client.dll free custom data
		clgame.drawFuncs.R_ProcessEntData( false );
	}

	if( clgame.entities )
		Mem_Free( clgame.entities );
	clgame.entities = NULL;

	if( clgame.static_entities )
		Mem_Free( clgame.static_entities );
	clgame.static_entities = NULL;

	if( cls.packet_entities )
		Z_Free( cls.packet_entities );

	cls.packet_entities = NULL;
	cls.num_client_entities = 0;
	cls.next_client_entities = 0;
	clgame.numStatics = 0;
}

void CL_ClearEdicts( void )
{
	if( clgame.entities != NULL )
		return;

	// in case we stopped with error
	clgame.maxEntities = 2;
	CL_InitEdicts();
}

/*
==================
CL_ClearSpriteTextures

free studio cache on change level
==================
*/
void CL_ClearSpriteTextures( void )
{
	int	i;

	for( i = 1; i < MAX_CLIENT_SPRITES; i++ )
		clgame.sprites[i].needload = NL_UNREFERENCED;
}

/*
=============
CL_LoadHudSprite

upload sprite frames
=============
*/
static qboolean CL_LoadHudSprite( const char *szSpriteName, model_t *m_pSprite, uint type, uint texFlags )
{
	byte	*buf;
	size_t	size;
	qboolean	loaded;

	Assert( m_pSprite != NULL );

	Q_strncpy( m_pSprite->name, szSpriteName, sizeof( m_pSprite->name ));

	// it's hud sprite, make difference names to prevent free shared textures
	if( type == SPR_CLIENT || type == SPR_HUDSPRITE )
		SetBits( m_pSprite->flags, MODEL_CLIENT );
	m_pSprite->numtexinfo = texFlags; // store texFlags into numtexinfo

	if( FS_FileSize( szSpriteName, false ) == -1 )
	{
		if( cls.state != ca_active && cl.maxclients > 1 )
		{
			// trying to download sprite from server
			CL_AddClientResource( szSpriteName, t_model );
			m_pSprite->needload = NL_NEEDS_LOADED;
			return true;
		}
		else
		{
			Con_Reportf( S_ERROR "%s couldn't load\n", szSpriteName );
			Mod_UnloadSpriteModel( m_pSprite );
			return false;
		}
	}

	buf = FS_LoadFile( szSpriteName, &size, false );
	ASSERT( buf != NULL );

	if( type == SPR_MAPSPRITE )
		Mod_LoadMapSprite( m_pSprite, buf, size, &loaded );
	else Mod_LoadSpriteModel( m_pSprite, buf, &loaded, texFlags );		

	Mem_Free( buf );

	if( !loaded )
	{
		Mod_UnloadSpriteModel( m_pSprite );
		return false;
	}

	m_pSprite->needload = NL_PRESENT;

	return true;
}

/*
=============
CL_LoadSpriteModel

some sprite models is exist only at client: HUD sprites,
tent sprites or overview images
=============
*/
static model_t *CL_LoadSpriteModel( const char *filename, uint type, uint texFlags )
{
	char	name[MAX_QPATH];
	model_t	*mod;
	int	i;

	if( !COM_CheckString( filename ))
	{
		Con_Reportf( S_ERROR "CL_LoadSpriteModel: bad name!\n" );
		return NULL;
	}

	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );

	// slot 0 isn't used
	for( i = 1, mod = clgame.sprites; i < MAX_CLIENT_SPRITES; i++, mod++ )
	{
		if( !Q_stricmp( mod->name, name ))
		{
			if( mod->needload == NL_NEEDS_LOADED )
			{
				if( CL_LoadHudSprite( name, mod, type, texFlags ))
					return mod;
			}

			// prolonge registration
			mod->needload = NL_PRESENT;
			return mod;
		}
	}

	// find a free model slot spot
	for( i = 1, mod = clgame.sprites; i < MAX_CLIENT_SPRITES; i++, mod++ )
		if( !mod->name[0] ) break; // this is a valid spot

	if( i == MAX_CLIENT_SPRITES ) 
	{
		Con_Printf( S_ERROR "MAX_CLIENT_SPRITES limit exceeded (%d)\n", MAX_CLIENT_SPRITES );
		return NULL;
	}

	// load new map sprite
	if( CL_LoadHudSprite( name, mod, type, texFlags ))
		return mod;
	return NULL;
}

/*
=============
CL_LoadClientSprite

load sprites for temp ents
=============
*/
model_t *CL_LoadClientSprite( const char *filename )
{
	return CL_LoadSpriteModel( filename, SPR_CLIENT, 0 );
}

/*
===============================================================================
	CGame Builtin Functions

===============================================================================
*/
/*
=========
pfnSPR_LoadExt

=========
*/
HSPRITE pfnSPR_LoadExt( const char *szPicName, uint texFlags )
{
	model_t	*spr;

	if(( spr = CL_LoadSpriteModel( szPicName, SPR_CLIENT, texFlags )) == NULL )
		return 0;

	return (spr - clgame.sprites); // return index
}

/*
=========
pfnSPR_Load

=========
*/
HSPRITE pfnSPR_Load( const char *szPicName )
{
	model_t	*spr;

	if(( spr = CL_LoadSpriteModel( szPicName, SPR_HUDSPRITE, 0 )) == NULL )
		return 0;

	return (spr - clgame.sprites); // return index
}

/*
=============
CL_GetSpritePointer

=============
*/
const model_t *CL_GetSpritePointer( HSPRITE hSprite )
{
	model_t	*mod;

	if( hSprite <= 0 || hSprite >= MAX_CLIENT_SPRITES )
		return NULL; // bad image
	mod = &clgame.sprites[hSprite];

	if( mod->needload == NL_NEEDS_LOADED )
	{
		int	type = FBitSet( mod->flags, MODEL_CLIENT ) ? SPR_HUDSPRITE : SPR_MAPSPRITE;

		if( CL_LoadHudSprite( mod->name, mod, type, mod->numtexinfo ))
			return mod;
	}

	if( mod->mempool )
	{
		mod->needload = NL_PRESENT;
		return mod;
	}

	return NULL;
}

/*
=========
pfnSPR_Frames

=========
*/
static int pfnSPR_Frames( HSPRITE hPic )
{
	int	numFrames;

	R_GetSpriteParms( NULL, NULL, &numFrames, 0, CL_GetSpritePointer( hPic ));

	return numFrames;
}

/*
=========
pfnSPR_Height

=========
*/
static int pfnSPR_Height( HSPRITE hPic, int frame )
{
	int	sprHeight;

	R_GetSpriteParms( NULL, &sprHeight, NULL, frame, CL_GetSpritePointer( hPic ));

	return sprHeight;
}

/*
=========
pfnSPR_Width

=========
*/
static int pfnSPR_Width( HSPRITE hPic, int frame )
{
	int	sprWidth;

	R_GetSpriteParms( &sprWidth, NULL, NULL, frame, CL_GetSpritePointer( hPic ));

	return sprWidth;
}

/*
=========
pfnSPR_Set

=========
*/
static void pfnSPR_Set( HSPRITE hPic, int r, int g, int b )
{
	clgame.ds.pSprite = CL_GetSpritePointer( hPic );
	clgame.ds.spriteColor[0] = bound( 0, r, 255 );
	clgame.ds.spriteColor[1] = bound( 0, g, 255 );
	clgame.ds.spriteColor[2] = bound( 0, b, 255 );
	clgame.ds.spriteColor[3] = 255;
}

/*
=========
pfnSPR_Draw

=========
*/
static void pfnSPR_Draw( int frame, int x, int y, const wrect_t *prc )
{
	/*p*/glDisable( GL_BLEND );

	SPR_DrawGeneric( frame, x, y, -1, -1, prc );
}

/*
=========
pfnSPR_DrawHoles

=========
*/
static void pfnSPR_DrawHoles( int frame, int x, int y, const wrect_t *prc )
{
	/*p*/glEnable( GL_ALPHA_TEST );
	/*p*/glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	/*p*/glEnable( GL_BLEND );

	SPR_DrawGeneric( frame, x, y, -1, -1, prc );

	/*p*/glDisable( GL_ALPHA_TEST );
	/*p*/glDisable( GL_BLEND );
}

/*
=========
pfnSPR_DrawAdditive

=========
*/
static void pfnSPR_DrawAdditive( int frame, int x, int y, const wrect_t *prc )
{
	/*p*/glEnable( GL_BLEND );
	/*p*/glBlendFunc( GL_ONE, GL_ONE );

	SPR_DrawGeneric( frame, x, y, -1, -1, prc );

	/*p*/glDisable( GL_BLEND );
}

/*
=========
pfnSPR_GetList

for parsing half-life scripts - hud.txt etc
=========
*/
static client_sprite_t *pfnSPR_GetList( char *psz, int *piCount )
{
	cached_spritelist_t	*pEntry = &clgame.sprlist[0];
	int		slot, index, numSprites = 0;
	char		*afile, *pfile;
	string		token;

	if( piCount ) *piCount = 0;

	// see if already in list
	// NOTE: client.dll is cache hud.txt but reparse weapon lists again and again
	// obviously there a memory leak by-design. Cache the sprite lists to prevent it
	for( slot = 0; slot < MAX_CLIENT_SPRITES && pEntry->szListName[0]; slot++ )
	{
		pEntry = &clgame.sprlist[slot];

		if( !Q_stricmp( pEntry->szListName, psz ))
		{
			if( piCount ) *piCount = pEntry->count;
			return pEntry->pList;
		}
	}

	if( slot == MAX_CLIENT_SPRITES )
	{
		Con_Printf( S_ERROR "SPR_GetList: overflow cache!\n" );
		return NULL;
          }

	if( !clgame.itemspath[0] )	// typically it's sprites\*.txt
		COM_ExtractFilePath( psz, clgame.itemspath );

	afile = FS_LoadFile( psz, NULL, false );
	if( !afile ) return NULL;

	pfile = afile;
	pfile = COM_ParseFile( pfile, token );          
	numSprites = Q_atoi( token );

	Q_strncpy( pEntry->szListName, psz, sizeof( pEntry->szListName ));

	// name, res, pic, x, y, w, h
	pEntry->pList = Mem_Calloc( cls.mempool, sizeof( client_sprite_t ) * numSprites );

	for( index = 0; index < numSprites; index++ )
	{
		if(( pfile = COM_ParseFile( pfile, token )) == NULL )
			break;

		Q_strncpy( pEntry->pList[index].szName, token, sizeof( pEntry->pList[0].szName ));

		// read resolution
		pfile = COM_ParseFile( pfile, token );
		pEntry->pList[index].iRes = Q_atoi( token );

		// read spritename
		pfile = COM_ParseFile( pfile, token );
		Q_strncpy( pEntry->pList[index].szSprite, token, sizeof( pEntry->pList[0].szSprite ));

		// parse rectangle
		pfile = COM_ParseFile( pfile, token );
		pEntry->pList[index].rc.left = Q_atoi( token );

		pfile = COM_ParseFile( pfile, token );
		pEntry->pList[index].rc.top = Q_atoi( token );

		pfile = COM_ParseFile( pfile, token );
		pEntry->pList[index].rc.right = pEntry->pList[index].rc.left + Q_atoi( token );

		pfile = COM_ParseFile( pfile, token );
		pEntry->pList[index].rc.bottom = pEntry->pList[index].rc.top + Q_atoi( token );

		pEntry->count++;
	}

	if( index < numSprites )
		Con_DPrintf( S_WARN "unexpected end of %s (%i should be %i)\n", psz, numSprites, index );
	if( piCount ) *piCount = pEntry->count;
	Mem_Free( afile );

	return pEntry->pList;
}

/*
=============
CL_FillRGBA

=============
*/
void CL_FillRGBA( int x, int y, int w, int h, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );

	SPR_AdjustSize( (float *)&x, (float *)&y, (float *)&w, (float *)&h );

	/*p*/glDisable( GL_TEXTURE_2D );
	/*p*/glEnable( GL_BLEND );
	/*p*/glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	/*p*/glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	/*p*/glColor4f( r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f );

	/*p*/glBegin( GL_QUADS );
		/*p*/glVertex2f( x, y );
		/*p*/glVertex2f( x + w, y );
		/*p*/glVertex2f( x + w, y + h );
		/*p*/glVertex2f( x, y + h );
	/*p*/glEnd ();

	/*p*/glColor3f( 1.0f, 1.0f, 1.0f );
	/*p*/glEnable( GL_TEXTURE_2D );
	/*p*/glDisable( GL_BLEND );
}

/*
=============
pfnGetScreenInfo

get actual screen info
=============
*/
static int pfnGetScreenInfo( SCREENINFO *pscrinfo )
{
	// setup screen info
	clgame.scrInfo.iSize = sizeof( clgame.scrInfo );
	clgame.scrInfo.iFlags = SCRINFO_SCREENFLASH;

	if( Cvar_VariableInteger( "hud_scale" ))
	{
		if( glState.width < 640 )
		{
			// virtual screen space 320x200
			clgame.scrInfo.iWidth = 320;
			clgame.scrInfo.iHeight = 200;
		}
		else
		{
			// virtual screen space 640x480
			clgame.scrInfo.iWidth = 640;
			clgame.scrInfo.iHeight = 480;
		}
		clgame.scrInfo.iFlags |= SCRINFO_STRETCHED;
	}
	else
	{
		clgame.scrInfo.iWidth = glState.width;
		clgame.scrInfo.iHeight = glState.height;
		clgame.scrInfo.iFlags &= ~SCRINFO_STRETCHED;
	}

	if( !pscrinfo ) return 0;

	if( pscrinfo->iSize != clgame.scrInfo.iSize )
		clgame.scrInfo.iSize = pscrinfo->iSize;

	// copy screeninfo out
	memcpy( pscrinfo, &clgame.scrInfo, clgame.scrInfo.iSize );

	return 1;
}

/*
=============
pfnSetCrosshair

setup crosshair
=============
*/
static void pfnSetCrosshair( HSPRITE hspr, wrect_t rc, int r, int g, int b )
{
	clgame.ds.rgbaCrosshair[0] = (byte)r;
	clgame.ds.rgbaCrosshair[1] = (byte)g;
	clgame.ds.rgbaCrosshair[2] = (byte)b;
	clgame.ds.rgbaCrosshair[3] = (byte)0xFF;
	clgame.ds.pCrosshair = CL_GetSpritePointer( hspr );
	clgame.ds.rcCrosshair = rc;
}

/*
=============
pfnHookUserMsg

=============
*/
static int pfnHookUserMsg( const char *pszName, pfnUserMsgHook pfn )
{
	int	i;

	// ignore blank names or invalid callbacks
	if( !pszName || !*pszName || !pfn )
		return 0;	

	for( i = 0; i < MAX_USER_MESSAGES && clgame.msg[i].name[0]; i++ )
	{
		// see if already hooked
		if( !Q_stricmp( clgame.msg[i].name, pszName ))
			return 1;
	}

	if( i == MAX_USER_MESSAGES ) 
	{
		Host_Error( "HookUserMsg: MAX_USER_MESSAGES hit!\n" );
		return 0;
	}

	// hook new message
	Q_strncpy( clgame.msg[i].name, pszName, sizeof( clgame.msg[i].name ));
	clgame.msg[i].func = pfn;

	return 1;
}

/*
=============
pfnServerCmd

=============
*/
static int pfnServerCmd( const char *szCmdString )
{
	string	buf;

	if( !COM_CheckString( szCmdString ))
		return 0;

	// just like the client typed "cmd xxxxx" at the console
	Q_snprintf( buf, sizeof( buf ) - 1, "cmd %s\n", szCmdString );
	Cbuf_AddText( buf );

	return 1;
}

/*
=============
pfnClientCmd

=============
*/
static int pfnClientCmd( const char *szCmdString )
{
	if( !COM_CheckString( szCmdString ))
		return 0;

	if( cls.initialized )
	{
		Cbuf_AddText( szCmdString );
		Cbuf_AddText( "\n" );
	}
	else
	{
		// will exec later
		Q_strncat( host.deferred_cmd, va( "%s\n", szCmdString ), sizeof( host.deferred_cmd )); 
	}

	return 1;
}

/*
=============
pfnGetPlayerInfo

=============
*/
static void pfnGetPlayerInfo( int ent_num, hud_player_info_t *pinfo )
{
	player_info_t	*player;

	ent_num -= 1; // player list if offset by 1 from ents

	if( ent_num >= cl.maxclients || ent_num < 0 || !cl.players[ent_num].name[0] )
	{
		pinfo->name = NULL;
		pinfo->thisplayer = false;
		return;
	}

	player = &cl.players[ent_num];
	pinfo->thisplayer = ( ent_num == cl.playernum ) ? true : false;
	pinfo->name = player->name;
	pinfo->model = player->model;
	pinfo->spectator = player->spectator;		
	pinfo->ping = player->ping;
	pinfo->packetloss = player->packet_loss;
	pinfo->topcolor = player->topcolor;
	pinfo->bottomcolor = player->bottomcolor;
}

/*
=============
pfnPlaySoundByName

=============
*/
static void pfnPlaySoundByName( const char *szSound, float volume )
{
	int hSound = S_RegisterSound( szSound );
	S_StartSound( NULL, cl.viewentity, CHAN_ITEM, hSound, volume, ATTN_NORM, PITCH_NORM, SND_STOP_LOOPING );
}

/*
=============
pfnPlaySoundByIndex

=============
*/
static void pfnPlaySoundByIndex( int iSound, float volume )
{
	int hSound;

	// make sure what we in-bounds
	iSound = bound( 0, iSound, MAX_SOUNDS );
	hSound = cl.sound_index[iSound];
	if( !hSound ) return;

	S_StartSound( NULL, cl.viewentity, CHAN_ITEM, hSound, volume, ATTN_NORM, PITCH_NORM, SND_STOP_LOOPING );
}

/*
=============
pfnTextMessageGet

returns specified message from titles.txt
=============
*/
client_textmessage_t *CL_TextMessageGet( const char *pName )
{
	int	i;

	// first check internal messages
	for( i = 0; i < MAX_TEXTCHANNELS; i++ )
	{
		if( !Q_strcmp( pName, va( TEXT_MSGNAME, i )))
			return cl_textmessage + i;
	}

	// find desired message
	for( i = 0; i < clgame.numTitles; i++ )
	{
		if( !Q_stricmp( pName, clgame.titles[i].pName ))
			return clgame.titles + i;
	}
	return NULL; // found nothing
}

/*
=============
pfnDrawCharacter

returns drawed chachter width (in real screen pixels)
=============
*/
static int pfnDrawCharacter( int x, int y, int number, int r, int g, int b )
{
	if( !cls.creditsFont.valid )
		return 0;

	number &= 255;

	if( number < 32 ) return 0;
	if( y < -clgame.scrInfo.iCharHeight )
		return 0;

	clgame.ds.adjust_size = true;
	pfnPIC_Set( cls.creditsFont.hFontTexture, r, g, b, 255 );
	pfnPIC_DrawAdditive( x, y, -1, -1, &cls.creditsFont.fontRc[number] );
	clgame.ds.adjust_size = false;

	return clgame.scrInfo.charWidths[number];
}

/*
=============
pfnDrawConsoleString

drawing string like a console string 
=============
*/
int pfnDrawConsoleString( int x, int y, char *string )
{
	int	drawLen;

	if( !COM_CheckString( string ))
		return 0; // silent ignore
	Con_SetFont( con_fontsize->value );

	clgame.ds.adjust_size = true;
	drawLen = Con_DrawString( x, y, string, clgame.ds.textColor );
	MakeRGBA( clgame.ds.textColor, 255, 255, 255, 255 );
	clgame.ds.adjust_size = false;

	Con_RestoreFont();

	return (x + drawLen); // exclude color prexfixes
}

/*
=============
pfnDrawSetTextColor

set color for anything
=============
*/
void pfnDrawSetTextColor( float r, float g, float b )
{
	// bound color and convert to byte
	clgame.ds.textColor[0] = (byte)bound( 0, r * 255, 255 );
	clgame.ds.textColor[1] = (byte)bound( 0, g * 255, 255 );
	clgame.ds.textColor[2] = (byte)bound( 0, b * 255, 255 );
	clgame.ds.textColor[3] = (byte)0xFF;
}

/*
=============
pfnDrawConsoleStringLen

compute string length in screen pixels
=============
*/
void pfnDrawConsoleStringLen( const char *pText, int *length, int *height )
{
	Con_SetFont( con_fontsize->value );
	Con_DrawStringLen( pText, length, height );
	Con_RestoreFont();
}

/*
=============
pfnConsolePrint

prints directly into console (can skip notify)
=============
*/
static void pfnConsolePrint( const char *string )
{
	Con_Printf( "%s", string );
}

/*
=============
pfnCenterPrint

holds and fade message at center of screen
like trigger_multiple message in q1
=============
*/
static void pfnCenterPrint( const char *string )
{
	CL_CenterPrint( string, 0.25f );
}

/*
=========
GetWindowCenterX

=========
*/
static int pfnGetWindowCenterX( void )
{
	return host.window_center_x;
}

/*
=========
GetWindowCenterY

=========
*/
static int pfnGetWindowCenterY( void )
{
	return host.window_center_y;
}

/*
=============
pfnGetViewAngles

return interpolated angles from previous frame
=============
*/
static void pfnGetViewAngles( float *angles )
{
	if( angles ) VectorCopy( cl.viewangles, angles );
}

/*
=============
pfnSetViewAngles

return interpolated angles from previous frame
=============
*/
static void pfnSetViewAngles( float *angles )
{
	if( angles ) VectorCopy( angles, cl.viewangles );
}

/*
=============
pfnPhysInfo_ValueForKey

=============
*/
static const char* pfnPhysInfo_ValueForKey( const char *key )
{
	return Info_ValueForKey( cls.physinfo, key );
}

/*
=============
pfnServerInfo_ValueForKey

=============
*/
static const char* pfnServerInfo_ValueForKey( const char *key )
{
	return Info_ValueForKey( cl.serverinfo, key );
}

/*
=============
pfnGetClientMaxspeed

value that come from server
=============
*/
static float pfnGetClientMaxspeed( void )
{
	return cl.local.maxspeed;
}

/*
=============
pfnGetMousePosition

=============
*/
static void pfnGetMousePosition( int *mx, int *my )
{
#ifndef _XBOX
	POINT	curpos;

	GetCursorPos( &curpos );
	ScreenToClient( host.hWnd, &curpos );

	if( mx ) *mx = curpos.x;
	if( my ) *my = curpos.y;
#endif
}

/*
=============
pfnIsNoClipping

=============
*/
int pfnIsNoClipping( void )
{
	return ( cl.frames[cl.parsecountmod].playerstate[cl.playernum].movetype == MOVETYPE_NOCLIP );
}

/*
=============
pfnGetViewModel

=============
*/
static cl_entity_t* pfnGetViewModel( void )
{
	return &clgame.viewent;
}

/*
=============
pfnGetClientTime

=============
*/
static float pfnGetClientTime( void )
{
	return cl.time;
}

/*
=============
pfnCalcShake

=============
*/
void pfnCalcShake( void )
{
	int	i;
	float	fraction, freq;
	float	localAmp;

	if( clgame.shake.time == 0 )
		return;

	if(( cl.time > clgame.shake.time ) || clgame.shake.amplitude <= 0 || clgame.shake.frequency <= 0 )
	{
		memset( &clgame.shake, 0, sizeof( clgame.shake ));
		return;
	}

	if( cl.time > clgame.shake.next_shake )
	{
		// higher frequency means we recalc the extents more often and perturb the display again
		clgame.shake.next_shake = cl.time + ( 1.0f / clgame.shake.frequency );

		// compute random shake extents (the shake will settle down from this)
		for( i = 0; i < 3; i++ )
			clgame.shake.offset[i] = COM_RandomFloat( -clgame.shake.amplitude, clgame.shake.amplitude );
		clgame.shake.angle = COM_RandomFloat( -clgame.shake.amplitude * 0.25f, clgame.shake.amplitude * 0.25f );
	}

	// ramp down amplitude over duration (fraction goes from 1 to 0 linearly with slope 1/duration)
	fraction = ( clgame.shake.time - cl.time ) / clgame.shake.duration;

	// ramp up frequency over duration
	if( fraction )
	{
		freq = ( clgame.shake.frequency / fraction );
	}
	else
	{
		freq = 0;
	}

	// square fraction to approach zero more quickly
	fraction *= fraction;

	// Sine wave that slowly settles to zero
	fraction = fraction * sin( cl.time * freq );
	
	// add to view origin
	VectorScale( clgame.shake.offset, fraction, clgame.shake.applied_offset );

	// add to roll
	clgame.shake.applied_angle = clgame.shake.angle * fraction;

	// drop amplitude a bit, less for higher frequency shakes
	localAmp = clgame.shake.amplitude * ( host.frametime / ( clgame.shake.duration * clgame.shake.frequency ));
	clgame.shake.amplitude -= localAmp;
}

/*
=============
pfnApplyShake

=============
*/
void pfnApplyShake( float *origin, float *angles, float factor )
{
	if( origin ) VectorMA( origin, factor, clgame.shake.applied_offset, origin );
	if( angles ) angles[ROLL] += clgame.shake.applied_angle * factor;
}
	
/*
=============
pfnIsSpectateOnly

=============
*/
static int pfnIsSpectateOnly( void )
{
	return (cls.spectator != 0);
}

/*
=============
pfnPointContents

=============
*/
static int pfnPointContents( const float *p, int *truecontents )
{
	int	cont, truecont;

	truecont = cont = PM_PointContents( clgame.pmove, p );
	if( truecontents ) *truecontents = truecont;

	if( cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN )
		cont = CONTENTS_WATER;
	return cont;
}

/*
=============
pfnTraceLine

=============
*/
static pmtrace_t *pfnTraceLine( float *start, float *end, int flags, int usehull, int ignore_pe )
{
	static pmtrace_t	tr;
	int		old_usehull;

	old_usehull = clgame.pmove->usehull;
	clgame.pmove->usehull = usehull;	

	switch( flags )
	{
	case PM_TRACELINE_PHYSENTSONLY:
		tr = PM_PlayerTraceExt( clgame.pmove, start, end, 0, clgame.pmove->numphysent, clgame.pmove->physents, ignore_pe, NULL );
		break;
	case PM_TRACELINE_ANYVISIBLE:
		tr = PM_PlayerTraceExt( clgame.pmove, start, end, 0, clgame.pmove->numvisent, clgame.pmove->visents, ignore_pe, NULL );
		break;
	}

	clgame.pmove->usehull = old_usehull;

	return &tr;
}

static void pfnPlaySoundByNameAtLocation( char *szSound, float volume, float *origin )
{
	int hSound = S_RegisterSound( szSound );
	S_StartSound( origin, 0, CHAN_AUTO, hSound, volume, ATTN_NORM, PITCH_NORM, 0 );
}

/*
=============
pfnPrecacheEvent

=============
*/
static word pfnPrecacheEvent( int type, const char* psz )
{
	return CL_EventIndex( psz );
}

/*
=============
pfnHookEvent

=============
*/
static void pfnHookEvent( const char *filename, pfnEventHook pfn )
{
	char		name[64];
	cl_user_event_t	*ev;
	int		i;

	// ignore blank names
	if( !filename || !*filename )
		return;	

	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );

	// find an empty slot
	for( i = 0; i < MAX_EVENTS; i++ )
	{
		ev = clgame.events[i];		
		if( !ev ) break;

		if( !Q_stricmp( name, ev->name ) && ev->func != NULL )
		{
			Con_Reportf( S_WARN "CL_HookEvent: %s already hooked!\n", name );
			return;
		}
	}

	CL_RegisterEvent( i, name, pfn );
}

/*
=============
pfnKillEvent

=============
*/
static void pfnKillEvents( int entnum, const char *eventname )
{
	int		i;
	event_state_t	*es;
	event_info_t	*ei;
	int		eventIndex = CL_EventIndex( eventname );

	if( eventIndex < 0 || eventIndex >= MAX_EVENTS )
		return;

	if( entnum < 0 || entnum > clgame.maxEntities )
		return;

	es = &cl.events;

	// find all events with specified index and kill it
	for( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		ei = &es->ei[i];

		if( ei->index == eventIndex && ei->entity_index == entnum )
		{
			CL_ResetEvent( ei );
			break;
		}
	}
}

/*
=============
pfnPlaySound

=============
*/
void pfnPlaySound( int ent, float *org, int chan, const char *samp, float vol, float attn, int flags, int pitch )
{
	S_StartSound( org, ent, chan, S_RegisterSound( samp ), vol, attn, pitch, flags );
}

/*
=============
CL_FindModelIndex

=============
*/
int CL_FindModelIndex( const char *m )
{
	char		filepath[MAX_QPATH];
	static float	lasttimewarn;
	int		i;

	if( !COM_CheckString( m ))
		return 0;

	Q_strncpy( filepath, m, sizeof( filepath ));
	COM_FixSlashes( filepath );

	for( i = 0; i < cl.nummodels; i++ )
	{
		if( !cl.models[i+1] )
			continue;

		if( !Q_stricmp( cl.models[i+1]->name, filepath ))
			return i+1;
	}

	if( lasttimewarn < host.realtime )
	{
		// tell user about problem (but don't spam console)
		Con_Printf( S_ERROR "%s not precached\n", filepath );
		lasttimewarn = host.realtime + 1.0f;
	}

	return 0;
}

/*
=============
pfnIsLocal

=============
*/
int pfnIsLocal( int playernum )
{
	if( playernum == cl.playernum )
		return true;
	return false;
}

/*
=============
pfnLocalPlayerDucking

=============
*/
int pfnLocalPlayerDucking( void )
{
	return (cl.local.usehull == 1) ? true : false;
}

/*
=============
pfnLocalPlayerViewheight

=============
*/
void pfnLocalPlayerViewheight( float *view_ofs )
{
	if( view_ofs ) VectorCopy( cl.viewheight, view_ofs );		
}

/*
=============
pfnLocalPlayerBounds

=============
*/
void pfnLocalPlayerBounds( int hull, float *mins, float *maxs )
{
	if( hull >= 0 && hull < 4 )
	{
		if( mins ) VectorCopy( clgame.pmove->player_mins[hull], mins );
		if( maxs ) VectorCopy( clgame.pmove->player_maxs[hull], maxs );
	}
}

/*
=============
pfnIndexFromTrace

=============
*/
int pfnIndexFromTrace( struct pmtrace_s *pTrace )
{
	if( pTrace->ent >= 0 && pTrace->ent < clgame.pmove->numphysent )
	{
		// return cl.entities number
		return clgame.pmove->physents[pTrace->ent].info;
	}
	return -1;
}

/*
=============
pfnGetPhysent

=============
*/
physent_t *pfnGetPhysent( int idx )
{
	if( idx >= 0 && idx < clgame.pmove->numphysent )
	{
		// return physent
		return &clgame.pmove->physents[idx];
	}
	return NULL;
}

/*
=============
pfnGetVisent

=============
*/
physent_t *pfnGetVisent( int idx )
{
	if( idx >= 0 && idx < clgame.pmove->numvisent )
	{
		// return physent
		return &clgame.pmove->visents[idx];
	}
	return NULL;
}

/*
=============
pfnSetTraceHull

=============
*/
void CL_SetTraceHull( int hull )
{
	clgame.pmove->usehull = bound( 0, hull, 3 );

}

/*
=============
pfnPlayerTrace

=============
*/
void CL_PlayerTrace( float *start, float *end, int traceFlags, int ignore_pe, pmtrace_t *tr )
{
	if( !tr ) return;
	*tr = PM_PlayerTraceExt( clgame.pmove, start, end, traceFlags, clgame.pmove->numphysent, clgame.pmove->physents, ignore_pe, NULL );
}

/*
=============
pfnPlayerTraceExt

=============
*/
void CL_PlayerTraceExt( float *start, float *end, int traceFlags, int (*pfnIgnore)( physent_t *pe ), pmtrace_t *tr )
{
	if( !tr ) return;
	*tr = PM_PlayerTraceExt( clgame.pmove, start, end, traceFlags, clgame.pmove->numphysent, clgame.pmove->physents, -1, pfnIgnore );
}

/*
=============
pfnTraceTexture

=============
*/
static const char *pfnTraceTexture( int ground, float *vstart, float *vend )
{
	physent_t *pe;

	if( ground < 0 || ground >= clgame.pmove->numphysent )
		return NULL; // bad ground

	pe = &clgame.pmove->physents[ground];
	return PM_TraceTexture( pe, vstart, vend );
}

/*
=============
pfnTraceSurface

=============
*/
static struct msurface_s *pfnTraceSurface( int ground, float *vstart, float *vend )
{
	physent_t *pe;

	if( ground < 0 || ground >= clgame.pmove->numphysent )
		return NULL; // bad ground

	pe = &clgame.pmove->physents[ground];
	return PM_TraceSurface( pe, vstart, vend );
}

/*
=============
pfnGetMovevars

=============
*/
static movevars_t *pfnGetMoveVars( void )
{
	return &clgame.movevars;
}
	
/*
=============
pfnStopAllSounds

=============
*/
void pfnStopAllSounds( int ent, int entchannel )
{
	S_StopSound( ent, entchannel, NULL );
}

/*
=============
CL_LoadModel

=============
*/
model_t *CL_LoadModel( const char *modelname, int *index )
{
	int	i;

	if( index ) *index = -1;

	if(( i = CL_FindModelIndex( modelname )) == 0 )
		return NULL;

	if( index ) *index = i;

	return CL_ModelHandle( i );
}

int CL_AddEntity( int entityType, cl_entity_t *pEnt )
{
	if( !pEnt ) return false;

	// clear effects for all temp entities
	if( !pEnt->index ) pEnt->curstate.effects = 0;

	// let the render reject entity without model
	return CL_AddVisibleEntity( pEnt, entityType );
}

/*
=============
pfnGetGameDirectory

=============
*/
const char *pfnGetGameDirectory( void )
{
	static char	szGetGameDir[MAX_SYSPATH];

	Q_sprintf( szGetGameDir, "%s/%s", host.rootdir, GI->gamedir );
	return szGetGameDir;
}

/*
=============
Key_LookupBinding

=============
*/
const char *Key_LookupBinding( const char *pBinding )
{
	return Key_KeynumToString( Key_GetKey( pBinding ));
}

/*
=============
pfnGetLevelName

=============
*/
static const char *pfnGetLevelName( void )
{
	static char	mapname[64];

	if( cls.state >= ca_connected )
		Q_snprintf( mapname, sizeof( mapname ), "maps/%s.bsp", clgame.mapname );
	else mapname[0] = '\0'; // not in game

	return mapname;
}

/*
=============
pfnGetScreenFade

=============
*/
static void pfnGetScreenFade( struct screenfade_s *fade )
{
	if( fade ) *fade = clgame.fade;
}

/*
=============
pfnSetScreenFade

=============
*/
static void pfnSetScreenFade( struct screenfade_s *fade )
{
	if( fade ) clgame.fade = *fade;
}

/*
=============
pfnLoadMapSprite

=============
*/
model_t *pfnLoadMapSprite( const char *filename )
{
	return CL_LoadSpriteModel( filename, SPR_MAPSPRITE, 0 );
}

/*
=============
PlayerInfo_ValueForKey

=============
*/
const char *PlayerInfo_ValueForKey( int playerNum, const char *key )
{
	// find the player
	if(( playerNum > cl.maxclients ) || ( playerNum < 1 ))
		return NULL;

	if( !cl.players[playerNum-1].name[0] )
		return NULL;

	return Info_ValueForKey( cl.players[playerNum-1].userinfo, key );
}

/*
=============
PlayerInfo_SetValueForKey

=============
*/
void PlayerInfo_SetValueForKey( const char *key, const char *value )
{
	convar_t	*var;

	if( !Q_strcmp( Info_ValueForKey( cls.userinfo, key ), value ))
		return; // no changes ?

	var = Cvar_FindVar( key );

	if( var && FBitSet( var->flags, FCVAR_USERINFO ))
	{
		Cvar_DirectSet( var, value );
	}
	else if( Info_SetValueForStarKey( cls.userinfo, key, value, MAX_INFO_STRING ))
	{
		// time to update server copy of userinfo
		CL_ServerCommand( true, "setinfo \"%s\" \"%s\"\n", key, value );
	}
}

/*
=============
pfnGetPlayerUniqueID

=============
*/
qboolean pfnGetPlayerUniqueID( int iPlayer, char playerID[16] )
{
	if( iPlayer < 1 || iPlayer > cl.maxclients )
		return false;

	// make sure there is a player here..
	if( !cl.players[iPlayer-1].userinfo[0] || !cl.players[iPlayer-1].name[0] )
		return false;

	memcpy( playerID, cl.players[iPlayer-1].hashedcdkey, 16 );
	return true;
}

/*
=============
pfnGetTrackerIDForPlayer

obsolete, unused
=============
*/
int pfnGetTrackerIDForPlayer( int playerSlot )
{
	return 0;
}

/*
=============
pfnGetPlayerForTrackerID

obsolete, unused
=============
*/
int pfnGetPlayerForTrackerID( int trackerID )
{
	return 0;
}

/*
=============
pfnServerCmdUnreliable

=============
*/
int pfnServerCmdUnreliable( char *szCmdString )
{
	if( !COM_CheckString( szCmdString ))
		return 0;

	MSG_BeginClientCmd( &cls.datagram, clc_stringcmd );
	MSG_WriteString( &cls.datagram, szCmdString );

	return 1;
}

/*
=============
pfnGetMousePos

=============
*/
void pfnGetMousePos( struct tagPOINT *ppt )
{
#ifndef _XBOX
	GetCursorPos( ppt );
#endif
}

/*
=============
pfnSetMousePos

=============
*/
void pfnSetMousePos( int mx, int my )
{
#ifndef _XBOX
	SetCursorPos( mx, my );
#endif
}

/*
=============
pfnSetMouseEnable

legacy of dinput code
=============
*/
void pfnSetMouseEnable( qboolean fEnable )
{
}

/*
=============
pfnParseFile

handle colon separately
=============
*/
char *pfnParseFile( char *data, char *token )
{
	char	*out;

	host.com_handlecolon = true;
	out = COM_ParseFile( data, token );
	host.com_handlecolon = false;

	return out;
}

/*
=============
pfnGetServerTime

=============
*/
float pfnGetClientOldTime( void )
{
	return cl.oldtime;
}

/*
=============
pfnGetGravity

=============
*/
float pfnGetGravity( void )
{
	return clgame.movevars.gravity;
}

/*
=============
LocalPlayerInfo_ValueForKey

=============
*/
const char *LocalPlayerInfo_ValueForKey( const char* key )
{
	return Info_ValueForKey( cls.userinfo, key );
}

/*
=============
CL_FillRGBABlend

=============
*/
void CL_FillRGBABlend( int x, int y, int w, int h, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );

	SPR_AdjustSize( (float *)&x, (float *)&y, (float *)&w, (float *)&h );

	/*p*/glDisable( GL_TEXTURE_2D );
	/*p*/glEnable( GL_BLEND );
	/*p*/glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	/*p*/glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	/*p*/glColor4f( r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f );

	/*p*/glBegin( GL_QUADS );
		/*p*/glVertex2f( x, y );
		/*p*/glVertex2f( x + w, y );
		/*p*/glVertex2f( x + w, y + h );
		/*p*/glVertex2f( x, y + h );
	/*p*/glEnd ();

	/*p*/glColor3f( 1.0f, 1.0f, 1.0f );
	/*p*/glEnable( GL_TEXTURE_2D );
	/*p*/glDisable( GL_BLEND );
}

/*
=============
pfnGetAppID

=============
*/
int pfnGetAppID( void )
{
	return 70;
}

/*
=================
TriApi implementation

=================
*/
/*
=============
TriRenderMode

set rendermode
=============
*/
void TriRenderMode( int mode )
{
	switch( mode )
	{
	case kRenderNormal:
		/*p*/glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		/*p*/glDisable( GL_BLEND );
		/*p*/glDepthMask( GL_TRUE );
		break;
	case kRenderTransAlpha:
		/*p*/glEnable( GL_BLEND );
		/*p*/glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		/*p*/glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		/*p*/glDepthMask( GL_FALSE );
		break;
	case kRenderTransColor:
	case kRenderTransTexture:
		/*p*/glEnable( GL_BLEND );
		/*p*/glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		break;
	case kRenderGlow:
	case kRenderTransAdd:
		/*p*/glBlendFunc( GL_SRC_ALPHA, GL_ONE );
		/*p*/glEnable( GL_BLEND );
		/*p*/glDepthMask( GL_FALSE );
		break;
	}

	clgame.ds.renderMode = mode;
}

/*
=============
TriBegin

begin triangle sequence
=============
*/
void TriBegin( int mode )
{
	switch( mode )
	{
	case TRI_POINTS:
		mode = GL_POINTS;
		break;
	case TRI_TRIANGLES:
		mode = GL_TRIANGLES;
		break;
	case TRI_TRIANGLE_FAN:
		mode = GL_TRIANGLE_FAN;
		break;
	case TRI_QUADS:
		mode = GL_QUADS;
		break;
	case TRI_LINES:
		mode = GL_LINES;
		break;
	case TRI_TRIANGLE_STRIP:
		mode = GL_TRIANGLE_STRIP;
		break;
	case TRI_QUAD_STRIP:
		mode = GL_QUAD_STRIP;
		break;
	case TRI_POLYGON:
	default:	mode = GL_POLYGON;
		break;
	}

	/*p*/glBegin( mode );
}

/*
=============
TriEnd

draw triangle sequence
=============
*/
void TriEnd( void )
{
	/*p*/glEnd();
}

/*
=============
TriColor4f

=============
*/
void TriColor4f( float r, float g, float b, float a )
{
	if( clgame.ds.renderMode == kRenderTransAlpha )
		/*p*/glColor4ub( r * 255.9f, g * 255.9f, b * 255.9f, a * 255.0f );
	else /*p*/glColor4f( r * a, g * a, b * a, 1.0 );

	clgame.ds.triRGBA[0] = r;
	clgame.ds.triRGBA[1] = g;
	clgame.ds.triRGBA[2] = b;
	clgame.ds.triRGBA[3] = a;
}

/*
=============
TriColor4ub

=============
*/
void TriColor4ub( byte r, byte g, byte b, byte a )
{
	clgame.ds.triRGBA[0] = r * (1.0f / 255.0f);
	clgame.ds.triRGBA[1] = g * (1.0f / 255.0f);
	clgame.ds.triRGBA[2] = b * (1.0f / 255.0f);
	clgame.ds.triRGBA[3] = a * (1.0f / 255.0f);

	/*p*/glColor4f( clgame.ds.triRGBA[0], clgame.ds.triRGBA[1], clgame.ds.triRGBA[2], 1.0f );
}

/*
=============
TriTexCoord2f

=============
*/
void TriTexCoord2f( float u, float v )
{
	/*p*/glTexCoord2f( u, v );
}

/*
=============
TriVertex3fv

=============
*/
void TriVertex3fv( const float *v )
{
	/*p*/glVertex3fv( v );
}

/*
=============
TriVertex3f

=============
*/
void TriVertex3f( float x, float y, float z )
{
	/*p*/glVertex3f( x, y, z );
}

/*
=============
TriBrightness

=============
*/
void TriBrightness( float brightness )
{
	float	r, g, b;

	r = clgame.ds.triRGBA[0] * clgame.ds.triRGBA[3] * brightness;
	g = clgame.ds.triRGBA[1] * clgame.ds.triRGBA[3] * brightness;
	b = clgame.ds.triRGBA[2] * clgame.ds.triRGBA[3] * brightness;

	/*p*/glColor4f( r, g, b, 1.0f );
}

/*
=============
TriCullFace

=============
*/
void TriCullFace( int mode )
{
	switch( mode )
	{
	case TRI_FRONT:
		clgame.ds.cullMode = GL_FRONT;
		break;
	default:
		clgame.ds.cullMode = GL_NONE;
		break;
	}

	GL_Cull( clgame.ds.cullMode );
}

/*
=============
TriSpriteTexture

bind current texture
=============
*/
int TriSpriteTexture( model_t *pSpriteModel, int frame )
{
	int	gl_texturenum;

	if(( gl_texturenum = R_GetSpriteTexture( pSpriteModel, frame )) == 0 )
		return 0;

	if( gl_texturenum <= 0 || gl_texturenum > MAX_TEXTURES )
		gl_texturenum = tr.defaultTexture;

	GL_Bind( GL_TEXTURE0, gl_texturenum );

	return 1;
}

/*
=============
TriWorldToScreen

convert world coordinates (x,y,z) into screen (x, y)
=============
*/
int TriWorldToScreen( float *world, float *screen )
{
	int	retval;

	retval = R_WorldToScreen( world, screen );

	screen[0] =  0.5f * screen[0] * (float)clgame.viewport[2];
	screen[1] = -0.5f * screen[1] * (float)clgame.viewport[3];
	screen[0] += 0.5f * (float)clgame.viewport[2];
	screen[1] += 0.5f * (float)clgame.viewport[3];

	return retval;
}

/*
=============
TriFog

enables global fog on the level
=============
*/
void TriFog( float flFogColor[3], float flStart, float flEnd, int bOn )
{
	// overrided by internal fog
	if( RI.fogEnabled ) return;
	RI.fogCustom = bOn;

	// check for invalid parms
	if( flEnd <= flStart )
	{
		glState.isFogEnabled = false;
		RI.fogCustom = false;
		/*p*/glDisable( GL_FOG );
		return;
	}

	if( RI.fogCustom )
		/*p*/glEnable( GL_FOG );
	else /*p*/glDisable( GL_FOG );

	// copy fog params
	RI.fogColor[0] = flFogColor[0] / 255.0f;
	RI.fogColor[1] = flFogColor[1] / 255.0f;
	RI.fogColor[2] = flFogColor[2] / 255.0f;
	RI.fogStart = flStart;
	RI.fogColor[3] = 1.0f;
	RI.fogDensity = 0.0f;
	RI.fogSkybox = true;
	RI.fogEnd = flEnd;

	/*p*/glFogi( GL_FOG_MODE, GL_LINEAR );
	/*p*/glFogfv( GL_FOG_COLOR, RI.fogColor );
	/*p*/glFogf( GL_FOG_START, RI.fogStart );
	/*p*/glFogf( GL_FOG_END, RI.fogEnd );
	/*p*/glHint( GL_FOG_HINT, GL_NICEST );
}

/*
=============
TriGetMatrix

very strange export
=============
*/
void TriGetMatrix( const int pname, float *matrix )
{
	/*p*/glGetFloatv( pname, matrix );
}

/*
=============
TriBoxInPVS

check box in pvs (absmin, absmax)
=============
*/
int TriBoxInPVS( float *mins, float *maxs )
{
	return Mod_BoxVisible( mins, maxs, Mod_GetCurrentVis( ));
}

/*
=============
TriLightAtPoint

NOTE: dlights are ignored
=============
*/
void TriLightAtPoint( float *pos, float *value )
{
	colorVec	vLightColor;

	if( !pos || !value ) return;

	vLightColor = R_LightPoint( pos );

	value[0] = vLightColor.r;
	value[1] = vLightColor.g;
	value[2] = vLightColor.b;
}

/*
=============
TriColor4fRendermode

Heavy legacy of Quake...
=============
*/
void TriColor4fRendermode( float r, float g, float b, float a, int rendermode )
{
	if( clgame.ds.renderMode == kRenderTransAlpha )
	{
		clgame.ds.triRGBA[3] = a / 255.0f;
		/*p*/glColor4f( r, g, b, a );
	}
	else /*p*/glColor4f( r * a, g * a, b * a, 1.0f );
}

/*
=============
TriForParams

=============
*/
void TriFogParams( float flDensity, int iFogSkybox )
{
	RI.fogDensity = flDensity;
	RI.fogSkybox = iFogSkybox;
}

/*
=================
DemoApi implementation

=================
*/
/*
=================
Demo_IsRecording

=================
*/
static int Demo_IsRecording( void )
{
	return cls.demorecording;
}

/*
=================
Demo_IsPlayingback

=================
*/
static int Demo_IsPlayingback( void )
{
	return cls.demoplayback;
}

/*
=================
Demo_IsTimeDemo

=================
*/
static int Demo_IsTimeDemo( void )
{
	return cls.timedemo;
}

/*
=================
Demo_WriteBuffer

=================
*/
static void Demo_WriteBuffer( int size, byte *buffer )
{
	CL_WriteDemoUserMessage( buffer, size );
}

/*
=================
NetworkApi implementation

=================
*/
/*
=================
NetAPI_InitNetworking

=================
*/
void NetAPI_InitNetworking( void )
{
	NET_Config( true ); // allow remote
}

/*
=================
NetAPI_InitNetworking

=================
*/
void NetAPI_Status( net_status_t *status )
{
	qboolean	connected = false;
	int	packet_loss = 0;

	Assert( status != NULL );

	if( cls.state > ca_disconnected && cls.state != ca_cinematic )
		connected = true;

	if( cls.state == ca_active )
		packet_loss = bound( 0, (int)cls.packet_loss, 100 );

	status->connected = connected;
	status->connection_time = (connected) ? (host.realtime - cls.netchan.connect_time) : 0.0;
	status->latency = (connected) ? cl.frames[cl.parsecountmod].latency : 0.0;
	status->remote_address = cls.netchan.remote_address;
	status->packet_loss = packet_loss;
	status->local_address = net_local;
	status->rate = rate->value;
}

/*
=================
NetAPI_SendRequest

=================
*/
void NetAPI_SendRequest( int context, int request, int flags, double timeout, netadr_t *remote_address, net_api_response_func_t response )
{
	net_request_t	*nr = NULL;
	string		req;
	int		i;

	if( !response )
	{
		Con_DPrintf( S_ERROR "Net_SendRequest: no callbcak specified for request with context %i!\n", context );
		return;
	}

	if( remote_address->type >= NA_IPX )
		return; // IPX no longer support

	// find a free request
	for( i = 0; i < MAX_REQUESTS; i++ )
	{
		nr = &clgame.net_requests[i];
		if( !nr->pfnFunc ) break;
	}

	if( i == MAX_REQUESTS )
	{
		double	max_timeout = 0;

		// no free requests? use oldest
		for( i = 0, nr = NULL; i < MAX_REQUESTS; i++ )
		{
			if(( host.realtime - clgame.net_requests[i].timesend ) > max_timeout )
			{
				max_timeout = host.realtime - clgame.net_requests[i].timesend;
				nr = &clgame.net_requests[i];
			}
		}
	}

	Assert( nr != NULL );

	// clear slot
	memset( nr, 0, sizeof( *nr ));

	// create a new request
	nr->timesend = host.realtime;
	nr->timeout = nr->timesend + timeout;
	nr->pfnFunc = response;
	nr->resp.context = context;
	nr->resp.type = request;	
	nr->resp.remote_address = *remote_address; 
	nr->flags = flags;

	if( request == NETAPI_REQUEST_SERVERLIST )
	{
		char	fullquery[512] = "1\xFF" "0.0.0.0:0\0" "\\gamedir\\";

		// make sure what port is specified
		if( !nr->resp.remote_address.port ) nr->resp.remote_address.port = MSG_BigShort( PORT_MASTER );

		// grab the list from the master server
		Q_strcpy( &fullquery[22], GI->gamedir );
		NET_SendPacket( NS_CLIENT, Q_strlen( GI->gamedir ) + 23, fullquery, nr->resp.remote_address );
		clgame.request_type = NET_REQUEST_CLIENT;
		clgame.master_request = nr; // holds the master request unitl the master acking
	}
	else
	{
		// local servers request
		Q_snprintf( req, sizeof( req ), "netinfo %i %i %i", PROTOCOL_VERSION, context, request );
		Netchan_OutOfBandPrint( NS_CLIENT, nr->resp.remote_address, req );
	}
}

/*
=================
NetAPI_CancelRequest

=================
*/
void NetAPI_CancelRequest( int context )
{
	net_request_t	*nr;
	int		i;

	// find a specified request
	for( i = 0; i < MAX_REQUESTS; i++ )
	{
		nr = &clgame.net_requests[i];

		if( clgame.net_requests[i].resp.context == context )
		{
			if( nr->pfnFunc )
			{
				SetBits( nr->resp.error, NET_ERROR_TIMEOUT );
				nr->resp.ping = host.realtime - nr->timesend;
				nr->pfnFunc( &nr->resp );
                              }

			if( clgame.net_requests[i].resp.type == NETAPI_REQUEST_SERVERLIST && &clgame.net_requests[i] == clgame.master_request )
			{
				if( clgame.request_type == NET_REQUEST_CLIENT )
					clgame.request_type = NET_REQUEST_CANCEL;
				clgame.master_request = NULL;
			}

			memset( &clgame.net_requests[i], 0, sizeof( net_request_t ));
			break;
		}
	}
}

/*
=================
NetAPI_CancelAllRequests

=================
*/
void NetAPI_CancelAllRequests( void )
{
	net_request_t	*nr;
	int		i;

	// tell the user about cancel
	for( i = 0; i < MAX_REQUESTS; i++ )
	{
		nr = &clgame.net_requests[i];
		if( !nr->pfnFunc ) continue;	// not used
		SetBits( nr->resp.error, NET_ERROR_TIMEOUT );
		nr->resp.ping = host.realtime - nr->timesend;
		nr->pfnFunc( &nr->resp );
	}

	memset( clgame.net_requests, 0, sizeof( clgame.net_requests ));
	clgame.request_type = NET_REQUEST_CANCEL;
	clgame.master_request = NULL;
}

/*
=================
NetAPI_AdrToString

=================
*/
char *NetAPI_AdrToString( netadr_t *a )
{
	return NET_AdrToString( *a );
}

/*
=================
NetAPI_CompareAdr

=================
*/
int NetAPI_CompareAdr( netadr_t *a, netadr_t *b )
{
	return NET_CompareAdr( *a, *b );
}

/*
=================
NetAPI_StringToAdr

=================
*/
int NetAPI_StringToAdr( char *s, netadr_t *a )
{
	return NET_StringToAdr( s, a );
}

/*
=================
NetAPI_ValueForKey

=================
*/
const char *NetAPI_ValueForKey( const char *s, const char *key )
{
	return Info_ValueForKey( s, key );
}

/*
=================
NetAPI_RemoveKey

=================
*/
void NetAPI_RemoveKey( char *s, const char *key )
{
	Info_RemoveKey( s, key );
}

/*
=================
NetAPI_SetValueForKey

=================
*/
void NetAPI_SetValueForKey( char *s, const char *key, const char *value, int maxsize )
{
	if( key[0] == '*' ) return;
	Info_SetValueForStarKey( s, key, value, maxsize );
}


/*
=================
IVoiceTweak implementation

TODO: implement
=================
*/
/*
=================
Voice_StartVoiceTweakMode

=================
*/
int Voice_StartVoiceTweakMode( void )
{
	return 0;
}

/*
=================
Voice_EndVoiceTweakMode

=================
*/
void Voice_EndVoiceTweakMode( void )
{
}

/*
=================
Voice_SetControlFloat

=================
*/	
void Voice_SetControlFloat( VoiceTweakControl iControl, float value )
{
}

/*
=================
Voice_GetControlFloat

=================
*/
float Voice_GetControlFloat( VoiceTweakControl iControl )
{
	return 1.0f;
}

/*
=============
pfnEngineStub

extended iface stubs
=============
*/
static void pfnEngineStub( void )
{
}

// shared between client and server			
triangleapi_t gTriApi =
{
	TRI_API_VERSION,	
	TriRenderMode,
	TriBegin,
	TriEnd,
	TriColor4f,
	TriColor4ub,
	TriTexCoord2f,
	TriVertex3fv,
	TriVertex3f,
	TriBrightness,
	TriCullFace,
	TriSpriteTexture,
	R_WorldToScreen,	// NOTE: XPROJECT, YPROJECT should be done in client.dll
	TriFog,
	R_ScreenToWorld,
	TriGetMatrix,
	TriBoxInPVS,
	TriLightAtPoint,
	TriColor4fRendermode,
	TriFogParams,
};

static efx_api_t gEfxApi =
{
	R_AllocParticle,
	R_BlobExplosion,
	R_Blood,
	R_BloodSprite,
	R_BloodStream,
	R_BreakModel,
	R_Bubbles,
	R_BubbleTrail,
	R_BulletImpactParticles,
	R_EntityParticles,
	R_Explosion,
	R_FizzEffect,
	R_FireField,
	R_FlickerParticles,
	R_FunnelSprite,
	R_Implosion,
	R_LargeFunnel,
	R_LavaSplash,
	R_MultiGunshot,
	R_MuzzleFlash,
	R_ParticleBox,
	R_ParticleBurst,
	R_ParticleExplosion,
	R_ParticleExplosion2,
	R_ParticleLine,
	R_PlayerSprites,
	R_Projectile,
	R_RicochetSound,
	R_RicochetSprite,
	R_RocketFlare,
	R_RocketTrail,
	R_RunParticleEffect,
	R_ShowLine,
	R_SparkEffect,
	R_SparkShower,
	R_SparkStreaks,
	R_Spray,
	R_Sprite_Explode,
	R_Sprite_Smoke,
	R_Sprite_Spray,
	R_Sprite_Trail,
	R_Sprite_WallPuff,
	R_StreakSplash,
	R_TracerEffect,
	R_UserTracerParticle,
	R_TracerParticles,
	R_TeleportSplash,
	R_TempSphereModel,
	R_TempModel,
	R_DefaultSprite,
	R_TempSprite,
	CL_DecalIndex,
	CL_DecalIndexFromName,
	CL_DecalShoot,
	R_AttachTentToPlayer,
	R_KillAttachedTents,
	R_BeamCirclePoints,
	R_BeamEntPoint,
	R_BeamEnts,
	R_BeamFollow,
	R_BeamKill,
	R_BeamLightning,
	R_BeamPoints,
	R_BeamRing,
	CL_AllocDlight,
	CL_AllocElight,
	CL_TempEntAlloc,
	CL_TempEntAllocNoModel,
	CL_TempEntAllocHigh,
	CL_TempEntAllocCustom,
	R_GetPackedColor,
	R_LookupColor,
	CL_DecalRemoveAll,
	CL_FireCustomDecal,
};

static event_api_t gEventApi =
{
	EVENT_API_VERSION,
	pfnPlaySound,
	S_StopSound,
	CL_FindModelIndex,
	pfnIsLocal,
	pfnLocalPlayerDucking,
	pfnLocalPlayerViewheight,
	pfnLocalPlayerBounds,
	pfnIndexFromTrace,
	pfnGetPhysent,
	CL_SetUpPlayerPrediction,
	CL_PushPMStates,
	CL_PopPMStates,
	CL_SetSolidPlayers,
	CL_SetTraceHull,
	CL_PlayerTrace,
	CL_WeaponAnim,
	pfnPrecacheEvent,
	CL_PlaybackEvent,
	pfnTraceTexture,
	pfnStopAllSounds,
	pfnKillEvents,
	CL_PlayerTraceExt,		// Xash3D added
	CL_SoundFromIndex,
	pfnTraceSurface,
	pfnGetMoveVars,
	CL_VisTraceLine,
	pfnGetVisent,
	CL_TestLine,
	CL_PushTraceBounds,
	CL_PopTraceBounds,
};

static demo_api_t gDemoApi =
{
	Demo_IsRecording,
	Demo_IsPlayingback,
	Demo_IsTimeDemo,
	Demo_WriteBuffer,
};

static net_api_t gNetApi =
{
	NetAPI_InitNetworking,
	NetAPI_Status,
	NetAPI_SendRequest,
	NetAPI_CancelRequest,
	NetAPI_CancelAllRequests,
	NetAPI_AdrToString,
	NetAPI_CompareAdr,
	NetAPI_StringToAdr,
	NetAPI_ValueForKey,
	NetAPI_RemoveKey,
	NetAPI_SetValueForKey,
};

static IVoiceTweak gVoiceApi =
{
	Voice_StartVoiceTweakMode,
	Voice_EndVoiceTweakMode,
	Voice_SetControlFloat,
	Voice_GetControlFloat,
};

// engine callbacks
static cl_enginefunc_t gEngfuncs = 
{
	pfnSPR_Load,
	pfnSPR_Frames,
	pfnSPR_Height,
	pfnSPR_Width,
	pfnSPR_Set,
	pfnSPR_Draw,
	pfnSPR_DrawHoles,
	pfnSPR_DrawAdditive,
	SPR_EnableScissor,
	SPR_DisableScissor,
	pfnSPR_GetList,
	CL_FillRGBA,
	pfnGetScreenInfo,
	pfnSetCrosshair,
	pfnCvar_RegisterClientVariable,
	Cvar_VariableValue,
	Cvar_VariableString,
	Cmd_AddClientCommand,
	pfnHookUserMsg,
	pfnServerCmd,
	pfnClientCmd,
	pfnGetPlayerInfo,
	pfnPlaySoundByName,
	pfnPlaySoundByIndex,
	AngleVectors,
	CL_TextMessageGet,
	pfnDrawCharacter,
	pfnDrawConsoleString,
	pfnDrawSetTextColor,
	pfnDrawConsoleStringLen,
	pfnConsolePrint,
	pfnCenterPrint,
	pfnGetWindowCenterX,
	pfnGetWindowCenterY,
	pfnGetViewAngles,
	pfnSetViewAngles,
	CL_GetMaxClients,
	Cvar_SetValue,
	Cmd_Argc,
	Cmd_Argv,
	Con_Printf,
	Con_DPrintf,
	Con_NPrintf,
	Con_NXPrintf,
	pfnPhysInfo_ValueForKey,
	pfnServerInfo_ValueForKey,
	pfnGetClientMaxspeed,
	COM_CheckParm,
	Key_Event,
	pfnGetMousePosition,
	pfnIsNoClipping,
	CL_GetLocalPlayer,
	pfnGetViewModel,
	CL_GetEntityByIndex,
	pfnGetClientTime,
	pfnCalcShake,
	pfnApplyShake,
	pfnPointContents,
	CL_WaterEntity,
	pfnTraceLine,
	CL_LoadModel,
	CL_AddEntity,
	CL_GetSpritePointer,
	pfnPlaySoundByNameAtLocation,
	pfnPrecacheEvent,
	CL_PlaybackEvent,
	CL_WeaponAnim,
	COM_RandomFloat,
	COM_RandomLong,
	pfnHookEvent,
	Con_Visible,
	pfnGetGameDirectory,
	pfnCVarGetPointer,
	Key_LookupBinding,
	pfnGetLevelName,
	pfnGetScreenFade,
	pfnSetScreenFade,
	VGui_GetPanel,
	VGui_ViewportPaintBackground,
	COM_LoadFile,
	pfnParseFile,
	COM_FreeFile,
	&gTriApi,
	&gEfxApi,
	&gEventApi,
	&gDemoApi,
	&gNetApi,
	&gVoiceApi,
	pfnIsSpectateOnly,
	pfnLoadMapSprite,
	COM_AddAppDirectoryToSearchPath,
	COM_ExpandFilename,
	PlayerInfo_ValueForKey,
	PlayerInfo_SetValueForKey,
	pfnGetPlayerUniqueID,
	pfnGetTrackerIDForPlayer,
	pfnGetPlayerForTrackerID,
	pfnServerCmdUnreliable,
	pfnGetMousePos,
	pfnSetMousePos,
	pfnSetMouseEnable,
// MARTY - Extensions
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	pfnGetClientOldTime,
	pfnGetGravity,
	CL_ModelHandle,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	LocalPlayerInfo_ValueForKey,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	Cvar_Set,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	Sys_DoubleTime,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	pfnEngineStub,
	CL_FillRGBABlend,
	pfnGetAppID,
	pfnEngineStub,
	pfnEngineStub,
};

void CL_UnloadProgs( void )
{
	if( !clgame.hInstance ) return;

	CL_FreeEdicts();
	CL_FreeTempEnts();
	CL_FreeViewBeams();
	CL_FreeParticles();
	CL_ClearAllRemaps();
	Mod_ClearUserData();
#ifdef _VGUI
	VGui_Shutdown();
#endif

	// NOTE: HLFX 0.5 has strange bug: hanging on exit if no map was loaded
	if( Q_stricmp( GI->gamedir, "hlfx" ) || GI->version != 0.5f )
		clgame.dllFuncs.pfnShutdown();

	Cvar_FullSet( "cl_background", "0", FCVAR_READ_ONLY );
	Cvar_FullSet( "host_clientloaded", "0", FCVAR_READ_ONLY );

	COM_FreeLibrary( clgame.hInstance );
	Mem_FreePool( &cls.mempool );
	Mem_FreePool( &clgame.mempool );
	memset( &clgame, 0, sizeof( clgame ));

	Cvar_Unlink( FCVAR_CLIENTDLL );
	Cmd_Unlink( CMD_CLIENTDLL );
}

qboolean CL_LoadProgs( const char *name )
{
	static playermove_t		gpMove;
	const dllfunc_t		*func;
#ifndef _HARDLINKED
	CL_EXPORT_FUNCS		GetClientAPI; // single export
#endif
	qboolean			critical_exports = true;

	if( clgame.hInstance ) CL_UnloadProgs();

	// initialize PlayerMove
	clgame.pmove = &gpMove;

	cls.mempool = Mem_AllocPool( "Client Static Pool" );
	clgame.mempool = Mem_AllocPool( "Client Edicts Zone" );
	clgame.entities = NULL;

	clgame.hInstance = COM_LoadLibrary( name, false, false );
	if( !clgame.hInstance ) return false;

	// clear exports
	for( func = cdll_exports; func && func->name; func++ )
		*func->func = NULL;

#ifndef _HARDLINKED
	// trying to get single export
	if(( GetClientAPI = (void *)COM_GetProcAddress( clgame.hInstance, "GetClientAPI" )) != NULL )
	{
		Con_Reportf( "CL_LoadProgs: found single callback export\n" );		

		// trying to fill interface now
		GetClientAPI( &clgame.dllFuncs );

		// check critical functions again
		for( func = cdll_exports; func && func->name; func++ )
		{
			if( func->func == NULL )
				break; // BAH critical function was missed
		}

		// because all the exports are loaded through function 'F"
		if( !func || !func->name )
			critical_exports = false;
	}

	for( func = cdll_exports; func && func->name != NULL; func++ )
	{
		if( *func->func != NULL )
			continue;	// already get through 'F'

		// functions are cleared before all the extensions are evaluated
		if(( *func->func = (void *)COM_GetProcAddress( clgame.hInstance, func->name )) == NULL )
		{
          		Con_Reportf( "CL_LoadProgs: failed to get address of %s proc\n", func->name );

			if( critical_exports )
			{
				COM_FreeLibrary( clgame.hInstance );
				clgame.hInstance = NULL;
				return false;
			}
		}
	}
#else
	clgame.dllFuncs.pfnInitialize = &StaticInitialize;
	clgame.dllFuncs.pfnVidInit = &StaticHUD_VidInit;
	clgame.dllFuncs.pfnInit = &StaticHUD_Init;
	clgame.dllFuncs.pfnShutdown = &StaticHUD_Shutdown;
	clgame.dllFuncs.pfnRedraw = &StaticHUD_Redraw;
	clgame.dllFuncs.pfnUpdateClientData = &StaticHUD_UpdateClientData;
	clgame.dllFuncs.pfnReset = &StaticHUD_Reset;
	clgame.dllFuncs.pfnPlayerMove = &StaticHUD_PlayerMove;
	clgame.dllFuncs.pfnPlayerMoveInit = &StaticHUD_PlayerMoveInit;
	clgame.dllFuncs.pfnPlayerMoveTexture = &StaticHUD_PlayerMoveTexture;
	clgame.dllFuncs.pfnConnectionlessPacket = &StaticHUD_ConnectionlessPacket;
	clgame.dllFuncs.pfnGetHullBounds = &StaticHUD_GetHullBounds;
	clgame.dllFuncs.pfnFrame = &StaticHUD_Frame;
	clgame.dllFuncs.pfnPostRunCmd = &StaticHUD_PostRunCmd;
	clgame.dllFuncs.pfnKey_Event = &StaticHUD_Key_Event;
	clgame.dllFuncs.pfnAddEntity = &StaticHUD_AddEntity;
	clgame.dllFuncs.pfnCreateEntities = &StaticHUD_CreateEntities;
	clgame.dllFuncs.pfnStudioEvent = &StaticHUD_StudioEvent;
	clgame.dllFuncs.pfnTxferLocalOverrides = &StaticHUD_TxferLocalOverrides;
	clgame.dllFuncs.pfnProcessPlayerState = &StaticHUD_ProcessPlayerState;
	clgame.dllFuncs.pfnTxferPredictionData = &StaticHUD_TxferPredictionData;
	clgame.dllFuncs.pfnTempEntUpdate = &StaticHUD_TempEntUpdate;
	clgame.dllFuncs.pfnDrawNormalTriangles = &StaticHUD_DrawNormalTriangles;
	clgame.dllFuncs.pfnDrawTransparentTriangles = &StaticHUD_DrawTransparentTriangles;
	clgame.dllFuncs.pfnGetUserEntity = &StaticHUD_GetUserEntity;
	clgame.dllFuncs.pfnDemo_ReadBuffer = &StaticDemo_ReadBuffer;
	clgame.dllFuncs.CAM_Think = &StaticCAM_Think;
	clgame.dllFuncs.CL_IsThirdPerson = &StaticCL_IsThirdPerson;
	clgame.dllFuncs.CL_CameraOffset = &StaticCL_CameraOffset;
	clgame.dllFuncs.CL_CreateMove = &StaticCL_CreateMove;
	clgame.dllFuncs.IN_ActivateMouse = &StaticIN_ActivateMouse;
	clgame.dllFuncs.IN_DeactivateMouse = &StaticIN_DeactivateMouse;
	clgame.dllFuncs.IN_MouseEvent = &StaticIN_MouseEvent;
	clgame.dllFuncs.IN_Accumulate = &StaticIN_Accumulate;
	clgame.dllFuncs.IN_ClearStates = &StaticIN_ClearStates;
	clgame.dllFuncs.pfnCalcRefdef = &StaticV_CalcRefdef;
	clgame.dllFuncs.KB_Find = &StaticKB_Find;

#ifdef _XBOX // MARTY - Custom Xbox only call to pump gamepad key messages
	clgame.dllFuncs.IN_XBoxGamepadButtons = &StaticIN_XBoxGamepadButtons;
#endif

#endif

#ifndef _HARDLINKED

	// it may be loaded through 'GetClientAPI' so we don't need to clear them
	if( critical_exports )
	{
		// clear new exports
		for( func = cdll_new_exports; func && func->name; func++ )
			*func->func = NULL;
	}

	for( func = cdll_new_exports; func && func->name != NULL; func++ )
	{
		if( *func->func != NULL )
			continue;	// already get through 'F'

		// functions are cleared before all the extensions are evaluated
		// NOTE: new exports can be missed without stop the engine
		if(( *func->func = (void *)COM_GetProcAddress( clgame.hInstance, func->name )) == NULL )
			Con_Reportf( "CL_LoadProgs: failed to get address of %s proc\n", func->name );
	}
#else
	clgame.dllFuncs.pfnGetStudioModelInterface = &StaticHUD_GetStudioModelInterface;
	clgame.dllFuncs.pfnDirectorMessage = &StaticHUD_DirectorMessage;
	clgame.dllFuncs.pfnVoiceStatus = &StaticHUD_VoiceStatus;
#endif //_HARDLINKED

	if( !clgame.dllFuncs.pfnInitialize( &gEngfuncs, CLDLL_INTERFACE_VERSION ))
	{
		COM_FreeLibrary( clgame.hInstance );
		Con_Reportf( "CL_LoadProgs: can't init client API\n" );
		clgame.hInstance = NULL;
		return false;
	}

	Cvar_FullSet( "host_clientloaded", "1", FCVAR_READ_ONLY );

	clgame.maxRemapInfos = 0; // will be alloc on first call CL_InitEdicts();
	clgame.maxEntities = 2; // world + localclient (have valid entities not in game)

	CL_InitCDAudio( "media\\cdaudio.txt" ); // MARTY - Fixed slashes
	CL_InitTitles( "titles.txt" );
	CL_InitParticles ();
	CL_InitViewBeams ();
	CL_InitTempEnts ();

	if( !R_InitRenderAPI())	// Xash3D extension
		Con_Reportf( S_WARN "CL_LoadProgs: couldn't get render API\n" );

	CL_InitEdicts ();		// initailize local player and world
	CL_InitClientMove();	// initialize pm_shared

	// initialize game
	clgame.dllFuncs.pfnInit();

	CL_InitStudioAPI( );

#ifdef _VGUI // Don't need VGUI for stock HL
	// initialize VGui
	VGui_Startup ();
#endif

	// trying to grab them from client.dll
	cl_righthand = Cvar_FindVar( "cl_righthand" );

	if( cl_righthand == NULL )
		cl_righthand = Cvar_Get( "cl_righthand", "0", FCVAR_ARCHIVE, "flip viewmodel (left to right)" );

	return true;
}