/*
cl_gameui.c - menu dlls interaction
Copyright (C) 2010 Uncle Mike

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
#include "gl_local.h"
#include "library.h"
#include "input.h"

#ifdef _HARDLINKED
int StaticGetMenuAPI( UI_FUNCTIONS *pFunctionTable, ui_enginefuncs_t* pEngfuncsFromEngine, ui_globalvars_t *pGlobals );
#endif

static MENUAPI	GetMenuAPI;
static void 	UI_UpdateUserinfo( void );

gameui_static_t	gameui;

void UI_UpdateMenu( float realtime )
{
	if( !gameui.hInstance ) return;

	// if some deferred cmds is waiting
	if( UI_IsVisible() && COM_CheckString( host.deferred_cmd ))
	{
		Cbuf_AddText( host.deferred_cmd );
		host.deferred_cmd[0] = '\0';
		Cbuf_Execute();
		return;
	}

	// don't show menu while level is loaded
	if( GameState->nextstate != STATE_RUNFRAME && !GameState->loadGame )
		return;

	// menu time (not paused, not clamped)
	gameui.globals->time = host.realtime;
	gameui.globals->frametime = host.realframetime;
	gameui.globals->demoplayback = cls.demoplayback;
	gameui.globals->demorecording = cls.demorecording;
	gameui.globals->allow_console = host.allow_console;

	gameui.dllFuncs.pfnRedraw( realtime );
	UI_UpdateUserinfo();
}

void UI_KeyEvent( int key, qboolean down )
{
	if( !gameui.hInstance ) return;
	gameui.dllFuncs.pfnKeyEvent( key, down );
}

void UI_MouseMove( int x, int y )
{
	if( !gameui.hInstance ) return;
	gameui.dllFuncs.pfnMouseMove( x, y );
}

void UI_SetActiveMenu( qboolean fActive )
{
#ifdef _AVIPLAYBACK
	movie_state_t	*cin_state;
#endif

	if( !gameui.hInstance )
	{
		if( !fActive )
			Key_SetKeyDest( key_game );
		return;
	}

	gameui.drawLogo = fActive;
	gameui.dllFuncs.pfnSetActiveMenu( fActive );

#ifdef _AVIPLAYBACK
	if( !fActive )
	{
		// close logo when menu is shutdown
		cin_state = AVI_GetState( CIN_LOGO );
		AVI_CloseVideo( cin_state );
	}
#endif
}

void UI_AddServerToList( netadr_t adr, const char *info )
{
	if( !gameui.hInstance ) return;
	gameui.dllFuncs.pfnAddServerToList( adr, info );
}

void UI_GetCursorPos( int *pos_x, int *pos_y )
{
	if( !gameui.hInstance ) return;
	gameui.dllFuncs.pfnGetCursorPos( pos_x, pos_y );
}

void UI_SetCursorPos( int pos_x, int pos_y )
{
	if( !gameui.hInstance ) return;
	gameui.dllFuncs.pfnSetCursorPos( pos_x, pos_y );
}

void UI_ShowCursor( qboolean show )
{
	if( !gameui.hInstance ) return;
	gameui.dllFuncs.pfnShowCursor( show );
}

qboolean UI_CreditsActive( void )
{
	if( !gameui.hInstance ) return 0;
	return gameui.dllFuncs.pfnCreditsActive();
}

void UI_CharEvent( int key )
{
	if( !gameui.hInstance ) return;
	gameui.dllFuncs.pfnCharEvent( key );
}

qboolean UI_MouseInRect( void )
{
	if( !gameui.hInstance ) return 1;
	return gameui.dllFuncs.pfnMouseInRect();
}

qboolean UI_IsVisible( void )
{
	if( !gameui.hInstance ) return 0;
	return gameui.dllFuncs.pfnIsVisible();
}

static void UI_DrawLogo( const char *filename, float x, float y, float width, float height )
{
#ifdef _AVIPLAYBACK // MARTY - No video decoder on Xbox atm, only used for intro logo
	static float	cin_time;
	static int	last_frame = -1;
	byte		*cin_data = NULL;
	movie_state_t	*cin_state;
	int		cin_frame;
	qboolean		redraw = false;

	if( !gameui.drawLogo ) return;
	cin_state = AVI_GetState( CIN_LOGO );

	if( !AVI_IsActive( cin_state ))
	{
		string		path;
		const char	*fullpath;
	
		// run cinematic if not
		Q_snprintf( path, sizeof( path ), "media/%s", filename );
		COM_DefaultExtension( path, ".avi" );
		fullpath = FS_GetDiskPath( path, false );

		if( FS_FileExists( path, false ) && !fullpath )
		{
			Con_Printf( S_ERROR "Couldn't load %s from packfile. Please extract it\n", path );
			gameui.drawLogo = false;
			return;
		}

		AVI_OpenVideo( cin_state, fullpath, false, true );
		if( !( AVI_GetVideoInfo( cin_state, &gameui.logo_xres, &gameui.logo_yres, &gameui.logo_length )))
		{
			AVI_CloseVideo( cin_state );
			gameui.drawLogo = false;
			return;
		}

		cin_time = 0.0f;
		last_frame = -1;
	}

	if( width <= 0 || height <= 0 )
	{
		// precache call, don't draw
		cin_time = 0.0f;
		last_frame = -1;
		return;
	}

	// advances cinematic time (ignores maxfps and host_framerate settings)
	cin_time += host.realframetime;

	// restarts the cinematic
	if( cin_time > gameui.logo_length )
		cin_time = 0.0f;

	// read the next frame
	cin_frame = AVI_GetVideoFrameNumber( cin_state, cin_time );

	if( cin_frame != last_frame )
	{
		cin_data = AVI_GetVideoFrame( cin_state, cin_frame );
		last_frame = cin_frame;
		redraw = true;
	}

	R_DrawStretchRaw( x, y, width, height, gameui.logo_xres, gameui.logo_yres, cin_data, redraw );
#endif
}

static int UI_GetLogoWidth( void )
{
	return gameui.logo_xres;
}

static int UI_GetLogoHeight( void )
{
	return gameui.logo_yres;
}

static float UI_GetLogoLength( void )
{
	return gameui.logo_length;
}

static void UI_UpdateUserinfo( void )
{
	player_info_t	*player;

	if( !host.userinfo_changed )
		return;

	player = &gameui.playerinfo;

	Q_strncpy( player->userinfo, cls.userinfo, sizeof( player->userinfo ));
	Q_strncpy( player->name, Info_ValueForKey( player->userinfo, "name" ), sizeof( player->name ));
	Q_strncpy( player->model, Info_ValueForKey( player->userinfo, "model" ), sizeof( player->model ));
	player->topcolor = Q_atoi( Info_ValueForKey( player->userinfo, "topcolor" ));
	player->bottomcolor = Q_atoi( Info_ValueForKey( player->userinfo, "bottomcolor" ));
	host.userinfo_changed = false; // we got it
}
	
void Host_Credits( void )
{
	if( !gameui.hInstance ) return;
	gameui.dllFuncs.pfnFinalCredits();
}

static void UI_ConvertGameInfo( GAMEINFO *out, gameinfo_t *in )
{
	Q_strncpy( out->gamefolder, in->gamefolder, sizeof( out->gamefolder ));
	Q_strncpy( out->startmap, in->startmap, sizeof( out->startmap ));
	Q_strncpy( out->trainmap, in->trainmap, sizeof( out->trainmap ));
	Q_strncpy( out->title, in->title, sizeof( out->title ));
	Q_strncpy( out->version, va( "%g", in->version ), sizeof( out->version ));

	Q_strncpy( out->game_url, in->game_url, sizeof( out->game_url ));
	Q_strncpy( out->update_url, in->update_url, sizeof( out->update_url ));
	Q_strncpy( out->size, Q_pretifymem( in->size, 0 ), sizeof( out->size ));
	Q_strncpy( out->type, in->type, sizeof( out->type ));
	Q_strncpy( out->date, in->date, sizeof( out->date ));

	out->gamemode = in->gamemode;

	if( in->nomodels )
		out->flags |= GFL_NOMODELS;
	if( in->noskills )
		out->flags |= GFL_NOSKILLS;
}

static qboolean PIC_Scissor( float *x, float *y, float *width, float *height, float *u0, float *v0, float *u1, float *v1 )
{
	float	dudx, dvdy;

	// clip sub rect to sprite
	if(( width == 0 ) || ( height == 0 ))
		return false;

	if( *x + *width <= gameui.ds.scissor_x )
		return false;
	if( *x >= gameui.ds.scissor_x + gameui.ds.scissor_width )
		return false;
	if( *y + *height <= gameui.ds.scissor_y )
		return false;
	if( *y >= gameui.ds.scissor_y + gameui.ds.scissor_height )
		return false;

	dudx = (*u1 - *u0) / *width;
	dvdy = (*v1 - *v0) / *height;

	if( *x < gameui.ds.scissor_x )
	{
		*u0 += (gameui.ds.scissor_x - *x) * dudx;
		*width -= gameui.ds.scissor_x - *x;
		*x = gameui.ds.scissor_x;
	}

	if( *x + *width > gameui.ds.scissor_x + gameui.ds.scissor_width )
	{
		*u1 -= (*x + *width - (gameui.ds.scissor_x + gameui.ds.scissor_width)) * dudx;
		*width = gameui.ds.scissor_x + gameui.ds.scissor_width - *x;
	}

	if( *y < gameui.ds.scissor_y )
	{
		*v0 += (gameui.ds.scissor_y - *y) * dvdy;
		*height -= gameui.ds.scissor_y - *y;
		*y = gameui.ds.scissor_y;
	}

	if( *y + *height > gameui.ds.scissor_y + gameui.ds.scissor_height )
	{
		*v1 -= (*y + *height - (gameui.ds.scissor_y + gameui.ds.scissor_height)) * dvdy;
		*height = gameui.ds.scissor_y + gameui.ds.scissor_height - *y;
	}
	return true;
}

/*
====================
PIC_DrawGeneric

draw hudsprite routine
====================
*/
static void PIC_DrawGeneric( float x, float y, float width, float height, const wrect_t *prc )
{
	float	s1, s2, t1, t2;
	int	w, h;

	// assume we get sizes from image
	R_GetTextureParms( &w, &h, gameui.ds.gl_texturenum );

	if( prc )
	{
		// calc user-defined rectangle
		s1 = (float)prc->left / (float)w;
		t1 = (float)prc->top / (float)h;
		s2 = (float)prc->right / (float)w;
		t2 = (float)prc->bottom / (float)h;

		if( width == -1 && height == -1 )
		{
			width = prc->right - prc->left;
			height = prc->bottom - prc->top;
		}
	}
	else
	{
		s1 = t1 = 0.0f;
		s2 = t2 = 1.0f;
	}

	if( width == -1 && height == -1 )
	{
		width = w;
		height = h;
	}

	// pass scissor test if supposed
	if( gameui.ds.scissor_test && !PIC_Scissor( &x, &y, &width, &height, &s1, &t1, &s2, &t2 ))
		return;

	PicAdjustSize( &x, &y, &width, &height );
	R_DrawStretchPic( x, y, width, height, s1, t1, s2, t2, gameui.ds.gl_texturenum );
	/*p*/glColor4ub( 255, 255, 255, 255 );
}

