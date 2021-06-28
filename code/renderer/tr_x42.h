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


#ifndef TR_X42_H
#define TR_X42_H

/*
==============================================================================

  .x42 file format

  Grab a copy of the maya2q3 source code and read the comments in q3types.h to
  get a better understanding of x42's in-file structure. maya2q3 is free to
  download from http://svn.hermitworksentertainment.com/maya2q3.

==============================================================================
*/

typedef signed char				s8;
typedef unsigned char			u8;		//sized types, adjust on other systems!
typedef signed short int		s16;
typedef unsigned short int		u16;
typedef unsigned int			u32;

typedef u16 x42NameIndex_t;	//index of the first char of a null-terminated string in the strings blob
typedef u16 x42Index_t;

typedef float					f32;

/*
typedef f32 vec2_t[2];
typedef f32 vec3_t[3];
typedef f32 vec4_t[4];
typedef f32 quat_t[4]; //i, j, k, real
*/

typedef u8 rgba_t[4];

typedef struct shpere_t
{
	vec3_t		center;
	f32			radius;
} sphere_t;

typedef struct aabb_t
{
	vec3_t		mins;
	vec3_t		maxs;
} aabb_t;

#define X42_FLOAT_S16N_PACK( x ) ((s16)((float)(x) * 32767.0F))
#define X42_FLOAT_S16N_UNPACK( x ) ((float)(int)(x) * (1.0F / 32767.0F))

#define X42_FLOAT_S8N_PACK( x ) ((s8)((float)(x) * 127.0F))
#define X42_FLOAT_S8N_UNPACK( x ) ((float)(int)(x) * (1.0F / 127.0F))
#define X42_FLOAT_U8N_PACK( x ) ((u8)((float)(x) * 255.0F))
#define X42_FLOAT_U8N_UNPACK( x ) ((float)(int)(x) * (1.0F / 255.0F))

#define X42_IDENT						(('2' << 24) | ('4' << 16) | ('W' << 8) | 'H')

//supported versions
#define X42_VER_V5						101		//big increment in case anyone ever has to do a 4* for some unknown reason

#define X42_CURRENT_VERSION X42_VER_V5

#define X42_WEIGHTS_PER_VERT			4

#define X42_MF_HAS_NORMALS				0x0001
#define X42_MF_HAS_TANGENT_BASIS		0x0002
#define X42_MF_HAS_TEXTURE_COORDINATES	0x0004
#define X42_MF_HAS_COLORS				0x0008
#define X42_MF_UNIFORM_SCALE			0x0100

typedef struct x42Header_ident_t
{
	u32			ident;				//must be X42_IDENT
	u32			version;			//must be X42_VER_Vx (see above for valid values)
} x42Header_ident_t;

typedef struct x42Header_v5_t
{
	u16			modelFlags;			//one or more X42_MF_* values

	s16			baseFrame;
	u16			numFrames;			//may be zero

	u16			nameBlobLen;		//may be zero

	u16			numBones;			//may be zero
	u16			numAnimGroups;		//zero if numBones is zero, else must be at least 1

	u16			numPosValues;		//may be zero
	u16			numRotValues;		//may be zero
	u16			numScaleValues;		//may be zero

	u16			keyStreamLength;	//may be zero
	u16			numAnims;			//may be zero

	u16			numTags;			//may be zero
	u16			numInfluences;		//may be zero

	u16			numLods;			//may be zero iff numGroups is zero
	u16			numGroups;			//may be zero

	u32			numVerts;			//may be zero iff numGroups is zero
	u32			numIndices;			//may be zero

	aabb_t		boundingBox;
	sphere_t	boundingSphere;
} x42Header_v5_t;

typedef struct x42PackHeader_v5_t
{
	vec2_t		animPosPack[3];
	vec2_t		animScalePack;
	vec2_t		vertPosPack[3];
	vec2_t		vertTcPack[2];
} x42PackHeader_v5_t;

#define X42_MAX_FRAMES			0xFFFF
#define X42_MAX_ANIMGROUPS		32

#define X42_KT_POSITION			0	//index is the bone index, value (follows) is the position value index
#define X42_KT_SCALE			1	//index is the bone index, value (follows) is the scale value index
#define X42_KT_ROTATION			2	//index is the bone index, value (follows) is the rotation value index

typedef struct x42KeyStreamEntry_v5_t
{
	u16			type;			//one of X42_KT_*
	u16			bone;			//a valid bone index (cannot be X42_MODEL_BONE)
	u16			frame;			//the frame number, in the range [0, numFrames)
	u16			value;
} x42KeyStreamEntry_v5_t;

