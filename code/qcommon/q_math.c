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
//
// q_math.c -- stateless support routines that are included in each code module

// Some of the vector functions are static inline in q_shared.h. q3asm
// doesn't understand static functions though, so we only want them in
// one file. That's what this is about.
#ifdef Q3_VM
#define __Q3_VM_MATH
#endif

#include "q_shared.h"

vec3_t	vec3_origin = {0,0,0};


vec4_t		colorBlack	= {0, 0, 0, 1};
vec4_t		colorRed	= {1, 0, 0, 1};
vec4_t		colorGreen	= {0, 1, 0, 1};
vec4_t		colorBlue	= {0, 0, 1, 1};
vec4_t		colorYellow	= {1, 1, 0, 1};
vec4_t		colorMagenta= {1, 0, 1, 1};
vec4_t		colorCyan	= {0, 1, 1, 1};
vec4_t		colorWhite	= {1, 1, 1, 1};
vec4_t		colorLtGrey	= {0.75, 0.75, 0.75, 1};
vec4_t		colorMdGrey	= {0.5, 0.5, 0.5, 1};
vec4_t		colorDkGrey	= {0.25, 0.25, 0.25, 1};

const vec4_t	g_color_table[9] =
{
	{0.0, 0.0, 0.0, 1.0},
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{0.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	{0.5, 0.5, 0.5, 1.0},
};


static const vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
{-0.525731f, 0.000000f, 0.850651f}, {-0.442863f, 0.238856f, 0.864188f}, 
{-0.295242f, 0.000000f, 0.955423f}, {-0.309017f, 0.500000f, 0.809017f}, 
{-0.162460f, 0.262866f, 0.951056f}, {0.000000f, 0.000000f, 1.000000f}, 
{0.000000f, 0.850651f, 0.525731f}, {-0.147621f, 0.716567f, 0.681718f}, 
{0.147621f, 0.716567f, 0.681718f}, {0.000000f, 0.525731f, 0.850651f}, 
{0.309017f, 0.500000f, 0.809017f}, {0.525731f, 0.000000f, 0.850651f}, 
{0.295242f, 0.000000f, 0.955423f}, {0.442863f, 0.238856f, 0.864188f}, 
{0.162460f, 0.262866f, 0.951056f}, {-0.681718f, 0.147621f, 0.716567f}, 
{-0.809017f, 0.309017f, 0.500000f},{-0.587785f, 0.425325f, 0.688191f}, 
{-0.850651f, 0.525731f, 0.000000f},{-0.864188f, 0.442863f, 0.238856f}, 
{-0.716567f, 0.681718f, 0.147621f},{-0.688191f, 0.587785f, 0.425325f}, 
{-0.500000f, 0.809017f, 0.309017f}, {-0.238856f, 0.864188f, 0.442863f}, 
{-0.425325f, 0.688191f, 0.587785f}, {-0.716567f, 0.681718f, -0.147621f}, 
{-0.500000f, 0.809017f, -0.309017f}, {-0.525731f, 0.850651f, 0.000000f}, 
{0.000000f, 0.850651f, -0.525731f}, {-0.238856f, 0.864188f, -0.442863f}, 
{0.000000f, 0.955423f, -0.295242f}, {-0.262866f, 0.951056f, -0.162460f}, 
{0.000000f, 1.000000f, 0.000000f}, {0.000000f, 0.955423f, 0.295242f}, 
{-0.262866f, 0.951056f, 0.162460f}, {0.238856f, 0.864188f, 0.442863f}, 
{0.262866f, 0.951056f, 0.162460f}, {0.500000f, 0.809017f, 0.309017f}, 
{0.238856f, 0.864188f, -0.442863f},{0.262866f, 0.951056f, -0.162460f}, 
{0.500000f, 0.809017f, -0.309017f},{0.850651f, 0.525731f, 0.000000f}, 
{0.716567f, 0.681718f, 0.147621f}, {0.716567f, 0.681718f, -0.147621f}, 
{0.525731f, 0.850651f, 0.000000f}, {0.425325f, 0.688191f, 0.587785f}, 
{0.864188f, 0.442863f, 0.238856f}, {0.688191f, 0.587785f, 0.425325f}, 
{0.809017f, 0.309017f, 0.500000f}, {0.681718f, 0.147621f, 0.716567f}, 
{0.587785f, 0.425325f, 0.688191f}, {0.955423f, 0.295242f, 0.000000f}, 
{1.000000f, 0.000000f, 0.000000f}, {0.951056f, 0.162460f, 0.262866f}, 
{0.850651f, -0.525731f, 0.000000f},{0.955423f, -0.295242f, 0.000000f}, 
{0.864188f, -0.442863f, 0.238856f}, {0.951056f, -0.162460f, 0.262866f}, 
{0.809017f, -0.309017f, 0.500000f}, {0.681718f, -0.147621f, 0.716567f}, 
{0.850651f, 0.000000f, 0.525731f}, {0.864188f, 0.442863f, -0.238856f}, 
{0.809017f, 0.309017f, -0.500000f}, {0.951056f, 0.162460f, -0.262866f}, 
{0.525731f, 0.000000f, -0.850651f}, {0.681718f, 0.147621f, -0.716567f}, 
{0.681718f, -0.147621f, -0.716567f},{0.850651f, 0.000000f, -0.525731f}, 
{0.809017f, -0.309017f, -0.500000f}, {0.864188f, -0.442863f, -0.238856f}, 
{0.951056f, -0.162460f, -0.262866f}, {0.147621f, 0.716567f, -0.681718f}, 
{0.309017f, 0.500000f, -0.809017f}, {0.425325f, 0.688191f, -0.587785f}, 
{0.442863f, 0.238856f, -0.864188f}, {0.587785f, 0.425325f, -0.688191f}, 
{0.688191f, 0.587785f, -0.425325f}, {-0.147621f, 0.716567f, -0.681718f}, 
{-0.309017f, 0.500000f, -0.809017f}, {0.000000f, 0.525731f, -0.850651f}, 
{-0.525731f, 0.000000f, -0.850651f}, {-0.442863f, 0.238856f, -0.864188f}, 
{-0.295242f, 0.000000f, -0.955423f}, {-0.162460f, 0.262866f, -0.951056f}, 
{0.000000f, 0.000000f, -1.000000f}, {0.295242f, 0.000000f, -0.955423f}, 
{0.162460f, 0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f}, 
{-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f}, 
{0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f}, 
{0.147621f, -0.716567f, -0.681718f}, {0.000000f, -0.525731f, -0.850651f}, 
{0.309017f, -0.500000f, -0.809017f}, {0.442863f, -0.238856f, -0.864188f}, 
{0.162460f, -0.262866f, -0.951056f}, {0.238856f, -0.864188f, -0.442863f}, 
{0.500000f, -0.809017f, -0.309017f}, {0.425325f, -0.688191f, -0.587785f}, 
{0.716567f, -0.681718f, -0.147621f}, {0.688191f, -0.587785f, -0.425325f}, 
{0.587785f, -0.425325f, -0.688191f}, {0.000000f, -0.955423f, -0.295242f}, 
{0.000000f, -1.000000f, 0.000000f}, {0.262866f, -0.951056f, -0.162460f}, 
{0.000000f, -0.850651f, 0.525731f}, {0.000000f, -0.955423f, 0.295242f}, 
{0.238856f, -0.864188f, 0.442863f}, {0.262866f, -0.951056f, 0.162460f}, 
{0.500000f, -0.809017f, 0.309017f}, {0.716567f, -0.681718f, 0.147621f}, 
{0.525731f, -0.850651f, 0.000000f}, {-0.238856f, -0.864188f, -0.442863f}, 
{-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f}, 
{-0.850651f, -0.525731f, 0.000000f}, {-0.716567f, -0.681718f, -0.147621f}, 
{-0.716567f, -0.681718f, 0.147621f}, {-0.525731f, -0.850651f, 0.000000f}, 
{-0.500000f, -0.809017f, 0.309017f}, {-0.238856f, -0.864188f, 0.442863f}, 
{-0.262866f, -0.951056f, 0.162460f}, {-0.864188f, -0.442863f, 0.238856f}, 
{-0.809017f, -0.309017f, 0.500000f}, {-0.688191f, -0.587785f, 0.425325f}, 
{-0.681718f, -0.147621f, 0.716567f}, {-0.442863f, -0.238856f, 0.864188f}, 
{-0.587785f, -0.425325f, 0.688191f}, {-0.309017f, -0.500000f, 0.809017f}, 
{-0.147621f, -0.716567f, 0.681718f}, {-0.425325f, -0.688191f, 0.587785f}, 
{-0.162460f, -0.262866f, 0.951056f}, {0.442863f, -0.238856f, 0.864188f}, 
{0.162460f, -0.262866f, 0.951056f}, {0.309017f, -0.500000f, 0.809017f}, 
{0.147621f, -0.716567f, 0.681718f}, {0.000000f, -0.525731f, 0.850651f}, 
{0.425325f, -0.688191f, 0.587785f}, {0.587785f, -0.425325f, 0.688191f}, 
{0.688191f, -0.587785f, 0.425325f}, {-0.955423f, 0.295242f, 0.000000f}, 
{-0.951056f, 0.162460f, 0.262866f}, {-1.000000f, 0.000000f, 0.000000f}, 
{-0.850651f, 0.000000f, 0.525731f}, {-0.955423f, -0.295242f, 0.000000f}, 
{-0.951056f, -0.162460f, 0.262866f}, {-0.864188f, 0.442863f, -0.238856f}, 
{-0.951056f, 0.162460f, -0.262866f}, {-0.809017f, 0.309017f, -0.500000f}, 
{-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f}, 
{-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f}, 
{-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f}, 
{-0.688191f, 0.587785f, -0.425325f}, {-0.587785f, 0.425325f, -0.688191f}, 
{-0.425325f, 0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f}, 
{-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};

//==============================================================

#if 0 //__LCC__

int VectorCompare( const vec3_t v1, const vec3_t v2 ) {
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2]) {
		return 0;
	}			
	return 1;
}

