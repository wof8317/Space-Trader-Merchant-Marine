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

#include "tr_local.h"
#include "tr_state-local.h"

static samplerState_t	defaultSampler2D;
static samplerState_t	defaultSamplerRECT;

static struct
{
	image_t			*currTex[STATE_MAX_TEXTURE_UNITS];
	samplerState_t	samplers[STATE_MAX_TEXTURE_UNITS];

	bool			allowAniso;

	GLenum			currTmu;
	GLenum			currClTmu;

	uint			numTmus;
	GLfloat			maxAniso, defAniso;

	int				minLod;
} gls;

void R_StateSetActiveClientTmuUntracked( GLenum tmu )
{
	if( gls.currClTmu == tmu )
		return;

	glClientActiveTextureARB( tmu );
	gls.currClTmu = tmu;
}

void R_StateSetActiveTmuUntracked( GLenum tmu )
{
	if( gls.currTmu == tmu )
		return;

	glActiveTextureARB( tmu );
	gls.currTmu = tmu;
}

GLenum R_StateGetRectTarget( void )
{
	if( GLEW_ARB_texture_rectangle )
		return GL_TEXTURE_RECTANGLE_ARB;
	else if( GLEW_EXT_texture_rectangle )
		return GL_TEXTURE_RECTANGLE_EXT;
	else if( GLEW_NV_texture_rectangle )
		return GL_TEXTURE_RECTANGLE_NV;
	else
		return 0;
}

qboolean R_StateTargetIsRect( GLenum target )
{
	switch( target )
	{
	case GL_TEXTURE_1D:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_3D_EXT:
	case GL_TEXTURE_CUBE_MAP_ARB:
		return qfalse;

	case GL_TEXTURE_RECTANGLE_ARB:
	//case GL_TEXTURE_RECTANGLE_EXT: ( == GL_TEXTURE_RECTANGLE_ARB )
	//case GL_TEXTURE_RECTANGLE_NV:	( == GL_TEXTURE_RECTANGLE_ARB )
		return qtrue;
	}

	return qfalse;
}

static void R_StateSetTextureMinLodInternal( GLenum target, GLint minLod, GLenum tmu )
{
	GLint w, h;
	uint m, n;

	/*
		Yes, I'm calling it Lod. Yes, this should use GL_TEXTURE_MIN_LOD.

		But ATI hardware sucks at GL_TEXTURE_MIN_LOD. Turning it on for
		even one texture literally forces the application into software
		mode. So instead we invite do an almost equivalent thing with the
		GL_TEXTURE_BASE_LEVEL parameter.
	*/

	if( !GLEW_VERSION_1_2 && !GLEW_SGIS_texture_lod )
		return;

	R_StateSetActiveTmuUntracked( tmu );

	if( R_StateTargetIsRect( target ) )
		//no mips to control
		return;

	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_WIDTH, &w );
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_HEIGHT, &h );

	if( w < 0 || h < 0 )
		//what happened here?
		return;

	m = (w > h) ? w : h;

	//get the log2 of the max dimension
	n = 0;
	while( m >> n )
		n++;
	n--;

	//see if there actually is a mip level beyond the base level
	glGetTexLevelParameteriv( target, 1, GL_TEXTURE_WIDTH, &w );
	if( !w )
		return;

	//clamp the Lod value
	if( minLod > (int)n )
		minLod = n;
	if( minLod < 0 )
		minLod = 0;

	glTexParameteri( target, GL_TEXTURE_BASE_LEVEL_SGIS, minLod );
}

