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

backEndData_t	*backEndData[SMP_FRAMES];
backEndState_t	backEnd;


static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

static void RB_SetProjection2D( qboolean value )
{
	if( backEnd.projection2D == value )
		return;

	backEnd.projection2D = value;
	
	if( backEnd.projection2D )
	{
		/*
		if( backEnd.ingameMenuProj )
		{
			R_StateSetDefaultDepthTest( qtrue );
			R_StateSetDefaultDepthFunc( GL_LEQUAL );
		}
		else
		*/
		{
			R_StateSetDefaultDepthTest( qfalse );
		}

		R_StateSetDefaultBlend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else
	{
		R_StateSetDefaultDepthTest( qtrue );
		R_StateSetDefaultDepthFunc( GL_LEQUAL );

		R_StateSetDefaultBlend( GL_ONE, GL_ZERO );
	}
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( uint stateBits )
{
	GLenum srcFactor, dstFactor;

	if( stateBits & GLS_DEPTHFUNC_EQUAL )
		R_StateSetDepthFunc( GL_EQUAL );

	if( stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS) )
	{
		switch ( stateBits & GLS_SRCBLEND_BITS )
		{
		case GLS_SRCBLEND_ZERO:
			srcFactor = GL_ZERO;
			break;
		case GLS_SRCBLEND_ONE:
			srcFactor = GL_ONE;
			break;
		case GLS_SRCBLEND_DST_COLOR:
			srcFactor = GL_DST_COLOR;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
			srcFactor = GL_ONE_MINUS_DST_COLOR;
			break;
		case GLS_SRCBLEND_SRC_ALPHA:
			srcFactor = GL_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
			srcFactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_DST_ALPHA:
			srcFactor = GL_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
			srcFactor = GL_ONE_MINUS_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ALPHA_SATURATE:
			srcFactor = GL_SRC_ALPHA_SATURATE;
			break;
		default:
			srcFactor = GL_ONE;		// to get warning to shut up
			ri.Error( ERR_DROP, "GL_State: invalid src blend state bits\n" );
			break;
		}

		switch ( stateBits & GLS_DSTBLEND_BITS )
		{
		case GLS_DSTBLEND_ZERO:
			dstFactor = GL_ZERO;
			break;
		case GLS_DSTBLEND_ONE:
			dstFactor = GL_ONE;
			break;
		case GLS_DSTBLEND_SRC_COLOR:
			dstFactor = GL_SRC_COLOR;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
			dstFactor = GL_ONE_MINUS_SRC_COLOR;
			break;
		case GLS_DSTBLEND_SRC_ALPHA:
			dstFactor = GL_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
			dstFactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_DST_ALPHA:
			dstFactor = GL_DST_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
			dstFactor = GL_ONE_MINUS_DST_ALPHA;
			break;
		default:
			dstFactor = GL_ZERO;		// to get warning to shut up
			ri.Error( ERR_DROP, "GL_State: invalid dst blend state bits\n" );
			break;
		}

		R_StateSetBlend( srcFactor, dstFactor );
	}
	else
	{
		R_StateSetBlend( GL_ONE, GL_ZERO );
	}

	R_StateSetDepthMask( (stateBits & GLS_DEPTHMASK_TRUE) ? qtrue : qfalse );
	
	if( stateBits & GLS_POLYMODE_LINE )
		R_StateSetRasterMode( GL_LINE );

	if( stateBits & GLS_DEPTHTEST_DISABLE )
		R_StateSetDepthTest( qfalse );

	switch( stateBits & GLS_ATEST_BITS )
	{
	case 0:
		break;

	case GLS_ATEST_GT_0:
		R_StateSetAlphaTest( GL_GREATER, 0.0f );
		break;

	case GLS_ATEST_LT_80:
		R_StateSetAlphaTest( GL_LESS, 0.5f );
		break;

	case GLS_ATEST_GE_80:
		R_StateSetAlphaTest( GL_GEQUAL, 0.5f );
		break;

	default:
		assert( 0 );
		break;
	}
}



/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void ) {
	float		c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
	glClearColor( c, c, c, 1 );
	glClear( GL_COLOR_BUFFER_BIT );
	glClearColor( 0, 0, 0, 0 );

	backEnd.isHyperspace = qtrue;
}


