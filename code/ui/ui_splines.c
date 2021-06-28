/*
===========================================================================
Copyright (C) 2006 HermitWorks Entertainment Corporation

This file is part of Space Trader source code.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
===========================================================================
*/

#include "ui_local.h"

/*
	Ye Olde Cubic NURBS code.
*/

//little struct to cache the aweful coefficients
typedef struct blendFunc3_t
{
    float c1_shift, c1_coeff,
        d1_coeff; // = 3 / c1_coeff
    float c2_cubic, c2_quadratic, c2_linear, c2_const,
        d2_quadratic, // = 3 * c2_cubic
        d2_linear; // = 2 * c2_quadratic, d2_const = c2_linear
    float c3_cubic, c3_quadratic, c3_linear, c3_const,
        d3_quadratic, // = 3 * c3_cubic
        d3_linear; // = 2 * c3_quadratic, d3_const = c3_linear
    float c4_shift, c4_coeff,
        d4_coeff; // = 3 / c4_coeff

    const float *u;
} blendFunc3_t;

static void Spl_BlendFunc3Init( blendFunc3_t *f, const float *knots )
{
	const float *u;
	float a, b, c, d;
	float c3, c2, c1, c0;

	u = knots;
	f->u = knots;

	f->c1_shift = -u[0];
	a = ((u[3] - u[0]) * (u[2] - u[0]) * (u[1] - u[0]));
	f->c1_coeff = 1.0F / a;
	f->d1_coeff = 3.0F / a;

	a = u[3] - u[0];
	b = (u[2] - u[0]) * (u[2] - u[1]);
	c = (u[3] - u[1]) * (u[2] - u[1]);
	d = (u[4] - u[1]) * (u[3] - u[1]) * (u[2] - u[1]);

	b *= a;
	c *= a;

	if( b != 0 )
	{
		c3 = -1.0F;
		c2 = 2.0F * u[0] + u[2];
		c1 = -(u[0] * u[0] + 2.0F * u[0] * u[2]);
		c0 = u[0] * u[0] * u[2];

		f->c2_cubic = c3 / b;
		f->c2_quadratic = c2 / b;
		f->c2_linear = c1 / b;
		f->c2_const = c0 / b;
	}
	else
	{
		f->c2_cubic = 0;
		f->c2_quadratic = 0;
		f->c2_linear = 0;
		f->c2_const = 0;
	}

	if( c != 0 )
	{
		c3 = -1.0F;
		c2 = u[0] + u[1] + u[3];
		c1 = -(u[3] * (u[0] + u[1]) + u[0] * u[1]);
		c0 = u[0] * u[1] * u[3];

		f->c2_cubic += c3 / c;
		f->c2_quadratic += c2 / c;
		f->c2_linear += c1 / c;
		f->c2_const += c0 / c;
	}

	if( d != 0 )
	{
		c3 = -1.0F;
		c2 = 2.0F * u[1] + u[4];
		c1 = -(u[1] * u[1] + 2.0F * u[1] * u[4]);
		c0 = u[1] * u[1] * u[4];

		f->c2_cubic += c3 / d;
		f->c2_quadratic += c2 / d;
		f->c2_linear += c1 / d;
		f->c2_const += c0 / d;
	}

	f->d2_quadratic = f->c2_cubic * 3.0F;
	f->d2_linear = f->c2_quadratic * 2.0F;

	a = (u[3] - u[0]) * (u[3] - u[1]) * (u[3] - u[2]);
	b = u[4] - u[1];
	c = (u[3] - u[1]) * (u[3] - u[2]);
	d = (u[4] - u[2]) * (u[3] - u[2]);
		
	c *= b;
	d *= b;

	if( a != 0 )
	{
		c3 = 1.0F;
		c2 = -(2.0F * u[3] + u[0]);
		c1 = u[3] * u[3] + 2.0F * u[0] * u[3];
		c0 = -(u[3] * u[3] * u[0]);

		f->c3_cubic = c3 / a;
		f->c3_quadratic = c2 / a;
		f->c3_linear = c1 / a;
		f->c3_const = c0 / a;
	}
	else
	{
		f->c3_cubic = 0;
		f->c3_quadratic = 0;
		f->c3_linear = 0;
		f->c3_const = 0;
	}

	if( c != 0 )
	{
		c3 = 1.0F;
		c2 = -(u[1] + u[3] + u[4]);
		c1 = u[3] * u[4] + u[1] * (u[3] + u[4]);
		c0 = -(u[1] * u[3] * u[4]);

		f->c3_cubic += c3 / c;
		f->c3_quadratic += c2 / c;
		f->c3_linear += c1 / c;
		f->c3_const += c0 / c;
	}

	if( d != 0 )
	{
		c3 = 1.0F;
		c2 = -(2.0F * u[4] + u[2]);
		c1 = u[4] * u[4] + 2.0F * u[2] * u[4];
		c0 = -(u[2] * u[4] * u[4]);

		f->c3_cubic += c3 / d;
		f->c3_quadratic += c2 / d;
		f->c3_linear += c1 / d;
		f->c3_const += c0 /d;
	}

	f->d3_quadratic = f->c3_cubic * 3.0F;
	f->d3_linear = f->c3_quadratic * 2.0F;

	f->c4_shift = u[4];
	a = (u[4] - u[1]) * (u[4] - u[2]) * (u[4] - u[3]);
	f->c4_coeff = 1.0F / a;
	f->d4_coeff = -3.0F / a;
}