static void R_StateSetTextureSamplerInternal( samplerState_t t, GLenum tmu )
{
	bool isRect;
	const samplerState_t *d;

	uint idx = tmu - GL_TEXTURE0_ARB;
	image_t *img = gls.currTex[idx];

	gls.samplers[idx] = t;

	if( !img )
		return;

	R_StateSetActiveTmuUntracked( tmu );

	isRect = R_StateTargetIsRect( img->texTarget );
	d = isRect ? &defaultSamplerRECT : &defaultSampler2D;

	if( !t.minFilter ) t.minFilter = d->minFilter;
	if( !t.magFilter ) t.magFilter = d->magFilter;
	if( !t.wrapS ) t.wrapS = d->wrapS;
	if( !t.wrapT ) t.wrapT = d->wrapT;
	if( !t.wrapR ) t.wrapR = d->wrapR;
	if( !t.maxAniso ) t.maxAniso = d->maxAniso;

	if( !img->mipmap || isRect )
	{
		switch( t.minFilter )
		{
		case GL_LINEAR_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
			t.minFilter = GL_LINEAR;
			break;

		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_NEAREST_MIPMAP_NEAREST:
			t.minFilter = GL_NEAREST;
			break;
		}
	}

	if( isRect )
	{
		//impose remaining textureRECT limits		

		switch( t.wrapS )
		{
		case GL_REPEAT:
			t.wrapS = GL_CLAMP_TO_EDGE_EXT;
			break;
		}

		switch( t.wrapT )
		{
		case GL_REPEAT:
			t.wrapT = GL_CLAMP_TO_EDGE_EXT;
			break;
		}
	}

	//update the filter
	if( img->samplerState.minFilter != t.minFilter )
	{
		glTexParameteri( img->texTarget, GL_TEXTURE_MIN_FILTER, t.minFilter );
		img->samplerState.minFilter = t.minFilter;
	}

	if( img->samplerState.magFilter != t.magFilter )
	{
		glTexParameteri( img->texTarget, GL_TEXTURE_MAG_FILTER, t.magFilter );
		img->samplerState.magFilter = t.magFilter;
	}

	//update the address mode
	switch( img->texTarget )
	{
	case GL_TEXTURE_3D_EXT:
		if( t.wrapR != img->samplerState.wrapR )
		{
			glTexParameteri( img->texTarget, GL_TEXTURE_WRAP_R_EXT, t.wrapR );
			img->samplerState.wrapR = t.wrapR;
		}
		//fallthrough is intended

	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE_ARB:
	//case GL_TEXTURE_RECTANGLE_EXT: ( == GL_TEXTURE_RECTANGLE_ARB )
	//case GL_TEXTURE_RECTANGLE_NV:	( == GL_TEXTURE_RECTANGLE_ARB )
	case GL_TEXTURE_CUBE_MAP_ARB:
		if( t.wrapT != img->samplerState.wrapT )
		{
			glTexParameteri( img->texTarget, GL_TEXTURE_WRAP_T, t.wrapT );
			img->samplerState.wrapT = t.wrapT;
		}
		//fallthrough is intended

	case GL_TEXTURE_1D:
		if( t.wrapS != img->samplerState.wrapS )
		{
			glTexParameteri( img->texTarget, GL_TEXTURE_WRAP_S, t.wrapS );
			img->samplerState.wrapS = t.wrapS;
		}
		break;
	}

	if( img->mipmap && gls.allowAniso )
	{
		if( t.maxAniso != img->samplerState.maxAniso )
		{
			glTexParameterf( img->texTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, t.maxAniso );
			img->samplerState.maxAniso = t.maxAniso;
		}
	}
}

static void R_StateResetTextureSamplerInternal( GLenum tmu )
{
	const image_t *img = gls.currTex[tmu - GL_TEXTURE0_ARB];

	qboolean isRect;
	const samplerState_t *d;

	if( !img )
		return;

	isRect = R_StateTargetIsRect( img->texTarget );
	d = isRect ? &defaultSamplerRECT : &defaultSampler2D;
	
	R_StateSetTextureSamplerInternal( *d, tmu );
}

