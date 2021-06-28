/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2007 HermitWorks Entertainment Corporation

This file is part of the Space Trader source code.

The Space Trader source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

The Space Trader source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the Space Trader source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


#ifndef TR_LOCAL_H
#define TR_LOCAL_H

//#define Q_DO_NOT_DISABLE_USEFUL_WARNINGS

#include "../qcommon/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "../renderer/tr_public.h"
#include "../renderer/tr_common.h"
#include "qgl.h"

#ifndef TR_COMPILE_NO_SSE
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4799 ) //missing emms in some xmmintrin.h routines
#endif

#ifdef _MSC_VER
#pragma warning( pop )
#endif
#endif

#define GL_INDEX_TYPE		GL_UNSIGNED_SHORT
typedef GLushort glIndex_t;

#define GL_INDEX_TYPE_MAX_VALUE ((glIndex_t)~0)

#ifdef _MSC_VER
#define alloca _alloca
#endif

#ifndef MACOS_X
typedef unsigned short ushort;
#endif

#define Swap2_( l ) ((((l) << 8) & 0xFF00) | (((l) >> 8) & 0xFF))
#define Swap2s( l ) (short)Swap2_( l )
#define Swap2u( l ) (ushort)Swap2_( l )

#define Swap4_( l ) ((((l) << 24) & 0xFF000000) | (((l) << 8) & 0x00FF0000) | (((l) >> 8) & 0x0000FF00) | (((l) >> 24) & 0x000000FF))
#define Swap4s( l ) (int)Swap4_( l )
#define Swap4u( l ) (uint)Swap4_( l )

static ID_INLINE float SwapF( float f ) { union { uint i; float f; } u; u.f = f; u.i = Swap4u( u.i ); return u.f; }

//
// state management
//

#define NEEDED_TEXTURE_UNITS 4

#if NEEDED_TEXTURE_UNITS > 16
#error Need too many texture units!
#elif NEEDED_TEXTURE_UNITS > 8
#define STATE_MAX_TEXTURE_UNITS 16
#define DECLARE_TEXTURE_STATE_SET( _bn )	\
	_bn##First,								\
	_bn##0 = _bn##First,					\
	_bn##1,									\
	_bn##2,									\
	_bn##3,									\
	_bn##4,									\
	_bn##5,									\
	_bn##6,									\
	_bn##7,									\
	_bn##8,									\
	_bn##9,									\
	_bn##10,								\
	_bn##11,								\
	_bn##12,								\
	_bn##13,								\
	_bn##14,								\
	_bn##15,								\
	_bn##Last = _bn##15
#elif NEEDED_TEXTURE_UNITS > 4
#define STATE_MAX_TEXTURE_UNITS 8
#define DECLARE_TEXTURE_STATE_SET( _bn )	\
	_bn##First,								\
	_bn##0 = _bn##First,					\
	_bn##1,									\
	_bn##2,									\
	_bn##3,									\
	_bn##4,									\
	_bn##5,									\
	_bn##6,									\
	_bn##7,									\
	_bn##Last = _bn##7
#elif NEEDED_TEXTURE_UNITS > 2
#define STATE_MAX_TEXTURE_UNITS 4
#define DECLARE_TEXTURE_STATE_SET( _bn )	\
	_bn##First,								\
	_bn##0 = _bn##First,					\
	_bn##1,									\
	_bn##2,									\
	_bn##3,									\
	_bn##Last = _bn##3
#elif NEEDED_TEXTURE_UNITS > 1
#define STATE_MAX_TEXTURE_UNITS 2
#define DECLARE_TEXTURE_STATE_SET( _bn )	\
	_bn##First,								\
	_bn##0 = _bn##First,					\
	_bn##1,									\
	_bn##Last = _bn##1
#else
#define STATE_MAX_TEXTURE_UNITS 1
#define DECLARE_TEXTURE_STATE_SET( _bn )	\
	_bn##First,								\
	_bn##0 = _bn##First,					\
	_bn##Last = _bn##0
#endif
/*
	Tracked states have defaults and can be
	reset to their default value using the group
	restore mechanism.
*/
typedef enum trackedStateName_e
{
	TSN_Blend,
	TSN_AlphaTest,
	TSN_Cull,
	TSN_RasterMode,
	TSN_PolygonOffset,

	TSN_DepthTest,
	TSN_DepthFunc,
	TSN_DepthMask,
	TSN_DepthRange,

	DECLARE_TEXTURE_STATE_SET( TSN_Texture ),
	DECLARE_TEXTURE_STATE_SET( TSN_TexSampler ),
	DECLARE_TEXTURE_STATE_SET( TSN_TexEnvMode ),

	TSN_TextureFirstState = TSN_Texture0,
	TSN_TextureLastState = TSN_TexEnvModeLast,

	TSN_MaxStandardStates,
	TSN_FirstCustomState = TSN_MaxStandardStates,

	TSN_MaxCustomStates = TSN_FirstCustomState + 8,
	TSN_MaxStates = TSN_MaxCustomStates
} trackedStateName_t;

struct trackedState_s;
typedef void (*stateReset_t)( struct trackedState_s *s );

typedef struct trackedState_s
{
	trackedStateName_t	name;
	stateReset_t		reset;

	union
	{
		GLenum			asglenum;
		GLuint			asgluint;
		int				asint;
		uint			asuint;
		void			*asptr;
	} data;
} trackedState_t;


/*
	Counted states don't have well defined defaults.
	They just keep a "last updated" number that
	increments (with wrap) on each update. They're
	useful for minimizing the work state mirroring has
	to do.
*/
typedef enum countedStateName_e
{
	CSN_ModelViewMatrix,
	CSN_ProjectionMatrix,

	//the TexMatN states are updated automatically as the image changes 
	DECLARE_TEXTURE_STATE_SET( CSN_TexMat ),

	CSN_TextureFirstState = CSN_TexMat0,
	CSN_TextureLastState = CSN_TexMatLast,

	CSN_MaxStandardStates,
	CSN_FirstCustomState = CSN_MaxStandardStates,
	
	CSN_MaxCustomStates = CSN_FirstCustomState + 8,
	CSN_MaxStates = CSN_MaxCustomStates
} countedStateName_t;

typedef struct countedState_s
{
	countedStateName_t	name;
	uint				count;
	
	union
	{
		GLenum			asglenum;
		int				asint;
		uint			asuint;
		void			*asptr;
	} data;
} countedState_t;

typedef enum cullType_t
{
	CT_FRONT_SIDED,		//show front
	CT_BACK_SIDED,		//show back
	CT_TWO_SIDED,		//show both
} cullType_t;


typedef enum texAddrMode_e
{
	TAM_Normalized,
	TAM_Dimensions,
} texAddrMode_t;

typedef struct samplerState_t
{
	GLenum			wrapR, wrapS, wrapT;		// set to GL_NONE (0) for target defaults
	GLenum			minFilter, magFilter;		// set to GL_NONE (0) for Cvar defaults
	GLfloat			maxAniso;
} samplerState_t;

typedef struct image_t
{
	GLenum				texTarget;				// gl texture target
	GLuint				texnum;					// gl texture binding
	texAddrMode_t		addrMode;
	int					uploadWidth, uploadHeight, uploadDepth;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE

	samplerState_t		samplerState;

	char				imgName[MAX_QPATH];		// game path, including extension
	int					width, height, depth;	// source image
	
	int					frameUsed;				// for texture usage in frame statistics
	int					internalFormat;

	qboolean			hasAlpha;
	qboolean			mipmap;

	struct image_t*		next;
} image_t;

typedef struct textureEnv_t
{
	bool		complex;

	GLenum		color_mode;
	GLenum		alpha_mode;

	struct
	{
		GLenum	color_src;
		GLenum	color_op;
		GLenum	alpha_src;
		GLenum	alpha_op;
	} src[3];

	GLfloat		color[4];

	GLfloat		color_scale;
	GLfloat		alpha_scale;
} textureEnv_t;

void R_StateResolveTextureEnv( textureEnv_t *env );

void R_StateInit( void );
void R_StateKill( void );

trackedState_t* R_StateRegisterCustomTrackedState( stateReset_t resetFn );
countedState_t* R_StateRegisterCustomCountedState( uint startCount );

GLenum R_StateGetRectTarget( void );
qboolean R_StateTargetIsRect( GLenum target );

