/*
con_utils.c - console helpers
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
#include "kbutton.h"

extern convar_t	*con_gamemaps;

typedef struct autocomplete_list_s
{
	const char *name;
	qboolean (*func)( const char *s, char *name, int length );
} autocomplete_list_t;

/*
=======================================================================

			FILENAME AUTOCOMPLETION
=======================================================================
*/
/*
=====================================
Cmd_GetMapList

Prints or complete map filename
=====================================
*/
qboolean Cmd_GetMapList( const char *s, char *completedname, int length )
{
	search_t		*t;
	file_t		*f;
	string		message;
	string		compiler;
	string		generator;
	string		matchbuf;
	byte		buf[MAX_SYSPATH]; // 1 kb
	int		i, nummaps;

	t = FS_Search( va( "maps/%s*.bsp", s ), true, con_gamemaps->value );
	if( !t ) return false;

	COM_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length ) 
		Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for( i = 0, nummaps = 0; i < t->numfilenames; i++ )
	{
		char		entfilename[MAX_QPATH];
		const char	*ext = COM_FileExtension( t->filenames[i] ); 
		int		ver = -1, lumpofs = 0, lumplen = 0;
		char		*ents = NULL, *pfile;
		qboolean		validmap = false;
		int		version = 0;

		if( Q_stricmp( ext, "bsp" )) continue;
		Q_strncpy( message, "^1error^7", sizeof( message ));
		Q_strncpy( compiler, "", sizeof( compiler ));
		Q_strncpy( generator, "", sizeof( generator ));
		f = FS_Open( t->filenames[i], "rb", con_gamemaps->value );
	
		if( f )
		{
			dheader_t		*header;
			dextrahdr_t	*hdrext;

			memset( buf, 0, sizeof( buf ));
			FS_Read( f, buf, sizeof( buf ));
			header = (dheader_t *)buf;
			ver = header->version;

			// check all the lumps and some other errors
			if( Mod_TestBmodelLumps( t->filenames[i], buf, true ))
			{
				lumpofs = header->lumps[LUMP_ENTITIES].fileofs;
				lumplen = header->lumps[LUMP_ENTITIES].filelen;
				ver = header->version;
			}

			hdrext = (dextrahdr_t *)((byte *)buf + sizeof( dheader_t ));
			if( hdrext->id == IDEXTRAHEADER ) version = hdrext->version;

			Q_strncpy( entfilename, t->filenames[i], sizeof( entfilename ));
			COM_StripExtension( entfilename );
			COM_DefaultExtension( entfilename, ".ent" );
			ents = FS_LoadFile( entfilename, NULL, true );

			if( !ents && lumplen >= 10 )
			{
				FS_Seek( f, lumpofs, SEEK_SET );
				ents = (char *)Mem_Calloc( host.mempool, lumplen + 1 );
				FS_Read( f, ents, lumplen );
			}

			if( ents )
			{
				// if there are entities to parse, a missing message key just
				// means there is no title, so clear the message string now
				char	token[2048];

				message[0] = 0; // remove 'error'
				pfile = ents;

				while(( pfile = COM_ParseFile( pfile, token )) != NULL )
				{
					if( !Q_strcmp( token, "{" )) continue;
					else if( !Q_strcmp( token, "}" )) break;
					else if( !Q_strcmp( token, "message" ))
					{
						// get the message contents
						pfile = COM_ParseFile( pfile, message );
					}
					else if( !Q_strcmp( token, "compiler" ) || !Q_strcmp( token, "_compiler" ))
					{
						// get the message contents
						pfile = COM_ParseFile( pfile, compiler );
					}
					else if( !Q_strcmp( token, "generator" ) || !Q_strcmp( token, "_generator" ))
					{
						// get the message contents
						pfile = COM_ParseFile( pfile, generator );
					}
				}
				Mem_Free( ents );
			}
		}

		if( f ) FS_Close(f);
		COM_FileBase( t->filenames[i], matchbuf );

		switch( ver )
		{
		case Q1BSP_VERSION:
			Q_strncpy( buf, "Quake", sizeof( buf ));
			break;
		case QBSP2_VERSION:
			Q_strncpy( buf, "Darkplaces BSP2", sizeof( buf ));
			break;
		case HLBSP_VERSION:
			switch( version )
			{
			case 1: Q_strncpy( buf, "XashXT old format", sizeof( buf )); break;
			case 2: Q_strncpy( buf, "Paranoia 2: Savior", sizeof( buf )); break;
			case 4: Q_strncpy( buf, "Half-Life extended", sizeof( buf )); break;
			default: Q_strncpy( buf, "Half-Life", sizeof( buf )); break;
			}
			break;
		default:	Q_strncpy( buf, "??", sizeof( buf )); break;
		}

		Con_Printf( "%16s (%s) ^3%s^7 ^2%s %s^7\n", matchbuf, buf, message, compiler, generator );
		nummaps++;
	}

	Con_Printf( "\n^3 %i maps found.\n", nummaps );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	for( i = 0; matchbuf[i]; i++ )
	{
		if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
			completedname[i] = 0;
	}
	return true;
}