void R_StateSetDefaultTextureModesUntracked( GLenum target, qboolean mipmap, GLenum tmu )
{
	bool isRect = R_StateTargetIsRect( target );
	const samplerState_t *d = isRect ? &defaultSamplerRECT : &defaultSampler2D;
									 
	GLenum minFilter = d->minFilter;

	R_StateSetActiveTmuUntracked( tmu );

	if( !mipmap )
	{
		switch( minFilter )
		{
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_LINEAR:
			minFilter = GL_LINEAR;
			break;

		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
			minFilter = GL_NEAREST;
			break;
		}
	}

	glTexParameteri( target, GL_TEXTURE_MIN_FILTER, minFilter );
	glTexParameteri( target, GL_TEXTURE_MAG_FILTER, d->magFilter );
	
	switch( target )
	{
	case GL_TEXTURE_3D_EXT:
		glTexParameteri( target, GL_TEXTURE_WRAP_R_EXT, d->wrapR );
		//fallthrough is intended

	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE_ARB:
	//case GL_TEXTURE_RECTANGLE_EXT: ( == GL_TEXTURE_RECTANGLE_ARB )
	//case GL_TEXTURE_RECTANGLE_NV:	( == GL_TEXTURE_RECTANGLE_ARB )
	case GL_TEXTURE_CUBE_MAP_ARB:
		glTexParameteri( target, GL_TEXTURE_WRAP_T, d->wrapT );
		//fallthrough is intended

	case GL_TEXTURE_1D:
		glTexParameteri( target, GL_TEXTURE_WRAP_S, d->wrapS );
		break;
	}

	R_StateSetTextureMinLodInternal( target, gls.minLod, tmu );
}

static void R_StateSetTextureInternal( image_t *image, GLenum tmu )
{
	qboolean resetMatrix = qfalse;
	uint idx = tmu - GL_TEXTURE0_ARB;
	image_t **img = gls.currTex + idx;

	R_StateSetActiveTmuUntracked( tmu );

	if( !image && *img )
	{
		glBindTexture( (*img)->texTarget, 0 );
		glDisable( (*img)->texTarget );
	}
	else
	{
		if( *img == image )
			return;

		if( *img )
		{
			if( (*img)->texTarget != image->texTarget )
			{
				glDisable( (*img)->texTarget );
				glEnable( image->texTarget );
			}
						
			resetMatrix = (*img)->addrMode != image->addrMode ||
				((*img)->addrMode == TAM_Dimensions && ((*img)->uploadWidth != image->uploadWidth ||
				(*img)->uploadHeight != image->uploadHeight));
		}
		else
		{
			glEnable( image->texTarget );
			resetMatrix = qtrue;
		}
	}

	if( resetMatrix )
	{
		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();

		if( image && image->addrMode == TAM_Dimensions )
			glScalef( (float)image->uploadWidth, (float)image->uploadHeight, (float)image->uploadDepth );

		R_StateCountIncrement( CSN_TexMat0 + idx );
	}

	if( image )
	{
		image->frameUsed = tr.frameCount;
	
		if( !*img || (*img)->texnum != image->texnum )
		{
			glBindTexture( image->texTarget, image->texnum );

			*img = image;
			R_StateSetTextureSamplerInternal( gls.samplers[idx], tmu );
		}
	}
	else
		*img = NULL;
}

static void R_StateSetTextureInternalRaw( GLuint tex, GLenum target, qboolean mipmap, GLenum tmu )
{
	static int dummyflip[STATE_MAX_TEXTURE_UNITS];
	static image_t dummies[STATE_MAX_TEXTURE_UNITS][2];

	int idx = tmu - GL_TEXTURE0_ARB;
	image_t *di = dummies[idx] + dummyflip[idx];

	Com_Memset( di, 0, sizeof( image_t ) );

	di->addrMode = TAM_Normalized;
	di->texTarget = target;
	di->texnum = tex;
	di->mipmap = mipmap;

	dummyflip[idx] = 1 - dummyflip[idx];

	R_StateSetTextureInternal( di, tmu );
}

static struct
{
	const char		*name;
	GLenum			min2D;
	GLenum			minRECT;
	GLenum			mag;
} modes[] =
{
	{ "Nearest", GL_NEAREST, GL_NEAREST, GL_NEAREST },
	{ "Linear", GL_LINEAR, GL_LINEAR, GL_LINEAR },
	{ "NearestMipNearest", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_NEAREST },
	{ "LinearMipNearest", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, GL_LINEAR },
	{ "NearestMipLinear", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST, GL_NEAREST },
	{ "LinearMipLinear", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_LINEAR },
	{ "AnisoMipLinear", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_LINEAR },
};

