/*
cl_video.c - avi video player
Copyright (C) 2009 Uncle Mike

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
#include "gl_local.h"

/*
=================================================================

AVI PLAYING

=================================================================
*/

static long		xres, yres;
static float		video_duration;
static float		cin_time;
static int		cin_frame;
#ifdef _AVIPLAYBACK // MARTY TODO - No video decoder on Xbox atm, only used for intro logo
static wavdata_t		cin_audio;
static movie_state_t	*cin_state;
#endif

/*
==================
SCR_NextMovie

Called when a demo or cinematic finishes
If the "nextmovie" cvar is set, that command will be issued
==================
*/
qboolean SCR_NextMovie( void )
{
#ifdef _AVIPLAYBACK
	string	str;

	if( cls.movienum == -1 )
	{
		S_StopAllSounds( true );
		SCR_StopCinematic();
		CL_CheckStartupDemos();
		return false; // don't play movies
	}

	if( !cls.movies[cls.movienum][0] || cls.movienum == MAX_MOVIES )
	{
		S_StopAllSounds( true );
		SCR_StopCinematic();
		cls.movienum = -1;
		CL_CheckStartupDemos();
		return false;
	}

	Q_snprintf( str, MAX_STRING, "movie %s full\n", cls.movies[cls.movienum] );

	Cbuf_InsertText( str );
	cls.movienum++;
#endif
	return true;
}

#ifdef _AVIPLAYBACK
void SCR_CreateStartupVids( void )
{
	file_t	*f;

	f = FS_Open( DEFAULT_VIDEOLIST_PATH, "w", false );
	if( !f ) return;

	// make standard video playlist: sierra, valve
	FS_Print( f, "media/sierra.avi\n" );
	FS_Print( f, "media/valve.avi\n" );
	FS_Close( f );
}
#endif

void SCR_CheckStartupVids( void )
{
#ifdef _AVIPLAYBACK
	int	c = 0;
	char	*afile, *pfile;
	string	token;
#endif

	if( Sys_CheckParm( "-nointro" ) || host_developer.value || cls.demonum != -1 || GameState->nextstate != STATE_RUNFRAME )
	{
		// don't run movies where we in developer-mode
		cls.movienum = -1;
		CL_CheckStartupDemos();
		return;
	}

#ifdef _AVIPLAYBACK
	if( !FS_FileExists( DEFAULT_VIDEOLIST_PATH, false ))
		SCR_CreateStartupVids();

	afile = FS_LoadFile( DEFAULT_VIDEOLIST_PATH, NULL, false );
	if( !afile ) return; // something bad happens

	pfile = afile;

	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		Q_strncpy( cls.movies[c], token, sizeof( cls.movies[0] ));

		if( ++c > MAX_MOVIES - 1 )
		{
			Con_Printf( S_WARN "too many movies (%d) specified in %s\n", MAX_MOVIES, DEFAULT_VIDEOLIST_PATH );
			break;
		}
	}

	Mem_Free( afile );

	// run cinematic
	cls.movienum = 0;
	SCR_NextMovie ();
	Cbuf_Execute();
#endif
}
		
/*
==================
SCR_RunCinematic
==================
*/
void SCR_RunCinematic( void )
{
#ifdef _AVIPLAYBACK
	if( cls.state != ca_cinematic )
		return;

	if( !AVI_IsActive( cin_state ))
	{
		SCR_NextMovie( );
		return;
	}

	if( UI_IsVisible( ))
	{
		// these can happens when user set +menu_ option to cmdline
		AVI_CloseVideo( cin_state );
		cls.state = ca_disconnected;
		Key_SetKeyDest( key_menu );
		S_StopStreaming();
		cls.movienum = -1;
		cin_time = 0.0f;
		cls.signon = 0;
		return;
	}

	// advances cinematic time (ignores maxfps and host_framerate settings)	
	cin_time += host.realframetime;

	// stop the video after it finishes
	if( cin_time > video_duration + 0.1f )
	{
		SCR_NextMovie( );
		return;
	}

	// read the next frame
	cin_frame = AVI_GetVideoFrameNumber( cin_state, cin_time );
#endif
}

