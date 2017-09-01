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

#ifndef _XBOX //MARTY
#define iDirectSoundCreate( a, b, c )	pDirectSoundCreate( a, b, c )

static HRESULT ( _stdcall *pDirectSoundCreate)(GUID* lpGUID, LPDIRECTSOUND* lpDS, IUnknown* pUnkOuter );

static dllfunc_t dsound_funcs[] =
{
{ "DirectSoundCreate", (void **) &pDirectSoundCreate },
{ NULL, NULL }
};

dll_info_t dsound_dll = { "dsound.dll", dsound_funcs, false };
#endif //_XBOX

#define SAMPLE_16BIT_SHIFT		1
#define SECONDARY_BUFFER_SIZE		0x10000

typedef enum
{
	SIS_SUCCESS,
	SIS_FAILURE,
	SIS_NOTAVAIL
} si_state_t;

convar_t		*s_primary;

static HWND	snd_hwnd;
static qboolean	snd_firsttime = true;
static qboolean	primary_format_set;

/* 
=======================================================================
Global variables. Must be visible to window-procedure function 
so it can unlock and free the data block after it has been played.
=======================================================================
*/ 
DWORD		locksize;
/*H*/LPSTR		lpData, lpData2; //MARTY
//LPWAVEHDR		lpWaveHdr; //MARTY
DWORD		gSndBufSize;
MMTIME		mmstarttime;
LPDIRECTSOUNDBUFFER pDSBuf, pDSPBuf;
LPDIRECTSOUND	pDS;

qboolean SNDDMA_InitDirect( void *hInst );
void SNDDMA_FreeSound( void );

/* 
static const char *DSoundError( int error ) //MARTY
{
	switch( error )
	{
	case DSERR_BUFFERLOST:
		return "DSERR_BUFFERLOST";
	case DSERR_INVALIDCALL:
		return "DSERR_INVALIDCALLS";
	case DSERR_INVALIDPARAM:
		return "DSERR_INVALIDPARAM";
	case DSERR_PRIOLEVELNEEDED:
		return "DSERR_PRIOLEVELNEEDED";
	}
	return "Unknown Error";
}*/