void R_StateSetTextureModeCvar( const char *string )
{
	int i;
	
	for( i = 0; i < lengthof( modes ); i++ )
		if ( Q_stricmp( modes[i].name, string ) == 0 )
			break;

	if( i == lengthof( modes ) )
	{
		ri.Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	defaultSampler2D.magFilter = modes[i].mag;
	defaultSampler2D.minFilter = modes[i].min2D;
	defaultSamplerRECT.magFilter = modes[i].mag;
	defaultSamplerRECT.minFilter = modes[i].minRECT;

	//values apply as of next R_StateSetTexture or R_StateSetTextureSampler
}

void R_StateSetTextureAnisotropyCvar( int aniso )
{
	if( aniso < 1 )
		aniso = 1;
	if( aniso > gls.maxAniso )
		aniso = gls.maxAniso;

	gls.defAniso = aniso;
	defaultSampler2D.maxAniso = aniso;

	//values apply as of next R_StateSetTexture or R_StateSetTextureSampler
}

void R_StateSetTextureMinLodCvar( int minLod )
{
	int i;

	if( minLod < 0 ) minLod = 0;
	if( minLod > 1000 ) minLod = 1000;

	gls.minLod = minLod;

	//change all the existing texture objects
	for( i = 0; i < tr.numImages; i++ )
	{
		image_t	*glt = tr.images[ i ];
		if( glt->mipmap )
		{
			R_StateSetTexture( glt, GL_TEXTURE0_ARB );
			R_StateSetTextureMinLodInternal( glt->texTarget, minLod, GL_TEXTURE0_ARB );
		}
	}
}

/*
	Setters and resetters go here.
*/

void R_StateSetTexture( image_t *image, GLenum tmu )
{
	trackedStateName_t state = (trackedStateName_t)(TSN_Texture0 + (tmu - GL_TEXTURE0_ARB));

	if( !image )
		R_StateSetDefault( state );
	else
	{
		R_StateSetTextureInternal( image, tmu );
		R_StateChanged( state );
	}
}

void R_StateSetTextureRaw( GLuint tex, GLenum target, qboolean mipmap, GLenum tmu )
{
	trackedStateName_t state = (trackedStateName_t)(TSN_Texture0 + (tmu - GL_TEXTURE0_ARB));

	if( !tex )
		R_StateSetDefault( state );
	else
	{
		R_StateSetTextureInternalRaw( tex, target, mipmap, tmu );
		R_StateChanged( state );
	}
}

static void R_StateResetTexture( trackedState_t *s )
{
	R_StateSetTextureInternal( NULL, s->data.asglenum );
}

void R_StateSetTextureSampler( const samplerState_t *sampler, GLenum tmu )
{
	R_StateSetTextureSamplerInternal( *sampler, tmu );
}

static void R_StateResetTextureSampler( trackedState_t *s )
{
	R_StateResetTextureSamplerInternal( s->data.asglenum );
}

void R_StateSetTextureEnvMode( GLenum env, GLenum tmu )
{
	trackedStateName_t state = (trackedStateName_t)(TSN_TexEnvMode0 + (tmu - GL_TEXTURE0_ARB));

	switch( env )
	{
	case 0:
	case GL_MODULATE:
		R_StateSetDefault( state );
		return;

	case GL_REPLACE:
	case GL_DECAL:
	case GL_BLEND:
	case GL_ADD:
		R_StateSetActiveTmuUntracked( tmu );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, env );
		break;

	case GL_COMBINE_ARB:
		ri.Error( ERR_DROP, "R_StateSetTextureEnvMode: cannot set GL_COMBINE_ARB through this function, use R_StateSetTextureEnvMode2\n" );

	default:
		ri.Error( ERR_DROP, "R_StateSetTextureEnvMode: invalid env '%d' passed\n", env );
		break;
	}

	R_StateChanged( state );
}

