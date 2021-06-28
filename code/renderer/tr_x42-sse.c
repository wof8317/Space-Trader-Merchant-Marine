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

#ifndef	TR_COMPILE_NO_SSE
#include <xmmintrin.h>

/**************************************************************************
	General SSE support routines.
**************************************************************************/
static uint sse_count;
static uint sse_csr;

void R_x42SSEBegin( void )
{
	uint saveCsr, newCsr;

	if( sse_count++ )
		return;

	saveCsr = _mm_getcsr();
	newCsr = saveCsr;
	
	newCsr |= 0x8000; //flush to zero
	newCsr = (newCsr & ~_MM_MASK_MASK) | (~0 & _MM_MASK_MASK); //mask out all exceptions
	newCsr = (newCsr & ~_MM_EXCEPT_MASK) | 0; //clear exception state

	if( cpuInfo.sse.hasDAZ )
		newCsr |= 0x40;

	_mm_setcsr( newCsr & cpuInfo.sse.mxcsr_mask );

	sse_csr = saveCsr;
}

void R_x42SSEEnd( void )
{
	ASSERT( sse_count );

	if( --sse_count )
		return;

	_mm_setcsr( sse_csr );
}

/**************************************************************************
	Quaternion interpolation and supporting math routines.
**************************************************************************/

#ifdef _MSC_VER
#define DO_INLINE __forceinline
#else
#define DO_INLINE ID_INLINE
#endif

NOGLOBALALIAS static DO_INLINE float rsqrt( float x )
{
	long i;
	float y, r;

	y = x * 0.5F;

	i = *(long*)&x;
	i = 0x5f3759dF - (i >> 1);
	r = *(float*)&i;
	
	r = r * (1.5F - r * r * y);
	
	return r;
}

NOGLOBALALIAS static DO_INLINE float sin_0_HalfPi( float a )
{
	float s, t;

	s = a * a;

	t = -2.39e-08F;
	t *= s;

	t += 2.7526e-06F;
	t *= s;
	
	t += -1.98409e-04F;
	t *= s;
	
	t += 8.3333315e-03F;
	t *= s;
	
	t += -1.666666664e-01F;
	t *= s;
	
	t += 1.0f;
	t *= a;
	
	return t;
}

NOGLOBALALIAS static DO_INLINE float atan2_pos( float y, float x )
{
	float a, d, s, t;
	
	if( y > x )
	{
		a = -x / y;
		d = ((float)M_PI / 2.0F);
	}
	else
	{
		a = y / x;
		d = 0.0F;
	}

	s = a * a;
	
	t = 0.0028662257F;
	t *= s;
	
	t += -0.0161657367F;
	t *= s;
	
	t += 0.0429096138F;
	t *= s;
	
	t += -0.0752896400F;
	t *= s;
	
	t += 0.1065626393F;
	t *= s;
	
	t += -0.1420889944F;
	t *= s;
	
	t += 0.1999355085F;
	t *= s;
	
	t += -0.3333314528F;
	t *= s;
	
	t += 1.0F;
	t *= a;
	
	t += d;
	
	return t;
}

NOGLOBALALIAS void quat_interp_SSE( float * RESTRICT o,
	const float * RESTRICT a, const float * RESTRICT b, float t )
{
	float cosTheta;
	float ta, tb;
	bool neg, norm;

	register __m128 qa = _mm_load_ps( a );
	register __m128 qb = _mm_load_ps( b );
	register __m128 qo;

	{
		__m128 a, b, c, d, e;
		
		a = _mm_mul_ps( qa, qb );
		b = _mm_shuffle_ps( a, a, _MM_SHUFFLE( 0, 1, 2, 3 ) );
		
		c = _mm_add_ps( a, b );
		d = _mm_shuffle_ps( c, c, _MM_SHUFFLE( 1, 0, 0, 1 ) );

		e = _mm_add_ps( c, d );

		_mm_store_ss( &cosTheta, e );
	}

	if( cosTheta < 0.0F )
	{
		cosTheta = -cosTheta;
		neg = true;
	}
	else
		neg = false;

	if( cosTheta >= 1.0F - 1e-5F )
	{
		//quats are very close, just lerp to avoid the division by zero
		ta = 1.0F - t;
		tb = t;

		norm = false;
	}
	else if( cosTheta >= 0.2F )
	{
		//quats are somewhat close - normalized lerp will do
		ta = 1.0F - t;
		tb = t;

		norm = true;
	}
	else
	{
		//do full slerp - quickly
		float sinThetaSqr = 1.0F - (cosTheta * cosTheta);
		float sinThetaInv = rsqrt( sinThetaSqr );
		float theta = atan2_pos( sinThetaSqr * sinThetaInv, cosTheta );

		//we're good to SLERP
		ta = sin_0_HalfPi( (1.0F - t) * theta ) * sinThetaInv;
		tb = sin_0_HalfPi( t * theta ) * sinThetaInv;

		norm = false;
	}

	if( neg )
		tb = -tb;

	qo = _mm_add_ps(
		_mm_mul_ps( qa, _mm_load1_ps( &ta ) ),
		_mm_mul_ps( qb, _mm_load1_ps( &tb ) ) );

	if( norm )
	{
		__m128 a, b, c, d, e;
		
		a = _mm_mul_ps( qo, qo );
		b = _mm_shuffle_ps( a, a, _MM_SHUFFLE( 0, 1, 2, 3 ) );
		
		c = _mm_add_ps( a, b );
		d = _mm_shuffle_ps( c, c, _MM_SHUFFLE( 1, 0, 0, 1 ) );

		e = _mm_add_ps( c, d );
		e = _mm_rsqrt_ss( e );

		qo = _mm_mul_ps( qo,
			_mm_shuffle_ps( e, e, _MM_SHUFFLE( 0, 0, 0, 0 ) ) );
	}

	_mm_store_ps( o, qo );
}