#define X42_NO_LOOP			((u16)0xFFFF)

typedef struct x42Animation_v5_t
{
	x42NameIndex_t	name;
	
	/*
		If lastFrame < firstFrame then the anim plays the frames
		in reverse order.
		
		If loopStart is X42_NO_LOOP then it and loopEnd are ignored.

		If lastFrame < firstFrame and loopStart is not X42_NO_LOOP
		then loopEnd must be <= to loopStart, else loopEnd must
		be >= loopStart.
	*/
	u16				firstFrame;
	u16				lastFrame;			//inclusive
	u16				loopStart;
	u16				loopEnd;			//inclusive
	
	u16				frameRate;			//in frames per *millisecond*
} x42Animation_v5_t;

typedef struct x42AnimGroup_v5_t
{
	x42NameIndex_t	name;
	u16				beginBone;
	u16				endBone;
} x42AnimGroup_v5_t;

#define X42_MODEL_BONE ((u16)0xFFFF)

#define X42_BF_NONE					0x0000
#define X42_BF_USE_INV_PARENT_SCALE	0x0001

typedef struct x42Bone_v5_t
{
	x42NameIndex_t	name;

	u16				flags;				//one or more X42_BF_* values
	u16				parentIdx;			//index of the parent bone, must be less than this bone's index
										//X42_MODEL_BONE denotes a top-level bone

	f32				extent;				//the furthest reaches of this bone's influence
} x42Bone_v5_t;

typedef struct x42Influence_v5_t
{
	u16				bone;				//indexes into the bones array
	affine_t		meshToBone;			//transforms points from bind pose into bone space
} x42Influence_v5_t;

typedef struct x42Tag_v5_t
{
	x42NameIndex_t	name;
	u16				bone;
	affine_t		tagMatrix;
} x42Tag_v5_t;

#define X42_PT_POINTS			0x0000	//GL_POINTS

#define X42_PT_LINE_LIST		0x0001	//GL_LINES
#define X42_PT_LINE_STRIP		0x0003	//GL_LINE_STRIP

#define X42_PT_TRIANGLE_LIST	0x0004	//GL_TRIANGLES
#define X42_PT_TRIANGLE_STRIP	0x0005	//GL_TRIANGLE_STRIP
#define X42_PT_TRIANGLE_FAN		0x0006	//GL_TRIANGLE_FAN

#define X42_MAX_INFLUENCES_PER_BATCH_V5	60

#define X42_NO_INDICES ((u32)0xFFFFFFFF)

typedef struct x42Group_v5_t
{
	x42NameIndex_t	material;
	x42NameIndex_t	surfaceName;

	/*
		This is an optimization hint for the renderer.
		It is set to 1, 2, 3, or 4.

		If the value is one, then each vertex in this
		group will have one influence index in the first
		element of its indices array and the first element
		of its weights array will be set to 1.0F. The last
		three elements of its weights and indices arrays
		will be zero.

		If the value is two, then each vertex in this
		group will have two influence indices in the first
		two elements of its indices array and the first
		two elements of its weights array will add up
		to 1.0F. The last two elements of the its weights
		and indices arrays will be zero.

		And so on.
	*/
	u16				maxVertInfluences;

	/*
		Indices into the global influences array.
	
		Note that vertices in this group index
		into *this* influence array, not the global
		one, that is to say that given:
	
			a vertex weighted against influence 0
			and an influences array starting with 4, 3, 9, 0, 2...
	
		the vertex will be written with an influence
		index of 3 since influence 0 is loaded as the
		fourth (zero-indexed) influence of this group.
	*/
	u16				numInfluences;
	u16				influences[X42_MAX_INFLUENCES_PER_BATCH_V5];
	
	u16				primType;			//the type of primitive for this group
		
	/*
		These values define the part or the vertex
		and index array that belongs to this group.

		Note that in the case of primType_t::Points
		there is *no* index data: firstIndex gets
		ignored and the renderer draws numPrims verts
		starting at firstVert and going in sequence.
		This is a DX compatibility thing - get over it.

		If firstIndex is X42_NO_INDICES then the group is
		simply drawn unindexed (pretend indices are 0, 1, 2, ... ).
	*/
	u32				firstVert;			//index of the first vertex for this group
	u32				numVerts;			//number of vertices referenced in this group
	u32				firstIndex;			//index of the first index for this group,
										//if this is X42_NO_INDICES then the group's vertices aren't indexed
	u32				numElems;			//number of indices (or vertices if non-indexed) to draw
} x42Group_v5_t;