typedef unsigned short stateGroup_t;
stateGroup_t R_StateBeginGroup( void );
//marks the state as having been set in the current group
//only call if not using the R_StateSet**** functions
void R_StateJoinGroup( trackedStateName_t state );
void R_StateRestoreGroupStates( stateGroup_t group );
void R_StateRestorePriorGroupStates( stateGroup_t group );
void R_StateRestoreSubsequentGroupStates( stateGroup_t group );
void R_StateRestoreAllStates( void );
qboolean R_StateIsCurrent( trackedStateName_t state );

void R_StateCountIncrement( countedStateName_t state );
uint R_StateGetCount( countedStateName_t state );

//forces a reset of this state when the next group state call is made
void R_StateForceReset( trackedStateName_t state );
//sets the state to its default value if it's been modified
void R_StateRestoreState( trackedStateName_t state );
//sets the state to its default value whether it's been modified or not
void R_StateForceRestoreState( trackedStateName_t state );

void R_StateRestoreTextureStates( void );
void R_StateForceRestoreTextureStates( void );

//Cvar change notification callbacks.
void R_StateSetTextureModeCvar( const char *string );
void R_StateSetTextureAnisotropyCvar( int aniso );
void R_StateSetTextureMinLodCvar( int minLod );

//query functions
uint R_StateGetNumTextureUnits( void );

//untracked state sets
void R_StateSetDefaultTextureModesUntracked( GLenum target, qboolean mipmap, GLenum tmu );
void R_StateSetActiveClientTmuUntracked( GLenum tmu );
void R_StateSetActiveTmuUntracked( GLenum tmu );

//counted states
void R_StateSetModelViewMatrixCountedRaw( const float *mat );
void R_StateSetModelViewMatrixCountedAffine( const affine_t *mat );
void R_StateSetProjectionMatrixCountedRaw( const float *mat );
void R_StateMulProjectionMatrixCountedRaw( const float *mat );
void R_StateSetTextureMatrixCountedRaw( const float *mat, GLenum tmu );
void R_StateSetTextureMatrixCountedAffine( const affine_t *mat, GLenum tmu );
void R_StateMulTextureMatrixCountedRaw( const float *mat, GLenum tmu );
void R_StateMulTextureMatrixCountedAffine( const affine_t *mat, GLenum tmu );

//tracked states
void R_StateSetTexture( struct image_t *image, GLenum tmu );
void R_StateSetTextureRaw( GLuint tex, GLenum target, qboolean mipmap, GLenum tmu );
void R_StateSetTextureSampler( const struct samplerState_t *sampler, GLenum tmu );
void R_StateSetTextureEnvMode( GLenum env, GLenum tmu );
void R_StateSetTextureEnvMode2( const textureEnv_t *env, GLenum tmu );

void R_StateSetDefaultBlend( GLenum src, GLenum dst );
void R_StateSetBlend( GLenum src, GLenum dst );
void R_StateSetAlphaTest( GLenum func, float ref );
void R_StateSetDefaultCull( GLenum visFace ); //accepts GL_FRONT, GL_BACK, GL_FRONT_AND_BACK where GL_FRONT means *show* the front
void R_StateSetCull( GLenum visFace );
void R_StateSetRasterMode( GLenum mode );
void R_StateSetShadeModel( GLenum model );
void R_StateSetPolygonOffset( float f, float u );

void R_StateSetDefaultDepthTest( qboolean val );
void R_StateSetDefaultDepthFunc( GLenum val );

void R_StateSetDepthTest( qboolean depthTest );
void R_StateSetDepthFunc( GLenum func );
void R_StateSetDepthMask( qboolean mask );
void R_StateSetDefaultDepthRange( float n, float f );
void R_StateSetDepthRange( float n, float f );

// everything that is needed by the backend needs
// to be double buffered to allow it to run in
// parallel on a dual cpu machine
#define	SMP_FRAMES		2

// 12 bits
// see QSORT_SHADERNUM_SHIFT
// can't be increased without changing bit packing for drawsurfs
#define	MAX_SHADERS				16384

typedef struct dlight_s {
	vec3_t	origin;
	vec3_t	color;				// range from 0.0 to 1.0, should be color normalized
	float	radius;

	vec3_t	transformed;		// origin in local coordinate system
	int		additive;			// texture detail is lost tho when the lightmap is dark
} dlight_t;


// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
typedef struct {
	refEntity_t	e;

	float		axisLength;		// compensate for non-normalized axis

	qboolean	needDlights;	// true for bmodels that touch a dlight
	qboolean	lightingCalculated;
	vec3_t		lightDir;		// normalized direction towards light
	vec3_t		ambientLight;	// color normalized to 0-255
	int			ambientLightInt;	// 32 bit rgba packed
	vec3_t		directedLight;
} trRefEntity_t;

typedef struct {
	vec3_t		origin;			// in world coordinates
	vec3_t		axis[3];		// orientation in world
	vec3_t		viewOrigin;		// viewParms->or.origin in local coordinates
	float		modelMatrix[16];
} orientationr_t;


//===============================================================================

typedef enum
{
	SS_BAD,
	SS_PORTAL,			// mirrors, portals, viewscreens
	SS_ENVIRONMENT,		// sky box
	SS_OPAQUE,			// opaque

	SS_DECAL,			// scorch marks, etc.
	SS_SEE_THROUGH,		// ladders, grates, grills that may have small blended edges
						// in addition to alpha test
	SS_BANNER,

	SS_FLARE,			// this is where the flare occlusion queries will be issued

	SS_FOG,

	SS_UNDERWATER,		// for items that should be drawn in front of the water plane

	SS_BLEND0,			// regular transparency and filters
	SS_BLEND1,			// generally only used for additive type effects
	SS_BLEND2,
	SS_BLEND3,

	SS_BLEND6,
	SS_STENCIL_SHADOW,
	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST			// blood blobs
} shaderSort_t;


#define MAX_SHADER_STAGES 8

typedef enum {
	GF_NONE,

	GF_SIN,
	GF_SQUARE,
	GF_TRIANGLE,
	GF_SAWTOOTH, 
	GF_INVERSE_SAWTOOTH, 

	GF_NOISE

} genFunc_t;


typedef enum {
	DEFORM_NONE,
	DEFORM_WAVE,
	DEFORM_NORMALS,
	DEFORM_EXPAND,
	DEFORM_BULGE,
	DEFORM_MOVE,
	DEFORM_PROJECTION_SHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2,
	DEFORM_TEXT0,
	DEFORM_TEXT1,
	DEFORM_TEXT2,
	DEFORM_TEXT3,
	DEFORM_TEXT4,
	DEFORM_TEXT5,
	DEFORM_TEXT6,
	DEFORM_TEXT7
} deform_t;

typedef enum
{
	AGEN_IDENTITY,
	AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	AGEN_LIGHTING_SPECULAR,
	AGEN_GLOW_HALO,
	AGEN_WAVEFORM,
	AGEN_PORTAL,
	AGEN_CONST
} alphaGen_t;

typedef enum
{
	CGEN_BAD,
	CGEN_SKIP,
	CGEN_IDENTITY_LIGHTING,	// tr.identityLight
	CGEN_IDENTITY,			// always (1,1,1,1)
	CGEN_ENTITY,			// grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,	// grabbed from 1 - entity.modulate
	CGEN_EXACT_VERTEX,		// tess.vertexColors
	CGEN_VERTEX,			// tess.vertexColors * tr.identityLight
	CGEN_ONE_MINUS_VERTEX,
	CGEN_WAVEFORM,			// programmatically generated
	CGEN_LIGHTING_DIFFUSE,
	CGEN_LIGHTING_DIFFUSE2,
	CGEN_INVERSE_LIGHTING_DIFFUSE,
	CGEN_FOG,				// standard fog
	CGEN_CONST,				// fixed color
} colorGen_t;

typedef enum {
	TCGEN_BAD,
	TCGEN_IDENTITY,			// clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_FOG,
	TCGEN_VECTOR			// S and T from world coordinates
} texCoordGen_t;

typedef enum {
	ACFF_NONE,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
} acff_t;

typedef struct {
	genFunc_t	func;

	float base;
	float amplitude;
	float phase;
	float frequency;
} waveForm_t;

#define TR_MAX_TEXMODS 4

typedef enum {
	TMOD_NONE,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE
} texMod_t;

#define	MAX_SHADER_DEFORMS	3
typedef struct {
	deform_t	deformation;			// vertex coordinate modification type

	vec3_t		moveVector;
	waveForm_t	deformationWave;
	float		deformationSpread;

	float		bulgeWidth;
	float		bulgeHeight;
	float		bulgeSpeed;
} deformStage_t;


