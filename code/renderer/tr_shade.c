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

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

/*
===================
R_DrawStripElements

===================
*/
static int		c_vertexes;		// for seeing how long our average strips are
static int		c_begins;
static void R_DrawStripElements( int numIndexes, const glIndex_t *indexes, void ( APIENTRY *element )(GLint) ) {
	int i;
	int last[3] = { -1, -1, -1 };
	qboolean even;

	c_begins++;

	if ( numIndexes <= 0 ) {
		return;
	}

	glBegin( GL_TRIANGLE_STRIP );

	// prime the strip
	element( indexes[0] );
	element( indexes[1] );
	element( indexes[2] );
	c_vertexes += 3;

	last[0] = indexes[0];
	last[1] = indexes[1];
	last[2] = indexes[2];

	even = qfalse;

	for ( i = 3; i < numIndexes; i += 3 )
	{
		// odd numbered triangle in potential strip
		if ( !even )
		{
			// check previous triangle to see if we're continuing a strip
			if ( ( (int)indexes[i+0] == last[2] ) && ( (int)indexes[i+1] == last[1] ) )
			{
				element( indexes[i+2] );
				c_vertexes++;
				assert( indexes[i+2] < tess.numVertexes );
				even = qtrue;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				glEnd();

				glBegin( GL_TRIANGLE_STRIP );
				c_begins++;

				element( indexes[i+0] );
				element( indexes[i+1] );
				element( indexes[i+2] );

				c_vertexes += 3;

				even = qfalse;
			}
		}
		else
		{
			// check previous triangle to see if we're continuing a strip
			if ( ( last[2] == (int)indexes[i+1] ) && ( last[0] == (int)indexes[i+0] ) )
			{
				element( indexes[i+2] );
				c_vertexes++;

				even = qfalse;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				glEnd();

				glBegin( GL_TRIANGLE_STRIP );
				c_begins++;

				element( indexes[i+0] );
				element( indexes[i+1] );
				element( indexes[i+2] );
				c_vertexes += 3;

				even = qfalse;
			}
		}

		// cache the last three vertices
		last[0] = indexes[i+0];
		last[1] = indexes[i+1];
		last[2] = indexes[i+2];
	}

	glEnd();
}



/*
==================
R_DrawElements

Optionally performs our own glDrawElements that looks for strip conditions
instead of using the single glDrawElements call that may be inefficient
without compiled vertex arrays.
==================
*/
static void R_DrawElements( int numIndexes, const glIndex_t *indexes )
{
	int		primitives;

	primitives = r_primitives->integer;

	// default is to use triangles if compiled vertex arrays are present
	if( primitives == 0 )
		primitives = 2;

	if( primitives == 2 || tess.primType != GL_TRIANGLES )
	{
		glDrawElements( tess.primType, 
						numIndexes,
						GL_INDEX_TYPE,
						indexes );

		backEnd.pc.c_drawCalls++;

		return;
	}

	if( primitives == 1 )
	{
		R_DrawStripElements( numIndexes,  indexes, glArrayElement );
		return;
	}

	// anything else will cause no drawing
}


/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t	tess;
static qboolean	setArraysOnce;

static void R_SetCull( cullType_t ct )
{
	switch( ct )
	{
	case CT_FRONT_SIDED:
		R_StateSetCull( backEnd.viewParms.isMirror ? GL_FRONT : GL_BACK );
		break;

	case CT_BACK_SIDED:
		R_StateSetCull( backEnd.viewParms.isMirror ? GL_BACK : GL_FRONT );
		break;

	case CT_TWO_SIDED:
		R_StateSetCull( GL_FRONT_AND_BACK );
		break;
	}
}

