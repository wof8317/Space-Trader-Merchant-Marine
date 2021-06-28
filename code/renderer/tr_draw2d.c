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
	Some bezier code.

	Algorithms are based on an adaptive subdivision
	method credited to Jens Gravesen.
*/

static void R_Bez3Split( vec2_t d0[4], vec2_t d1[4], const vec2_t src[4] )
{
    int i, j;
    vec3_t tmp[4][4];
    
    for( i = 0; i < 4; i++ )
		Vec2_Cpy( tmp[0][i], src[i] );

    for( i = 1; i < 4; i++ )
	{  
		for( j = 0; j < 4 - i; j++ )
		{
			vec2_t ha, hb;

			Vec2_Scale( ha, tmp[i - 1][j], 0.5F );
			Vec2_Scale( hb, tmp[i - 1][j + 1], 0.5F );

			Vec2_Add( tmp[i][j], ha, hb );
		}
	}
    
    for( i = 0; i < 4; i++ )
	{
		Vec2_Cpy( d0[i], tmp[i][0] );
		Vec2_Cpy( d1[i], tmp[3 - i][i] );
	}
}

static void R_Bez3Analyze_R( int *pNumDivs, float *pLen, const vec2_t bez[4], float tol )
{
	int i;
	float aLen, cLen;
	  
	aLen = 0.0F;
	for( i = 0; i < 3; i++ )
		aLen += Vec2_Dist( bez[i], bez[i + 1] );

	cLen = Vec2_Dist( bez[0], bez[3] );
	
	if( aLen - cLen > tol )
	{
		vec2_t b0[4], b1[4];

		(*pNumDivs)++;
		R_Bez3Split( b0, b1, bez );
		
		R_Bez3Analyze_R( pNumDivs, pLen, b0, tol );
		R_Bez3Analyze_R( pNumDivs, pLen, b1, tol );
	}
	else
		*pLen += (aLen + cLen) * 0.5F;
}

static void R_Bez3Analyze( int *pNumDivs, float *pLen, const vec2_t bez[4], float tol )
{
	*pNumDivs = 0;
	*pLen = 0;

	R_Bez3Analyze_R( pNumDivs, pLen, bez, tol );
}

static ID_INLINE byte R_Bez3ClampColorChannel( float f )
{
	int c = (int)f;

	if( c < 0 ) c = 0;
	if( c > 0xFF ) c = 0xFF;

	return (byte)c;
}

static ID_INLINE int R_Bez3PackColor( const vec4_t color )
{
	byte ret[4];

	ret[0] = R_Bez3ClampColorChannel( color[0] * 0xFF );
	ret[1] = R_Bez3ClampColorChannel( color[1] * 0xFF );
	ret[2] = R_Bez3ClampColorChannel( color[2] * 0xFF );
	ret[3] = R_Bez3ClampColorChannel( color[3] * 0xFF );

	return *(int*)ret;
}

static ID_INLINE int R_Bez3SampleColor( float t, float t0, float t1, const vec4_t c[2] )
{
	float nt;
	vec4_t cf;

	t = t * (t1 - t0) + t0;
	nt = 1.0F - t;

	cf[0] = c[0][0] * nt + c[1][0] * t;	
	cf[1] = c[0][1] * nt + c[1][1] * t;
	cf[2] = c[0][2] * nt + c[1][2] * t;
	cf[3] = c[0][3] * nt + c[1][3] * t;

	return R_Bez3PackColor( cf );
}

