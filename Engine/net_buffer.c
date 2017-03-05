/*
net_buffer.c - network bitbuffer io functions
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
#include "protocol.h"
#include "net_buffer.h"
#include "mathlib.h"

// precalculated bit masks for WriteUBitLong.
// Using these tables instead of doing the calculations
// gives a 33% speedup in WriteUBitLong.

static dword	BitWriteMasks[32][33];
static dword	ExtraMasks[32];

short BF_BigShort( short swap )
{
	short *s = &swap;
	
	__asm {
		mov ebx, s
		mov al, [ebx+1]
		mov ah, [ebx  ]
		mov [ebx], ax
	}
	return *s;
}

void BF_InitMasks( void )
{
	uint	startbit, endbit;
	uint	maskBit, nBitsLeft;

	for( startbit = 0; startbit < 32; startbit++ )
	{
		for( nBitsLeft = 0; nBitsLeft < 33; nBitsLeft++ )
		{
			endbit = startbit + nBitsLeft;

			BitWriteMasks[startbit][nBitsLeft] = BIT( startbit ) - 1;
			if( endbit < 32 ) BitWriteMasks[startbit][nBitsLeft] |= ~(BIT( endbit ) - 1 );
		}
	}

	for( maskBit = 0; maskBit < 32; maskBit++ )
		ExtraMasks[maskBit] = BIT( maskBit ) - 1;
}
 
void BF_InitExt( sizebuf_t *bf, const char *pDebugName, void *pData, int nBytes, int nMaxBits )
{
	bf->pDebugName = pDebugName;

	BF_StartWriting( bf, pData, nBytes, 0, nMaxBits );
}

void BF_StartWriting( sizebuf_t *bf, void *pData, int nBytes, int iStartBit, int nBits )
{
	// make sure it's dword aligned and padded.
//	Assert(( nBytes % 4 ) == 0 );
	Assert(((dword)pData & 3 ) == 0 );

	bf->pData = (byte *)pData;

	if( nBits == -1 )
	{
		bf->nDataBits = nBytes << 3;
	}
	else
	{
		Assert( nBits <= nBytes * 8 );
		bf->nDataBits = nBits;
	}

	bf->iCurBit = iStartBit;
	bf->bOverflow = false;
}

/*
=======================
MSG_Clear

for clearing overflowed buffer
=======================
*/
void BF_Clear( sizebuf_t *bf )
{
	bf->iCurBit = 0;
	bf->bOverflow = false;
}

static qboolean BF_Overflow( sizebuf_t *bf, int nBits )
{
	if( bf->iCurBit + nBits > bf->nDataBits )
		bf->bOverflow = true;
	return bf->bOverflow;
}

qboolean BF_CheckOverflow( sizebuf_t *bf )
{
	ASSERT( bf );
	
	return BF_Overflow( bf, 0 );
}

void BF_SeekToBit( sizebuf_t *bf, int bitPos )
{
	bf->iCurBit = bitPos;
}

void BF_SeekToByte( sizebuf_t *bf, int bytePos )
{
	bf->iCurBit = bytePos << 3;
}

void BF_WriteOneBit( sizebuf_t *bf, int nValue )
{
	if( !BF_Overflow( bf, 1 ))
	{
		if( nValue ) bf->pData[bf->iCurBit>>3] |= (1 << ( bf->iCurBit & 7 ));
		else bf->pData[bf->iCurBit>>3] &= ~(1 << ( bf->iCurBit & 7 ));

		bf->iCurBit++;
	}
}

