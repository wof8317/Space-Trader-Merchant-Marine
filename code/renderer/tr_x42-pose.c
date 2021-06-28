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

static void R_x42DoStartPadScan( const x42Model_t *mod, uint animGroup, x42KsStart_t *s, float time )
{
	const x42keyStreamEntry_t * RESTRICT ks, * RESTRICT ksEnd;
	x42KsGroupFrames_t * RESTRICT gf;

	uint beginBone, endBone;

	if( s->frame.maxFrame >= time )
		return;

	beginBone = mod->animGroups[animGroup].beginBone;
	endBone = mod->animGroups[animGroup].endBone;

	ksEnd = mod->keyStream + mod->header.keyStreamLength;
	
	gf = &s->frame;
	ks = gf->ksPos;

	gf->maxFrame = mod->header.numFrames;
	gf->ksPos = ksEnd;

	for( ; ks < ksEnd; ks++ )
	{
		x42KsBoneEntry_t *dst;

		if( ks->bone < beginBone || ks->bone >= endBone )
			continue;

		dst = s->kpairs[ks->bone - beginBone][ks->type];

		if( dst[1].frame >= time )
		{
			//would be pushing back a value that puts the min up too high, stop
			gf->maxFrame = dst[1].frame;
			gf->ksPos = ks;

			break;
		}

		if( dst[1].frame > gf->minFrame )
			//minFrame is the max of the older pair values
			gf->minFrame = dst[1].frame;

		dst[0] = dst[1];
		dst[1].frame = ks->frame;
		dst[1].value = ks->value;
	}
}

void R_x42PoseInitKsStarts( x42Model_t *mod )
{
	int i, ig;
	const x42keyStreamEntry_t *ksEnd = mod->keyStream + mod->header.keyStreamLength;

	int nKsStarts = r_x42cacheCtl->integer;
	if( nKsStarts < 0 )
		nKsStarts = 0;

	mod->startCaches = (x42KsStart_t**)ri.Hunk_Alloc( sizeof( x42KsStart_t* ) * mod->header.numAnimGroups, h_low );

	for( ig = 0; ig < mod->header.numAnimGroups; ig++ )
	{
		int nBones, cb;
		const x42keyStreamEntry_t * RESTRICT ks;
		const x42animGroup_t *g = mod->animGroups + ig;
		x42KsStart_t *s, *d;

		nBones = g->endBone - g->beginBone;
		cb = sizeof( x42KsBoneEntries_t ) * nBones;
		mod->startCaches[ig] = (x42KsStart_t*)ri.Hunk_Alloc( sizeof( x42KsStart_t ) * (nKsStarts + 1), h_low );

		s = mod->startCaches[ig] + 0;

		s->lastusedFrame = 0;
		s->kpairs = (x42KsBoneEntries_t*)ri.Hunk_Alloc( cb, h_low );
		
		//init this one, copy it to the others later
		ks = mod->keyStream + 6 * g->beginBone;
		for( i = 0; i < nBones; i++ )
		{
			int it;
			for( it = 0; it < 3; it++ )
			{
				s->kpairs[i][it][0].frame = ks[0].frame;
				s->kpairs[i][it][0].value = ks[0].value;
				s->kpairs[i][it][1].frame = ks[1].frame;
				s->kpairs[i][it][1].value = ks[1].value;

				ks += 2;
			}
		}

		s->frame.minFrame = 0;
		s->frame.maxFrame = mod->header.numFrames;
		s->frame.ksPos = ksEnd;

		for( ks = mod->keyStream + 6 * mod->header.numBones; ks < ksEnd; ks++ )
		{
			if( ks->bone < g->beginBone || ks->bone >= g->endBone )
				continue;

			s->frame.maxFrame = s->kpairs[ks->bone - g->beginBone][ks->type][1].frame;
			s->frame.ksPos = ks;
			break;
		}

		//init the rest of the start pads
		for( i = 0; i < nKsStarts; i++ )
		{
			s = mod->startCaches[ig] + i;
			d = mod->startCaches[ig] + i + 1;

			//clone the one before
			d->frame = s->frame;
			d->lastusedFrame = 0;
			d->kpairs = (x42KsBoneEntries_t*)ri.Hunk_Alloc( cb, h_low );
			Com_Memcpy( d->kpairs, s->kpairs, cb );

			//advance it into the stream
			R_x42DoStartPadScan( mod, ig, d,
				mod->header.numFrames * ((float)(i + 1) / (float)(nKsStarts + 1)) );
		}
	}
}

