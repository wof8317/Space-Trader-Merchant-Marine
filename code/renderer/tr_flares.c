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

/*
=============================================================================

LIGHT FLARES

A light flare is an effect that takes place inside the eye when bright light
sources are visible.  The size of the flare reletive to the screen is nearly
constant, irrespective of distance, but the intensity should be proportional to the
projected area of the light source.

A surface that has been flagged as having a light flare will calculate the depth
buffer value that it's midpoint should have when the surface is added.

After all opaque surfaces have been rendered, the depth buffer is read back for
each flare in view.  If the point has not been obscured by a closer surface, the
flare should be drawn.

Surfaces that have a repeated texture should never be flagged as flaring, because
there will only be a single flare added at the midpoint of the polygon.

To prevent abrupt popping, the intensity of the flare is interpolated up and
down as it changes visibility.  This involves scene to scene state, unlike almost
all other aspects of the renderer, and is complicated by the fact that a single
frame may have multiple scenes.

RB_RenderFlares() will be called once per view (twice in a mirrored scene, potentially
up to five or more times in a frame with 3D status bar icons).

=============================================================================
*/

typedef struct
{
	vec2_t			projPos;	//[-1, 1] screen position
	float			eyeD;		//[0, 1] depth buffer value
	float			cosAngle;	//[0, 1] cosine of view angle
	GLuint			queries[2];	//denom, num
	shader_t		*shader;	//the flare shader
} flare_t;

static struct
{
	uint			numFlares;
	flare_t			flares[128];
} fd;

void R_ClearFlares( void )
{
	uint i;

	if( GLEW_ARB_occlusion_query )
	{
		for( i = 0; i < lengthof( fd.flares ); i++ )
			glDeleteQueriesARB( lengthof( fd.flares[i].queries ), fd.flares[i].queries );
	}

	Com_Memset( &fd, 0, sizeof( fd ) );
}

static void Matrix_MulA( vec3_t o, const float * RESTRICT m, const vec3_t v )
{
	o[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12];
	o[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13];
	o[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
}

static void Matrix_MulV( vec3_t o, const float * RESTRICT m, const vec3_t v )
{
	o[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2];
	o[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2];
	o[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2];
}

static void Matrix_MulH( vec3_t o, const float * RESTRICT m, const vec3_t v )
{
	float w;

	o[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12];
	o[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13];
	o[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];

	w = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15];
	w = 1.0F / w;

	o[0] *= w;
	o[1] *= w;
	o[2] *= w;
}

void RB_AddFlare( const vec3_t point, bool infinitelyFar, float size, shader_t *shader )
{
	GLint queryBits;

	flare_t *flare;

	vec3_t pTmp, pCam, pClip;
	
	stateGroup_t sg;

	if( !GLEW_ARB_occlusion_query || !r_flares->integer )
		return;

	glGetQueryivARB( GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, &queryBits );
	if( queryBits < 10 )
		return;

	if( !shader->flareParams )
		return;

	if( fd.numFlares == lengthof( fd.flares ) )
		return;

	//will render, reserve a flare object
	flare = fd.flares + fd.numFlares++;

	//transform into camera space
	if( infinitelyFar )
		Matrix_MulV( pCam, backEnd.or.modelMatrix, point );
	else
		Matrix_MulA( pCam, backEnd.or.modelMatrix, point );
	
	//adjust the test size
	//size *= 1 + (VectorLength( pCam ) - backEnd.viewParms.zNear) / (backEnd.viewParms.zFar - backEnd.viewParms.zNear);

	VectorCopy( pCam, pTmp );
	VectorNormalize( pTmp );
	flare->cosAngle = -pTmp[2];

	//then into clip
	Matrix_MulH( pClip, backEnd.viewParms.projectionMatrix, pCam );
		
	//set up the flare struct
	Vec2_Cpy( flare->projPos, pClip );
	flare->eyeD = VectorLength( pCam );
	flare->shader = shader;

	//set up the queries
	if( !glIsQueryARB( flare->queries[0] ) )
		glGenQueriesARB( lengthof( flare->queries ), flare->queries );

#if 0
	//flare test quad debugging option
	glColor3f( 1, 0, 1 );
#else
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
#endif

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

	sg = R_StateBeginGroup();

	R_StateSetDepthMask( false );
	R_StateSetCull( GL_FRONT_AND_BACK );
	
	R_StateRestorePriorGroupStates( sg );

	sg = R_StateBeginGroup();

	R_StateSetDepthFunc( GL_ALWAYS );

	//denominator (samples on screen)
	glBeginQueryARB( GL_SAMPLES_PASSED_ARB, flare->queries[0] );

	glBegin( GL_QUADS );
	{
		glVertex3f( pCam[0] - size, pCam[1] - size, pCam[2] );
		glVertex3f( pCam[0] + size, pCam[1] - size, pCam[2] );
		glVertex3f( pCam[0] + size, pCam[1] + size, pCam[2] );
		glVertex3f( pCam[0] - size, pCam[1] + size, pCam[2] );	
	}
	glEnd();

	glEndQueryARB( GL_SAMPLES_PASSED_ARB );

#if 0
	//flare test quad debugging option
	glColor3f( 1, 1, 0 );
#endif

	R_StateRestoreGroupStates( sg );

	glBeginQueryARB( GL_SAMPLES_PASSED_ARB, flare->queries[1] );

	glBegin( GL_QUADS );
	{
		glVertex3f( pCam[0] - size, pCam[1] - size, pCam[2] );
		glVertex3f( pCam[0] + size, pCam[1] - size, pCam[2] );
		glVertex3f( pCam[0] + size, pCam[1] + size, pCam[2] );
		glVertex3f( pCam[0] - size, pCam[1] + size, pCam[2] );	
	}
	glEnd();

	glEndQueryARB( GL_SAMPLES_PASSED_ARB );

	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}

