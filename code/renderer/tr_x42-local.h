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

#ifndef TR_X42_LOCAL_H
#define TR_X42_LOCAL_H

typedef struct x42KsGroupFrames_t
{
	ushort				minFrame;
	ushort				maxFrame;
	const x42keyStreamEntry_t	*ksPos;
} x42KsGroupFrames_t;

typedef struct x42KsBoneEntry_t
{
	ushort				frame;
	ushort				value;
} x42KsBoneEntry_t, x42KsBoneEntries_t[3][2]; //type, lower/upper

typedef struct x42KsCache_t
{
	int					lastUsedFrame;
	struct x42KsCache_t	*next;

	x42KsGroupFrames_t	*groupFrames;
	x42KsBoneEntries_t	*kpairs;
} x42KsCache_t;

typedef struct x42KsStart_t
{
	int					lastusedFrame;
	x42KsGroupFrames_t	frame;
	x42KsBoneEntries_t	*kpairs;
} x42KsStart_t;

void R_x42PoseInitKsStarts( x42Model_t *mod );

/*
	Our models don't have a lot of scale in them, so we can skip the inverse-transpose
	step of the normal transformation to get a fairly significant speed improvement.

	If the artists ever go nuts with non-uniform scaling values then either make them
	stop and fix it, or comment out the following line.
*/
#define X42_SKIP_INVERSE_TRANSPOSE

extern affine_t x42_m2q3;

#define AlignP( p, a ) (byte*)Align( (size_t)(p), a )

void R_x42PoseBuildPose( x42Pose_t * out, const x42Model_t *mod, int currFrame,
	const animGroupFrame_t * groupFrames0, uint numGroupFrames0, const boneOffset_t * boneOffsets0, uint numBoneOffsets0,
	const animGroupFrame_t * groupFrames1, uint numGroupFrames1, const boneOffset_t * boneOffsets1, uint numBoneOffsets1,
	const animGroupTransition_t *frameLerps, uint numFrameLerps );

bool R_AnimateX42Group( const x42Model_t *mod, const x42group_t *g, const x42Pose_t *pose,
	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride,
	vec4_t * RESTRICT oTan, size_t oTanStride, vec4_t * RESTRICT oBin, size_t oBinStride );

NOGLOBALALIAS void quat_interp( float * RESTRICT o, const float * RESTRICT a, const float * RESTRICT b, float t );

#ifndef	TR_COMPILE_NO_SSE
bool R_AnimSupportsSSE( void );

void R_x42SSEBegin( void );
void R_x42SSEEnd( void );

NOGLOBALALIAS void quat_interp_SSE( float * RESTRICT o,
	const float * RESTRICT a, const float * RESTRICT b, float t );

void R_AnimateX42VertsTangentBasis_SSE( const affine_t *iinfs, uint numInfs, uint maxInfsPerVert,
	const x42vertAnim_t * RESTRICT modVertPos, const x42vertTangent_t * RESTRICT modVertTanBasis, uint numVerts,
	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride,
	vec4_t * RESTRICT oTan, size_t oTanStride, vec4_t * RESTRICT oBin, size_t oBinStride );
void R_AnimateX42VertsNormsOnly_SSE( const affine_t *iinfs, uint numInfs, uint maxInfsPerVert,
	const x42vertAnim_t * RESTRICT modVertPos, const x42vertNormal_t * RESTRICT modVertNorm, uint numVerts,
	vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride );

void R_CopyX42VertsTangentBasis_SSE( const x42vertAnim_t * RESTRICT modVertPos,
	const x42vertNormal_t * RESTRICT modVertNorm, const x42vertTangent_t * RESTRICT modVertTanBasis,
	uint numVerts, vec4_t * RESTRICT oPos, size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride,
	vec4_t * RESTRICT oTan, size_t oTanStride, vec4_t * RESTRICT oBin, size_t oBinStride );
void R_CopyX42VertsNormsOnly_SSE( const x42vertAnim_t * RESTRICT modVertPos,
	const x42vertNormal_t * RESTRICT modVertNorm, uint numVerts, vec4_t * RESTRICT oPos,
	size_t oPosStride, vec4_t * RESTRICT oNorm, size_t oNormStride );
#endif

#endif