static void R_Bez3Build_R( vec2_t *outPt, vec2_t *outUv, vec4_t *outColor, int *outIdx,
	const vec2_t inPt[4], const vec4_t inColor[2], linTexGen_t texGen, float t0, float t1, float tol )
{
	int i;
	float aLen, cLen;
	  
	aLen = 0.0F;
	for( i = 0; i < 3; i++ )
		aLen += Vec2_Dist( inPt[i], inPt[i + 1] );

	cLen = Vec2_Dist( inPt[0], inPt[3] );
	
	if( aLen - cLen > tol )
	{
		vec2_t b0[4], b1[4];
		float tM;

		R_Bez3Split( b0, b1, inPt );
		tM = (t0 + t1) * 0.5F;
		
		R_Bez3Build_R( outPt, outUv, outColor, outIdx, b0, inColor, texGen, t0, tM, tol );
		R_Bez3Build_R( outPt, outUv, outColor, outIdx, b1, inColor, texGen, tM, t1, tol );
	}
	else
	{
		Vec2_Cpy( outPt[*outIdx + 0], inPt[1] );
		Vec2_Cpy( outPt[*outIdx + 1], inPt[2] );
		Vec2_Cpy( outPt[*outIdx + 2], inPt[3] );
		
		if( inColor )
		{
			Vec4_Lrp( outColor[*outIdx + 0], inColor[0], inColor[1], 0.33F * (t1 - t0) + t0 );
			Vec4_Lrp( outColor[*outIdx + 1], inColor[0], inColor[1], 0.66F * (t1 - t0) + t0 );
			Vec4_Lrp( outColor[*outIdx + 2], inColor[0], inColor[1], t1 );
		}

		if( texGen == LINTEX_LINEAR )
		{
			outUv[*outIdx + 0][0] = 0.33F * (t1 - t0) + t0;
			outUv[*outIdx + 0][1] = 0.0F;
			
			outUv[*outIdx + 1][0] = 0.66F * (t1 - t0) + t0;
			outUv[*outIdx + 1][1] = 0.0F;

			outUv[*outIdx + 2][0] = t1;
			outUv[*outIdx + 2][1] = 0.0F;
		}

		*outIdx += 3;
	}
}

static int R_Bez3Build( vec2_t *outPt, vec2_t *outUv, vec4_t *outColor,
	const vec2_t inPt[4], const vec4_t inColor[2], linTexGen_t texGen, float t0, float t1, float tol )
{
	int outIdx = 0;

	R_Bez3Build_R( outPt, outUv, outColor, &outIdx, inPt, inColor, texGen, t0, t1, tol );

	return outIdx;
}