/*
=================
R_BindAnimatedImage

=================
*/
static void R_BindAnimatedImage( textureBundle_t *bundle, GLenum tmu )
{
	int		index;

	if ( bundle->isVideoMap )
	{
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if( bundle->numImageAnimations <= 1 )
	{
		R_StateSetTexture( bundle->image[0], tmu );
	}
	else
	{

		// it is necessary to do this messy calc to make sure animations line up
		// exactly with waveforms of the same frequency
		index = (int)(tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
		index >>= FUNCTABLE_SIZE2;

		if ( index < 0 ) {
			index = 0;	// may happen with shader time offsets
		}
		index %= bundle->numImageAnimations;

		R_StateSetTexture( bundle->image[ index ], tmu );
	}

	R_StateSetTextureSampler( &bundle->sampler, tmu );
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris( shaderCommands_t *input )
{
	stateGroup_t sg = R_StateBeginGroup();

	R_StateSetTexture( tr.whiteImage, GL_TEXTURE0 );
	R_SetCull( CT_TWO_SIDED );

	glColor4f( 1, 1, 1, 1 );

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
	R_StateSetDepthRange( 0, 0 );

	glLineWidth( 1 );

	R_StateRestorePriorGroupStates( sg );

	glDisableClientState( GL_COLOR_ARRAY );

	R_StateSetActiveClientTmuUntracked( GL_TEXTURE1 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	
	R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	glVertexPointer( 3, GL_FLOAT, 16, input->xyz ); // padded for SIMD
	glTexCoord2f( 0, 0 );

	if( GLEW_EXT_compiled_vertex_array )
		glLockArraysEXT(0, input->numVertexes);

	R_DrawElements( input->numIndexes, input->indexes );

	if( GLEW_EXT_compiled_vertex_array )
		glUnlockArraysEXT();
}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals( shaderCommands_t *input )
{
	int		i;
	vec3_t	temp;

	stateGroup_t sg = R_StateBeginGroup();

	R_StateSetTexture( tr.whiteImage, GL_TEXTURE0 );

	glColor4f( 1, 1, 1, 1 );
	R_StateSetDepthRange( 0, 0 ); // never occluded
	
	R_SetCull( CT_TWO_SIDED );
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
	glLineWidth( 1 );

	R_StateRestorePriorGroupStates( sg );

	glBegin( GL_LINES );
	for( i = 0; i < input->numVertexes; i++ )
	{
		glVertex3fv( input->xyz[i] );
		VectorMA( input->xyz[i], 2, input->normal[i], temp );
		glVertex3fv( temp );
	}
	glEnd();
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum, GLenum primType )
{
	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.primType = primType;
	tess.dlightBits = 0;		// will be OR'd in by surface functions
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
		tess.shaderTime = tess.shader->clampTime;
	}


}

void RB_CheckSurface( shader_t *shader, int fogNum, GLenum primType )
{
/*
	Given:

	#define GL_POINTS			0x0000		//no restart
	#define GL_LINES			0x0001		//no restart
	#define GL_LINE_LOOP		0x0002		//restart
	#define GL_LINE_STRIP		0x0003		//restart
	#define GL_TRIANGLES		0x0004		//no restart
	#define GL_TRIANGLE_STRIP	0x0005		//restart
	#define GL_TRIANGLE_FAN		0x0006		//restart
	#define GL_QUADS			0x0007		//no restart
	#define GL_QUAD_STRIP		0x0008		//restart
	#define GL_POLYGON			0x0009		//restart

	We can define a bitmask to determine whether we need a reset or not by
	placing a 1 in the n'th lowest bit if a restart is not needed. This gives
	us the following least-significant bits:

		1001 0011

	Which is:

		0x93
*/

#define RESTART_MASK 0x93
	
	if( shader != tess.shader || fogNum != tess.fogNum ||
		primType != tess.primType || !((RESTART_MASK >> primType) & 0x1) )
	{
		if( tess.numIndexes )
			RB_EndSurface();
		
		if( backEnd.projection2D )
			backEnd.currentEntity = &backEnd.entity2D;

		RB_BeginSurface( shader, 0, primType );
	}

#undef RESTART_MASK
}

/*
===================
DrawMultitextured

output = t0 * t1 or t0 + t1

t0 = most upstream according to spec
t1 = most downstream according to spec
===================
*/
static stateGroup_t DrawMultitextured( const shaderCommands_t *input, int stage, stateGroup_t prevSg, qboolean firstPass )
{
	shaderStage_t	*pStage;

	pStage = tess.xstages[stage];

	R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );

	R_StateSetActiveClientTmuUntracked( GL_TEXTURE1 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[1] );

	{
		stateGroup_t sg = R_StateBeginGroup();

		GL_State( pStage->stateBits );

		if( pStage->lineWidth > 0.0F )
			glLineWidth( pStage->lineWidth );

		R_BindAnimatedImage( &pStage->bundle[0], GL_TEXTURE0 );
		R_BindAnimatedImage( &pStage->bundle[1], GL_TEXTURE1 );
			
		R_StateSetTextureEnvMode( r_lightmap->integer ? GL_REPLACE : tess.shader->multitextureEnv, GL_TEXTURE1 );

		if( firstPass )
			R_StateRestorePriorGroupStates( prevSg );
		else
			R_StateRestoreGroupStates( prevSg );

		prevSg = sg;

		R_DrawElements( input->numIndexes, input->indexes );
	}

	return prevSg;
}



/*
===================
ProjectDlightTexture

Perform dynamic lighting with another rendering pass
===================
*/
static stateGroup_t ProjectDlightTexture( stateGroup_t prevSg )
{
	int		i, l;
#if idppc_altivec
	vec_t	origin0, origin1, origin2;
	float   texCoords0, texCoords1;
	vector float floatColorVec0, floatColorVec1;
	vector float modulateVec, colorVec, zero;
	vector short colorShort;
	vector signed int colorInt;
	vector unsigned char floatColorVecPerm, modulatePerm, colorChar;
	vector unsigned char vSel = (vector unsigned char)(0x00, 0x00, 0x00, 0xff,
							   0x00, 0x00, 0x00, 0xff,
							   0x00, 0x00, 0x00, 0xff,
							   0x00, 0x00, 0x00, 0xff);
#else
	vec3_t	origin;
#endif
	float	*texCoords;
	byte	*colors;
	byte	clipBits[SHADER_MAX_VERTEXES];
	float	texCoordsArray[SHADER_MAX_VERTEXES][2];
	byte	colorArray[SHADER_MAX_VERTEXES][4];
	glIndex_t	hitIndexes[SHADER_MAX_INDEXES];
	int		numIndexes;
	float	scale;
	float	radius;
	vec3_t	floatColor;
	float	modulate;

	qboolean firstPass;

	if ( !backEnd.refdef.num_dlights ) {
		return prevSg;
	}

	glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

#if idppc_altivec
	// There has to be a better way to do this so that floatColor 
	// and/or modulate are already 16-byte aligned.
	floatColorVecPerm = vec_lvsl(0,(float *)floatColor);
	modulatePerm = vec_lvsl(0,(float *)&modulate);
	modulatePerm = (vector unsigned char)vec_splat((vector unsigned int)modulatePerm,0);
	zero = (vector float)vec_splat_s8(0);
#endif

	firstPass = qtrue;
	for ( l = 0 ; l < backEnd.refdef.num_dlights ; l++ ) {
		dlight_t	*dl;

		if ( !( tess.dlightBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this light
		}
		texCoords = texCoordsArray[0];
		colors = colorArray[0];

		dl = &backEnd.refdef.dlights[l];
#if idppc_altivec
		origin0 = dl->transformed[0];
		origin1 = dl->transformed[1];
		origin2 = dl->transformed[2];
#else
		VectorCopy( dl->transformed, origin );
#endif
		radius = dl->radius;
		scale = 1.0f / radius;

		floatColor[0] = dl->color[0] * 255.0f;
		floatColor[1] = dl->color[1] * 255.0f;
		floatColor[2] = dl->color[2] * 255.0f;
#if idppc_altivec
		floatColorVec0 = vec_ld(0, floatColor);
		floatColorVec1 = vec_ld(11, floatColor);
		floatColorVec0 = vec_perm(floatColorVec0,floatColorVec0,floatColorVecPerm);
#endif
		for ( i = 0 ; i < tess.numVertexes ; i++, texCoords += 2, colors += 4 ) {
#if idppc_altivec
			vec_t dist0, dist1, dist2;
#else
			vec3_t	dist;
#endif
			int		clip;

#if idppc_altivec
			//VectorSubtract( origin, tess.xyz[i], dist );
			dist0 = origin0 - tess.xyz[i][0];
			dist1 = origin1 - tess.xyz[i][1];
			dist2 = origin2 - tess.xyz[i][2];
			texCoords0 = 0.5f + dist0 * scale;
			texCoords1 = 0.5f + dist1 * scale;

			clip = 0;
			if ( texCoords0 < 0.0f ) {
				clip |= 1;
			} else if ( texCoords0 > 1.0f ) {
				clip |= 2;
			}
			if ( texCoords1 < 0.0f ) {
				clip |= 4;
			} else if ( texCoords1 > 1.0f ) {
				clip |= 8;
			}
			texCoords[0] = texCoords0;
			texCoords[1] = texCoords1;
			
			// modulate the strength based on the height and color
			if ( dist2 > radius ) {
				clip |= 16;
				modulate = 0.0f;
			} else if ( dist2 < -radius ) {
				clip |= 32;
				modulate = 0.0f;
			} else {
				dist2 = Q_fabs(dist2);
				if ( dist2 < radius * 0.5f ) {
					modulate = 1.0f;
				} else {
					modulate = 2.0f * (radius - dist2) * scale;
				}
			}
			clipBits[i] = clip;

			modulateVec = vec_ld(0,(float *)&modulate);
			modulateVec = vec_perm(modulateVec,modulateVec,modulatePerm);
			colorVec = vec_madd(floatColorVec0,modulateVec,zero);
			colorInt = vec_cts(colorVec,0);	// RGBx
			colorShort = vec_pack(colorInt,colorInt);		// RGBxRGBx
			colorChar = vec_packsu(colorShort,colorShort);	// RGBxRGBxRGBxRGBx
			colorChar = vec_sel(colorChar,vSel,vSel);		// RGBARGBARGBARGBA replace alpha with 255
			vec_ste((vector unsigned int)colorChar,0,(unsigned int *)colors);	// store color
#else
			VectorSubtract( origin, tess.xyz[i], dist );
			texCoords[0] = 0.5f + dist[0] * scale;
			texCoords[1] = 0.5f + dist[1] * scale;

			clip = 0;
			if ( texCoords[0] < 0.0f ) {
				clip |= 1;
			} else if ( texCoords[0] > 1.0f ) {
				clip |= 2;
			}
			if ( texCoords[1] < 0.0f ) {
				clip |= 4;
			} else if ( texCoords[1] > 1.0f ) {
				clip |= 8;
			}
			// modulate the strength based on the height and color
			if ( dist[2] > radius ) {
				clip |= 16;
				modulate = 0.0f;
			} else if ( dist[2] < -radius ) {
				clip |= 32;
				modulate = 0.0f;
			} else {
				dist[2] = Q_fabs(dist[2]);
				if ( dist[2] < radius * 0.5f ) {
					modulate = 1.0f;
				} else {
					modulate = 2.0f * (radius - dist[2]) * scale;
				}
			}
			clipBits[i] = clip;

			colors[0] = (int)(floatColor[0] * modulate);
			colors[1] = (int)(floatColor[1] * modulate);
			colors[2] = (int)(floatColor[2] * modulate);
			colors[3] = 255;
#endif
		}

		backEnd.pc.c_dlightVertexes += tess.numVertexes;

		// build a list of triangles that need light
		numIndexes = 0;
		for ( i = 0 ; i < tess.numIndexes ; i += 3 ) {
			int		a, b, c;

			a = tess.indexes[i];
			b = tess.indexes[i+1];
			c = tess.indexes[i+2];
			if ( clipBits[a] & clipBits[b] & clipBits[c] ) {
				continue;	// not lighted
			}
			hitIndexes[numIndexes] = a;
			hitIndexes[numIndexes+1] = b;
			hitIndexes[numIndexes+2] = c;
			numIndexes += 3;
		}

		if ( !numIndexes ) {
			continue;
		}

		R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, texCoordsArray[0] );

		glEnableClientState( GL_COLOR_ARRAY );
		glColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );

		{
			stateGroup_t sg = R_StateBeginGroup();
			samplerState_t sampler = { GL_CLAMP_TO_EDGE_EXT, GL_CLAMP_TO_EDGE_EXT, GL_CLAMP_TO_EDGE_EXT, GL_NONE, GL_NONE, 0 }; 

			R_StateSetTexture( tr.dlightImage, GL_TEXTURE0 );
			R_StateSetTextureSampler( &sampler, GL_TEXTURE0 );

			// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
			// where they aren't rendered
			GL_State( dl->additive ? (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL) :
				(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL) );

			if( firstPass )
			{
				R_StateRestorePriorGroupStates( prevSg );
				firstPass = qfalse;
			}
			else
				R_StateRestoreGroupStates( prevSg );

			prevSg = sg;

			R_DrawElements( numIndexes, hitIndexes );
		}

		backEnd.pc.c_totalIndexes += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;
	}

	glPopClientAttrib();

	return prevSg;
}


/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static stateGroup_t RB_FogPass( stateGroup_t prevSg )
{
	fog_t		*fog;
	int			i;

	fog = tr.world->fogs + tess.fogNum;

	for ( i = 0; i < tess.numVertexes; i++ ) {
		* ( int * )&tess.svars.colors[i] = fog->colorInt;
	}

	RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[0] );

	glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

	glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

	R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );
					 
	{
		stateGroup_t sg;
		samplerState_t sampler;

		sampler.minFilter = GL_NONE;
		sampler.magFilter = GL_NONE;
		sampler.maxAniso = 0;
		sampler.wrapR = GL_CLAMP_TO_EDGE;
		sampler.wrapS = GL_CLAMP_TO_EDGE;
		sampler.wrapT = GL_CLAMP_TO_EDGE;
		
		sg = R_StateBeginGroup();

		R_StateSetTexture( tr.fogImage, GL_TEXTURE0 );
		R_StateSetTextureSampler( &sampler, GL_TEXTURE0 );

		GL_State( tess.shader->fogPass == FP_EQUAL ?
			(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL) : 
			(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA) );

		R_StateRestorePriorGroupStates( sg );
		prevSg = sg;

		R_DrawElements( tess.numIndexes, tess.indexes );
	}

	glPopClientAttrib();

	return prevSg;
}