vec_t VectorLength( const vec3_t v ) {
	return (vec_t)sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

vec_t VectorLengthSquared( const vec3_t v ) {
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

vec_t Distance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return VectorLength( v );
}

vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
void VectorNormalizeFast( vec3_t v )
{
	float ilength;

	ilength = Q_rsqrt( DotProduct( v, v ) );

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

void VectorInverse( vec3_t v ){
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross ) {
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}
#endif

//=======================================================

signed char ClampChar( int i )
{
	if( i < -128 )
		return -128;
	
	if( i > 127 )
		return 127;

	return (signed char)i;
}

signed short ClampShort( int i )
{
	if( i < -32768 )
		return -32768;

	if( i > 0x7fff )
		return 0x7fff;
	
	return (signed short)i;
}


// this isn't a real cheap function to call!
int DirToByte( vec3_t dir ) {
	int		i, best;
	float	d, bestd;

	if ( !dir ) {
		return 0;
	}

	bestd = 0;
	best = 0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vec3_t dir ) {
	if ( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopy( vec3_origin, dir );
		return;
	}
	VectorCopy (bytedirs[b], dir);
}


unsigned ColorBytes3( float r, float g, float b )
{
	unsigned i;

	((byte*)&i)[0] = (byte)(r * 0xFF);
	((byte*)&i)[1] = (byte)(g * 0xFF);
	((byte*)&i)[2] = (byte)(b * 0xFF);

	return i;
}

unsigned ColorBytes4( float r, float g, float b, float a )
{
	unsigned i;

	((byte*)&i)[0] = (byte)(r * 0xFF);
	((byte*)&i)[1] = (byte)(g * 0xFF);
	((byte*)&i)[2] = (byte)(b * 0xFF);
	((byte*)&i)[3] = (byte)(a * 0xFF);

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out )
{
	float	max;
	
	max = in[0];
	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}
	return max;
}


/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c ) {
	vec3_t	d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );
	if ( VectorNormalize( plane ) == 0 ) {
		return qfalse;
	}

	plane[3] = DotProduct( a, plane );
	return qtrue;
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,
							 float degrees ) {
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	vec3_t vr, vup, vf;
	float	rad;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad = DEG2RAD( degrees );
	zrot[0][0] = cosf( rad );
	zrot[0][1] = sinf( rad );
	zrot[1][0] = -sinf( rad );
	zrot[1][1] = cosf( rad );

	MatrixMultiply( m, zrot, tmpmat );
	MatrixMultiply( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ ) {
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[3], float yaw ) {

	// create an arbitrary axis[1] 
	PerpendicularVector( axis[1], axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw ) {
		vec3_t	temp;

		VectorCopy( axis[1], temp );
		RotatePointAroundVector( axis[1], axis[0], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[0], axis[1], axis[2] );
}



void vectoangles( const vec3_t value1, vec3_t angles ) {
	float	forward;
	float	yaw, pitch;
	
	if ( value1[1] == 0 && value1[0] == 0 ) {
		yaw = 0;
		if ( value1[2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if ( value1[0] ) {
			yaw = ( atan2f ( value1[1], value1[0] ) * 180 / M_PI );
		}
		else if ( value1[1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrtf ( value1[0]*value1[0] + value1[1]*value1[1] );
		pitch = ( atan2f(value1[2], forward) * 180.0f / M_PI );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}


/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] ) {
	vec3_t	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}

void AxisClear( vec3_t axis[3] ) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] ) {
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom =  DotProduct( normal, normal );
#ifndef Q3_VM
	assert( Q_fabs(inv_denom) != 0.0f ); // bk010122 - zero vectors get here
#endif
	inv_denom = 1.0f / inv_denom;

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up) {
	float		d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}


void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out )
{
	out[0] = DotProduct( in, matrix[0] );
	out[1] = DotProduct( in, matrix[1] );
	out[2] = DotProduct( in, matrix[2] );
}

//============================================================================

#if !idppc
/*
** float q_rsqrt( float number )
*/
float Q_rsqrt( float number )
{
	fi_t t;	
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	t.f  = number;
	t.i  = 0x5f3759dF - (t.i >> 1);
	y  = t.f;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	//assert( !isnan(y) ); // bk010122 - FPE?
	return y;
}

float Q_fabs( float f ) {
	fi_t t;
	t.f = f;
	t.i &= 0x7FFFFFFF;
	return t.f;
}
#endif

/*
=====================
Q_acos

the msvc acos doesn't always return a value between -PI and PI:

int i;
i = 1065353246;
acos(*(float*) &i) == -1.#IND0
=====================
*/
float Q_acos(float c) {
	float angle;

	angle = acos(c);

	if (angle > M_PI) {
		return (float)M_PI;
	}
	if (angle < -M_PI) {
		return (float)M_PI;
	}
	return angle;
}


//============================================================

/*
===============
LerpAngle

===============
*/
float LerpAngle (float from, float to, float frac) {
	float	a;

	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}
	a = from + frac * (to - from);

	return a;
}


/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float	AngleSubtract( float a1, float a2 ) {
	float	a;

	a = a1 - a2;
	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}


void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 ) {
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}