typedef struct x42LodRange_v5_t
{
	u16				lodName;
	u16				firstGroup;
	u16				numGroups;
} x42LodRange_v5_t;

/*
==============================================================================

  .x42 runtime

==============================================================================
*/

typedef struct x42header_t
{
	x42Header_ident_t	ident;

	u16					modelFlags;			//one or more X42_MF_* values

	s16					baseFrame;
	u16					numFrames;

	u16					nameBlobLen;

	u16					numBones;
	u16					numAnimGroups;

	u16					numPosValues;
	u16					numRotValues;
	u16					numScaleValues;

	u16					keyStreamLength;
	u16					numAnims;

	u16					numTags;
	u16					numInfluences;

	u16					numLods;
	u16					numGroups;

	u32					numVerts;
	u32					numIndices;

	aabb_t				boundingBox;
	sphere_t			boundingSphere;
} x42header_t;

typedef x42KeyStreamEntry_v5_t	x42keyStreamEntry_t;
typedef x42Animation_v5_t		x42animation_t;
typedef x42AnimGroup_v5_t		x42animGroup_t;
typedef x42Bone_v5_t			x42bone_t;
typedef x42Influence_v5_t		x42influence_t;
typedef x42Tag_v5_t				x42tag_t;
typedef x42LodRange_v5_t		x42lodRange_t;
typedef x42Group_v5_t			x42group_t;
typedef struct x42vertAnim_t
{
	vec3_t			pos;
	u8				idx[X42_WEIGHTS_PER_VERT];
	f32				wt[X42_WEIGHTS_PER_VERT];
} x42vertAnim_t;
typedef struct x42vertNormal_t
{
	vec3_t			norm;
	f32				pad;
} x42vertNormal_t;
typedef struct x42vertTangent_t
{
	vec3_t			tan;
	f32				nfac0;
	vec3_t			bit;
	f32				nfac1;				//same as nfac0, just repeated
} x42vertTangent_t;
typedef x42Index_t				x42index_t;

typedef struct x42data_t
{
	//copied in from libx42
	x42header_t			header;
	
	x42keyStreamEntry_t	*keyStream;
	x42animation_t		*animations;

	vec3_t				*posValues;
	quat_t				*rotValues;
	vec3_t				*scaleValues;

	x42animGroup_t		*animGroups;	//may be null
	x42bone_t			*bones;			//may be null
	u8					*boneGroups;
	x42influence_t		*influences;	//may be null
	x42tag_t			*tags;			//may be null

	x42lodRange_t		*lods;
	x42group_t			*groups;

	x42vertAnim_t		*vertPos;
	x42vertNormal_t		*vertNorm;		//may be null
	x42vertTangent_t	*vertTan;		//may be null
	vec2_t				*vertTc;		//may be null
	rgba_t				*vertCl;		//may be null

	x42index_t			*indices;

	const char			*strings;

	//st engine specific
	int					*groupMats;
	struct x42KsStart_t	**startCaches;
	struct x42KsCache_t	*poseCaches;
} x42data_t;

typedef x42data_t x42Model_t;

typedef struct x42Surface_t
{
	surfaceType_t		surfType;
	x42Model_t			*model;
	x42group_t			*group;
	x42Pose_t			*pose;
} x42Surface_t;

void RB_SurfaceX42( x42Surface_t *s );

void R_AddX42Surfaces( trRefEntity_t *ent );

int R_LerpX42Tag( affine_t *tag, const x42Model_t *mod, int startFrame, int endFrame,  float frac, const char *tagName );

//
// animation poses
//

qhandle_t Q_EXTERNAL_CALL RE_BuildPose( qhandle_t h_mod,
	const animGroupFrame_t *groupFrames0, uint numGroupFrames0, const boneOffset_t *boneOffsets0, uint numBoneOffsets0,
	const animGroupFrame_t *groupFrames1, uint numGroupFrames1, const boneOffset_t *boneOffsets1, uint numBoneOffsets1,
	const animGroupTransition_t *frameLerps, uint numFrameLerps );
qboolean Q_EXTERNAL_CALL RE_LerpTagFromPose( affine_t *tag, qhandle_t h_mod, qhandle_t pose, const char *tagName );

//
// load
//

struct model_t;
bool R_LoadX42( struct model_t *mod, void *buffer, int filesize, const char *name );

#endif //TR_X42_H
