/*
gl_image.c - texture uploading and processing
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

#define TEXTURES_HASH_SIZE	64

static int	r_textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
static int	r_textureMagFilter = GL_LINEAR;
static gltexture_t	r_textures[MAX_TEXTURES];
static gltexture_t	*r_texturesHashTable[TEXTURES_HASH_SIZE];
static int	r_numTextures;
static byte	*scaledImage = NULL;			// pointer to a scaled image
static byte	data2D[BLOCK_SIZE_MAX*BLOCK_SIZE_MAX*4];	// intermediate texbuffer
static rgbdata_t	r_image;					// generic pixelbuffer used for internal textures

// internal tables
static vec3_t	r_luminanceTable[256];	// RGB to luminance
static byte	r_particleTexture[8][8] =
{
{0,0,0,0,0,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,1,1,1,1,1,1,0},
{0,1,1,1,1,1,1,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,0,0,0,0,0},
};

const char *GL_Target( GLenum target )
{
	switch( target )
	{
	case GL_TEXTURE_1D:
		return "1D";
	case GL_TEXTURE_2D:
		return "2D";
	case GL_TEXTURE_3D:
		return "3D";
	case GL_TEXTURE_CUBE_MAP_ARB:
		return "Cube";
	}
	return "??";
}

/*
=================
GL_Bind
=================
*/
void GL_Bind( GLint tmu, GLenum texnum )
{
	gltexture_t	*texture;

	// missed texture ?
	if( texnum <= 0 ) texnum = tr.defaultTexture;
	ASSERT( texnum > 0 && texnum < MAX_TEXTURES );

	GL_SelectTexture( tmu );

	texture = &r_textures[texnum];

	if( glState.currentTextureTargets[tmu] != texture->target )
	{
		if( glState.currentTextureTargets[tmu] != GL_NONE )
			/*p*/glDisable( glState.currentTextureTargets[tmu] );
		glState.currentTextureTargets[tmu] = texture->target;
		/*p*/glEnable( glState.currentTextureTargets[tmu] );
	}

	if( glState.currentTextures[tmu] == texture->texnum )
		return;

	/*p*/glBindTexture( texture->target, texture->texnum );
	glState.currentTextures[tmu] = texture->texnum;
}

/*
=================
R_GetTexture
=================
*/
gltexture_t *R_GetTexture( GLenum texnum )
{
	ASSERT( texnum >= 0 && texnum < MAX_TEXTURES );
	return &r_textures[texnum];
}

/*
=================
GL_SetTextureType

Just for debug (r_showtextures uses it)
=================
*/
void GL_SetTextureType( GLenum texnum, GLenum type )
{
	if( texnum <= 0 ) return;
	ASSERT( texnum >= 0 && texnum < MAX_TEXTURES );
	r_textures[texnum].texType = type;
}

/*
=================
GL_TexFilter
=================
*/
void GL_TexFilter( gltexture_t *tex, qboolean update )
{
	qboolean	allowNearest;

	switch( tex->texType )
	{
	case TEX_NOMIP:
	case TEX_CUBEMAP:
	case TEX_LIGHTMAP:
		allowNearest = false;
		break;
	default:
		allowNearest = true;
		break;
	}

	// set texture filter
	if( tex->flags & TF_DEPTHMAP )
	{
		/*p*/glTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		/*p*/glTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		/*p*/glTexParameteri( tex->target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB );
		/*p*/glTexParameteri( tex->target, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL );
		/*p*/glTexParameteri( tex->target, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY );
	
		if( /*GL_Support( GL_ANISOTROPY_EXT )*/FGL_GL_ANISOTROPY_EXT /*)*/ )
			/*p*/glTexParameterf( tex->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f );
	}
	else if( tex->flags & TF_NOMIPMAP )
	{
		if( tex->flags & TF_NEAREST )
		{
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		}
		else
		{
			if( r_textureMagFilter == GL_NEAREST && allowNearest )
			{
				/*p*/glTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, r_textureMagFilter );
				/*p*/glTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, r_textureMagFilter );
			}
			else
			{
				/*p*/glTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				/*p*/glTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			}
		}
	}
	else
	{
		if( tex->flags & TF_NEAREST )
		{
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		}
		else
		{
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, r_textureMinFilter );
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, r_textureMagFilter );
		}

		// set texture anisotropy if available
		if( /*GL_Support( GL_ANISOTROPY_EXT )*/FGL_GL_ANISOTROPY_EXT) //MARTY FIXME WIP - Hard coded due to FakeGL limitations!
			/*p*/glTexParameterf( tex->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_texture_anisotropy->value );

		// set texture LOD bias if available
		if( /*GL_Support( GL_TEXTURE_LODBIAS )*/FGL_GL_TEXTURE_LODBIAS) //MARTY FIXME WIP - Hard coded due to FakeGL limitations!
			/*p*/glTexParameterf( tex->target, GL_TEXTURE_LOD_BIAS_EXT, gl_texture_lodbias->value );
	}

	if( update ) return;

	if( tex->flags & TF_BORDER && !GL_Support( GL_CLAMP_TEXBORDER_EXT ))
	{
		// border is not support, use clamp instead
		tex->flags &= ~TF_BORDER;
		tex->flags |= TF_CLAMP;
	}

	// set texture wrap
	if( tex->flags & TF_CLAMP )
	{
		if(/*GL_Support( GL_CLAMPTOEDGE_EXT )*/FGL_GL_CLAMPTOEDGE_EXT ) //MARTY FIXME WIP - Hard coded due to FakeGL limitations!
		{ 
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

			if( tex->target != GL_TEXTURE_1D )
				/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

			if( tex->target == GL_TEXTURE_3D )
				/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
		}
		else
		{
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_S, GL_CLAMP );

			if( tex->target != GL_TEXTURE_1D )
				/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_T, GL_CLAMP );

			if( tex->target == GL_TEXTURE_3D )
				/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_R, GL_CLAMP );
		}
	}
	else if( tex->flags & TF_BORDER )
	{
		/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );

		if( tex->target != GL_TEXTURE_1D )
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

		if( tex->target == GL_TEXTURE_3D )
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER );
	}
	else
	{
		/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_S, GL_REPEAT );

		if( tex->target != GL_TEXTURE_1D )
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_T, GL_REPEAT );

		if( tex->target == GL_TEXTURE_3D )
			/*p*/glTexParameteri( tex->target, GL_TEXTURE_WRAP_R, GL_REPEAT );
	}
}