curve_t* Q_EXTERNAL_CALL RE_CurveCreate( const vec2_t *pts, const vec4_t *colors, const linType_t *lins,
	linTexGen_t texGen, const vec3_t texGenMat[2], const vec3_t mtx[2], float tol, curvePlacement_t placement )
{
	int i;
	int numVerts;
	int numSegs;
	float lineLen;
	float *segLens;
	const linType_t *lin;
	float t0, t1;
	const vec2_t *pv;
	
	size_t vertSize;
	byte *buff;
	curve_t *cv;

	numSegs = 0;
	for( lin = lins; *lin != LIN_END; lin++ )
		if( *lin != LIN_END )
			numSegs++;

	segLens = (float*)alloca( sizeof( float ) * numSegs );

	lineLen = 0;
	numVerts = 1;
	numSegs = 0;

	pv = pts;

	//get the number of verts we're gonna generate
	//and the linear length of the resulting line
	for( lin = lins; *lin != LIN_END; lin++ )
	{
		float len = 0;

		switch( *lin )
		{
		case LIN_LINE:
			len = Vec2_Dist( pv[0], pv[1] );
			pv++;
			numVerts++;
			break;

		case LIN_BEZ3:
			{
				int numDivs;

				R_Bez3Analyze( &numDivs, &len, pv, tol );
				pv += 3;
				numVerts += (numDivs + 1) * 3;
			}
			break;

		default:
			ri.Error( ERR_DROP, "Invalid line type." );
			break;
		}

		segLens[numSegs++] = len;
		lineLen += len;
	}

	vertSize = sizeof( vec2_t );

	if( colors )
		vertSize += sizeof( vec4_t );

	switch( texGen )
	{
	case LINTEX_LINEAR:
	case LINTEX_PLANAR:
		vertSize += sizeof( vec2_t );
		break;
	}

	switch( placement )
	{
	case CVPLACE_HEAP:
		buff = (byte*)ri.Malloc( sizeof( curve_t ) + vertSize * numVerts );
		break;

	case CVPLACE_HUNK:
		buff = (byte*)ri.Hunk_Alloc( sizeof( curve_t ) + vertSize * numVerts, h_low );
		break;

	case CVPLACE_HUNKTEMP:
		buff = (byte*)ri.Hunk_AllocateTempMemory( sizeof( curve_t ) + vertSize * numVerts );
		break;

	case CVPLACE_FRAME:
		buff = (byte*)ri.Hunk_AllocateFrameMemory( sizeof( curve_t ) + vertSize * numVerts );
		break;
		
	default:
		ri.Error( ERR_DROP, "Invalid curve placement." );
		buff = 0; //warning be gone
		break;
	}

	//set up cv and copy the first vert
	cv = (curve_t*)buff;
	buff += sizeof( curve_t );

	cv->numPts = numVerts;

	cv->pts = (vec2_t*)buff;
	buff += sizeof( vec2_t ) * numVerts;
	Vec2_Cpy( cv->pts[0], pts[0] );
		
	cv->placement = placement;

	switch( texGen )
	{
	case LINTEX_NONE:
		cv->uvs = NULL;
		break;

	case LINTEX_LINEAR:
		cv->uvs = (vec2_t*)buff;
		buff += sizeof( vec2_t ) * numVerts;

		cv->uvs[0][0] = 0.0F;
		cv->uvs[0][1] = 0.0F;
		break;

	case LINTEX_PLANAR:
		cv->uvs = (vec2_t*)buff;
		buff += sizeof( vec2_t ) * numVerts;
		break;
	}

	if( colors )
	{
		cv->colors = (vec4_t*)buff;
		memcpy( cv->colors[0], colors[0], sizeof( vec4_t ) );
	}
	else
		cv->colors = NULL;
		
	t0 = 0;
	numSegs = 0;
	numVerts = 1;

	for( lin = lins; *lin != LIN_END; lin++ )
	{
		t1 = t0 + segLens[numSegs] / lineLen;

		switch( *lin )
		{
		case LIN_LINE:
			Vec2_Cpy( cv->pts[numVerts], pts[1] );
			pts++;
			
			if( texGen == LINTEX_LINEAR )
			{
				cv->uvs[numVerts][0] = t1;
				cv->uvs[numVerts][1] = 0.0F;
			}

			if( colors )
			{
				memcpy( cv->colors[numVerts], colors[1], sizeof( vec4_t ) );
				colors++;
			}
			
			numVerts++;
			break;

		case LIN_BEZ3:
			{
				numVerts += R_Bez3Build( cv->pts + numVerts, cv->uvs ? cv->uvs + numVerts : NULL,
					cv->colors ? cv->colors + numVerts : NULL, pts, colors, texGen, t0, t1, tol );

				pts += 3;
				if( colors ) colors++;
			}
			break;
		}

		t0 = t1;
		numSegs++;
	}

	if( texGen == LINTEX_PLANAR )
		for( i = 0; i < numVerts; i++ )
		{
			cv->uvs[i][0] = Vec2_Dot( cv->pts[i], texGenMat[0] ) + texGenMat[0][2];
			cv->uvs[i][1] = Vec2_Dot( cv->pts[i], texGenMat[1] ) + texGenMat[1][2];
		}

	if( mtx )
		for( i = 0; i < numVerts; i++ )
		{
			cv->pts[i][0] = Vec2_Dot( cv->pts[i], mtx[0] ) + mtx[0][2];
			cv->pts[i][1] = Vec2_Dot( cv->pts[i], mtx[1] ) + mtx[1][2];
		}

	return cv;
}

void Q_EXTERNAL_CALL RE_CurveDelete( curve_t *curve )
{
	switch( curve->placement )
	{
	case CVPLACE_HEAP:
		ri.Free( curve );
		break;

	case CVPLACE_HUNK:
		break;

	case CVPLACE_HUNKTEMP:
		ri.Hunk_FreeTempMemory( curve );
		break;

	case CVPLACE_FRAME:
		break;
		
	default:
		ri.Error( ERR_DROP, "Invalid curve placement." );
	}
}

