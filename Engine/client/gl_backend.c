/*
gl_backend.c - rendering backend
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
#include "mathlib.h"

char		r_speeds_msg[MAX_SYSPATH];
ref_speeds_t	r_stats;	// r_speeds counters

/*
===============
R_SpeedsMessage
===============
*/
qboolean R_SpeedsMessage( char *out, size_t size )
{
	if( clgame.drawFuncs.R_SpeedsMessage != NULL )
	{
		if( clgame.drawFuncs.R_SpeedsMessage( out, size ))
			return true;
		// otherwise pass to default handler
	}

	if( r_speeds->value <= 0 ) return false;
	if( !out || !size ) return false;

	Q_strncpy( out, r_speeds_msg, size );

	return true;
}

/*
==============
R_Speeds_Printf

helper to print into r_speeds message
==============
*/
void R_Speeds_Printf( const char *msg, ... )
{
	va_list	argptr;
	char	text[2048];

	va_start( argptr, msg );
	Q_vsprintf( text, msg, argptr );
	va_end( argptr );

	Q_strncat( r_speeds_msg, text, sizeof( r_speeds_msg ));
}

/*
==============
GL_BackendStartFrame
==============
*/
void GL_BackendStartFrame( void )
{
	r_speeds_msg[0] = '\0';
}

/*
==============
GL_BackendEndFrame
==============
*/
void GL_BackendEndFrame( void )
{
	mleaf_t	*curleaf;

	if( r_speeds->value <= 0 || !RI.drawWorld )
		return;

	if( !RI.viewleaf )
		curleaf = cl.worldmodel->leafs;
	else curleaf = RI.viewleaf;

	R_Speeds_Printf( "Renderer: ^1Engine^7\n\n" );

	switch( (int)r_speeds->value )
	{
	case 1:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i wpoly, %3i apoly\n%3i epoly, %3i spoly",
		r_stats.c_world_polys, r_stats.c_alias_polys, r_stats.c_studio_polys, r_stats.c_sprite_polys );
		break;		
	case 2:
		R_Speeds_Printf( "visible leafs:\n%3i leafs\ncurrent leaf %3i\n", r_stats.c_world_leafs, curleaf - cl.worldmodel->leafs );
		R_Speeds_Printf( "ReciusiveWorldNode: %3lf secs\nDrawTextureChains %lf\n", r_stats.t_world_node, r_stats.t_world_draw );
		break;
	case 3:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i alias models drawn\n%3i studio models drawn\n%3i sprites drawn",
		r_stats.c_alias_models_drawn, r_stats.c_studio_models_drawn, r_stats.c_sprite_models_drawn );
		break;
	case 4:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i static entities\n%3i normal entities\n%3i server entities",
		r_numStatics, r_numEntities - r_numStatics, pfnNumberOfEntities( ));
		break;
	case 5:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i tempents\n%3i viewbeams\n%3i particles",
		r_stats.c_active_tents_count, r_stats.c_view_beams_count, r_stats.c_particle_count );
		break;
	}

	memset( &r_stats, 0, sizeof( r_stats ));
}

/*
=================
GL_LoadTexMatrix
=================
*/
void GL_LoadTexMatrix( const matrix4x4 m )
{
	/*p*/glMatrixMode( GL_TEXTURE );
	GL_LoadMatrix( m );
	glState.texIdentityMatrix[glState.activeTMU] = false;
}

/*
=================
GL_LoadTexMatrixExt
=================
*/
void GL_LoadTexMatrixExt( const float *glmatrix )
{
	Assert( glmatrix != NULL );
	/*p*/glMatrixMode( GL_TEXTURE );
	/*p*/glLoadMatrixf( glmatrix );
	glState.texIdentityMatrix[glState.activeTMU] = false;
}

/*
=================
GL_LoadMatrix
=================
*/
void GL_LoadMatrix( const matrix4x4 source )
{
	GLfloat	dest[16];

	Matrix4x4_ToArrayFloatGL( source, dest );
	/*p*/glLoadMatrixf( dest );
}

/*
=================
GL_LoadIdentityTexMatrix
=================
*/
void GL_LoadIdentityTexMatrix( void )
{
	if( glState.texIdentityMatrix[glState.activeTMU] )
		return;

	/*p*/glMatrixMode( GL_TEXTURE );
	/*p*/glLoadIdentity();
	glState.texIdentityMatrix[glState.activeTMU] = true;
}