typedef struct {
	texMod_t		type;

	// used for TMOD_TURBULENT and TMOD_STRETCH
	waveForm_t		wave;

	// used for TMOD_TRANSFORM
	float			matrix[2][2];		// s' = s * m[0][0] + t * m[1][0] + trans[0]
	float			translate[2];		// t' = s * m[0][1] + t * m[0][1] + trans[1]

	// used for TMOD_SCALE
	float			scale[2];			// s *= scale[0]
	                                    // t *= scale[1]

	// used for TMOD_SCROLL
	float			scroll[2];			// s' = s + scroll[0] * time
										// t' = t + scroll[1] * time

	// + = clockwise
	// - = counterclockwise
	float			rotateSpeed;

} texModInfo_t;


#define	MAX_IMAGE_ANIMATIONS	8

typedef struct {
	image_t			*image[MAX_IMAGE_ANIMATIONS];
	int				numImageAnimations;
	float			imageAnimationSpeed;

	textureEnv_t	texEnv;

	samplerState_t	sampler;
		
	texCoordGen_t	tcGen;
	vec3_t			tcGenVectors[2];

	int				numTexMods;
	texModInfo_t	*texMods;

	int				videoMapHandle;
	qboolean		isLightmap;
	qboolean		vertexLightmap;
	qboolean		isVideoMap;
} textureBundle_t;

#define NUM_TEXTURE_BUNDLES 2

struct shaderCommands_s;

typedef stateGroup_t (*customStageFunc_t)(
	const struct shaderCommands_s *input,			
	int stage,										//the index of the stage that's rendering
	stateGroup_t sg,								//the previous state group
	qboolean restorePrior							//true to restore everything prior to the previous group,
													//	false to restore only what's current in it
	);												//return the state group used for the current render

typedef struct
{
	qboolean		active;
	
	textureBundle_t	bundle[NUM_TEXTURE_BUNDLES];
	union
	{
		image_t		*deluxeMap;
		image_t		*normalMap;
	};
	float			bumpDepth;

	waveForm_t		rgbWave;
	colorGen_t		rgbGen;

	waveForm_t		alphaWave;
	alphaGen_t		alphaGen;

	byte			constantColor[4];			// for CGEN_CONST and AGEN_CONST
	float			params[4];					// used by misc {RGB|A}GEN
	float			specCtrl[4];

	float			lineWidth;

	unsigned		stateBits;					// GLS_xxxx mask

	acff_t			adjustColorsForFog;

	qboolean		isDetail;
	
	customStageFunc_t	customDraw;
} shaderStage_t;

#define LIGHTMAP_2D			-4		// shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX	-3		// pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE	-2
#define	LIGHTMAP_NONE		-1

typedef enum {
	FP_NONE,		// surface is translucent and will just be adjusted properly
	FP_EQUAL,		// surface is opaque but possibly alpha tested
	FP_LE			// surface is trnaslucent, but still needs a fog pass (fog surface)
} fogPass_t;

typedef struct
{
	float			cloudHeight;
	GLenum			srcblend, dstblend;
	image_t			*cube;
	image_t			*outerbox[6];
	
	struct
	{
		image_t*	image;
		float		threshold;
		float		size;
		int			start,end;
	} stars[ 8 ];

	vec3_t			rotAxis;
	float			rot_base;
	float			rot_speed;

} skyParms_t;

typedef struct
{
	vec3_t			color;
	float			depthForOpaque;
} fogParms_t;

typedef struct
{
	float			pos;

	float			color[3];

	/*
		The distance and occlusion values used in the rest of the calculations
		are controlled by the following functions:

		Let: d = camera-space distance from the eye
		Let: v = the fraction of the test quad that *isn't* occluded
		Let: a = the cosine of the flare's view angle (1 = center screen)
		
		Then the effective occlusion factor O, distance D, and angle A are:

			   (1 - v) - occRange[0]
		O = ---------------------------
			 occRange[1] - occRange[0]


			       d - distRange[0]
		D = -----------------------------
			 distRange[1] - distRange[0]

			       a - angleRange[0]
		A = --------------------------------
			 angleRange[1] - angleRange[0]
		
	*/
	float			occRange[2];
	float			distRange[2];
	float			angleRange[2];

	/*
		This scales the screen-space quad based on the flare's distance from the
		camera. This is useful for "primary" flare objects such as the immediate
		corona around a light source.

		Let: D = scaled distance from the eye (see distRange)
		Let: a = sizeAtten

		Then image size scale S is calculated as:

							1
		S = ------------------------------
			 a[0] + a[1] * D + a[2] * D^2
	*/
	float			sizeAtten[3];

	/*
		The flare intensity I is calculated as follows:

		Let: D = scaled distance from the eye (see distRange)
		Let: O = the effective occlusion factor (see comment for occRange)
		Let: A = the effective view angle (see comment for angleRange
		Let: i = intAtten

							1								1								 1
		I = ------------------------------ * ------------------------------	* ------------------------------
			 i[0] + i[1] * O + i[2] * O^2	  i[3] + i[4] * D + i[5] * D^2     i[6] + i[7] * A + i[8] * A^2
	*/
	float			intAtten[9];

	qboolean		rotate;				// if true the quad turns such that the bottom of the
										// image always points to the center of the screen
	image_t			*img;
} flarePic_t;

typedef struct
{
	uint			numPics;
	flarePic_t		pics[8];
} flareParams_t;

typedef enum vertAttribs_t
{
	VA_NONE			= 0x0000,
	VA_POSITION		= 0x0001,				// always needed, here for completeness
	VA_NORMAL		= 0x0002,
	VA_TANGENT		= 0x0004,
	VA_BINORMAL		= 0x0008,
	VA_TEXCOORD0	= 0x0010,
	VA_TEXCOORD1	= 0x0020,
	VA_COLOR		= 0x0040,
} vertAttribs_t;

typedef struct shader_s
{
	char			name[MAX_QPATH];	// game path, including extension
	int				lightmapIndex;		// for a shader to match, both name and lightmapIndex must match

	int				index;				// this shader == tr.shaders[index]
	int				sortedIndex;		// this shader == tr.sortedShaders[sortedIndex]

	float			sort;				// lower numbered shaders draw before higher numbered

	qboolean		defaultShader;		// we want to return index 0 if the shader failed to
										// load for some reason, but R_FindShader should
										// still keep a name allocated for it, so if
										// something calls RE_RegisterShader again with
										// the same name, we don't try looking for it again

	qboolean		explicitlyDefined;	// found in a .shader file

	int				surfaceFlags;		// if explicitlyDefined, this will have SURF_* flags
	int				contentFlags;

	qboolean		entityMergable;		// merge across entites optimizable (smoke, blood)

	qboolean		isSky;
	skyParms_t		sky;
	fogParms_t		fogParms;

	flareParams_t	*flareParams;		// if not null this is a flare

	float			portalRange;		// distance to fog out at

	int				multitextureEnv;	// 0, GL_MODULATE, GL_ADD (FIXME: put in stage)

	cullType_t		cullType;			// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	int				polygonOffset;		// set for decals and other items that must be offset 
	qboolean		noMipMaps;			// for console fonts, 2D elements, etc.
	qboolean		noPicMip;			// for images that must always be full resolution

	char			special[32];

	fogPass_t		fogPass;			// draw a blended pass, possibly with depth test equals

	vertAttribs_t	neededAttribs;

	int				numDeforms;
	deformStage_t	deforms[MAX_SHADER_DEFORMS];

	int				numUnfoggedPasses;
	shaderStage_t	*stages[MAX_SHADER_STAGES];		

	void			(*optimalStageIteratorFunc)( void );

	float			clampTime;			// time this shader is clamped to
	float			timeOffset;			// current time offset for this shader

	struct shader_s	*remappedShader;	// current shader this one is remapped too

	struct shader_s	*next;
} shader_t;


// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct
{
	int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewaxis[3];		// transformation matrix

	float		dofStrength, dofDepth;

	int			time;				// time in milliseconds for shader effects and other time dependent rendering issues
	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];
	qboolean	areamaskModified;	// qtrue if areamask changed since last scene

	float		floatTime;			// tr.refdef.time / 1000.0

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int			num_entities;
	trRefEntity_t	*entities;

	int			num_dlights;
	struct dlight_s	*dlights;

	int			numPolys;
	struct srfPoly_s	*polys;

	int			numDrawSurfs;
	struct drawSurf_s	*drawSurfs;
} trRefdef_t;


//=================================================================================