static void R_x42InitKsCache( const x42Model_t *mod, x42KsCache_t *cache, const float * RESTRICT times )
{
	int i;

	int nKsStarts = r_x42cacheCtl->integer;
	if( nKsStarts < 0 )
		nKsStarts = 0;

	for( i = 0; i < mod->header.numAnimGroups; i++ )
	{
		int j;
		x42KsGroupFrames_t *gf = cache->groupFrames + i;
		const x42animGroup_t *g = mod->animGroups + i;

		if( gf->ksPos && gf->minFrame <= times[i] )
			continue;

		for( j = 0; j < nKsStarts; j++ )
		{
			if( mod->startCaches[i][j + 1].frame.minFrame > times[i] )
				break;
		}

		//have picked the one we want to copy - copy it!
		*gf = mod->startCaches[i][j].frame;
		Com_Memcpy( cache->kpairs + g->beginBone, mod->startCaches[i][j].kpairs,
			sizeof( x42KsBoneEntries_t ) * (g->endBone - g->beginBone) );
	}
}

static x42KsCache_t* R_x42GetKsCache( const x42Model_t *mod,
	int currFrame, const float *groupTimes )
{
	/*
		TODO: this is *NOT* thread safe, so something needs to be done about
		front end/back end calling if r_smp is on!!!
	*/

	uint i;
	float bestDist = 0.0F;
	x42KsCache_t **k, *best, *ret;

	for( best = NULL, k = (x42KsCache_t**)&mod->poseCaches; *k; k = &(*k)->next )
	{
		bool good;
		float currDist;
		
		good = true;
		for( i = 0; i < mod->header.numAnimGroups; i++ )
		{
			if( groupTimes[i] < (*k)->groupFrames[i].minFrame ||
				groupTimes[i] > (*k)->groupFrames[i].maxFrame )
			{
				good = false;
				break;
			}
		}

		if( good )
			return *k;

		if( (*k)->lastUsedFrame == currFrame )
			//can't reuse something that's been used
			//this frame unless it's an exact match
			continue;

		currDist = 0.0F;
		for( i = 0; i < mod->header.numAnimGroups; i++ )
		{
			float d;
			if( (*k)->groupFrames[i].minFrame > groupTimes[i] )
				d = groupTimes[i];
			else
				d = groupTimes[i] - (*k)->groupFrames[i].minFrame;

			if( d > currDist )
				currDist = d;
		}

		if( !best )
		{
			best = *k;
			bestDist = currDist;
			continue;
		}

		if( currDist >= bestDist )
			continue;

		best = *k;
		bestDist = currDist;
		continue;
	}

	if( best )
		//found one!
		return best;

	//create a new cache
	ret = (x42KsCache_t*)ri.Hunk_Alloc( sizeof( x42KsCache_t ), h_low );
	ret->groupFrames = (x42KsGroupFrames_t*)ri.Hunk_Alloc( sizeof( x42KsGroupFrames_t ) * mod->header.numAnimGroups, h_low );
	ret->kpairs = (x42KsBoneEntries_t*)ri.Hunk_Alloc( sizeof( x42KsBoneEntries_t ) * mod->header.numBones, h_low );
	ret->lastUsedFrame = currFrame - 1;
	ret->next = NULL;

	R_x42InitKsCache( mod, ret, groupTimes );

	*k = ret;

	return ret;
}

