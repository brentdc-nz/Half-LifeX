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

	if( r_speeds->integer <= 0 ) return false;
	if( !out || !size ) return false;

	Q_strncpy( out, r_speeds_msg, size );

	return true;
}

/*
==============
GL_BackendStartFrame
==============
*/
void GL_BackendStartFrame( void )
{
	r_speeds_msg[0] = '\0';

	if( !RI.drawWorld ) R_Set2DMode( false );
}

/*
==============
GL_BackendEndFrame
==============
*/
void GL_BackendEndFrame( void )
{
	// go into 2D mode (in case we draw PlayerSetup between two 2d calls)
	if( !RI.drawWorld ) R_Set2DMode( true );

	if( r_speeds->integer <= 0 || !RI.drawWorld )
		return;

	switch( r_speeds->integer )
	{
	case 1:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i wpoly, %3i bpoly\n%3i epoly, %3i spoly",
		r_stats.c_world_polys, r_stats.c_brush_polys, r_stats.c_studio_polys, r_stats.c_sprite_polys );
		break;		
	case 2:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "visible leafs:\n%3i leafs\ncurrent leaf %3i",
		r_stats.c_world_leafs, r_viewleaf - cl.worldmodel->leafs );
		break;
	case 3:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i studio models drawn\n%3i sprites drawn",
		r_stats.c_studio_models_drawn, r_stats.c_sprite_models_drawn );
		break;
	case 4:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i static entities\n%3i normal entities",
		r_numStatics, r_numEntities - r_numStatics );
		break;
	case 5:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i tempents\n%3i viewbeams\n%3i particles",
		r_stats.c_active_tents_count, r_stats.c_view_beams_count, r_stats.c_particle_count );
		break;
	case 6:
		Q_snprintf( r_speeds_msg, sizeof( r_speeds_msg ), "%3i mirrors\n", r_stats.c_mirror_passes );
		break;
	}

	Q_memset( &r_stats, 0, sizeof( r_stats ));
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
	ASSERT( glmatrix != NULL );
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
void GL_SelectTexture( GLenum texture )
{
	if( !GL_Support( GL_ARB_MULTITEXTURE ))
		return;

#ifndef _USEFAKEGL01 //MARTY FIXME WIP
	// don't allow negative texture units
	if((GLint)texture < 0 ) texture = 0;
	if( glState.activeTMU == texture )
		return;

	glState.activeTMU = texture;

	if( pglActiveTextureARB )
	{
		pglActiveTextureARB( texture + GL_TEXTURE0_ARB );
		pglClientActiveTextureARB( texture + GL_TEXTURE0_ARB );
	}
	else if( pglSelectTextureSGIS )
	{
		pglSelectTextureSGIS( texture + GL_TEXTURE0_SGIS );
	}
#endif
}

/*
==============
GL_DisableAllTexGens
==============
*//*
void GL_DisableAllTexGens( void )
{
	GL_TexGen( GL_S, 0 );
	GL_TexGen( GL_T, 0 );
	GL_TexGen( GL_R, 0 );
	GL_TexGen( GL_Q, 0 );
}
*/
/*
==============
GL_CleanUpTextureUnits
==============
*//*
void GL_CleanUpTextureUnits( int last )
{
	int	i;

	for( i = glState.activeTMU; i > (last - 1); i-- )
	{
		// disable upper units
		if( glState.currentTextureTargets[i] != GL_NONE )
		{
			pglDisable( glState.currentTextureTargets[i] );
			glState.currentTextureTargets[i] = GL_NONE;
			glState.currentTextures[i] = -1; // unbind texture
		}

		GL_LoadIdentityTexMatrix();
		GL_DisableAllTexGens();
		GL_SelectTexture( i - 1 );
	}
}*/

/*
=================
GL_MultiTexCoord2f
=================
*/
void GL_MultiTexCoord2f( GLenum texture, GLfloat s, GLfloat t )
{
	if( pglMultiTexCoord2f )
	{
		pglMultiTexCoord2f( texture + GL_TEXTURE0_ARB, s, t );
	}
	else if( pglMTexCoord2fSGIS )
	{
		pglMTexCoord2fSGIS( texture + GL_TEXTURE0_SGIS, s, t );
	}
}

/*
=================
GL_TexGen
=================
*//*
void GL_TexGen( GLenum coord, GLenum mode )
{
	int	tmu = glState.activeTMU;
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
*/
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