void R_StateSetTextureEnvMode2( const textureEnv_t *env, GLenum tmu )
{
	trackedStateName_t state = (trackedStateName_t)(TSN_TexEnvMode0 + (tmu - GL_TEXTURE0_ARB));

	bool setColor;

	if( !env->complex )
	{
		R_StateSetTextureEnvMode( env->color_mode, tmu );
		return;
	}

	R_StateSetActiveTmuUntracked( tmu );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
	
	setColor = false;

	glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, env->color_mode );

	//set the first source
	if( env->src[0].color_src == GL_CONSTANT_ARB )
		setColor = true;
	glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, env->src[0].color_src );
	glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, env->src[0].color_op );

	if( env->color_mode != GL_REPLACE )
	{
		//set second color source
		if( env->src[1].color_src == GL_CONSTANT_ARB )
			setColor = true;
		glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, env->src[1].color_src );
		glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, env->src[1].color_op );

		if( env->color_mode == GL_INTERPOLATE_ARB )
		{
			//set the third color source
			if( env->src[2].color_src == GL_CONSTANT_ARB )
				setColor = true;
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, env->src[2].color_src );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, env->src[2].color_op );
		}
	}

	if( env->color_mode != GL_DOT3_RGBA_ARB ) //alpha is irrelevant in this case
	{
		glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, env->alpha_mode );

		if( env->src[0].alpha_src == GL_CONSTANT_ARB )
			setColor = true;
		glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, env->src[0].alpha_src );
		glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, env->src[0].alpha_op );

		if( env->alpha_mode != GL_REPLACE )
		{
			//set second alpha source
			if( env->src[1].alpha_src == GL_CONSTANT_ARB )
				setColor = true;
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, env->src[1].alpha_src );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, env->src[1].alpha_op );

			if( env->alpha_mode == GL_INTERPOLATE_ARB )
			{
				if( env->src[2].alpha_src == GL_CONSTANT_ARB )
					setColor = true;
				glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, env->src[2].alpha_src );
				glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, env->src[2].alpha_op );
			}
		}
	}

	glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, env->color_scale );
	glTexEnvf( GL_TEXTURE_ENV, GL_ALPHA_SCALE, env->alpha_scale );

#undef resolve_def

	if( setColor )
		glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env->color );

	R_StateChanged( state );
}

static void R_StateResetTextureEnvMode( trackedState_t *s )
{
	R_StateSetActiveTmuUntracked( s->data.asglenum );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
}

void R_StateResolveTextureEnv( textureEnv_t *env )
{
	env->complex = false;

#define resolve_def( v, d ) { if( !env->v ) env->v = d; if( env->v != d ) env->complex = true; }

	resolve_def( color_mode, GL_MODULATE );
	resolve_def( alpha_mode, GL_MODULATE );

	resolve_def( src[0].color_src, GL_TEXTURE );
	resolve_def( src[0].color_op, GL_SRC_COLOR );
	resolve_def( src[0].alpha_src, GL_TEXTURE );
	resolve_def( src[0].alpha_op, GL_SRC_ALPHA );

	resolve_def( src[1].color_src, GL_PREVIOUS_ARB );
	resolve_def( src[1].color_op, GL_SRC_COLOR );
	resolve_def( src[1].alpha_src, GL_PREVIOUS_ARB );
	resolve_def( src[1].alpha_op, GL_SRC_ALPHA );

	resolve_def( src[2].color_src, GL_CONSTANT_ARB );
	resolve_def( src[2].color_op, GL_SRC_ALPHA );
	resolve_def( src[2].alpha_src, GL_CONSTANT_ARB );
	resolve_def( src[2].alpha_op, GL_SRC_ALPHA );

	resolve_def( color_scale, 1.0F );
	resolve_def( alpha_scale, 1.0F );
#undef resolve_def
}

void R_StateSetTextureMatrixCountedRaw( const float *mat, GLenum tmu )
{
	uint idx = tmu - GL_TEXTURE0_ARB;

	R_StateSetActiveTmuUntracked( tmu );
	glMatrixMode( GL_TEXTURE );
	glLoadMatrixf( mat );

	R_StateCountIncrement( CSN_TexMat0 + idx );
}

void R_StateSetTextureMatrixCountedAffine( const affine_t *mat, GLenum tmu )
{
	float m[16];
	uint idx = tmu - GL_TEXTURE0_ARB;

	Matrix_SetFromAffine( m, mat );

	R_StateSetActiveTmuUntracked( tmu );
	glMatrixMode( GL_TEXTURE );
	glLoadMatrixf( m );

	R_StateCountIncrement( CSN_TexMat0 + idx );
}

void R_StateMulTextureMatrixCountedRaw( const float *mat, GLenum tmu )
{
	uint idx = tmu - GL_TEXTURE0_ARB;

	R_StateSetActiveTmuUntracked( tmu );
	glMatrixMode( GL_TEXTURE );
	glMultMatrixf( mat );

	R_StateCountIncrement( CSN_TexMat0 + idx );
}