static void R_ShapeTriangulateCurve( const vec2_t *pts, uint numPts, ushort **out )
{
	uint numTris, lastNumTris;
	const float eps = 0.0001F;
	bool *used;

	if( numPts < 3 )
		return;

	//assuming we won't have the figure crossing itself...
	//...that would be very bad indeed...

	//use ear clipping to triangulate
	used = (bool*)alloca( sizeof( bool ) * numPts );
	Com_Memset( used, 0, sizeof( bool ) * numPts );

	numTris = 0;
	lastNumTris = numTris - 1;

	while( numTris < numPts - 2 )
	{
		uint i, ia, ib, ic;
		vec2_t va, vb, vc, ea, eb, ec, xa, xb, xc;
		float dab, dbc, dca;
		bool good;

		if( lastNumTris == numTris )
			//important - if the line self-intersects this is
			//the only thing that will make the loop terminate
			break;

		lastNumTris = numTris;

		for( ia = 0; ia < numPts; ia++ )
		{
			if( used[ia] )
				continue;

			ib = ia;
			do
			{
				if( ib++ == numPts )
					ib = 0;
			} while( used[ib] && ib != ia );

			ic = ib;
			do
			{
				if( ic++ == numPts )
					ic = 0;
			} while( used[ic] && ic != ia && ic != ib );

			if( ia == ib || ia == ic || ib == ic )
				//degenerate
				continue;

			//get vertices
			Vec2_Cpy( va, pts[ia] );
			Vec2_Cpy( vb, pts[ib] );
			Vec2_Cpy( vc, pts[ic] );
			
			//get opposing edge vectors
			Vec2_Sub( ea, vc, vb );
			dbc = Vec2_Dot( ea, ea );
			if( dbc < eps )
				//degenerate triangle
				continue;

			Vec2_Sub( eb, va, vc );
			dca = Vec2_Dot( eb, eb );
			if( dca < eps )
				//degenerate triangle
				continue;

			Vec2_Sub( ec, vb, va );
			dab = Vec2_Dot( ec, ec );
			if( dab < eps )
				//degenerate triangle
				continue;

			if( (ea[0] * eb[1] - ea[1] * eb[0]) > 0 )
				//triangle winds the wrong way - is outside the shape
				continue;

			xa[0] = ea[1];
			xa[1] = -ea[0];
			xb[0] = eb[1];
			xb[1] = -eb[0];
			xc[0] = ec[1];
			xc[1] = -ec[0];
			
			Vec2_Norm( xa, xa );
			Vec2_Norm( xb, xb );
			Vec2_Norm( xc, xc );
			
			//make sure we don't hold any outer vertices
			good = true;
			for( i = 0; i < numPts && good; i++ )
			{
				vec2_t v, da, db, dc;

				if( i == ia || i == ib || i == ic )
					//no need to check the current triangle
					continue;

				Vec2_Cpy( v, pts[i] );

				Vec2_Sub( da, v, va );
				Vec2_Norm( da, da );
				Vec2_Sub( db, v, vb );
				Vec2_Norm( db, db );
				Vec2_Sub( dc, v, vc );
				Vec2_Norm( dc, dc );
				
				if( Vec2_Dot( xb, dc ) < 0 )
					continue;

				if( Vec2_Dot( xa, db ) < 0 )
					continue;

				if( Vec2_Dot( xc, da ) < 0 )
					continue;

				//bad triangle
				good = false;
			}

			if( !good )
				//triangle crosses over an outer edge, discard
				continue;

			//we have our triangle, pull it out
			*((*out)++) = (ushort)ia;
			*((*out)++) = (ushort)ic;
			*((*out)++) = (ushort)ib;
						
			used[ib] = true;
			ia--; //start at ia again, try to make fans if we can...

			if( ++numTris == numPts - 1 )
				break;
		}
	}
}

typedef struct
{
	shapeGen_t	gen;
	uint		numPts;
	uint		numIndices;
} shapeElem_t;

typedef struct
{
	uint		numPts;

	vec2_t		*pts;
	vec2_t		*uvs;
	uint		*cls;

	ushort		*indices;

	uint		numElems;
	shapeElem_t	*elems;
} shape_t;