static void SetViewportAndScissor( void )
{
	R_StateSetProjectionMatrixCountedRaw( backEnd.viewParms.projectionMatrix );
	
	// set the window clipping
	glViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	glScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView( void )
{
	backEnd.frameViewCount++;

	// sync with gl if needed
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		glFinish ();
		glState.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	RB_SetProjection2D( qfalse );

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	glClear( GL_DEPTH_BUFFER_BIT );

	if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	R_StateForceReset( TSN_Cull ); // force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal )
	{
		float	plane[4];
		double	plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct (backEnd.viewParms.or.axis[0], plane);
		plane2[1] = DotProduct (backEnd.viewParms.or.axis[1], plane);
		plane2[2] = DotProduct (backEnd.viewParms.or.axis[2], plane);
		plane2[3] = DotProduct (plane, backEnd.viewParms.or.origin) - plane[3];

		R_StateSetModelViewMatrixCountedRaw( s_flipMatrix );
		glClipPlane (GL_CLIP_PLANE0, plane2);
		glEnable (GL_CLIP_PLANE0);
	} else {
		glDisable (GL_CLIP_PLANE0);
	}
}


#define	MAC_EVENT_PUMP_MSEC		5

/*
==================
RB_RenderDrawSurfList
==================
*/
void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs )
{
	shader_t		*shader, *oldShader;
	int				fogNum, oldFogNum;
	int				entityNum, oldEntityNum;
	int				dlighted, oldDlighted;
	qboolean		depthRange, oldDepthRange;
	int				i;
	drawSurf_t		*drawSurf;
	unsigned int	oldSort;
	float			originalTime;

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();

	// draw everything
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDepthRange = qfalse;
	oldDlighted = qfalse;
	oldSort = (unsigned int)~0;
	depthRange = qfalse;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for( i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++ )
	{
		if ( drawSurf->sort == (unsigned int)oldSort ) {
			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		oldSort = drawSurf->sort;
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted );

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if (shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted 
			|| ( entityNum != oldEntityNum && !shader->entityMergable ) ) {
			if (oldShader != NULL) {
				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum, GL_TRIANGLES );
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			depthRange = qfalse;

			if ( entityNum != ENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.or );

				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights ) {
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
				}

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) {
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.or = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
			}

			R_StateSetModelViewMatrixCountedRaw( backEnd.or.modelMatrix );

			//
			// change depthrange if needed
			//
			if( oldDepthRange != depthRange )
			{
				R_StateSetDefaultDepthRange( 0, depthRange ? 0.3F : 1.0F );
				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if( oldShader )
		RB_EndSurface();

	// go back to the world modelview matrix
	R_StateSetModelViewMatrixCountedRaw( backEnd.viewParms.world.modelMatrix );
	if( depthRange )
		R_StateSetDefaultDepthRange( 0, 1 );

#if 0
	RB_DrawSun();
#endif
	// darken down any stencil shadows
	RB_ShadowFinish();		

	// add light flares on lights that aren't obscured
	RB_RenderFlares();
}


/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
================
RB_SetGL2D

================
*/
void RB_SetGL2D( void )
{
	RB_SetProjection2D( qtrue );

	// set 2D virtual screen size
	glViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	glScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );

	glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	glOrtho( 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
	R_StateCountIncrement( CSN_ProjectionMatrix );

	glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
	R_StateCountIncrement( CSN_ModelViewMatrix );

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	R_StateSetDefaultCull( GL_FRONT_AND_BACK );

	glDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}


/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void Q_EXTERNAL_CALL RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	int			i, j;
	int			start, end;

	if ( !tr.registered ) {
		return;
	}
	R_SyncRenderThread();

	// we definately want to sync every frame for the cinematics
	glFinish();

	start = end = 0;
	if ( r_speeds->integer ) {
		start = ri.Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ ) {
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ ) {
	}
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		ri.Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

	R_StateSetTexture( tr.scratchImage[client], GL_TEXTURE0_ARB );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );	
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}

	if ( r_speeds->integer ) {
		end = ri.Milliseconds();
		ri.Printf( PRINT_ALL, "glTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	RB_SetGL2D();

	glColor3f( tr.identityLight, tr.identityLight, tr.identityLight );

	glBegin (GL_QUADS);
	glTexCoord2f ( 0.5f / cols,  0.5f / rows );
	glVertex2f (x, y);
	glTexCoord2f ( ( cols - 0.5f ) / cols ,  0.5f / rows );
	glVertex2f (x+w, y);
	glTexCoord2f ( ( cols - 0.5f ) / cols, ( rows - 0.5f ) / rows );
	glVertex2f (x+w, y+h);
	glTexCoord2f ( 0.5f / cols, ( rows - 0.5f ) / rows );
	glVertex2f (x, y+h);
	glEnd ();
}

void Q_EXTERNAL_CALL RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {

	R_StateSetTexture( tr.scratchImage[client], GL_TEXTURE0_ARB );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );	
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}
}