// skins allow models to be retextured without modifying the model file
typedef struct {
	char		name[MAX_QPATH];
	shader_t	*shader;
} skinSurface_t;

typedef struct skin_s {
	char		name[MAX_QPATH];		// game path, including extension
	int			numSurfaces;
	skinSurface_t	*surfaces[MD3_MAX_SURFACES];
} skin_t;


typedef struct fog_t
{
	char		shader[MAX_QPATH];

	int			originalBrushNumber;
	vec3_t		bounds[2];

	unsigned	colorInt;				// in packed byte format
	float		tcScale;				// texture coordinate vector scales
	fogParms_t	parms;

	// for clipping distance in fog when outside
	int			hasSurface;
	float		surface[4];
} fog_t;

typedef struct {
	orientationr_t	or;
	orientationr_t	world;
	vec3_t		pvsOrigin;			// may be different than or.origin for portals
	qboolean	isPortal;			// true if this view is through a portal
	qboolean	isMirror;			// the portal is a mirror, invert the face culling
	int			frameSceneNum;		// copied from tr.frameSceneNum
	int			frameCount;			// copied from tr.frameCount
	cplane_t	portalPlane;		// clip anything behind this if mirroring
	int			viewportX, viewportY, viewportWidth, viewportHeight;
	float		fovX, fovY;
	float		projectionMatrix[16];
	cplane_t	frustum[4];
	vec3_t		visBounds[2];
	float		zNear, zFar;
} viewParms_t;


/*
==============================================================================

SURFACES

==============================================================================
*/

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
typedef enum
{
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_BSPEXMESH,
	SF_POLY,
	SF_MD3,
	SF_X42,
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

typedef struct drawSurf_s
{
	uint				sort;			// bit combination for fast compares
	surfaceType_t		*surface;		// any of surface*_t
} drawSurf_t;

#define	MAX_FACE_POINTS		64

#define	MAX_PATCH_SIZE		32			// max dimensions of a patch mesh in map file
#define	MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
typedef struct srfPoly_s {
	surfaceType_t	surfaceType;
	qhandle_t		hShader;
	int				fogIndex;
	int				numVerts;
	polyVert_t		*verts;
} srfPoly_t;

typedef struct srfFlare_t
{
	surfaceType_t	surfaceType;	//must be SF_FLARE
	vec3_t			origin;			//in entity local coords (will go through MODELVIEW)
	vec3_t			normal;			//currently ignored
	vec3_t			color;			//currently ignored
} srfFlare_t;

typedef struct srfGridMesh_t
{
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits[SMP_FRAMES];

	// culling information
	vec3_t			meshBounds[2];
	vec3_t			localOrigin;
	float			meshRadius;

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t			lodOrigin;
	float			lodRadius;
	int				lodFixed;
	int				lodStitched;

	// vertexes
	int				width, height;
	float			*widthLodError;
	float			*heightLodError;
	drawVert_t		verts[1];		// variable sized
} srfGridMesh_t;

typedef struct srfSurfaceFace_t
{
	surfaceType_t	surfaceType;
	cplane_t		plane;

	// dynamic lighting information
	int				dlightBits[SMP_FRAMES];

	// triangle definitions (no normals at points)
	int				numVerts;
	int				numIndices;

	drawVert_t		*verts;
	int				*indices;										// there is a variable length list of indices here also
} srfSurfaceFace_t;


// misc_models in maps are turned into direct geometry by q3map
typedef struct srfTriangles_t
{
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits[SMP_FRAMES];

	// culling information (FIXME: use this!)
	vec3_t			bounds[2];
	vec3_t			localOrigin;
	float			radius;

	// triangle definitions
	int				numIndices;
	int				*indexes;

	int				numVerts;
	drawVert_t		*verts;
} srfTriangles_t;


extern	void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void *);

/*
==============================================================================

x42 types

==============================================================================
*/

#include "tr_x42.h"

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

typedef struct msurface_ex_t
{
	surfaceType_t		surfType;

	int					viewCount;
	drawSurf_t			*ds;
	int					dlightBits[SMP_FRAMES];

	GLenum				primType;

	uint				numVerts;
	drawVert_ex_t		*verts;
	uint				numIndices;
	ushort				*indices;
} msurface_ex_t;

typedef struct msurface_s
{
	int					viewCount;		// if == tr.viewCount, already added

	struct shader_s		*shader;
	int					fogIndex;

	surfaceType_t		*data;			// any of srf*_t

	msurface_ex_t		*redirect;
} msurface_t;

#define	CONTENTS_NODE		-1
typedef struct mnode_t
{
	// common with leaf and node
	int			contents;		// -1 for nodes, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	vec3_t		mins, maxs;		// for bounding box culling
	struct mnode_t	*parent;

	// node specific
	cplane_t	*plane;
	struct mnode_t	*children[2];	

	// leaf specific
	int			cluster;
	int			area;

	int			*firstmarksurface;
	int			nummarksurfaces;
} mnode_t;

typedef struct bmodel_t
{
	vec3_t		bounds[2];		// for culling
	int			firstSurfaceNum;
	int			numSurfaces;
} bmodel_t;

typedef enum lightType_t
{
	LIGHT_NONE,
	LIGHT_GRID,
	LIGHT_TREE_16,
	LIGHT_TREE_32,
} lightType_t;

typedef struct world_t
{
	char		name[MAX_QPATH];		// ie: maps/tim_dm2.bsp
	char		baseName[MAX_QPATH];	// ie: tim_dm2

	int			dataSize;

	int			numShaders;
	dshader_t	*shaders;

	bmodel_t	*bmodels;

	int			numplanes;
	cplane_t	*planes;

	int			numnodes;		// includes leafs
	int			numDecisionNodes;
	mnode_t		*nodes;

	int				numsurfaces;
	msurface_t		*surfaces;

	int				nummarksurfaces;
	int				*marksurfaces;

	uint			numExSurfs;
	msurface_ex_t	*exSurfs;

	int				numfogs;
	fog_t			*fogs;

	vec3_t			lightGridOrigin;
	vec3_t			lightGridSize;
	vec3_t			lightGridInverseSize;
	int				lightGridBounds[3];

	lightType_t		lightGridType;
	void			*lightGridData0, *lightGridData1;

	qboolean		deluxeMapping;

	int				numClusters;
	int				clusterBytes;
	const byte		*vis;			// may be passed in by CM_LoadMap to save space

	byte			*novis;			// clusterBytes of 0xff

	char			*entityString;
	char			*entityParsePoint;

	float			fogColor[4];
} world_t;

//======================================================================

typedef enum {
	MOD_BAD,
	MOD_BRUSH,
	MOD_MESH,
	MOD_X42,
} modtype_t;

typedef struct model_t
{
	char		name[MAX_QPATH];
	modtype_t	type;
	int			index;				// model = tr.models[model->index]

	int			dataSize;			// just for listing purposes
	bmodel_t	*bmodel;			// only if type == MOD_BRUSH
	md3Header_t *md3[MD3_MAX_LODS];	// only if type == MOD_MESH
	x42Model_t	*x42;

	int			 numLods;
} model_t;


#define	MAX_MOD_KNOWN	1024

void		R_ModelInit (void);
model_t		*R_GetModelByHandle( qhandle_t hModel );
int			Q_EXTERNAL_CALL R_LerpTag( affine_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName );
void		Q_EXTERNAL_CALL R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

void		Q_EXTERNAL_CALL R_Modellist_f (void);

//====================================================
extern	refimport_t		ri;

#define	MAX_DRAWIMAGES			2048
#define	MAX_LIGHTMAPS			256
#define	MAX_SKINS				1024


#define	MAX_DRAWSURFS			0x10000
#define	DRAWSURF_MASK			(MAX_DRAWSURFS-1)

/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

21 - 31	: sorted shader index
11 - 20	: entity index
2 - 6	: fog index
//2		: used to be clipped flag REMOVED - 03.21.00 rad
0 - 1	: dlightmap index

	TTimo - 1.32
17-31 : sorted shader index
7-16  : entity index
2-6   : fog index
0-1   : dlightmap index
*/
#define	QSORT_SHADERNUM_SHIFT	17
#define QSORT_SHADERNUM_MASK	0xFFFE0000
#define	QSORT_ENTITYNUM_SHIFT	7
#define QSORT_ENTITYNUM_MASK	0x0001FF80
#define	QSORT_FOGNUM_SHIFT		2
#define QSORT_FOGNUM_MASK		0x0000007C
#define	QSORT_DLIGHT_SHIFT		0
#define QSORT_DLIGHT_MASK		0x00000003