/*
=================
GL_SelectTexture
=================
*/
void GL_SelectTexture( GLint tmu )
{
	if( !GL_Support( GL_ARB_MULTITEXTURE ))
		return;

	// don't allow negative texture units
	if( tmu < 0 ) return;

	if( tmu >= GL_MaxTextureUnits( ))
	{
		Con_Reportf( S_ERROR "GL_SelectTexture: bad tmu state %i\n", tmu );
		return; 
	}

	if( glState.activeTMU == tmu )
		return;

	glState.activeTMU = tmu;
#ifndef _XBOX // TODO
	if( pglActiveTextureARB )
	{
		pglActiveTextureARB( tmu + GL_TEXTURE0_ARB );

		if( tmu < glConfig.max_texture_coords )
			pglClientActiveTextureARB( tmu + GL_TEXTURE0_ARB );
	}
#endif
}

/*
==============
GL_DisableAllTexGens
==============
*/
void GL_DisableAllTexGens( void )
{
#ifndef _XBOX
	GL_TexGen( GL_S, 0 );
	GL_TexGen( GL_T, 0 );
	GL_TexGen( GL_R, 0 );
	GL_TexGen( GL_Q, 0 );
#endif
}

/*
==============
GL_CleanUpTextureUnits
==============
*/
void GL_CleanUpTextureUnits( int last )
{
	int	i;

	for( i = glState.activeTMU; i > (last - 1); i-- )
	{
		// disable upper units
		if( glState.currentTextureTargets[i] != GL_NONE )
		{
			/*p*/glDisable( glState.currentTextureTargets[i] );
			glState.currentTextureTargets[i] = GL_NONE;
			glState.currentTextures[i] = -1; // unbind texture
		}

		GL_SetTexCoordArrayMode( GL_NONE );
		GL_LoadIdentityTexMatrix();
		GL_DisableAllTexGens();
		GL_SelectTexture( i - 1 );
	}
}

/*
==============
GL_CleanupAllTextureUnits
==============
*/
void GL_CleanupAllTextureUnits( void )
{
	if( !glw_state.initialized ) return;
	// force to cleanup all the units
	GL_SelectTexture( GL_MaxTextureUnits() - 1 );
	GL_CleanUpTextureUnits( 0 );
}

/*
=================
GL_MultiTexCoord2f
=================
*/
void GL_MultiTexCoord2f( GLenum texture, GLfloat s, GLfloat t )
{
	if( !GL_Support( GL_ARB_MULTITEXTURE ))
		return;

	if( pglMultiTexCoord2f != NULL )
		pglMultiTexCoord2f( texture + GL_TEXTURE0_ARB, s, t );
}

/*
=================
GL_TextureTarget
=================
*/
void GL_TextureTarget( uint target )
{
	if( glState.activeTMU < 0 || glState.activeTMU >= GL_MaxTextureUnits( ))
	{
		Con_Reportf( S_ERROR "GL_TextureTarget: bad tmu state %i\n", glState.activeTMU );
		return; 
	}

	if( glState.currentTextureTargets[glState.activeTMU] != target )
	{
		if( glState.currentTextureTargets[glState.activeTMU] != GL_NONE )
			/*p*/glDisable( glState.currentTextureTargets[glState.activeTMU] );
		glState.currentTextureTargets[glState.activeTMU] = target;
		if( target != GL_NONE )
			/*p*/glEnable( glState.currentTextureTargets[glState.activeTMU] );
	}
}

/*
=================
GL_TexGen
=================
*/
#ifndef _XBOX
void GL_TexGen( GLenum coord, GLenum mode )
{
	int	tmu = min( glConfig.max_texture_coords, glState.activeTMU );
	int	bit, gen;

	switch( coord )
	{
	case GL_S:
		bit = 1;
		gen = GL_TEXTURE_GEN_S;
		break;
	case GL_T:
		bit = 2;
		gen = GL_TEXTURE_GEN_T;
		break;
	case GL_R:
		bit = 4;
		gen = GL_TEXTURE_GEN_R;
		break;
	case GL_Q:
		bit = 8;
		gen = GL_TEXTURE_GEN_Q;
		break;
	default: return;
	}

	if( mode )
	{
		if( !( glState.genSTEnabled[tmu] & bit ))
		{
			pglEnable( gen );
			glState.genSTEnabled[tmu] |= bit;
		}
		pglTexGeni( coord, GL_TEXTURE_GEN_MODE, mode );
	}
	else
	{
		if( glState.genSTEnabled[tmu] & bit )
		{
			pglDisable( gen );
			glState.genSTEnabled[tmu] &= ~bit;
		}
	}
}
#endif

