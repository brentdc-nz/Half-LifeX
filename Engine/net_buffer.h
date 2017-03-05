/*
net_buffer.h - network message io functions
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

#ifndef NET_BUFFER_H
#define NET_BUFFER_H

#include "features.h"

/*
==============================================================================

			MESSAGE IO FUNCTIONS
	       Handles byte ordering and avoids alignment errors
==============================================================================
*/

// Pad a number so it lies on an N byte boundary.
// So PAD_NUMBER(0,4) is 0 and PAD_NUMBER(1,4) is 4
#define PAD_NUMBER( num, boundary )	((( num ) + (( boundary ) - 1 )) / ( boundary )) * ( boundary )

_inline int BitByte( int bits )
{
	return PAD_NUMBER( bits, 8 ) >> 3;
}

typedef struct
{
	qboolean		bOverflow;	// overflow reading or writing
	const char	*pDebugName;	// buffer name (pointer to const name)

	byte		*pData;
	int		iCurBit;
	int		nDataBits;
} sizebuf_t;

#define BF_WriteUBitLong( bf, data, bits )	BF_WriteUBitLongExt( bf, data, bits, true );
#define BF_StartReading			BF_StartWriting
#define BF_GetNumBytesRead			BF_GetNumBytesWritten
#define BF_GetRealBytesRead			BF_GetRealBytesWritten
#define BF_GetNumBitsRead			BF_GetNumBitsWritten
#define BF_ReadBitAngles			BF_ReadBitVec3Coord
#define BF_ReadString( bf )			BF_ReadStringExt( bf, false )
#define BF_ReadStringLine( bf )		BF_ReadStringExt( bf, true )
#define BF_ReadAngle( bf )			(float)(BF_ReadChar( bf ) * ( 360.0f / 256.0f ))
#define BF_Init( bf, name, data, bytes )	BF_InitExt( bf, name, data, bytes, -1 )

// common functions
void BF_InitExt( sizebuf_t *bf, const char *pDebugName, void *pData, int nBytes, int nMaxBits );
void BF_InitMasks( void );	// called once at startup engine
void BF_SeekToBit( sizebuf_t *bf, int bitPos );
void BF_SeekToByte( sizebuf_t *bf, int bytePos );
void BF_ExciseBits( sizebuf_t *bf, int startbit, int bitstoremove );
qboolean BF_CheckOverflow( sizebuf_t *bf );
short BF_BigShort( short swap );

// init writing
void BF_StartWriting( sizebuf_t *bf, void *pData, int nBytes, int iStartBit, int nBits );
void BF_Clear( sizebuf_t *bf );

// Bit-write functions
void BF_WriteOneBit( sizebuf_t *bf, int nValue );
void BF_WriteUBitLongExt( sizebuf_t *bf, uint curData, int numbits, qboolean bCheckRange );
void BF_WriteSBitLong( sizebuf_t *bf, int data, int numbits );
void BF_WriteBitLong( sizebuf_t *bf, uint data, int numbits, qboolean bSigned );
qboolean BF_WriteBits( sizebuf_t *bf, const void *pData, int nBits );
void BF_WriteBitAngle( sizebuf_t *bf, float fAngle, int numbits );
void BF_WriteBitCoord( sizebuf_t *bf, const float f );
void BF_WriteBitFloat( sizebuf_t *bf, float val );
void BF_WriteBitVec3Coord( sizebuf_t *bf, const float *fa );
void BF_WriteBitNormal( sizebuf_t *bf, float f );
void BF_WriteBitVec3Normal( sizebuf_t *bf, const float *fa );

// Byte-write functions
void BF_WriteChar( sizebuf_t *bf, int val );
void BF_WriteByte( sizebuf_t *bf, int val );
void BF_WriteShort( sizebuf_t *bf, int val );
void BF_WriteWord( sizebuf_t *bf, int val );
void BF_WriteLong( sizebuf_t *bf, long val );
void BF_WriteCoord( sizebuf_t *bf, float val );
void BF_WriteFloat( sizebuf_t *bf, float val );
qboolean BF_WriteBytes( sizebuf_t *bf, const void *pBuf, int nBytes );	// same as MSG_WriteData
qboolean BF_WriteString( sizebuf_t *bf, const char *pStr );		// returns false if it overflows the buffer.

// delta-write functions
qboolean BF_WriteDeltaMovevars( sizebuf_t *sb, struct movevars_s *from, struct movevars_s *to );

// helper functions
_inline int BF_GetNumBytesWritten( sizebuf_t *bf ) { return BitByte( bf->iCurBit ); }
_inline int BF_GetRealBytesWritten( sizebuf_t *bf ) { return bf->iCurBit >> 3; }	// unpadded
_inline int BF_GetNumBitsWritten( sizebuf_t *bf ) { return bf->iCurBit; }
_inline int BF_GetMaxBits( sizebuf_t *bf ) { return bf->nDataBits; }
_inline int BF_GetMaxBytes( sizebuf_t *bf ) { return bf->nDataBits >> 3; }
_inline int BF_GetNumBitsLeft( sizebuf_t *bf ) { return bf->nDataBits - bf->iCurBit; }
_inline int BF_GetNumBytesLeft( sizebuf_t *bf ) { return BF_GetNumBitsLeft( bf ) >> 3; }
_inline byte *BF_GetData( sizebuf_t *bf ) { return bf->pData; }

// Bit-read functions
int BF_ReadOneBit( sizebuf_t *bf );
float BF_ReadBitFloat( sizebuf_t *bf );
qboolean BF_ReadBits( sizebuf_t *bf, void *pOutData, int nBits );
float BF_ReadBitAngle( sizebuf_t *bf, int numbits );
int BF_ReadSBitLong( sizebuf_t *bf, int numbits );
uint BF_ReadUBitLong( sizebuf_t *bf, int numbits );
uint BF_ReadBitLong( sizebuf_t *bf, int numbits, qboolean bSigned );
float BF_ReadBitCoord( sizebuf_t *bf );
void BF_ReadBitVec3Coord( sizebuf_t *bf, vec3_t fa );
float BF_ReadBitNormal( sizebuf_t *bf );
void BF_ReadBitVec3Normal( sizebuf_t *bf, vec3_t fa );

// Byte-read functions
int BF_ReadChar( sizebuf_t *bf );
int BF_ReadByte( sizebuf_t *bf );
int BF_ReadShort( sizebuf_t *bf );
int BF_ReadWord( sizebuf_t *bf );
long BF_ReadLong( sizebuf_t *bf );
float BF_ReadCoord( sizebuf_t *bf );
float BF_ReadFloat( sizebuf_t *bf );
qboolean BF_ReadBytes( sizebuf_t *bf, void *pOut, int nBytes );
char *BF_ReadStringExt( sizebuf_t *bf, qboolean bLine );

// delta-read functions
void BF_ReadDeltaMovevars( sizebuf_t *sb, struct movevars_s *from, struct movevars_s *to );
					
#endif//NET_BUFFER_H