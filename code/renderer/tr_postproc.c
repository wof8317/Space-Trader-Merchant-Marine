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

static void R_PpMakeRenderTexture( GLuint to, GLsizei w, GLsizei h, GLenum attachment );

static void R_DepthRenderInit( void );
static void R_DepthRenderKill( void );

static void R_BloomInit( void );
static void R_BloomKill( void );

//
// Post Process - Shared
//

static struct
{
	//use a seperate texture to copy so as
	//not to invalidate the framebuffer linkage
	GLuint cTex;

	int w, h;

	GLenum texTarget;
} ppd;

void R_PpInit( void )
{
	Com_Memset( &ppd, 0, sizeof( ppd ) );

	ppd.texTarget = R_StateGetRectTarget();

	if( !ppd.texTarget )
	{
		memset( &ppd, 0, sizeof( ppd ) );
		return;
	}
	
	ppd.w = glConfig.vidWidth;
	ppd.h = glConfig.vidHeight;

	glGenTextures( 1, &ppd.cTex );
	R_PpMakeRenderTexture( ppd.cTex, ppd.w, ppd.h, 0 );

	R_DepthRenderInit();
	R_BloomInit();

	return;
}

void R_PpKill( void )
{
	glDeleteTextures( 1, &ppd.cTex );

	R_BloomKill();
	R_DepthRenderKill();
}

static void R_PpMakeRenderTexture( GLuint to, GLsizei w, GLsizei h, GLenum attachment )
{
	glBindTexture( ppd.texTarget, to );

	glTexImage2D( ppd.texTarget, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL );

	glTexParameteri( ppd.texTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( ppd.texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( ppd.texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE_EXT );
	glTexParameteri( ppd.texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE_EXT );

	glBindTexture( ppd.texTarget, 0 );

	//attach as a render target
	if( attachment )
		glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, attachment, ppd.texTarget, to, 0 );
}

static void R_PpDoReadback( void )
{
	GLenum oldDB, oldRB;
	
	glGetIntegerv( GL_READ_BUFFER, (GLint*)&oldRB );
	glGetIntegerv( GL_DRAW_BUFFER, (GLint*)&oldDB );

	//copy from the buffer we were drawing to
	glReadBuffer( oldDB );

	R_StateSetTextureRaw( ppd.cTex, ppd.texTarget, qfalse, GL_TEXTURE0_ARB );
	glCopyTexImage2D( ppd.texTarget, 0, GL_RGB8, 0, 0, ppd.w, ppd.h, 0 );

	//done reading
	glReadBuffer( oldRB );
}

//
// Depth Render - Shared
//

static struct
{
	enum
	{
		DRM_NONE,
		DRM_GENERIC,
		DRM_ALPHA_DOF,
	}			mode;

	GLuint		fbo;
	GLuint		dTex;
	GLenum		texTarget;
} drd;

qboolean R_PpNeedsDepthRender( void )
{
	return drd.mode != DRM_NONE;
}

void R_PpBeginDepthRender( void )
{
}

void R_PpEndDepthRender( void )
{
}

static void R_DepthRenderInit( void )
{
	Com_Memset( &drd, 0, sizeof( drd ) );

	if( r_depthOfField->integer )
		drd.mode = DRM_ALPHA_DOF;
}

static void R_DepthRenderKill( void )
{
}

//
// Depth of Field
//

qboolean R_DofIsEnabled( void )
{
	return qfalse;
	//return R_DepthRenderAvailable();
}

static void R_DofRenderDof( void )
{
	if( !R_DofIsEnabled() || !r_depthOfField->integer )
		return;
}

//
// Bloom
//

static struct
{	   
	GLuint	fbo;
	GLuint	tex[2];
} bs;

static float GetGaussianWeight( float n, float strength )
{
	return 1.0F / sqrtf( 2 * (float)M_PI * strength ) *
		expf( -(n * n) / (2 * strength * strength) );
}

static void R_BloomSetupBlurParams( float du, float dv )
{
	int i;
	vec4_t taps[14];
	float strength, wtSum;
	
	strength = r_bloom->value;
	if( strength < 0 )
		strength = 0;

	taps[0][3] = GetGaussianWeight( 0, strength );
	wtSum = taps[0][3];

	for( i = 0; i < lengthof( taps ) / 2; i++ )
	{
		float wt = GetGaussianWeight( i + 1, strength );
		float ofs = i * 2.0F + 0.5F;

		taps[i * 2 + 0][0] = du * ofs;
		taps[i * 2 + 0][1] = dv * ofs;
		taps[i * 2 + 0][2] = wt;

		taps[i * 2 + 1][0] = -du * ofs;
		taps[i * 2 + 1][1] = -dv * ofs;
		taps[i * 2 + 1][2] = wt;

		wtSum += wt * 2;
	}
	
	wtSum = 1.0F / wtSum;

	taps[0][3] *= wtSum;
	for( i = 0; i < lengthof( taps ); i++ )
	{
		taps[i][2] *= wtSum;
		glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, i, taps[i] );
	}
}