/*
=================
GL_SetTexCoordArrayMode
=================
*/
void GL_SetTexCoordArrayMode( GLenum mode )
{
	int	tmu = min( glConfig.max_texture_coords, glState.activeTMU );
	int	bit, cmode = glState.texCoordArrayMode[tmu];

	if( mode == GL_TEXTURE_COORD_ARRAY )
		bit = 1;
	else if( mode == GL_TEXTURE_CUBE_MAP_ARB )
		bit = 2;
	else bit = 0;

	if( cmode != bit )
	{
#ifdef _XBOX
		/*p*/glDisable( GL_TEXTURE_CUBE_MAP_ARB );
		/*p*/glEnable( GL_TEXTURE_CUBE_MAP_ARB );
#else
		if( cmode == 1 ) pglDisableClientState( GL_TEXTURE_COORD_ARRAY );
		else if( cmode == 2 ) pglDisable( GL_TEXTURE_CUBE_MAP_ARB );

		if( bit == 1 ) pglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		else if( bit == 2 ) pglEnable( GL_TEXTURE_CUBE_MAP_ARB );
#endif
		glState.texCoordArrayMode[tmu] = bit;
	}
}

/*
=================
GL_Cull
=================
*/
void GL_Cull( GLenum cull )
{
	if( !cull )
	{
		/*p*/glDisable( GL_CULL_FACE );
		glState.faceCull = 0;
		return;
	}

	/*p*/glEnable( GL_CULL_FACE );
	/*p*/glCullFace( cull );
	glState.faceCull = cull;
}

void GL_SetRenderMode( int mode )
{
	/*p*/glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	switch( mode )
	{
	case kRenderNormal:
	default:
		/*p*/glDisable( GL_BLEND );
		/*p*/glDisable( GL_ALPHA_TEST );
		break;
	case kRenderTransColor:
	case kRenderTransTexture:
		/*p*/glEnable( GL_BLEND );
		/*p*/glDisable( GL_ALPHA_TEST );
		/*p*/glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		break;
	case kRenderTransAlpha:
		/*p*/glDisable( GL_BLEND );
		/*p*/glEnable( GL_ALPHA_TEST );
		break;
	case kRenderGlow:
	case kRenderTransAdd:
		/*p*/glEnable( GL_BLEND );
		/*p*/glDisable( GL_ALPHA_TEST );
		/*p*/glBlendFunc( GL_SRC_ALPHA, GL_ONE );
		break;
	}
}

/*
==============================================================================

SCREEN SHOTS

==============================================================================
*/
// used for 'env' and 'sky' shots
typedef struct envmap_s
{
	vec3_t	angles;
	int	flags;
} envmap_t;

const envmap_t r_skyBoxInfo[6] =
{
{{   0, 270, 180}, IMAGE_FLIP_X },
{{   0,  90, 180}, IMAGE_FLIP_X },
{{ -90,   0, 180}, IMAGE_FLIP_X },
{{  90,   0, 180}, IMAGE_FLIP_X },
{{   0,   0, 180}, IMAGE_FLIP_X },
{{   0, 180, 180}, IMAGE_FLIP_X },
};

const envmap_t r_envMapInfo[6] =
{
{{  0,   0,  90}, 0 },
{{  0, 180, -90}, 0 },
{{  0,  90,   0}, 0 },
{{  0, 270, 180}, 0 },
{{-90, 180, -90}, 0 },
{{ 90,   0,  90}, 0 }
};