/**************************************************************************
	Vertex animation routines.
**************************************************************************/

bool R_AnimSupportsSSE( void )
{
	return cpuInfo.sse.enabled && !r_nosse->integer;
}

typedef __m128 affine_sse[3];

/*
	Full skinning (generating a tangent basis).
*/

void R_AnimateX42VertsTangentBasis_SSE( const affine_t *iinfs, uint numInfs, uint maxInfsPerVert,
	const x42vertAnim_t * RESTRICT modVertPos, const x42vertTangent_t * RESTRICT modVertTanBasis, uint numVerts,

	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride,
	vec4_t * RESTRICT oTan, size_t oTanStride, vec4_t * RESTRICT oBin, size_t oBinStride )
{
	uint i;

	__m128 infs[X42_MAX_INFLUENCES_PER_BATCH_V5][3];

	ASSERT( sse_count );

	for( i = 0; i < numInfs; i++ )
	{
		const affine_t *m = iinfs + i;

		//put into SSE-ready structures
		infs[i][0] = _mm_set_ps( m->origin[0], m->axis[0][2], m->axis[0][1], m->axis[0][0] );
		infs[i][1] = _mm_set_ps( m->origin[1], m->axis[1][2], m->axis[1][1], m->axis[1][0] );
		infs[i][2] = _mm_set_ps( m->origin[2], m->axis[2][2], m->axis[2][1], m->axis[2][0] );
	}

	for( i = 0; i < numVerts; i++ )
	{
		register uint indices;
		register __m128 m0, m1, m2;
		
		register __m128 ip;
		register __m128 it, ib;

		register __m128 mw;

		register __m128 pos;
		register __m128 tan, bin, norm;

		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertTangent_t * RESTRICT vt = modVertTanBasis + i;
					
		_mm_prefetch( (char*)(modVertPos + i + 2 ), _MM_HINT_NTA );
		
		indices = *(uint*)vp->idx;

		{
			register uint idx = indices & 0xFF;
			register __m128 wt = _mm_load1_ps( vp->wt + 0 );

			m0 = _mm_mul_ps( infs[idx][0], wt );
			m1 = _mm_mul_ps( infs[idx][1], wt );
			m2 = _mm_mul_ps( infs[idx][2], wt );
		}

		_mm_prefetch( (char*)(modVertTanBasis + i + 2 ), _MM_HINT_NTA );

		if( maxInfsPerVert >= 2 && (*(uint*)(vp->wt + 1) & 0x7FFFFFFF) )
		{
			register uint idx = indices >> (1 * 8) & 0xFF;
			register __m128 wt = _mm_load1_ps( vp->wt + 1 );

			m0 = _mm_add_ps( m0, _mm_mul_ps( infs[idx][0], wt ) );
			m1 = _mm_add_ps( m1, _mm_mul_ps( infs[idx][1], wt ) );
			m2 = _mm_add_ps( m2, _mm_mul_ps( infs[idx][2], wt ) );
		}
		
		if( maxInfsPerVert >= 3 && (*(uint*)(vp->wt + 2) & 0x7FFFFFFF) )
		{
			register uint idx = indices >> (2 * 8) & 0xFF;
			register __m128 wt = _mm_load1_ps( vp->wt + 2 );

			m0 = _mm_add_ps( m0, _mm_mul_ps( infs[idx][0], wt ) );
			m1 = _mm_add_ps( m1, _mm_mul_ps( infs[idx][1], wt ) );
			m2 = _mm_add_ps( m2, _mm_mul_ps( infs[idx][2], wt ) );
		}

		if( maxInfsPerVert == 4 && (*(uint*)(vp->wt + 3) & 0x7FFFFFFF) )
		{
			register uint idx = indices >> (3 * 8) & 0xFF;
			register __m128 wt = _mm_load1_ps( vp->wt + 3 );

			m0 = _mm_add_ps( m0, _mm_mul_ps( infs[idx][0], wt ) );
			m1 = _mm_add_ps( m1, _mm_mul_ps( infs[idx][1], wt ) );
			m2 = _mm_add_ps( m2, _mm_mul_ps( infs[idx][2], wt ) );
		}

		/*
			Load up the rest of the vertex data.

			Note that pos is read differently because
			it isn't followed by a valid float (indices).
		*/
					
		ip = _mm_load_ps( vp->pos );
		ip = _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 2, 2, 1, 0 ) );

		it = _mm_load_ps( vt->tan );
		ib = _mm_load_ps( vt->bit );
		
		mw = _mm_shuffle_ps( _mm_shuffle_ps( m0, m1, _MM_SHUFFLE( 3, 3, 3, 3 ) ), m2, _MM_SHUFFLE( 0, 3, 2, 0 ) );			
		
		pos = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );
		
		pos = _mm_add_ps( pos, mw );
				
		_mm_stream_ps( *oPos, pos );

		tan = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( it, it, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( it, it, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( it, it, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );

		bin = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( ib, ib, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( ib, ib, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( ib, ib, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );


		norm = _mm_sub_ps(
			_mm_mul_ps(
				_mm_shuffle_ps( tan, tan, _MM_SHUFFLE( 3, 0, 2, 1 ) ),
				_mm_shuffle_ps( bin, bin, _MM_SHUFFLE( 3, 1, 0, 2 ) )
			),
			_mm_mul_ps(
				_mm_shuffle_ps( tan, tan, _MM_SHUFFLE( 3, 1, 0, 2 ) ),
				_mm_shuffle_ps( bin, bin, _MM_SHUFFLE( 3, 0, 2, 1 ) )
			) );

		norm = _mm_mul_ps( norm, _mm_shuffle_ps( ib, ib, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ); //ib.w = vt.nf1

		//renormalize the normal
		{
			register __m128 nLen;

			nLen = _mm_mul_ps( norm, norm );
			nLen = _mm_add_ps(
				_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 0, 0, 0, 0 ) ),
				_mm_add_ps(
					_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 1, 1, 1, 1 ) ),
					_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 2, 2, 2, 2 ) )
				) );
			norm = _mm_mul_ps( norm, _mm_rsqrt_ps( nLen ) );
		}
				
		//write the rest of the output - completing the cache line
		_mm_stream_ps( *oNorm, norm );
		_mm_stream_ps( *oTan, tan );
		_mm_stream_ps( *oBin, bin );

		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
		oTan = (vec4_t*)((size_t)oTan + oTanStride);
		oBin = (vec4_t*)((size_t)oBin + oBinStride);	
	}

	//make sure everything is in RAM before we leave this function
	_mm_sfence();
}

