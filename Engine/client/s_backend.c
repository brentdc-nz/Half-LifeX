/*
s_backend.c - sound hardware output
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
#include "sound.h"
#include <dsound.h>

#ifndef _HARDLINKED
#define iDirectSoundCreate( a, b, c )	pDirectSoundCreate( a, b, c )

static HRESULT ( _stdcall *pDirectSoundCreate)(GUID* lpGUID, LPDIRECTSOUND* lpDS, IUnknown* pUnkOuter );

static dllfunc_t dsound_funcs[] =
{
{ "DirectSoundCreate", (void **) &pDirectSoundCreate },
{ NULL, NULL }
};

dll_info_t dsound_dll = { "dsound.dll", dsound_funcs, false };
#endif

#define SAMPLE_16BIT_SHIFT		1
#define SECONDARY_BUFFER_SIZE		0x10000

typedef enum
{
	SIS_SUCCESS,
	SIS_FAILURE,
	SIS_NOTAVAIL
} si_state_t;

static qboolean		snd_firsttime = true;
static qboolean		primary_format_set;
static HWND		snd_hwnd;

/* 
=======================================================================
Global variables. Must be visible to window-procedure function 
so it can unlock and free the data block after it has been played.
=======================================================================
*/ 
DWORD		locksize;
/*H*/LPSTR		lpData, lpData2; // MARTY
DWORD		gSndBufSize;
MMTIME		mmstarttime;
LPDIRECTSOUNDBUFFER pDSBuf, pDSPBuf;
LPDIRECTSOUND	pDS;

qboolean SNDDMA_InitDirect( void *hInst );
void SNDDMA_FreeSound( void );

#ifndef _XBOX
static const char *DSoundError( int error )
{
	switch( error )
	{
	case DSERR_BUFFERLOST:
		return "buffer is lost";
	case DSERR_INVALIDCALL:
		return "invalid call";
	case DSERR_INVALIDPARAM:
		return "invalid param";
	case DSERR_PRIOLEVELNEEDED:
		return "invalid priority level";
	}

	return "unknown error";
}
#endif

/*
==================
DS_CreateBuffers
==================
*/
static qboolean DS_CreateBuffers( void *hInst )
{
	DSBUFFERDESC	dsbuf;
#ifndef _XBOX
	DSBCAPS		dsbcaps;
#endif
	WAVEFORMATEX	pformat, format;
#ifdef _XBOX
     DSMIXBINVOLUMEPAIR dsmbvp[8] = {
    {DSMIXBIN_FRONT_LEFT, DSBVOLUME_MAX},
    {DSMIXBIN_FRONT_RIGHT, DSBVOLUME_MAX},
    {DSMIXBIN_FRONT_CENTER, DSBVOLUME_MAX},
    {DSMIXBIN_FRONT_CENTER, DSBVOLUME_MAX},
    {DSMIXBIN_BACK_LEFT, DSBVOLUME_MAX},
    {DSMIXBIN_BACK_RIGHT, DSBVOLUME_MAX},
    {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MAX},
    {DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MAX}};

	DSMIXBINS dsmb;
#endif
	memset( &format, 0, sizeof( format ));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.wBitsPerSample = 16;
	format.nSamplesPerSec = SOUND_DMA_SPEED;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign; 
	format.cbSize = 0;

	if( DS_OK != IDirectSound_SetCooperativeLevel( pDS, hInst, DSSCL_EXCLUSIVE ))
	{
		Con_DPrintf( S_ERROR "DirectSound: failed to set EXCLUSIVE coop level\n" );
		SNDDMA_FreeSound();
		return false;
	}

	// get access to the primary buffer, if possible, so we can set the sound hardware format
	memset( &dsbuf, 0, sizeof( dsbuf ));
	dsbuf.dwSize = sizeof( DSBUFFERDESC );
	dsbuf.dwFlags = 0;//DSBCAPS_PRIMARYBUFFER; // MARTY
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = &format;//NULL // MARTY

#ifndef _XBOX
	memset( &dsbcaps, 0, sizeof( dsbcaps ));
	dsbcaps.dwSize = sizeof( dsbcaps );
#endif	
	primary_format_set = false;

	if( IDirectSound_CreateSoundBuffer( pDS, &dsbuf, &pDSPBuf, NULL ) == DS_OK )
	{
		pformat = format;

		if( IDirectSoundBuffer8_SetFormat( pDSPBuf, &pformat ) != DS_OK )
		{
			if( snd_firsttime )
				Con_DPrintf( S_ERROR "DirectSound: failed to set primary sound format\n" );
		}
		else
		{
			primary_format_set = true;
		}
	}

	// create the secondary buffer we'll actually work with
	memset( &dsbuf, 0, sizeof( dsbuf ));
	dsbuf.dwSize = sizeof( DSBUFFERDESC );
	dsbuf.dwFlags = (DSBCAPS_CTRLFREQUENCY/*|DSBCAPS_LOCSOFTWARE*/); // MARTY
	dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
	dsbuf.lpwfxFormat = &format;

#ifndef _XBOX
	memset( &dsbcaps, 0, sizeof( dsbcaps ));
	dsbcaps.dwSize = sizeof( dsbcaps );
#endif

	if( IDirectSound_CreateSoundBuffer( pDS, &dsbuf, &pDSBuf, NULL ) != DS_OK )
	{
#ifndef _XBOX
		// couldn't get hardware, fallback to software.
		dsbuf.dwFlags = (DSBCAPS_LOCSOFTWARE|DSBCAPS_GETCURRENTPOSITION2);

		if( pDS->lpVtbl->CreateSoundBuffer( pDS, &dsbuf, &pDSBuf, NULL ) != DS_OK )
		{
			Con_DPrintf( S_ERROR "DirectSound: failed to create secondary buffer\n" );
			SNDDMA_FreeSound ();
			return false;
		}
#else // We don't fall back to software on Xbox
		Con_DPrintf( S_ERROR "DirectSound: failed to create secondary buffer\n" );
		SNDDMA_FreeSound ();
		return false;
#endif
	}

#ifndef _XBOX
	if( pDSBuf->lpVtbl->GetCaps( pDSBuf, &dsbcaps ) != DS_OK )
	{
		Con_DPrintf( S_ERROR "DirectSound: failed to get capabilities\n" );
		SNDDMA_FreeSound ();
		return false;
	}
#endif

	// make sure mixer is active
	if( IDirectSoundBuffer_Play( pDSBuf, 0, 0, DSBPLAY_LOOPING ) != DS_OK )
	{
		Con_DPrintf( S_ERROR "DirectSound: failed to create circular buffer\n" );
		SNDDMA_FreeSound ();
		return false;
	}

	// we don't want anyone to access the buffer directly w/o locking it first
	lpData = NULL;
	dma.samplepos = 0;
	snd_hwnd = (HWND)hInst;
	gSndBufSize = dsbuf.dwBufferBytes;//dsbcaps.dwBufferBytes; // MARTY
	dma.samples = gSndBufSize / 2;
	dma.buffer = (byte *)lpData;

#ifdef _XBOX
	dsmb.dwMixBinCount = 8;
	dsmb.lpMixBinVolumePairs = dsmbvp;
 
	IDirectSoundBuffer_SetHeadroom(pDSBuf, DSBHEADROOM_MIN);
	IDirectSoundBuffer_SetMixBins(pDSBuf, &dsmb);

	IDirectSoundBuffer_SetVolume(pDSBuf, DSBVOLUME_MAX);
#endif

	SNDDMA_BeginPainting();
	if( dma.buffer ) memset( dma.buffer, 0, dma.samples * 2 );
	SNDDMA_Submit();

	return true;
}

