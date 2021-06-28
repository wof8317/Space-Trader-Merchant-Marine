/*
===========================================================================
Copyright (C) 2006 Hermitworks Entertainment Corp

This file is part of Space Trader source code.

Space Trader source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Space Trader source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Space Trader source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "tr_local.h"
#include "tr_x42-local.h"

static void R_AnimateX42VertsTangentBasis( const affine_t *infs, uint numInfs, uint maxInfsPerVert,
	const x42vertAnim_t * RESTRICT modVertPos, const x42vertTangent_t * RESTRICT modvertTan, uint numVerts,

	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride,
	vec4_t * RESTRICT oTan, size_t oTanStride, vec4_t * RESTRICT oBin, size_t oBinStride )
{
	uint i;

	for( i = 0; i < numVerts; i++ )
	{
		uint j;

		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertTangent_t * RESTRICT vt = modvertTan + i;
		
		affine_t m;
		Affine_Scale( &m, infs + vp->idx[0], vp->wt[0] );
		for( j = 1; j < maxInfsPerVert; j++ )
			Affine_Mad( &m, infs + vp->idx[j], vp->wt[j], &m );

		Affine_MulPos( *oPos, &m, vp->pos );

		Affine_MulVec( *oTan, &m, vt->tan );
		Affine_MulVec( *oBin, &m, vt->bit );

		{
			vec3_t n;
			Vec3_Cross( n, *oTan, *oBin );		
			VectorScale( n, vt->nfac0 / VectorLength( n ), *oNorm );
		}

		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
		oTan = (vec4_t*)((size_t)oTan + oTanStride);
		oBin = (vec4_t*)((size_t)oBin + oBinStride);
	}
}

static void R_AnimateX42VertsNormsOnly( const affine_t *infs, uint numInfs, uint maxInfsPerVert,
	const x42vertAnim_t * RESTRICT modVertPos, const x42vertNormal_t * RESTRICT modVertNorm, uint numVerts,
	
	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride )
{
	uint i;

	for( i = 0; i < numVerts; i++ )
	{
		uint j;

		vec3_t n;
		float len;

#ifndef X42_SKIP_INVERSE_TRANSPOSE
		float it[3][3], det;
#endif

		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertNormal_t * RESTRICT vn = modVertNorm + i;
		
		affine_t m;
		Affine_Scale( &m, infs + vp->idx[0], vp->wt[0] );
		for( j = 1; j < maxInfsPerVert; j++ )
			Affine_Mad( &m, infs + vp->idx[j], vp->wt[j], &m );

		Affine_MulPos( *oPos, &m, vp->pos );

#ifdef X42_SKIP_INVERSE_TRANSPOSE
		Affine_MulVec( n, &m, vn->norm );
#else
		/*
			Compute the cofactor matrix into it.
		*/
		it[0][0] = m.axis[1][1] * m.axis[2][2] - m.axis[1][2] * m.axis[2][1];
		it[0][1] = m.axis[1][2] * m.axis[2][0] - m.axis[1][0] * m.axis[2][2];
		it[0][2] = m.axis[1][0] * m.axis[2][1] - m.axis[1][1] * m.axis[2][0];

		it[1][0] = m.axis[2][1] * m.axis[0][2] - m.axis[2][2] * m.axis[0][1];
		it[1][1] = m.axis[2][2] * m.axis[0][0] - m.axis[2][0] * m.axis[0][2];
		it[1][2] = m.axis[2][0] * m.axis[0][1] - m.axis[2][1] * m.axis[0][0];

		it[2][0] = m.axis[0][1] * m.axis[1][2] - m.axis[0][2] * m.axis[1][1];
		it[2][1] = m.axis[0][2] * m.axis[1][0] - m.axis[0][0] * m.axis[1][2];
		it[2][2] = m.axis[0][0] * m.axis[1][1] - m.axis[0][1] * m.axis[1][0];

		/*
			Compute the determinant.
		*/
		det = m.axis[0][0] * it[0][0] + m.axis[0][1] * it[0][1] + m.axis[0][2] * it[0][2];

		/*
			Complete the inverse. Note that we're not taking the adjoint. This would be
			redundant since we're computing an inverse-transpose:

				adj( transpose( A ) ) = cof( transpose( transpose( A ) ) ) = cof( A )
		*/

		det = 1.0F / det;
		
		it[0][0] *= det;
		it[0][1] *= det;
		it[0][2] *= det;
		
		it[1][0] *= det;
		it[1][1] *= det;
		it[1][2] *= det;

		it[2][0] *= det;
		it[2][1] *= det;
		it[2][2] *= det;

		n[0] = it[0][0] * vn->norm[0] + it[1][0] * vn->norm[1] + it[2][0] * vn->norm[2];
		n[1] = it[0][1] * vn->norm[0] + it[1][1] * vn->norm[1] + it[2][1] * vn->norm[2];
		n[2] = it[0][2] * vn->norm[0] + it[1][2] * vn->norm[1] + it[2][2] * vn->norm[2];