qhandle_t Q_EXTERNAL_CALL RE_ShapeCreate( const curve_t *curves, const shapeGen_t *genTypes, int numElems )
{
	uint i, j, cVtx, cIdx;
	ushort *idxs;
	shape_t *sh;

	sh = (shape_t*)ri.Hunk_Alloc( sizeof( shape_t ), h_low );
	Com_Memset( sh, 0, sizeof( shape_t ) );

	for( i = 0; i < numElems; i++ )
		sh->numPts += curves[i].numPts;
	
	sh->pts = (vec2_t*)ri.Hunk_Alloc( sizeof( vec2_t ) * sh->numPts, h_low );
	sh->uvs = (vec2_t*)ri.Hunk_Alloc( sizeof( vec2_t ) * sh->numPts, h_low );
	sh->cls = (uint*)ri.Hunk_Alloc( sizeof( uint ) * sh->numPts, h_low );

	sh->numElems = numElems;
	sh->elems = (shapeElem_t*)ri.Hunk_Alloc( sizeof( shapeElem_t ) * numElems, h_low );
	
	//copy all the data in

	cVtx = 0;
	cIdx = 0;
	for( i = 0; i < numElems; i++ )
	{
		const curve_t *cv = curves + i;

		vec2_t *pts = sh->pts + cVtx;
		vec2_t *uvs = sh->uvs + cVtx;
		uint *cls = sh->cls + cVtx;

		cVtx += cv->numPts;

		for( j = 0; j < cv->numPts; j++ )
			Vec2_Cpy( pts[j], cv->pts[j] );

		if( cv->uvs )
		{
			for( j = 0; j < cv->numPts; j++ )
				Vec2_Cpy( uvs[j], cv->uvs[j] );
		}
		else
			Com_Memset( uvs, 0, sizeof( vec2_t ) * cv->numPts );

		if( cv->colors )
		{
			for( j = 0; j < cv->numPts; j++ )
				cls[j] = ColorBytes4( cv->colors[j][0], cv->colors[j][1], cv->colors[j][2], cv->colors[j][3] );
		}
		else
			Com_Memset( cls, ~0, sizeof( uint ) * cv->numPts );

		if( genTypes[i] & SHAPEGEN_FILL )
		{
			if( genTypes[i] & (SHAPEGEN_FILL_USE_INDICES & ~SHAPEGEN_FILL) )
				cIdx += cv->numIndices;
			else
				cIdx += (cv->numPts - 2) * 3;
		}

		if( genTypes[i] & SHAPEGEN_LINE )
		{
			cIdx += cv->numPts;

			if( genTypes[i] & (SHAPEGEN_LINE_LOOP & ~SHAPEGEN_LINE) )
				cIdx += 1;
		}
	}

	idxs = (short*)ri.Hunk_Alloc( sizeof( short) * cIdx, h_low );
	sh->indices = idxs;

	for( i = 0; i < numElems; i++ )
	{
		const curve_t *cv = curves + i;
		
		sh->elems[i].gen = genTypes[i];
		sh->elems[i].numPts = cv->numPts;
				
		if( genTypes[i] & SHAPEGEN_FILL )
		{
			if( genTypes[i] & (SHAPEGEN_FILL_USE_INDICES & ~SHAPEGEN_FILL) )
			{
				Com_Memcpy( idxs, cv->indices, sizeof( short ) * cv->numIndices );
				idxs += cv->numIndices;

				sh->elems[i].numIndices = cv->numIndices;
			}
			else
			{
				ushort *startIdxz = idxs;
				R_ShapeTriangulateCurve( cv->pts, cv->numPts, &idxs );

				sh->elems[i].numIndices = idxs - startIdxz;
			}
		}

		if( genTypes[i] & SHAPEGEN_LINE )
		{
			for( j = 0; j < cv->numPts; j++ )
				*(idxs++) = (short)j;

			sh->elems[i].numIndices = cv->numPts;

			if( genTypes[i] & (SHAPEGEN_LINE_LOOP & ~SHAPEGEN_LINE) )
			{
				*(idxs++) = 0;
				sh->elems[i].numIndices++;
			}
		}
	}

	return (qhandle_t)sh;
}