extern	int			gl_filter_min, gl_filter_max;

/*
** performanceCounters_t
*/
typedef struct {
	int		c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int		c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int		c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int		c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int		c_leafs;
	int		c_dlightSurfaces;
	int		c_dlightSurfacesCulled;
} frontEndCounters_t;

#define	FOG_TABLE_SIZE		256
#define FUNCTABLE_SIZE		8192
#define FUNCTABLE_SIZE2		10
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)


// the renderer front end should never modify glstate_t
typedef struct
{
	qboolean	finishCalled;
} glstate_t;


typedef struct {
	int		c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	float	c_overDraw;
	
	int		c_dlightVertexes;
	int		c_dlightIndexes;

	int		c_flareAdds;
	int		c_flareTests;
	int		c_flareRenders;

	int		msec;			// total msec for backend run

	int		c_drawCalls;
} backEndCounters_t;

// all state modified by the back end is seperated
// from the front end state
typedef struct {
	int			smpFrame;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	orientationr_t	or;
	backEndCounters_t	pc;
	qboolean	isHyperspace;
	trRefEntity_t	*currentEntity;
	qboolean	skyRenderedThisView;	// flag for drawing sun

	qboolean	projection2D;	// if qtrue, drawstretchpic doesn't need to change modes
	byte		color2D[4];
	qboolean	vertexes2D;		// shader needs to be finished
	trRefEntity_t	entity2D;	// currentEntity will point at this when doing 2D rendering

	int frameViewCount;
} backEndState_t;

/*
** trGlobals_t 
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
typedef struct trGlobals_t
{
	qboolean				registered;		// cleared at shutdown, set at beginRegistration

	int						visCount;		// incremented every time a new vis cluster is entered
	int						frameCount;		// incremented every frame
	int						sceneCount;		// incremented every scene
	int						viewCount;		// incremented every view (twice a scene if portaled)
											// and every R_MarkFragments call

	int						smpFrame;		// toggles from 0 to 1 every endFrame

	int						frameSceneNum;	// zeroed at RE_BeginFrame

	qboolean				worldMapLoaded;
	world_t					*world;

	const byte				*externalVisData;	// from RE_SetWorldVisData, shared with CM_Load

	image_t					*defaultImage;
	image_t					*scratchImage[32];
	image_t					*fogImage;
	image_t					*dlightImage;	// inverse-quare highlight for projective adding
	image_t					*flareImage;
	image_t					*whiteImage;			// full of 0xFF
	image_t					*blackImage;			// full of 0x00
	image_t					*identityLightImage;	// full of tr.identityLightByte
	image_t					*defNormMap;			// the unit vector <0, 0, 1> scaled and biased into [0, 1]
	image_t					*imageNotFoundImage;

	shader_t				*defaultShader;
	shader_t				*whiteShader;
	shader_t				*shadowShader;
	shader_t				*orbitShader;

	shader_t				*sunShader;

	int						numLightmaps;
	image_t					*lightmaps[MAX_LIGHTMAPS];
	image_t					*deluxemaps[MAX_LIGHTMAPS];

	image_t					*lightGridDiffuseDirection;
	image_t					*lightGridDiffuseColor;
	image_t					*lightGridAmbientColor;

	trRefEntity_t			*currentEntity;
	trRefEntity_t			worldEntity;		// point currentEntity at this when rendering world
	int						currentEntityNum;
	int						shiftedEntityNum;	// currentEntityNum << QSORT_ENTITYNUM_SHIFT
	model_t					*currentModel;

	viewParms_t				viewParms;

	float					identityLight;		// 1.0 / ( 1 << overbrightBits )
	int						identityLightByte;	// identityLight * 255
	int						overbrightBits;		// r_overbrightBits->integer, but set to 0 if no hw gamma

	orientationr_t			or;					// for current entity

	trRefdef_t				refdef;

	int						viewCluster;

	vec3_t					sunLight;			// from the sky shader for this level
	vec3_t					sunDirection;

	frontEndCounters_t		pc;
	int						frontEndMsec;		// not in pc due to clearing issue

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t					*models[MAX_MOD_KNOWN];
	int						numModels;

	int						numImages;
	image_t					*images[MAX_DRAWIMAGES];

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int						numShaders;
	shader_t				*shaders[MAX_SHADERS];
	shader_t				*sortedShaders[MAX_SHADERS];

	int						numSkins;
	skin_t					*skins[MAX_SKINS];

	float					sinTable[FUNCTABLE_SIZE];
	float					squareTable[FUNCTABLE_SIZE];
	float					triangleTable[FUNCTABLE_SIZE];
	float					sawToothTable[FUNCTABLE_SIZE];
	float					inverseSawToothTable[FUNCTABLE_SIZE];
	float					fogTable[FOG_TABLE_SIZE];
} trGlobals_t;

extern backEndState_t	backEnd;
extern trGlobals_t	tr;
extern vidConfig_t	glConfig;		// outside of TR since it shouldn't be cleared during ref re-init
extern glstate_t	glState;		// outside of TR since it shouldn't be cleared during ref re-init


//
// cvars
//
extern cvar_t	*r_railWidth;
extern cvar_t	*r_railCoreWidth;
extern cvar_t	*r_railSegmentLength;

extern cvar_t	*r_ignore;				// used for debugging anything
extern cvar_t	*r_verbose;				// used for verbose debug spew
extern cvar_t	*r_ignoreFastPath;		// allows us to ignore our Tess fast paths

extern cvar_t	*r_znear;				// near Z clip plane

extern cvar_t	*r_stencilbits;			// number of desired stencil bits
extern cvar_t	*r_depthbits;			// number of desired depth bits
extern cvar_t	*r_colorbits;			// number of desired color bits, only relevant for fullscreen
extern cvar_t	*r_stereo;				// desired pixelformat stereo flag
extern cvar_t	*r_texturebits;			// number of desired texture bits
										// 0 = use framebuffer depth
										// 16 = use 16-bit textures
										// 32 = use 32-bit textures
										// all else = error

extern cvar_t	*r_measureOverdraw;		// enables stencil buffer overdraw measurement

extern cvar_t	*r_lodbias;				// push/pull LOD transitions
extern cvar_t	*r_lodscale;

extern cvar_t	*r_primitives;			// "0" = based on compiled vertex array existance
										// "1" = glDrawElemet tristrips
										// "2" = glDrawElements triangles
										// "-1" = no drawing

extern cvar_t	*r_inGameVideo;				// controls whether in game video should be draw
extern cvar_t	*r_fastsky;				// controls whether sky should be cleared or drawn
extern cvar_t	*r_drawSun;				// controls drawing of sun quad
extern cvar_t	*r_dynamiclight;		// dynamic lights enabled/disabled
extern cvar_t	*r_dlightBacks;			// dlight non-facing surfaces for continuity

extern	cvar_t	*r_norefresh;			// bypasses the ref rendering
extern	cvar_t	*r_drawentities;		// disable/enable entity rendering
extern	cvar_t	*r_drawworld;			// disable/enable world rendering
extern	cvar_t	*r_speeds;				// various levels of information display
extern  cvar_t	*r_detailTextures;		// enables/disables detail texturing stages
extern	cvar_t	*r_novis;				// disable/enable usage of PVS
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_facePlaneCull;		// enables culling of planar surfaces with back side test
extern	cvar_t	*r_nocurves;
extern	cvar_t	*r_showcluster;

extern cvar_t	*r_mode;				// video mode
extern cvar_t	*r_fsmode;
extern cvar_t	*r_fsmonitor;
extern cvar_t	*r_fullscreen;
extern cvar_t	*r_gamma;
extern cvar_t	*r_displayRefresh;		// optional display refresh option
extern cvar_t	*r_ignorehwgamma;		// overrides hardware gamma capabilities

extern cvar_t	*r_ext_compressed_textures;		// these control use of specific extensions
extern cvar_t	*r_ext_gamma_control;
extern cvar_t	*r_ext_texenv_op;
extern cvar_t	*r_ext_multitexture;
extern cvar_t	*r_ext_compiled_vertex_array;
extern cvar_t	*r_ext_texture_env_add;

extern	cvar_t	*r_nobind;						// turns off binding to appropriate textures
extern	cvar_t	*r_singleShader;				// make most world faces use default shader
extern	cvar_t	*r_roundImagesDown;
extern	cvar_t	*r_colorMipLevels;				// development aid to see texture mip usage
extern	cvar_t	*r_finish;
extern	cvar_t	*r_drawBuffer;
extern	cvar_t	*r_swapInterval;
extern	cvar_t	*r_textureMode;
extern	cvar_t	*r_textureAniso;
extern	cvar_t	*r_textureLod;
extern	cvar_t	*r_offsetFactor;
extern	cvar_t	*r_offsetUnits;

extern cvar_t	*r_x42cacheCtl;					// number of launch pad cache entries to maintain (not counting the base one)

extern	cvar_t	*r_fullbright;					// avoid lightmap pass
extern	cvar_t	*r_lightmap;					// render lightmaps only
extern	cvar_t	*r_deluxemap;					// set level of deluxe mapping
extern	cvar_t	*r_vertexLight;					// vertex lighting mode for better performance
extern	cvar_t	*r_uiFullScreen;				// ui is running fullscreen

extern	cvar_t	*r_logFile;						// number of frames to emit GL logs
extern	cvar_t	*r_showtris;					// enables wireframe rendering of the world
extern	cvar_t	*r_showsky;						// forces sky in front of all surfaces
extern	cvar_t	*r_shownormals;					// draws wireframe normals
extern	cvar_t	*r_clear;						// force screen clear every frame

extern	cvar_t	*r_flares;						// light flares

extern	cvar_t	*r_intensity;

extern	cvar_t	*r_lockpvs;
extern	cvar_t	*r_noportals;
extern	cvar_t	*r_portalOnly;

extern	cvar_t	*r_subdivisions;
extern	cvar_t	*r_lodCurveError;
extern	cvar_t	*r_smp;
extern	cvar_t	*r_showSmp;
extern	cvar_t	*r_skipBackEnd;

extern	cvar_t	*r_ignoreGLErrors;

extern	cvar_t	*r_overBrightBits;
extern	cvar_t	*r_mapOverBrightBits;

extern	cvar_t	*r_debugSurface;
extern	cvar_t	*r_debugAas;

extern	cvar_t	*r_clobberNPOTtex;

extern	cvar_t	*r_showImages;
extern	cvar_t	*r_debugSort;

extern	cvar_t	*r_printShaders;
extern	cvar_t	*r_saveFontData;

extern	cvar_t	*r_bloom;
extern	cvar_t	*r_bloomThresh;
extern	cvar_t	*r_bloomCombine;
extern	cvar_t	*r_depthOfField;
extern	cvar_t	*r_fsaa;
extern	cvar_t	*r_skin;

extern	cvar_t	*r_nosse;

#ifdef _DEBUG
extern	cvar_t	*r_logLevel;
#endif

//====================================================================

float R_NoiseGet4f( float x, float y, float z, float t );
void  R_NoiseInit( void );

void R_SwapBuffers( int );

void R_RenderView( viewParms_t *parms );

void R_AddMD3Surfaces( trRefEntity_t *e );
void R_AddNullModelSurfaces( trRefEntity_t *e );
void R_AddBeamSurfaces( trRefEntity_t *e );
void R_AddRailSurfaces( trRefEntity_t *e, qboolean isUnderwater );
void R_AddLightningBoltSurfaces( trRefEntity_t *e );

void R_AddPolygonSurfaces( void );

uint R_ComposeSort( int entityNum, shader_t *shader, int fogNum, int dlightMap );
void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap );

drawSurf_t* R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap );

void Q_EXTERNAL_CALL RE_MenuSurfBegin( int surfNum );
void Q_EXTERNAL_CALL RE_MenuSurfEnd( void );

#define	CULL_IN		0		// completely unclipped
#define	CULL_CLIP	1		// clipped by one or more planes
#define	CULL_OUT	2		// completely outside the clipping planes
void R_LocalNormalToWorld (vec3_t local, vec3_t world);
void R_LocalPointToWorld (const vec3_t local, vec3_t world);
int R_CullLocalBox (vec3_t bounds[2]);
int R_CullPointAndRadius( vec3_t origin, float radius );
int R_CullLocalPointAndRadius( const vec3_t origin, float radius );
float R_ProjectRadius( float r, vec3_t location );

void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *or );

/*
** GL wrapper/helper functions
*/
void GL_State( uint stateVector );