/*
===============
ComputeColors
===============
*/
static void ComputeColors( shaderStage_t *pStage )
{
	int		i;

	//
	// rgbGen
	//
	switch ( pStage->rgbGen )
	{
		case CGEN_SKIP:
			break;
		case CGEN_IDENTITY:
			Com_Memset( tess.svars.colors, 0xff, tess.numVertexes * 4 );
			break;
		default:
		case CGEN_IDENTITY_LIGHTING:
			Com_Memset( tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );
			break;
		case CGEN_LIGHTING_DIFFUSE:
			RB_CalcDiffuseColor( ( unsigned char * ) tess.svars.colors );
			break;
		case CGEN_LIGHTING_DIFFUSE2:
			RB_CalcDiffuseColor2( pStage, (byte*)tess.svars.colors );
			break;
		case CGEN_INVERSE_LIGHTING_DIFFUSE:
			RB_CalcDiffuseColor( ( unsigned char * ) tess.svars.colors );
			for( i = 0; i < tess.numVertexes; i++ )
			{
				tess.svars.colors[i][0] = 0xFF - tess.svars.colors[i][0];
				tess.svars.colors[i][1] = 0xFF - tess.svars.colors[i][1];
				tess.svars.colors[i][2] = 0xFF - tess.svars.colors[i][2];
			}
			break;

		case CGEN_EXACT_VERTEX:
			Com_Memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
			break;
		case CGEN_CONST:
			for ( i = 0; i < tess.numVertexes; i++ ) {
				*(int *)tess.svars.colors[i] = *(int *)pStage->constantColor;
			}
			break;
		case CGEN_VERTEX:
			if ( tr.identityLight == 1 )
			{
				Com_Memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
			}
			else
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
					tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
					tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
					tess.svars.colors[i][3] = tess.vertexColors[i][3];
				}
			}
			break;
		case CGEN_ONE_MINUS_VERTEX:
			if ( tr.identityLight == 1 )
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
					tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
					tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
				}
			}
			else
			{
				for ( i = 0; i < tess.numVertexes; i++ )
				{
					tess.svars.colors[i][0] = ( 255 - tess.vertexColors[i][0] ) * tr.identityLight;
					tess.svars.colors[i][1] = ( 255 - tess.vertexColors[i][1] ) * tr.identityLight;
					tess.svars.colors[i][2] = ( 255 - tess.vertexColors[i][2] ) * tr.identityLight;
				}
			}
			break;
		case CGEN_FOG:
			{
				fog_t		*fog;

				fog = tr.world->fogs + tess.fogNum;

				for ( i = 0; i < tess.numVertexes; i++ ) {
					* ( int * )&tess.svars.colors[i] = fog->colorInt;
				}
			}
			break;
		case CGEN_WAVEFORM:
			RB_CalcWaveColor( &pStage->rgbWave, ( unsigned char * ) tess.svars.colors );
			break;
		case CGEN_ENTITY:
			RB_CalcColorFromEntity( ( unsigned char * ) tess.svars.colors );
			break;
		case CGEN_ONE_MINUS_ENTITY:
			RB_CalcColorFromOneMinusEntity( ( unsigned char * ) tess.svars.colors );
			break;
	}

	//
	// alphaGen
	//
	switch ( pStage->alphaGen )
	{
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		if ( pStage->rgbGen != CGEN_IDENTITY ) {
			if ( ( pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) ||
				 pStage->rgbGen != CGEN_VERTEX ) {
				for ( i = 0; i < tess.numVertexes; i++ ) {
					tess.svars.colors[i][3] = 0xff;
				}
			}
		}
		break;
	case AGEN_CONST:
		if ( pStage->rgbGen != CGEN_CONST ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[i][3] = pStage->constantColor[3];
			}
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha( &pStage->alphaWave, ( unsigned char * ) tess.svars.colors );
		break;
	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha( pStage, (byte*)tess.svars.colors );
		break;
	case AGEN_GLOW_HALO:
		RB_CalcGlowHaloAlpha( pStage, (byte*)tess.svars.colors );
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( ( unsigned char * ) tess.svars.colors );
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( ( unsigned char * ) tess.svars.colors );
		break;
    case AGEN_VERTEX:
		if ( pStage->rgbGen != CGEN_VERTEX ) {
			for ( i = 0; i < tess.numVertexes; i++ ) {
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
        break;
    case AGEN_ONE_MINUS_VERTEX:
        for ( i = 0; i < tess.numVertexes; i++ )
        {
			tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
        }
        break;
	case AGEN_PORTAL:
		{
			unsigned char alpha;

			for ( i = 0; i < tess.numVertexes; i++ )
			{
				float len;
				vec3_t v;

				VectorSubtract( tess.xyz[i], backEnd.viewParms.or.origin, v );
				len = VectorLength( v );

				len /= tess.shader->portalRange;

				if ( len < 0 )
				{
					alpha = 0;
				}
				else if ( len > 1 )
				{
					alpha = 0xff;
				}
				else
				{
					alpha = len * 0xff;
				}

				tess.svars.colors[i][3] = alpha;
			}
		}
		break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	if ( tess.fogNum )
	{
		switch ( pStage->adjustColorsForFog )
		{
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog( ( unsigned char * ) tess.svars.colors );
			break;
		case ACFF_NONE:
			break;
		}
	}
}

/*
===============
ComputeTexCoords
===============
*/
static void ComputeTexCoords( shaderStage_t *pStage ) {
	int		i;
	int		b;

	for ( b = 0; b < NUM_TEXTURE_BUNDLES; b++ ) {
		int tm;

		//
		// generate the texture coordinates
		//
		switch ( pStage->bundle[b].tcGen )
		{
		case TCGEN_IDENTITY:
			Com_Memset( tess.svars.texcoords[b], 0, sizeof( float ) * 2 * tess.numVertexes );
			break;
		case TCGEN_TEXTURE:
			for ( i = 0 ; i < tess.numVertexes ; i++ ) {
				tess.svars.texcoords[b][i][0] = tess.texCoords[i][0][0];
				tess.svars.texcoords[b][i][1] = tess.texCoords[i][0][1];
			}
			break;
		case TCGEN_LIGHTMAP:
			for ( i = 0 ; i < tess.numVertexes ; i++ ) {
				tess.svars.texcoords[b][i][0] = tess.texCoords[i][1][0];
				tess.svars.texcoords[b][i][1] = tess.texCoords[i][1][1];
			}
			break;
		case TCGEN_VECTOR:
			for ( i = 0 ; i < tess.numVertexes ; i++ ) {
				tess.svars.texcoords[b][i][0] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[0] );
				tess.svars.texcoords[b][i][1] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[1] );
			}
			break;
		case TCGEN_FOG:
			RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[b] );
			break;
		case TCGEN_ENVIRONMENT_MAPPED:
			RB_CalcEnvironmentTexCoords( ( float * ) tess.svars.texcoords[b] );
			break;
		case TCGEN_BAD:
			return;
		}

		//
		// alter texture coordinates
		//
		for ( tm = 0; tm < pStage->bundle[b].numTexMods ; tm++ ) {
			switch ( pStage->bundle[b].texMods[tm].type )
			{
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;		// break out of for loop
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords( &pStage->bundle[b].texMods[tm].wave, 
						                 ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_ENTITY_TRANSLATE:
				RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord,
									 ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords( pStage->bundle[b].texMods[tm].scroll,
										 ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords( pStage->bundle[b].texMods[tm].scale,
									 ( float * ) tess.svars.texcoords[b] );
				break;
			
			case TMOD_STRETCH:
				RB_CalcStretchTexCoords( &pStage->bundle[b].texMods[tm].wave, 
						               ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords( &pStage->bundle[b].texMods[tm],
						                 ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords( pStage->bundle[b].texMods[tm].rotateSpeed,
										( float * ) tess.svars.texcoords[b] );
				break;

			default:
				ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n", pStage->bundle[b].texMods[tm].type, tess.shader->name );
				break;
			}
		}
	}
}

/*
** RB_IterateStagesGeneric
*/
static stateGroup_t RB_IterateStagesGeneric( shaderCommands_t *input, stateGroup_t prevSg )
{
	int stage;

	GLimp_LogComment( 1, "BEGIN RB_IterateStagesGeneric( %s )", input->shader->name );

	for( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = tess.xstages[stage];

		if( !pStage )
			break;

		GLimp_LogComment( 2, "LOOP START Stage %i", stage );

		if( !setArraysOnce )
		{
			ComputeColors( pStage );
			ComputeTexCoords( pStage );

			glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

			glEnableClientState( GL_COLOR_ARRAY );
			glColorPointer( 4, GL_UNSIGNED_BYTE, 0, input->svars.colors );
		}

		//
		// do multitexture
		//
		if( pStage->customDraw )
		{
			GLimp_LogComment( 3, "Stage is Custom Draw" );
			prevSg = pStage->customDraw( input, stage, prevSg, stage == 0 );
		}
		else
		{
			qboolean multitex = pStage->bundle[ 1 ].image[ 0 ] != 0;

			GLimp_LogComment( 3, multitex ? "Stage is Multi-Texture" : "Stage is Single-Texture" );

			if( !setArraysOnce )
			{
				if( multitex )
				{
					R_StateSetActiveClientTmuUntracked( GL_TEXTURE1 );
					glEnableClientState( GL_TEXTURE_COORD_ARRAY );
					glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[1] );
				}

				R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
			}

			{
				stateGroup_t sg = R_StateBeginGroup();
				
				if( pStage->bundle[0].vertexLightmap && ( (r_vertexLight->integer && !r_uiFullScreen->integer) ) && r_lightmap->integer )
				{
					R_StateSetTexture( tr.whiteImage, GL_TEXTURE0 );
					R_StateSetTextureEnvMode2( &pStage->bundle[0].texEnv, GL_TEXTURE0 );
				}
				else 
				{
					if( multitex )
					{
						R_BindAnimatedImage( pStage->bundle + 1, GL_TEXTURE1 );
						R_StateSetTextureEnvMode2( &pStage->bundle[1].texEnv, GL_TEXTURE1 );
					}

					R_BindAnimatedImage( &pStage->bundle[0], GL_TEXTURE0 );
					R_StateSetTextureEnvMode2( &pStage->bundle[0].texEnv, GL_TEXTURE0 );
				}

				GL_State( pStage->stateBits );

				if( pStage->lineWidth > 0.0F )
					glLineWidth( pStage->lineWidth );

				if( stage == 0 )
					R_StateRestorePriorGroupStates( prevSg );
				else
					R_StateRestoreGroupStates( prevSg );

				prevSg = sg;

				R_DrawElements( input->numIndexes, input->indexes );
			}

			GLimp_LogComment( 2, "LOOP END Stage %i", stage );
		}

		if( !setArraysOnce )
			glPopClientAttrib();

		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap || pStage->bundle[0].vertexLightmap ) )
		{
			break;
		}
	}

	GLimp_LogComment( 1, "END RB_IterateStagesGeneric( %s )", input->shader->name );

	return prevSg;
}