/*
	Simple skinning (only vertex position and normal).
*/
static void R_AnimateX42VertsNormsOnly_Max1Wt_SSE( const affine_sse *infs,
	const x42vertAnim_t * RESTRICT modVertPos, const x42vertNormal_t * RESTRICT modVertNorm, uint numVerts,
	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride )
{
	uint i;

	for( i = 0; i < numVerts; i++ )
	{
		register __m128 m0, m1, m2;

#ifndef X42_SKIP_INVERSE_TRANSPOSE
		register __m128 t0, t1, t2;
		register __m128 rcpdet;
#endif

		register __m128 ip;
		register __m128 in;

		register __m128 mw;
		register __m128 pos;
		register __m128 norm;

		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertNormal_t * RESTRICT vn = modVertNorm + i;
					
		_mm_prefetch( (char*)(modVertPos + i + 1 ), _MM_HINT_NTA );
		_mm_prefetch( (char*)(modVertNorm + i + 1 ), _MM_HINT_NTA );
		
		{
			register uint idx = vp->idx[0];
			m0 = _mm_load_ps( (float*)(infs[idx] + 0) );
			m1 = _mm_load_ps( (float*)(infs[idx] + 1) );
			m2 = _mm_load_ps( (float*)(infs[idx] + 2) );
		}

		/*
			Load up the rest of the vertex data.

			Note that pos is read differently because
			it isn't followed by a valid float (indices).
		*/
					
		ip = _mm_load_ps( vp->pos );
		ip = _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 2, 2, 1, 0 ) );
		in = _mm_load_ps( vn->norm );
		
		mw = _mm_shuffle_ps( _mm_shuffle_ps( m0, m1, _MM_SHUFFLE( 3, 3, 3, 3 ) ), m2, _MM_SHUFFLE( 0, 3, 2, 0 ) );			
		
		pos = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );
		
		pos = _mm_add_ps( pos, mw );
				
		_mm_stream_ps( *oPos, pos );

