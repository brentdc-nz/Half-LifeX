/*
gl_remap.c - remap model textures
Copyright (C) 2011 Uncle Mike

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
#include "studio.h"

static void CL_PaletteHueReplace( byte *palSrc, int newHue, int start, int end )
{
	float	r, g, b;
	float	maxcol, mincol;
	float	hue, val, sat;
	int	i;

	hue = (float)(newHue * ( 360.0f / 255 ));

	for( i = start; i <= end; i++ )
	{
		r = palSrc[i*3+0];
		g = palSrc[i*3+1];
		b = palSrc[i*3+2];
		
		maxcol = max( max( r, g ), b ) / 255.0f;
		mincol = min( min( r, g ), b ) / 255.0f;
		
		val = maxcol;
		sat = (maxcol - mincol) / maxcol;

		mincol = val * (1.0f - sat);

		if( hue <= 120.0f )
		{
			b = mincol;
			if( hue < 60 )
			{
				r = val;
				g = mincol + hue * (val - mincol) / (120.0f - hue);
			}
			else
			{
				g = val;
				r = mincol + (120.0f - hue) * (val - mincol) / hue;
			}
		}
		else if( hue <= 240.0f )
		{
			r = mincol;
			if( hue < 180.0f )
			{
				g = val;
				b = mincol + (hue - 120.0f) * (val - mincol) / (240.0f - hue);
			}
			else
			{
				b = val;
				g = mincol + (240.0f - hue) * (val - mincol) / (hue - 120.0f);
			}
		}
		else
		{
			g = mincol;
			if( hue < 300.0f )
			{
				b = val;
				r = mincol + (hue - 240.0f) * (val - mincol) / (360.0f - hue);
			}
			else
			{
				r = val;
				b = mincol + (360.0f - hue) * (val - mincol) / (hue - 240.0f);
			}
		}

		palSrc[i*3+0] = (byte)(r * 255);
		palSrc[i*3+1] = (byte)(g * 255);
		palSrc[i*3+2] = (byte)(b * 255);
	}
}

/*
====================
CL_GetRemapInfoForEntity

Returns remapinfo slot for specified entity
====================
*/
remap_info_t *CL_GetRemapInfoForEntity( cl_entity_t *e )
{
	if( !e ) return NULL;

	if( e == &clgame.viewent )
		return clgame.remap_info[clgame.maxEntities];

	return clgame.remap_info[e->curstate.number];
}

/*
====================
CL_CmpStudioTextures

return true if equal
====================
*/
qboolean CL_CmpStudioTextures( int numtexs, mstudiotexture_t *p1, mstudiotexture_t *p2 )
{
	int	i;

	if( !p1 || !p2 ) return false;

	for( i = 0; i < numtexs; i++, p1++, p2++ )
	{
		if( p1->flags & STUDIO_NF_COLORMAP )
			continue;	// colormaps always has different indexes

		if( p1->index != p2->index )
			return false;
	} 
	return true;
}

/*
====================
CL_CreateRawTextureFromPixels

Convert texture_t struct into mstudiotexture_t prototype
====================
*/
byte *CL_CreateRawTextureFromPixels( texture_t *tx, size_t *size, int topcolor, int bottomcolor )
{
	static mstudiotexture_t	pin;
	byte			*pal;

	ASSERT( size != NULL );

	*size = sizeof( pin ) + (tx->width * tx->height) + 768;

	// fill header
	if( !pin.name[0] ) Q_strncpy( pin.name, "#raw_remap_image.mdl", sizeof( pin.name ));
	pin.flags = STUDIO_NF_COLORMAP; // just in case :-)
	pin.index = (int)(tx + 1); // pointer to pixels
	pin.width = tx->width;
	pin.height = tx->height;

	// update palette
	pal = (byte *)(tx + 1) + (tx->width * tx->height);
	CL_PaletteHueReplace( pal, topcolor, tx->anim_min, tx->anim_max );
	CL_PaletteHueReplace( pal, bottomcolor, tx->anim_max + 1, tx->anim_total );

	return (byte *)&pin;
}

