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
	i = 0x5F375A86 - (i >> 1);
	r = *(float*)&i;
	
	r = r * (1.5F - r * r * y);
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

NOGLOBALALIAS void quat_interp( float * RESTRICT o, const float * RESTRICT a, const float * RESTRICT b, float t )
{
	float cosTheta;
	float ta, tb;
	bool neg, norm;

	cosTheta = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];

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

	o[0] = a[0] * ta + b[0] * tb;
	o[1] = a[1] * ta + b[1] * tb;
	o[2] = a[2] * ta + b[2] * tb;
	o[3] = a[3] * ta + b[3] * tb;

	if( norm )
	{
		float l = o[0] * o[0] + o[1] * o[1] + o[2] * o[2] + o[3] * o[3];
		l = rsqrt( l );

		o[0] *= l;
		o[1] *= l;
		o[2] *= l;
		o[3] *= l;
	}
}