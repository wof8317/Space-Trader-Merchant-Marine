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

static struct
{
	GLenum			blendFunc[2];
	qboolean		depthTest;
	GLenum			depthFunc;
	float			depthRange[2];
	GLenum			visFace;
} sds;

void R_StateSetDefaultDepthTest( qboolean val )
{
	if( val == sds.depthTest )
		return;

	sds.depthTest = val;
	R_StateForceReset( TSN_DepthTest );
}

void R_StateSetDefaultDepthFunc( GLenum val )
{
	if( val == sds.depthFunc )
		return;

	sds.depthFunc = val;
	R_StateForceReset( TSN_DepthFunc );
}

void R_StateSetDepthTest( qboolean depthTest )
{
	if( depthTest == sds.depthTest )
		R_StateSetDefault( TSN_DepthTest );
	else if( depthTest )
	{
		glEnable( GL_DEPTH_TEST );
		R_StateChanged( TSN_DepthTest );
	}
	else
	{
		glDisable( GL_DEPTH_TEST );
		R_StateChanged( TSN_DepthTest );
	}
}

static void R_StateResetDepthTest( trackedState_t *s )
{
	if( sds.depthTest )
		glEnable( GL_DEPTH_TEST );
	else
		glDisable( GL_DEPTH_TEST );
}

void R_StateSetDepthFunc( GLenum func )
{
	if( func == sds.depthFunc )
		R_StateSetDefault( TSN_DepthFunc );
	else
	{
		glDepthFunc( func );
		R_StateChanged( TSN_DepthFunc );
	}
}

static void R_StateResetDepthFunc( trackedState_t *s )
{
	glDepthFunc( sds.depthFunc );
}

void R_StateSetDepthMask( qboolean mask )
{
	if( mask )
		R_StateSetDefault( TSN_DepthMask );
	else
	{
		glDepthMask( GL_FALSE );
		R_StateChanged( TSN_DepthMask );
	}
}

static void R_StateResetDepthMask( trackedState_t *s )
{
	glDepthMask( GL_TRUE );
}

void R_StateSetDefaultDepthRange( float n, float f )
{
	if( n == sds.depthRange[0] && f == sds.depthRange[1] )
		return;

	sds.depthRange[0] = n;
	sds.depthRange[1] = f;

	R_StateForceReset( TSN_DepthRange );
}

void R_StateSetDepthRange( float n, float f )
{
	if( n == sds.depthRange[0] && f == sds.depthRange[1] )
		R_StateSetDefault( TSN_DepthRange );
	else
	{
		glDepthRange( n, f );
		R_StateChanged( TSN_DepthRange );
	}
}

static void R_StateResetDepthRange( trackedState_t *s )
{
	glDepthRange( sds.depthRange[0], sds.depthRange[1] );
}

void R_StateSetDefaultBlend( GLenum src, GLenum dst )
{
	if( src == sds.blendFunc[0] && dst == sds.blendFunc[1] )
		return;

	sds.blendFunc[0] = src;
	sds.blendFunc[1] = dst;

	R_StateForceReset( TSN_Blend );
}

void R_StateSetBlend( GLenum src, GLenum dst )
{
	if( src == sds.blendFunc[0] && dst == sds.blendFunc[1] )
		R_StateSetDefault( TSN_Blend );
	else
	{
		if( src == GL_ONE && dst == GL_ZERO )
			glDisable( GL_BLEND );
		else
		{
			glEnable( GL_BLEND );
			glBlendFunc( src, dst );
		}

		R_StateChanged( TSN_Blend );
	}
}

static void R_StateResetBlend( trackedState_t *s )
{
	if( sds.blendFunc[0] == GL_ONE && sds.blendFunc[1] == GL_ZERO )
		glDisable( GL_BLEND );
	else
	{
		glEnable( GL_BLEND );
		glBlendFunc( sds.blendFunc[0], sds.blendFunc[1] );
	}
}

static void R_StateResetGLEnable( trackedState_t *s )
{
	glEnable( s->data.asglenum );
}

static void R_StateResetGLDisable( trackedState_t *s )
{
	glDisable( s->data.asglenum );
}

void R_StateSetAlphaTest( GLenum func, float ref )
{
	if( func == GL_ALWAYS )
		R_StateForceRestoreState( TSN_AlphaTest );
	else
	{
		glEnable( GL_ALPHA_TEST );
		glAlphaFunc( func, ref );

		R_StateChanged( TSN_AlphaTest );
	}
}

//AlphaTest - resets via R_StateResetGLDisable

void R_StateSetDefaultCull( GLenum visFace )
{
	if( sds.visFace == visFace )
		return;

	sds.visFace = visFace;
	R_StateForceReset( TSN_Cull );
}