bool R_ShadeSupportsDeluxeMapping( void )
{
	if( R_StateGetNumTextureUnits() < 4 )
		return false;
	
	if( R_SpIsStandardVertexProgramSupported( SPV_DELUXEMAP ) &&
		R_SpIsStandardFragmentProgramSupported( SPF_DELUXEMAP ) )
		return true;

	if( R_SpIsStandardVertexProgramSupported( SPV_DELUXEMAP_NO_SPEC ) &&
		GLEW_ARB_texture_env_combine &&
		GLEW_ARB_texture_env_dot3 )
		return true;

	return false;
}

static stateGroup_t RB_ShadeDrawEntitySpecular( const shaderCommands_t *input, int stage, stateGroup_t sgPrev, qboolean restorePrior )
{
	stateGroup_t sg;
	shaderStage_t *pStage = input->xstages[stage];

	GLimp_LogComment( 1, "BEGIN RB_ShadeDrawEntitySpecular( %s )", input->shader->name );

#ifdef _DEBUG
	if( backEnd.currentEntity->e.reType == RT_MODEL )
	{
		model_t *m = R_GetModelByHandle( backEnd.currentEntity->e.hModel );
		if( m->name[0] )
			GLimp_LogComment( 3, "Entity: %s", m->name );
	}
#endif

	if( !setArraysOnce )
	{
		R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
	}

	glEnableClientState( GL_NORMAL_ARRAY );
	glNormalPointer( GL_FLOAT, sizeof( input->normal[0] ), input->normal );
	
	sg = R_StateBeginGroup();
	
	R_BindAnimatedImage( &pStage->bundle[0], GL_TEXTURE0 );

	GL_State( pStage->stateBits );

	R_SpSetStandardVertexProgram( SPV_SPEC );

	R_SpSetStandardFragmentProgram( SPF_SPEC );
	{
		if( backEnd.currentEntity )
		{
			float tmp[4];
			tmp[3] = 0;

			//send up light direction
			{
				affine_t a;
				Affine_SetFromMatrix( &a, backEnd.or.modelMatrix );
				Affine_MulVec( tmp, &a, backEnd.currentEntity->lightDir );
				VectorNormalize( tmp );
			}
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 0, tmp );

			VectorScale( backEnd.currentEntity->ambientLight, 1.0F / 255.0F, tmp );
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 1, tmp );

			VectorScale( backEnd.currentEntity->directedLight, 1.0F / 255.0F, tmp );
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 2, tmp );

			tmp[0] = backEnd.currentEntity->directedLight[0] * pStage->specCtrl[0] / 255.0F;
			tmp[1] = backEnd.currentEntity->directedLight[1] * pStage->specCtrl[1] / 255.0F;
			tmp[2] = backEnd.currentEntity->directedLight[2] * pStage->specCtrl[2] / 255.0F;
			tmp[3] = pStage->specCtrl[3];

			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 3, tmp );		
		}
		else
		{
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 0, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 1, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 2, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 3, 0, 0, 0, 0 );
		}
	}

	if( restorePrior )
		R_StateRestorePriorGroupStates( sgPrev );
	else
		R_StateRestoreGroupStates( sgPrev );

	R_DrawElements( input->numIndexes, input->indexes );

	if( setArraysOnce )
		//no push/pop on the outside, kill the normal array
		glDisableClientState( GL_NORMAL_ARRAY );

	GLimp_LogComment( 1, "END RB_ShadeDrawEntitySpecular( %s )", input->shader->name );

	return sg;
}

