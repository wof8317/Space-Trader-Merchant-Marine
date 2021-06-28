/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#ifndef __TR_TYPES_H
#define __TR_TYPES_H


#define	MAX_DLIGHTS		32			// can't be increased, because bit flags are used on surfaces
#define	MAX_ENTITIES	1023		// can't be increased without changing drawsurf bit packing

// renderfx flags
#define	RF_MINLIGHT			0x00000001		// allways have some light (viewmodel, some items)
#define	RF_THIRD_PERSON		0x00000002		// don't draw through eyes, only mirrors (player bodies, chat sprites)
#define	RF_FIRST_PERSON		0x00000004		// only draw through eyes (view weapon, damage blood blob)
#define	RF_DEPTHHACK		0x00000008		// for view weapon Z crunching
#define RF_NO_CULL			0x00000010		// assume it's always visible
#define	RF_NOSHADOW			0x00000020		// don't add stencil shadows

#define RF_LIGHTING_ORIGIN	0x00000040		// use refEntity->lightingOrigin instead of refEntity->origin
											// for lighting.  This allows entities to sink into the floor
											// with their origin going solid, and allows all parts of a
											// player to get the same lighting

#define	RF_SHADOW_PLANE		0x00000080		// use refEntity->shadowPlane

//applies only to animated MD3 and X42 models
#define	RF_WRAP_FRAMES		0x01000000		// mod the model frames by the maxframes to allow continuous
											// animation without needing to know the frame count

//applies only to RT_MODEL
#define RF_ORBIT_LINE		0x01000000		// use GL lines rather than strips

// refdef flags
#define RDF_NOWORLDMODEL	1		// used for player configuration screen

#define RDF_SOLARLIGHT		2		// we're drawing out in space, white directional sun light, no ambient light
									// treat refEntity->lightingOrigin (if RF_LIGHTING_ORIGIN) as the sun direction
									// or assume the sun is at the origin

#define RDF_HYPERSPACE		4		// teleportation effect

typedef struct
{
	vec3_t		xyz;
	float		st[2];
	byte		modulate[4];
} polyVert_t;

typedef struct
{
	vec2_t		xy;
	float		st[2];
	byte		modulate[4];
} polyVert2D_t;

typedef struct poly_s {
	qhandle_t			hShader;
	int					numVerts;
	polyVert_t			*verts;
} poly_t;