/*
===============
VID_WriteOverviewScript

Create overview script file
===============
*/
void VID_WriteOverviewScript( void )
{
	ref_overview_t	*ov = &clgame.overView;
	string		filename;
	file_t		*f;

	Q_snprintf( filename, sizeof( filename ), "overviews/%s.txt", clgame.mapname );

	f = FS_Open( filename, "w", false );
	if( !f ) return;

	FS_Printf( f, "// overview description file for %s.bsp\n\n", clgame.mapname );
	FS_Print( f, "global\n{\n" );
	FS_Printf( f, "\tZOOM\t%.2f\n", ov->flZoom );
	FS_Printf( f, "\tORIGIN\t%.2f\t%.2f\t%.2f\n", ov->origin[0], ov->origin[1], ov->origin[2] );
	FS_Printf( f, "\tROTATED\t%i\n", ov->rotated ? 1 : 0 );
	FS_Print( f, "}\n\nlayer\n{\n" );
	FS_Printf( f, "\tIMAGE\t\"overviews/%s.bmp\"\n", clgame.mapname );
	FS_Printf( f, "\tHEIGHT\t%.2f\n", ov->zFar );	// ???
	FS_Print( f, "}\n" );

	FS_Close( f );
}

qboolean VID_ScreenShot( const char *filename, int shot_type )
{
	rgbdata_t *r_shot;
	uint	flags = IMAGE_FLIP_Y;
	int	width = 0, height = 0;
	qboolean	result;

	r_shot = Mem_Calloc( r_temppool, sizeof( rgbdata_t ));
	r_shot->width = (glState.width + 3) & ~3;
	r_shot->height = (glState.height + 3) & ~3;
	r_shot->flags = IMAGE_HAS_COLOR;
	r_shot->type = PF_RGB_24;
	r_shot->size = r_shot->width * r_shot->height * PFDesc[r_shot->type].bpp;
	r_shot->palette = NULL;
	r_shot->buffer = Mem_Malloc( r_temppool, r_shot->size );

	// get screen frame
	/*p*/glReadPixels( 0, 0, r_shot->width, r_shot->height, GL_RGB, GL_UNSIGNED_BYTE, r_shot->buffer );

	switch( shot_type )
	{
	case VID_SCREENSHOT:
		break;
	case VID_SNAPSHOT:
		FS_AllowDirectPaths( true );
		break;
	case VID_LEVELSHOT:
		flags |= IMAGE_RESAMPLE;
		if( glState.wideScreen )
		{
			height = 480;
			width = 800;
		}
		else
		{
			height = 480;
			width = 640;
		}
		break;
	case VID_MINISHOT:
		flags |= IMAGE_RESAMPLE;
		height = 200;
		width = 320;
		break;
	case VID_MAPSHOT:
		VID_WriteOverviewScript();		// store overview script too
		flags |= IMAGE_RESAMPLE|IMAGE_QUANTIZE;	// GoldSrc request overviews in 8-bit format
		height = 768;
		width = 1024;
		break;
	}

	Image_Process( &r_shot, width, height, flags, 0.0f );

	// write image
	result = FS_SaveImage( filename, r_shot );
#ifndef _XBOX
	host.write_to_clipboard = false;		// disable write to clipboard
#endif
	FS_AllowDirectPaths( false );			// always reset after store screenshot
	FS_FreeImage( r_shot );

	return result;
}

/*
=================
VID_CubemapShot
=================
*/
qboolean VID_CubemapShot( const char *base, uint size, const float *vieworg, qboolean skyshot )
{
	rgbdata_t		*r_shot, *r_side;
	byte		*temp = NULL;
	byte		*buffer = NULL;
	string		basename;
	int		i = 1, flags, result;

	if( !RI.drawWorld || !cl.worldmodel )
		return false;

	// make sure the specified size is valid
	while( i < size ) i<<=1;

	if( i != size ) return false;
	if( size > glState.width || size > glState.height )
		return false;

	// setup refdef
	RI.params |= RP_ENVVIEW;	// do not render non-bmodel entities

	// alloc space
	temp = Mem_Malloc( r_temppool, size * size * 3 );
	buffer = Mem_Malloc( r_temppool, size * size * 3 * 6 );
	r_shot = Mem_Calloc( r_temppool, sizeof( rgbdata_t ));
	r_side = Mem_Calloc( r_temppool, sizeof( rgbdata_t ));

	// use client vieworg
	if( !vieworg ) vieworg = RI.vieworg;

	for( i = 0; i < 6; i++ )
	{
		// go into 3d mode
		R_Set2DMode( false );

		if( skyshot )
		{
			R_DrawCubemapView( vieworg, r_skyBoxInfo[i].angles, size );
			flags = r_skyBoxInfo[i].flags;
		}
		else
		{
			R_DrawCubemapView( vieworg, r_envMapInfo[i].angles, size );
			flags = r_envMapInfo[i].flags;
		}

		/*p*/glReadPixels( 0, 0, size, size, GL_RGB, GL_UNSIGNED_BYTE, temp );
		r_side->flags = IMAGE_HAS_COLOR;
		r_side->width = r_side->height = size;
		r_side->type = PF_RGB_24;
		r_side->size = r_side->width * r_side->height * 3;
		r_side->buffer = temp;

		if( flags ) Image_Process( &r_side, 0, 0, flags, 0.0f );
		memcpy( buffer + (size * size * 3 * i), r_side->buffer, size * size * 3 );
	}

	RI.params &= ~RP_ENVVIEW;

	r_shot->flags = IMAGE_HAS_COLOR;
	r_shot->flags |= (skyshot) ? IMAGE_SKYBOX : IMAGE_CUBEMAP;
	r_shot->width = size;
	r_shot->height = size;
	r_shot->type = PF_RGB_24;
	r_shot->size = r_shot->width * r_shot->height * 3 * 6;
	r_shot->palette = NULL;
	r_shot->buffer = buffer;

	// make sure what we have right extension
	Q_strncpy( basename, base, MAX_STRING );
	COM_StripExtension( basename );
	COM_DefaultExtension( basename, ".tga" );

	// write image as 6 sides
	result = FS_SaveImage( basename, r_shot );
	FS_FreeImage( r_shot );
	FS_FreeImage( r_side );

	return result;
}