#endif
		
		//normalize and write the result
		len = VectorLength( n );
		VectorScale( n, 1.0F / len, *oNorm );

		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
	}
}

static void R_AnimateX42VertsNoNormals( const affine_t *infs, uint numInfs, uint maxInfsPerVert,
	const x42vertAnim_t * RESTRICT modVertPos, uint numVerts,
	
	vec4_t * RESTRICT oPos, size_t oPosStride )
{
	uint i;

	for( i = 0; i < numVerts; i++ )
	{
		uint j;

		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		
		affine_t m;
		Affine_Scale( &m, infs + vp->idx[0], vp->wt[0] );
		for( j = 1; j < maxInfsPerVert; j++ )
			Affine_Mad( &m, infs + vp->idx[j], vp->wt[j], &m );

		Affine_MulPos( *oPos, &m, vp->pos );

		oPos = (vec4_t*)((size_t)oPos + oPosStride);
	}
}

static void R_CopyX42VertsTangentBasis( const x42vertAnim_t * RESTRICT modVertPos,
	const x42vertNormal_t * RESTRICT modVertNorm, const x42vertTangent_t * RESTRICT modvertTan,
	uint numVerts, vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride,
	vec4_t * RESTRICT oTan, size_t oTanStride, vec4_t * RESTRICT oBin, size_t oBinStride )
{
	uint i;

	for( i = 0; i < numVerts; i++ )
	{
		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertNormal_t * RESTRICT vn = modVertNorm + i;
		const x42vertTangent_t * RESTRICT vt = modvertTan + i;

		VectorCopy( vp->pos, *oPos );
		VectorCopy( vn->norm, *oNorm );
		VectorCopy( vt->tan, *oTan );
		VectorCopy( vt->bit, *oBin );

		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
		oTan = (vec4_t*)((size_t)oTan + oTanStride);
		oBin = (vec4_t*)((size_t)oBin + oBinStride);	
	}
}

static void R_CopyX42VertsNormsOnly( const x42vertAnim_t * RESTRICT modVertPos,
	const x42vertNormal_t * RESTRICT modVertNorm, uint numVerts, vec4_t * RESTRICT oPos,
	size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride )
{
	uint i;

	for( i = 0; i < numVerts; i++ )
	{
		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertNormal_t * RESTRICT vn = modVertNorm + i;

		VectorCopy( vp->pos, *oPos );
		VectorCopy( vn->norm, *oNorm );

		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
	}
}

static void R_CopyX42VertsNoNormals( const x42vertAnim_t * RESTRICT modVertPos, uint numVerts,
	vec4_t * RESTRICT oPos, size_t oPosStride )
{
	uint i;

	for( i = 0; i < numVerts; i++ )
	{
		const x42vertAnim_t * RESTRICT vp = modVertPos + i;

		VectorCopy( vp->pos, *oPos );

		oPos = (vec4_t*)((size_t)oPos + oPosStride);
	}
}