static void R_x42DoKsScan( const x42Model_t *mod, x42KsCache_t *cache,
	const float * RESTRICT times )
{
	uint i, gMask;

	const x42keyStreamEntry_t * RESTRICT ks, *ksEnd = mod->keyStream + mod->header.keyStreamLength;

	gMask = 0;
	ks = NULL;

	for( i = 0; i < mod->header.numAnimGroups; i++ )
	{
		//minFrame is guaranteed to be <= times[i] by caller
		if( cache->groupFrames[i].maxFrame >= times[i] )
			continue;

		gMask |= 1 << i;

		if( !ks )
		{
			ks = cache->groupFrames[i].ksPos;
		}
		else if( cache->groupFrames[i].ksPos < ks )
		{
			ks = cache->groupFrames[i].ksPos;
		}
	}

	if( !gMask )
		return;
	
	for( ; ks < ksEnd; ks++ )
	{
		uint group;
		x42KsBoneEntry_t *dst;
		x42KsGroupFrames_t * RESTRICT gf;

		group = mod->boneGroups[ks->bone];
		
		if( !(gMask & (1 << group)) )
			//done with this group
			continue;

		gf = cache->groupFrames + group;

		if( ks < gf->ksPos )
			//not yet applicable to this group
			continue;

		dst = cache->kpairs[ks->bone][ks->type];

		if( dst[1].frame >= times[group] )
		{
			//would be pushing back a value that puts the min up too high, stop
			gf->maxFrame = dst[1].frame;
			gf->ksPos = ks;
			gMask &= ~(1 << group);

			if( !gMask )
				break;

			continue;
		}

		if( dst[1].frame > gf->minFrame )
			//minFrame is the max of the older pair values
			gf->minFrame = dst[1].frame;

		dst[0] = dst[1];
		dst[1].frame = ks->frame;
		dst[1].value = ks->value;
	}

	if( gMask )
	{
		for( i = 0; i < mod->header.numAnimGroups; i++ )
		{
			if( gMask & (1 << i) )
			{
				cache->groupFrames[i].maxFrame = mod->header.numFrames;
				cache->groupFrames[i].ksPos = ksEnd;
			}
		}
	}
}

static void R_x42UpdateKsCache( const x42Model_t *mod, x42KsCache_t *cache, const float *groupTimes )
{
	R_x42InitKsCache( mod, cache, groupTimes ); //internally skips anything it can
	R_x42DoKsScan( mod, cache, groupTimes );
}

static void R_x42LerpTracks( const x42Model_t *mod, const x42KsCache_t *cache,
	vec3_t * RESTRICT pos, quat_t * RESTRICT rot, vec3_t * RESTRICT scale,
	const float *groupTimes )
{
	const vec3_t * RESTRICT pv = mod->posValues;
	const quat_t * RESTRICT rv = mod->rotValues;
	const vec3_t * RESTRICT sv = mod->scaleValues;

	uint ig;
	for( ig = 0; ig < mod->header.numAnimGroups; ig++ )
	{
		uint ib;

		uint beginBone = mod->animGroups[ig].beginBone;
		uint endBone = mod->animGroups[ig].endBone;

		float frame = groupTimes[ig];

		for( ib = beginBone; ib < endBone; ib++ )
		{
			const x42KsBoneEntry_t *p;
			
			p = cache->kpairs[ib][X42_KT_POSITION];

			if( p[0].value == p[1].value )
				VectorCopy( pv[p[0].value], pos[ib] );
			else
			{
				float s = (int)p[0].frame;
				float e = (int)p[1].frame;

				float t = (frame - s) / (e - s);

				Vec3_Lrp( pos[ib], pv[p[0].value], pv[p[1].value], t );
			}

			p = cache->kpairs[ib][X42_KT_SCALE];

			if( p[0].value == p[1].value )
				VectorCopy( sv[p[0].value], scale[ib] );
			else
			{
				float s = (int)p[0].frame;
				float e = (int)p[1].frame;

				float t = (frame - s) / (e - s);

				Vec3_Lrp( scale[ib], sv[p[0].value], sv[p[1].value], t );
			}

			p = cache->kpairs[ib][X42_KT_ROTATION];

			if( p[0].value == p[1].value )
				Quat_Cpy( rot[ib], rv[p[0].value] );
			else
			{
				float s = (int)p[0].frame;
				float e = (int)p[1].frame;

				float t = (frame - s) / (e - s);

				quat_interp( rot[ib], rv[p[0].value], rv[p[1].value], t );
			}
		}
	}
}

