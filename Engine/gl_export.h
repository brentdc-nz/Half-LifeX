//MARTY Start
//===============================================
// OpenGL Extensions:
// FakeGL can only return multitexture extensions
// The others need to be hardcoded on/off atm.
//===============================================

// Multitexture Extensions:

//Included in FakeGL:

#define GL_TEXTURE0_ARB		0x84C0

void ( APIENTRY *pglMultiTexCoord1f) (GLenum, GLfloat);
void ( APIENTRY *pglMultiTexCoord2f) (GLenum, GLfloat, GLfloat);
//void ( APIENTRY *pglMultiTexCoord3f) (GLenum, GLfloat, GLfloat, GLfloat); //MARTY - Not used.
//void ( APIENTRY *pglMultiTexCoord4f) (GLenum, GLfloat, GLfloat, GLfloat, GLfloat); //MARTY - Not used.
void ( APIENTRY *pglActiveTextureARB)( GLenum );
void ( APIENTRY *pglClientActiveTexture) (GLenum);
void ( APIENTRY *pglClientActiveTextureARB)( GLenum );

//===

//Extension not included in FakeGL, need the function pointers tho.

#define GL_TEXTURE0_SGIS	0x835E

void ( APIENTRY * pglSelectTextureSGIS) ( GLenum );
void ( APIENTRY * pglMTexCoord2fSGIS) ( GLenum, GLfloat, GLfloat );

//===

//Cant use FakeGL GL_GetProcAddress for the below
//turn them on/off here.

#define FGL_GL_ARB_TEXTURE_NPOT_EXT		FALSE
#define FGL_GL_ANISOTROPY_EXT			TRUE
#define FGL_GL_TEXTURE_LODBIAS			TRUE
#define FGL_GL_CLAMPTOEDGE_EXT			TRUE
#define FGL_GL_SGIS_MIPMAPS_EXT			TRUE
#define FGL_GL_TEXTURECUBEMAP_EXT		FALSE
#define FGL_GL_TEXTURE_COMPRESSION_EXT	FALSE

//===============================================
//MARTY - Extensions End

//MARTY - These match FakeGL but different names
#define GL_TEXTURE_CUBE_MAP_ARB				0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB	0x8515
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB	0x851C
#define GL_MAX_3D_TEXTURE_SIZE				0x8073
#define GL_MAX_TEXTURE_LOD_BIAS_EXT			0x84FD
#define GL_MAX_TEXTURE_UNITS_ARB			0x84E2

#define GL_BGRA								0x80E1

#define GL_TEXTURE_RECTANGLE_NV				0x84F5
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_NV	0x84F8
#define GL_TEXTURE_RECTANGLE_EXT			0x84F5
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT	0x84F8

#define GL_TEXTURE_3D						0x806F

#define GL_TEXTURE_WRAP_R					0x8072

#define GL_CLAMP_TO_EDGE					0x812F

#define GL_TEXTURE_LOD_BIAS_EXT				0x8501

#define GL_TEXTURE_MAX_ANISOTROPY_EXT		0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT	0x84FF

#define GL_GENERATE_MIPMAP_HINT_SGIS      	0x8192
#define GL_GENERATE_MIPMAP_SGIS           	0x8191

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT		0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT	0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT	0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT	0x83F3
#define GL_COMPRESSED_ALPHA_ARB				0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB			0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB	0x84EB
#define GL_COMPRESSED_INTENSITY_ARB			0x84EC
#define GL_COMPRESSED_RGB_ARB				0x84ED
#define GL_COMPRESSED_RGBA_ARB				0x84EE

#define GL_DEPTH_TEXTURE_MODE_ARB		0x884B
#define GL_TEXTURE_COMPARE_MODE_ARB		0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB		0x884D
#define GL_COMPARE_R_TO_TEXTURE_ARB		0x884E
#define GL_TEXTURE_COMPARE_FAIL_VALUE_ARB	0x80BF


#define GL_TEXTURE_ENV			0x2300
#define GL_TEXTURE_ENV_MODE			0x2200
#define GL_TEXTURE_ENV_COLOR			0x2201
#define GL_TEXTURE_1D			0x0DE0
#define GL_TEXTURE_2D			0x0DE1
#define GL_TEXTURE_WRAP_S			0x2802
#define GL_TEXTURE_WRAP_T			0x2803
#define GL_TEXTURE_WRAP_R			0x8072
#define GL_TEXTURE_BORDER_COLOR		0x1004
#define GL_TEXTURE_MAG_FILTER			0x2800
#define GL_TEXTURE_MIN_FILTER			0x2801
#define GL_PACK_ALIGNMENT			0x0D05
#define GL_UNPACK_ALIGNMENT			0x0CF5
#define GL_TEXTURE_BINDING_1D			0x8068
#define GL_TEXTURE_BINDING_2D			0x8069
#define GL_CLAMP_TO_EDGE                  	0x812F
#define GL_CLAMP_TO_BORDER                	0x812D
#define GL_NEAREST				0x2600
#define GL_LINEAR				0x2601
#define GL_NEAREST_MIPMAP_NEAREST		0x2700
#define GL_NEAREST_MIPMAP_LINEAR		0x2702
#define GL_LINEAR_MIPMAP_NEAREST		0x2701
#define GL_LINEAR_MIPMAP_LINEAR		0x2703