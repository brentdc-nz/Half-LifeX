//=======================================================================
//			Copyright (C) XashXT Group 2011
//		         stringlib.h - safety string routines 
//=======================================================================
#ifndef STRINGLIB_H
#define STRINGLIB_H

#include <string.h>
//#include "port.h"

//#ifndef _WIN32 //MARTY
#include <stdarg.h>
//#endif

#define MAX_TOKEN_CHARS	1024

extern const char *UTIL_FileExtension( const char *in );
extern void Q_strnupr( const char *in, char *out, size_t size_out );
extern void Q_strnlwr( const char *in, char *out, size_t size_out );
extern bool Q_isdigit( const char *str );
extern int Q_strlen( const char *string );
extern char Q_toupper( const char in );
extern char Q_tolower( const char in );
extern size_t Q_strncat( char *dst, const char *src, size_t size );
extern size_t Q_strncpy( char *dst, const char *src, size_t size );
extern char *copystring( const char *s ); // don't forget release memory after use
#define freestring( a ) (delete [](a))
int Q_atoi( const char *str );
float Q_atof( const char *str );
Vector Q_atov( const char *str );
char *Q_strchr( const char *s, char c );
char *Q_strrchr( const char *s, char c );
int Q_strnicmp( const char *s1, const char *s2, int n );
int Q_strncmp( const char *s1, const char *s2, int n );
char *Q_strstr( const char *string, const char *string2 );
char *Q_stristr( const char *string, const char *string2 );

int Q_vsnprintf( char *buffer, size_t buffersize, const char *format, va_list args );

int Q_snprintf( char *buffer, size_t buffersize, const char *format, ... );
/*int Q_sprintf( char *buffer, const char *format, ... );
char *Q_pretifymem( float value, int digitsafterdecimal );
char *va( const char *format, ... );

#define Q_strupr( in, out ) Q_strnupr( in, out, 99999 )
#define Q_strlwr( int, out ) Q_strnlwr( in, out, 99999 )*/
#define Q_strcat( dst, src ) Q_strncat( dst, src, 99999 )
#define Q_strcpy( dst, src ) Q_strncpy( dst, src, 99999 )
#define Q_stricmp( s1, s2 ) Q_strnicmp( s1, s2, 99999 )
#define Q_strcmp( s1, s2 ) Q_strncmp( s1, s2, 99999 )
#define Q_vsprintf( buffer, format, args ) Q_vsnprintf( buffer, 99999, format, args )/*
#define Q_memprint( val) Q_pretifymem( val, 2 )*/

//
// common.cpp
//
void COM_FileBase( const char *in, char *out );
void COM_ExtractFilePath( const char *path, char *dest );/*
void COM_StripExtension( char *path );
void COM_StripExtension( const char *in, char *out, int destsize );
void COM_DefaultExtension( char *path, const char *extension );
char *COM_ParseFileExt( char *data, char *token, long token_size );
#define COM_ParseFile( data, token ) COM_ParseFileExt( data, token, sizeof( token ))
unsigned int COM_HashKey( const char *string, unsigned int hashSize );

//
// parser.cpp
//
void COM_SkipBracedSection( char **program );
void COM_SkipRestOfLine( char **data );
char *COM_ParseExt( char **data_p, bool allowLineBreaks );
#define COM_Parse( data_p )	COM_ParseExt( data_p, true )
int COM_CompressText( char *data_p );
int COM_TokenWaiting( char *buffer );*/

#endif//STRINGLIB_H