/*
====================
CL_DuplicateTexture

Dupliacte texture with remap pixels
====================
*/
void CL_DuplicateTexture( mstudiotexture_t *ptexture, int topcolor, int bottomcolor )
{
	gltexture_t	*glt;
	texture_t		*tx = NULL;
	char		texname[128];
	int		i, size, index;
	byte		paletteBackup[768];
	byte		*raw, *pal;

	// save of the real texture index
	index = ptexture->index;
	glt = R_GetTexture( index );
	Q_snprintf( texname, sizeof( texname ), "#%i_%s", RI.currententity->curstate.number, glt->name + 1 );

	// search for pixels
	for( i = 0; i < RI.currentmodel->numtextures; i++ )
	{
		tx = RI.currentmodel->textures[i];
		if( tx->gl_texturenum == index )
			break; // found
	}

	ASSERT( tx != NULL );

	// backup original palette
	pal = (byte *)(tx + 1) + (tx->width * tx->height);
	memcpy( paletteBackup, pal, 768 );

	raw = CL_CreateRawTextureFromPixels( tx, &size, topcolor, bottomcolor );
	ptexture->index = GL_LoadTexture( texname, raw, size, TF_FORCE_COLOR ); // do copy
	GL_SetTextureType( ptexture->index, TEX_REMAP );

	// restore original palette
	memcpy( pal, paletteBackup, 768 );
}

/*
====================
CL_UpdateTexture

Update texture top and bottom colors
====================
*/
void CL_UpdateTexture( mstudiotexture_t *ptexture, int topcolor, int bottomcolor )
{
	gltexture_t	*glt;
	rgbdata_t		*pic;
	texture_t		*tx = NULL;
	char		texname[128], name[128], mdlname[128];
	int		i, size, index;
	byte		paletteBackup[768];
	byte		*raw, *pal;

	// save of the real texture index
	glt = R_GetTexture( ptexture->index );

	// build name of original texture
	Q_strncpy( mdlname, RI.currentmodel->name, sizeof( mdlname ));
	FS_FileBase( ptexture->name, name );
	FS_StripExtension( mdlname );

	Q_snprintf( texname, sizeof( texname ), "#%s/%s.mdl", mdlname, name );
	index = GL_FindTexture( texname );
	if( !index ) return; // couldn't find texture

	// search for pixels
	for( i = 0; i < RI.currentmodel->numtextures; i++ )
	{
		tx = RI.currentmodel->textures[i];
		if( tx->gl_texturenum == index )
			break; // found
	}

	ASSERT( tx != NULL );

	// backup original palette
	pal = (byte *)(tx + 1) + (tx->width * tx->height);
	memcpy( paletteBackup, pal, 768 );

	raw = CL_CreateRawTextureFromPixels( tx, &size, topcolor, bottomcolor );
	pic = FS_LoadImage( glt->name, raw, size );
	if( !pic )
	{
		MsgDev( D_ERROR, "Couldn't update texture %s\n", glt->name );
		return;
	}

	index = GL_LoadTextureInternal( glt->name, pic, 0, true );
	FS_FreeImage( pic );

	// restore original palette
	memcpy( pal, paletteBackup, 768 );

	ASSERT( index == ptexture->index );
}