/*
=====================================
Cmd_GetDemoList

Prints or complete demo filename
=====================================
*/
qboolean Cmd_GetDemoList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numdems;

	// lookup only in gamedir
	t = FS_Search( va( "%s*.dem", s ), true, true );
	if( !t ) return false;

	COM_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length )
		Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for( i = 0, numdems = 0; i < t->numfilenames; i++ )
	{
		if( Q_stricmp( COM_FileExtension( t->filenames[i] ), "dem" ))
			continue;

		COM_FileBase( t->filenames[i], matchbuf );
		Con_Printf( "%16s\n", matchbuf );
		numdems++;
	}

	Con_Printf( "\n^3 %i demos found.\n", numdems );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

/*
=====================================
Cmd_GetMovieList

Prints or complete movie filename
=====================================
*/
qboolean Cmd_GetMovieList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, nummovies;

	t = FS_Search( va( "media/%s*.avi", s ), true, false );
	if( !t ) return false;

	COM_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length )
		Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for(i = 0, nummovies = 0; i < t->numfilenames; i++)
	{
		if( Q_stricmp( COM_FileExtension( t->filenames[i] ), "avi" ))
			continue;

		COM_FileBase( t->filenames[i], matchbuf );
		Con_Printf( "%16s\n", matchbuf );
		nummovies++;
	}

	Con_Printf( "\n^3 %i movies found.\n", nummovies );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}

	return true;
}

/*
=====================================
Cmd_GetMusicList

Prints or complete background track filename
=====================================
*/
qboolean Cmd_GetMusicList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numtracks;

	t = FS_Search( va( "media/%s*.*", s ), true, false );
	if( !t ) return false;

	COM_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length )
		Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for(i = 0, numtracks = 0; i < t->numfilenames; i++)
	{
		const char *ext = COM_FileExtension( t->filenames[i] ); 

		if( Q_stricmp( ext, "wav" ) && Q_stricmp( ext, "mp3" ))
			continue;

		COM_FileBase( t->filenames[i], matchbuf );
		Con_Printf( "%16s\n", matchbuf );
		numtracks++;
	}

	Con_Printf( "\n^3 %i soundtracks found.\n", numtracks );
	Mem_Free(t);

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

/*
=====================================
Cmd_GetSavesList

Prints or complete savegame filename
=====================================
*/
qboolean Cmd_GetSavesList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numsaves;

	t = FS_Search( va( "%s%s*.sav", DEFAULT_SAVE_DIRECTORY, s ), true, true );	// lookup only in gamedir
	if( !t ) return false;

	COM_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length )
		Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for( i = 0, numsaves = 0; i < t->numfilenames; i++ )
	{
		if( Q_stricmp( COM_FileExtension( t->filenames[i] ), "sav" ))
			continue;

		COM_FileBase( t->filenames[i], matchbuf );
		Con_Printf( "%16s\n", matchbuf );
		numsaves++;
	}

	Con_Printf( "\n^3 %i saves found.\n", numsaves );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}

	return true;
}