float	AngleMod(float a) {
	a = (360.0f/65536.0f) * ((int)(a*(65536.0f/360.0f)) & 65535);
	return a;
}


/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360 ( float angle ) {
	return (360.0f / 65536) * ((int)(angle * (65536 / 360.0f)) & 65535);
}


/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180 ( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0 ) {
		angle -= 360.0;
	}
	return angle;
}


/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta ( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}


//============================================================


/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits( cplane_t *out )
{
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for( j = 0; j < 3; j++ )
		if( out->normal[j] < 0 )
			bits |= 1 << j;

	out->signbits = (byte)bits;
}


/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2

// this is the slow, general version
int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	vec3_t	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct (p->normal, corners[0]) - p->dist;
	dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}

==================
*/

#if !id386

int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist1, dist2;
	int		sides;

// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}
#elif __GNUC__
#ifdef MACOS_X
#ifndef id386
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist1, dist2;
	int		sides;
	
	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}
	
	// general case
	switch (p->signbits)
	{
		case 0:
			dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
			dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
			break;
		case 1:
			dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
			dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
			break;
		case 2:
			dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
			dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
			break;
		case 3:
			dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
			dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
			break;
		case 4:
			dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
			dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
			break;
		case 5:
			dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
			dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
			break;
		case 6:
			dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
			dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
			break;
		case 7:
			dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
			dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
			break;
		default:
			dist1 = dist2 = 0;		// shut up compiler
			break;
	}
	
	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;
	
	return sides;
}
#endif
#endif
// use matha.s
#else