/*
===============================================================================
	MainUI Builtin Functions

===============================================================================
*/
/*
=========
pfnPIC_Load

=========
*/
static HIMAGE pfnPIC_Load( const char *szPicName, const byte *image_buf, long image_size, long flags )
{
	HIMAGE	tx;

	if( !COM_CheckString( szPicName ))
	{
		Con_Reportf( S_ERROR "CL_LoadImage: bad name!\n" );
		return 0;
	}

	// add default parms to image
	SetBits( flags, TF_IMAGE );

	Image_SetForceFlags( IL_LOAD_DECAL ); // allow decal images for menu
	tx = GL_LoadTexture( szPicName, image_buf, image_size, flags );
	Image_ClearForceFlags();

	return tx;
}

/*
=========
pfnPIC_Width

=========
*/
static int pfnPIC_Width( HIMAGE hPic )
{
	int	picWidth;

	R_GetTextureParms( &picWidth, NULL, hPic );

	return picWidth;
}

/*
=========
pfnPIC_Height

=========
*/
static int pfnPIC_Height( HIMAGE hPic )
{
	int	picHeight;

	R_GetTextureParms( NULL, &picHeight, hPic );

	return picHeight;
}

/*
=========
pfnPIC_Set

=========
*/
void pfnPIC_Set( HIMAGE hPic, int r, int g, int b, int a )
{
	gameui.ds.gl_texturenum = hPic;
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );
	/*p*/glColor4ub( r, g, b, a );
}