static void RB_ShapeTess( const shape_t *shape, const vec3_t m[2] )
{
	uint i;
	uint cV, cI;

	cV = 0;
	cI = 0;

	for( i = 0; i < shape->numElems; i++ )
	{
		int bV;
		uint j;
		const shapeElem_t *el = shape->elems + i;

		const vec2_t *pts = shape->pts + cV;
		const vec2_t *uvs = shape->uvs + cV;
		const uint *cls = shape->cls + cV;

		cV += el->numPts;
		
		bV = -1;

		if( el->gen & SHAPEGEN_FILL )
		{
			RB_CheckSurface( tess.shader, tess.fogNum, GL_TRIANGLES );
			RB_CHECKOVERFLOW( el->numPts, el->numIndices );

			bV = tess.numVertexes;
			for( j = 0; j < el->numPts; j++ )
			{
				tess.xyz[bV + j][0] = pts[j][0] * m[0][0] + pts[j][1] * m[0][1] + m[0][2];
				tess.xyz[bV + j][1] = pts[j][0] * m[1][0] + pts[j][1] * m[1][1] + m[1][2];
				tess.xyz[bV + j][2] = 0;

				tess.texCoords[bV + j][0][0] = uvs[j][0];
				tess.texCoords[bV + j][0][1] = uvs[j][1];

				*(uint*)tess.vertexColors[bV + j] = cls[j];
			}

			tess.numVertexes += el->numPts;

			for( j = 0; j < el->numIndices; j++ )
				tess.indexes[tess.numIndexes + j] = shape->indices[cI++] + bV;

			tess.numIndexes += el->numIndices;
		}
		
		if( el->gen & SHAPEGEN_LINE )
		{
			RB_CheckSurface( tess.shader, tess.fogNum, GL_LINE_STRIP );
			RB_CHECKOVERFLOW( el->numPts, el->numIndices );

			if( bV == -1 || tess.numVertexes == 0 )
			{
				bV = tess.numVertexes;
				for( j = 0; j < el->numPts; j++ )
				{
					tess.xyz[bV + j][0] = pts[j][0] * m[0][0] + pts[j][1] * m[0][1] + m[0][2];
					tess.xyz[bV + j][1] = pts[j][0] * m[1][0] + pts[j][1] * m[1][1] + m[1][2];
					tess.xyz[bV + j][2] = 0;

					tess.texCoords[bV + j][0][0] = uvs[j][0];
					tess.texCoords[bV + j][0][1] = uvs[j][1];

					*(uint*)tess.vertexColors[bV + j] = cls[j];
				}

				tess.numVertexes += el->numPts;
			}

			for( j = 0; j < el->numIndices; j++ )
				tess.indexes[tess.numIndexes + j] = shape->indices[cI++] + bV;

			tess.numIndexes += el->numIndices;
		}
	}
}

typedef struct
{
	shader_t			*shader;
	shape_t				*sh;
	vec3_t				m[2];
} shapeDrawCmd_t;

void RB_ShapeDraw( const void *data )
{
	const shapeDrawCmd_t *sh = (const shapeDrawCmd_t*)data;

	if( !backEnd.projection2D )
		RB_SetGL2D();

	RB_CheckSurface( sh->shader, 0, GL_TRIANGLES );
	RB_ShapeTess( sh->sh, sh->m );
}

void Q_EXTERNAL_CALL RE_ShapeDraw( qhandle_t shape, qhandle_t shader, vec3_t m[ 2 ] )
{
	shapeDrawCmd_t *cmd = (shapeDrawCmd_t*)R_GetCommandBuffer( RB_ShapeDraw, sizeof( shapeDrawCmd_t ) );

	if( !cmd )
		return;

	cmd->shader = shader ? R_GetShaderByHandle( shader ) : tr.whiteShader;
	cmd->sh = (shape_t*)shape;
	Com_Memcpy( cmd->m, m, sizeof( cmd->m ) );
}

typedef struct
{
	shader_t			*shader;
	uint				numShapes;
	//shape pointers go inline here
} shapeDrawMultiCmd_t;

void RB_ShapeDrawMulti( const void *data )
{
	uint i;
	const shapeDrawMultiCmd_t *cmd = (const shapeDrawMultiCmd_t*)data;
	const shape_t *shapes = (shape_t*)(cmd + 1);
	const vec3_t m[ 2 ] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };

	if( !backEnd.projection2D )
		RB_SetGL2D();

	RB_CheckSurface( cmd->shader, 0, GL_TRIANGLES );
	
	for( i = 0; i < cmd->numShapes; i++ )
		RB_ShapeTess( shapes + i, m );
}

void Q_EXTERNAL_CALL RE_ShapeDrawMulti( void **shapes, unsigned int numShapes, qhandle_t shader )
{
	shapeDrawMultiCmd_t *cmd;
	
	cmd = (shapeDrawMultiCmd_t*)R_GetCommandBuffer( RB_ShapeDrawMulti, sizeof( *cmd ) + sizeof( shape_t* ) * numShapes );

	if( !cmd )
		return;

	cmd->shader = shader ? R_GetShaderByHandle( shader ) : tr.whiteShader;
	cmd->numShapes = numShapes;
	
	Com_Memcpy( cmd + 1, shapes, sizeof( shape_t* ) * numShapes );
}