/*
=============
RE/RB_SetColor
=============
*/
static void RB_SetColor( const void *data )
{
	const setColorCommand_t	*cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 0xFF;
	backEnd.color2D[1] = cmd->color[1] * 0xFF;
	backEnd.color2D[2] = cmd->color[2] * 0xFF;
	backEnd.color2D[3] = cmd->color[3] * 0xFF;
}

void Q_EXTERNAL_CALL RE_SetColor( const float *rgba ) {
	setColorCommand_t	*cmd;

  if ( !tr.registered ) {
    return;
  }
	cmd = R_GetCommandBuffer( RB_SetColor, sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	if ( !rgba ) {
		static float colorWhite[4] = { 1, 1, 1, 1 };

		rgba = colorWhite;
	}

	cmd->color[0] = rgba[0];
	cmd->color[1] = rgba[1];
	cmd->color[2] = rgba[2];
	cmd->color[3] = rgba[3];
}

static void RB_SetColorRGB( const void *data )
{
	const setColorRGBCommand_t	*cmd;

	cmd = (const setColorRGBCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 0xFF;
	backEnd.color2D[1] = cmd->color[1] * 0xFF;
	backEnd.color2D[2] = cmd->color[2] * 0xFF;
}

void Q_EXTERNAL_CALL RE_SetColorRGB( const float *rgb ) {
	setColorRGBCommand_t	*cmd;

  if ( !tr.registered ) {
    return;
  }
	cmd = R_GetCommandBuffer( RB_SetColorRGB, sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	if ( !rgb ) {
		cmd->color[ 0 ] =
		cmd->color[ 1 ] =
		cmd->color[ 2 ] = 1.0f;

	} else
	{
		cmd->color[ 0 ] = rgb[ 0 ];
		cmd->color[ 1 ] = rgb[ 1 ];
		cmd->color[ 2 ] = rgb[ 2 ];
	}
}

/*
=============
RE/RB_StretchPic
=============
*/
typedef struct stretchPicCommand_t
{
	float		x, y;
	float		w, h;
	float		s1, t1;
	float		s2, t2;

	float		mat[2][3];

	shader_t	*shader;
} stretchPicCommand_t;

static void RB_StretchPic ( const void *data )
{
	const stretchPicCommand_t *cmd = (const stretchPicCommand_t *)data;

	int i;

	shader_t *shader;
	int numVerts, numIndexes;

	if( !backEnd.projection2D )
		RB_SetGL2D();

	shader = cmd->shader;
	RB_CheckSurface( shader, 0, GL_TRIANGLES );

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	*(int *)tess.vertexColors[ numVerts ] =
		*(int *)tess.vertexColors[ numVerts + 1 ] =
		*(int *)tess.vertexColors[ numVerts + 2 ] =
		*(int *)tess.vertexColors[ numVerts + 3 ] = *(int *)backEnd.color2D;

	tess.xyz[ numVerts ][0] = cmd->x;
	tess.xyz[ numVerts ][1] = cmd->y;
	tess.xyz[ numVerts ][2] = 0;

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][1] = cmd->y;
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = cmd->x;
	tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	for( i = 0; i < 4; i++ )
	{
		vec2_t v;

		Vec2_Cpy( v, tess.xyz[numVerts + i] );

		tess.xyz[numVerts + i][0] = v[0] * cmd->mat[0][0] + v[1] * cmd->mat[0][1] + cmd->mat[0][2];
		tess.xyz[numVerts + i][1] = v[0] * cmd->mat[1][0] + v[1] * cmd->mat[1][1] + cmd->mat[1][2];
	}
}

void Q_EXTERNAL_CALL RE_StretchPic( float x, float y, float w, float h, 
	float mat[2][3], float s1, float t1, float s2, float t2, qhandle_t hShader )
{
	stretchPicCommand_t	*cmd;

	if( !tr.registered )
		return;

	cmd = R_GetCommandBuffer( RB_StretchPic, sizeof( *cmd ) );
	if( !cmd )
		return;

	cmd->shader = R_GetShaderByHandle( hShader );

	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;

	if( mat )
		Com_Memcpy( cmd->mat, mat, sizeof( cmd->mat ) );
	else
	{
		cmd->mat[0][0] = 1.0F;	cmd->mat[0][1] = 0.0F;	cmd->mat[0][2] = 0.0F;
		cmd->mat[1][0] = 0.0F;	cmd->mat[1][1] = 1.0F;	cmd->mat[1][2] = 0.0F;
	}
}

void Q_EXTERNAL_CALL RE_StretchPicMulti( const stretchRect_t *rects, unsigned int numRects, qhandle_t hShader )
{
	uint i;	
	for( i = 0; i < numRects; i++ )
	{
		const stretchRect_t *r = rects + i;
		RE_StretchPic( r->pos[0], r->pos[1], r->size[0], r->size[1],
			NULL, r->tc0[0], r->tc0[1], r->tc1[0], r->tc1[1], hShader );
	}
}

typedef struct drawPolysCmd_t
{
	uint			numVertsPerPoly;
	uint			numPolys;
	shader_t		*shader;

	polyVert2D_t	verts[1]; //actually the start of an in-line array
} drawPolysCmd_t;

void RB_DrawPolys( const void *data )
{
	const drawPolysCmd_t *cmd = (drawPolysCmd_t*)data;

	uint i;
	uint numVerts, numIndices;

	float *xy, *st;
	uint *colors;
	glIndex_t *indices;
	uint baseVtx;

	if( !backEnd.projection2D )
		RB_SetGL2D();

	numVerts = cmd->numVertsPerPoly * cmd->numPolys;
	numIndices = cmd->numPolys * (cmd->numVertsPerPoly - 2) * 3;

	RB_CheckSurface( cmd->shader, 0, GL_TRIANGLES );

	RB_CHECKOVERFLOW( numVerts, numIndices );

	baseVtx = tess.numVertexes;
	xy = tess.xyz[baseVtx];
	st = tess.texCoords[baseVtx][0];
	colors = (uint*)tess.vertexColors + baseVtx;
	
	tess.numVertexes = baseVtx + numVerts;

	for( i = 0; i < numVerts; i++ )
	{
		xy[0] = cmd->verts[i].xy[0];
		xy[1] = cmd->verts[i].xy[1];
		xy[2] = 0;

		st[0] = cmd->verts[i].st[0];
		st[1] = cmd->verts[i].st[1];

		*colors = *(uint*)cmd->verts[i].modulate;

		xy++;
		st++;
		colors++;
	}

	indices = tess.indexes + tess.numIndexes;

	for( i = 0; i < cmd->numPolys; i++ )
	{
		uint j;

		//emit fan-order indices
		for( j = 2; j < cmd->numVertsPerPoly; j++ )
		{
			indices[0] = baseVtx;
			indices[1] = baseVtx + j - 1;
			indices[2] = baseVtx + j;

			indices += 3;
		}

		baseVtx += cmd->numVertsPerPoly;
	}

	tess.numIndexes += numIndices;
}

void Q_EXTERNAL_CALL RE_DrawPolys( const polyVert2D_t *verts, uint numVertsPerPoly, uint numPolys, qhandle_t hShader )
{
	uint numVerts;
	drawPolysCmd_t *cmd;

	numVerts = numPolys * numVertsPerPoly;

	cmd = (drawPolysCmd_t*)R_GetCommandBuffer( RB_DrawPolys, sizeof( drawPolysCmd_t ) + sizeof( polyVert2D_t ) * (numVerts - 1) );

	if( !cmd )
		return;

	cmd->numVertsPerPoly = numVertsPerPoly;
	cmd->numPolys = numPolys;
	cmd->shader = R_GetShaderByHandle( hShader );
	Com_Memcpy( cmd->verts, verts, sizeof( polyVert2D_t ) * numVerts );
}

/*
=============
RB_DrawSurfs
R_AddDrawSurfCmd
=============
*/
void RB_DrawSurfs( const void *data )
{
	const drawSurfsCommand_t	*cmd;

	// finish any 2D drawing if needed
	if( tess.numIndexes )
		RB_EndSurface();

	cmd = (const drawSurfsCommand_t*)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	R_StateSetDefaultCull( backEnd.viewParms.isMirror ? GL_FRONT : GL_BACK );

	RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );

	//done drawing surfaces, make sure we're using the right cull orrientation
	backEnd.viewParms.isMirror = qfalse;
	R_StateSetDefaultCull( GL_FRONT );
}

void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs )
{
	drawSurfsCommand_t	*cmd;

	cmd = R_GetCommandBuffer( RB_DrawSurfs, sizeof( *cmd ) );
	if( !cmd )
		return;
	
	cmd->drawSurfs = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}