stateGroup_t RB_ShadeDrawEntitySpecularNormalMapped( const shaderCommands_t *input, int stage, stateGroup_t sgPrev, qboolean restorePrior )
{
	stateGroup_t sg;
	shaderStage_t *pStage = input->xstages[stage];

	GLimp_LogComment( 1, "BEGIN RB_ShadeDrawEntitySpecular( %s )", input->shader->name );

#ifdef _DEBUG
	if( backEnd.currentEntity->e.reType == RT_MODEL )
	{
		model_t *m = R_GetModelByHandle( backEnd.currentEntity->e.hModel );
		if( m->name[0] )
			GLimp_LogComment( 3, "Entity: %s", m->name );
	}
#endif

	if( !setArraysOnce )
	{
		R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
	}

	glEnableVertexAttribArrayARB( 6 );
	glVertexAttribPointerARB( 6, 3, GL_FLOAT, GL_FALSE, sizeof( input->tangent[0] ), input->tangent );
	glEnableVertexAttribArrayARB( 7 );
	glVertexAttribPointerARB( 7, 3, GL_FLOAT, GL_FALSE, sizeof( input->binormal[0] ), input->binormal );
	glEnableClientState( GL_NORMAL_ARRAY );
	glNormalPointer( GL_FLOAT, sizeof( input->normal[0] ), input->normal );
	
	sg = R_StateBeginGroup();
	
	R_BindAnimatedImage( &pStage->bundle[0], GL_TEXTURE0 );
	R_StateSetTexture( pStage->normalMap, GL_TEXTURE1 );

	GL_State( pStage->stateBits );

	R_SpSetStandardVertexProgram( SPV_SPEC_NM );
	{
		float tmp[4];
		tmp[3] = 1;
		
		VectorCopy( backEnd.or.viewOrigin, tmp );
		glProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, tmp );
	}

	R_SpSetStandardFragmentProgram( SPF_SPEC_NM );
	{
		if( backEnd.currentEntity )
		{
			float tmp[4];
			tmp[3] = 1;

			VectorScale( backEnd.currentEntity->ambientLight, 1.0F / 255.0F, tmp );
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 0, tmp );

			VectorScale( backEnd.currentEntity->directedLight, 1.0F / 255.0F, tmp );
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 1, tmp );

			tmp[0] = backEnd.currentEntity->directedLight[0] * pStage->specCtrl[0] / 255.0F;
			tmp[1] = backEnd.currentEntity->directedLight[1] * pStage->specCtrl[1] / 255.0F;
			tmp[2] = backEnd.currentEntity->directedLight[2] * pStage->specCtrl[2] / 255.0F;
			tmp[3] = pStage->specCtrl[3];
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 2, tmp );

			VectorScale( backEnd.currentEntity->directedLight, 0.0F / 255.0F, tmp );
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 3, tmp );

			VectorCopy( backEnd.currentEntity->lightDir, tmp );
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 4, tmp );

			tmp[0] = 2;
			tmp[1] = -1;
			tmp[2] = 4 - 4 * sqrtf( pStage->bumpDepth );
			tmp[3] = -0.5F * tmp[2];
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 5, tmp );
		}
		else
		{
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 0, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 1, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 2, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 3, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 4, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 5, 0, 0, 0, 0 );
		}
	}

	if( restorePrior )
		R_StateRestorePriorGroupStates( sgPrev );
	else
		R_StateRestoreGroupStates( sgPrev );

	R_DrawElements( input->numIndexes, input->indexes );

	if( setArraysOnce )
		//no push/pop on the outside, kill the normal array
		glDisableClientState( GL_NORMAL_ARRAY );

	glDisableVertexAttribArrayARB( 7 );
	glDisableVertexAttribArrayARB( 6 );

	GLimp_LogComment( 1, "END RB_ShadeDrawEntitySpecular( %s )", input->shader->name );

	return sg;
}

static stateGroup_t RB_ShadeDrawEntityDiffuseInternal( const shaderCommands_t *input, int stage, stateGroup_t sgPrev, qboolean restorePrior, bool invert )
{
	stateGroup_t sg;
	shaderStage_t *pStage = input->xstages[stage];

	GLimp_LogComment( 1, "BEGIN RB_ShadeDrawEntityDiffuse( %s )", input->shader->name );

#ifdef _DEBUG
	if( backEnd.currentEntity->e.reType == RT_MODEL )
	{
		model_t *m = R_GetModelByHandle( backEnd.currentEntity->e.hModel );
		if( m->name[0] )
			GLimp_LogComment( 3, "Entity: %s", m->name );
	}
#endif

	if( !setArraysOnce )
	{
		R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
	}

	glEnableClientState( GL_NORMAL_ARRAY );
	glNormalPointer( GL_FLOAT, sizeof( input->normal[0] ), input->normal );
	
	sg = R_StateBeginGroup();
	
	R_BindAnimatedImage( &pStage->bundle[0], GL_TEXTURE0 );

	GL_State( pStage->stateBits );

	R_SpSetStandardVertexProgram( SPV_DIFFUSE );

	R_SpSetStandardFragmentProgram( SPF_DIFFUSE );
	{
		if( backEnd.currentEntity )
		{
			float tmp[4];
			tmp[3] = 0;

			//send up light direction
			{
				affine_t a;
				Affine_SetFromMatrix( &a, backEnd.or.modelMatrix );
				Affine_MulVec( tmp, &a, backEnd.currentEntity->lightDir );
				VectorNormalize( tmp );
			}
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 0, tmp );

			VectorScale( backEnd.currentEntity->ambientLight, 1.0F / 255.0F, tmp );
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 1, tmp );

			VectorScale( backEnd.currentEntity->directedLight, 1.0F / 255.0F, tmp );
			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 2, tmp );

			if( pStage->rgbGen == CGEN_INVERSE_LIGHTING_DIFFUSE || invert )
			{
				tmp[0] = -1;
				tmp[1] = 1;
			}
			else
			{
				tmp[0] = 1;
				tmp[1] = 0;
			}

			tmp[2] = tmp[3] = 0;

			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 3, tmp );		
		}
		else
		{
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 0, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 1, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 2, 0, 0, 0, 0 );
			glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 3, 0, 0, 0, 0 );
		}
	}

	if( restorePrior )
		R_StateRestorePriorGroupStates( sgPrev );
	else
		R_StateRestoreGroupStates( sgPrev );

	R_DrawElements( input->numIndexes, input->indexes );

	if( setArraysOnce )
		//no push/pop on the outside, kill the normal array
		glDisableClientState( GL_NORMAL_ARRAY );

	GLimp_LogComment( 1, "END RB_ShadeDrawEntityDiffuse( %s )", input->shader->name );

	return sg;
}