/*
	Builds a pose based on a single set of time values.
*/
static void R_x42PoseGetSinglePoseTracks( vec3_t * RESTRICT posTracks, quat_t * RESTRICT rotTracks,
	vec3_t * RESTRICT scaleTracks, int currFrame, const x42Model_t *mod, const animGroupFrame_t * groupFrames,
	uint numGroupFrames, const boneOffset_t * boneOffsets, uint numBoneOffsets )
{
	uint i, j;
	bool useTwoTimes;
	float *times0, *times1;
	animGroupFrame_t *adjustedGroupFrames;
	x42KsCache_t *cache;

	if( !numGroupFrames )
		ri.Error( ERR_DROP, "BuildPose: invalid group frames." );

	if( groupFrames[0].animGroup != 0 )
		ri.Error( ERR_DROP, "BuildPose: invalid group frames." );

	for( i = 1; i < numGroupFrames; i++ )
		if( groupFrames[i].animGroup <= groupFrames[i - 1].animGroup )
			ri.Error( ERR_DROP, "BuildPose: invalid group frames." );

	adjustedGroupFrames = (animGroupFrame_t*)ri.Hunk_AllocateTempMemory( sizeof( animGroupFrame_t ) * numGroupFrames );
	Com_Memcpy( adjustedGroupFrames, groupFrames, sizeof( animGroupFrame_t ) * numGroupFrames );

	for( i = 0; i < numGroupFrames; i++ )
	{
		animGroupFrame_t *gf = adjustedGroupFrames + i;

		gf->frame0 -= (int)mod->header.baseFrame;
		gf->frame1 -= (int)mod->header.baseFrame;

		if( gf->frame0 < 0 )
			gf->frame0 = 0;

		if( gf->frame1 < 0 )
			gf->frame1 = 0;

		if( gf->wrapFrames )
		{
			gf->frame0 %= mod->header.numFrames;
			gf->frame1 %= mod->header.numFrames;
		}
		else
		{
			if( (uint)gf->frame0 >= mod->header.numFrames )
				gf->frame0 = mod->header.numFrames - 1;

			if( (uint)gf->frame1 >= mod->header.numFrames )
				gf->frame1 = mod->header.numFrames - 1;		
		}

		if( gf->interp < 0.0F )
			gf->interp = 0.0F;
		else if( gf->interp > 1.0F )
			gf->interp = 1.0F;
	}

	useTwoTimes = false;
	for( i = 0; i < numGroupFrames; i++ )
	{
		int d = adjustedGroupFrames[i].frame1 - adjustedGroupFrames[i].frame0;
		if( d < 0 )
			useTwoTimes = true;

		if( d > 1 )
			useTwoTimes = true;
	}

	times0 = (float*)ri.Hunk_AllocateTempMemory( sizeof( float ) * mod->header.numAnimGroups * 2 );
	times1 = times0 + mod->header.numAnimGroups;

	j = 0;
	for( i = 0; i < mod->header.numAnimGroups; i++ )
	{
		while( j < numGroupFrames - 1 && adjustedGroupFrames[j + 1].animGroup <= i )
			j++;

		if( useTwoTimes )
		{
			times0[i] = (float)adjustedGroupFrames[j].frame0;
			times1[i] = (float)adjustedGroupFrames[j].frame1;
		}
		else
		{
			times0[i] = (float)adjustedGroupFrames[j].frame0 +
				(float)(adjustedGroupFrames[j].frame1 - adjustedGroupFrames[j].frame0) *
				adjustedGroupFrames[j].interp;
		}
	}

	cache = R_x42GetKsCache( mod, currFrame, times0 );
	cache->lastUsedFrame = currFrame;

	if( useTwoTimes )
	{
		//for non-consecutive frames we have to lerp out each
		//frame and then lerp between the two results
		vec3_t * RESTRICT posTracks2 = (vec3_t*)ri.Hunk_AllocateTempMemory( sizeof( vec3_t ) * mod->header.numBones );
		byte *_rotTracks2 = ri.Hunk_AllocateTempMemory( sizeof( quat_t ) * mod->header.numBones + 16 );
		quat_t * RESTRICT rotTracks2 = (quat_t*)AlignP( _rotTracks2, 16 );
		vec3_t * RESTRICT scaleTracks2 = (vec3_t*)ri.Hunk_AllocateTempMemory( sizeof( vec3_t ) * mod->header.numBones );

		uint currGroup = 0;

		R_x42UpdateKsCache( mod, cache, times0 );
		R_x42LerpTracks( mod, cache, posTracks, rotTracks, scaleTracks, times0 );

		R_x42UpdateKsCache( mod, cache, times1 );
		R_x42LerpTracks( mod, cache, posTracks2, rotTracks2, scaleTracks2, times1 );

		for( i = 0; i < mod->header.numBones; i++ )
		{
			float interp;

			//might be time to advance a group
			while( adjustedGroupFrames[currGroup].animGroup < mod->boneGroups[i] && currGroup < numGroupFrames - 1 )
			{
				if( adjustedGroupFrames[currGroup + 1].animGroup <= mod->boneGroups[i] )
					currGroup++;
				else
					break;
			}

			interp = adjustedGroupFrames[currGroup].interp;

			Vec3_Lrp( posTracks[i], posTracks[i], posTracks2[i], interp );
			Vec3_Lrp( scaleTracks[i], scaleTracks[i], scaleTracks2[i], interp );
			quat_interp( rotTracks[i], rotTracks[i], rotTracks2[i], interp );
		}

		//note, these frees must come in *stack* order or we'll blow the hunk alloc
		ri.Hunk_FreeTempMemory( scaleTracks2 );
		ri.Hunk_FreeTempMemory( _rotTracks2 );
		ri.Hunk_FreeTempMemory( posTracks2 );
	}
	else
	{
		R_x42UpdateKsCache( mod, cache, times0 );
		R_x42LerpTracks( mod, cache, posTracks, rotTracks, scaleTracks, times0 );
	}

	for( i = 0; i < numBoneOffsets; i++ )
	{
		const boneOffset_t *bo = boneOffsets + i;

		uint j;
		for( j = 0; j < mod->header.numBones; j++ )
		{
			if( !strcmp( mod->strings + mod->bones[j].name, bo->boneName ) )
			{
				quat_t tmp;

				VectorAdd( posTracks[j], bo->pos, posTracks[j] );
				VectorAdd( scaleTracks[j], bo->scale, scaleTracks[j] );

				Quat_Mul( tmp, bo->rot, rotTracks[j] );
				Quat_Cpy( rotTracks[j], tmp );

				break;
			}
		}
	}

	ri.Hunk_FreeTempMemory( times0 );
	ri.Hunk_FreeTempMemory( adjustedGroupFrames );
}