/*
=====================================
Cmd_GetConfigList

Prints or complete .cfg filename
=====================================
*/
qboolean Cmd_GetConfigList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numconfigs;

	t = FS_Search( va( "%s*.cfg", s ), true, false );
	if( !t ) return false;

	COM_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length )
		Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for( i = 0, numconfigs = 0; i < t->numfilenames; i++ )
	{
		if( Q_stricmp( COM_FileExtension( t->filenames[i] ), "cfg" ))
			continue;

		COM_FileBase( t->filenames[i], matchbuf );
		Con_Printf( "%16s\n", matchbuf );
		numconfigs++;
	}

	Con_Printf( "\n^3 %i configs found.\n", numconfigs );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}

	return true;
}

/*
=====================================
Cmd_GetSoundList

Prints or complete sound filename
=====================================
*/
qboolean Cmd_GetSoundList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numsounds;

	t = FS_Search( va( "%s%s*.*", DEFAULT_SOUNDPATH, s ), true, false );
	if( !t ) return false;

	Q_strncpy( matchbuf, t->filenames[0] + Q_strlen( DEFAULT_SOUNDPATH ), MAX_STRING ); 
	COM_StripExtension( matchbuf ); 
	if( completedname && length )
		Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for(i = 0, numsounds = 0; i < t->numfilenames; i++)
	{
		const char *ext = COM_FileExtension( t->filenames[i] ); 

		if( Q_stricmp( ext, "wav" ) && Q_stricmp( ext, "mp3" ))
			continue;

		Q_strncpy( matchbuf, t->filenames[i] + Q_strlen( DEFAULT_SOUNDPATH ), MAX_STRING ); 
		COM_StripExtension( matchbuf );
		Con_Printf( "%16s\n", matchbuf );
		numsounds++;
	}

	Con_Printf( "\n^3 %i sounds found.\n", numsounds );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}

	return true;
}

/*
=====================================
Cmd_GetItemsList

Prints or complete item classname (weapons only)
=====================================
*/
qboolean Cmd_GetItemsList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numitems;

	if( !clgame.itemspath[0] ) return false; // not in game yet
	t = FS_Search( va( "%s/%s*.txt", clgame.itemspath, s ), true, false );
	if( !t ) return false;

	COM_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length )
		Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for(i = 0, numitems = 0; i < t->numfilenames; i++)
	{
		if( Q_stricmp( COM_FileExtension( t->filenames[i] ), "txt" ))
			continue;

		COM_FileBase( t->filenames[i], matchbuf );
		Con_Printf( "%16s\n", matchbuf );
		numitems++;
	}

	Con_Printf( "\n^3 %i items found.\n", numitems );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