/*
=========
pfnPIC_Draw

=========
*/
void pfnPIC_Draw( int x, int y, int width, int height, const wrect_t *prc )
{
	GL_SetRenderMode( kRenderNormal );
	PIC_DrawGeneric( x, y, width, height, prc );
}

/*
=========
pfnPIC_DrawTrans

=========
*/
void pfnPIC_DrawTrans( int x, int y, int width, int height, const wrect_t *prc )
{
	GL_SetRenderMode( kRenderTransTexture );
	PIC_DrawGeneric( x, y, width, height, prc );
}

/*
=========
pfnPIC_DrawHoles

=========
*/
void pfnPIC_DrawHoles( int x, int y, int width, int height, const wrect_t *prc )
{
	GL_SetRenderMode( kRenderTransAlpha );
	PIC_DrawGeneric( x, y, width, height, prc );
}

/*
=========
pfnPIC_DrawAdditive

=========
*/
void pfnPIC_DrawAdditive( int x, int y, int width, int height, const wrect_t *prc )
{
	GL_SetRenderMode( kRenderTransAdd );
	PIC_DrawGeneric( x, y, width, height, prc );
}

/*
=========
pfnPIC_EnableScissor

=========
*/
static void pfnPIC_EnableScissor( int x, int y, int width, int height )
{
	// check bounds
	x = bound( 0, x, gameui.globals->scrWidth );
	y = bound( 0, y, gameui.globals->scrHeight );
	width = bound( 0, width, gameui.globals->scrWidth - x );
	height = bound( 0, height, gameui.globals->scrHeight - y );

	gameui.ds.scissor_x = x;
	gameui.ds.scissor_width = width;
	gameui.ds.scissor_y = y;
	gameui.ds.scissor_height = height;
	gameui.ds.scissor_test = true;
}