//=======================================================

/*
===============
R_ShowTextures

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.
===============
*/
void R_ShowTextures( void )
{
#ifndef _XBOX // We don't need this debug tool on Xbox atm
	gl_texture_t	*image;
	float		x, y, w, h;
	int		total, start, end;
	int		i, j, k, base_w, base_h;
	rgba_t		color = { 192, 192, 192, 255 };
	int		charHeight, numTries = 0;
	static qboolean	showHelp = true;
	string		shortname;

	if( !CVAR_TO_BOOL( gl_showtextures ))
		return;

	if( showHelp )
	{
		CL_CenterPrint( "use '<-' and '->' keys to change atlas page, ESC to quit", 0.25f );
		showHelp = false;
	}

	GL_SetRenderMode( kRenderNormal );
	/*p*/glClear( GL_COLOR_BUFFER_BIT );
	/*p*/glFinish();

	base_w = 8;	// textures view by horizontal
	base_h = 6;	// textures view by vertical

rebuild_page:
	total = base_w * base_h;
	start = total * (gl_showtextures->value - 1);
	end = total * gl_showtextures->value;
	if( end > MAX_TEXTURES ) end = MAX_TEXTURES;

	w = glState.width / base_w;
	h = glState.height / base_h;

	Con_DrawStringLen( NULL, NULL, &charHeight );

	for( i = j = 0; i < MAX_TEXTURES; i++ )
	{
		image = R_GetTexture( i );
		if( j == start ) break; // found start
		if( /*p*/glIsTexture( image->texnum )) j++;
	}

	if( i == MAX_TEXTURES && gl_showtextures->value != 1 )
	{
		// bad case, rewind to one and try again
		Cvar_SetValue( "r_showtextures", max( 1, gl_showtextures->value - 1 ));
		if( ++numTries < 2 ) goto rebuild_page;	// to prevent infinite loop
	}

	for( k = 0; i < MAX_TEXTURES; i++ )
	{
		if( j == end ) break; // page is full

		image = R_GetTexture( i );
		if( !/*p*/glIsTexture( image->texnum ))
			continue;

		x = k % base_w * w;
		y = k / base_w * h;

		/*p*/glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		GL_Bind( GL_TEXTURE0, i ); // NOTE: don't use image->texnum here, because skybox has a 'wrong' indexes

		if( FBitSet( image->flags, TF_DEPTHMAP ) && !FBitSet( image->flags, TF_NOCOMPARE ))
			/*p*/glTexParameteri( image->target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE );

		/*p*/glBegin( GL_QUADS );
		/*p*/glTexCoord2f( 0, 0 );
		/*p*/glVertex2f( x, y );
		if( image->target == GL_TEXTURE_RECTANGLE_EXT )
			/*p*/glTexCoord2f( image->width, 0 );
		else /*p*/glTexCoord2f( 1, 0 );
		/*p*/glVertex2f( x + w, y );
		if( image->target == GL_TEXTURE_RECTANGLE_EXT )
			/*p*/glTexCoord2f( image->width, image->height );
		else /*p*/glTexCoord2f( 1, 1 );
		/*p*/glVertex2f( x + w, y + h );
		if( image->target == GL_TEXTURE_RECTANGLE_EXT )
			/*p*/glTexCoord2f( 0, image->height );
		else /*p*/glTexCoord2f( 0, 1 );
		/*p*/glVertex2f( x, y + h );
		/*p*/glEnd();

		if( FBitSet( image->flags, TF_DEPTHMAP ) && !FBitSet( image->flags, TF_NOCOMPARE ))
			/*p*/glTexParameteri( image->target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB );

		COM_FileBase( image->name, shortname );
		if( Q_strlen( shortname ) > 18 )
		{
			// cutoff too long names, it looks ugly
			shortname[16] = '.';
			shortname[17] = '.';
			shortname[18] = '\0';
		}
		Con_DrawString( x + 1, y + h - charHeight, shortname, color );
		j++, k++;
	}

	CL_DrawCenterPrint ();
	/*p*/glFinish();
#endif
}