bool R_AnimateX42Group( const x42Model_t *mod, const x42group_t *g, const x42Pose_t *pose,

	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride,
	vec4_t * RESTRICT oTan, size_t oTanStride, vec4_t * RESTRICT oBin, size_t oBinStride )
{
	uint i;

	bool copyOnly = g->maxVertInfluences == 0;
	
	affine_t infs[X42_MAX_INFLUENCES_PER_BATCH_V5];
	if( !copyOnly )
	{
		for( i = 0; i < g->numInfluences; i++ )
		{
			const x42influence_t *inf = mod->influences + g->influences[i];

			Affine_Mul( infs + i,
				(inf->bone != X42_MODEL_BONE) ? pose->boneMats + inf->bone : &x42_m2q3,
				&inf->meshToBone );
		}
	}

#ifndef TR_COMPILE_NO_SSE
	if( R_AnimSupportsSSE() )
		R_x42SSEBegin();
#endif

	if( oTan && oBin && mod->vertTan )
	{
		if( copyOnly )
		{

#ifndef TR_COMPILE_NO_SSE
			if( R_AnimSupportsSSE() )
			{
				R_CopyX42VertsTangentBasis_SSE( mod->vertPos + g->firstVert, mod->vertNorm + g->firstVert,
					mod->vertTan + g->firstVert, g->numVerts, oPos, oPosStride, oNorm, oNormStride,
					oTan, oTanStride, oBin, oBinStride );
			}
			else
#endif
			{
				R_CopyX42VertsTangentBasis( mod->vertPos + g->firstVert, mod->vertNorm + g->firstVert,
					mod->vertTan + g->firstVert, g->numVerts, oPos, oPosStride, oNorm, oNormStride,
					oTan, oTanStride, oBin, oBinStride );
			}

		}
		else
		{
			//not a simple copy

#ifndef TR_COMPILE_NO_SSE
			if( R_AnimSupportsSSE() )
			{
				R_AnimateX42VertsTangentBasis_SSE( infs, g->numInfluences, g->maxVertInfluences, mod->vertPos + g->firstVert,
					mod->vertTan + g->firstVert, g->numVerts,
					oPos, oPosStride, oNorm, oNormStride, oTan, oTanStride, oBin, oBinStride );
			}
			else
#endif
			{
				R_AnimateX42VertsTangentBasis( infs, g->numInfluences, g->maxVertInfluences, mod->vertPos + g->firstVert,
					mod->vertTan + g->firstVert, g->numVerts,
					oPos, oPosStride, oNorm, oNormStride, oTan, oTanStride, oBin, oBinStride );
			}

		}
	}
	else if( oNorm && mod->vertNorm )
	{
		if( copyOnly )
		{

#ifndef TR_COMPILE_NO_SSE
			if( R_AnimSupportsSSE() )
			{
				R_CopyX42VertsNormsOnly_SSE( mod->vertPos + g->firstVert, mod->vertNorm + g->firstVert, g->numVerts,
					oPos, oPosStride, oNorm, oNormStride );
			}
			else
#endif
			{
				R_CopyX42VertsNormsOnly( mod->vertPos + g->firstVert, mod->vertNorm + g->firstVert, g->numVerts,
					oPos, oPosStride, oNorm, oNormStride );
			}

		}
		else
		{
			//not copy only

#ifndef TR_COMPILE_NO_SSE
			if( R_AnimSupportsSSE() )
			{
				R_AnimateX42VertsNormsOnly_SSE( infs, g->numInfluences, g->maxVertInfluences, mod->vertPos + g->firstVert,
					mod->vertNorm + g->firstVert, g->numVerts, oPos, oPosStride, oNorm, oNormStride );
			}
			else
#endif
			{
				R_AnimateX42VertsNormsOnly( infs, g->numInfluences, g->maxVertInfluences, mod->vertPos + g->firstVert,
					mod->vertNorm + g->firstVert, g->numVerts, oPos, oPosStride, oNorm, oNormStride );
			}

		}
	}
	else
	{
		if( copyOnly )
		{

			R_CopyX42VertsNoNormals( mod->vertPos + g->firstVert, g->numVerts, oPos, oPosStride );

		}
		else
		{

			R_AnimateX42VertsNoNormals( infs, g->numInfluences, g->maxVertInfluences, mod->vertPos + g->firstVert, g->numVerts,
				oPos, oPosStride );

		}
	}

#ifndef TR_COMPILE_NO_SSE
	if( R_AnimSupportsSSE() )
		R_x42SSEEnd();
#endif

	return true;
}