static stateGroup_t RB_ShadeDrawEntityDiffuse( const shaderCommands_t *input, int stage, stateGroup_t sgPrev, qboolean restorePrior )
{
	return RB_ShadeDrawEntityDiffuseInternal( input, stage, sgPrev, restorePrior, false );
}

static stateGroup_t RB_ShadeDrawEntityInverseDiffuse( const shaderCommands_t *input, int stage, stateGroup_t sgPrev, qboolean restorePrior )
{
	return RB_ShadeDrawEntityDiffuseInternal( input, stage, sgPrev, restorePrior, true );
}

static stateGroup_t RB_ShadeDrawDeluxeMapDebugInfo( const shaderCommands_t *input, int stage, stateGroup_t sgPrev, qboolean restorePrior )
{
	stateGroup_t sg;
	shaderStage_t *pStage = input->xstages[stage];
	shaderStage_t *pNextStage = input->xstages[stage + 1];

	int tcidx0 = 0, tcidx1 = -1;

	GLimp_LogComment( 1, "BEGIN RB_ShadeDrawDeluxeMapDebugInfo( %s )", input->shader->name );

	glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );
	
	sg = R_StateBeginGroup();

	switch( r_lightmap->integer )
	{
	case 2: //normal
		R_StateSetTexture( pNextStage->normalMap, GL_TEXTURE0 );
		R_StateSetTextureSampler( &pNextStage->bundle[0].sampler, GL_TEXTURE0 );
		
		tcidx0 = 0;
		break;

	case 3: //deluxe
		R_StateSetTexture( pStage->deluxeMap, GL_TEXTURE0 );
		R_StateSetTextureSampler( &pStage->bundle[0].sampler, GL_TEXTURE0 );

		tcidx0 = 1;
		break;

	case 4: //n dot l
		R_StateSetTexture( pNextStage->normalMap, GL_TEXTURE0 );
		R_StateSetTextureSampler( &pNextStage->bundle[0].sampler, GL_TEXTURE0 );
		R_StateSetTexture( pStage->deluxeMap, GL_TEXTURE1 );
		R_StateSetTextureSampler( &pStage->bundle[0].sampler, GL_TEXTURE1 );

		R_StateSetTextureEnvMode( GL_REPLACE, GL_TEXTURE0 );
		{
			textureEnv_t env;

			env.complex = true;

			env.color_mode = GL_DOT3_RGB_ARB;
			env.alpha_mode = GL_REPLACE;

			env.src[0].color_src = GL_PREVIOUS_ARB;
			env.src[0].color_op = GL_SRC_COLOR;
			env.src[0].alpha_src = GL_PREVIOUS_ARB;
			env.src[0].alpha_op = GL_SRC_ALPHA;

			env.src[1].color_src = GL_TEXTURE;
			env.src[1].color_op = GL_SRC_COLOR;
			env.src[1].alpha_src = GL_TEXTURE;
			env.src[1].alpha_op = GL_SRC_ALPHA;

			env.color_scale = 1.0F;
			env.alpha_scale = 1.0F;

			R_StateSetTextureEnvMode2( &env, GL_TEXTURE1 );
		}

		tcidx0 = 0;
		tcidx1 = 1;
		break;

	default: //light
		R_StateSetTexture( pStage->bundle[0].image[0], GL_TEXTURE0 );
		R_StateSetTextureSampler( &pStage->bundle[0].sampler, GL_TEXTURE0 );

		tcidx0 = 1;
		break;
	}

	R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[tcidx0] );

	if( tcidx1 != -1 )
	{
		R_StateSetActiveClientTmuUntracked( GL_TEXTURE1 );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[tcidx1] );
	}

	GL_State( GLS_DEFAULT );

	if( restorePrior )
		R_StateRestorePriorGroupStates( sgPrev );
	else
		R_StateRestoreGroupStates( sgPrev );

	R_DrawElements( input->numIndexes, input->indexes );

	glPopClientAttrib();

	GLimp_LogComment( 1, "END RB_ShadeDrawDeluxeMapDebugInfo( %s )", input->shader->name );

	return sg;
}

static stateGroup_t RB_ShadeDrawDeluxeMap( const shaderCommands_t *input, int stage, stateGroup_t sgPrev, qboolean restorePrior )
{
	bool setExAttribs;

	stateGroup_t sg;
	shaderStage_t *pStage = input->xstages[stage];
	shaderStage_t *pNextStage = input->xstages[stage + 1];

	if( r_lightmap->integer )
		return RB_ShadeDrawDeluxeMapDebugInfo( input, stage, sgPrev, restorePrior );

	GLimp_LogComment( 1, "BEGIN RB_ShadeDrawDeluxeMap( %s )", input->shader->name );

	if( !setArraysOnce )
	{
		R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
	}

	R_StateSetActiveClientTmuUntracked( GL_TEXTURE1 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[1] );

	sg = R_StateBeginGroup();

	GL_State( GLS_DEFAULT );

	setExAttribs = false;
	switch( r_deluxemap->integer )
	{
	case 1:
	deluxe_no_spec:
		//deluxe w/o spec

		//expanding the texture data using a vertex program is a touch
		//faster than uploading each set of texture coordinates twice
		R_SpSetStandardVertexProgram( SPV_DELUXEMAP_NO_SPEC );

		R_StateSetTexture( pNextStage->normalMap, GL_TEXTURE0 );
		R_StateSetTextureSampler( &pNextStage->bundle[0].sampler, GL_TEXTURE0 );
		R_StateSetTexture( pStage->deluxeMap, GL_TEXTURE1 );
		R_StateSetTextureSampler( &pStage->bundle[0].sampler, GL_TEXTURE1 );
		R_BindAnimatedImage( pNextStage->bundle + 0, GL_TEXTURE2 ); //diffuse
		R_BindAnimatedImage( pStage->bundle + 0, GL_TEXTURE3 ); //light

		R_StateSetTextureEnvMode( GL_REPLACE, GL_TEXTURE0 );
		{
			textureEnv_t env;

			env.complex = true;
			env.color_mode = GL_DOT3_RGBA_ARB;
			env.src[0].color_src = GL_TEXTURE;
			env.src[0].color_op = GL_SRC_COLOR;
			env.src[1].color_src = GL_PREVIOUS_ARB;
			env.src[1].color_op = GL_SRC_COLOR;
			env.color_scale = 1.0F;
			env.alpha_scale = 1.0F;

			R_StateSetTextureEnvMode2( &env, GL_TEXTURE1 );
		}
		R_StateSetTextureEnvMode( GL_MODULATE, GL_TEXTURE2 );
		R_StateSetTextureEnvMode( GL_MODULATE, GL_TEXTURE3 );
		break;

	case 2:
		//deluxe w/ spec
		if( !R_SpIsStandardVertexProgramSupported( SPV_DELUXEMAP ) ||
			!R_SpIsStandardFragmentProgramSupported( SPF_DELUXEMAP ) )
			goto deluxe_no_spec;

		R_StateSetTexture( pNextStage->normalMap, GL_TEXTURE0 );
		R_StateSetTextureSampler( &pNextStage->bundle[0].sampler, GL_TEXTURE0 );
		R_StateSetTexture( pStage->deluxeMap, GL_TEXTURE1 );
		R_StateSetTextureSampler( &pStage->bundle[0].sampler, GL_TEXTURE1 );
		R_BindAnimatedImage( pNextStage->bundle + 0, GL_TEXTURE2 ); //diffuse
		R_BindAnimatedImage( pStage->bundle + 0, GL_TEXTURE3 ); //light

		setExAttribs = true;

		glEnableClientState( GL_NORMAL_ARRAY );
		glNormalPointer( GL_FLOAT, sizeof( input->normal[0] ), input->normal );

		glEnableVertexAttribArrayARB( 6 );
		glVertexAttribPointerARB( 6, 3, GL_FLOAT, GL_FALSE, sizeof( input->tangent[0] ), input->tangent );
		
		glEnableVertexAttribArrayARB( 7 );
		glVertexAttribPointerARB( 7, 3, GL_FLOAT, GL_FALSE, sizeof( input->binormal[0] ), input->binormal );

		R_SpSetStandardVertexProgram( SPV_DELUXEMAP );
		{
			vec4_t tmp;
			tmp[3] = 0;

			VectorCopy( backEnd.or.viewOrigin, tmp );
			glProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 0, tmp );
		}

		R_SpSetStandardFragmentProgram( SPF_DELUXEMAP );
		{
			vec4_t tmp;

			VectorSet( tmp, 0.5F, 0.53F, 0.51F ); //specular color = lightmap * this
			tmp[3] = 35; //specular exponent

			glProgramLocalParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 0, tmp );
		}
		break;

	default:
		//default light mapping
		R_BindAnimatedImage( pNextStage->bundle + 0, GL_TEXTURE0 ); //diffuse
		R_BindAnimatedImage( pStage->bundle + 0, GL_TEXTURE1 ); //light

		R_StateSetTextureEnvMode( GL_REPLACE, GL_TEXTURE0 );
		R_StateSetTextureEnvMode( GL_MODULATE, GL_TEXTURE0 );
		break;
	}

	if( restorePrior )
		R_StateRestorePriorGroupStates( sgPrev );
	else
		R_StateRestoreGroupStates( sgPrev );

	R_DrawElements( input->numIndexes, input->indexes );

	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	if( setExAttribs )
	{
		glDisableVertexAttribArrayARB( 7 );
		glDisableVertexAttribArrayARB( 6 );
		glDisableClientState( GL_NORMAL_ARRAY );
	}

	if( !setArraysOnce )
	{
		R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	GLimp_LogComment( 1, "END RB_ShadeDrawDeluxeMap( %s )", input->shader->name );

	return sg;
}

