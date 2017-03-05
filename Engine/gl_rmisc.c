/*
gl_rmisc.c - renderer misceallaneous
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
#include "gl_local.h"
#include "mod_local.h"

typedef struct
{
	const char	*texname;
	const char	*detail;
	const char	material;
	int		lMin;
	int		lMax;
} dmaterial_t;

// default rules for apply detail textures.
// maybe move this to external script?
static const dmaterial_t detail_table[] =
{
{ "crt",		"dt_conc",	'C', 0, 0 },	// concrete
{ "rock",		"dt_rock1",	'C', 0, 0 },
{ "conc", 	"dt_conc",	'C', 0, 0 },
{ "wall", 	"dt_brick",	'C', 0, 0 },
{ "crete",	"dt_conc",	'C', 0, 0 },
{ "generic",	"dt_brick",	'C', 0, 0 },
{ "metal",	"dt_metal%i",	'M', 1, 2 },	// metal
{ "mtl",		"dt_metal%i",	'M', 1, 2 },
{ "pipe",		"dt_metal%i",	'M', 1, 2 },
{ "elev",		"dt_metal%i",	'M', 1, 2 },
{ "sign",		"dt_metal%i",	'M', 1, 2 },
{ "barrel",	"dt_metal%i",	'M', 1, 2 },
{ "bath",		"dt_ssteel1",	'M', 1, 2 },
{ "refbridge",	"dt_metal%i",	'M', 1, 2 },
{ "panel",	"dt_ssteel1",	'M', 0, 0 },
{ "brass",	"dt_ssteel1",	'M', 0, 0 },
{ "car",		"dt_metal%i",	'M', 1, 2 },
{ "circuit",	"dt_metal%i",	'M', 1, 2 },
{ "steel",	"dt_ssteel1",	'M', 0, 0 },
{ "reflect",	"dt_ssteel1",	'G', 0, 0 },
{ "dirt",		"dt_ground%i",	'D', 1, 5 },	// dirt
{ "drt",		"dt_ground%i",	'D', 1, 5 },
{ "out",		"dt_ground%i",	'D', 1, 5 },
{ "grass",	"dt_grass1",	'D', 0, 0 },
{ "mud",		"dt_carpet1",	'D', 0, 0 },
{ "vent",		"dt_ssteel1",	'V', 1, 4 },	// vent
{ "duct",		"dt_ssteel1",	'V', 1, 4 },
{ "tile",		"dt_smooth%i",	'T', 1, 2 },
{ "labflr",	"dt_smooth%i",	'T', 1, 2 },
{ "bath",		"dt_smooth%i",	'T', 1, 2 },
{ "grate",	"dt_stone%i",	'G', 1, 4 },	// vent
{ "stone",	"dt_stone%i",	'G', 1, 4 },
{ "grt",		"dt_stone%i",	'G', 1, 4 },
{ "wood",		"dt_wood%i",	'W', 1, 3 },
{ "wd",		"dt_wood%i",	'W', 1, 3 },
{ "table",	"dt_wood%i",	'W', 1, 3 },
{ "board",	"dt_wood%i",	'W', 1, 3 },
{ "chair",	"dt_wood%i",	'W', 1, 3 },
{ "brd",		"dt_wood%i",	'W', 1, 3 },
{ "carp",		"dt_carpet1",	'W', 1, 3 },
{ "book",		"dt_wood%i",	'W', 1, 3 },
{ "box",		"dt_wood%i",	'W', 1, 3 },
{ "cab",		"dt_wood%i",	'W', 1, 3 },
{ "couch",	"dt_wood%i",	'W', 1, 3 },
{ "crate",	"dt_wood%i",	'W', 1, 3 },
{ "poster",	"dt_plaster%i",	'W', 1, 2 },
{ "sheet",	"dt_plaster%i",	'W', 1, 2 },
{ "stucco",	"dt_plaster%i",	'W', 1, 2 },
{ "comp",		"dt_smooth1",	'P', 0, 0 },
{ "cmp",		"dt_smooth1",	'P', 0, 0 },
{ "elec",		"dt_smooth1",	'P', 0, 0 },
{ "vend",		"dt_smooth1",	'P', 0, 0 },
{ "monitor",	"dt_smooth1",	'P', 0, 0 },
{ "phone",	"dt_smooth1",	'P', 0, 0 },
{ "glass",	"dt_ssteel1",	'Y', 0, 0 },
{ "window",	"dt_ssteel1",	'Y', 0, 0 },
{ "flesh",	"dt_rough1",	'F', 0, 0 },
{ "meat",		"dt_rough1",	'F', 0, 0 },
{ "fls",		"dt_rough1",	'F', 0, 0 },
{ "ground",	"dt_ground%i",	'D', 1, 5 },
{ "gnd",		"dt_ground%i",	'D', 1, 5 },
{ "snow",		"dt_snow%i",	'O', 1, 2 },	// snow
{ NULL, NULL, 0, 0, 0 }
};

static const char *R_DetailTextureForName( const char *name )
{
	const dmaterial_t	*table;

	if( !name || !*name ) return NULL;
	if( !Q_strnicmp( name, "sky", 3 ))
		return NULL; // never details for sky

	// never apply details for liquids
	if( !Q_strnicmp( name + 1, "!lava", 5 ))
		return NULL;
	if( !Q_strnicmp( name + 1, "!slime", 6 ))
		return NULL;
	if( !Q_strnicmp( name, "!cur_90", 7 ))
		return NULL;
	if( !Q_strnicmp( name, "!cur_0", 6 ))
		return NULL;
	if( !Q_strnicmp( name, "!cur_270", 8 ))
		return NULL;
	if( !Q_strnicmp( name, "!cur_180", 8 ))
		return NULL;
	if( !Q_strnicmp( name, "!cur_up", 7 ))
		return NULL;
	if( !Q_strnicmp( name, "!cur_dwn", 8 ))
		return NULL;
	if( name[0] == '!' )
		return NULL;

	// never apply details to the special textures
	if( !Q_strnicmp( name, "origin", 6 ))
		return NULL;
	if( !Q_strnicmp( name, "clip", 4 ))
		return NULL;
	if( !Q_strnicmp( name, "hint", 4 ))
		return NULL;
	if( !Q_strnicmp( name, "skip", 4 ))
		return NULL;
	if( !Q_strnicmp( name, "translucent", 11 ))
		return NULL;
	if( !Q_strnicmp( name, "3dsky", 5 ))	// xash-mod support :-)
		return NULL;
	if( !Q_strnicmp( name, "scroll", 6 ))
		return NULL;
	if( name[0] == '@' )
		return NULL;

	// last check ...
	if( !Q_strnicmp( name, "null", 4 ))
		return NULL;

	for( table = detail_table; table && table->texname; table++ )
	{
		if( Q_stristr( name, table->texname ))
		{
			if(( table->lMin + table->lMax ) > 0 )
				return va( table->detail, Com_RandomLong( table->lMin, table->lMax )); 
			return table->detail;
		}
	}

	return NULL;
}

void R_CreateDetailTexturesList( const char *filename )
{
	file_t		*detail_txt = NULL;
	const char	*detail_name, *texname;
	int		i;

	for( i = 0; i < cl.worldmodel->numtextures; i++ )
	{
		texname = cl.worldmodel->textures[i]->name;
		detail_name = R_DetailTextureForName( texname );
		if( !detail_name ) continue;

		// detailtexture detected
		if( detail_name )
		{
			if( !detail_txt ) detail_txt = FS_Open( filename, "w", false ); 
			if( !detail_txt )
			{
				MsgDev( D_ERROR, "Can't write %s\n", filename );
				break;
			}

			// store detailtexture description
			FS_Printf( detail_txt, "%s detail/%s 10.0 10.0\n", texname, detail_name );
		}
	}

	if( detail_txt ) FS_Close( detail_txt );
}

void R_ParseDetailTextures( const char *filename )
{
	char	*afile, *pfile;
	string	token, texname, detail_texname;
	float	xScale, yScale;
	texture_t	*tex;
	int	i;

	if( r_detailtextures->integer >= 2 && !FS_FileExists( filename, false ))
	{
		// use built-in generator for detail textures
		R_CreateDetailTexturesList( filename );
	}

	afile = FS_LoadFile( filename, NULL, false );
	if( !afile ) return;

	pfile = afile;

	// format: 'texturename' 'detailtexture' 'xScale' 'yScale'
	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		texname[0] = '\0';

		// read texname
		if( token[0] == '{' )
		{
			// NOTE: COM_ParseFile handled some symbols seperately
			// this code will be fix it
			pfile = COM_ParseFile( pfile, token );
			Q_strncat( texname, "{", sizeof( texname ));
			Q_strncat( texname, token, sizeof( texname ));
		}
		else Q_strncpy( texname, token, sizeof( texname ));

		// read detailtexture name
		pfile = COM_ParseFile( pfile, token );
		Q_snprintf( detail_texname, sizeof( detail_texname ), "gfx/%s.tga", token );

		// read scales
		pfile = COM_ParseFile( pfile, token );
		xScale = Q_atof( token );		

		pfile = COM_ParseFile( pfile, token );
		yScale = Q_atof( token );

		if( xScale <= 0.0f || yScale <= 0.0f )
			continue;

		// search for existing texture and uploading detail texture
		for( i = 0; i < cl.worldmodel->numtextures; i++ )
		{
			tex = cl.worldmodel->textures[i];

			if( Q_stricmp( tex->name, texname ))
				continue;

			tex->dt_texturenum = GL_LoadTexture( detail_texname, NULL, 0, TF_FORCE_COLOR );

			// texture is loaded
			if( tex->dt_texturenum )
			{
				gltexture_t	*glt;

				GL_SetTextureType( tex->dt_texturenum, TEX_DETAIL );
				glt = R_GetTexture( tex->gl_texturenum );
				glt->xscale = xScale;
				glt->yscale = yScale;
			}
			break;
		}
	}

	Mem_Free( afile );
}

void R_NewMap( void )
{
	texture_t	*tx;
	int	i;

	R_ClearDecals(); // clear all level decals

	GL_BuildLightmaps ();
	R_SetupSky( cl.refdef.movevars->skyName );

	// clear out efrags in case the level hasn't been reloaded
	for( i = 0; i < cl.worldmodel->numleafs; i++ )
		cl.worldmodel->leafs[i+1].efrags = NULL;

	tr.skytexturenum = -1;
	r_viewleaf = r_oldviewleaf = NULL;

	// clearing texture chains
	for( i = 0; i < cl.worldmodel->numtextures; i++ )
	{
		if( !cl.worldmodel->textures[i] )
			continue;

		tx = cl.worldmodel->textures[i];

		if( !Q_strncmp( tx->name, "sky", 3 ) && tx->width == 256 && tx->height == 128)
			tr.skytexturenum = i;

 		tx->texturechain = NULL;
	}

	// upload detailtextures
	if( r_detailtextures->integer )
	{
		string	mapname, filepath;

		Q_strncpy( mapname, cl.worldmodel->name, sizeof( mapname ));
		FS_StripExtension( mapname );
		Q_sprintf( filepath, "%s_detail.txt", mapname );

		R_ParseDetailTextures( filepath );
	}
}