#define GLS_SRCBLEND_ZERO						0x00000001
#define GLS_SRCBLEND_ONE						0x00000002
#define GLS_SRCBLEND_DST_COLOR					0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR		0x00000004
#define GLS_SRCBLEND_SRC_ALPHA					0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		0x00000006
#define GLS_SRCBLEND_DST_ALPHA					0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE				0x00000009
#define		GLS_SRCBLEND_BITS					0x0000000f

#define GLS_DSTBLEND_ZERO						0x00000010
#define GLS_DSTBLEND_ONE						0x00000020
#define GLS_DSTBLEND_SRC_COLOR					0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		0x00000040
#define GLS_DSTBLEND_SRC_ALPHA					0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		0x00000060
#define GLS_DSTBLEND_DST_ALPHA					0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		0x00000080
#define		GLS_DSTBLEND_BITS					0x000000f0

#define GLS_DEPTHMASK_TRUE						0x00000100

#define GLS_POLYMODE_LINE						0x00001000

#define GLS_DEPTHTEST_DISABLE					0x00010000
#define GLS_DEPTHFUNC_EQUAL						0x00020000

#define GLS_ATEST_GT_0							0x10000000
#define GLS_ATEST_LT_80							0x20000000
#define GLS_ATEST_GE_80							0x40000000
#define		GLS_ATEST_BITS						0x70000000

#define GLS_DEFAULT			GLS_DEPTHMASK_TRUE

void	Q_EXTERNAL_CALL RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
void	Q_EXTERNAL_CALL RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

void		Q_EXTERNAL_CALL RE_BeginFrame( stereoFrame_t stereoFrame );
void		Q_EXTERNAL_CALL RE_BeginRegistration( vidConfig_t *glconfig );

void		Q_EXTERNAL_CALL RE_LoadWorldMap( const char *mapname );
void		Q_EXTERNAL_CALL RE_LoadWorldMapFast( void *pMapData, const char *name );
void Q_EXTERNAL_CALL RE_PreloadDDSImage( void *pImageData, const char *name );

void		Q_EXTERNAL_CALL RE_SetWorldVisData( const byte *vis );
qhandle_t	Q_EXTERNAL_CALL RE_RegisterModel( const char *name );
qhandle_t	Q_EXTERNAL_CALL RE_RegisterSkin( const char *name );
void		Q_EXTERNAL_CALL RE_Shutdown( qboolean destroyWindow );

qboolean	Q_EXTERNAL_CALL R_GetEntityToken( char *buffer, int size );

model_t		*R_AllocModel( void );

void    	R_Init( void );

typedef enum imageLoadFlags_t
{
	ILF_NONE				= 0x00000000,
	ILF_ALLOW_DEFAULT		= 0x00000001,
	ILF_VECTOR_SB3			= 0x00000002,	//image is a scaled biased vector image (normal map, etc) - mip appropriately
} imageLoadFlags_t;

void R_LoadImage( const char *name, byte **pic, int *width, int *height );
image_t* R_FindImageFile( const char *name, qboolean mipmap, imageLoadFlags_t flags );
qhandle_t Q_EXTERNAL_CALL RE_RegisterImage( const char *path, qboolean mipMap );

image_t *R_CreateImage( const char *name, const byte *pic, int width, int height, qboolean mipmap, imageLoadFlags_t flags );
qboolean	R_GetModeInfo( int *width, int *height, float *windowAspect, int mode );

void		R_SetColorMappings( void );
void		R_GammaCorrect( byte *buffer, int bufSize );

void	Q_EXTERNAL_CALL R_ImageList_f( void );
void	Q_EXTERNAL_CALL R_SkinList_f( void );
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=516
void RB_TakeScreenshotCmd( const void *data );
void	Q_EXTERNAL_CALL R_ScreenShot_f( void );

void	R_InitFogTable( void );
float	R_FogFactor( float s, float t );
void	R_InitImages( void );
void	R_DeleteTextures( void );
int		R_SumOfUsedImages( void );
void	R_InitSkins( void );
skin_t	*R_GetSkinByHandle( qhandle_t hSkin );

void R_PpInit( void );
void R_PpKill( void );

qboolean R_PpNeedsDepthRender( void );
void R_PpBeginDepthRender( void );
void R_PpEndDepthRender( void );

qboolean R_DofIsEnabled( void );

qboolean R_BloomIsEnabled( void );
void Q_EXTERNAL_CALL RE_PpDoPostProcess( void );