typedef enum
{
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,

	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_RINGS,
	RT_LIGHTNING,

	RT_SKYCUBE,
	RT_ORBIT,
	RT_FLARE,
	
	RT_PORTALSURFACE,		// doesn't draw anything, just info for portals
	RT_MENUSURFACE,

	RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

typedef struct animGroupFrame_t
{
	uint			animGroup;

	int				frame0, frame1;
	float			interp;
	
	qboolean		wrapFrames;
} animGroupFrame_t;

typedef struct boneOffset_t
{
	char			boneName[MAX_QPATH];

	quat_t			rot;
	vec3_t			pos;
	vec3_t			scale;
} boneOffset_t;

typedef struct animGroupTransition_t
{
	uint			animGroup;
	float			interp;
} animGroupTransition_t;

typedef struct
{
	uint					numBones;
	affine_t				*boneMats;
	uint					numTags;
	affine_t				*tagMats;
} x42Pose_t;

typedef struct {
	refEntityType_t	reType;
	int			renderfx;

	qhandle_t	hModel;				// opaque type outside refresh

	// most recent data
	vec3_t		lightingOrigin;		// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float		shadowPlane;		// projection shadows go here, stencils go slightly lower

	vec3_t		axis[3];			// rotation vectors
	qboolean	nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
	float		origin[3];			// also used as MODEL_BEAM's "from"
	int			frame;				// also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	float		oldorigin[3];		// also used as MODEL_BEAM's "to"
	int			oldframe;
	float		backlerp;			// 0.0 = current, 1.0 = old

	qhandle_t	poseData;			// only used for x42 models

	// texturing
	int			skinNum;			// inline skin index
	qhandle_t	customSkin;			// NULL for default skin
	qhandle_t	customShader;		// use one image for the entire thing

	// misc
	byte		shaderRGBA[4];		// colors used by rgbgen entity shaders
	float		shaderTexCoord[2];	// texture coordinates used by tcMod entity modifiers
	float		shaderTime;			// subtracted from refdef time to control effect start times

	// extra sprite information
	float		radius;
	float		rotation;

	//extra orbits info
	float		width;
} refEntity_t;


#define	MAX_RENDER_STRINGS			8
#define	MAX_RENDER_STRING_LENGTH	32

typedef struct
{
	int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewaxis[3];		// transformation matrix

	float		dofStrength, dofDepth;

	// time in milliseconds for shader effects and other time dependent rendering issues
	int			time;

	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
} refdef_t;


typedef enum {
	STEREO_CENTER,
	STEREO_LEFT,
	STEREO_RIGHT
} stereoFrame_t;


/*
** vidConfig_t
**
** Contains variables specific to the video driver configuration
** being run right now.  These are constant once the video driver
** subsystem is initialized.
*/

typedef struct
{
	char					renderer_string[MAX_STRING_CHARS];
	char					vendor_string[MAX_STRING_CHARS];
	char					version_string[MAX_STRING_CHARS];
	char					extensions_string[BIG_INFO_STRING];

	int						maxTextureSize;			// queried from GL
	int						maxActiveTextures;		// multitexture ability

	int						colorBits, depthBits, stencilBits, fsaaSamples;

	qboolean				deviceSupportsGamma;
	qboolean				textureEnvAddAvailable;

	int						vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float					windowAspect;
	float					xscale,yscale,xbias;

	int						displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
	// used CDS.
	qboolean				isFullscreen;
	qboolean				stereoEnabled;
	qboolean				smpActive;
} vidConfig_t;


//
// font support 
//
#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPHS_PER_FONT GLYPH_END - GLYPH_START + 1
typedef struct {
	int			height;			// number of scan lines
	int			top;			// top of glyph in buffer
	int			bottom;			// bottom of glyph in buffer
	int			pitch;			// width for copying
	int			xSkip;			// x adjustment
	int			imageWidth;		// width of actual image
	int			imageHeight;	// height of actual image
	float		s;				// x offset in image where glyph starts
	float		t;				// y offset in image where glyph starts
	float		s2;
	float		t2;
	qhandle_t	glyph;			// handle to the shader with the glyph
} glyphInfo_t;

typedef struct {
	glyphInfo_t	glyphs [GLYPHS_PER_FONT];
	float		glyphScale;
	int			pointSize;
} fontInfo_t;

typedef struct lineInfo_t
{
	const char* text;	// text
	int			count;	// number of characters
	float		sa;		// offset per white space
	float		ox;		// ofset from left bounds
	float		width;	// width of line
	float		height;	// height of line

	float		startColor[4];
	float		endColor[4];
	float		defaultColor[4];
} lineInfo_t;




//
// curve shapes
//
typedef enum
{
	LIN_END,
	LIN_LINE,
	LIN_BEZ3,
} linType_t;

typedef enum
{
	LINTEX_NONE,
	LINTEX_LINEAR,
	LINTEX_PLANAR,
} linTexGen_t;

typedef enum
{
	CVPLACE_HEAP,
	CVPLACE_HUNK,
	CVPLACE_HUNKTEMP,
	CVPLACE_FRAME,
} curvePlacement_t;

typedef struct
{
	int					numPts;
	vec2_t				*pts;
	vec2_t				*uvs;
	vec4_t				*colors;

	int					numIndices;
	short				*indices;

	curvePlacement_t	placement;
} curve_t;

typedef enum
{
	SHAPEGEN_NONE				= 0x00000000,

	SHAPEGEN_FILL				= 0x00000001,
	SHAPEGEN_FILL_USE_INDICES	= 0x00000002 | SHAPEGEN_FILL,

	SHAPEGEN_LINE				= 0x00000010,
	SHAPEGEN_LINE_LOOP			= 0x00000020 | SHAPEGEN_LINE,

	SHAPEGEN_FILL_AND_OUTLINE = SHAPEGEN_FILL | SHAPEGEN_LINE_LOOP,
} shapeGen_t;

//
// For drawing multiple rectangles at once
//

typedef struct
{
	vec2_t	pos, size;
	vec2_t	tc0, tc1;
	vec4_t	color;
} stretchRect_t;

//
//  Custom semantics
//

typedef enum
{
	PARAMCLASS_SCALAR,
	PARAMCLASS_VECTOR,
	PARAMCLASS_MATRIX,
	PARAMCLASS_ARRAY,
	PARAMCLASS_OBJECT,
} effectParamClass_t;

typedef enum
{
	PARAMTYPE_FLOAT,
	PARAMTYPE_INTEGER,
	PARAMTYPE_BOOLEAN, //actually an *int*, but clamped to 0/1
	PARAMTYPE_TEXTURE,
} effectParamType_t;

typedef enum
{
	PARAMORDER_ROWMAJOR,
	PARAMORDER_COLUMNMAJOR,
} effectParamOrder_t;

typedef enum
{
	TEXT_ALIGN_LEFT = 0,
	TEXT_ALIGN_CENTER = 1,
	TEXT_ALIGN_RIGHT = 2,
	TEXT_ALIGN_JUSTIFY = 3,

	TEXT_ALIGN_NOCLIP = 0x0080,
} textAlign_e;

typedef enum
{
	TEXT_STYLE_SHADOWED = 2,
	TEXT_STYLE_OUTLINED = 4,
	TEXT_STYLE_BLINK = 8,
	TEXT_STYLE_ITALIC = 16,

} textStyle_e;




#if defined( Q3_VM ) || defined( _WIN32 )

#define OPENGL_DRIVER_NAME	"opengl32"

#elif defined( MACOS_X )

#define OPENGL_DRIVER_NAME	"/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"

#else

#define OPENGL_DRIVER_NAME	"libGL.so.1"

#endif	// !defined _WIN32

#endif	// __TR_TYPES_H