static stateGroup_t RB_ShadeSkipStage( const shaderCommands_t *input, int stage, stateGroup_t sgPrev, qboolean restorePrior )
{
	/*
		The caller expects state management to happen here,
		so do that but *nothing* else...
	*/

	stateGroup_t sg;

	GLimp_LogComment( 1, "BEGIN RB_ShadeSkipStage( %s )", input->shader->name );

	sg = R_StateBeginGroup();

	if( restorePrior )
		R_StateRestorePriorGroupStates( sgPrev );
	else
		R_StateRestoreGroupStates( sgPrev );

	GLimp_LogComment( 1, "END RB_ShadeSkipStage( %s )", input->shader->name );

	return sg;
}

static qboolean R_IsSpecStage( shaderStage_t *pStage )
{
	const textureEnv_t *env0 = &pStage->bundle[0].texEnv;
	const textureEnv_t *env1 = &pStage->bundle[1].texEnv;

	if( pStage->rgbGen == CGEN_LIGHTING_DIFFUSE && pStage->alphaGen == AGEN_LIGHTING_SPECULAR &&
		pStage->bundle[0].image[0] &&
		pStage->bundle[1].numImageAnimations <= 1 && pStage->bundle[1].image[0] == tr.whiteImage &&
		!env0->complex && env0->color_mode == GL_MODULATE &&
		env1->complex && pStage->bundle[0].tcGen == TCGEN_TEXTURE )
	{
		if( env1->color_mode == GL_ADD && env1->alpha_mode == GL_REPLACE &&
			env1->src[0].color_src == GL_PREVIOUS && env1->src[0].color_op == GL_SRC_COLOR &&
			env1->src[0].alpha_src == GL_PREVIOUS && env1->src[0].alpha_op == GL_SRC_ALPHA &&
			env1->src[1].color_src == GL_PREVIOUS && env1->src[1].color_op == GL_SRC_ALPHA &&
			env1->src[1].alpha_src == GL_PREVIOUS && env1->src[1].alpha_op == GL_SRC_ALPHA )
		{
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean R_IsDiffuseStage( shaderStage_t *pStage )
{
	if( (pStage->rgbGen == CGEN_LIGHTING_DIFFUSE || pStage->rgbGen == CGEN_INVERSE_LIGHTING_DIFFUSE) &&
		pStage->alphaGen == AGEN_IDENTITY &&
		pStage->bundle[0].image[0] && !pStage->bundle[1].image[0] &&
		!pStage->bundle[0].texEnv.complex && pStage->bundle[0].texEnv.color_mode == GL_MODULATE &&
		pStage->bundle[0].tcGen == TCGEN_TEXTURE )
	{
		return qtrue;
	}

	return qfalse;
}

void R_ShadeComputeOptimalGenericStageIteratorFuncs( shader_t *shader, shaderStage_t *stages )
{
	int stage;
		
	qboolean skip = qfalse;

	if( Q_stricmp( shader->special, "starmap:planet-dnch" ) == 0 &&
		stages[0].active && stages[1].active && stages[2].active &&
		R_IsSpecStage( stages + 0 ) &&
		stages[1].rgbGen == CGEN_INVERSE_LIGHTING_DIFFUSE &&
		((stages[1].stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE)) &&
		stages[2].rgbGen == CGEN_LIGHTING_DIFFUSE &&
		((stages[2].stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)) )
	{
		//stages[0].customDraw = RB_ShadeDrawPlanetNDCH;
		//stages[1].customDraw = RB_ShadeSkipStage;
		//stages[2].customDraw = RB_ShadeSkipStage;

		//return;
	}

	for( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = stages + stage;
		shaderStage_t *pNextStage = 0;

		if( !pStage->active )
			break;

		if( stage < MAX_SHADER_STAGES - 1 && stages[stage + 1].active )
			pNextStage = stages + stage + 1;

		if( skip )
		{
			pStage->customDraw = RB_ShadeSkipStage;
			skip = qfalse;
			continue;
		}

		pStage->customDraw = NULL;
		
		if( pNextStage && pNextStage->normalMap )
		{
			if( R_ShadeSupportsDeluxeMapping() && pStage->deluxeMap )
			{
				pStage->customDraw = RB_ShadeDrawDeluxeMap;


				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
				pStage->bundle[1].tcGen = TCGEN_LIGHTMAP;

				shader->neededAttribs |= VA_NORMAL | VA_TANGENT | VA_BINORMAL;

				skip = qtrue; //RB_ShadeDrawDeluxeMap will draw both this and the next, don't let the next also draw
			}
			continue;
		}

		if( pStage->bundle[1].image[0] && shader->multitextureEnv )
		{
			pStage->customDraw = DrawMultitextured;
			continue;
		}

		if(	R_SpIsStandardVertexProgramSupported( SPV_SPEC ) &&
			R_SpIsStandardFragmentProgramSupported( SPF_SPEC ) &&
			R_IsSpecStage( pStage ) )
		{
			if( pStage->normalMap && R_SpIsStandardVertexProgramSupported( SPV_SPEC_NM ) &&
				R_SpIsStandardFragmentProgramSupported( SPF_SPEC_NM ) )
			{
				shader->neededAttribs |= VA_NORMAL | VA_TANGENT | VA_BINORMAL;
				pStage->customDraw = RB_ShadeDrawEntitySpecularNormalMapped;
			}
			else
				pStage->customDraw = RB_ShadeDrawEntitySpecular;

			if( pNextStage )
			{
				if( pNextStage->rgbGen == CGEN_SKIP )
					pNextStage->rgbGen = pStage->rgbGen;
				if( pNextStage->alphaGen == AGEN_SKIP )
					pNextStage->alphaGen = pStage->alphaGen;
			}

			pStage->rgbGen = CGEN_SKIP;
			pStage->alphaGen = AGEN_SKIP;

			continue;
		}

		if( R_SpIsStandardVertexProgramSupported( SPV_DIFFUSE ) &&
			R_SpIsStandardFragmentProgramSupported( SPF_DIFFUSE ) &&
			R_IsDiffuseStage( pStage ) )
		{
			pStage->customDraw = (pStage->rgbGen == CGEN_INVERSE_LIGHTING_DIFFUSE) ?
				RB_ShadeDrawEntityInverseDiffuse : RB_ShadeDrawEntityDiffuse;

			if( pNextStage )
			{
				if( pNextStage->rgbGen == CGEN_SKIP )
					pNextStage->rgbGen = pStage->rgbGen;
				if( pNextStage->alphaGen == AGEN_SKIP )
					pNextStage->alphaGen = pStage->alphaGen;
			}

			pStage->rgbGen = CGEN_SKIP;
			pStage->alphaGen = AGEN_SKIP;

			continue;
		}
	}
}

/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void )
{
	shaderCommands_t *input = &tess;

	RB_DeformTessGeometry();

	GLimp_LogComment( 1, "BEGIN RB_StageIteratorGeneric( %s )", input->shader->name );
	
	//
	// if there is only a single pass then we can enable color
	// and texture arrays before we compile, otherwise we need
	// to avoid compiling those arrays since they will change
	// during multipass rendering
	//
	if ( tess.numPasses > 1 || input->shader->multitextureEnv )
	{
		setArraysOnce = qfalse;
		glDisableClientState (GL_COLOR_ARRAY);

		R_StateSetActiveClientTmuUntracked( GL_TEXTURE1 );
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);

		R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}
	else
	{
		setArraysOnce = qtrue;

		glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

		if( input->shader->stages[ 0 ] )
		{
			ComputeColors( input->shader->stages[0] );
			ComputeTexCoords( input->shader->stages[0] );
		}

		glEnableClientState( GL_COLOR_ARRAY);
		glColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

		if ( input->shader->stages[ 0 ] && input->shader->stages[ 0 ]->bundle[ 1 ].image[ 0 ] )
		{
			R_StateSetActiveClientTmuUntracked( GL_TEXTURE1 );
			glEnableClientState( GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[1] );
		}

		R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );
	}

	//
	// lock XYZ
	//
	glVertexPointer( 3, GL_FLOAT, 16, input->xyz ); // padded for SIMD
	if( GLEW_EXT_compiled_vertex_array )
		glLockArraysEXT( 0, input->numVertexes );

	{
		stateGroup_t sg = R_StateBeginGroup();
		R_SetCull( input->shader->cullType );

		if( input->shader->polygonOffset )
			R_StateSetPolygonOffset( r_offsetFactor->value * input->shader->polygonOffset,
				r_offsetUnits->value * input->shader->polygonOffset );

		//
		// call shader function
		//
		sg = RB_IterateStagesGeneric( input, sg );

		// 
		// now do any dynamic lighting needed
		//
		if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE
			&& !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) ) {
			sg = ProjectDlightTexture( sg );
		}

		//
		// now do fog
		//
		if ( tess.fogNum && tess.shader->fogPass ) {
			RB_FogPass( sg );
		}
	}

	// 
	// unlock arrays
	//
	if( GLEW_EXT_compiled_vertex_array )
		glUnlockArraysEXT();

	if( setArraysOnce )
		glPopClientAttrib();

	GLimp_LogComment( 1, "END RB_StageIteratorGeneric( %s )", input->shader->name );
}