//
// tr_shader.c
//
qhandle_t		 RE_RegisterShaderLightMap( const char *name, int lightmapIndex );
qhandle_t		 Q_EXTERNAL_CALL RE_RegisterShader( const char *name );
qhandle_t		 Q_EXTERNAL_CALL RE_RegisterShaderNoMip( const char *name );

shader_t	*R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage );
shader_t	*R_GetShaderByHandle( qhandle_t hShader );
shader_t	*R_GetShaderByState( int index, long *cycleTime );
shader_t *R_FindShaderByName( const char *name );
void		Q_EXTERNAL_CALL R_InitShaders( void );
void		Q_EXTERNAL_CALL R_ShaderList_f( void );
void    R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);

qboolean Q_EXTERNAL_CALL RE_SetShaderValue( qhandle_t shader, const char *valName, effectParamClass_t paramClass,
						effectParamType_t baseType, unsigned int s0, unsigned int s1, effectParamOrder_t order, const void *value );
int Q_EXTERNAL_CALL RE_GetShaderVariations( qhandle_t shader );
qhandle_t Q_EXTERNAL_CALL RE_GetShaderVariation( qhandle_t shader, int varIdx );

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_Init( void );
void		Glimp_PrintSystemInfo( void );
void		GLimp_Shutdown( void );
void		GLimp_EndFrame( void );

qboolean	GLimp_SpawnRenderThread( void (*function)( void ) );
void		*GLimp_RendererSleep( void );
void		GLimp_FrontEndSleep( void );
void		GLimp_WakeRenderer( void *data );

extern bool	glimp_suspendRender;

#ifdef _DEBUG
void GLimp_LogComment( int level, char *comment, ... );
#else

#ifdef __GNUC__
#define GLimp_LogComment( ... )
#else
#define GLimp_LogComment
#endif

#endif

// NOTE TTimo linux works with float gamma value, not the gamma table
//   the params won't be used, getting the r_gamma cvar directly
void		GLimp_SetGamma( unsigned char red[256], 
						    unsigned char green[256],
							unsigned char blue[256] );


/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/
typedef byte color4ub_t[4];

typedef struct stageVars
{
	DECL_ALIGN_PRE( 16 ) color4ub_t			colors[SHADER_MAX_VERTEXES];
	DECL_ALIGN_PRE( 16 ) vec2_t				texcoords[NUM_TEXTURE_BUNDLES][SHADER_MAX_VERTEXES];
} stageVars_t;

typedef struct shaderCommands_s
{
	DECL_ALIGN_PRE( 16 ) vec4_t				xyz[SHADER_MAX_VERTEXES];
	
	DECL_ALIGN_PRE( 16 ) vec4_t				normal[SHADER_MAX_VERTEXES];
	DECL_ALIGN_PRE( 16 ) vec4_t				tangent[SHADER_MAX_VERTEXES];
	DECL_ALIGN_PRE( 16 ) vec4_t				binormal[SHADER_MAX_VERTEXES];
	
	DECL_ALIGN_PRE( 16 ) vec2_t				texCoords[SHADER_MAX_VERTEXES][2];
	
	DECL_ALIGN_PRE( 16 ) color4ub_t			vertexColors[SHADER_MAX_VERTEXES];
	
	DECL_ALIGN_PRE( 16 ) glIndex_t			indexes[SHADER_MAX_INDEXES];

	stageVars_t	svars;

	shader_t	*shader;
	float		shaderTime;
	int			fogNum;
	GLenum		primType;

	int			dlightBits;	// or together of all vertexDlightBits

	int			numIndexes;
	int			numVertexes;

	// info extracted from current shader
	int			numPasses;
	void		(*currentStageIteratorFunc)( void );
	shaderStage_t	**xstages;
} shaderCommands_t;

extern	shaderCommands_t	tess;

void RB_CheckSurface( shader_t *shader, int fogNum, GLenum primType );
void RB_BeginSurface( shader_t *shader, int fogNum, GLenum primType );
void RB_EndSurface( void );
void RB_CheckOverflow( int verts, int indexes );
#define RB_CHECKOVERFLOW(v,i) if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES ) {RB_CheckOverflow(v,i);}

void RB_StageIteratorGeneric( void );
void RB_StageIteratorSky( void );
void RB_StageIteratorVertexLitTexture( void );
void RB_StageIteratorLightmappedMultitexture( void );

bool R_ShadeSupportsDeluxeMapping( void );
void R_ShadeComputeOptimalGenericStageIteratorFuncs( shader_t *shader, shaderStage_t *stages );

void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte *color );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2, bool pointNormalsOutward );

void RB_ShowImages( void );


/*
============================================================

WORLD MAP

============================================================
*/

void R_AddBrushModelSurfaces( trRefEntity_t *e );
void R_AddWorldSurfaces( void );
qboolean Q_EXTERNAL_CALL R_inPVS( const vec3_t p1, const vec3_t p2 );


/*
============================================================

FLARES

============================================================
*/

void R_ClearFlares( void );

void RB_AddFlare( const vec3_t point, bool infinitelyFar, float size, shader_t *shader );
void RB_AddDlightFlares( void );
void RB_RenderFlares( void );

/*
============================================================

LIGHTS

============================================================
*/

void R_DlightBmodel( bmodel_t *bmodel );
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *or );
int Q_EXTERNAL_CALL R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );


/*
============================================================

SHADOWS

============================================================
*/

void RB_ShadowTessEnd( void );
void RB_ShadowFinish( void );
void RB_ProjectionShadowDeform( void );

/*
============================================================

SKIES

============================================================
*/

void R_InitSkyTexCoords( float cloudLayerHeight );

void RB_DrawSun( void );
void RB_DrawOrbit( void );

/*
============================================================

CURVE TESSELATION

============================================================
*/

#define PATCH_STITCHING

srfGridMesh_t *R_SubdividePatchToGrid( int width, int height,
								drawVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] );
srfGridMesh_t *R_GridInsertColumn( srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror );
srfGridMesh_t *R_GridInsertRow( srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror );
void R_FreeSurfaceGridMesh( srfGridMesh_t *grid );

/*
============================================================

MARKERS, POLYGON PROJECTION ON WORLD POLYGONS

============================================================
*/

int Q_EXTERNAL_CALL R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );


/*
============================================================

SCENE GENERATION

============================================================
*/

void R_ToggleSmpFrame( void );

void Q_EXTERNAL_CALL RE_ClearScene( void );
void Q_EXTERNAL_CALL RE_AddRefEntityToScene( const refEntity_t *ent );
void Q_EXTERNAL_CALL RE_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
void Q_EXTERNAL_CALL RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void Q_EXTERNAL_CALL RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void Q_EXTERNAL_CALL RE_RenderScene( const refdef_t *fd );

/*
=============================================================
=============================================================
*/
void	R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst );
void	R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window );

void	RB_DeformTessGeometry( void );

void	RB_CalcEnvironmentTexCoords( float *dstTexCoords );
void	RB_CalcFogTexCoords( float *dstTexCoords );
void	RB_CalcScrollTexCoords( const float scroll[2], float *dstTexCoords );
void	RB_CalcRotateTexCoords( float rotSpeed, float *dstTexCoords );
void	RB_CalcScaleTexCoords( const float scale[2], float *dstTexCoords );
void	RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *dstTexCoords );
void	RB_CalcTransformTexCoords( const texModInfo_t *tmi, float *dstTexCoords );
void	RB_CalcModulateColorsByFog( unsigned char *dstColors );
void	RB_CalcModulateAlphasByFog( unsigned char *dstColors );
void	RB_CalcModulateRGBAsByFog( unsigned char *dstColors );
void	RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors );
void	RB_CalcWaveColor( const waveForm_t *wf, unsigned char *dstColors );
void	RB_CalcAlphaFromEntity( unsigned char *dstColors );
void	RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors );
void	RB_CalcStretchTexCoords( const waveForm_t *wf, float *texCoords );
void	RB_CalcColorFromEntity( unsigned char *dstColors );
void	RB_CalcColorFromOneMinusEntity( unsigned char *dstColors );
void	RB_CalcSpecularAlpha( const shaderStage_t *pStage, unsigned char *alphas );
void	RB_CalcGlowHaloAlpha( const shaderStage_t *pStage, byte * RESTRICT alpha );
void	RB_CalcDiffuseColor( unsigned char *colors );
void	RB_CalcDiffuseColor2( const shaderStage_t *pStage, byte *color );

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

#define MAX_RENDER_COMMAND_MEM 0x80000
#define	MAX_RENDER_COMMANDS	0x2000