/*
=========
pfnPIC_DisableScissor

=========
*/
static void pfnPIC_DisableScissor( void )
{
	gameui.ds.scissor_x = 0;
	gameui.ds.scissor_width = 0;
	gameui.ds.scissor_y = 0;
	gameui.ds.scissor_height = 0;
	gameui.ds.scissor_test = false;
}

/*
=============
pfnFillRGBA

=============
*/
static void pfnFillRGBA( int x, int y, int width, int height, int r, int g, int b, int a )
{
	r = bound( 0, r, 255 );
	g = bound( 0, g, 255 );
	b = bound( 0, b, 255 );
	a = bound( 0, a, 255 );
	/*p*/glColor4ub( r, g, b, a );
	GL_SetRenderMode( kRenderTransTexture );
	R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, tr.whiteTexture );
	/*p*/glColor4ub( 255, 255, 255, 255 );
}

/*
=============
pfnClientCmd

=============
*/
static void pfnClientCmd( int exec_now, const char *szCmdString )
{
	if( !szCmdString || !szCmdString[0] )
		return;

	Cbuf_AddText( szCmdString );
	Cbuf_AddText( "\n" );

	// client command executes immediately
	if( exec_now ) Cbuf_Execute();
}

/*
=============
pfnPlaySound

=============
*/
static void pfnPlaySound( const char *szSound )
{
	if( !COM_CheckString( szSound )) return;
	S_StartLocalSound( szSound, VOL_NORM, false );
}