#ifdef X42_SKIP_INVERSE_TRANSPOSE
		norm = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );
#else
		/*
			Compute the inverse-transpose of the matrix.
		*/

#define swz120( v ) _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 0, 2, 1 ) )
#define swz201( v ) _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 1, 0, 2 ) )
		
		t0 = _mm_sub_ps( _mm_mul_ps( swz120( m1 ), swz201( m2 ) ), _mm_mul_ps( swz201( m1 ), swz120( m2 ) ) );
		t1 = _mm_sub_ps( _mm_mul_ps( swz201( m0 ), swz120( m2 ) ), _mm_mul_ps( swz120( m0 ), swz201( m2 ) ) );
		t2 = _mm_sub_ps( _mm_mul_ps( swz120( m0 ), swz201( m1 ) ), _mm_mul_ps( swz201( m0 ), swz120( m1 ) ) );

#undef swz201
#undef swz120

		rcpdet = _mm_mul_ps( t0, m0 );
		rcpdet = _mm_add_ss( rcpdet, _mm_add_ss( _mm_shuffle_ps( rcpdet, rcpdet, 1 ), _mm_shuffle_ps( rcpdet, rcpdet, 2 ) ) );
		rcpdet = _mm_rcp_ss( rcpdet );
		rcpdet = _mm_shuffle_ps( rcpdet, rcpdet, _MM_SHUFFLE( 0, 0, 0, 0 ) );

		t0 = _mm_mul_ps( t0, rcpdet );
		t1 = _mm_mul_ps( t1, rcpdet );
		t2 = _mm_mul_ps( t2, rcpdet );
				
		/*
			Transform the normal.
		*/

		norm = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 0, 0, 0, 0 ) ), t0 ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 1, 1, 1, 1 ) ), t1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 2, 2, 2, 2 ) ), t2 ) );
#endif
		//renormalize the normal
		{
			register __m128 nLen;

			nLen = _mm_mul_ps( norm, norm );
			nLen = _mm_add_ps(
				_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 0, 0, 0, 0 ) ),
				_mm_add_ps(
					_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 1, 1, 1, 1 ) ),
					_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 2, 2, 2, 2 ) )
				) );
			norm = _mm_mul_ps( norm, _mm_rsqrt_ps( nLen ) );
		}

		//write the last of the vert
		_mm_stream_ps( *oNorm, norm );
		
		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
	}
}

static void R_AnimateX42VertsNormsOnly_Max2Wt_SSE( const affine_sse *infs,
	const x42vertAnim_t * RESTRICT modVertPos, const x42vertNormal_t * RESTRICT modVertNorm, uint numVerts,
	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride )
{
	uint i;

	for( i = 0; i < numVerts; i++ )
	{
		register uint indices;
		register __m128 m0, m1, m2;

#ifndef X42_SKIP_INVERSE_TRANSPOSE
		register __m128 t0, t1, t2;
		register __m128 rcpdet;
#endif

		register __m128 ip;
		register __m128 in;

		register __m128 mw;
		register __m128 pos;
		register __m128 norm;

		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertNormal_t * RESTRICT vn = modVertNorm + i;
					
		_mm_prefetch( (char*)(modVertPos + i + 1 ), _MM_HINT_NTA );
		_mm_prefetch( (char*)(modVertNorm + i + 1 ), _MM_HINT_NTA );
		
		indices = *(uint*)vp->idx;

		{
			register uint idx;
			register __m128 wt;

			idx = indices & 0xFF;
			wt = _mm_load1_ps( vp->wt + 0 );

			m0 = _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 0) ), wt );
			m1 = _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 1) ), wt );
			m2 = _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 2) ), wt );
		
			idx = indices >> (1 * 8) & 0xFF;
			wt = _mm_load1_ps( vp->wt + 1 );
			
			m0 = _mm_add_ps( m0, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 0) ), wt ) );
			m1 = _mm_add_ps( m1, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 1) ), wt ) );
			m2 = _mm_add_ps( m2, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 2) ), wt ) );
		}

		/*
			Load up the rest of the vertex data.

			Note that pos is read differently because
			it isn't followed by a valid float (indices).
		*/
					
		ip = _mm_load_ps( vp->pos );
		ip = _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 2, 2, 1, 0 ) );
		in = _mm_load_ps( vn->norm );
		
		mw = _mm_shuffle_ps( _mm_shuffle_ps( m0, m1, _MM_SHUFFLE( 3, 3, 3, 3 ) ), m2, _MM_SHUFFLE( 0, 3, 2, 0 ) );			
		
		pos = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );
		
		pos = _mm_add_ps( pos, mw );
				
		_mm_stream_ps( *oPos, pos );