__declspec( naked ) int QDECL BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, struct cplane_s * p )
{
	//shut up unreferenced param warnings as these are accessed by asm names
	static int bops_initialized;
	static int Ljmptab[8];

	emins; emaxs; p;

	__asm
	{
		push ebx
			
		cmp bops_initialized, 1
		je  initialized
		mov bops_initialized, 1
		
		mov Ljmptab[0*4], offset Lcase0
		mov Ljmptab[1*4], offset Lcase1
		mov Ljmptab[2*4], offset Lcase2
		mov Ljmptab[3*4], offset Lcase3
		mov Ljmptab[4*4], offset Lcase4
		mov Ljmptab[5*4], offset Lcase5
		mov Ljmptab[6*4], offset Lcase6
		mov Ljmptab[7*4], offset Lcase7
			
	initialized:

		mov edx,dword ptr[4+12+esp]
		mov ecx,dword ptr[4+4+esp]
		xor eax,eax
		mov ebx,dword ptr[4+8+esp]
		mov al,byte ptr[17+edx]
		cmp al,8
		jge Lerror
		fld dword ptr[0+edx]
		fld st(0)
		jmp dword ptr[Ljmptab+eax*4]
	Lcase0:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
	Lcase1:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
	Lcase2:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
	Lcase3:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
	Lcase4:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
	Lcase5:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
	Lcase6:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
	Lcase7:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
	LSetSides:
		faddp st(2),st(0)
		fcomp dword ptr[12+edx]
		xor ecx,ecx
		fnstsw ax
		fcomp dword ptr[12+edx]
		and ah,1
		xor ah,1
		add cl,ah
		fnstsw ax
		and ah,1
		add ah,ah
		add cl,ah
		pop ebx
		mov eax,ecx
		ret
	Lerror:
		int 3
	}
}

#endif

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs ) {
	int		i;
	vec3_t	corner;
	float	a, b;

	for (i=0 ; i<3 ; i++) {
		a = fabsf( mins[i] );
		b = fabsf( maxs[i] );
		corner[i] = a > b ? a : b;
	}

	return VectorLength (corner);
}