/*
==================
DS_DestroyBuffers
==================
*/
static void DS_DestroyBuffers( void )
{
	if( pDS ) IDirectSound_SetCooperativeLevel( pDS, snd_hwnd, DSSCL_NORMAL );

	if( pDSBuf )
	{
		IDirectSoundBuffer_Stop( pDSBuf );
		IDirectSoundBuffer_Release( pDSBuf );
	}

	// only release primary buffer if it's not also the mixing buffer we just released
	if( pDSPBuf && ( pDSBuf != pDSPBuf ))
		IDirectSoundBuffer_Release( pDSPBuf );

	dma.buffer = NULL;
	pDSPBuf = NULL;
	pDSBuf = NULL;
}

/*
==================
SNDDMA_LockSound
==================
*/
void SNDDMA_LockSound( void )
{
	if( pDSBuf != NULL )
		IDirectSoundBuffer_Stop( pDSBuf );
}

/*
==================
SNDDMA_LockSound
==================
*/
void SNDDMA_UnlockSound( void )
{
	if( pDSBuf != NULL )
		IDirectSoundBuffer_Play( pDSBuf, 0, 0, DSBPLAY_LOOPING );
}

/*
==================
SNDDMA_FreeSound
==================
*/
void SNDDMA_FreeSound( void )
{
	if( pDS )
	{
		DS_DestroyBuffers();
		IDirectSound_Release( pDS );
#ifndef _HARDLINKED
		Sys_FreeLibrary( &dsound_dll );
#endif
	}

	lpData = NULL;
	pDSPBuf = NULL;
	pDSBuf = NULL;
	pDS = NULL;
}