#ifdef X42_SKIP_INVERSE_TRANSPOSE
		norm = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );
#else
		/*
			Compute the inverse-transpose of the matrix.
		*/

#define swz120( v ) _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 0, 2, 1 ) )
#define swz201( v ) _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 1, 0, 2 ) )
		
		t0 = _mm_sub_ps( _mm_mul_ps( swz120( m1 ), swz201( m2 ) ), _mm_mul_ps( swz201( m1 ), swz120( m2 ) ) );
		t1 = _mm_sub_ps( _mm_mul_ps( swz201( m0 ), swz120( m2 ) ), _mm_mul_ps( swz120( m0 ), swz201( m2 ) ) );
		t2 = _mm_sub_ps( _mm_mul_ps( swz120( m0 ), swz201( m1 ) ), _mm_mul_ps( swz201( m0 ), swz120( m1 ) ) );

#undef swz201
#undef swz120

		rcpdet = _mm_mul_ps( t0, m0 );
		rcpdet = _mm_add_ss( rcpdet, _mm_add_ss( _mm_shuffle_ps( rcpdet, rcpdet, 1 ), _mm_shuffle_ps( rcpdet, rcpdet, 2 ) ) );
		rcpdet = _mm_rcp_ss( rcpdet );
		rcpdet = _mm_shuffle_ps( rcpdet, rcpdet, _MM_SHUFFLE( 0, 0, 0, 0 ) );

		t0 = _mm_mul_ps( t0, rcpdet );
		t1 = _mm_mul_ps( t1, rcpdet );
		t2 = _mm_mul_ps( t2, rcpdet );
				
		/*
			Transform the normal.
		*/

		norm = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 0, 0, 0, 0 ) ), t0 ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 1, 1, 1, 1 ) ), t1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 2, 2, 2, 2 ) ), t2 ) );
#endif

		//renormalize the normal
		{
			register __m128 nLen;

			nLen = _mm_mul_ps( norm, norm );
			nLen = _mm_add_ps(
				_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 0, 0, 0, 0 ) ),
				_mm_add_ps(
					_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 1, 1, 1, 1 ) ),
					_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 2, 2, 2, 2 ) )
				) );
			norm = _mm_mul_ps( norm, _mm_rsqrt_ps( nLen ) );
		}

		//write the last of the vert
		_mm_stream_ps( *oNorm, norm );
		
		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
	}
}

static void R_AnimateX42VertsNormsOnly_Max3Wt_SSE( const affine_sse *infs,
	const x42vertAnim_t * RESTRICT modVertPos, const x42vertNormal_t * RESTRICT modVertNorm, uint numVerts,
	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride )
{
	uint i;

	for( i = 0; i < numVerts; i++ )
	{
		register uint indices;
		register __m128 m0, m1, m2;

#ifndef X42_SKIP_INVERSE_TRANSPOSE
		register __m128 t0, t1, t2;
		register __m128 rcpdet;
#endif

		register __m128 ip;
		register __m128 in;

		register __m128 mw;
		register __m128 pos;
		register __m128 norm;

		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertNormal_t * RESTRICT vn = modVertNorm + i;
					
		_mm_prefetch( (char*)(modVertPos + i + 1 ), _MM_HINT_NTA );
		_mm_prefetch( (char*)(modVertNorm + i + 1 ), _MM_HINT_NTA );
		
		indices = *(uint*)vp->idx;

		{
			register uint idx;
			register __m128 wt;

			idx = indices & 0xFF;
			wt = _mm_load1_ps( vp->wt + 0 );

			m0 = _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 0) ), wt );
			m1 = _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 1) ), wt );
			m2 = _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 2) ), wt );
		
			idx = indices >> (1 * 8) & 0xFF;
			wt = _mm_load1_ps( vp->wt + 1 );
			
			m0 = _mm_add_ps( m0, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 0) ), wt ) );
			m1 = _mm_add_ps( m1, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 1) ), wt ) );
			m2 = _mm_add_ps( m2, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 2) ), wt ) );

			idx = indices >> (2 * 8) & 0xFF;
			wt = _mm_load1_ps( vp->wt + 2 );
			
			m0 = _mm_add_ps( m0, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 0) ), wt ) );
			m1 = _mm_add_ps( m1, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 1) ), wt ) );
			m2 = _mm_add_ps( m2, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 2) ), wt ) );
		}

		/*
			Load up the rest of the vertex data.

			Note that pos is read differently because
			it isn't followed by a valid float (indices).
		*/
					
		ip = _mm_load_ps( vp->pos );
		ip = _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 2, 2, 1, 0 ) );
		in = _mm_load_ps( vn->norm );
		
		mw = _mm_shuffle_ps( _mm_shuffle_ps( m0, m1, _MM_SHUFFLE( 3, 3, 3, 3 ) ), m2, _MM_SHUFFLE( 0, 3, 2, 0 ) );			
		
		pos = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );
		
		pos = _mm_add_ps( pos, mw );
				
		_mm_stream_ps( *oPos, pos );