/*
=============
pfnDrawCharacter

quakefont draw character
=============
*/
static void pfnDrawCharacter( int ix, int iy, int iwidth, int iheight, int ch, int ulRGBA, HIMAGE hFont )
{
	rgba_t	color;
	float	row, col, size;
	float	s1, t1, s2, t2;
	float	x = ix, y = iy;
	float	width = iwidth;
	float	height = iheight;

	ch &= 255;

	if( ch == ' ' ) return;
	if( y < -height ) return;

	color[3] = (ulRGBA & 0xFF000000) >> 24;
	color[0] = (ulRGBA & 0xFF0000) >> 16;
	color[1] = (ulRGBA & 0xFF00) >> 8;
	color[2] = (ulRGBA & 0xFF) >> 0;
	/*p*/glColor4ubv( color );

	col = (ch & 15) * 0.0625f + (0.5f / 256.0f);
	row = (ch >> 4) * 0.0625f + (0.5f / 256.0f);
	size = 0.0625f - (1.0f / 256.0f);

	s1 = col;
	t1 = row;
	s2 = s1 + size;
	t2 = t1 + size;

	// pass scissor test if supposed
	if( gameui.ds.scissor_test && !PIC_Scissor( &x, &y, &width, &height, &s1, &t1, &s2, &t2 ))
		return;

	GL_SetRenderMode( kRenderTransTexture );
	R_DrawStretchPic( x, y, width, height, s1, t1, s2, t2, hFont );
	/*p*/glColor4ub( 255, 255, 255, 255 );
}

/*
=============
UI_DrawConsoleString

drawing string like a console string 
=============
*/
static int UI_DrawConsoleString( int x, int y, const char *string )
{
	int	drawLen;

	if( !string || !*string ) return 0; // silent ignore
	drawLen = Con_DrawString( x, y, string, gameui.ds.textColor );
	MakeRGBA( gameui.ds.textColor, 255, 255, 255, 255 );

	return (x + drawLen); // exclude color prexfixes
}

/*
=============
pfnDrawSetTextColor

set color for anything
=============
*/
static void UI_DrawSetTextColor( int r, int g, int b, int alpha )
{
	// bound color and convert to byte
	gameui.ds.textColor[0] = r;
	gameui.ds.textColor[1] = g;
	gameui.ds.textColor[2] = b;
	gameui.ds.textColor[3] = alpha;
}

/*
====================
pfnGetPlayerModel

for drawing playermodel previews
====================
*/
static cl_entity_t* pfnGetPlayerModel( void )
{
	return &gameui.playermodel;
}

/*
====================
pfnSetPlayerModel

for drawing playermodel previews
====================
*/
static void pfnSetPlayerModel( cl_entity_t *ent, const char *path )
{
	ent->model = Mod_ForName( path, false, false );
	ent->curstate.modelindex = MAX_MODELS; // unreachable index
}

/*
====================
pfnClearScene

for drawing playermodel previews
====================
*/
static void pfnClearScene( void )
{
	R_PushScene();
	R_ClearScene();
}

/*
====================
pfnRenderScene

for drawing playermodel previews
====================
*/
static void pfnRenderScene( const ref_viewpass_t *rvp )
{
	// to avoid division by zero
	if( !rvp || rvp->fov_x <= 0.0f || rvp->fov_y <= 0.0f )
		return;

	// don't allow special modes from menu
	((ref_viewpass_t *)&rvp)->flags = 0;

	R_Set2DMode( false );
	R_RenderFrame( rvp );
	R_Set2DMode( true );
	R_PopScene();
}

/*
====================
pfnAddEntity

adding player model into visible list
====================
*/
static int pfnAddEntity( int entityType, cl_entity_t *ent )
{
	if( !R_AddEntity( ent, entityType ))
		return false;
	return true;
}

/*
====================
pfnClientJoin

send client connect
====================
*/
static void pfnClientJoin( const netadr_t adr )
{
	Cbuf_AddText( va( "connect %s\n", NET_AdrToString( adr )));
}

/*
====================
pfnKeyGetOverstrikeMode

get global key overstrike state
====================
*/
static int pfnKeyGetOverstrikeMode( void )
{
	return host.key_overstrike;
}

/*
====================
pfnKeySetOverstrikeMode

set global key overstrike mode
====================
*/
static void pfnKeySetOverstrikeMode( int fActive )
{
	host.key_overstrike = fActive;
}

/*
====================
pfnKeyGetState

returns kbutton struct if found
====================
*/
static void *pfnKeyGetState( const char *name )
{
	if( clgame.dllFuncs.KB_Find )
		return clgame.dllFuncs.KB_Find( name );
	return NULL;
}