/*
=================
R_SetTextureParameters
=================
*/
void R_SetTextureParameters( void )
{
	gltexture_t	*texture;
	int		i;

	if( !Q_stricmp( gl_texturemode->string, "GL_NEAREST" ))
	{
		r_textureMinFilter = GL_NEAREST;
		r_textureMagFilter = GL_NEAREST;
	}
	else if( !Q_stricmp( gl_texturemode->string, "GL_LINEAR" ))
	{
		r_textureMinFilter = GL_LINEAR;
		r_textureMagFilter = GL_LINEAR;
	}
	else if( !Q_stricmp( gl_texturemode->string, "GL_NEAREST_MIPMAP_NEAREST" ))
	{
		r_textureMinFilter = GL_NEAREST_MIPMAP_NEAREST;
		r_textureMagFilter = GL_NEAREST;
	}
	else if( !Q_stricmp( gl_texturemode->string, "GL_LINEAR_MIPMAP_NEAREST" ))
	{
		r_textureMinFilter = GL_LINEAR_MIPMAP_NEAREST;
		r_textureMagFilter = GL_LINEAR;
	}
	else if( !Q_stricmp( gl_texturemode->string, "GL_NEAREST_MIPMAP_LINEAR" ))
	{
		r_textureMinFilter = GL_NEAREST_MIPMAP_LINEAR;
		r_textureMagFilter = GL_NEAREST;
	}
	else if( !Q_stricmp( gl_texturemode->string, "GL_LINEAR_MIPMAP_LINEAR" ))
	{
		r_textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
		r_textureMagFilter = GL_LINEAR;
	}
	else
	{
		MsgDev( D_ERROR, "gl_texturemode invalid mode %s, defaulting to GL_LINEAR_MIPMAP_LINEAR\n", gl_texturemode->string );
		Cvar_Set( "gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
		r_textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
		r_textureMagFilter = GL_LINEAR;
	}

	gl_texturemode->modified = false;

	if( /*GL_Support( GL_ANISOTROPY_EXT )*/FGL_GL_ANISOTROPY_EXT) //MARTY FIXME WIP - Hard coded due to FakeGL limitations!
	{
		if( gl_texture_anisotropy->value > glConfig.max_texture_anisotropy )
			Cvar_SetFloat( "gl_anisotropy", glConfig.max_texture_anisotropy );
		else if( gl_texture_anisotropy->value < 1.0f )
			Cvar_SetFloat( "gl_anisotropy", 1.0f );
	}

	gl_texture_anisotropy->modified = false;

	if( /*GL_Support( GL_TEXTURE_LODBIAS )*/FGL_GL_TEXTURE_LODBIAS) //MARTY FIXME WIP - Hard coded due to FakeGL limitations!
	{
		if( gl_texture_lodbias->value > glConfig.max_texture_lodbias )
			Cvar_SetFloat( "r_texture_lodbias", glConfig.max_texture_lodbias );
		else if( gl_texture_lodbias->value < -glConfig.max_texture_lodbias )
			Cvar_SetFloat( "r_texture_lodbias", -glConfig.max_texture_lodbias );
	}

	gl_texture_lodbias->modified = false;

	// change all the existing mipmapped texture objects
	for( i = 0, texture = r_textures; i < r_numTextures; i++, texture++ )
	{
		if( !texture->texnum ) continue;	// free slot
		GL_Bind( GL_TEXTURE0, i );
		GL_TexFilter( texture, true );
	}
}

/*
===============
R_TextureList_f
===============
*/
void R_TextureList_f( void )
{
	gltexture_t	*image;
	int		i, texCount, bytes = 0;

	Msg( "\n" );
	Msg("      -w-- -h-- -size- -fmt- type -data-- -encode-- -wrap-- -name--------\n" );

	for( i = texCount = 0, image = r_textures; i < r_numTextures; i++, image++ )
	{
		if( !image->texnum ) continue;

		bytes += image->size;
		texCount++;

		Msg( "%4i: ", i );
		Msg( "%4i %4i ", image->width, image->height );
		Msg( "%5ik ", image->size >> 10 );

		switch( image->format )
		{
		case GL_COMPRESSED_RGBA_ARB:
			Msg( "CRGBA " );
			break;
		case GL_COMPRESSED_RGB_ARB:
			Msg( "CRGB  " );
			break;
		case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
			Msg( "CLA   " );
			break;
		case GL_COMPRESSED_LUMINANCE_ARB:
			Msg( "CL    " );
			break;
		case GL_COMPRESSED_ALPHA_ARB:
			Msg( "CA    " );
			break;
		case GL_COMPRESSED_INTENSITY_ARB:
			Msg( "CI    " );
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			Msg( "DXT1c " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			Msg( "DXT1a " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			Msg( "DXT3  " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			Msg( "DXT5  " );
			break;
		case GL_RGBA:
			Msg( "RGBA  " );
			break;
		case GL_RGBA8:
			Msg( "RGBA8 " );
			break;
		case GL_RGBA4:
			Msg( "RGBA4 " );
			break;
		case GL_RGB:
			Msg( "RGB   " );
			break;
		case GL_RGB8:
			Msg( "RGB8  " );
			break;
		case GL_RGB5:
			Msg( "RGB5  " );
			break;
		case GL_LUMINANCE4_ALPHA4:
			Msg( "L4A4  " );
			break;
		case GL_LUMINANCE_ALPHA:
		case GL_LUMINANCE8_ALPHA8:
			Msg( "L8A8  " );
			break;
		case GL_LUMINANCE4:
			Msg( "L4    " );
			break;
		case GL_LUMINANCE:
		case GL_LUMINANCE8:
			Msg( "L8    " );
			break;
		case GL_ALPHA8:
			Msg( "A8    " );
			break;
		case GL_INTENSITY8:
			Msg( "I8    " );
			break;
		case GL_DEPTH_COMPONENT:
			Msg( "DEPTH " );
			break;			
		default:
			Msg( "????? " );
			break;
		}

		switch( image->target )
		{
		case GL_TEXTURE_1D:
			Msg( " 1D  " );
			break;
		case GL_TEXTURE_2D:
			Msg( " 2D  " );
			break;
		case GL_TEXTURE_3D:
			Msg( " 3D  " );
			break;
		case GL_TEXTURE_CUBE_MAP_ARB:
			Msg( "CUBE " );
			break;
		default:
			Msg( "???? " );
			break;
		}

		if( image->flags & TF_NOMIPMAP )
			Msg( "linear " );
		if( image->flags & TF_NEAREST )
			Msg( "nearest" );
		else Msg( "default" );

		if( image->flags & TF_CLAMP )
			Msg( " clamp  " );
		else if( image->flags & TF_BORDER )
			Msg( " border " );
		else Msg( " repeat " );
		Msg( "  %s\n", image->name );
	}

	Msg( "---------------------------------------------------------\n" );
	Msg( "%i total textures\n", texCount );
	Msg( "%s total memory used\n", Q_memprint( bytes ));
	Msg( "\n" );
}

/*
================
GL_CalcTextureSamples
================
*/
int GL_CalcTextureSamples( int flags )
{
	if( flags & IMAGE_HAS_COLOR )
		return (flags & IMAGE_HAS_ALPHA) ? 4 : 3;
	return (flags & IMAGE_HAS_ALPHA) ? 2 : 1;
}

/*
================
GL_RoundImageDimensions
================
*/
void GL_RoundImageDimensions( word *width, word *height, texFlags_t flags, qboolean force )
{
	int	scaledWidth, scaledHeight;

	scaledWidth = *width;
	scaledHeight = *height;

	if( flags & ( TF_TEXTURE_1D|TF_TEXTURE_3D )) return;

	if( force || !/*GL_Support( GL_ARB_TEXTURE_NPOT_EXT )*/FGL_GL_ARB_TEXTURE_NPOT_EXT)  //MARTY FIXME WIP - Hard coded due to FakeGL limitations!
	{
		// find nearest power of two, rounding down if desired
		scaledWidth = NearestPOW( scaledWidth, gl_round_down->integer );
		scaledHeight = NearestPOW( scaledHeight, gl_round_down->integer );
	}

	if( flags & TF_SKYSIDE )
	{
		// let people sample down the sky textures for speed
		scaledWidth >>= gl_skymip->integer;
		scaledHeight >>= gl_skymip->integer;
	}
	else if(!( flags & TF_NOPICMIP ))
	{
		// let people sample down the world textures for speed
		scaledWidth >>= gl_picmip->integer;
		scaledHeight >>= gl_picmip->integer;
	}

	if( flags & TF_CUBEMAP )
	{
		while( scaledWidth > glConfig.max_cubemap_size || scaledHeight > glConfig.max_cubemap_size )
		{
			scaledWidth >>= 1;
			scaledHeight >>= 1;
		}
	}
	else
	{
		while( scaledWidth > glConfig.max_2d_texture_size || scaledHeight > glConfig.max_2d_texture_size )
		{
			scaledWidth >>= 1;
			scaledHeight >>= 1;
		}
	}

	if( scaledWidth < 1 ) scaledWidth = 1;
	if( scaledHeight < 1 ) scaledHeight = 1;

	*width = scaledWidth;
	*height = scaledHeight;
}

/*
===============
GL_TextureFormat
===============
*/
static GLenum GL_TextureFormat( gltexture_t *tex, int *samples )
{
	qboolean	compress;
	GLenum	format;

	// check if it should be compressed
	if( !gl_compress_textures->integer || ( tex->flags & TF_UNCOMPRESSED ))
		compress = false;
	else compress = /*GL_Support( GL_TEXTURE_COMPRESSION_EXT )*/FGL_GL_TEXTURE_COMPRESSION_EXT; //MARTY FIXME WIP - Hard coded due to FakeGL limitations!

	// set texture format
	if( tex->flags & TF_DEPTHMAP )
	{
		format = GL_DEPTH_COMPONENT;
		tex->flags &= ~TF_INTENSITY;
	}
	else if( compress )
	{
		switch( *samples )
		{
		case 1: format = GL_COMPRESSED_LUMINANCE_ARB; break;
		case 2: format = GL_COMPRESSED_LUMINANCE_ALPHA_ARB; break;
		case 3: format = GL_COMPRESSED_RGB_ARB; break;
		case 4:
		default: format = GL_COMPRESSED_RGBA_ARB; break;
		}

		if( tex->flags & TF_INTENSITY )
			format = GL_COMPRESSED_INTENSITY_ARB;
		tex->flags &= ~TF_INTENSITY;
	}
	else
	{
		int	bits = glw_state.desktopBitsPixel;

		switch( *samples )
		{
		case 1: format = GL_LUMINANCE8; break;
		case 2: format = GL_LUMINANCE8_ALPHA8; break;
		case 3:
			if( gl_luminance_textures->integer )
			{
				switch( bits )
				{
				case 16: format = GL_LUMINANCE4; break;
				case 32: format = GL_LUMINANCE8; break;
				default: format = GL_LUMINANCE; break;
				}
				*samples = 1;	// merge for right calc statistics
			}
			else
			{
				switch( bits )
				{
				case 16: format = GL_RGB5; break;
				case 32: format = GL_RGB8; break;
				default: format = GL_RGB; break;
				}
			}
			break;		
		case 4:
		default:
			if( gl_luminance_textures->integer )
			{
				switch( bits )
				{
				case 16: format = GL_LUMINANCE4_ALPHA4; break;
				case 32: format = GL_LUMINANCE8_ALPHA8; break;
				default: format = GL_LUMINANCE_ALPHA; break;
				}
				*samples = 2;	// merge for right calc statistics
			}
			else
			{
				switch( bits )
				{
				case 16: format = GL_RGBA4; break;
				case 32: format = GL_RGBA8; break;
				default: format = GL_RGBA; break;
				}
			}
			break;
		}

		if( tex->flags & TF_INTENSITY )
			format = GL_INTENSITY8;
		tex->flags &= ~TF_INTENSITY;
	}
	return format;
}

/*
=================
GL_ResampleTexture

Assume input buffer is RGBA
=================
*/
byte *GL_ResampleTexture( const byte *source, int inWidth, int inHeight, int outWidth, int outHeight, qboolean isNormalMap )
{
	uint		frac, fracStep;
	uint		*in = (uint *)source;
	uint		p1[0x1000], p2[0x1000];
	byte		*pix1, *pix2, *pix3, *pix4;
	uint		*out, *inRow1, *inRow2;
	vec3_t		normal;
	int		i, x, y;

	if( !source ) return NULL;

	scaledImage = Mem_Realloc( r_temppool, scaledImage, outWidth * outHeight * 4 );
	fracStep = inWidth * 0x10000 / outWidth;
	out = (uint *)scaledImage;

	frac = fracStep >> 2;
	for( i = 0; i < outWidth; i++ )
	{
		p1[i] = 4 * (frac >> 16);
		frac += fracStep;
	}

	frac = (fracStep >> 2) * 3;
	for( i = 0; i < outWidth; i++ )
	{
		p2[i] = 4 * (frac >> 16);
		frac += fracStep;
	}

	if( isNormalMap )
	{
		for( y = 0; y < outHeight; y++, out += outWidth )
		{
			inRow1 = in + inWidth * (int)(((float)y + 0.25f) * inHeight/outHeight);
			inRow2 = in + inWidth * (int)(((float)y + 0.75f) * inHeight/outHeight);

			for( x = 0; x < outWidth; x++ )
			{
				pix1 = (byte *)inRow1 + p1[x];
				pix2 = (byte *)inRow1 + p2[x];
				pix3 = (byte *)inRow2 + p1[x];
				pix4 = (byte *)inRow2 + p2[x];

				normal[0] = (pix1[0] * (1.0f/127.0f) - 1.0f) + (pix2[0] * (1.0f/127.0f) - 1.0f) + (pix3[0] * (1.0f/127.0f) - 1.0f) + (pix4[0] * (1.0f/127.0f) - 1.0f);
				normal[1] = (pix1[1] * (1.0f/127.0f) - 1.0f) + (pix2[1] * (1.0f/127.0f) - 1.0f) + (pix3[1] * (1.0f/127.0f) - 1.0f) + (pix4[1] * (1.0f/127.0f) - 1.0f);
				normal[2] = (pix1[2] * (1.0f/127.0f) - 1.0f) + (pix2[2] * (1.0f/127.0f) - 1.0f) + (pix3[2] * (1.0f/127.0f) - 1.0f) + (pix4[2] * (1.0f/127.0f) - 1.0f);

				if( !VectorNormalizeLength( normal ))
					VectorSet( normal, 0.0f, 0.0f, 1.0f );

				((byte *)(out+x))[0] = (byte)(128 + 127 * normal[0]);
				((byte *)(out+x))[1] = (byte)(128 + 127 * normal[1]);
				((byte *)(out+x))[2] = (byte)(128 + 127 * normal[2]);
				((byte *)(out+x))[3] = 255;
			}
		}
	}
	else
	{
		for( y = 0; y < outHeight; y++, out += outWidth )
		{
			inRow1 = in + inWidth * (int)(((float)y + 0.25f) * inHeight/outHeight);
			inRow2 = in + inWidth * (int)(((float)y + 0.75f) * inHeight/outHeight);

			for( x = 0; x < outWidth; x++ )
			{
				pix1 = (byte *)inRow1 + p1[x];
				pix2 = (byte *)inRow1 + p2[x];
				pix3 = (byte *)inRow2 + p1[x];
				pix4 = (byte *)inRow2 + p2[x];

				((byte *)(out+x))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
				((byte *)(out+x))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
				((byte *)(out+x))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
				((byte *)(out+x))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
			}
		}
	}

	return scaledImage;
}

/*
=================
GL_ApplyGamma

Assume input buffer is RGBA
=================
*/
byte *GL_ApplyGamma( const byte *source, int pixels, qboolean isNormalMap )
{
	byte	*in = (byte *)source;
	byte	*out = (byte *)source;
	int	i;

	if( !isNormalMap )
	{
		for( i = 0; i < pixels; i++, in += 4 )
		{
			in[0] = TextureToGamma( in[0] );
			in[1] = TextureToGamma( in[1] );
			in[2] = TextureToGamma( in[2] );
		}
	}
	return out;
}

/*
=================
GL_BuildMipMap

Operates in place, quartering the size of the texture
=================
*/
static void GL_BuildMipMap( byte *in, int width, int height, qboolean isNormalMap )
{
	byte	*out = in;
	vec3_t	normal;
	int	x, y;

	width <<= 2;
	height >>= 1;

	if( isNormalMap )
	{
		for( y = 0; y < height; y++, in += width )
		{
			for( x = 0; x < width; x += 8, in += 8, out += 4 )
			{
				normal[0] = (in[0] * (1.0f/127.0f) - 1.0f) + (in[4] * (1.0f/127.0f) - 1.0f) + (in[width+0] * (1.0f/127.0f) - 1.0f) + (in[width+4] * (1.0f/127.0f) - 1.0f);
				normal[1] = (in[1] * (1.0f/127.0f) - 1.0f) + (in[5] * (1.0f/127.0f) - 1.0f) + (in[width+1] * (1.0f/127.0f) - 1.0f) + (in[width+5] * (1.0f/127.0f) - 1.0f);
				normal[2] = (in[2] * (1.0f/127.0f) - 1.0f) + (in[6] * (1.0f/127.0f) - 1.0f) + (in[width+2] * (1.0f/127.0f) - 1.0f) + (in[width+6] * (1.0f/127.0f) - 1.0f);

				if( !VectorNormalizeLength( normal ))
					VectorSet( normal, 0.0f, 0.0f, 1.0f );

				out[0] = (byte)(128 + 127 * normal[0]);
				out[1] = (byte)(128 + 127 * normal[1]);
				out[2] = (byte)(128 + 127 * normal[2]);
				out[3] = 255;
			}
		}
	}
	else
	{
		for( y = 0; y < height; y++, in += width )
		{
			for( x = 0; x < width; x += 8, in += 8, out += 4 )
			{
				out[0] = (in[0] + in[4] + in[width+0] + in[width+4]) >> 2;
				out[1] = (in[1] + in[5] + in[width+1] + in[width+5]) >> 2;
				out[2] = (in[2] + in[6] + in[width+2] + in[width+6]) >> 2;
				out[3] = (in[3] + in[7] + in[width+3] + in[width+7]) >> 2;
			}
		}
	}
}

/*
===============
GL_GenerateMipmaps

sgis generate mipmap
===============
*/
void GL_GenerateMipmaps( byte *buffer, rgbdata_t *pic, gltexture_t *tex, GLenum glTarget, GLenum inFormat, int side, qboolean subImage )
{
	int	mipLevel;
	int	dataType = GL_UNSIGNED_BYTE;
	int	w, h;

	// not needs
	if( tex->flags & TF_NOMIPMAP )
		return;

	if( /*GL_Support( GL_SGIS_MIPMAPS_EXT )*/FGL_GL_SGIS_MIPMAPS_EXT ) //MARTY - Hard coded due to FakeGL limitations!
	{
		/*p*/glHint( GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST );
		/*p*/glTexParameteri( glTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
		/*p*/glGetError(); // clear error queue on mips generate
		return; 
	}

	if( tex->flags & TF_FLOAT )
		dataType = GL_FLOAT;

	mipLevel = 0;
	w = tex->width;
	h = tex->height;

	// software mipmap generator
	while( w > 1 || h > 1 )
	{
		// build the mipmap
		GL_BuildMipMap( buffer, w, h, ( tex->flags & TF_NORMALMAP ));

		w = (w+1)>>1;
		h = (h+1)>>1;
		mipLevel++;

		if( subImage ) /*p*/glTexSubImage2D( tex->target + side, mipLevel, 0, 0, w, h, inFormat, GL_UNSIGNED_BYTE, buffer );
		else /*p*/glTexImage2D( tex->target + side, mipLevel, tex->format, w, h, 0, inFormat, GL_UNSIGNED_BYTE, buffer );
		if( /*p*/glGetError( )) break; // can't create mip levels
	}
}

/*
=================
GL_MakeLuminance

Converts the given image to luminance
=================
*/
void GL_MakeLuminance( rgbdata_t *in )
{
	byte	luminance;
	float	r, g, b;
	int	x, y;

	for( y = 0; y < in->height; y++ )
	{
		for( x = 0; x < in->width; x++ )
		{
			r = r_luminanceTable[in->buffer[4*(y*in->width+x)+0]][0];
			g = r_luminanceTable[in->buffer[4*(y*in->width+x)+1]][1];
			b = r_luminanceTable[in->buffer[4*(y*in->width+x)+2]][2];

			luminance = (byte)(r + g + b);

			in->buffer[4*(y*in->width+x)+0] = luminance;
			in->buffer[4*(y*in->width+x)+1] = luminance;
			in->buffer[4*(y*in->width+x)+2] = luminance;
		}
	}
}

/*
===============
GL_UploadTexture

upload texture into video memory
===============
*/
static void GL_UploadTexture( rgbdata_t *pic, gltexture_t *tex, qboolean subImage )
{
	byte		*buf, *data;
	const byte	*bufend;
	GLenum		outFormat, inFormat, glTarget;
	uint		i, s, numSides, offset = 0;
	int		texsize = 0, img_flags = 0, samples;
	GLint		dataType = GL_UNSIGNED_BYTE;

	ASSERT( pic != NULL && tex != NULL );

	tex->srcWidth = tex->width = pic->width;
	tex->srcHeight = tex->height = pic->height;
	s = tex->srcWidth * tex->srcHeight;

	tex->fogParams[0] = pic->fogParams[0];
	tex->fogParams[1] = pic->fogParams[1];
	tex->fogParams[2] = pic->fogParams[2];
	tex->fogParams[3] = pic->fogParams[3];

	// NOTE: normalmaps must be power of two or software mip generator will stop working
	GL_RoundImageDimensions( &tex->width, &tex->height, tex->flags, false );

	if( s&3 )
	{
		// will be resample, just tell me for debug targets
		MsgDev( D_NOTE, "GL_Upload: %s s&3 [%d x %d]\n", tex->name, tex->srcWidth, tex->srcHeight );
	}
			
	// copy flag about luma pixels
	if( pic->flags & IMAGE_HAS_LUMA )
		tex->flags |= TF_HAS_LUMA;

	// create luma texture from quake texture
	if( tex->flags & TF_MAKELUMA )
	{
		img_flags |= IMAGE_MAKE_LUMA;
		tex->flags &= ~TF_MAKELUMA;
	}

	if( !subImage && tex->flags & TF_KEEP_8BIT )
		tex->original = FS_CopyImage( pic ); // because current pic will be expanded to rgba

	if( !subImage && tex->flags & TF_KEEP_RGBDATA )
		tex->original = pic; // no need to copy

	// we need to expand image into RGBA buffer
	if( pic->type == PF_INDEXED_24 || pic->type == PF_INDEXED_32 )
		img_flags |= IMAGE_FORCE_RGBA;

	// processing image before uploading (force to rgba, make luma etc)
	if( pic->buffer ) Image_Process( &pic, 0, 0, 0.0f, img_flags );

	if( tex->flags & TF_LUMINANCE )
	{
		GL_MakeLuminance( pic );
		tex->flags &= ~TF_LUMINANCE;
		pic->flags &= ~IMAGE_HAS_COLOR;
	}

	samples = GL_CalcTextureSamples( pic->flags );

	if( pic->flags & IMAGE_HAS_ALPHA )
		tex->flags |= TF_HAS_ALPHA;

	// determine format
	inFormat = PFDesc[pic->type].glFormat;
	outFormat = GL_TextureFormat( tex, &samples );
	tex->format = outFormat;

	// determine target
	tex->target = glTarget = GL_TEXTURE_2D;
	numSides = 1;

	if( tex->flags & TF_DEPTHMAP )
	{
		inFormat = GL_DEPTH_COMPONENT;
		dataType = GL_UNSIGNED_BYTE;
	}
	else if( pic->flags & IMAGE_CUBEMAP )
	{
		if( /*GL_Support( GL_TEXTURECUBEMAP_EXT )*/FGL_GL_TEXTURECUBEMAP_EXT ) //MARTY FIXME WIP - Hard coded due to FakeGL limitations!
		{		
			numSides = 6;
			tex->target = glTarget = GL_TEXTURE_CUBE_MAP_ARB;
			tex->flags |= TF_CUBEMAP;

			if( tex->flags & TF_BORDER )
			{
				// don't use border for cubemaps
				tex->flags &= ~TF_BORDER;
				tex->flags |= TF_CLAMP;
			}
		}
		else
		{
			MsgDev( D_WARN, "GL_UploadTexture: cubemaps isn't supported, %s ignored\n", tex->name );
			tex->flags &= ~TF_CUBEMAP;
		}
	}
	else if( tex->flags & TF_TEXTURE_1D )
	{
		// determine target
		tex->target = glTarget = GL_TEXTURE_1D;
	}
	else if( tex->flags & TF_TEXTURE_3D )
	{
		// determine target
		tex->target = glTarget = GL_TEXTURE_3D;
	}

	if( tex->flags & TF_FLOAT )
		dataType = GL_FLOAT;

	/*p*/glBindTexture( tex->target, tex->texnum );

	buf = pic->buffer;
	bufend = pic->buffer + pic->size;
	offset = pic->width * pic->height * PFDesc[pic->type].bpp;

	// NOTE: probably this code relies when gl_compressed_textures is enabled
	texsize = tex->width * tex->height * samples;

	// determine some texTypes
	if( tex->flags & TF_NOPICMIP )
		tex->texType = TEX_NOMIP;
	else if( tex->flags & TF_CUBEMAP )
		tex->texType = TEX_CUBEMAP;
	else if(( tex->flags & TF_DECAL ) == TF_DECAL )
		tex->texType = TEX_DECAL;

	// uploading texture into video memory
	for( i = 0; i < numSides; i++ )
	{
		if( buf != NULL && buf >= bufend )
			Host_Error( "GL_UploadTexture: %s image buffer overflow\n", tex->name );

		// copy or resample the texture
		if(( tex->width == tex->srcWidth && tex->height == tex->srcHeight ) || ( tex->flags & TF_TEXTURE_3D ))
		{
			data = buf;
		}
		else
		{
			data = GL_ResampleTexture( buf, tex->srcWidth, tex->srcHeight, tex->width, tex->height, ( tex->flags & TF_NORMALMAP ));
		}

		if( !glConfig.deviceSupportsGamma )
		{
			if(!( tex->flags & TF_NOMIPMAP ) && !( tex->flags & TF_SKYSIDE ) && !( tex->flags & TF_TEXTURE_3D ))
				data = GL_ApplyGamma( data, tex->width * tex->height, ( tex->flags & TF_NORMALMAP ));
		}		

		if( glTarget == GL_TEXTURE_1D )
//MARTY FIXME WIP
/*		{
			if( subImage ) pglTexSubImage1D( tex->target, 0, 0, tex->width, inFormat, dataType, data );
			else pglTexImage1D( tex->target, 0, outFormat, tex->width, 0, inFormat, dataType, data );
		}
		else if( glTarget == GL_TEXTURE_CUBE_MAP_ARB )
		{
			if( GL_Support( GL_SGIS_MIPMAPS_EXT )) GL_GenerateMipmaps( data, pic, tex, glTarget, inFormat, i, subImage );
			if( subImage ) pglTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, 0, 0, tex->width, tex->height, inFormat, dataType, data );
			else pglTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, outFormat, tex->width, tex->height, 0, inFormat, dataType, data );
			if( !GL_Support( GL_SGIS_MIPMAPS_EXT )) GL_GenerateMipmaps( data, pic, tex, glTarget, inFormat, i, subImage );
		}
		else if( glTarget == GL_TEXTURE_3D )
		{
			if( subImage ) pglTexSubImage3D( tex->target, 0, 0, 0, 0, tex->width, tex->height, pic->depth, inFormat, dataType, data );
			else pglTexImage3D( tex->target, 0, outFormat, tex->width, tex->height, pic->depth, 0, inFormat, dataType, data );
		}
		else
		{
*/			if( /*GL_Support( GL_SGIS_MIPMAPS_EXT )*/FGL_GL_SGIS_MIPMAPS_EXT ) GL_GenerateMipmaps( data, pic, tex, glTarget, inFormat, i, subImage );
			if( subImage ) /*p*/glTexSubImage2D( tex->target, 0, 0, 0, tex->width, tex->height, inFormat, dataType, data );
			else /*p*/glTexImage2D( tex->target, 0, outFormat, tex->width, tex->height, 0, inFormat, dataType, data );
			if( !/*GL_Support( GL_SGIS_MIPMAPS_EXT )*/FGL_GL_SGIS_MIPMAPS_EXT ) GL_GenerateMipmaps( data, pic, tex, glTarget, inFormat, i, subImage );
//		}

		if( numSides > 1 ) buf += offset;
		tex->size += texsize;

		// clear gl error
		while( /*p*/glGetError() != GL_NO_ERROR );
	}
}

/*
================
GL_LoadTexture
================
*/
int GL_LoadTexture( const char *name, const byte *buf, size_t size, int flags )
{
	gltexture_t	*tex;
	rgbdata_t		*pic;
	uint		i, hash;
	uint		picFlags = 0;

	if( !name || !name[0] || !glw_state.initialized )
		return 0;

	if( Q_strlen( name ) >= sizeof( r_textures->name ))
	{
		MsgDev( D_ERROR, "GL_LoadTexture: too long name %s\n", name );
		return 0;
	}

	// HACKHACK: get rid of black vertical line on a 'BlackMesa map'
	if( !Q_strcmp( name, "#lab1_map1.mip" ) || !Q_strcmp( name, "#lab1_map2.mip" ))
	{
		flags |= TF_NEAREST;
	}

	// see if already loaded
	hash = Com_HashKey( name, TEXTURES_HASH_SIZE );

	for( tex = r_texturesHashTable[hash]; tex != NULL; tex = tex->nextHash )
	{
		if( !Q_stricmp( tex->name, name ))
			return tex->texnum;
	}

	if( flags & TF_NOFLIP_TGA )
		picFlags |= IL_DONTFLIP_TGA;

	if( flags & TF_KEEP_8BIT )
		picFlags |= IL_KEEP_8BIT;	

	// set some image flags
	Image_SetForceFlags( picFlags );

	pic = FS_LoadImage( name, buf, size );
	if( !pic ) return 0; // couldn't loading image

	// force upload texture as RGB or RGBA (detail textures requires this)
	if( flags & TF_FORCE_COLOR ) pic->flags |= IMAGE_HAS_COLOR;

	// find a free texture slot
	if( r_numTextures == MAX_TEXTURES )
		Host_Error( "GL_LoadTexture: MAX_TEXTURES limit exceeds\n" );

	// find a free texture_t slot
	for( i = 0, tex = r_textures; i < r_numTextures; i++, tex++ )
		if( !tex->name[0] ) break;

	if( i == r_numTextures )
	{
		if( r_numTextures == MAX_TEXTURES )
			Host_Error( "GL_LoadTexture: MAX_TEXTURES limit exceeds\n" );
		r_numTextures++;
	}

	tex = &r_textures[i];
	Q_strncpy( tex->name, name, sizeof( tex->name ));
	tex->flags = flags;

	if( flags & TF_SKYSIDE )
		tex->texnum = tr.skyboxbasenum++;
	else tex->texnum = i; // texnum is used for fast acess into r_textures array too

	GL_UploadTexture( pic, tex, false );
	GL_TexFilter( tex, false ); // update texture filter, wrap etc

	if(!( flags & ( TF_KEEP_8BIT|TF_KEEP_RGBDATA )))
		FS_FreeImage( pic ); // release source texture

	// add to hash table
	hash = Com_HashKey( tex->name, TEXTURES_HASH_SIZE );
	tex->nextHash = r_texturesHashTable[hash];
	r_texturesHashTable[hash] = tex;

	// NOTE: always return texnum as index in array or engine will stop work !!!
	return i;
}

/*
================
GL_LoadTextureInternal
================
*/
int GL_LoadTextureInternal( const char *name, rgbdata_t *pic, texFlags_t flags, qboolean update )
{
	gltexture_t	*tex;
	uint		i, hash;

	if( !name || !name[0] || !glw_state.initialized )
		return 0;

	if( Q_strlen( name ) >= sizeof( r_textures->name ))
	{
		MsgDev( D_ERROR, "GL_LoadTexture: too long name %s\n", name );
		return 0;
	}

	// see if already loaded
	hash = Com_HashKey( name, TEXTURES_HASH_SIZE );

	for( tex = r_texturesHashTable[hash]; tex != NULL; tex = tex->nextHash )
	{
		if( !Q_stricmp( tex->name, name ))
		{
			 if( update ) break;
			 return tex->texnum;
		}
	}

	if( !pic ) return 0; // couldn't loading image
	if( update && !tex )
	{
		Host_Error( "Couldn't find texture %s for update\n", name );
	}

	// force upload texture as RGB or RGBA (detail textures requires this)
	if( flags & TF_FORCE_COLOR ) pic->flags |= IMAGE_HAS_COLOR;

	// find a free texture slot
	if( r_numTextures == MAX_TEXTURES )
		Host_Error( "GL_LoadTexture: MAX_TEXTURES limit exceeds\n" );

	if( !update )
	{
		// find a free texture_t slot
		for( i = 0, tex = r_textures; i < r_numTextures; i++, tex++ )
			if( !tex->name[0] ) break;

		if( i == r_numTextures )
		{
			if( r_numTextures == MAX_TEXTURES )
				Host_Error( "GL_LoadTexture: MAX_TEXTURES limit exceeds\n" );
			r_numTextures++;
		}

		tex = &r_textures[i];
		hash = Com_HashKey( name, TEXTURES_HASH_SIZE );
		Q_strncpy( tex->name, name, sizeof( tex->name ));
		tex->texnum = i;	// texnum is used for fast acess into r_textures array too
		tex->flags = flags;
	}
	else
	{
		tex->flags |= flags;
	}

	GL_UploadTexture( pic, tex, update );
	GL_TexFilter( tex, update ); // update texture filter, wrap etc

	if( !update )
          {
		// add to hash table
		hash = Com_HashKey( tex->name, TEXTURES_HASH_SIZE );
		tex->nextHash = r_texturesHashTable[hash];
		r_texturesHashTable[hash] = tex;
	}

	return tex->texnum;
}

/*
================
GL_CreateTexture

creates an empty 32-bit texture (just reserve slot)
================
*/
int GL_CreateTexture( const char *name, int width, int height, const void *buffer, texFlags_t flags )
{
	rgbdata_t	r_empty;
	int	texture;

	memset( &r_empty, 0, sizeof( r_empty ));
	r_empty.width = width;
	r_empty.height = height;
	r_empty.type = PF_RGBA_32;
	r_empty.size = r_empty.width * r_empty.height * 4;
	r_empty.flags = IMAGE_HAS_COLOR | (( flags & TF_HAS_ALPHA ) ? IMAGE_HAS_ALPHA : 0 );
	r_empty.buffer = (byte *)buffer;

	if( flags & TF_TEXTURE_1D )
	{
		r_empty.height = 1;
		r_empty.size = r_empty.width * 4;
	}
	else if( flags & TF_TEXTURE_3D )
	{
		if( !GL_Support( GL_TEXTURE_3D_EXT ))
			return 0;

		r_empty.depth = r_empty.width;
		r_empty.size = r_empty.width * r_empty.height * r_empty.depth * 4;
	}

	if( flags & TF_FLOAT )
		r_empty.size *= 4;

	texture = GL_LoadTextureInternal( name, &r_empty, flags, false );

	if( flags & TF_DEPTHMAP )
		GL_SetTextureType( texture, TEX_DEPTHMAP );
	else GL_SetTextureType( texture, TEX_CUSTOM );

	return texture;
}

/*
================
GL_ProcessTexture
================
*/
void GL_ProcessTexture( int texnum, float gamma, int topColor, int bottomColor )
{
	gltexture_t	*image;
	rgbdata_t		*pic;
	int		flags = 0;

	if( texnum <= 0 ) return; // missed image
	ASSERT( texnum > 0 && texnum < MAX_TEXTURES );
	image = &r_textures[texnum];

	// select mode
	if( gamma != -1.0f )
	{
		flags = IMAGE_LIGHTGAMMA;
	}
	else if( topColor != -1 && bottomColor != -1 )
	{
		flags = IMAGE_REMAP;
	}
	else
	{
		MsgDev( D_ERROR, "GL_ProcessTexture: bad operation for %s\n", image->name );
		return;
	}

	if(!( image->flags & (TF_KEEP_RGBDATA|TF_KEEP_8BIT)) || !image->original )
	{
		MsgDev( D_ERROR, "GL_ProcessTexture: no input data for %s\n", image->name );
		return;
	}

	// all the operations makes over the image copy not an original
	pic = FS_CopyImage( image->original );
	Image_Process( &pic, topColor, bottomColor, gamma, flags );

	GL_UploadTexture( pic, image, true );
	GL_TexFilter( image, true ); // update texture filter, wrap etc

	FS_FreeImage( pic );
}

/*
================
GL_LoadTexture
================
*/
int GL_FindTexture( const char *name )
{
	gltexture_t	*tex;
	uint		hash;

	if( !name || !name[0] || !glw_state.initialized )
		return 0;

	if( Q_strlen( name ) >= sizeof( r_textures->name ))
	{
		MsgDev( D_ERROR, "GL_FindTexture: too long name %s\n", name );
		return 0;
	}

	// see if already loaded
	hash = Com_HashKey( name, TEXTURES_HASH_SIZE );

	for( tex = r_texturesHashTable[hash]; tex != NULL; tex = tex->nextHash )
	{
		if( !Q_stricmp( tex->name, name ))
			return tex->texnum;
	}

	return 0;
}

/*
================
GL_FreeImage

Frees image by name
================
*/
void GL_FreeImage( const char *name )
{
	gltexture_t	*tex;
	uint		hash;

	if( !name || !name[0] || !glw_state.initialized )
		return;

	if( Q_strlen( name ) >= sizeof( r_textures->name ))
	{
		MsgDev( D_ERROR, "GL_FreeImage: too long name %s\n", name, sizeof( r_textures->name ));
		return;
	}

	// see if already loaded
	hash = Com_HashKey( name, TEXTURES_HASH_SIZE );

	for( tex = r_texturesHashTable[hash]; tex != NULL; tex = tex->nextHash )
	{
		if( !Q_stricmp( tex->name, name ))
		{
			GL_FreeTexture( tex - r_textures );
			return;
		}
	}
}

/*
================
GL_FreeTexture
================
*/
void GL_FreeTexture( GLenum texnum )
{
	uint		hash;
	gltexture_t	*image;
	gltexture_t	*cur;
	gltexture_t	**prev;

	// number 0 it's already freed
	if( texnum <= 0 || !glw_state.initialized )
		return;

	ASSERT( texnum > 0 && texnum < MAX_TEXTURES );
	image = &r_textures[texnum];

	if( !image->name[0] )
	{
		if( image->texnum != 0 )
			MsgDev( D_ERROR, "trying to free unnamed texture with texnum %i\n", image->texnum );
		return;
	}

	// remove from hash table
	hash = Com_HashKey( image->name, TEXTURES_HASH_SIZE );
	prev = &r_texturesHashTable[hash];

	while( 1 )
	{
		cur = *prev;
		if( !cur ) break;

		if( cur == image )
		{
			*prev = cur->nextHash;
			break;
		}
		prev = &cur->nextHash;
	}

	// release source
	if( image->flags & (TF_KEEP_RGBDATA|TF_KEEP_8BIT) && image->original )
		FS_FreeImage( image->original );

	/*p*/glDeleteTextures( 1, &image->texnum );
	memset( image, 0, sizeof( *image ));
}

/*
==============================================================================

INTERNAL TEXTURES

==============================================================================
*/
/*
==================
R_InitDefaultTexture
==================
*/
static rgbdata_t *R_InitDefaultTexture( texFlags_t *flags )
{
	int	x, y;

	// also use this for bad textures, but without alpha
	r_image.width = r_image.height = 16;
	r_image.buffer = data2D;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;

	*flags = 0;

	// emo-texture from quake1
	for( y = 0; y < 16; y++ )
	{
		for( x = 0; x < 16; x++ )
		{
			if(( y < 8 ) ^ ( x < 8 ))
				((uint *)&data2D)[y*16+x] = 0xFFFF00FF;
			else ((uint *)&data2D)[y*16+x] = 0xFF000000;
		}
	}
	return &r_image;
}

/*
==================
R_InitParticleTexture
==================
*/
static rgbdata_t *R_InitParticleTexture( texFlags_t *flags )
{
	int	x, y;
	int	dx2, dy, d;

	// particle texture
	r_image.width = r_image.height = 16;
	r_image.buffer = data2D;
	r_image.flags = (IMAGE_HAS_COLOR|IMAGE_HAS_ALPHA);
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;

	*flags = TF_NOPICMIP|TF_NOMIPMAP;

	for( x = 0; x < 16; x++ )
	{
		dx2 = x - 8;
		dx2 = dx2 * dx2;

		for( y = 0; y < 16; y++ )
		{
			dy = y - 8;
			d = 255 - 35 * sqrt( dx2 + dy * dy );
			data2D[( y*16 + x ) * 4 + 3] = bound( 0, d, 255 );
		}
	}
	return &r_image;
}

/*
==================
R_InitParticleTexture2
==================
*/
static rgbdata_t *R_InitParticleTexture2( texFlags_t *flags )
{
	int	x, y;

	// particle texture
	r_image.width = r_image.height = 8;
	r_image.buffer = data2D;
	r_image.flags = (IMAGE_HAS_COLOR|IMAGE_HAS_ALPHA);
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;

	*flags = TF_NOPICMIP|TF_NOMIPMAP;

	for( x = 0; x < 8; x++ )
	{
		for( y = 0; y < 8; y++ )
		{
			data2D[(y * 8 + x) * 4 + 0] = 255;
			data2D[(y * 8 + x) * 4 + 1] = 255;
			data2D[(y * 8 + x) * 4 + 2] = 255;
			data2D[(y * 8 + x) * 4 + 3] = r_particleTexture[x][y] * 255;
		}
	}
	return &r_image;
}

/*
==================
R_InitSkyTexture
==================
*/
static rgbdata_t *R_InitSkyTexture( texFlags_t *flags )
{
	int	i;

	// skybox texture
	for( i = 0; i < 256; i++ )
		((uint *)&data2D)[i] = 0xFFFFDEB5;

	*flags = TF_NOPICMIP|TF_UNCOMPRESSED;

	r_image.buffer = data2D;
	r_image.width = r_image.height = 16;
	r_image.size = r_image.width * r_image.height * 4;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;

	return &r_image;
}

/*
==================
R_InitCinematicTexture
==================
*/
static rgbdata_t *R_InitCinematicTexture( texFlags_t *flags )
{
	r_image.buffer = data2D;
	r_image.type = PF_RGBA_32;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.width = r_image.height = 256;
	r_image.size = r_image.width * r_image.height * 4;

	*flags = TF_NOMIPMAP|TF_NOPICMIP|TF_UNCOMPRESSED|TF_CLAMP;

	return &r_image;
}

/*
==================
R_InitSolidColorTexture
==================
*/
static rgbdata_t *R_InitSolidColorTexture( texFlags_t *flags, int color )
{
	// solid color texture
	r_image.width = r_image.height = 1;
	r_image.buffer = data2D;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGB_24;
	r_image.size = r_image.width * r_image.height * 3;

	*flags = TF_NOPICMIP|TF_UNCOMPRESSED;

	data2D[0] = data2D[1] = data2D[2] = color;
	return &r_image;
}

/*
==================
R_InitWhiteTexture
==================
*/
static rgbdata_t *R_InitWhiteTexture( texFlags_t *flags )
{
	return R_InitSolidColorTexture( flags, 255 );
}

/*
==================
R_InitGrayTexture
==================
*/
static rgbdata_t *R_InitGrayTexture( texFlags_t *flags )
{
	return R_InitSolidColorTexture( flags, 127 );
}

/*
==================
R_InitBlackTexture
==================
*/
static rgbdata_t *R_InitBlackTexture( texFlags_t *flags )
{
	return R_InitSolidColorTexture( flags, 0 );
}

static rgbdata_t *R_InitDlightTexture( texFlags_t *flags )
{
	// solid color texture
	r_image.width = BLOCK_SIZE_DEFAULT; 
	r_image.height = BLOCK_SIZE_DEFAULT;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;
	r_image.buffer = data2D;

	memset( data2D, 0x00, r_image.size );

	*flags = TF_NOPICMIP|TF_UNCOMPRESSED|TF_NOMIPMAP;

	return &r_image;
}

static rgbdata_t *R_InitDlightTexture2( texFlags_t *flags )
{
	// solid color texture
	r_image.width = BLOCK_SIZE_MAX; 
	r_image.height = BLOCK_SIZE_MAX;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;
	r_image.buffer = data2D;

	memset( data2D, 0x00, r_image.size );

	*flags = TF_NOPICMIP|TF_UNCOMPRESSED|TF_NOMIPMAP;

	return &r_image;
}

/*
==================
R_InitBuiltinTextures
==================
*/
static void R_InitBuiltinTextures( void )
{
	rgbdata_t		*pic;
	texFlags_t	flags;

	const struct
	{
		char	*name;
		int	*texnum;
		rgbdata_t	*(*init)( texFlags_t *flags );
		int	texType;
	}
	textures[] =
	{
	{ "*default", &tr.defaultTexture, R_InitDefaultTexture, TEX_SYSTEM },
	{ "*white", &tr.whiteTexture, R_InitWhiteTexture, TEX_SYSTEM },
	{ "*gray", &tr.grayTexture, R_InitGrayTexture, TEX_SYSTEM },
	{ "*black", &tr.blackTexture, R_InitBlackTexture, TEX_SYSTEM },
	{ "*particle", &tr.particleTexture, R_InitParticleTexture, TEX_SYSTEM },
	{ "*particle2", &tr.particleTexture2, R_InitParticleTexture2, TEX_SYSTEM },
	{ "*cintexture", &tr.cinTexture, R_InitCinematicTexture, TEX_NOMIP },	// force linear filter
	{ "*dlight", &tr.dlightTexture, R_InitDlightTexture, TEX_LIGHTMAP },
	{ "*dlight2", &tr.dlightTexture2, R_InitDlightTexture2, TEX_LIGHTMAP },
	{ "*sky", &tr.skyTexture, R_InitSkyTexture, TEX_SYSTEM },
	{ NULL, NULL, NULL }
	};
	size_t	i, num_builtin_textures = sizeof( textures ) / sizeof( textures[0] ) - 1;

	for( i = 0; i < num_builtin_textures; i++ )
	{
		memset( &r_image, 0, sizeof( rgbdata_t ));
		memset( data2D, 0xFF, sizeof( data2D ));

		pic = textures[i].init( &flags );
		if( pic == NULL ) continue;
		*textures[i].texnum = GL_LoadTextureInternal( textures[i].name, pic, flags, false );

		GL_SetTextureType( *textures[i].texnum, textures[i].texType );
	}
}

/*
===============
R_InitImages
===============
*/
void R_InitImages( void )
{
	uint	i, hash;
	float	f;

	scaledImage = NULL;
	memset( r_textures, 0, sizeof( r_textures ));
	memset( r_texturesHashTable, 0, sizeof( r_texturesHashTable ));
	r_numTextures = 0;

	// create unused 0-entry
	Q_strncpy( r_textures->name, "*unused*", sizeof( r_textures->name ));
	hash = Com_HashKey( r_textures->name, TEXTURES_HASH_SIZE );
	r_textures->nextHash = r_texturesHashTable[hash];
	r_texturesHashTable[hash] = r_textures;
	r_numTextures = 1;

	// build luminance table
	for( i = 0; i < 256; i++ )
	{
		f = (float)i;
		r_luminanceTable[i][0] = f * 0.299f;
		r_luminanceTable[i][1] = f * 0.587f;
		r_luminanceTable[i][2] = f * 0.114f;
	}

	// set texture parameters
	R_SetTextureParameters();
	R_InitBuiltinTextures();
}

/*
===============
R_ShutdownImages
===============
*/
void R_ShutdownImages( void )
{
	gltexture_t	*image;
	int		i;

	if( !glw_state.initialized ) return;

	for( i = ( MAX_TEXTURE_UNITS - 1); i >= 0; i-- )
	{
		if( i >= GL_MaxTextureUnits( ))
			continue;

		GL_SelectTexture( i );
		/*p*/glBindTexture( GL_TEXTURE_2D, 0 );

		if( /*GL_Support( GL_TEXTURECUBEMAP_EXT )*/ FGL_GL_TEXTURECUBEMAP_EXT ) //MARTY
			/*p*/glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, 0 );
	}

	for( i = 0, image = r_textures; i < r_numTextures; i++, image++ )
	{
		if( !image->texnum ) continue;
		GL_FreeTexture( i );
	}

	memset( tr.lightmapTextures, 0, sizeof( tr.lightmapTextures ));
	memset( r_texturesHashTable, 0, sizeof( r_texturesHashTable ));
	memset( r_textures, 0, sizeof( r_textures ));
	r_numTextures = 0;
}