#ifdef X42_SKIP_INVERSE_TRANSPOSE
		norm = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );
#else
		/*
			Compute the inverse-transpose of the matrix.
		*/

#define swz120( v ) _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 0, 2, 1 ) )
#define swz201( v ) _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 1, 0, 2 ) )
		
		t0 = _mm_sub_ps( _mm_mul_ps( swz120( m1 ), swz201( m2 ) ), _mm_mul_ps( swz201( m1 ), swz120( m2 ) ) );
		t1 = _mm_sub_ps( _mm_mul_ps( swz201( m0 ), swz120( m2 ) ), _mm_mul_ps( swz120( m0 ), swz201( m2 ) ) );
		t2 = _mm_sub_ps( _mm_mul_ps( swz120( m0 ), swz201( m1 ) ), _mm_mul_ps( swz201( m0 ), swz120( m1 ) ) );

#undef swz201
#undef swz120

		rcpdet = _mm_mul_ps( t0, m0 );
		rcpdet = _mm_add_ss( rcpdet, _mm_add_ss( _mm_shuffle_ps( rcpdet, rcpdet, 1 ), _mm_shuffle_ps( rcpdet, rcpdet, 2 ) ) );
		rcpdet = _mm_rcp_ss( rcpdet );
		rcpdet = _mm_shuffle_ps( rcpdet, rcpdet, _MM_SHUFFLE( 0, 0, 0, 0 ) );

		t0 = _mm_mul_ps( t0, rcpdet );
		t1 = _mm_mul_ps( t1, rcpdet );
		t2 = _mm_mul_ps( t2, rcpdet );
				
		/*
			Transform the normal.
		*/

		norm = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 0, 0, 0, 0 ) ), t0 ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 1, 1, 1, 1 ) ), t1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 2, 2, 2, 2 ) ), t2 ) );
#endif

		//renormalize the normal
		{
			register __m128 nLen;

			nLen = _mm_mul_ps( norm, norm );
			nLen = _mm_add_ps(
				_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 0, 0, 0, 0 ) ),
				_mm_add_ps(
					_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 1, 1, 1, 1 ) ),
					_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 2, 2, 2, 2 ) )
				) );
			norm = _mm_mul_ps( norm, _mm_rsqrt_ps( nLen ) );
		}

		//write the last of the vert
		_mm_stream_ps( *oNorm, norm );
		
		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
	}
}