void BF_WriteUBitLongExt( sizebuf_t *bf, uint curData, int numbits, qboolean bCheckRange )
{
#ifdef _NETDEBUG
	// make sure it doesn't overflow.
	if( bCheckRange && numbits < 32 )
	{
		if( curData >= (dword)BIT( numbits ))
			MsgDev( D_ERROR, "Write%s: out of range value!\n", bf->pDebugName );
	}
#endif
	Assert( numbits >= 0 && numbits <= 32 );

	// bounds checking..
	if(( bf->iCurBit + numbits ) > bf->nDataBits )
	{
		bf->bOverflow = true;
		bf->iCurBit = bf->nDataBits;
	}
	else
	{
		int	nBitsLeft = numbits;
		int	iCurBit = bf->iCurBit;
		uint	iDWord = iCurBit >> 5;	// Mask in a dword.
		dword	iCurBitMasked;
		int	nBitsWritten;

		Assert(( iDWord * 4 + sizeof( long )) <= (uint)BF_GetMaxBytes( bf ));

		iCurBitMasked = iCurBit & 31;
		((dword *)bf->pData)[iDWord] &= BitWriteMasks[iCurBitMasked][nBitsLeft];
		((dword *)bf->pData)[iDWord] |= curData << iCurBitMasked;

		// did it span a dword?
		nBitsWritten = 32 - iCurBitMasked;

		if( nBitsWritten < nBitsLeft )
		{
			nBitsLeft -= nBitsWritten;
			iCurBit += nBitsWritten;
			curData >>= nBitsWritten;

			iCurBitMasked = iCurBit & 31;
			((dword *)bf->pData)[iDWord+1] &= BitWriteMasks[iCurBitMasked][nBitsLeft];
			((dword *)bf->pData)[iDWord+1] |= curData << iCurBitMasked;
		}
		bf->iCurBit += numbits;
	}
}

/*
=======================
BF_WriteSBitLong

sign bit comes first
=======================
*/
void BF_WriteSBitLong( sizebuf_t *bf, int data, int numbits )
{
	// do we have a valid # of bits to encode with?
	Assert( numbits >= 1 );

	// NOTE: it does this wierdness here so it's bit-compatible with regular integer data in the buffer.
	// (Some old code writes direct integers right into the buffer).
	if( data < 0 )
	{
#ifdef _NETDEBUG
	if( numbits < 32 )
	{
		// Make sure it doesn't overflow.
		if( data < 0 )
		{
			Assert( data >= -BIT( numbits - 1 ));
		}
		else
		{
			Assert( data < BIT( numbits - 1 ));
		}
	}
#endif
		BF_WriteUBitLongExt( bf, (uint)( 0x80000000 + data ), numbits - 1, false );
		BF_WriteOneBit( bf, 1 );
	}
	else
	{
		BF_WriteUBitLong( bf, (uint)data, numbits - 1 );
		BF_WriteOneBit( bf, 0 );
	}
}

void BF_WriteBitLong( sizebuf_t *bf, uint data, int numbits, qboolean bSigned )
{
	if( bSigned )
		BF_WriteSBitLong( bf, (int)data, numbits );
	else BF_WriteUBitLong( bf, data, numbits );
}

qboolean BF_WriteBits( sizebuf_t *bf, const void *pData, int nBits )
{
	byte	*pOut = (byte *)pData;
	int	nBitsLeft = nBits;

	// get output dword-aligned.
	while((( dword )pOut & 3 ) != 0 && nBitsLeft >= 8 )
	{
		BF_WriteUBitLongExt( bf, *pOut, 8, false );

		nBitsLeft -= 8;
		++pOut;
	}

	// read dwords.
	while( nBitsLeft >= 32 )
	{
		BF_WriteUBitLongExt( bf, *(( dword *)pOut ), 32, false );

		pOut += sizeof( dword );
		nBitsLeft -= 32;
	}

	// read the remaining bytes.
	while( nBitsLeft >= 8 )
	{
		BF_WriteUBitLongExt( bf, *pOut, 8, false );

		nBitsLeft -= 8;
		++pOut;
	}
	
	// Read the remaining bits.
	if( nBitsLeft )
	{
		BF_WriteUBitLongExt( bf, *pOut, nBitsLeft, false );
	}

	return !bf->bOverflow;
}


void BF_WriteBitAngle( sizebuf_t *bf, float fAngle, int numbits )
{
	uint	mask, shift;
	int	d;

	// clamp the angle before receiving
	if( fAngle > 360.0f ) fAngle -= 360.0f; 
	else if( fAngle < 0 ) fAngle += 360.0f;

	shift = ( 1 << numbits );
	mask = shift - 1;

	d = (int)( fAngle * shift ) / 360;
	d &= mask;

	BF_WriteUBitLong( bf, (uint)d, numbits );
}