void R_StateSetCull( GLenum visFace )
{
	if( visFace == sds.visFace )
		R_StateSetDefault( TSN_Cull );
	else
	{
		switch( visFace )
		{
		case GL_FRONT:
			glEnable( GL_CULL_FACE );
			glCullFace( GL_BACK );
			break;

		case GL_BACK:
			glEnable( GL_CULL_FACE );
			glCullFace( GL_FRONT );
			break;

		case GL_FRONT_AND_BACK:
			glDisable( GL_CULL_FACE );
			break;

		NO_DEFAULT;
		}

		R_StateChanged( TSN_Cull );
	}
}

static void R_StateResetCull( trackedState_t *s )
{
	switch( sds.visFace )
	{
	case GL_FRONT:
			glEnable( GL_CULL_FACE );
			glCullFace( GL_BACK );
			break;

	case GL_BACK:
		glEnable( GL_CULL_FACE );
		glCullFace( GL_FRONT );
		break;

	case GL_FRONT_AND_BACK:
		glDisable( GL_CULL_FACE );
		break;

	NO_DEFAULT;
	}
}

//Cull - reset via R_StateResetGLEnable

void R_StateSetPolygonOffset( float f, float u )
{
	if( f == 0 && u == 0 )
		R_StateSetDefault( TSN_PolygonOffset );
	else
	{
		glEnable( GL_POLYGON_OFFSET_FILL );
		glPolygonOffset( f, u );
		R_StateChanged( TSN_PolygonOffset );
	}
}

//PolygonOffset - reset via R_StateResetGLDisable

void R_StateSetRasterMode( GLenum mode )
{
	if( mode == GL_FILL )
		R_StateSetDefault( TSN_RasterMode );
	else
	{
		glPolygonMode( GL_FRONT_AND_BACK, mode );
		R_StateChanged( TSN_RasterMode );
	}
}

static void R_StateResetRasterMode( trackedState_t *s )
{
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

void R_StateSetModelViewMatrixCountedRaw( const float *mat )
{
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( mat );

	R_StateCountIncrement( CSN_ModelViewMatrix );
}

void R_StateSetModelViewMatrixCountedAffine( const affine_t *mat )
{
	float m[16];
	Matrix_SetFromAffine( m, mat );

	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( m );

	R_StateCountIncrement( CSN_ModelViewMatrix );
}

void R_StateSetProjectionMatrixCountedRaw( const float *mat )
{
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( mat );

	R_StateCountIncrement( CSN_ProjectionMatrix );
}

void R_StateMulProjectionMatrixCountedRaw( const float *mat )
{
	glMatrixMode( GL_PROJECTION );
	glMultMatrixf( mat );

	R_StateCountIncrement( CSN_ProjectionMatrix );
}

void R_StateSetupStates( void )
{
	sds.blendFunc[0] = GL_ONE;
	sds.blendFunc[1] = GL_ZERO;
	
	sds.depthTest = qtrue;
	
	sds.depthFunc = GL_LEQUAL;
	
	sds.depthRange[0] = 0;
	sds.depthRange[1] = 1;

	sds.visFace = GL_FRONT;

	//set up the tracked states
	{
		trackedState_t *s;

#define DefineTrackedState( _name, _reset )					\
	{														\
		s = track.s + (int)(_name);							\
		memset( s, 0, sizeof( *s ) );						\
		s->name = (_name);									\
		s->reset = _reset;									\
	}

#define DefineTrackedState_GLDisable( _name, _glenum )		\
	{														\
		DefineTrackedState( _name, R_StateResetGLDisable );	\
		s->data.asglenum = _glenum;							\
	}

#define DefineTrackedState_GLEnable( _name, _glenum )		\
	{														\
		DefineTrackedState( _name, R_StateResetGLEnable );	\
		s->data.asglenum = _glenum;							\
	}

		DefineTrackedState( TSN_Blend, R_StateResetBlend );
		DefineTrackedState_GLDisable( TSN_AlphaTest, GL_ALPHA_TEST );
		DefineTrackedState( TSN_Cull, R_StateResetCull );
		DefineTrackedState( TSN_RasterMode, R_StateResetRasterMode );
		DefineTrackedState_GLDisable( TSN_PolygonOffset, GL_POLYGON_OFFSET_FILL );

		DefineTrackedState( TSN_DepthTest, R_StateResetDepthTest );
		DefineTrackedState( TSN_DepthFunc, R_StateResetDepthFunc );
		DefineTrackedState( TSN_DepthMask, R_StateResetDepthMask );
		DefineTrackedState( TSN_DepthRange, R_StateResetDepthRange );

#undef DefineTrackedState_GLEnable
#undef DefineTrackedState_GLDisable
#undef DefineTrackedState
	}

	//set up the counted states
	{
		countedState_t *s;

#define DefineCountedState( _name )							\
	{														\
		s = count.s + (int)(_name);							\
		memset( s, 0, sizeof( *s ) );						\
		s->name = (_name);									\
	}

		DefineCountedState( CSN_ModelViewMatrix );
		DefineCountedState( CSN_ProjectionMatrix );

#undef DefineCountedState
	}
}