#define POINT_SIZE		16.0f
#define NODE_INTERVAL_X(x)	(x * 16.0f)
#define NODE_INTERVAL_Y(x)	(x * 16.0f)

void R_DrawLeafNode( float x, float y, float scale )
{
	float downScale = scale * 0.25f;// * POINT_SIZE;

	R_DrawStretchPic( x - downScale * 0.5f, y - downScale * 0.5f, downScale, downScale, 0, 0, 1, 1, tr.particleTexture );
}

void R_DrawNodeConnection( float x, float y, float x2, float y2 )
{
	/*p*/glBegin( GL_LINES );
		/*p*/glVertex2f( x, y );
		/*p*/glVertex2f( x2, y2 );
	/*p*/glEnd();
}

void R_ShowTree_r( mnode_t *node, float x, float y, float scale, int shownodes )
{
	float	downScale = scale * 0.8f;

	downScale = Q_max( downScale, 1.0f );

	if( !node ) return;

	tr.recursion_level++;

	if( node->contents < 0 )
	{
		mleaf_t	*leaf = (mleaf_t *)node;

		if( tr.recursion_level > tr.max_recursion )
			tr.max_recursion = tr.recursion_level;

		if( shownodes == 1 )
		{
			if( cl.worldmodel->leafs == leaf )
				/*p*/glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
			else if( RI.viewleaf && RI.viewleaf == leaf )
				/*p*/glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
			else /*p*/glColor4f( 0.0f, 1.0f, 0.0f, 1.0f );
			R_DrawLeafNode( x, y, scale );
		}
		tr.recursion_level--;
		return;
	}

	if( shownodes == 1 )
	{
		/*p*/glColor4f( 0.0f, 0.0f, 1.0f, 1.0f );
		R_DrawLeafNode( x, y, scale );
	}
	else if( shownodes == 2 )
	{
		R_DrawNodeConnection( x, y, x - scale, y + scale );
		R_DrawNodeConnection( x, y, x + scale, y + scale );
	}

	R_ShowTree_r( node->children[1], x - scale, y + scale, downScale, shownodes );
	R_ShowTree_r( node->children[0], x + scale, y + scale, downScale, shownodes );

	tr.recursion_level--;
}

void R_ShowTree( void )
{
	float	x = (float)((glState.width - (int)POINT_SIZE) >> 1);
	float	y = NODE_INTERVAL_Y(1.0);

	if( !cl.worldmodel || !CVAR_TO_BOOL( r_showtree ))
		return;

	tr.recursion_level = 0;

	/*p*/glEnable( GL_BLEND );
	/*p*/glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	/*p*/glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	/*p*/glLineWidth( 2.0f );
	/*p*/glColor3f( 1, 0.7f, 0 );
	/*p*/glDisable( GL_TEXTURE_2D );
	R_ShowTree_r( cl.worldmodel->nodes, x, y, tr.max_recursion * 3.5f, 2 );
	/*p*/glEnable( GL_TEXTURE_2D );
	/*p*/glLineWidth( 1.0f );

	R_ShowTree_r( cl.worldmodel->nodes, x, y, tr.max_recursion * 3.5f, 1 );

	Con_NPrintf( 0, "max recursion %d\n", tr.max_recursion );
}