/*
=========
pfnMemAlloc

=========
*/
static void *pfnMemAlloc( size_t cb, const char *filename, const int fileline )
{
	return _Mem_Alloc( gameui.mempool, cb, true, filename, fileline );
}

/*
=========
pfnMemFree

=========
*/
static void pfnMemFree( void *mem, const char *filename, const int fileline )
{
	_Mem_Free( mem, filename, fileline );
}

/*
=========
pfnGetGameInfo

=========
*/
static int pfnGetGameInfo( GAMEINFO *pgameinfo )
{
	if( !pgameinfo ) return 0;

	*pgameinfo = gameui.gameInfo;
	return 1;
}

/*
=========
pfnGetGamesList

=========
*/
static GAMEINFO **pfnGetGamesList( int *numGames )
{
	if( numGames ) *numGames = SI.numgames;
	return gameui.modsInfo;
}

/*
=========
pfnGetFilesList

release prev search on a next call
=========
*/
static char **pfnGetFilesList( const char *pattern, int *numFiles, int gamedironly )
{
	static search_t	*t = NULL;

	if( t ) Mem_Free( t ); // release prev search

	t = FS_Search( pattern, true, gamedironly );
	if( !t )
	{
		if( numFiles ) *numFiles = 0;
		return NULL;
	}

	if( numFiles ) *numFiles = t->numfilenames;
	return t->filenames;
}

/*
=========
pfnGetClipboardData

pointer must be released in call place
=========
*/
static char *pfnGetClipboardData( void )
{
	return Sys_GetClipboardData();
}

/*
=========
pfnCheckGameDll

=========
*/
int pfnCheckGameDll( void )
{
	void	*hInst;

	if( SV_Initialized( )) return true;

	if(( hInst = COM_LoadLibrary( GI->game_dll, true, false )) != NULL )
	{
		COM_FreeLibrary( hInst );
		return true;
	}
	return false;
}

/*
=========
pfnChangeInstance

=========
*/
static void pfnChangeInstance( const char *newInstance, const char *szFinalMessage )
{
	if( !szFinalMessage ) szFinalMessage = "";
	if( !newInstance || !*newInstance ) return;

	Host_NewInstance( newInstance, szFinalMessage );
}

/*
=========
pfnHostEndGame

=========
*/
static void pfnHostEndGame( const char *szFinalMessage )
{
	if( !szFinalMessage ) szFinalMessage = "";
	Host_EndGame( true, szFinalMessage );
}

/*
=========
pfnStartBackgroundTrack

=========
*/
static void pfnStartBackgroundTrack( const char *introTrack, const char *mainTrack )
{
	S_StartBackgroundTrack( introTrack, mainTrack, 0, false );
}

// engine callbacks
static ui_enginefuncs_t gEngfuncs = 
{
	pfnPIC_Load,
	GL_FreeImage,
	pfnPIC_Width,
	pfnPIC_Height,
	pfnPIC_Set,
	pfnPIC_Draw,
	pfnPIC_DrawHoles,
	pfnPIC_DrawTrans,
	pfnPIC_DrawAdditive,
	pfnPIC_EnableScissor,
	pfnPIC_DisableScissor,
	pfnFillRGBA,
	pfnCvar_RegisterGameUIVariable,
	Cvar_VariableValue,
	Cvar_VariableString,
	Cvar_Set,
	Cvar_SetValue,
	Cmd_AddGameUICommand,
	pfnClientCmd,
	Cmd_RemoveCommand,
	Cmd_Argc,
	Cmd_Argv,
	Cmd_Args,
	Con_Printf,
	Con_DPrintf,
	UI_NPrintf,
	UI_NXPrintf,
	pfnPlaySound,
	UI_DrawLogo,
	UI_GetLogoWidth,
	UI_GetLogoHeight,
	UI_GetLogoLength,
	pfnDrawCharacter,
	UI_DrawConsoleString,
	UI_DrawSetTextColor,
	Con_DrawStringLen,
	Con_DefaultColor,
	pfnGetPlayerModel,
	pfnSetPlayerModel,
	pfnClearScene,	
	pfnRenderScene,
	pfnAddEntity,
	Host_Error,
	FS_FileExists,
	pfnGetGameDir,
	Cmd_CheckMapsList,
	CL_Active,
	pfnClientJoin,
	COM_LoadFileForMe,
	COM_ParseFile,
	COM_FreeFile,
	Key_ClearStates,
	Key_SetKeyDest,
	Key_KeynumToString,
	Key_GetBinding,
	Key_SetBinding,
	Key_IsDown,
	pfnKeyGetOverstrikeMode,
	pfnKeySetOverstrikeMode,
	pfnKeyGetState,
	pfnMemAlloc,
	pfnMemFree,
	pfnGetGameInfo,
	pfnGetGamesList,
	pfnGetFilesList,
	SV_GetSaveComment,
	CL_GetDemoComment,
	pfnCheckGameDll,
	pfnGetClipboardData,
	Sys_ShellExecute,
	Host_WriteServerConfig,
	pfnChangeInstance,
	pfnStartBackgroundTrack,
	pfnHostEndGame,
	COM_RandomFloat,
	COM_RandomLong,
	IN_SetCursor,
	pfnIsMapValid,
	GL_ProcessTexture,
	COM_CompareFileTime,
	VID_GetModeString,
	COM_SaveFile,
	FS_Delete,
};