void RB_AddDlightFlares( void )
{
	if( !GLEW_ARB_occlusion_query )
		return;

	//ToDo: loop through the dlights and add flares
}

void RB_RenderFlares( void )
{
	uint i;
	stateGroup_t sg;
	samplerState_t sampler = { GL_CLAMP_TO_EDGE_EXT, GL_CLAMP_TO_EDGE_EXT, GL_CLAMP_TO_EDGE_EXT, GL_NONE, GL_NONE, 0 };

	vec2_t invVp;

	qboolean first;

	if( !fd.numFlares )
		return;

	//no need to test occ query,
	//won't be here if it's missing

	sg = R_StateBeginGroup();

	R_StateSetTextureSampler( &sampler, GL_TEXTURE0_ARB );
	R_StateSetBlend( GL_SRC_ALPHA, GL_ONE );
	R_StateSetCull( GL_FRONT_AND_BACK );
	R_StateSetDepthTest( qfalse );
	
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	invVp[0] = 1.0F / (float)backEnd.viewParms.viewportWidth;
	invVp[1] = 1.0F / (float)backEnd.viewParms.viewportHeight;

	first = qtrue;
	for( i = 0; i < fd.numFlares; i++ )
	{
		flare_t *f = fd.flares + i;

		uint j;

		GLuint num, denom;
		float fv;
		float rotAngle;

		glGetQueryObjectuivARB( f->queries[0], GL_QUERY_RESULT_ARB, &denom );
		glGetQueryObjectuivARB( f->queries[1], GL_QUERY_RESULT_ARB, &num );

		//assert( num <= denom );

		if( !denom )
			continue;
		
		fv = (float)num / (float)denom;
		if( fv < 0.0001F )
			continue;

		rotAngle = atan2f( f->projPos[0], f->projPos[1] ) * (180.0F / (float)M_PI);

		for( j = 0; j < f->shader->flareParams->numPics; j++ )
		{
			flarePic_t *p = f->shader->flareParams->pics + j;

			float S, O, I, D, A;
			vec2_t pos;

			stateGroup_t sgc;
      
      if (!p->img) continue;

			sgc = R_StateBeginGroup();
			R_StateSetTexture( p->img, GL_TEXTURE0_ARB );

			if( first )
			{
				R_StateRestorePriorGroupStates( sg );
				first = qfalse;
			}
			else
				R_StateRestoreGroupStates( sg );

			sg = sgc;

			glMatrixMode( GL_MODELVIEW ); //may have been changed to GL_TEXTURE by texture set or state restore

			D = (f->eyeD - p->distRange[0]) / (p->distRange[1] - p->distRange[0]);
			O = ((1 - fv) - p->occRange[0]) / (p->occRange[1] - p->occRange[0]);
			A = (f->cosAngle - p->angleRange[0]) / (p->angleRange[1] - p->angleRange[0]);

			if( D < 0 ) D = 0;
			if( O < 0 ) O = 0;
			if( A < 0 ) A = 0;

			S = p->sizeAtten[0] + p->sizeAtten[1] * D + p->sizeAtten[2] * D * D;
			I = (p->intAtten[0] + p->intAtten[1] * O + p->intAtten[2] * O * O) *
				(p->intAtten[3] + p->intAtten[4] * D + p->intAtten[5] * D * D) *
				(p->intAtten[6] + p->intAtten[7] * A + p->intAtten[8] * A * A);

			//clamp these before the divide
			if( I < 0.00001 ) I = 0.00001;
			if( S < 0.00001 ) S = 0.00001;

			I = 1.0F / I;
			S = 1.0F / S;

			if( I > 1 ) I = 1;

			glPushMatrix();
			
			pos[0] = f->projPos[0] * (1.0F - p->pos);
			pos[1] = f->projPos[1] * (1.0F - p->pos);
			glTranslatef( pos[0], pos[1], 0 );

			if( p->rotate )
				glRotatef( -rotAngle, 0, 0, 1.0F );

			glScalef( p->img->width * 0.5F * invVp[0] * S,
				p->img->height * 0.5F * invVp[1] * S, 1.0F );

			glColor4f( p->color[0], p->color[1], p->color[2], I );

			glBegin( GL_QUADS );
			{
				glTexCoord2f( 1, 0 );
				glVertex2f( 1, -1 );

				glTexCoord2f( 1, 1 );
				glVertex2f( 1, 1 );

				glTexCoord2f( 0, 1 );
				glVertex2f( -1, 1 );

				glTexCoord2f( 0, 0 );
				glVertex2f( -1, -1 );
			}
			glEnd();

			glPopMatrix();
		}

	}

	glMatrixMode( GL_PROJECTION );
	glPopMatrix();

	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	fd.numFlares = 0;
}