/*
=====================================
Cmd_GetCustomList

Prints or complete .HPK filenames
=====================================
*/
qboolean Cmd_GetCustomList( const char *s, char *completedname, int length )
{
	search_t		*t;
	string		matchbuf;
	int		i, numitems;

	t = FS_Search( va( "%s*.hpk", s ), true, false );
	if( !t ) return false;

	COM_FileBase( t->filenames[0], matchbuf ); 
	if( completedname && length )
		Q_strncpy( completedname, matchbuf, length );
	if( t->numfilenames == 1 ) return true;

	for(i = 0, numitems = 0; i < t->numfilenames; i++)
	{
		if( Q_stricmp( COM_FileExtension( t->filenames[i] ), "hpk" ))
			continue;

		COM_FileBase( t->filenames[i], matchbuf );
		Con_Printf( "%16s\n", matchbuf );
		numitems++;
	}

	Con_Printf( "\n^3 %i items found.\n", numitems );
	Mem_Free( t );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

/*
=====================================
Cmd_GetGameList

Prints or complete gamedir name
=====================================
*/
qboolean Cmd_GetGamesList( const char *s, char *completedname, int length )
{
	int	i, numgamedirs;
	string	gamedirs[MAX_MODS];
	string	matchbuf;

	// stand-alone games doesn't have cmd "game"
	if( !Cmd_Exists( "game" ))
		return false;

	// compare gamelist with current keyword
	for( i = 0, numgamedirs = 0; i < SI.numgames; i++ )
	{
		if(( *s == '*' ) || !Q_strnicmp( SI.games[i]->gamefolder, s, Q_strlen( s )))
			Q_strcpy( gamedirs[numgamedirs++], SI.games[i]->gamefolder ); 
	}

	if( !numgamedirs ) return false;
	Q_strncpy( matchbuf, gamedirs[0], MAX_STRING ); 
	if( completedname && length )
		Q_strncpy( completedname, matchbuf, length );
	if( numgamedirs == 1 ) return true;

	for( i = 0; i < numgamedirs; i++ )
	{
		Q_strncpy( matchbuf, gamedirs[i], MAX_STRING ); 
		Con_Printf( "%16s\n", matchbuf );
	}

	Con_Printf( "\n^3 %i games found.\n", numgamedirs );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

/*
=====================================
Cmd_GetCDList

Prints or complete CD command name
=====================================
*/
qboolean Cmd_GetCDList( const char *s, char *completedname, int length )
{
	int i, numcdcommands;
	string	cdcommands[8];
	string	matchbuf;

	const char *cd_command[] =
	{
	"info",
	"loop",
	"off",
	"on",
	"pause",
	"play",
	"resume",
	"stop",
	};

	// compare CD command list with current keyword
	for( i = 0, numcdcommands = 0; i < 8; i++ )
	{
		if(( *s == '*' ) || !Q_strnicmp( cd_command[i], s, Q_strlen( s )))
			Q_strcpy( cdcommands[numcdcommands++], cd_command[i] );
	}

	if( !numcdcommands ) return false;
	Q_strncpy( matchbuf, cdcommands[0], MAX_STRING );
	if( completedname && length )
		Q_strncpy( completedname, matchbuf, length );
	if( numcdcommands == 1 ) return true;

	for( i = 0; i < numcdcommands; i++ )
	{
		Q_strncpy( matchbuf, cdcommands[i], MAX_STRING );
		Con_Printf( "%16s\n", matchbuf );
	}

	Con_Printf( "\n^3 %i commands found.\n", numcdcommands );

	// cut shortestMatch to the amount common with s
	if( completedname && length )
	{
		for( i = 0; matchbuf[i]; i++ )
		{
			if( Q_tolower( completedname[i] ) != Q_tolower( matchbuf[i] ))
				completedname[i] = 0;
		}
	}
	return true;
}

qboolean Cmd_CheckMapsList_R( qboolean fRefresh, qboolean onlyingamedir )
{
	qboolean	use_filter = false;
	byte	buf[MAX_SYSPATH];
	string	mpfilter;
	char	*buffer;
	string	result;
	int	i, size;
	search_t	*t;
	file_t	*f;

	if( FS_FileSize( "maps.lst", onlyingamedir ) > 0 && !fRefresh )
		return true; // exist 

	// setup mpfilter
	Q_snprintf( mpfilter, sizeof( mpfilter ), "maps/%s", GI->mp_filter );
	t = FS_Search( "maps/*.bsp", false, onlyingamedir );

	if( !t )
	{
		if( onlyingamedir )
		{
			// mod doesn't contain any maps (probably this is a bot)
			return Cmd_CheckMapsList_R( fRefresh, false );
		}
		return false;
	}

	buffer = Mem_Calloc( host.mempool, t->numfilenames * 2 * sizeof( result ));
	use_filter = Q_strlen( GI->mp_filter ) ? true : false;

	for( i = 0; i < t->numfilenames; i++ )
	{
		char		*ents = NULL, *pfile;
		int		lumpofs = 0, lumplen = 0;
		string		mapname, message, entfilename;

		if( Q_stricmp( COM_FileExtension( t->filenames[i] ), "bsp" ))
			continue;

		if( use_filter && !Q_strnicmp( t->filenames[i], mpfilter, Q_strlen( mpfilter )))
			continue;

		f = FS_Open( t->filenames[i], "rb", onlyingamedir );
		COM_FileBase( t->filenames[i], mapname );

		if( f )
		{
			int	num_spawnpoints = 0;
			dheader_t	*header;

			memset( buf, 0, MAX_SYSPATH );
			FS_Read( f, buf, MAX_SYSPATH );
			header = (dheader_t *)buf;

			// check all the lumps and some other errors
			if( !Mod_TestBmodelLumps( t->filenames[i], buf, true ))
			{
				FS_Close( f );
				continue;
			}

			// after call Mod_TestBmodelLumps we gurantee what map is valid                              
			lumpofs = header->lumps[LUMP_ENTITIES].fileofs;
			lumplen = header->lumps[LUMP_ENTITIES].filelen;

			Q_strncpy( entfilename, t->filenames[i], sizeof( entfilename ));
			COM_StripExtension( entfilename );
			COM_DefaultExtension( entfilename, ".ent" );
			ents = FS_LoadFile( entfilename, NULL, true );

			if( !ents && lumplen >= 10 )
			{
				FS_Seek( f, lumpofs, SEEK_SET );
				ents = Z_Calloc( lumplen + 1 );
				FS_Read( f, ents, lumplen );
			}

			if( ents )
			{
				// if there are entities to parse, a missing message key just
				// means there is no title, so clear the message string now
				char	token[MAX_TOKEN];
				qboolean	worldspawn = true;

				Q_strncpy( message, "No Title", MAX_STRING );
				pfile = ents;

				while(( pfile = COM_ParseFile( pfile, token )) != NULL )
				{
					if( token[0] == '}' && worldspawn )
						worldspawn = false;
					else if( !Q_strcmp( token, "message" ) && worldspawn )
					{
						// get the message contents
						pfile = COM_ParseFile( pfile, message );
					}
					else if( !Q_strcmp( token, "classname" ))
					{
						pfile = COM_ParseFile( pfile, token );
						if( !Q_strcmp( token, GI->mp_entity ) || use_filter )
							num_spawnpoints++;
					}
					if( num_spawnpoints ) break; // valid map
				}
				Mem_Free( ents );
			}

			if( f ) FS_Close( f );

			if( num_spawnpoints )
			{
				// format: mapname "maptitle"\n
				Q_sprintf( result, "%s \"%s\"\n", mapname, message );
				Q_strcat( buffer, result ); // add new string
			}
		}
	}

	if( t ) Mem_Free( t ); // free search result
	size = Q_strlen( buffer );

	if( !size )
	{
          	if( buffer ) Mem_Free( buffer );

		if( onlyingamedir )
			return Cmd_CheckMapsList_R( fRefresh, false );
		return false;
	}

	// write generated maps.lst
	if( FS_WriteFile( "maps.lst", buffer, Q_strlen( buffer )))
	{
          	if( buffer ) Mem_Free( buffer );
		return true;
	}
	return false;
}

qboolean Cmd_CheckMapsList( qboolean fRefresh )
{
	return Cmd_CheckMapsList_R( fRefresh, true );
}

autocomplete_list_t cmd_list[] =
{
{ "map_background", Cmd_GetMapList },
{ "changelevel2", Cmd_GetMapList },
{ "changelevel", Cmd_GetMapList },
{ "playdemo", Cmd_GetDemoList, },
{ "timedemo", Cmd_GetDemoList, },
{ "playvol", Cmd_GetSoundList },
{ "hpkval", Cmd_GetCustomList },
{ "entpatch", Cmd_GetMapList },
{ "music", Cmd_GetMusicList, },
{ "movie", Cmd_GetMovieList },
{ "exec", Cmd_GetConfigList },
{ "give", Cmd_GetItemsList },
{ "drop", Cmd_GetItemsList },
{ "game", Cmd_GetGamesList },
{ "save", Cmd_GetSavesList },
{ "load", Cmd_GetSavesList },
{ "play", Cmd_GetSoundList },
{ "map", Cmd_GetMapList },
{ "cd", Cmd_GetCDList },
{ NULL }, // termiantor
};

/*
===============
Cmd_CheckName

compare first argument with string
===============
*/
static qboolean Cmd_CheckName( const char *name )
{
	if( !Q_stricmp( Cmd_Argv( 0 ), name ))
		return true;
	if( !Q_stricmp( Cmd_Argv( 0 ), va( "\\%s", name )))
		return true;
	return false;
}

/*
============
Cmd_AutocompleteName

Autocomplete filename
for various cmds
============
*/
qboolean Cmd_AutocompleteName( const char *source, char *buffer, size_t bufsize )
{
	autocomplete_list_t	*list;

	for( list = cmd_list; list->name; list++ )
	{
		if( Cmd_CheckName( list->name ))
			return list->func( source, buffer, bufsize ); 
	}

	return false;
}

/*
============
Cmd_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
static void Cmd_WriteOpenGLCvar( const char *name, const char *string, const char *desc, void *f )
{
	if( !COM_CheckString( desc ))
		return; // ignore cvars without description (fantom variables)
	FS_Printf( f, "%s \"%s\"\n", name, string );
}

static void Cmd_WriteHelp(const char *name, const char *unused, const char *desc, void *f )
{
	int	length;

	if( !desc || !Q_strcmp( desc, "" ))
		return; // ignore fantom cmds
	if( name[0] == '+' || name[0] == '-' )
		return; // key bindings	

	length = 3 - (Q_strlen( name ) / 10); // Asm_Ed default tab stop is 10

	if( length == 3 ) FS_Printf( f, "%s\t\t\t\"%s\"\n", name, desc );
	if( length == 2 ) FS_Printf( f, "%s\t\t\"%s\"\n", name, desc );
	if( length == 1 ) FS_Printf( f, "%s\t\"%s\"\n", name, desc );
	if( length == 0 ) FS_Printf( f, "%s \"%s\"\n", name, desc );
}

void Cmd_WriteOpenGLVariables( file_t *f )
{
	Cvar_LookupVars( FCVAR_GLCONFIG, NULL, f, Cmd_WriteOpenGLCvar ); 
}

/*
===============
Host_WriteConfig

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfig( void )
{
	kbutton_t	*mlook = NULL;
	kbutton_t	*jlook = NULL;
	file_t	*f;

	if( !clgame.hInstance ) return;

	f = FS_Open( "config.cfg", "w", false );
	if( f )
	{
		Con_Reportf( "Host_WriteConfig()\n" );
		FS_Printf( f, "//=======================================================================\n");
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s �\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\t\tconfig.cfg - archive of cvars\n" );
		FS_Printf( f, "//=======================================================================\n" );
		Key_WriteBindings( f );
		Cvar_WriteVariables( f, FCVAR_ARCHIVE );
		Info_WriteVars( f );

		if( clgame.hInstance )
		{
			mlook = (kbutton_t *)clgame.dllFuncs.KB_Find( "in_mlook" );
			jlook = (kbutton_t *)clgame.dllFuncs.KB_Find( "in_jlook" );
		}

		if( mlook && ( mlook->state & 1 )) 
			FS_Printf( f, "+mlook\n" );

		if( jlook && ( jlook->state & 1 ))
			FS_Printf( f, "+jlook\n" );

		FS_Printf( f, "exec userconfig.cfg" );

		FS_Close( f );
	}
	else Con_DPrintf( S_ERROR "Couldn't write config.cfg.\n" );
}

/*
===============
Host_WriteServerConfig

save serverinfo variables into server.cfg (using for dedicated server too)
===============
*/
void Host_WriteServerConfig( const char *name )
{
	file_t	*f;

	SV_InitGameProgs();	// collect user variables

	// FIXME: move this out until menu parser is done
	CSCR_LoadDefaultCVars( "settings.scr" );
	
	if(( f = FS_Open( name, "w", false )) != NULL )
	{
		FS_Printf( f, "//=======================================================================\n" );
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s �\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\tgame.cfg - multiplayer server temporare config\n" );
		FS_Printf( f, "//=======================================================================\n" );
		Cvar_WriteVariables( f, FCVAR_SERVER );
		CSCR_WriteGameCVars( f, "settings.scr" );
		FS_Close( f );
	}
	else Con_DPrintf( S_ERROR "Couldn't write %s.\n", name );

	SV_FreeGameProgs();	// release progs with all variables
}

/*
===============
Host_WriteOpenGLConfig

save opengl variables into opengl.cfg
===============
*/
void Host_WriteOpenGLConfig( void )
{
	file_t	*f;

	f = FS_Open( "opengl.cfg", "w", false );
	if( f )
	{
		Con_Reportf( "Host_WriteGLConfig()\n" );
		FS_Printf( f, "//=======================================================================\n" );
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s �\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\t    opengl.cfg - archive of opengl extension cvars\n");
		FS_Printf( f, "//=======================================================================\n" );
		FS_Printf( f, "\n" );
		Cmd_WriteOpenGLVariables( f );
		FS_Close( f );	
	}                                                
	else Con_DPrintf( S_ERROR "can't update opengl.cfg.\n" );
}

/*
===============
Host_WriteVideoConfig

save render variables into video.cfg
===============
*/
void Host_WriteVideoConfig( void )
{
	file_t	*f;

	f = FS_Open( "video.cfg", "w", false );
	if( f )
	{
		Con_Reportf( "Host_WriteVideoConfig()\n" );
		FS_Printf( f, "//=======================================================================\n" );
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s �\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\tvideo.cfg - archive of renderer variables\n");
		FS_Printf( f, "//=======================================================================\n" );
		Cvar_WriteVariables( f, FCVAR_RENDERINFO );
		FS_Close( f );	
	}                                                
	else Con_DPrintf( S_ERROR "can't update video.cfg.\n" );
}

void Key_EnumCmds_f( void )
{
	file_t	*f;

	FS_AllowDirectPaths( true );
	if( FS_FileExists( "../help.txt", false ))
	{
		Con_Printf( "help.txt already exist\n" );
		FS_AllowDirectPaths( false );
		return;
	}

	f = FS_Open( "../help.txt", "w", false );
	if( f )
	{
		FS_Printf( f, "//=======================================================================\n");
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s �\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\thelp.txt - xash commands and console variables\n");
		FS_Printf( f, "//=======================================================================\n");

		FS_Printf( f, "\n\n\t\t\tconsole variables\n\n");
		Cvar_LookupVars( 0, NULL, f, Cmd_WriteHelp ); 
		FS_Printf( f, "\n\n\t\t\tconsole commands\n\n");
		Cmd_LookupCmds( NULL, f, Cmd_WriteHelp ); 
  		FS_Printf( f, "\n\n");
		FS_Close( f );
		Con_Printf( "help.txt created\n" );
	}
	else Con_Printf( S_ERROR "couldn't write help.txt.\n");
	FS_AllowDirectPaths( false );
}