void ClearBounds( vec3_t mins, vec3_t maxs ) {
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs ) {
	if ( v[0] < mins[0] ) {
		mins[0] = v[0];
	}
	if ( v[0] > maxs[0]) {
		maxs[0] = v[0];
	}

	if ( v[1] < mins[1] ) {
		mins[1] = v[1];
	}
	if ( v[1] > maxs[1]) {
		maxs[1] = v[1];
	}

	if ( v[2] < mins[2] ) {
		mins[2] = v[2];
	}
	if ( v[2] > maxs[2]) {
		maxs[2] = v[2];
	}
}


vec_t VectorNormalize( vec3_t v ) {
	// NOTE: TTimo - Apple G4 altivec source uses double?
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrtf (length);

	if ( length ) {
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;
}

vec_t VectorNormalize2( const vec3_t v, vec3_t out) {
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrtf (length);

	if (length)
	{
#ifndef Q3_VM // bk0101022 - FPE related
//	  assert( ((Q_fabs(v[0])!=0.0f) || (Q_fabs(v[1])!=0.0f) || (Q_fabs(v[2])!=0.0f)) );
#endif
		ilength = 1/length;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	} else {
#ifndef Q3_VM // bk0101022 - FPE related
//	  assert( ((Q_fabs(v[0])==0.0f) && (Q_fabs(v[1])==0.0f) && (Q_fabs(v[2])==0.0f)) );
#endif
		VectorClear( out );
	}
		
	return length;

}

void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc) {
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct( const vec3_t v1, const vec3_t v2 ) {
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void _VectorScale( const vec3_t in, vec_t scale, vec3_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
	out[3] = in[3]*scale;
}


int Q_log2( int val ) {
	int answer;

	answer = 0;
	while ( ( val>>=1 ) != 0 ) {
		answer++;
	}
	return answer;
}



/*
=================
PlaneTypeForNormal
=================
*/
/*
int	PlaneTypeForNormal (vec3_t normal) {
	if ( normal[0] == 1.0 )
		return PLANE_X;
	if ( normal[1] == 1.0 )
		return PLANE_Y;
	if ( normal[2] == 1.0 )
		return PLANE_Z;
	
	return PLANE_NON_AXIAL;
}
*/


/*
================
MatrixMultiply
================
*/
void MatrixMultiply( float in1[3][3], float in2[3][3], float out[3][3] )
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}


void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sinf(angle);
	cy = cosf(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sinf(angle);
	cp = cosf(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sinf(angle);
	cr = cosf(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-sr*sp*cy+cr*sy);
		right[1] = (-sr*sp*sy-cr*cy);
		right[2] = -sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+sr*sy);
		up[1] = (cr*sp*sy-sr*cy);
		up[2] = cr*cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabsf( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

/*
================
Q_isnan

Don't pass doubles to this
================
*/
int Q_isnan( float x )
{
	union
	{
		float f;
		unsigned int i;
	} t;

	t.f = x;
	t.i &= 0x7FFFFFFF;
	t.i = 0x7F800000 - t.i;

	return (int)( (unsigned int)t.i >> 31 );
}

/*
 *	Some matrix math stuff.
 *
 *	Matrices are taken in column-major format.
 *
 *	Note: if it says "affine" in its name it *assumes* that the input is affine.
 *		If the input isn't affine, then there's no telling what you'll get out.
 *
 */

void Normalize3x3( float m[3][3] )
{
	int i;
	for( i = 0; i < 3; i++ )
	{
		float l = 1.0F / DotProduct( m[i], m[i] );
		VectorScale( m[i], l, m[i] );
	}
}

NOGLOBALALIAS void Affine_Inv( affine_t * RESTRICT o, const affine_t *or )
{
	int i, j;

	for( i = 0; i < 3; i++ )
		for( j = 0; j < 3; j++ )
			o->axis[i][j] = or->axis[j][i];

	for( i = 0; i < 3; i++ )
		o->origin[i] = -DotProduct( or->origin, or->axis[i] );
}

NOGLOBALALIAS void Affine_Mul( affine_t * RESTRICT o, const affine_t *a, const affine_t *b )
{
	int c, r;
	for( c = 0; c < 3; c++ )
		for( r = 0; r < 3; r++ )
			o->axis[c][r] =
				a->axis[0][r] * b->axis[c][0] +
				a->axis[1][r] * b->axis[c][1] +
				a->axis[2][r] * b->axis[c][2];

	for( r = 0; r < 3; r++ )
		o->origin[r] =
			a->axis[0][r] * b->origin[0] +
			a->axis[1][r] * b->origin[1] +
			a->axis[2][r] * b->origin[2] +
			a->origin[r];
}

NOGLOBALALIAS void Affine_MulVec( vec3_t o, const affine_t *m, const vec3_t v )
{
	int i;
	vec3_t t;
	
	if ( v == o ) {
		t[ 0 ] = v[ 0 ];
		t[ 1 ] = v[ 1 ];
		t[ 2 ] = v[ 2 ];
		v = t;
	}

	for( i = 0; i < 3; i++ )
		o[i] =
			m->axis[0][i] * v[0] +
			m->axis[1][i] * v[1] +
			m->axis[2][i] * v[2];
}

NOGLOBALALIAS void Affine_MulPos( vec3_t o, const affine_t *m, const vec3_t p )
{
	int i;

	Affine_MulVec( o, m, p );

	for( i = 0; i < 3; i++ )
		o[i] += m->origin[i];
}

NOGLOBALALIAS void Affine_SetFromMatrix( affine_t * RESTRICT o, const float *m )
{
	if( m )
	{
		memcpy( o->axis + 0, m + 0, sizeof( float ) * 3 );
		memcpy( o->axis + 1, m + 4, sizeof( float ) * 3 );
		memcpy( o->axis + 2, m + 8, sizeof( float ) * 3 );

		memcpy( o->origin, m + 12, sizeof( float ) * 3 );
	}
	else
	{
		int i;

		memset( o, 0, sizeof( affine_t ) );
		for( i = 0; i < 3; i++ )
			o->axis[i][i] = 1;
	}
}

NOGLOBALALIAS void Affine_Set( affine_t * RESTRICT o, const vec3_t axis[3], const vec3_t origin )
{
	int i;

	if( axis )
	{
		for( i = 0; i < 3; i++ )
			VectorCopy( axis[i], o->axis[i] );
	}
	else
	{
		memset( o->axis, 0, sizeof( o->axis ) );
		for( i = 0; i < 3; i++ )
			o->axis[i][i] = 1;
	}

	if( origin )
		VectorCopy( origin, o->origin );
	else
		memset( o->origin, 0, sizeof( o->origin ) );
}

NOGLOBALALIAS void Affine_Cpy( affine_t * RESTRICT d, const affine_t *s )
{
	memcpy( d, s, sizeof( affine_t ) );
}

NOGLOBALALIAS void Affine_Scale( affine_t * RESTRICT o, const affine_t *m, float s )
{
	int i, j;

	for( i = 0; i < 3; i++ )
		for( j = 0; j < 3; j++ )
			o->axis[i][j] = m->axis[i][j] * s;

	for( i = 0; i < 3; i++ )
		o->origin[i] = m->origin[i] * s;
}

NOGLOBALALIAS void Affine_Mad( affine_t * RESTRICT o, const affine_t *m, float s, const affine_t *a )
{
	int i, j;

	for( i = 0; i < 3; i++ )
		for( j = 0; j < 3; j++ )
			o->axis[i][j] = m->axis[i][j] * s + a->axis[i][j];

	for( i = 0; i < 3; i++ )
		o->origin[i] = m->origin[i] * s + a->origin[i];
}

NOGLOBALALIAS void Matrix_Trp( float o[16], float m[16] )
{
	o[0] = m[0]; o[1] = m[4]; o[2] = m[8]; o[3] = m[12];
	o[4] = m[1]; o[5] = m[5]; o[6] = m[9]; o[7] = m[13];
	o[8] = m[2]; o[9] = m[6]; o[10] = m[10]; o[11] = m[14];
	o[12] = m[3]; o[13] = m[7]; o[14] = m[11]; o[15] = m[15];
}

NOGLOBALALIAS void Matrix_SetFromAffine( float * RESTRICT o, const affine_t *m )
{
	o[0] = m->axis[0][0];
	o[1] = m->axis[0][1];
	o[2] = m->axis[0][2];
	o[3] = 0;
	
	o[4] = m->axis[1][0];
	o[5] = m->axis[1][1];
	o[6] = m->axis[1][2];
	o[7] = 0;
	
	o[8] = m->axis[2][0];
	o[9] = m->axis[2][1];
	o[10] = m->axis[2][2];
	o[11] = 0;
	
	o[12] = m->origin[0];
	o[13] = m->origin[1];
	o[14] = m->origin[2];
	o[15] = 1;
}

NOGLOBALALIAS void Quat_Cpy( quat_t dest, const quat_t src )
{
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];
	dest[3] = src[3];
}

NOGLOBALALIAS static ID_INLINE float rsqrt( float x )
{
	long i;
	float y, r;

	y = x * 0.5F;

	i = *(long*)(&x);
	i = 0x5f3759dF - (i >> 1);

	r = *(float*)(&i);
	
	r = r * (1.5F - r * r * y);
	r = r * (1.5F - r * r * y);
	
	return r;
}

NOGLOBALALIAS static ID_INLINE float sin_0_HalfPi( float a )
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

NOGLOBALALIAS static ID_INLINE float atan2_pos( float y, float x )
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

NOGLOBALALIAS void Quat_Slerp( quat_t o, const quat_t a, const quat_t b, float t )
{
	float cosTheta;
	float ta, tb;
	qboolean neg;

	cosTheta = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];

	if( cosTheta < 0.0F )
	{
		cosTheta = -cosTheta;
		neg = qtrue;
	}
	else
		neg = qfalse;

	if( cosTheta < 1.0F - 1e-5F )
	{
		float sinThetaSqr = 1.0F - (cosTheta * cosTheta);
		float sinThetaInv = rsqrt( sinThetaSqr );
		float theta = atan2_pos( sinThetaSqr * sinThetaInv, cosTheta );

		//we're good to SLERP
		ta = sin_0_HalfPi( (1.0F - t) * theta ) * sinThetaInv;
		tb = sin_0_HalfPi( t * theta ) * sinThetaInv;
	}
	else
	{
		//quats are very close, just lerp to avoid the division by zero
		ta = 1.0F - t;
		tb = t;
	}

	if( neg )
		tb = -tb;

	o[0] = a[0] * ta + b[0] * tb;
	o[1] = a[1] * ta + b[1] * tb;
	o[2] = a[2] * ta + b[2] * tb;
	o[3] = a[3] * ta + b[3] * tb;
}

NOGLOBALALIAS void Quat_SleprEx( quat_t o, const quat_t a, const quat_t b, float t, int direction )
{
	float cosTheta;
	float ta, tb;
	qboolean neg;

	cosTheta = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];

	if( direction )
	{
		if( direction > 0 )
		{
			if( cosTheta < 0.0F )
			{
				cosTheta = -cosTheta;
				neg = qtrue;
			}
			else
				neg = qfalse;
		}
		else
		{
			if( cosTheta > 0.0F )
			{
				cosTheta = -cosTheta;
				neg = qtrue;
			}
			else
				neg = qfalse;
		}
	}
	else
		neg = qfalse;

	if( cosTheta < 1.0F - 1e-5F )
	{
		float theta = acosf( cosTheta );
		float sinTheta = sinf( theta );
		float sinThetaInv = 1.0F / sinTheta;

		//we're good to SLERP
		ta = sinf( (1.0F - t) * theta ) * sinThetaInv;
		tb = sinf( t * theta ) * sinThetaInv;
	}
	else
	{
		//quats are very close, just lerp to avoid the division by zero
		ta = 1.0F - t;
		tb = t;
	}

	if( neg )
		tb = -tb;

	o[0] = a[0] * ta + b[0] * tb;
	o[1] = a[1] * ta + b[1] * tb;
	o[2] = a[2] * ta + b[2] * tb;
	o[3] = a[3] * ta + b[3] * tb;
}