static void R_AnimateX42VertsNormsOnly_Max4Wt_SSE( const affine_sse *infs,
	const x42vertAnim_t * RESTRICT modVertPos, const x42vertNormal_t * RESTRICT modVertNorm, uint numVerts,
	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride )
{
	uint i;

	for( i = 0; i < numVerts; i++ )
	{
		register uint indices;
		register __m128 m0, m1, m2;

#ifndef X42_SKIP_INVERSE_TRANSPOSE
		register __m128 t0, t1, t2;
		register __m128 rcpdet;
#endif

		register __m128 ip;
		register __m128 in;

		register __m128 mw;
		register __m128 pos;
		register __m128 norm;

		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertNormal_t * RESTRICT vn = modVertNorm + i;
					
		_mm_prefetch( (char*)(modVertPos + i + 1 ), _MM_HINT_NTA );
		_mm_prefetch( (char*)(modVertNorm + i + 1 ), _MM_HINT_NTA );
		
		indices = *(uint*)vp->idx;

		_mm_prefetch( (char*)(modVertPos + i + 1 ), _MM_HINT_NTA );
		_mm_prefetch( (char*)(modVertNorm + i + 1 ), _MM_HINT_NTA );
		
		indices = *(uint*)vp->idx;

		{
			register uint idx;
			register __m128 wt;

			idx = indices & 0xFF;
			wt = _mm_load1_ps( vp->wt + 0 );

			m0 = _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 0) ), wt );
			m1 = _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 1) ), wt );
			m2 = _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 2) ), wt );
		
			idx = indices >> (1 * 8) & 0xFF;
			wt = _mm_load1_ps( vp->wt + 1 );
			
			m0 = _mm_add_ps( m0, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 0) ), wt ) );
			m1 = _mm_add_ps( m1, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 1) ), wt ) );
			m2 = _mm_add_ps( m2, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 2) ), wt ) );

			idx = indices >> (2 * 8) & 0xFF;
			wt = _mm_load1_ps( vp->wt + 2 );
			
			m0 = _mm_add_ps( m0, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 0) ), wt ) );
			m1 = _mm_add_ps( m1, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 1) ), wt ) );
			m2 = _mm_add_ps( m2, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 2) ), wt ) );

			idx = indices >> (3 * 8) & 0xFF;
			wt = _mm_load1_ps( vp->wt + 3 );
			
			m0 = _mm_add_ps( m0, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 0) ), wt ) );
			m1 = _mm_add_ps( m1, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 1) ), wt ) );
			m2 = _mm_add_ps( m2, _mm_mul_ps( _mm_load_ps( (float*)(infs[idx] + 2) ), wt ) );
		}

		/*
			Load up the rest of the vertex data.

			Note that pos is read differently because
			it isn't followed by a valid float (indices).
		*/
					
		ip = _mm_load_ps( vp->pos );
		ip = _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 2, 2, 1, 0 ) );
		in = _mm_load_ps( vn->norm );
		
		mw = _mm_shuffle_ps( _mm_shuffle_ps( m0, m1, _MM_SHUFFLE( 3, 3, 3, 3 ) ), m2, _MM_SHUFFLE( 0, 3, 2, 0 ) );			
		
		pos = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( ip, ip, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );
		
		pos = _mm_add_ps( pos, mw );
				
		_mm_stream_ps( *oPos, pos );

#ifdef X42_SKIP_INVERSE_TRANSPOSE
		norm = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 0, 0, 0, 0 ) ), m0 ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 1, 1, 1, 1 ) ), m1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 2, 2, 2, 2 ) ), m2 ) );
#else
		/*
			Compute the inverse-transpose of the matrix.
		*/

#define swz120( v ) _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 0, 2, 1 ) )
#define swz201( v ) _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 1, 0, 2 ) )
		
		t0 = _mm_sub_ps( _mm_mul_ps( swz120( m1 ), swz201( m2 ) ), _mm_mul_ps( swz201( m1 ), swz120( m2 ) ) );
		t1 = _mm_sub_ps( _mm_mul_ps( swz201( m0 ), swz120( m2 ) ), _mm_mul_ps( swz120( m0 ), swz201( m2 ) ) );
		t2 = _mm_sub_ps( _mm_mul_ps( swz120( m0 ), swz201( m1 ) ), _mm_mul_ps( swz201( m0 ), swz120( m1 ) ) );

#undef swz201
#undef swz120

		rcpdet = _mm_mul_ps( t0, m0 );
		rcpdet = _mm_add_ss( rcpdet, _mm_add_ss( _mm_shuffle_ps( rcpdet, rcpdet, 1 ), _mm_shuffle_ps( rcpdet, rcpdet, 2 ) ) );
		rcpdet = _mm_rcp_ss( rcpdet );
		rcpdet = _mm_shuffle_ps( rcpdet, rcpdet, _MM_SHUFFLE( 0, 0, 0, 0 ) );

		t0 = _mm_mul_ps( t0, rcpdet );
		t1 = _mm_mul_ps( t1, rcpdet );
		t2 = _mm_mul_ps( t2, rcpdet );
				
		/*
			Transform the normal.
		*/

		norm = _mm_add_ps( _mm_add_ps(
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 0, 0, 0, 0 ) ), t0 ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 1, 1, 1, 1 ) ), t1 ) ),
			_mm_mul_ps( _mm_shuffle_ps( in, in, _MM_SHUFFLE( 2, 2, 2, 2 ) ), t2 ) );
#endif					

		//renormalize the normal
		{
			register __m128 nLen;

			nLen = _mm_mul_ps( norm, norm );
			nLen = _mm_add_ps(
				_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 0, 0, 0, 0 ) ),
				_mm_add_ps(
					_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 1, 1, 1, 1 ) ),
					_mm_shuffle_ps( nLen, nLen, _MM_SHUFFLE( 2, 2, 2, 2 ) )
				) );
			norm = _mm_mul_ps( norm, _mm_rsqrt_ps( nLen ) );
		}

		//write the last of the vert
		_mm_stream_ps( *oNorm, norm );
		
		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
	}
}