/*
	Builds a pose based on two blended time values.
*/
static void R_x42PoseGetBlendedPoseTracks( vec3_t * RESTRICT posTracks, quat_t * RESTRICT rotTracks,
	vec3_t * RESTRICT scaleTracks, const x42Model_t *mod, int currFrame,
	const animGroupFrame_t * groupFrames0, uint numGroupFrames0, const boneOffset_t * boneOffsets0, uint numBoneOffsets0,
	const animGroupFrame_t * groupFrames1, uint numGroupFrames1, const boneOffset_t * boneOffsets1, uint numBoneOffsets1,
	const animGroupTransition_t *frameLerps, uint numFrameLerps )
{
	uint i;
	qboolean allZero, allOne;

	if( !numFrameLerps )
		ri.Error( ERR_DROP, "Must have at least one frame lerp.\n" );
	if( frameLerps[0].animGroup != 0 )
		ri.Error( ERR_DROP, "First frame lerp must be for animation group zero.\n" );
	for( i = 1; i < numFrameLerps; i++ )
		if( frameLerps[i].animGroup <= frameLerps[i - 1].animGroup )
			ri.Error( ERR_DROP, "Invalid frameLerps, animGroup must be strictly increasing.\n" );

	allZero = qtrue;
	allOne = qtrue;
	for( i = 0; i < numFrameLerps && (allZero || allOne); i++ )
	{
		if( frameLerps[i].interp != 0 )
			allZero = qfalse;
		
		if( frameLerps[i].interp != 1 )
			allOne = qfalse;
	}

	if( allZero )
	{
		R_x42PoseGetSinglePoseTracks( posTracks, rotTracks, scaleTracks, currFrame,
			mod, groupFrames0, numGroupFrames0, boneOffsets0, numBoneOffsets0 );
	}
	else if( allOne )
	{
		R_x42PoseGetSinglePoseTracks( posTracks, rotTracks, scaleTracks, currFrame,
			mod, groupFrames1, numGroupFrames1, boneOffsets1, numBoneOffsets1 );
	}
	else
	{
		uint i;
		uint currGroup;

		vec3_t * RESTRICT posTracks2 = (vec3_t*)ri.Hunk_AllocateTempMemory( sizeof( vec3_t ) * mod->header.numBones );
		byte *_rotTracks2 = ri.Hunk_AllocateTempMemory( sizeof( quat_t ) * mod->header.numBones + 16 );
		quat_t * RESTRICT rotTracks2 = (quat_t*)AlignP( _rotTracks2, 16 );
		vec3_t * RESTRICT scaleTracks2 = (vec3_t*)ri.Hunk_AllocateTempMemory( sizeof( vec3_t ) * mod->header.numBones );

		R_x42PoseGetSinglePoseTracks( posTracks, rotTracks, scaleTracks, currFrame,
			mod, groupFrames0, numGroupFrames0, boneOffsets0, numBoneOffsets0 );
		R_x42PoseGetSinglePoseTracks( posTracks2, rotTracks2, scaleTracks2, currFrame,
			mod, groupFrames1, numGroupFrames1, boneOffsets1, numBoneOffsets1 );

		currGroup = 0;
		for( i = 0; i < mod->header.numBones; i++ )
		{
			float interp;

			//might be time to advance a group
			while( frameLerps[currGroup].animGroup < mod->boneGroups[i] &&
				currGroup < numFrameLerps - 1 && frameLerps[currGroup + 1].animGroup <= mod->boneGroups[i] )
			{
				currGroup++;
			}

			interp = frameLerps[currGroup].interp;

			Vec3_Lrp( posTracks[i], posTracks[i], posTracks2[i], interp );
			Vec3_Lrp( scaleTracks[i], scaleTracks[i], scaleTracks2[i], interp );
			quat_interp( rotTracks[i], rotTracks[i], rotTracks2[i], interp );
		}

		ri.Hunk_FreeTempMemory( scaleTracks2 );
		ri.Hunk_FreeTempMemory( _rotTracks2 );
		ri.Hunk_FreeTempMemory( posTracks2 );
	}
}