void BF_WriteBitCoord( sizebuf_t *bf, const float f )
{
	int	signbit = ( f <= -COORD_RESOLUTION );
	int	fractval = abs(( int )( f * COORD_DENOMINATOR )) & ( COORD_DENOMINATOR - 1 );
	int	intval = (int)abs( f );

	// Send the bit flags that indicate whether we have an integer part and/or a fraction part.
	BF_WriteOneBit( bf, intval );
	BF_WriteOneBit( bf, fractval );

	if( intval || fractval )
	{
		// send the sign bit
		BF_WriteOneBit( bf, signbit );

		// send the integer if we have one.
		if( intval )
		{
			// adjust the integers from [1..MAX_COORD_VALUE] to [0..MAX_COORD_VALUE-1]
			intval--;
			BF_WriteUBitLong( bf, (uint)intval, COORD_INTEGER_BITS );
		}
		
		// send the fraction if we have one
		if( fractval )
		{
			BF_WriteUBitLong( bf, (uint)fractval, COORD_FRACTIONAL_BITS );
		}
	}
}

void BF_WriteCoord( sizebuf_t *bf, float val )
{
	// g-cont. we loose precision here but keep old size of coord variable!
	if( host.features & ENGINE_WRITE_LARGE_COORD )
		BF_WriteShort( bf, (int)( val * 2.0f ));
	else BF_WriteShort( bf, (int)( val * 8.0f ));
}

void BF_WriteBitFloat( sizebuf_t *bf, float val )
{
	long	intVal;

	ASSERT( sizeof( long ) == sizeof( float ));
	ASSERT( sizeof( float ) == 4 );

	intVal = *((long *)&val );
	BF_WriteUBitLong( bf, intVal, 32 );
}

void BF_WriteBitVec3Coord( sizebuf_t *bf, const float *fa )
{
	int	xflag, yflag, zflag;

	xflag = ( fa[0] >= COORD_RESOLUTION ) || ( fa[0] <= -COORD_RESOLUTION );
	yflag = ( fa[1] >= COORD_RESOLUTION ) || ( fa[1] <= -COORD_RESOLUTION );
	zflag = ( fa[2] >= COORD_RESOLUTION ) || ( fa[2] <= -COORD_RESOLUTION );

	BF_WriteOneBit( bf, xflag );
	BF_WriteOneBit( bf, yflag );
	BF_WriteOneBit( bf, zflag );

	if( xflag ) BF_WriteBitCoord( bf, fa[0] );
	if( yflag ) BF_WriteBitCoord( bf, fa[1] );
	if( zflag ) BF_WriteBitCoord( bf, fa[2] );
}

void BF_WriteBitNormal( sizebuf_t *bf, float f )
{
	int	signbit = ( f <= -NORMAL_RESOLUTION );
	uint	fractval = abs(( int )(f * NORMAL_DENOMINATOR ));

	if( fractval > NORMAL_DENOMINATOR )
		fractval = NORMAL_DENOMINATOR;

	// send the sign bit
	BF_WriteOneBit( bf, signbit );

	// send the fractional component
	BF_WriteUBitLong( bf, fractval, NORMAL_FRACTIONAL_BITS );
}

void BF_WriteBitVec3Normal( sizebuf_t *bf, const float *fa )
{
	int	xflag, yflag;
	int	signbit;

	xflag = ( fa[0] >= NORMAL_RESOLUTION ) || ( fa[0] <= -NORMAL_RESOLUTION );
	yflag = ( fa[1] >= NORMAL_RESOLUTION ) || ( fa[1] <= -NORMAL_RESOLUTION );

	BF_WriteOneBit( bf, xflag );
	BF_WriteOneBit( bf, yflag );

	if( xflag ) BF_WriteBitNormal( bf, fa[0] );
	if( yflag ) BF_WriteBitNormal( bf, fa[1] );
	
	// Write z sign bit
	signbit = ( fa[2] <= -NORMAL_RESOLUTION );
	BF_WriteOneBit( bf, signbit );
}

