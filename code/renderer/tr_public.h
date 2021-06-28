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
#ifndef __TR_PUBLIC_H
#define __TR_PUBLIC_H

#include "tr_types.h"

#define	REF_API_VERSION		10

#define MAX_MENU_SURFACES 8

typedef struct
{
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void			(Q_EXTERNAL_CALL *Shutdown)( qboolean destroyWindow );

	/*
		All data that will be used in a level should be
		registered before rendering any frames to prevent disk hits,
		but they can still be registered at a later time
		if necessary.
		
		BeginRegistration makes any existing media pointers invalid
		and returns the current gl configuration, including screen width
		and height, which can be used by the client to intelligently
		size display elements
	*/
	void			(Q_EXTERNAL_CALL *BeginRegistration)( vidConfig_t *config );
	qhandle_t		(Q_EXTERNAL_CALL *RegisterModel)( const char *name );
	qhandle_t		(Q_EXTERNAL_CALL *RegisterSkin)( const char *name );
	qhandle_t		(Q_EXTERNAL_CALL *RegisterImage)( const char *path, qboolean mipMap );
	qhandle_t		(Q_EXTERNAL_CALL *RegisterShader)( const char *name );
	qhandle_t		(Q_EXTERNAL_CALL *RegisterShaderNoMip)( const char *name );
	void			(Q_EXTERNAL_CALL *LoadWorld)( const char *name );

	void			(Q_EXTERNAL_CALL *LoadWorldFast)( void *pMapData, const char *name );
	void			(Q_EXTERNAL_CALL *PreloadDDSImage)( void *pImageData, const char *name );

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void			(Q_EXTERNAL_CALL *SetWorldVisData)( const byte *vis );

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void			(Q_EXTERNAL_CALL *EndRegistration)( void );

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void			(Q_EXTERNAL_CALL *ClearScene)( void );
	/*
		Allocates a new pose from per-frame memory and fills it out with a model's pose data.

		Note: if framesLerp is *exactly* 0, the time and offset info for frame 1 is ignored and should be set zero'd out.
		Likewise if framesLerp is *exaclty* 1, the time and offset info for frame 0 is ignored and should be all zeros.

		In either case there must always be at least one group frame. The first group frame must *always* be for group zero.
	*/
	qhandle_t		(Q_EXTERNAL_CALL *BuildPose)( qhandle_t h_mod,
						const animGroupFrame_t *groupFrames0, uint numGroupFrames0, const boneOffset_t *boneOffsets0, uint numBoneOffsets0,
						const animGroupFrame_t *groupFrames1, uint numGroupFrames1, const boneOffset_t *boneOffsets1, uint numBoneOffsets1,
						const animGroupTransition_t *frameLerps, uint numFrameLerps );
	qboolean		(Q_EXTERNAL_CALL *LerpTagFromPose)( affine_t *tag, qhandle_t mod, qhandle_t pose, const char *tagName );

	void			(Q_EXTERNAL_CALL *AddRefEntityToScene)( const refEntity_t *re );
	void			(Q_EXTERNAL_CALL *AddPolyToScene)( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
	int				(Q_EXTERNAL_CALL *LightForPoint)( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
	void			(Q_EXTERNAL_CALL *AddLightToScene)( const vec3_t org, float intensity, float r, float g, float b );
	void			(Q_EXTERNAL_CALL *AddAdditiveLightToScene)( const vec3_t org, float intensity, float r, float g, float b );
	void			(Q_EXTERNAL_CALL *RenderScene)( const refdef_t *fd );

	void			(Q_EXTERNAL_CALL *SetColor)( const float *rgba );	// NULL = 1,1,1,1
	
	// Draw images for cinematic rendering, pass as 32 bit rgba
	void			(Q_EXTERNAL_CALL *DrawStretchRaw)(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
	void			(Q_EXTERNAL_CALL *UploadCinematic)(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

	void			(Q_EXTERNAL_CALL *BeginFrame)( stereoFrame_t stereoFrame );

	// if the pointers are not NULL, timing info will be returned
	void			(Q_EXTERNAL_CALL *EndFrame)( int *frontEndMsec, int *backEndMsec );


	int				(Q_EXTERNAL_CALL *MarkFragments)( int numPoints, const vec3_t *points, const vec3_t projection,
						int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

	int				(Q_EXTERNAL_CALL *LerpTag)( affine_t *tag,  qhandle_t model, int startFrame, int endFrame, 
						float frac, const char *tagName );
	void			(Q_EXTERNAL_CALL *ModelBounds)( qhandle_t model, vec3_t mins, vec3_t maxs );

	qhandle_t		(Q_EXTERNAL_CALL *RegisterFont)(const char *fontName );
	fontInfo_t*		(Q_EXTERNAL_CALL *GetFontFromFontSet)( qhandle_t fontSet, float fontScale );
	void			(Q_EXTERNAL_CALL *GetFonts)( char * buffer, int size );
	qboolean		(Q_EXTERNAL_CALL *GetEntityToken)( char *buffer, int size );
	qboolean		(Q_EXTERNAL_CALL *inPVS)( const vec3_t p1, const vec3_t p2 );

	/*
		Post processing.

		Call this method to apply bloom and other post processing effects to the scene.
	*/
	void			(Q_EXTERNAL_CALL *PostProcess)( void );

	/*
		2D onto 3D surface rendering.
		
		To render such a surface you must:
			-	Create a target face in your map.
				-	The target face must be a brush face.
				-	It must be rectangular (four sides, right angle corners).
				-	It must not be part of a mover.
				-	It must not be horizontal.
				-	The bottom edge must be parallell to the XY-plane.
				-	It must be a single face, two triangles that make a rectangle will *NOT* work.
				-	NOTE: Anyone that wants to relax these requirements should go read R_GetDestQuad in
					tr_backend.cpp. Adjusting the returned up, right, and forward vectors should be all
					you need to do.
			-	Pass a refEntity_t into render that lies within 64 units of the surface and has its skinNum
				field set to the index of the menu surface you want to render into this rectangle. This index
				must be in the range [0, MAX_MENU_SURFACES).
			-	Tell STRender what to draw:
				1.	Call MenuBeginSurf with the given index.
				2.	Call the 2D drawing commands, passing coordinates in 2D screen space (the same way you do
					any other 2D drawing.
					-	STRender will map the screen coordinates into the face.
					-	Don't use shaders that mess with depth buffering options.
					-	Do *NOT* try to draw the map or any 3D models here! The rule is that if you have to set
						up a refEntity_t to draw something, you can't draw it in this view.
				3.	Call MenuEndSurf.
			-	Have all of the above done *before* calling RenderScene.

		I'm using the word "menu" because that's what this was developed for,
		though I may rename it to something more generic in the future.
	*/
	void			(Q_EXTERNAL_CALL *MenuBeginSurf)( int surfNum );
	void			(Q_EXTERNAL_CALL *MenuEndSurf)( void );

	/*
		Rectangle drawing API.

		For complex shapes, please use the 2D Curve Shape API (below).

		This is the best way to draw a single quad or a list of quads that can't be compiled
		into a shape (as is the case for dynamically computed quads). However, multi-pass shaders
		don't work too well with DrawStretchPicMulti if the rects overlap.
	*/
	void			(Q_EXTERNAL_CALL *DrawStretchPic)( float x, float y, float w, float h, 
						float mat[2][3], float s1, float t1, float s2, float t2, qhandle_t hShader /* 0 for solid color */ );
	void			(Q_EXTERNAL_CALL *DrawStretchPicMulti)( const stretchRect_t *rects, unsigned int numRects,
						qhandle_t hShader /* 0 for solid color */ );

	void			(Q_EXTERNAL_CALL *DrawPolys)( const polyVert2D_t *verts, uint numVertsPerPoly,
						uint numPolys, qhandle_t hShader );
	/*
		Text drawing API.

		To just draw a line or two of text just call TextDrawLine.

		If you're going to draw many lines all at once use the text block API:

		1.	Call TextBeginBlock with the number of characters you are about to send in through TextDrawLine.
			-	Character count includes the cursor character.
			-	It is safe to ask for more characters than you plan to send in, just don't go overboard with
				giant character counts as this will force the GL to create an over-large VBO for the text which
				will be kept around (driver optimization).
			-	It is an error to send more than numChars characters in a block.
		2.	Call TextDrawLine repeatadely.
			-	You may use SetColor to change the font color.
			-	Do not call *any* other render functions until you end the block.
		3.	Call TextEndBlock to send the text down to the GL.

		Note that using multiple fonts or a font that splits its characters across multiple textures will
		negate much of the performance gain offered by this method as it will force an extra draw call each
		time the font texture changes and this will be *VERY* slow.
	*/
	void			(Q_EXTERNAL_CALL *TextBeginBlock)( unsigned int numChars );
	void			(Q_EXTERNAL_CALL *TextEndBlock)( void );
	void			(Q_EXTERNAL_CALL *TextDrawLine)( const lineInfo_t * line, fontInfo_t * font, float italic, float x, float y,
						float fontScale, float letterScaleX, float letterScaleY, int smallcaps, int cursorLoc, int cursorChar, qboolean colors );

	/*
		2D curve shapes API.

		1a.	Generate curves using CurveCreate.
			-	If you want to modify the curves you may change any of the element pointers in the returned struct
				to point off into your own memory but *you* are responsible for ensuring that there are actually numPts
				entries in your array and for freeing the array.
			-	If you want to remove a set of optinal curve data (color or uv), set the corresponding array pointer to
				NULL, defaults (all ones for color or all zeros for uv) will be submitted to the GL at render time.
			-	Deleting the shape will still free all of the arrays that were created with it, whether you've stomped on
				the pointers or not so don't bother saving and restoring them.
			-	Feel free to alter any of the data returned by this call *except* the placement field.
		1b.	OR fill out a curve_t struct yourself (in this case you don't need to set the placement field, but please don't
			try passing it to CurveDelete either.
		2.	Generate a shape by passing an array of curves and generation optios to ShapeCreate.
			- No need to delete shapes, they go when the map changes.
		3.	When you're done making all your shapes, delete the curves using CurveDelete (but feel free to keep any you might
			need to use again - that's the point of caching them seperately from the shapes.
		4.	Draw the shape by passing the returned handle to ShapeDraw or ShapeDrawMulti.
			-	Please, please, FOR THE LOVE OF GOD use ShapeDrawMulti if you can. It's *much* nicer to the GL than many calls
				to ShapeDraw as it only sets the shader once. Note, however, that multi-pass shaders won't work well with
				ShapeDrawMulti if the shapes overlap since each pass is run once over the whole bunch of shapes.
	*/
	curve_t*		(Q_EXTERNAL_CALL *CurveCreate)(
		const vec2_t *pts,					//control points (one to start plus one per straight segment or three per bez3 segment)
		const vec4_t *colors,				//colors (one to start plus one per segment)
		const linType_t *lins,				//segment types, must end with LIN_END
		linTexGen_t texGen,					//texcoord generation options
		const vec3_t texGenMat[2],			//if texGen == LINTEX_PLANAR, is a 2x3 matrix applied to <x, y, 1> to get texcoord
		const vec3_t mtx[2],				//if !null is multiplied into each point (texGen is calculated before this)
		float tol,							//curve tesselation tolerance
		curvePlacement_t placement			//memory chunk to put it in
		);
	void			(Q_EXTERNAL_CALL *CurveDelete)( curve_t *curve ); //delete an existing curve, unless it's on an auto-free allocator
	qhandle_t		(Q_EXTERNAL_CALL *ShapeCreate)( const curve_t *curves, const shapeGen_t *genTypes, int numShapes );
	void			(Q_EXTERNAL_CALL *ShapeDraw)(qhandle_t shape, qhandle_t shader /* 0 for solid color */, vec3_t m[ 2 ] );
	void			(Q_EXTERNAL_CALL *ShapeDrawMulti)( void **shapes, unsigned int numShapes, qhandle_t shader /* 0 for solid color */ );

	/*
		Use EffectRegisterGlobalParam to set up a global effect parameter. Any effects that load after
		the parameter is registered will automatically link to it. If an effect that references a global
		parameter loads before the parameter is registered, it will create the parameter and link to it.

		If an effect registers a global parameter and you attempt to register a parameter with the same name
		but with a different type this function will return NULL. If you attempt to register a paramater who's
		name conflicts with a standard semantic this function will return NULL.

		If you register a global parameter and an effect attempts to link agains it with a different type the
		link will fail, the effect will use its default value, and a warning will be printed to the console.

		A type is defined by a base type (float, int, or bool), a class( scalar, vector, matrix, array),
		and one or two (depending on the class) size values. A mismatch in any of the above will cause a
		parameter link to fail.

		Don't screw up the types!

		Parameter size rules are determined by the parameter class:

			Scalar			Set both s0 and s1 to zero. Parameter has one element.
			Vector			Set s0 to the vector length. Valid values range in [1, 4]. Parameter has s0 elements.
			Matrix			Set s0 to the number of rows and s1 to the number of columns. Valid values range in [1, 4]. Parameter has s0 * s1 elements.
			Array			Set s0 to the array length. Parameter has s0 elements.
			Object			Not supported for global params-will cause the funciton to fail.

		Note that a scalar, a vector with one element, a matrix with one row and one column, and a one element array are considered
		*distinct* types. You can't treat one as the other! The same applies for other combinations with the same number of elements.

		Effects can not bind unsized arrays to global parameters.

		Parameter values are set via EffectSetGlobalParam. You must pass this function a pointer to an array of the parameter's base type.
		The number of elements in the array must exactly match the parameter's size. Data may be either row or column major (you specify which).

		Names are case insensitive.

		If you try to register more than the maximum number of global parameters (128) this method will return NULL.

		Note that global parameters don't have a default value and don't revert to any prior value after being set. Unless you know exactly
		which shaders reference your paramater you must keep the value current and meaningful at all times.
	*/
	qhandle_t		(Q_EXTERNAL_CALL *EffectRegisterGlobalParam)( const char *name, effectParamClass_t paramClass, effectParamType_t baseType, unsigned int s0, unsigned int s1 );
	void			(Q_EXTERNAL_CALL *EffectSetGlobalParam)( qhandle_t param, effectParamOrder_t ordering, uint numVals, const void *vals );
	
	// 
	const char*		(Q_EXTERNAL_CALL *DumpGetImagesString)( const char *pre, const char *post, const char *fmt );
} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct
{
	// print message on the local console
	void	(Q_EXTERNAL_CALL_VA *Printf)( int printLevel, const char *fmt, ...);

	// abort the game
	void	(Q_EXTERNAL_CALL_VA *Error)( int errorLevel, const char *fmt, ...);

	// milliseconds should only be used for profiling, never
	// for anything game related.  Get time from the refdef
	int		(Q_EXTERNAL_CALL *Milliseconds)( void );

	// stack based memory allocation for per-level things that
	// won't be freed
#ifdef HUNK_DEBUG
	void*	(Q_EXTERNAL_CALL *Hunk_AllocDebug)( int size, ha_pref pref, char *label, char *file, int line );
#else
	void*	(Q_EXTERNAL_CALL *Hunk_Alloc)( int size, ha_pref pref );
#endif
	void*	(Q_EXTERNAL_CALL *Hunk_AllocateTempMemory)( int size );
	void	(Q_EXTERNAL_CALL *Hunk_FreeTempMemory)( void *block );

	void*	(Q_EXTERNAL_CALL *Hunk_AllocateFrameMemory)( size_t size );

	// dynamic memory allocator for things that need to be freed
	void*	(Q_EXTERNAL_CALL *Malloc)( int bytes );
	void	(Q_EXTERNAL_CALL *Free)( void *buf );

	cvar_t*	(Q_EXTERNAL_CALL *Cvar_Get)( const char *name, const char *value, int flags );
	void	(Q_EXTERNAL_CALL *Cvar_Set)( const char *name, const char *value );

	void	(Q_EXTERNAL_CALL *Cmd_AddCommand)( const char *name, void (Q_EXTERNAL_CALL *cmd)( void ) );
	void	(Q_EXTERNAL_CALL *Cmd_RemoveCommand)( const char *name );

	int		(Q_EXTERNAL_CALL *Cmd_Argc) (void);
	char*	(Q_EXTERNAL_CALL *Cmd_Argv) (int i);

	void	(Q_EXTERNAL_CALL *Cmd_ExecuteText) (int exec_when, const char *text);

	// visualization for debugging collision detection
	void	(Q_EXTERNAL_CALL *CM_DrawDebugSurface)( void (Q_EXTERNAL_CALL *drawPoly)( int color, int numPoints, float *points ) );
	// visualization for debugging aas files
	void	(Q_EXTERNAL_CALL *AAS_DrawDebugSurface)( void (Q_EXTERNAL_CALL *drawPoly)( int color, int numPoints, float *points ) );

	char **	(Q_EXTERNAL_CALL *FS_ListFiles)( const char *name, const char *extension, int *numfilesfound );
	void	(Q_EXTERNAL_CALL *FS_FreeFileList)( char **filelist );

	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int		(Q_EXTERNAL_CALL *FS_ReadFile)( const char *name, void **buf );
	void	(Q_EXTERNAL_CALL *FS_FreeFile)( void *buf );

	void	(Q_EXTERNAL_CALL *FS_WriteFile)( const char *qpath, const void *buffer, int size );

	int		(Q_EXTERNAL_CALL *FS_FOpenFile)( const char *qpath, fileHandle_t *file, fsMode_t mode );
	int		(Q_EXTERNAL_CALL *FS_Read)( void *buffer, int len, fileHandle_t f );
	int 	(Q_EXTERNAL_CALL *FS_Write)( const void *buffer, int len, fileHandle_t h );
	int		(Q_EXTERNAL_CALL *FS_Seek)( fileHandle_t f, long offset, int origin );
	void	(Q_EXTERNAL_CALL *FS_FCloseFile)( fileHandle_t f );

	qboolean (Q_EXTERNAL_CALL *FS_FileExists)( const char *file );

	// cinematic stuff
	void	(Q_EXTERNAL_CALL *CIN_UploadCinematic)(int handle);
	int		(Q_EXTERNAL_CALL *CIN_PlayCinematic)( const char *arg0, int xpos, int ypos, int width, int height, int bits);
	e_status (Q_EXTERNAL_CALL *CIN_RunCinematic) (int handle);

	byte*	(Q_EXTERNAL_CALL *CM_ClusterPVS)( int cluster );
	void*	(Q_EXTERNAL_CALL *PlatformGetVars)( void );
	qboolean (Q_EXTERNAL_CALL *Sys_LowPhysicalMemory)( void );
	const char * (Q_EXTERNAL_CALL *Con_GetText)( int console );
} refimport_t;

int Q_EXTERNAL_CALL RE_Startup( int apiVersion, refexport_t *out, const refimport_t *in );

#endif	// __TR_PUBLIC_H