static void R_BloomInit( void )
{
	GLenum status;
	GLint max;

	memset( &bs, 0, sizeof( bs ) );

	if( !GLEW_EXT_framebuffer_object )
		return;

	glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS_EXT, &max );

	if( !r_bloom->integer || max < 2 )
	{
		return;
	}

	/*
		If we can't run the appropriate fragment programs, we can't do bloom.
	*/
	if( !R_SpIsStandardFragmentProgramSupported( SPF_BLOOMSELECT ) ||
		!R_SpIsStandardFragmentProgramSupported( SPF_BLOOMBLUR ) ||
		!R_SpIsStandardFragmentProgramSupported( SPF_BLOOMCOMBINE ) )
	{
		return;
	}

	//initialize the render buffer object
	glGenFramebuffersEXT( 1, &bs.fbo );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, bs.fbo );

	glGenTextures( 2, bs.tex );
	R_PpMakeRenderTexture( bs.tex[0], ppd.w >> 2, ppd.h >> 2, GL_COLOR_ATTACHMENT0_EXT );
	R_PpMakeRenderTexture( bs.tex[1], ppd.w >> 2, ppd.h >> 2, GL_COLOR_ATTACHMENT1_EXT );
	
	status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
	if( status != GL_FRAMEBUFFER_COMPLETE_EXT )
		R_BloomKill();

	//bind back the window
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
}

static void R_BloomKill( void )
{
	if( bs.fbo )
	{
		glDeleteTextures( 2, bs.tex );
		glDeleteFramebuffersEXT( 1, &bs.fbo );
	}
	
	memset( &bs, 0, sizeof( bs ) );
}

qboolean R_BloomIsEnabled( void )
{
	return bs.fbo ? qtrue : qfalse;	
}

//pixel-perfect draw-over (assuming similar source and dest sizes)
static void R_BloomFSQuad(
	float du, float dv,	//texture size in pixels
	float mu, float mv,	//portion to render in pixels
	float dx, float dy)	//dest image size in pixels
{
	REF_PARAM( du );
	REF_PARAM( dv );
	REF_PARAM( dx );
	REF_PARAM( dy );

	R_StateSetActiveTmuUntracked( GL_TEXTURE0_ARB );

	glBegin( GL_QUADS );
	{
		glTexCoord2f( 0, 0 );
		glVertex2f( -1.0F, -1.0F );

		glTexCoord2f( 0, mv );
		glVertex2f( -1.0F, 1.0F );

		glTexCoord2f( mu, mv );
		glVertex2f( 1.0F, 1.0F );

		glTexCoord2f( mu, 0 );
		glVertex2f( 1.0F, -1.0F );
	}
	glEnd();
}

static qboolean ParseVectorNoParens( const char **text, int count, float *v )
{
	char *token;
	int i;

	for( i = 0; i < count; i++ )
	{
		token = COM_ParseExt( (char**)text, qfalse );
		
		if( !token[0] )
			return qfalse;
		
		v[i] = fatof( token );
	}

	return qtrue;
}