typedef void (*renderCommandFunc_t)( void *data );

typedef struct
{
	renderCommandFunc_t		func;
	void					*data;
} renderCommand_t;

typedef struct
{
	renderCommand_t			cmds[MAX_RENDER_COMMANDS];
	uint					cmdsUsed;
	byte					data[MAX_RENDER_COMMAND_MEM];
	uint					dataUsed;
} renderCommandList_t;

typedef struct {
	float	color[4];
} setColorCommand_t;

typedef struct {
	float	color[3];
} setColorRGBCommand_t;

typedef struct {
	int		buffer;
} drawBufferCommand_t;

typedef struct {
	image_t	*image;
	int		width;
	int		height;
	void	*data;
} subImageCommand_t;

typedef struct {
	int		buffer;
} endFrameCommand_t;

typedef struct {
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	drawSurf_t *drawSurfs;
	int		numDrawSurfs;
} drawSurfsCommand_t;

typedef struct {
	int x;
	int y;
	int width;
	int height;
	char *fileName;
} screenshotCommand_t;

// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define	MAX_POLYS		600
#define	MAX_POLYVERTS	3000

// all of the information needed by the back end must be
// contained in a backEndData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine
typedef struct {
	drawSurf_t	drawSurfs[MAX_DRAWSURFS];
	dlight_t	dlights[MAX_DLIGHTS];
	trRefEntity_t	entities[MAX_ENTITIES];
	srfPoly_t	*polys;//[MAX_POLYS];
	polyVert_t	*polyVerts;//[MAX_POLYVERTS];
	renderCommandList_t	commands;
} backEndData_t;

/*
=============================================================

RENDERER BACK END FUNCTIONS

=============================================================
*/

void RB_RenderThread( void );
void RB_ExecuteRenderCommands( renderCommandList_t *cmds );

void RB_SetGL2D( void );

//

extern	int		max_polys;
extern	int		max_polyverts;

extern	backEndData_t	*backEndData[SMP_FRAMES];	// the second one may not be allocated

extern	volatile renderCommandList_t	*renderCommandList;

extern	volatile qboolean	renderThreadActive;

void* R_GetCommandBuffer( renderCommandFunc_t cmd, uint bytes );
void* R_GetCommandMemory( uint bytes );

void RB_DrawSurfs( const void *data );
void RB_DrawBuffer( const void *data );
void RB_SwapBuffers( const void *data );

void R_InitCommandBuffers( void );
void R_ShutdownCommandBuffers( void );

void R_SyncRenderThread( void );

void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs );

void Q_EXTERNAL_CALL RE_SetColor( const float *rgba );
void Q_EXTERNAL_CALL RE_SetColorRGB( const float *rgb );
void Q_EXTERNAL_CALL RE_StretchPic ( float x, float y, float w, float h, float mat[2][3],
					  float s1, float t1, float s2, float t2, qhandle_t hShader );
void Q_EXTERNAL_CALL RE_StretchPicMulti( const stretchRect_t *rects, unsigned int numRects, qhandle_t hShader );
void Q_EXTERNAL_CALL RE_DrawPolys( const polyVert2D_t *verts, uint numVertsPerPoly, uint numPolys, qhandle_t hShader );
void Q_EXTERNAL_CALL RE_BeginFrame( stereoFrame_t stereoFrame );
void Q_EXTERNAL_CALL RE_EndFrame( int *frontEndMsec, int *backEndMsec );

// font stuff
void R_InitFreeType( void );
void R_DoneFreeType( void );
qhandle_t Q_EXTERNAL_CALL RE_RegisterFont(const char *fontName);
fontInfo_t * Q_EXTERNAL_CALL RE_GetFontFromFontSet( qhandle_t fontSet, float fontScale );
void Q_EXTERNAL_CALL RE_GetFonts( char * buffer, int size );

//text
void Q_EXTERNAL_CALL RE_TextDrawLine( const lineInfo_t * line, fontInfo_t * font, float italic, float x, float y,
	float fontScale, float letterScaleX, float letterScaleY, int smallcaps, int cursorLoc, int cursorChar, qboolean colors );
void Q_EXTERNAL_CALL RE_TextBeginBlock( unsigned int numChars );
void Q_EXTERNAL_CALL RE_TextEndBlock( void );

//shapes
curve_t* Q_EXTERNAL_CALL RE_CurveCreate( const vec2_t *pts, const vec4_t *colors, const linType_t *lins,
	linTexGen_t texGen, const vec3_t texGenMat[2], const vec3_t mtx[2], float tol, curvePlacement_t placement );
void Q_EXTERNAL_CALL RE_CurveDelete( curve_t *curve );
qhandle_t Q_EXTERNAL_CALL RE_ShapeCreate( const curve_t *curves, const shapeGen_t *genTypes, int numElems );
void Q_EXTERNAL_CALL RE_ShapeDraw( qhandle_t shape, qhandle_t shader, vec3_t m[ 2 ] );
void Q_EXTERNAL_CALL RE_ShapeDrawMulti( void **shapes, unsigned int numShapes, qhandle_t shader );

void GL_CheckErrors( void );

//shader programs
void R_SpInit();
void R_SpKill();

typedef enum
{
	SPV_DIFFUSE,
	SPV_SPEC,
	SPV_SPEC_NM,
	
	SPV_DELUXEMAP_NO_SPEC,
	SPV_DELUXEMAP,
	
	SPV_MAX_PROGRAMS,
} stdVertexProgs_t;

typedef enum
{
	SPF_BLOOMSELECT,
	SPF_BLOOMBLUR,
	SPF_BLOOMCOMBINE,

	SPF_DIFFUSE,
	SPF_SPEC,
	SPF_SPEC_NM,

	SPF_DELUXEMAP,

	SPF_MAX_PROGRAMS,
} stdFragmentProgs_t;

typedef enum
{
	VPF_NONE						= 0x00000000,
	VPF_POINSIZE					= 0x00000001,

	VPF_DISABLE						= 0x80000000,
} vpFlags_t;

typedef enum
{
	FPF_NONE						= 0x00000000,

	FPF_DISABLE						= 0x80000000,
} fpFlags_t;

bool R_SpIsStandardVertexProgramSupported( stdVertexProgs_t prog );
void R_SpSetStandardVertexProgram( stdVertexProgs_t prog );

bool R_SpIsStandardFragmentProgramSupported( stdFragmentProgs_t prog );
void R_SpSetStandardFragmentProgram( stdFragmentProgs_t prog );

void R_SpSetVertexProgram( GLuint prog, vpFlags_t flags );
void R_SpSetFragmentProgram( GLuint prog, fpFlags_t flags );

static ID_INLINE NOGLOBALALIAS void Vec2_Cpy( vec2_t o, const vec2_t v ) { o[0] = v[0]; o[1] = v[1]; };
static ID_INLINE NOGLOBALALIAS void Vec2_Scale( vec2_t o, const vec2_t v, float s ) { o[0] = v[0] * s; o[1] = v[1] * s; }
static ID_INLINE NOGLOBALALIAS void Vec2_Add( vec2_t o, const vec2_t a, const vec2_t b ) { o[0] = a[0] + b[0]; o[1] = a[1] + b[1]; }
static ID_INLINE NOGLOBALALIAS void Vec2_Sub( vec2_t o, const vec2_t a, const vec2_t b ) { o[0] = a[0] - b[0]; o[1] = a[1] - b[1]; }
static ID_INLINE NOGLOBALALIAS void Vec2_Mul( vec2_t o, const vec2_t a, const vec2_t b ) { o[0] = a[0] * b[0]; o[1] = a[1] * b[1]; }
static ID_INLINE NOGLOBALALIAS float Vec2_Dot( const vec2_t a, const vec2_t b ) { return a[0] * b[0] + a[1] * b[1]; }
static ID_INLINE NOGLOBALALIAS float Vec2_Len( const vec2_t v ) { return sqrtf( Vec2_Dot( v, v ) ); }
static ID_INLINE NOGLOBALALIAS float Vec2_Dist( const vec2_t a, const vec2_t b ) { vec2_t t; Vec2_Sub( t, a, b ); return Vec2_Len( t ); }
static ID_INLINE NOGLOBALALIAS void Vec2_Norm( vec2_t o, const vec2_t v ) { Vec2_Scale( o, v, 1.0F / Vec2_Len( v ) ); }

static ID_INLINE size_t Align( size_t s, size_t a ) { return s + ((a - (s % a)) % a); }


#endif //TR_LOCAL_H