/*
==================
DS_CreateBuffers
==================
*/
static qboolean DS_CreateBuffers( void *hInst )
{
	DSBUFFERDESC	dsbuf;
//	DSBCAPS		dsbcaps; //MARTY
	WAVEFORMATEX	pformat, format;
#ifdef _XBOX //MARTY
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
	Q_memset( &format, 0, sizeof( format ));

	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.wBitsPerSample = 16;
	format.nSamplesPerSec = SOUND_DMA_SPEED;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign; 
	format.cbSize = 0;

	MsgDev( D_NOTE, "DS_CreateBuffers: initialize\n" );

	MsgDev( D_NOTE, "DS_CreateBuffers: setting EXCLUSIVE coop level " );
	if( DS_OK != IDirectSound_SetCooperativeLevel( pDS, hInst, DSSCL_EXCLUSIVE ))
	{
		MsgDev( D_NOTE, "- failed\n" );
		SNDDMA_FreeSound();
		return false;
	}
	MsgDev( D_NOTE, "- ok\n" );

	// get access to the primary buffer, if possible, so we can set the sound hardware format
	Q_memset( &dsbuf, 0, sizeof( dsbuf ));
	dsbuf.dwSize = sizeof( DSBUFFERDESC );
	dsbuf.dwFlags = 0;//DSBCAPS_PRIMARYBUFFER; //MARTY
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = &format;//NULL //MARTY

//	Q_memset( &dsbcaps, 0, sizeof( dsbcaps )); //MARTY
//	dsbcaps.dwSize = sizeof( dsbcaps ); //MARTY
	primary_format_set = false;

	MsgDev( D_NOTE, "DS_CreateBuffers: creating primary buffer " );
	if( IDirectSound_CreateSoundBuffer( pDS, &dsbuf, &pDSPBuf, NULL ) == DS_OK )
	{
		pformat = format;

		MsgDev( D_NOTE, "- ok\n" );
		if( snd_firsttime )
			MsgDev( D_NOTE, "DS_CreateBuffers: setting primary sound format " );

		if( IDirectSoundBuffer8_SetFormat( pDSPBuf, &pformat ) != DS_OK )
		{
			if( snd_firsttime )
				MsgDev( D_NOTE, "- failed\n" );
		}
		else
		{
			if( snd_firsttime )
				MsgDev( D_NOTE, "- ok\n" );
			primary_format_set = true;
		}
	}
	else MsgDev( D_NOTE, "- failed\n" );

	if( !primary_format_set || !s_primary->integer )
	{
		// create the secondary buffer we'll actually work with
		Q_memset( &dsbuf, 0, sizeof( dsbuf ));
		dsbuf.dwSize = sizeof( DSBUFFERDESC );
		dsbuf.dwFlags = (DSBCAPS_CTRLFREQUENCY/*|DSBCAPS_LOCSOFTWARE*/); //MARTY
		dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
		dsbuf.lpwfxFormat = &format;

//		Q_memset( &dsbcaps, 0, sizeof( dsbcaps ));
//		dsbcaps.dwSize = sizeof( dsbcaps );

		MsgDev( D_NOTE, "DS_CreateBuffers: creating secondary buffer " );
		if( IDirectSound_CreateSoundBuffer( pDS, &dsbuf, &pDSBuf, NULL ) == DS_OK )
		{
			MsgDev( D_NOTE, "- ok\n" );
		}
		else
		{
			// couldn't get hardware, fallback to software.
//			dsbuf.dwFlags = (DSBCAPS_LOCSOFTWARE|DSBCAPS_GETCURRENTPOSITION2); //MARTY
			if( IDirectSound_CreateSoundBuffer( pDS, &dsbuf, &pDSBuf, NULL ) != DS_OK )
			{
				MsgDev( D_NOTE, "- failed\n" );
				SNDDMA_FreeSound ();
				return false;
			}
			MsgDev( D_INFO, "- failed. forced to software\n" );
		}

/*		if( pDSBuf->lpVtbl->GetCaps( pDSBuf, &dsbcaps ) != DS_OK ) //MARTY
		{
			MsgDev( D_ERROR, "DS_CreateBuffers: GetCaps failed\n");
			SNDDMA_FreeSound ();
			return false;
		}
*/		MsgDev( D_NOTE, "DS_CreateBuffers: using secondary sound buffer\n" );
	}
	else
	{
		MsgDev( D_NOTE, "DS_CreateBuffers: using primary sound buffer\n" );
		MsgDev( D_NOTE, "DS_CreateBuffers: setting WRITEPRIMARY coop level " );
		if( IDirectSound_SetCooperativeLevel( pDS, hInst, DSSCL_WRITEPRIMARY ) != DS_OK )
		{
			MsgDev( D_NOTE, "- failed\n" );
			SNDDMA_FreeSound ();
			return false;
		}
		MsgDev( D_NOTE, "- ok\n" );

/*		if( pDSPBuf->lpVtbl->GetCaps( pDSPBuf, &dsbcaps ) != DS_OK ) //MARTY
		{
			MsgDev( D_ERROR, "DS_CreateBuffers: GetCaps failed\n");
			SNDDMA_FreeSound ();
			return false;
		}
*/		pDSBuf = pDSPBuf;
	}

	// make sure mixer is active
	if( IDirectSoundBuffer_Play( pDSBuf, 0, 0, DSBPLAY_LOOPING ) != DS_OK )
	{
		MsgDev( D_ERROR, "DS_CreateBuffers: looped sound play failed\n" );
		SNDDMA_FreeSound ();
		return false;
	}

	// we don't want anyone to access the buffer directly w/o locking it first
	lpData = NULL;
	dma.samplepos = 0;
	snd_hwnd = (HWND)hInst;
	gSndBufSize = dsbuf.dwBufferBytes;//dsbcaps.dwBufferBytes; //MARTY
	dma.samples = gSndBufSize / 2;
	dma.buffer = (byte *)lpData;

#ifdef _XBOX //MARTY
	dsmb.dwMixBinCount = 8;
	dsmb.lpMixBinVolumePairs = dsmbvp;
 
	IDirectSoundBuffer_SetHeadroom(pDSBuf, DSBHEADROOM_MIN);
	IDirectSoundBuffer_SetMixBins(pDSBuf, &dsmb);

	IDirectSoundBuffer_SetVolume(pDSBuf, DSBVOLUME_MAX);
#endif //_XBOX

	SNDDMA_BeginPainting();
	if( dma.buffer ) Q_memset( dma.buffer, 0, dma.samples * 2 );
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
	MsgDev( D_NOTE, "DS_DestroyBuffers: shutdown\n" );

	if( pDS )
	{
		MsgDev( D_NOTE, "DS_DestroyBuffers: setting NORMAL coop level\n" );
		IDirectSound_SetCooperativeLevel( pDS, snd_hwnd, DSSCL_NORMAL );
	}

	if( pDSBuf )
	{
		MsgDev( D_NOTE, "DS_DestroyBuffers: stopping and releasing sound buffer\n" );
		IDirectSoundBuffer_Stop( pDSBuf );
		IDirectSoundBuffer_Release( pDSBuf );
	}

	// only release primary buffer if it's not also the mixing buffer we just released
	if( pDSPBuf && ( pDSBuf != pDSPBuf ))
	{
		MsgDev( D_NOTE, "DS_DestroyBuffers: releasing primary buffer\n" );
		IDirectSoundBuffer_Release( pDSPBuf );
	}

	pDSBuf = NULL;
	pDSPBuf = NULL;
	dma.buffer = NULL;
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
#ifndef _XBOX //MARTY
		Sys_FreeLibrary( &dsound_dll );
#endif
	}

	pDS = NULL;
	pDSBuf = NULL;
	pDSPBuf = NULL;
	lpData = NULL;