/*
=============
RB_DrawBuffer

=============
*/
void RB_DrawBuffer( const void *data )
{
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

	glDrawBuffer( cmd->buffer );
	
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	backEnd.frameViewCount = 0;

	R_StateRestoreAllStates();
	R_StateBeginGroup(); //start recording state changes
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void ) {
	int		i;
	image_t	*image;
	float	x, y, w, h;
	int		start, end;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	glClear( GL_COLOR_BUFFER_BIT );

	glFinish();

	start = ri.Milliseconds();

	for ( i=0 ; i<tr.numImages ; i++ ) {
		image = tr.images[i];

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		R_StateSetTexture( image, GL_TEXTURE0_ARB );
		glBegin (GL_QUADS);
		glTexCoord2f( 0, 0 );
		glVertex2f( x, y );
		glTexCoord2f( 1, 0 );
		glVertex2f( x + w, y );
		glTexCoord2f( 1, 1 );
		glVertex2f( x + w, y + h );
		glTexCoord2f( 0, 1 );
		glVertex2f( x, y + h );
		glEnd();
	}

	glFinish();

	end = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );

}


/*
=============
RB_SwapBuffers

=============
*/
void RB_SwapBuffers( const void *data )
{
	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer ) {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
		glReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory( stencilReadback );
	}


	if ( !glState.finishCalled ) {
		glFinish();
	}

	GLimp_EndFrame();

	RB_SetProjection2D( qfalse );
}