void BF_WriteChar( sizebuf_t *bf, int val )
{
	BF_WriteSBitLong( bf, val, sizeof( char ) << 3 );
}

void BF_WriteByte( sizebuf_t *bf, int val )
{
	BF_WriteUBitLong( bf, val, sizeof( byte ) << 3 );
}

void BF_WriteShort( sizebuf_t *bf, int val )
{
	BF_WriteSBitLong( bf, val, sizeof(short ) << 3 );
}

void BF_WriteWord( sizebuf_t *bf, int val )
{
	BF_WriteUBitLong( bf, val, sizeof( word ) << 3 );
}

void BF_WriteLong( sizebuf_t *bf, long val )
{
	BF_WriteSBitLong( bf, val, sizeof( long ) << 3 );
}

void BF_WriteFloat( sizebuf_t *bf, float val )
{
	BF_WriteBits( bf, &val, sizeof( val ) << 3 );
}

qboolean BF_WriteBytes( sizebuf_t *bf, const void *pBuf, int nBytes )
{
	return BF_WriteBits( bf, pBuf, nBytes << 3 );
}

qboolean BF_WriteString( sizebuf_t *bf, const char *pStr )
{
	if( pStr )
	{
		do
		{
			BF_WriteChar( bf, *pStr );
			pStr++;
		} while( *( pStr - 1 ));
	}
	else BF_WriteChar( bf, 0 );
	
	return !bf->bOverflow;
}

int BF_ReadOneBit( sizebuf_t *bf )
{
	if( !BF_Overflow( bf, 1 ))
	{
		int value = bf->pData[bf->iCurBit >> 3] & (1 << ( bf->iCurBit & 7 ));
		bf->iCurBit++;
		return !!value;
	}
	return 0;
}

uint BF_ReadUBitLong( sizebuf_t *bf, int numbits )
{
	int	idword1;
	uint	dword1, ret;

	if( numbits == 8 )
	{
		int leftBits = BF_GetNumBitsLeft( bf );

		if( leftBits >= 0 && leftBits < 8 )
			return 0;	// end of message
	}

	if(( bf->iCurBit + numbits ) > bf->nDataBits )
	{
		bf->bOverflow = true;
		bf->iCurBit = bf->nDataBits;
		return 0;
	}

	ASSERT( numbits > 0 && numbits <= 32 );

	// Read the current dword.
	idword1 = bf->iCurBit >> 5;
	dword1 = ((uint *)bf->pData)[idword1];
	dword1 >>= ( bf->iCurBit & 31 );	// get the bits we're interested in.

	bf->iCurBit += numbits;
	ret = dword1;

	// Does it span this dword?
	if(( bf->iCurBit - 1 ) >> 5 == idword1 )
	{
		if( numbits != 32 )
			ret &= ExtraMasks[numbits];
	}
	else
	{
		int	nExtraBits = bf->iCurBit & 31;
		uint	dword2 = ((uint *)bf->pData)[idword1+1] & ExtraMasks[nExtraBits];
		
		// no need to mask since we hit the end of the dword.
		// shift the second dword's part into the high bits.
		ret |= (dword2 << ( numbits - nExtraBits ));
	}
	return ret;
}

float BF_ReadBitFloat( sizebuf_t *bf )
{
	long	val;
	int	bit, byte;

	ASSERT( sizeof( float ) == sizeof( long ));
	ASSERT( sizeof( float ) == 4 );

	if( BF_Overflow( bf, 32 ))
		return 0.0f;

	bit = bf->iCurBit & 0x7;
	byte = bf->iCurBit >> 3;

	val = bf->pData[byte] >> bit;
	val |= ((int)bf->pData[byte + 1]) << ( 8 - bit );
	val |= ((int)bf->pData[byte + 2]) << ( 16 - bit );
	val |= ((int)bf->pData[byte + 3]) << ( 24 - bit );

	if( bit != 0 )
		val |= ((int)bf->pData[byte + 4]) << ( 32 - bit );
	bf->iCurBit += 32;

	return *((float *)&val);
}