//	lpWaveHdr = NULL;
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

#ifndef _XBOX //MARTY
	if( !dsound_dll.link )
	{
		if( !Sys_LoadLibrary( &dsound_dll ))
			return SIS_FAILURE;
	}
#endif

	MsgDev( D_NOTE, "SNDDMA_InitDirect: creating DS object " );
	if(( hresult = /*i*/DirectSoundCreate( NULL, &pDS, NULL )) != DS_OK )
	{
		if( /*hresult != DSERR_ALLOCATED*/0 ) //MARTY FIXME WIP
		{
			MsgDev( D_NOTE, "- failed\n" );
			return SIS_FAILURE;
		}

		MsgDev( D_NOTE, "- failed, hardware already in use\n" );
		return SIS_NOTAVAIL;
	}

	MsgDev( D_NOTE, "- ok\n" );
//	dscaps.dwSize = sizeof( dscaps ); //MARTY

	if( IDirectSound8_GetCaps( pDS, &dscaps ) != DS_OK )
		MsgDev( D_ERROR, "SNDDMA_InitDirect: GetCaps failed\n");

#ifndef _XBOX //MARTY
	if( dscaps.dwFlags & DSCAPS_EMULDRIVER )
	{
		MsgDev( D_ERROR, "SNDDMA_InitDirect: no DSound driver found\n" );
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
qboolean SNDDMA_Init( /*void *hInst*/ ) //MARTY
{
	si_state_t	stat = SIS_FAILURE;	// assume DirectSound won't initialize

	// already initialized
	if( dma.initialized ) return true;

	Q_memset( &dma, 0, sizeof( dma ));

	s_primary = Cvar_Get( "s_primary", "0", CVAR_INIT, "use direct primary buffer" ); 

	// init DirectSound
	stat = SNDDMA_InitDirect( /*hInst*/NULL ); //MARTY

	if( stat == SIS_SUCCESS )
	{
		if( snd_firsttime )
			MsgDev( D_INFO, "Audio: DirectSound\n" );
	}
	else
	{
		if( snd_firsttime )
			MsgDev( D_ERROR, "SNDDMA_Init: can't initialize sound device\n" );
		return false;
	}

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
			S_StopAllSounds();
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
		MsgDev( D_WARN, "SNDDMA_BeginPainting: couldn't get sound buffer status\n" );

#ifndef _XBOX //MARTY	
	if( dwStatus & DSBSTATUS_BUFFERLOST )
		IDirectSoundBuffer_Restore(pDSBuf);
#endif	

	if( !( dwStatus & DSBSTATUS_PLAYING ))
		IDirectSoundBuffer_Play( pDSBuf, 0, 0, DSBPLAY_LOOPING );

	// lock the dsound buffer
	dma.buffer = NULL;
	reps = 0;

	while(( hr = IDirectSoundBuffer_Lock( pDSBuf, 0, gSndBufSize, &pbuf, &locksize, &pbuf2, &dwSize2, 0 )) != DS_OK )
	{
		if( /*hr != DSERR_BUFFERLOST*/ 0 ) //MARTY FIXME WIP
		{
//			MsgDev( D_ERROR, "SNDDMA_BeginPainting: lock error '%s'\n", DSoundError( hr ));
			S_Shutdown ();
			return;
		}
		else IDirectSoundBuffer_Restore(pDSBuf);

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
S_PrintDeviceName
===========
*/
void S_PrintDeviceName( void )
{
	Msg( "Audio: DirectSound\n" );
}

/*
===========
S_Activate

Called when the main window gains or loses focus.
The window have been destroyed and recreated
between a deactivate and an activate.
===========
*/
#ifndef _XBOX //MARTY
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