/*
====================
RB_ExecuteRenderCommands

This function will be called synchronously if running without
smp extensions, or asynchronously by another thread.
====================
*/
void RB_ExecuteRenderCommands( renderCommandList_t *cmds )
{
	int		t1, t2;

	t1 = ri.Milliseconds ();

	if ( !r_smp->integer || cmds == &backEndData[0]->commands ) {
		backEnd.smpFrame = 0;
	} else {
		backEnd.smpFrame = 1;
	}

	if( !glimp_suspendRender )
	{
		uint i;

		for( i = 0; i < MAX_RENDER_COMMANDS; i++ )
		{
			if( !cmds->cmds[i].func )
				break;

			cmds->cmds[i].func( cmds->cmds[i].data );
		}
	}
	else
	{
		//try to do a swap to make it wake up when the window comes back
		GLimp_EndFrame();
		RB_SetProjection2D( qfalse );
	}

	t2 = ri.Milliseconds ();
	backEnd.pc.msec = t2 - t1;
}


/*
================
RB_RenderThread
================
*/
void RB_RenderThread( void ) {
	const void	*data;

	// wait for either a rendering command or a quit command
	while ( 1 ) {
		// sleep until we have work to do
		data = GLimp_RendererSleep();

		if ( !data ) {
			return;	// all done, renderer is shutting down
		}

		renderThreadActive = qtrue;

		RB_ExecuteRenderCommands( (renderCommandList_t*)data );

		renderThreadActive = qfalse;
	}
}