/*
==================
SCR_DrawCinematic

Returns true if a cinematic is active, meaning the view rendering
should be skipped
==================
*/
qboolean SCR_DrawCinematic( void )
{
#ifdef _AVIPLAYBACK
	static int	last_frame = -1;
	qboolean		redraw = false;
	byte		*frame = NULL;

	if( !glw_state.initialized || cin_time <= 0.0f )
		return false;

	if( cin_frame != last_frame )
	{
		frame = AVI_GetVideoFrame( cin_state, cin_frame );
		last_frame = cin_frame;
		redraw = true;
	}

	R_DrawStretchRaw( 0, 0, glState.width, glState.height, xres, yres, frame, redraw );
#endif

	return true;
}
  
/*
==================
SCR_PlayCinematic
==================
*/
#ifdef _AVIPLAYBACK
qboolean SCR_PlayCinematic( const char *arg )
{
	string		path;
	const char	*fullpath;

	fullpath = FS_GetDiskPath( arg, false );

	if( FS_FileExists( arg, false ) && !fullpath )
	{
		Con_Printf( S_ERROR "Couldn't load %s from packfile. Please extract it\n", path );
		return false;
	}

	AVI_OpenVideo( cin_state, fullpath, true, false );
	if( !AVI_IsActive( cin_state ))
	{
		AVI_CloseVideo( cin_state );
		return false;
	}

	if( !( AVI_GetVideoInfo( cin_state, &xres, &yres, &video_duration ))) // couldn't open this at all.
	{
		AVI_CloseVideo( cin_state );
		return false;
	}

	if( AVI_GetAudioInfo( cin_state, &cin_audio ))
	{
		// begin streaming
		S_StopAllSounds( true );
		S_StartStreaming();
	}

	UI_SetActiveMenu( false );
	cls.state = ca_cinematic;
	Con_FastClose();
	cin_time = 0.0f;
	cls.signon = 0;
	
	return true;
}
#endif

#ifdef _AVIPLAYBACK
long SCR_GetAudioChunk( char *rawdata, long length )
{
	int	r;

	r = AVI_GetAudioChunk( cin_state, rawdata, cin_audio.loopStart, length );
	cin_audio.loopStart += r; // advance play position

	return r;
}
#endif

#ifdef _AVIPLAYBACK
wavdata_t *SCR_GetMovieInfo( void )
{
	if( AVI_IsActive( cin_state ))
		return &cin_audio;
	return NULL;
}
#endif
	
/*
==================
SCR_StopCinematic
==================
*/
void SCR_StopCinematic( void )
{
	if( cls.state != ca_cinematic )
		return;
#ifdef _AVIPLAYBACK
	AVI_CloseVideo( cin_state );
	S_StopStreaming();
	cin_time = 0.0f;
#endif
	cls.state = ca_disconnected;
	cls.signon = 0;

	UI_SetActiveMenu( true );
}

/*
==================
SCR_InitCinematic
==================
*/
void SCR_InitCinematic( void )
{
#ifdef _AVIPLAYBACK // MARTY TODO - No video decoder on Xbox atm, only used for intro logo
	AVI_Initailize ();
	cin_state = AVI_GetState( CIN_MAIN );
#endif
}

/*
==================
SCR_FreeCinematic
==================
*/
void SCR_FreeCinematic( void )
{
#ifdef _AVIPLAYBACK
	movie_state_t	*cin_state;

	// release videos
	cin_state = AVI_GetState( CIN_LOGO );
	AVI_CloseVideo( cin_state );

	cin_state = AVI_GetState( CIN_MAIN );
	AVI_CloseVideo( cin_state );

	AVI_Shutdown();
#endif
}