/*
** RB_StageIteratorVertexLitTexture
*/
void RB_StageIteratorVertexLitTexture( void )
{
	shaderCommands_t *input;
	shader_t		*shader;
	
	input = &tess;

	shader = input->shader;

	//
	// compute colors
	//
	RB_CalcDiffuseColor( (unsigned char *) tess.svars.colors );

	GLimp_LogComment( 1, "BEGIN RB_StageIteratorVertexLitTexturedUnfogged( %s )", input->shader->name );

	//
	// set arrays and lock
	//

	glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

	glEnableClientState( GL_COLOR_ARRAY);
	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

	R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][0] );

	glVertexPointer( 3, GL_FLOAT, 16, input->xyz );

	if( GLEW_EXT_compiled_vertex_array )
		glLockArraysEXT( 0, input->numVertexes );

	{
		stateGroup_t sg = R_StateBeginGroup();

		R_SetCull( input->shader->cullType );
		GL_State( tess.xstages[0]->stateBits );
		R_BindAnimatedImage( &tess.xstages[0]->bundle[0], GL_TEXTURE0 );

		R_StateRestorePriorGroupStates( sg );

		R_DrawElements( input->numIndexes, input->indexes );

		// 
		// now do any dynamic lighting needed
		//
		if( tess.dlightBits && tess.shader->sort <= SS_OPAQUE )
			sg = ProjectDlightTexture( sg );

		//
		// now do fog
		//
		if( tess.fogNum && tess.shader->fogPass )
			sg = RB_FogPass( sg );
	}

	// 
	// unlock arrays
	//
	if( GLEW_EXT_compiled_vertex_array )
		glUnlockArraysEXT();

	glPopClientAttrib();

	GLimp_LogComment( 1, "END RB_StageIteratorVertexLitTexturedUnfogged( %s )", input->shader->name );
}

void RB_StageIteratorLightmappedMultitexture( void )
{
	shaderCommands_t *input = &tess;

	GLimp_LogComment( 1, "BEGIN RB_StageIteratorLightmappedMultitexture( %s )", input->shader->name );
	
	glVertexPointer( 3, GL_FLOAT, 16, input->xyz );

	glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

	glDisableClientState( GL_COLOR_ARRAY );
	glColor4f( 1, 1, 1, 1 );

	R_StateSetActiveClientTmuUntracked( GL_TEXTURE1 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][1] );

	R_StateSetActiveClientTmuUntracked( GL_TEXTURE0 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][0] );

	if( GLEW_EXT_compiled_vertex_array )
		glLockArraysEXT( 0, input->numVertexes );

	{
		stateGroup_t sg = R_StateBeginGroup();
	
		R_SetCull( input->shader->cullType );
		GL_State( GLS_DEFAULT );
		
		R_BindAnimatedImage( &tess.xstages[0]->bundle[0], GL_TEXTURE0 );
		R_BindAnimatedImage( &tess.xstages[0]->bundle[1], GL_TEXTURE1 );

		R_StateSetTextureEnvMode( r_lightmap->integer ? GL_REPLACE : GL_MODULATE, GL_TEXTURE1 );


		R_StateRestorePriorGroupStates( sg );

		R_DrawElements( input->numIndexes, input->indexes );

		if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE )
			sg = ProjectDlightTexture( sg );

		if( tess.fogNum && tess.shader->fogPass )
			sg = RB_FogPass( sg );
	}

	if( GLEW_EXT_compiled_vertex_array )
		glUnlockArraysEXT();

	glPopClientAttrib();

	GLimp_LogComment( 1, "END RB_StageIteratorLightmappedMultitexture( %s )", input->shader->name );
}

/*
** RB_EndSurface
*/
void RB_EndSurface( void )
{
	shaderCommands_t *input;

	input = &tess;

	if( input->numIndexes == 0 )
		return;

	if( input->indexes[SHADER_MAX_INDEXES-1] != 0 )
		ri.Error( ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit" );

	if( input->xyz[SHADER_MAX_VERTEXES-1][0] != 0 )
		ri.Error( ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit" );

	if( tess.shader == tr.shadowShader )
	{
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if( r_debugSort->integer && r_debugSort->integer < tess.shader->sort )
		return;
	
	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//

	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if( r_showtris->integer )
		DrawTris( input );

	if( r_shownormals->integer )
		DrawNormals( input );

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
}