static void Spl_BlendFunc3Eval( /* out */ float *value, /* out */ float *derivative, const blendFunc3_t *f, float u )
{
	float a, a2, a3;

	*value = 0.0F;
	*derivative = 0.0F;

	if( u < f->u[0] || u > f->u[4] )
		//outside the blending function entirely
		return;
		  
	//we know that u >= u0
	if( u < f->u[1] )
	{
		a = u + f->c1_shift;
		a2 = a * a;
		a3 = a2 * a;

		*value = a3 * f->c1_coeff;
		*derivative = a2 * f->d1_coeff;
	}

	if( u >= f->u[1] && u < f->u[2] )
	{
		*value += ((f->c2_cubic * u + f->c2_quadratic) * u + f->c2_linear) * u + f->c2_const;
		*derivative += (f->d2_quadratic * u + f->d2_linear) * u + f->c2_linear; //d2_const = c2_linear
	}

	if( u >= f->u[2] && u < f->u[3] )
	{
		*value += ((f->c3_cubic * u + f->c3_quadratic) * u + f->c3_linear) * u + f->c3_const;
		*derivative += (f->d3_quadratic * u + f->d3_linear) * u + f->c3_linear; //d3_const = c3_linear
	}
		
	if( u >= f->u[3] )
	{
		a = f->c4_shift - u;
		a2 = a * a;
		a3 = a2 * a;

		*value += a3 * f->c4_coeff;
		*derivative += a2 * f->d4_coeff;
	}
}

static void Spl_NormalizeKnots( float *knots, uint count )
{
	uint i;
	float sub = knots[0];
	float mul = 1.0F / (knots[count - 1] - sub);

	for( i = 1; i < count - 1; i++ )
		knots[i] = (knots[i] - sub) * mul;

	knots[0] = 0.0F;
	knots[count - 1] = 1.0F;
}

static void Spl_MakeDefaultKnots( float *knots, uint count, uint rank )
{
	uint i;
	uint d = min( rank + 1, count / 2 );
	float m;

	for( i = 0; i < d; i++ )
		knots[i] = 0;

	m = 0;
	for( i = d; i < count - d; i++ )
		knots[i] = (m += 1.0F);

	for( i = count - d; i < count; i++ )
		knots[i] = m;

	Spl_NormalizeKnots( knots, count );
}

#define AlignP( p, a ) (byte*)Align( (size_t)(p), a )

typedef struct curve3_t
{
	uint			numPts;
	float			*knots;
	blendFunc3_t	*blends;
	vec4_t			*controlPoints;
} curve3_t;

size_t Spl_CurveMemGetSize( uint numPoints )
{
	if( numPoints < 4 )
		return 0;

	return
		sizeof( curve3_t ) +
		sizeof( vec4_t ) * numPoints + 16 +			//give space to align
		sizeof( blendFunc3_t ) * numPoints +		//previous guarantees alignment here
		sizeof( float ) * (numPoints + 4);			//previous guarantees alignment here
}

curve3_t* Spl_CurveMemInit( void *mem, uint numPoints )
{
	curve3_t *ret = (curve3_t*)mem;

	byte *m = AlignP( (byte*)mem + sizeof( curve3_t ), 16 );

	ret->numPts = numPoints;
	
	ret->controlPoints = (vec4_t*)m;
	Com_Memset( ret->controlPoints, 0, sizeof( vec4_t ) * numPoints );
	m += sizeof( vec4_t ) * numPoints;

	ret->blends = (blendFunc3_t*)m;
	Com_Memset( ret->blends, 0, sizeof( blendFunc3_t ) * numPoints );
	m += sizeof( blendFunc3_t ) * numPoints;

	ret->knots = (float*)m;
	
	Spl_MakeDefaultKnots( ret->knots, ret->numPts + 4, 3 );
	Spl_CurveUpdateKnots( ret, ret->knots );

	return ret;
}

void Spl_CurveUpdateKnots( curve3_t *cv, const float *knots )
{
	uint i;

	if( cv->knots != knots )
		Com_Memcpy( cv->knots, knots, sizeof( float ) * (cv->numPts + 4) );

	for( i = 0; i < cv->numPts; i++ )
		Spl_BlendFunc3Init( cv->blends + i, cv->knots + i );
}

vec4_t* Spl_CurveGetPointsData( curve3_t *cv )
{
	return cv->controlPoints;
}

void Spl_CurveEval( /* out */ vec3_t position, /* out */ vec3_t tangent, const curve3_t *cv, float u )
{
	uint i, j;
	vec4_t du;
	vec4_t value;

	if( u < cv->knots[0] )
	{
		Vec4_Cpy( value, cv->controlPoints[0] );

		if( tangent )
			VectorClear( tangent );

		goto end;
	}

	if( u > cv->knots[cv->numPts + 4 - 1] )
	{
		Vec4_Cpy( value, cv->controlPoints[cv->numPts - 1] );

		if( tangent )
			VectorClear( tangent );

		goto end;
	}

	for( j = 0; j < 4; j++ )
		value[j] = 0;

	if( tangent )
		for( j = 0; j < 4; j++ )
			du[j] = 0;

	for( i = 0; i < cv->numPts; i++ )
	{
		float weight, deriv;
		Spl_BlendFunc3Eval( &weight, &deriv, cv->blends + i, u );

		for( j = 0; j < 4; j++ )
			value[j] += cv->controlPoints[i][j] * weight;

		if( tangent )
			for( j = 0; j < 4; j++ )
				du[j] += cv->controlPoints[i][j] * deriv;
	}

	if( tangent )
		for( j = 0; j < 3; j++ )
			tangent[j] = (du[j] * value[3] - value[j] * du[3]) * (value[3] * value[3]);

end:
	value[3] = 1.0F / value[3];
	for( j = 0; j < 3; j++ )
		position[j] = value[j] * value[3];
}