/*
	Collapses a set of pose data tracks into an array of bone and tag matrices.
*/
affine_t x42_m2q3 =
{
	{
		{ -1, 0, 0 },
		{ 0, 0, 1 },
		{ 0, 1, 0 }
	},
	{ 0, 0, 0 }
};

static void R_x42PoseCollapseToMatrices( x42Pose_t * out, const x42Model_t *mod, vec3_t * RESTRICT posTracks,
	quat_t * RESTRICT rotTracks, vec3_t * RESTRICT scaleTracks )
{
	uint i;

	for( i = 0; i < mod->header.numBones; i++ )
	{
		affine_t * RESTRICT o = out->boneMats + i;

		//none of our models make use of x42's BF_USEINVPARENTSCALE so
		//we don't have to handle that case in this loop -PD

		QuatToAffine( o, rotTracks[i] );

		o->axis[0][0] *= scaleTracks[i][0];
		o->axis[0][1] *= scaleTracks[i][0];
		o->axis[0][2] *= scaleTracks[i][0];

		o->axis[1][0] *= scaleTracks[i][1];
		o->axis[1][1] *= scaleTracks[i][1];
		o->axis[1][2] *= scaleTracks[i][1];

		o->axis[2][0] *= scaleTracks[i][2];
		o->axis[2][1] *= scaleTracks[i][2];
		o->axis[2][2] *= scaleTracks[i][2];

		o->origin[0] = posTracks[i][0];
		o->origin[1] = posTracks[i][1];
		o->origin[2] = posTracks[i][2];
	}

	for( i = 0; i < mod->header.numBones; i++ )
	{
		affine_t tmp;
		const x42bone_t *b = mod->bones + i;
		
		Affine_Mul( &tmp,
			(b->parentIdx != X42_MODEL_BONE) ? out->boneMats + b->parentIdx : &x42_m2q3,
			out->boneMats + i );
		Affine_Cpy( out->boneMats + i, &tmp );
	}

	for( i = 0; i < mod->header.numTags; i++ )
	{
		const x42tag_t *t = mod->tags + i;

		Affine_Mul( out->tagMats + i,
			(t->bone != X42_MODEL_BONE) ? out->boneMats + t->bone : &x42_m2q3,
			&t->tagMatrix );
	}
}