qboolean BF_ReadBits( sizebuf_t *bf, void *pOutData, int nBits )
{
	byte	*pOut = (byte *)pOutData;
	int	nBitsLeft = nBits;
	
	// get output dword-aligned.
	while((( dword )pOut & 3) != 0 && nBitsLeft >= 8 )
	{
		*pOut = (byte)BF_ReadUBitLong( bf, 8 );
		++pOut;
		nBitsLeft -= 8;
	}

	// read dwords.
	while( nBitsLeft >= 32 )
	{
		*((dword *)pOut) = BF_ReadUBitLong( bf, 32 );
		pOut += sizeof( dword );
		nBitsLeft -= 32;
	}

	// read the remaining bytes.
	while( nBitsLeft >= 8 )
	{
		*pOut = BF_ReadUBitLong( bf, 8 );
		++pOut;
		nBitsLeft -= 8;
	}
	
	// read the remaining bits.
	if( nBitsLeft )
	{
		*pOut = BF_ReadUBitLong( bf, nBitsLeft );
	}

	return !bf->bOverflow;
}

float BF_ReadBitAngle( sizebuf_t *bf, int numbits )
{
	float	fReturn, shift;
	int	i;

	shift = (float)( 1 << numbits );

	i = BF_ReadUBitLong( bf, numbits );
	fReturn = (float)i * ( 360.0f / shift );

	// clamp the finale angle
	if( fReturn < -180.0f ) fReturn += 360.0f; 
	else if( fReturn > 180.0f ) fReturn -= 360.0f;

	return fReturn;
}

// Append numbits least significant bits from data to the current bit stream
int BF_ReadSBitLong( sizebuf_t *bf, int numbits )
{
	int	r, sign;

	r = BF_ReadUBitLong( bf, numbits - 1 );

	// NOTE: it does this wierdness here so it's bit-compatible with regular integer data in the buffer.
	// (Some old code writes direct integers right into the buffer).
	sign = BF_ReadOneBit( bf );
	if( sign ) r = -( BIT( numbits - 1 ) - r );

	return r;
}

uint BF_ReadBitLong( sizebuf_t *bf, int numbits, qboolean bSigned )
{
	if( bSigned )
		return (uint)BF_ReadSBitLong( bf, numbits );
	return BF_ReadUBitLong( bf, numbits );
}


// Basic Coordinate Routines (these contain bit-field size AND fixed point scaling constants)
float BF_ReadBitCoord( sizebuf_t *bf )
{
	int	intval = 0, fractval = 0, signbit = 0;
	float	value = 0.0;

	// read the required integer and fraction flags
	intval = BF_ReadOneBit( bf );
	fractval = BF_ReadOneBit( bf );

	// if we got either parse them, otherwise it's a zero.
	if( intval || fractval )
	{
		// read the sign bit
		signbit = BF_ReadOneBit( bf );

		// if there's an integer, read it in
		if( intval )
		{
			// adjust the integers from [0..MAX_COORD_VALUE-1] to [1..MAX_COORD_VALUE]
			intval = BF_ReadUBitLong( bf, COORD_INTEGER_BITS ) + 1;
		}

		// if there's a fraction, read it in
		if( fractval )
		{
			fractval = BF_ReadUBitLong( bf, COORD_FRACTIONAL_BITS );
		}

		// calculate the correct floating point value
		value = intval + ((float)fractval * COORD_RESOLUTION );

		// fixup the sign if negative.
		if( signbit ) value = -value;
	}
	return value;
}

void BF_ReadBitVec3Coord( sizebuf_t *bf, vec3_t fa )
{
	int	xflag, yflag, zflag;

	// This vector must be initialized! Otherwise, If any of the flags aren't set, 
	// the corresponding component will not be read and will be stack garbage.
	fa[0] = fa[1] = fa[2] = 0.0f;

	xflag = BF_ReadOneBit( bf );
	yflag = BF_ReadOneBit( bf ); 
	zflag = BF_ReadOneBit( bf );

	if( xflag ) fa[0] = BF_ReadBitCoord( bf );
	if( yflag ) fa[1] = BF_ReadBitCoord( bf );
	if( zflag ) fa[2] = BF_ReadBitCoord( bf );
}