void R_StateMulTextureMatrixCountedAffine( const affine_t *mat, GLenum tmu )
{
	float m[16];
	uint idx = tmu - GL_TEXTURE0_ARB;

	Matrix_SetFromAffine( m, mat );

	R_StateSetActiveTmuUntracked( tmu );
	glMatrixMode( GL_TEXTURE );
	glMultMatrixf( m );

	R_StateCountIncrement( CSN_TexMat0 + idx );
}

uint R_StateGetNumTextureUnits( void )
{
	return gls.numTmus;
}

void R_StateSetupTextureStates( void )
{
	//init the bookkeeping data
	memset( &gls, 0, sizeof( gls ) );
	gls.currTmu = GL_TEXTURE0_ARB;

	defaultSampler2D.minFilter = GL_LINEAR_MIPMAP_LINEAR;
	defaultSampler2D.magFilter = GL_LINEAR;
	defaultSampler2D.wrapS = GL_REPEAT;
	defaultSampler2D.wrapT = GL_REPEAT;
	defaultSampler2D.wrapR = GL_REPEAT;
	defaultSampler2D.maxAniso = 1;

	defaultSamplerRECT.minFilter = GL_LINEAR;
	defaultSamplerRECT.magFilter = GL_LINEAR;
	defaultSamplerRECT.wrapS = GL_CLAMP_TO_EDGE_EXT;
	defaultSamplerRECT.wrapT = GL_CLAMP_TO_EDGE_EXT;
	defaultSamplerRECT.wrapR = GL_CLAMP_TO_EDGE_EXT;
	defaultSamplerRECT.maxAniso = 1;

	gls.allowAniso = false;
	if( GLEW_EXT_texture_filter_anisotropic )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gls.maxAniso );

		if( gls.maxAniso > 1 )
		{
			gls.allowAniso = true;

			if( gls.maxAniso > 8 )
				gls.defAniso = gls.maxAniso / 2;
			else
				gls.defAniso = gls.maxAniso;
		}
	}

	gls.minLod = 0;

	//set up the tracked states
	{
		uint i;
		GLuint numTmus;
		trackedState_t *s;

#define DefineTrackedState( _name, _reset )					\
	{														\
		s = track.s + (int)(_name);							\
		memset( s, 0, sizeof( *s ) );						\
		s->name = (trackedStateName_t)(_name);				\
		s->reset = _reset;									\
	}

		glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, (GLint*)&numTmus );

		if( numTmus > STATE_MAX_TEXTURE_UNITS )
			numTmus = STATE_MAX_TEXTURE_UNITS;

		gls.numTmus = numTmus;

#define DefineTrackedTextureStateI( _name, _reset )														\
	{																									\
		DefineTrackedState( _name + i, _reset );														\
		s->data.asglenum = GL_TEXTURE0_ARB + i;															\
	}

		for( i = 0; i < numTmus; i++ )
		{
			DefineTrackedTextureStateI( TSN_Texture0, R_StateResetTexture );
			DefineTrackedTextureStateI( TSN_TexSampler0, R_StateResetTextureSampler );
			DefineTrackedTextureStateI( TSN_TexEnvMode0, R_StateResetTextureEnvMode );
		}

		for( i = numTmus; i < STATE_MAX_TEXTURE_UNITS; i++ )
		{
			DefineTrackedTextureStateI( TSN_Texture0, NULL );
			DefineTrackedTextureStateI( TSN_TexSampler0, NULL );
			DefineTrackedTextureStateI( TSN_TexEnvMode0, NULL );
		}
#undef DefineTrackedTextureStateI

#undef DefineTrackedState_GLEnable
#undef DefineTrackedState_GLDisable
#undef DefineTrackedState
	}

	//set up the counted states
	{
		uint i;
		countedState_t *s;

#define DefineCountedState( _name )							\
	{														\
		s = count.s + (int)(_name);							\
		memset( s, 0, sizeof( *s ) );						\
		s->name = (_name);									\
	}

		for( i = CSN_TextureFirstState; i <= CSN_TextureLastState; i++ )
			DefineCountedState( i );

#undef DefineCountedState
	}
}