/*
	Builds a pose.

	Note: if framesLerp is *exactly* 0, groupFrames1 and boneOffsets1 are ignored and may be NULL. Similarly,
	if framesLerp is *exactly* 1, groupFrames0 and boneOffsets0 may be NULL.
*/
void R_x42PoseBuildPose( x42Pose_t * out, const x42Model_t *mod, int currFrame,
	const animGroupFrame_t * groupFrames0, uint numGroupFrames0, const boneOffset_t * boneOffsets0, uint numBoneOffsets0,
	const animGroupFrame_t * groupFrames1, uint numGroupFrames1, const boneOffset_t * boneOffsets1, uint numBoneOffsets1,
	const animGroupTransition_t *frameLerps, uint numFrameLerps )
{
	uint i;

	vec3_t * RESTRICT posTracks;
	byte *_rotTracks;
	quat_t * RESTRICT rotTracks;
	vec3_t * RESTRICT scaleTracks;

	if( !numFrameLerps )
		ri.Error( ERR_DROP, "Must have at least one frame lerp.\n" );
	if( frameLerps[0].animGroup != 0 )
		ri.Error( ERR_DROP, "First frame lerp must be for animation group zero.\n" );
	for( i = 1; i < numFrameLerps; i++ )
		if( frameLerps[i].animGroup <= frameLerps[i - 1].animGroup )
			ri.Error( ERR_DROP, "Invalid frameLerps, animGroup must be strictly increasing.\n" );

	posTracks = (vec3_t*)ri.Hunk_AllocateTempMemory( sizeof( vec3_t ) * mod->header.numBones );
	_rotTracks = ri.Hunk_AllocateTempMemory( sizeof( quat_t ) * mod->header.numBones + 16 );
	rotTracks = (quat_t*)AlignP( _rotTracks, 16 );
	scaleTracks = (vec3_t*)ri.Hunk_AllocateTempMemory( sizeof( vec3_t ) * mod->header.numBones );

	R_x42PoseGetBlendedPoseTracks( posTracks, rotTracks, scaleTracks, mod, currFrame,
		groupFrames0, numGroupFrames0, boneOffsets0, numBoneOffsets0,
		groupFrames1, numGroupFrames1, boneOffsets1, numBoneOffsets1,
		frameLerps, numFrameLerps );
	
	R_x42PoseCollapseToMatrices( out, mod, posTracks, rotTracks, scaleTracks );

	ri.Hunk_FreeTempMemory( scaleTracks );
	ri.Hunk_FreeTempMemory( _rotTracks );
	ri.Hunk_FreeTempMemory( posTracks );
}