static void R_BloomRenderBloom( void )
{
	int i;
	GLenum oldDB;
	stateGroup_t sg;

	const samplerState_t samp =
	{
		GL_CLAMP_TO_EDGE_EXT, GL_CLAMP_TO_EDGE_EXT, GL_CLAMP_TO_EDGE_EXT,
		GL_LINEAR, GL_LINEAR,
		1
	};
	
	if( !bs.fbo || r_bloom->value <= 0 )
		return;

	if( GLEW_ARB_multisample && glConfig.fsaaSamples > 1 )
		glDisable( GL_MULTISAMPLE_ARB );

	glGetIntegerv( GL_DRAW_BUFFER, (GLint*)&oldDB );

	R_PpDoReadback();

	//set state up
	sg = R_StateBeginGroup();

	R_StateSetDepthTest( qfalse );
	
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	R_StateSetTextureSampler( &samp, GL_TEXTURE0_ARB );
	R_StateSetTextureSampler( &samp, GL_TEXTURE1_ARB );

	//start rendering off screen
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, bs.fbo );
	
	glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );

	glViewport( 0, 0, ppd.w >> 2, ppd.h >> 2 );

	R_StateSetTextureRaw( ppd.cTex, ppd.texTarget, qfalse, GL_TEXTURE0_ARB );
	R_SpSetStandardFragmentProgram( SPF_BLOOMSELECT );
	{
		float thresh = r_bloomThresh->value;
		glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 0, thresh, 1 - thresh, 0, 0 );
	}
	
	R_StateSetCull( GL_FRONT_AND_BACK );

	R_StateRestorePriorGroupStates( sg );

	R_BloomFSQuad( (float)ppd.w, (float)ppd.h, (float)ppd.w, (float)ppd.h, (float)(ppd.w >> 2), (float)(ppd.h >> 2) );

	//flip back and forth applying blur
	R_SpSetStandardFragmentProgram( SPF_BLOOMBLUR );
	for( i = 0; i < 2; i++ )
	{
		R_StateSetTexture( NULL, GL_TEXTURE0_ARB );
		glDrawBuffer( GL_COLOR_ATTACHMENT1_EXT );
		R_BloomSetupBlurParams( 1, 0 );
		R_StateSetTextureRaw( bs.tex[0], ppd.texTarget, qfalse, GL_TEXTURE0_ARB );
		R_BloomFSQuad( (float)(ppd.w >> 2), (float)(ppd.h >> 2), (float)(ppd.w >> 2), (float)(ppd.h >> 2), (float)(ppd.w >> 2), (float)(ppd.h >> 2) );

		R_StateSetTexture( NULL, GL_TEXTURE0_ARB );
		glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
		R_BloomSetupBlurParams( 0, 1 );
		R_StateSetTextureRaw( bs.tex[1], ppd.texTarget, qfalse, GL_TEXTURE0_ARB );
		R_BloomFSQuad( (float)(ppd.w >> 2), (float)(ppd.h >> 2), (float)(ppd.w >> 2), (float)(ppd.h >> 2), (float)(ppd.w >> 2), (float)(ppd.h >> 2) );
	}

	//return to window rendering
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	glViewport( 0, 0, ppd.w, ppd.h );
	glDrawBuffer( oldDB );

	R_SpSetStandardFragmentProgram( SPF_BLOOMCOMBINE );

	{
		vec4_t c;
		const char *s = r_bloomCombine->string;

		ParseVectorNoParens( &s, 4, c );

		glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 0, c ); 
	}

	R_StateSetTextureRaw( bs.tex[0], ppd.texTarget, qfalse, GL_TEXTURE0_ARB );
	R_StateSetTextureRaw( ppd.cTex, ppd.texTarget, qfalse, GL_TEXTURE1_ARB );
		
	R_BloomFSQuad(
		(float)(ppd.w >> 2), (float)(ppd.h >> 2),
		(float)(ppd.w >> 2), (float)(ppd.h >> 2),
		(float)ppd.w, (float)ppd.h );

	if( GLEW_ARB_multisample && glConfig.fsaaSamples > 1 )
		glEnable( GL_MULTISAMPLE_ARB );

	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}

void RB_PpDoPostProcesses( const void *ignored )
{
	R_DofRenderDof();
	R_BloomRenderBloom();
}

void Q_EXTERNAL_CALL RE_PpDoPostProcess( void )
{
	R_GetCommandBuffer( RB_PpDoPostProcesses, 0 );
}