void UI_UnloadProgs( void )
{
	if( !gameui.hInstance ) return;

	// deinitialize game
	gameui.dllFuncs.pfnShutdown();

	Cvar_FullSet( "host_gameuiloaded", "0", FCVAR_READ_ONLY );

	COM_FreeLibrary( gameui.hInstance );
	Mem_FreePool( &gameui.mempool );
	memset( &gameui, 0, sizeof( gameui ));

	Cvar_Unlink( FCVAR_GAMEUIDLL );
	Cmd_Unlink( CMD_GAMEUIDLL );
}

qboolean UI_LoadProgs( void )
{
	static ui_enginefuncs_t	gpEngfuncs;
	static ui_globalvars_t	gpGlobals;
	int			i;

	if( gameui.hInstance ) UI_UnloadProgs();

	// setup globals
	gameui.globals = &gpGlobals;

#ifndef _HARDLINKED
	if(( gameui.hInstance = COM_LoadLibrary( va( "%s/menu.dll", GI->dll_path ), false, false )) == NULL )
	{
		if(( gameui.hInstance = COM_LoadLibrary( "menu.dll", false, true )) == NULL )
			return false;
	}
#else
	gameui.hInstance = (void*)1; // MARTY
#endif

#ifndef _HARDLINKED
	if(( GetMenuAPI = (MENUAPI)COM_GetProcAddress( gameui.hInstance, "GetMenuAPI" )) == NULL )
	{
		COM_FreeLibrary( gameui.hInstance );
		Con_Reportf( "UI_LoadProgs: can't init menu API\n" );
		gameui.hInstance = NULL;
		return false;
	}
#else
	// make local copy of engfuncs to prevent overwrite it with user dll
	memcpy( &gpEngfuncs, &gEngfuncs, sizeof( gpEngfuncs ));

	gameui.mempool = Mem_AllocPool( "Menu Pool" );

	if( !StaticGetMenuAPI( &gameui.dllFuncs, &gpEngfuncs, gameui.globals ))
	{
		COM_FreeLibrary( gameui.hInstance );
		Con_Reportf( "UI_LoadProgs: can't init menu API\n" );
		Mem_FreePool( &gameui.mempool );
		gameui.hInstance = NULL;
		return false;
	}
#endif //_HARDLINKED
	Cvar_FullSet( "host_gameuiloaded", "1", FCVAR_READ_ONLY );

	// setup gameinfo
	for( i = 0; i < SI.numgames; i++ )
	{
		gameui.modsInfo[i] = Mem_Calloc( gameui.mempool, sizeof( GAMEINFO ));
		UI_ConvertGameInfo( gameui.modsInfo[i], SI.games[i] );
	}

	UI_ConvertGameInfo( &gameui.gameInfo, SI.GameInfo ); // current gameinfo

	// setup globals
	gameui.globals->allow_console = host.allow_console;

	// initialize game
	gameui.dllFuncs.pfnInit();

	return true;
}