/*
====================
CL_AllocRemapInfo

Allocate new remap info per entity
and make copy of remap textures
====================
*/
void CL_AllocRemapInfo( int topcolor, int bottomcolor )
{
	remap_info_t	*info;
	studiohdr_t	*phdr;
	mstudiotexture_t	*src, *dst;
	int		i, size;

	if( !RI.currententity ) return;
	i = ( RI.currententity == &clgame.viewent ) ? clgame.maxEntities : RI.currententity->curstate.number;

	if( !RI.currentmodel || RI.currentmodel->type != mod_studio )
	{
		// entity has changed model by another type, release remap info
		if( clgame.remap_info[i] )
		{
			CL_FreeRemapInfo( clgame.remap_info[i] );
			clgame.remap_info[i] = NULL;
		}
		return; // missed or hided model, ignore it
	}

	// model doesn't contains remap textures
	if( RI.currentmodel->numtextures <= 0 )
	{
		// entity has changed model with no remap textures
		if( clgame.remap_info[i] )
		{
			CL_FreeRemapInfo( clgame.remap_info[i] );
			clgame.remap_info[i] = NULL;
		}
		return;
	}

	phdr = (studiohdr_t *)Mod_Extradata( RI.currentmodel );
	if( !phdr ) return;	// missed header ???

	src = (mstudiotexture_t *)(((byte *)phdr) + phdr->textureindex);
	dst = (clgame.remap_info[i] ? clgame.remap_info[i]->ptexture : NULL); 

	// NOTE: we must copy all the structures 'mstudiotexture_t' for easy acces when model is rendering
	if( !CL_CmpStudioTextures( phdr->numtextures, src, dst ) || clgame.remap_info[i]->model != RI.currentmodel )
	{
		// this code catches studiomodel change with another studiomodel with remap textures
		// e.g. playermodel 'barney' with playermodel 'gordon'
		if( clgame.remap_info[i] ) CL_FreeRemapInfo( clgame.remap_info[i] ); // free old info
		size = sizeof( remap_info_t ) + ( sizeof( mstudiotexture_t ) * phdr->numtextures );
		info = clgame.remap_info[i] = Mem_Alloc( clgame.mempool, size );	
		info->ptexture = (mstudiotexture_t *)(info + 1); // textures are immediately comes after remap_info
	}
	else
	{
		// studiomodel is valid, nothing to change
		return;
	}

	info->numtextures = phdr->numtextures;
	info->model = RI.currentmodel;
	info->topcolor = topcolor;
	info->bottomcolor = bottomcolor;

	src = (mstudiotexture_t *)(((byte *)phdr) + phdr->textureindex);
	dst = info->ptexture;

	// copy unchanged first
	memcpy( dst, src, sizeof( mstudiotexture_t ) * phdr->numtextures );

	// make local copies for remap textures
	for( i = 0; i < info->numtextures; i++ )
	{
		if( dst[i].flags & STUDIO_NF_COLORMAP )
			CL_DuplicateTexture( &dst[i], topcolor, bottomcolor );
	}
}

/*
====================
CL_UpdateRemapInfo

Update all remaps per entity
====================
*/
void CL_UpdateRemapInfo( int topcolor, int bottomcolor )
{
	remap_info_t	*info;
	mstudiotexture_t	*dst;
	int		i;

	i = ( RI.currententity == &clgame.viewent ) ? clgame.maxEntities : RI.currententity->curstate.number;
	info = clgame.remap_info[i];
	if( !info ) return; // no remap info

	if( info->topcolor == topcolor && info->bottomcolor == bottomcolor )
		return; // values is valid

	dst = info->ptexture;

	for( i = 0; i < info->numtextures; i++ )
	{
		if( dst[i].flags & STUDIO_NF_COLORMAP )
			CL_UpdateTexture( &dst[i], topcolor, bottomcolor );
	}

	info->topcolor = topcolor;
	info->bottomcolor = bottomcolor;
}

/*
====================
CL_FreeRemapInfo

Release remap info per entity
====================
*/
void CL_FreeRemapInfo( remap_info_t *info )
{
	int	i;

	ASSERT( info != NULL );

	// release all colormap texture copies
	for( i = 0; i < info->numtextures; i++ )
	{
		if( info->ptexture[i].flags & STUDIO_NF_COLORMAP )
			GL_FreeTexture( info->ptexture[i].index );
	}

	Mem_Free( info ); // release struct	
}

/*
====================
CL_ClearAllRemaps

Release all remap infos
====================
*/
void CL_ClearAllRemaps( void )
{
	int	i;

	if( clgame.remap_info )
	{
		for( i = 0; i < clgame.maxRemapInfos; i++ )
		{
			if( clgame.remap_info[i] )
				CL_FreeRemapInfo( clgame.remap_info[i] ); 
		}
		Mem_Free( clgame.remap_info );
	}
	clgame.remap_info = NULL;
}