/*
=================
GL_FrontFace
=================
*/
void GL_FrontFace( GLenum front )
{
	/*p*/glFrontFace( front ? GL_CW : GL_CCW );
	glState.frontFace = front;
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
================
VID_ImageAdjustGamma
================
*/
void VID_ImageAdjustGamma( byte *in, uint width, uint height )
{
	int	i, c = width * height;
	float	g = 1.0f / bound( 0.5f, vid_gamma->value, 2.3f );
	byte	r_gammaTable[256];	// adjust screenshot gamma
	byte	*p = in;

	// rebuild the gamma table	
	for( i = 0; i < 256; i++ )
	{
		if( g == 1.0f ) r_gammaTable[i] = i;
		else r_gammaTable[i] = bound( 0, 255 * pow((i + 0.5) / 255.5f, g ) + 0.5f, 255 );
	}

	// adjust screenshots gamma
	for( i = 0; i < c; i++, p += 3 )
	{
		p[0] = r_gammaTable[p[0]];
		p[1] = r_gammaTable[p[1]];
		p[2] = r_gammaTable[p[2]];
	}
}

qboolean VID_ScreenShot( const char *filename, int shot_type )
{
	rgbdata_t *r_shot;
	uint	flags = IMAGE_FLIP_Y;
	int	width = 0, height = 0;
	qboolean	result;

	r_shot = Mem_Alloc( r_temppool, sizeof( rgbdata_t ));
	r_shot->width = (glState.width + 3) & ~3;
	r_shot->height = (glState.height + 3) & ~3;
	r_shot->flags = IMAGE_HAS_COLOR;
	r_shot->type = PF_RGB_24;
	r_shot->size = r_shot->width * r_shot->height * PFDesc[r_shot->type].bpp;
	r_shot->palette = NULL;
	r_shot->buffer = Mem_Alloc( r_temppool, r_shot->size );

	// get screen frame
	/*p*/glReadPixels( 0, 0, r_shot->width, r_shot->height, GL_RGB, GL_UNSIGNED_BYTE, r_shot->buffer );

	switch( shot_type )
	{
	case VID_SCREENSHOT:
		if( !gl_overview->integer )
			VID_ImageAdjustGamma( r_shot->buffer, r_shot->width, r_shot->height ); // scrshot gamma
		break;
	case VID_SNAPSHOT:
//MARTY FIXME WIP
//		FS_AllowDirectPaths( true );
		break;
	case VID_LEVELSHOT:
		flags |= IMAGE_RESAMPLE;
		height = 384;
		width = 512;
		break;
	case VID_MINISHOT:
		flags |= IMAGE_RESAMPLE;
		height = 200;
		width = 320;
		break;
	case VID_MAPSHOT:
//MARTY FIXME WIP
//		V_WriteOverviewScript();		// store overview script too
//		flags |= IMAGE_RESAMPLE|IMAGE_QUANTIZE;	// GoldSrc request overviews in 8-bit format
//		height = 768;
//		width = 1024;
		break;
	}

	Image_Process( &r_shot, width, height, 0.0f, flags );

	// write image
	result = FS_SaveImage( filename, r_shot );
#ifndef _XBOX //MARTY
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
	temp = Mem_Alloc( r_temppool, size * size * 3 );
	buffer = Mem_Alloc( r_temppool, size * size * 3 * 6 );
	r_shot = Mem_Alloc( r_temppool, sizeof( rgbdata_t ));
	r_side = Mem_Alloc( r_temppool, sizeof( rgbdata_t ));

	// use client vieworg
	if( !vieworg ) vieworg = cl.refdef.vieworg;

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

		/*p*/glReadPixels( 0, glState.height - size, size, size, GL_RGB, GL_UNSIGNED_BYTE, temp );
		r_side->flags = IMAGE_HAS_COLOR;
		r_side->width = r_side->height = size;
		r_side->type = PF_RGB_24;
		r_side->size = r_side->width * r_side->height * 3;
		r_side->buffer = temp;

		if( flags ) Image_Process( &r_side, 0, 0, 0.0f, flags );
		Q_memcpy( buffer + (size * size * 3 * i), r_side->buffer, size * size * 3 );
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
	FS_StripExtension( basename );
	FS_DefaultExtension( basename, ".tga" );

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
	gltexture_t	*image;
	float		x, y, w, h;
	int		i, j, base_w, base_h;

	if( !gl_showtextures->integer )
		return;

	if( gl_showtextures->integer == TEX_DETAIL )
		/*p*/glClearColor( 1.0f, 0.0f, 0.5f, 1.0f );

	/*p*/glClear( GL_COLOR_BUFFER_BIT );
	/*p*/glFinish();

	switch( gl_showtextures->integer )
	{
	case TEX_LIGHTMAP:
	case TEX_VGUI:
	case TEX_DETAIL:
	case TEX_CUSTOM:
	case TEX_DEPTHMAP:
		// draw lightmaps as big images
		base_w = 5;
		base_h = 4;
		break;
	default:
		base_w = 16;
		base_h = 12;
		break;	
	}

	for( i = j = 0; i < MAX_TEXTURES; i++ )
	{
		image = R_GetTexture( i );
		if( !image->texnum ) continue;

		if( image->texType != gl_showtextures->integer )
			continue;

		w = glState.width / base_w;
		h = glState.height / base_h;
		x = j % base_w * w;
		y = j / base_w * h;

		/*p*/glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		GL_Bind( GL_TEXTURE0, i );

		if( image->texType == TEX_DEPTHMAP )
		{
			/*p*/glTexParameteri( image->target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE );
			/*p*/glTexParameteri( image->target, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE );
                    }

		/*p*/glBegin( GL_QUADS );
		/*p*/glTexCoord2f( 0, 0 );
		/*p*/glVertex2f( x, y );
		/*p*/glTexCoord2f( 1, 0 );
		/*p*/glVertex2f( x + w, y );
		/*p*/glTexCoord2f( 1, 1 );
		/*p*/glVertex2f( x + w, y + h );
		/*p*/glTexCoord2f( 0, 1 );
		/*p*/glVertex2f( x, y + h );
		/*p*/glEnd();

		if( image->texType == TEX_DEPTHMAP )
		{
			/*p*/glTexParameteri( image->target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB );
			/*p*/glTexParameteri( image->target, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY );
                    }

		j++;
	}
	/*p*/glFinish();
}

//=======================================================