void R_AnimateX42VertsNormsOnly_SSE( const affine_t *iinfs, uint numInfs, uint maxInfsPerVert,
	const x42vertAnim_t * RESTRICT modVertPos, const x42vertNormal_t * RESTRICT modVertNorm, uint numVerts,
	
	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride )
{
	uint i;

	__m128 infs[X42_MAX_INFLUENCES_PER_BATCH_V5][3];

	ASSERT( sse_count );

	for( i = 0; i < numInfs; i++ )
	{
		const affine_t *m = iinfs + i;

		//put into SSE-ready structures
		infs[i][0] = _mm_set_ps( m->origin[0], m->axis[0][2], m->axis[0][1], m->axis[0][0] );
		infs[i][1] = _mm_set_ps( m->origin[1], m->axis[1][2], m->axis[1][1], m->axis[1][0] );
		infs[i][2] = _mm_set_ps( m->origin[2], m->axis[2][2], m->axis[2][1], m->axis[2][0] );
	}

	switch( maxInfsPerVert )
	{
	case 1: R_AnimateX42VertsNormsOnly_Max1Wt_SSE( infs, modVertPos, modVertNorm, numVerts, oPos, oPosStride, oNorm, oNormStride ); break;
	case 2:	R_AnimateX42VertsNormsOnly_Max2Wt_SSE( infs, modVertPos, modVertNorm, numVerts, oPos, oPosStride, oNorm, oNormStride ); break;
	case 3:	R_AnimateX42VertsNormsOnly_Max3Wt_SSE( infs, modVertPos, modVertNorm, numVerts, oPos, oPosStride, oNorm, oNormStride ); break;
	case 4: R_AnimateX42VertsNormsOnly_Max4Wt_SSE( infs, modVertPos, modVertNorm, numVerts, oPos, oPosStride, oNorm, oNormStride ); break;

		NO_DEFAULT;
	}

	//make sure everything is in RAM before we leave this function
	_mm_sfence();
}

void R_CopyX42VertsTangentBasis_SSE( const x42vertAnim_t * RESTRICT modVertPos,
	const x42vertNormal_t * RESTRICT modVertNorm, const x42vertTangent_t * RESTRICT modVertTanBasis,
	uint numVerts, vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride,
	vec4_t * RESTRICT oTan, size_t oTanStride, vec4_t * RESTRICT oBin, size_t oBinStride )
{
	uint i;

	ASSERT( sse_count );

	for( i = 0; i < numVerts; i++ )
	{
		register __m128 ip, in;
		register __m128 it, ib;

		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertNormal_t * RESTRICT vn = modVertNorm + i;
		const x42vertTangent_t * RESTRICT vt = modVertTanBasis + i;

		_mm_prefetch( (char*)(modVertPos + i + 2 ), _MM_HINT_NTA );
		_mm_prefetch( (char*)(modVertNorm + i + 2 ), _MM_HINT_NTA );
		_mm_prefetch( (char*)(modVertTanBasis + i + 2 ), _MM_HINT_NTA );

		ip = _mm_load_ps( vp->pos );
		in = _mm_load_ps( vn->norm );
		it = _mm_load_ps( vt->tan );
		ib = _mm_load_ps( vt->bit );
		
		_mm_stream_ps( *oPos, ip );
		_mm_stream_ps( *oNorm, in );
		_mm_stream_ps( *oTan, it );
		_mm_stream_ps( *oBin, ib );

		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
		oTan = (vec4_t*)((size_t)oTan + oTanStride);
		oBin = (vec4_t*)((size_t)oBin + oBinStride);	
	}

	//make sure everything is in RAM before we leave this function
	_mm_sfence();
}

void R_CopyX42VertsNormsOnly_SSE( const x42vertAnim_t * RESTRICT modVertPos,
	const x42vertNormal_t * RESTRICT modVertNorm, uint numVerts, vec4_t * RESTRICT oPos,
	size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride )
{
	uint i;

	ASSERT( sse_count );

	for( i = 0; i < numVerts; i++ )
	{
		register __m128 ip, in;

		const x42vertAnim_t * RESTRICT vp = modVertPos + i;
		const x42vertNormal_t * RESTRICT vn = modVertNorm + i;
					
		_mm_prefetch( (char*)(modVertPos + i + 2 ), _MM_HINT_NTA );
		_mm_prefetch( (char*)(modVertNorm + i + 2 ), _MM_HINT_NTA );
					
		ip = _mm_load_ps( vp->pos );
		in = _mm_load_ps( vn->norm );
		
		_mm_stream_ps( *oPos, ip );
		_mm_stream_ps( *oNorm, in );

		oPos = (vec4_t*)((size_t)oPos + oPosStride);
		oNorm = (vec4_t*)((size_t)oNorm + oNormStride);
	}

	//make sure everything is in RAM before we leave this function
	_mm_sfence();
}
#endif