float BF_ReadBitNormal( sizebuf_t *bf )
{
	// read the sign bit
	int	signbit = BF_ReadOneBit( bf );

	// read the fractional part
	uint fractval = BF_ReadUBitLong( bf, NORMAL_FRACTIONAL_BITS );

	// calculate the correct floating point value
	float value = (float)fractval * NORMAL_RESOLUTION;

	// fixup the sign if negative.
	if( signbit ) value = -value;

	return value;
}

void BF_ReadBitVec3Normal( sizebuf_t *bf, vec3_t fa )
{
	int	xflag = BF_ReadOneBit( bf );
	int	yflag = BF_ReadOneBit( bf ); 
	int	znegative;
	float	fafafbfb;

	if( xflag ) fa[0] = BF_ReadBitNormal( bf );
	else fa[0] = 0.0f;

	if( yflag ) fa[1] = BF_ReadBitNormal( bf );
	else fa[1] = 0.0f;

	// the first two imply the third (but not its sign)
	znegative = BF_ReadOneBit( bf );
	fafafbfb = fa[0] * fa[0] + fa[1] * fa[1];

	if( fafafbfb < 1.0f )
		fa[2] = sqrt( 1.0f - fafafbfb );
	else fa[2] = 0.0f;

	if( znegative ) fa[2] = -fa[2];
}

int BF_ReadChar( sizebuf_t *bf )
{
	return BF_ReadSBitLong( bf, sizeof( char ) << 3 );
}

int BF_ReadByte( sizebuf_t *bf )
{
	return BF_ReadUBitLong( bf, sizeof( byte ) << 3 );
}

int BF_ReadShort( sizebuf_t *bf )
{
	return BF_ReadSBitLong( bf, sizeof( short ) << 3 );
}

int BF_ReadWord( sizebuf_t *bf )
{
	return BF_ReadUBitLong( bf, sizeof( word ) << 3 );
}

float BF_ReadCoord( sizebuf_t *bf )
{
	// g-cont. we loose precision here but keep old size of coord variable!
	if( host.features & ENGINE_WRITE_LARGE_COORD )
		return (float)(BF_ReadShort( bf ) * ( 1.0f / 2.0f ));
	return (float)(BF_ReadShort( bf ) * ( 1.0f / 8.0f ));
}

long BF_ReadLong( sizebuf_t *bf )
{
	return BF_ReadSBitLong( bf, sizeof( long ) << 3 );
}

float BF_ReadFloat( sizebuf_t *bf )
{
	float	ret;
	ASSERT( sizeof( ret ) == 4 );

	BF_ReadBits( bf, &ret, 32 );

	return ret;
}

qboolean BF_ReadBytes( sizebuf_t *bf, void *pOut, int nBytes )
{
	return BF_ReadBits( bf, pOut, nBytes << 3 );
}

char *BF_ReadStringExt( sizebuf_t *bf, qboolean bLine )
{
	static char	string[MAX_SYSPATH];
	int		l = 0, c;
	
	do
	{
		// use BF_ReadByte so -1 is out of bounds
		c = BF_ReadByte( bf );

		if( c == 0 ) break;
		else if( bLine && c == '\n' )
			break;

		// translate all fmt spec to avoid crash bugs
		// NOTE: but game strings leave unchanged. see pfnWriteString for details
		if( c == '%' ) c = '.';

		string[l] = c;
		l++;
	} while( l < sizeof( string ) - 1 );
	string[l] = 0; // terminator

	return string;
}

void BF_ExciseBits( sizebuf_t *bf, int startbit, int bitstoremove )
{
	int	i, endbit = startbit + bitstoremove;
	int	remaining_to_end = bf->nDataBits - endbit;
	sizebuf_t	temp;

	BF_StartWriting( &temp, bf->pData, bf->nDataBits << 3, startbit, -1 );
	BF_SeekToBit( bf, endbit );

	for( i = 0; i < remaining_to_end; i++ )
	{
		BF_WriteOneBit( &temp, BF_ReadOneBit( bf ));
	}

	BF_SeekToBit( bf, startbit );
	bf->nDataBits -= bitstoremove;
}