NOGLOBALALIAS void Quat_AxisAngle( quat_t o, const vec3_t ax, float an )
{
	float sa;

	an *= 0.5F;
	sa = sinf( an );

	o[0] = sa * ax[0];
	o[1] = sa * ax[1];
	o[2] = sa * ax[2];
	o[3] = cosf( an );
}

static vec3_t vec_i = { 1, 0, 0 };
static vec3_t vec_j = { 0, 1, 0 };
static vec3_t vec_k = { 0, 0, 1 };

NOGLOBALALIAS void Quat_FromEulerAngles( quat_t o, const vec3_t a )
{
	quat_t y, p, r, tmp;

	Quat_AxisAngle( y, vec_j, a[YAW] * (M_PI*2 / 360) );
	Quat_AxisAngle( p, vec_i, a[PITCH] * (M_PI*2 / 360) );
	Quat_AxisAngle( r, vec_k, a[ROLL] * (M_PI*2 / 360) );

	Quat_Mul( tmp, y, p );
	Quat_Mul( o, tmp, r );
}

NOGLOBALALIAS void Quat_Mul( quat_t o, const quat_t a, const quat_t b )
{
	/*
		From pretty much any math text you'll find that given:

			p = a + bi + cj + dk
			q = w + xi + yj + zk

		You get the product:
		
			pq =	(aw - bx - cy - dz) +
					(bw + ax + cz - dy)i +
					(cw + ay + dx - bz)j +
					(dw + az + by - cx)k
			
		So, keeping in mind that p = a and q = b, and that we store
		elements in (xi, yj, zk, w) order we have:

			ab =	(a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]) +
					(a[0] * b[3] + a[3] * b[0] + a[1] * b[2] - a[2] * b[1])i +
					(a[1] * b[3] + a[3] * b[1] + a[2] * b[0] - a[0] * b[2])j +
					(a[2] * b[3] + a[3] * b[2] + a[0] * b[1] - a[1] * b[0])k
	*/

	o[0] = a[0] * b[3] + a[3] * b[0] + a[1] * b[2] - a[2] * b[1];
	o[1] = a[1] * b[3] + a[3] * b[1] + a[2] * b[0] - a[0] * b[2];
	o[2] = a[2] * b[3] + a[3] * b[2] + a[0] * b[1] - a[1] * b[0];
	o[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
}

NOGLOBALALIAS void QuatToAffine( affine_t *o, const quat_t q )
{
	float xx = q[0] * q[0];
	float yy = q[1] * q[1];
	float zz = q[2] * q[2];
	
	float xy = q[0] * q[1];
	float xz = q[0] * q[2];
	float xw = q[0] * q[3];
	float yz = q[1] * q[2];
	float yw = q[1] * q[3];
	float zw = q[2] * q[3];

	o->axis[0][0] = 1.0F - 2.0F * (yy + zz);	o->axis[1][0] = 2.0F * (xy - zw);			o->axis[2][0] = 2.0F * (xz + yw);
	o->axis[0][1] = 2.0F * (xy + zw);			o->axis[1][1] = 1.0F - 2.0F * (xx + zz);	o->axis[2][1] = 2.0F * (yz - xw);
	o->axis[0][2] = 2.0F * (xz - yw);			o->axis[1][2] = 2.0F * (yz + xw);			o->axis[2][2] = 1.0F - 2.0F * (xx + yy);

	o->origin[0] = 0.0F;
	o->origin[1] = 0.0F;
	o->origin[2] = 0.0F;
}

NOGLOBALALIAS void QuatToAffine3x3( float o[3][3], const quat_t q )
{
	float xx = q[0] * q[0];
	float yy = q[1] * q[1];
	float zz = q[2] * q[2];
	
	float xy = q[0] * q[1];
	float xz = q[0] * q[2];
	float xw = q[0] * q[3];
	float yz = q[1] * q[2];
	float yw = q[1] * q[3];
	float zw = q[2] * q[3];

	o[0][0] = 1.0F - 2.0F * (yy + zz);	o[1][0] = 2.0F * (xy - zw);			o[2][0] = 2.0F * (xz + yw);
	o[0][1] = 2.0F * (xy + zw);			o[1][1] = 1.0F - 2.0F * (xx + zz);	o[2][1] = 2.0F * (yz - xw);
	o[0][2] = 2.0F * (xz - yw);			o[1][2] = 2.0F * (yz + xw);			o[2][2] = 1.0F - 2.0F * (xx + yy);
}

NOGLOBALALIAS void Quat_Ident( quat_t o ) { o[0] = 0; o[1] = 0; o[2] = 0; o[3] = 1; }

NOGLOBALALIAS void Vec2_Lrp( vec2_t o, const vec2_t a, const vec2_t b, float t )
{
	int i;
	float it = 1 - t;

	for( i = 0; i < 2; i++ )
		o[i] = a[i] * it + b[i] * t;
}

NOGLOBALALIAS void Vec3_Lrp( vec3_t o, const vec3_t a, const vec3_t b, float t )
{
	int i;
	float it = 1 - t;

	for( i = 0; i < 3; i++ )
		o[i] = a[i] * it + b[i] * t;
}

NOGLOBALALIAS void Vec4_Lrp( vec4_t o, const vec4_t a, const vec4_t b, float t )
{
	int i;
	float it = 1 - t;

	for( i = 0; i < 4; i++ )
		o[i] = a[i] * it + b[i] * t;
}

NOGLOBALALIAS vec_t Vec3_Dist( const vec3_t a, const vec3_t b )
{
	vec3_t c;
	VectorSubtract( a, b, c );

	return sqrtf( c[0] * c[0] + c[1] * c[1] + c[2] * c[2] );
}

NOGLOBALALIAS void Vec3_Min( vec3_t o, const vec3_t a, const vec3_t b )
{
	o[0] = (a[0] < b[0]) ? a[0] : b[0];
	o[1] = (a[1] < b[1]) ? a[1] : b[1];
	o[2] = (a[2] < b[2]) ? a[2] : b[2];
}

NOGLOBALALIAS void Vec3_Max( vec3_t o, const vec3_t a, const vec3_t b )
{
	o[0] = (a[0] > b[0]) ? a[0] : b[0];
	o[1] = (a[1] > b[1]) ? a[1] : b[1];
	o[2] = (a[2] > b[2]) ? a[2] : b[2];
}

NOGLOBALALIAS void Vec3_Cross( vec3_t o, const vec3_t a, const vec3_t b )
{
	o[0] = a[1] * b[2] - a[2] * b[1];
	o[1] = a[2] * b[0] - a[0] * b[2];
	o[2] = a[0] * b[1] - a[1] * b[0];
}

NOGLOBALALIAS void Vec4_Cpy( vec4_t o, const vec4_t i ) { o[0] = i[0]; o[1] = i[1]; o[2] = i[2]; o[3] = i[3]; }