/*
==================
SNDDMA_InitDirect

Direct-Sound support
==================
*/
si_state_t SNDDMA_InitDirect( void *hInst )
{
	DSCAPS	dscaps;
	HRESULT	hresult;

#ifndef _HARDLINKED
	if( !dsound_dll.link )
	{
		if( !Sys_LoadLibrary( &dsound_dll ))
			return SIS_FAILURE;
	}
#endif

	if(( hresult = /*i*/DirectSoundCreate( NULL, &pDS, NULL )) != DS_OK )
	{
#ifndef _XBOX
		if( hresult != DSERR_ALLOCATED )
			return SIS_FAILURE;
#endif
		Con_DPrintf( S_ERROR "DirectSound: hardware already in use\n" );
		return SIS_NOTAVAIL;
	}

#ifndef _XBOX
	dscaps.dwSize = sizeof( dscaps );
#endif

	if( IDirectSound8_GetCaps( pDS, &dscaps ) != DS_OK )
		Con_DPrintf( S_ERROR "DirectSound: failed to get capabilities\n" );

#ifndef _XBOX
	if( FBitSet( dscaps.dwFlags, DSCAPS_EMULDRIVER ))
	{
		Con_DPrintf( S_ERROR "DirectSound: driver not installed\n" );
		SNDDMA_FreeSound();
		return SIS_FAILURE;
	}
#endif

	if( !DS_CreateBuffers( hInst ))
		return SIS_FAILURE;

	return SIS_SUCCESS;
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/
int SNDDMA_Init( /*void *hInst*/ ) // MARTY
{
	// already initialized
	if( dma.initialized ) return true;

	memset( &dma, 0, sizeof( dma ));

	// init DirectSound
	if( SNDDMA_InitDirect( NULL/*hInst*/ ) != SIS_SUCCESS )
		return false;

	dma.initialized = true;
	snd_firsttime = false;

	return true;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos( void )
{
	int	s;
	MMTIME	mmtime;
	DWORD	dwWrite;

	if( !dma.initialized )
		return 0;
	
	mmtime.wType = TIME_SAMPLES;
	IDirectSoundBuffer_GetCurrentPosition( pDSBuf, &mmtime.u.sample, &dwWrite );
	s = mmtime.u.sample - mmstarttime.u.sample;

	s >>= SAMPLE_16BIT_SHIFT;
	s &= (dma.samples - 1);

	return s;
}

/*
==============
SNDDMA_GetSoundtime

update global soundtime
===============
*/
int SNDDMA_GetSoundtime( void )
{
	static int	buffers, oldsamplepos;
	int		samplepos, fullsamples;
	
	fullsamples = dma.samples / 2;

	// it is possible to miscount buffers
	// if it has wrapped twice between
	// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();

	if( samplepos < oldsamplepos )
	{
		buffers++; // buffer wrapped

		if( paintedtime > 0x40000000 )
		{	
			// time to chop things off to avoid 32 bit limits
			buffers = 0;
			paintedtime = fullsamples;
			S_StopAllSounds( true );
		}
	}

	oldsamplepos = samplepos;

	return (buffers * fullsamples + samplepos / 2);
}

/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
void SNDDMA_BeginPainting( void )
{
	int	reps;
	DWORD	dwSize2;
	DWORD	*pbuf, *pbuf2;
	HRESULT	hr;
	DWORD	dwStatus;

	if( !pDSBuf ) return;

	// if the buffer was lost or stopped, restore it and/or restart it
	if( IDirectSoundBuffer_GetStatus(pDSBuf, &dwStatus) != DS_OK )
		Con_DPrintf( S_ERROR "BeginPainting: couldn't get sound buffer status\n" );

#ifndef _XBOX // We have the buffer at all times on Xbox
	if( dwStatus & DSBSTATUS_BUFFERLOST )
		IDirectSoundBuffer_Restore(pDSBuf);
#endif

	if( !FBitSet( dwStatus, DSBSTATUS_PLAYING ))
		IDirectSoundBuffer_Play( pDSBuf, 0, 0, DSBPLAY_LOOPING );

	// lock the dsound buffer
	dma.buffer = NULL;
	reps = 0;

	while(( hr = IDirectSoundBuffer_Lock( pDSBuf, 0, gSndBufSize, &pbuf, &locksize, &pbuf2, &dwSize2, 0 )) != DS_OK )
	{
#ifndef _XBOX // We don't loose the buffer to other apps on Xbox
		if( hr != DSERR_BUFFERLOST )
		{
			Con_DPrintf( S_ERROR "BeginPainting: %s\n", DSoundError( hr ));
			S_Shutdown ();
			return;
		}
		else IDirectSoundBuffer_Restore(pDSBuf);
#else
		IDirectSoundBuffer_Restore(pDSBuf);
#endif

		if( ++reps > 2 ) return;
	}

	dma.buffer = (byte *)pbuf;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void SNDDMA_Submit( void )
{
	if( !dma.buffer ) return;
	// unlock the dsound buffer
	if( pDSBuf ) IDirectSoundBuffer_Unlock( pDSBuf, dma.buffer, locksize, NULL, 0 );
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown( void )
{
	if( !dma.initialized ) return;
	dma.initialized = false;
	SNDDMA_FreeSound();
}

/*
===========
S_Activate

Called when the main window gains or loses focus.
The window have been destroyed and recreated
between a deactivate and an activate.
===========
*/
#ifndef _XBOX
void S_Activate( qboolean active, void *hInst )
{
	if( !dma.initialized ) return;
	snd_hwnd = (HWND)hInst;

	if( !pDS || !snd_hwnd )
		return;

	if( active )
		DS_CreateBuffers( snd_hwnd );
	else DS_DestroyBuffers();
}
#endif