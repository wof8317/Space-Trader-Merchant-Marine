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

/*
	Must always have at least one group frame specified. The first group
	frame *must* be for group zero. Entries must be in order.
*/
qhandle_t Q_EXTERNAL_CALL RE_BuildPose( qhandle_t h_mod,
	const animGroupFrame_t *groupFrames0, uint numGroupFrames0, const boneOffset_t *boneOffsets0, uint numBoneOffsets0,
	const animGroupFrame_t *groupFrames1, uint numGroupFrames1, const boneOffset_t *boneOffsets1, uint numBoneOffsets1,
	const animGroupTransition_t *frameLerps, uint numFrameLerps )
{
	model_t *mod = R_GetModelByHandle( h_mod );
	if( !mod )
		return 0;

	switch( mod->type )
	{
		case MOD_X42:
			{
				x42Model_t *m = mod->x42;
				x42Pose_t *ret = (x42Pose_t*)ri.Hunk_AllocateFrameMemory(
					sizeof( x42Pose_t ) + sizeof( affine_t ) * (m->header.numBones + m->header.numTags) );

				ret->numBones = m->header.numBones;
				ret->boneMats = (affine_t*)((byte*)ret + sizeof( x42Pose_t ));
				ret->numTags = m->header.numTags;
				ret->tagMats = ret->boneMats + ret->numBones;

				R_x42PoseBuildPose( ret, m, tr.frameCount, groupFrames0, numGroupFrames0, boneOffsets0, numBoneOffsets0,
					groupFrames1, numGroupFrames1, boneOffsets1, numBoneOffsets1, frameLerps, numFrameLerps );

				return (qhandle_t)ret;
			}
			break;
		case MOD_BAD:
		case MOD_BRUSH:
			return 0;
			break;
	}

	return 0;
}

qboolean R_LerpX42TagFromPose( affine_t *tag, const x42Model_t *mod, const x42Pose_t *pose,  const char *tagName )
{
	uint i;
	for( i = 0; i < mod->header.numTags; i++ )
		if( Q_stricmp( mod->strings + mod->tags[i].name, tagName ) == 0 )
		{
			Com_Memcpy( tag, pose->tagMats + i, sizeof( affine_t ) );
			return qtrue;
		}

	//otherwise...
	Affine_Set( tag, 0, 0 );
	return qfalse;
}

int R_LerpX42Tag( affine_t *tag, const x42Model_t *mod, int startFrame, int endFrame, float frac, const char *tagName )
{
	int ret;
	x42Pose_t *pose;
	animGroupFrame_t frame;
	animGroupTransition_t trans;

	/*
		Perf warning:

		This is not an efficient way to do things. It's just here to catch horrible
		odd cases. The correct way is for the client to build a pose and use the pose
		to get the tag values. You have been warned!
	*/
	pose = (x42Pose_t*)ri.Hunk_AllocateTempMemory(
		sizeof( x42Pose_t ) + sizeof( affine_t ) * (mod->header.numBones + mod->header.numTags) );

	pose->numBones = mod->header.numBones;
	pose->boneMats = (affine_t*)((byte*)pose + sizeof( x42Pose_t ));
	pose->numTags = mod->header.numTags;
	pose->tagMats = pose->boneMats + pose->numBones;

	startFrame %= mod->header.numFrames;
	endFrame %= mod->header.numFrames;
	
	frame.animGroup = 0;
	frame.frame0 = startFrame;
	frame.frame1 = endFrame;
	frame.interp = frac;
	frame.wrapFrames = qtrue;

	trans.animGroup = 0;
	trans.interp = 0;

	R_x42PoseBuildPose( pose, mod, tr.frameCount, &frame, 1, NULL, 0, NULL, 0, NULL, 0, &trans, 1 );
							  
	ret = R_LerpX42TagFromPose( tag, mod, pose, tagName );

	ri.Hunk_FreeTempMemory( pose );

	return ret;
}

qboolean Q_EXTERNAL_CALL RE_LerpTagFromPose( affine_t *tag, qhandle_t h_mod, qhandle_t pose, const char *tagName )
{
	model_t *mod = R_GetModelByHandle( h_mod );
	if( !mod )
		return qfalse;

	switch( mod->type )
	{
		case MOD_X42:
			{
				return R_LerpX42TagFromPose( tag, mod->x42, (x42Pose_t*)pose, tagName );
			}
			break;

		case MOD_BAD:
		case MOD_BRUSH:
			return qfalse;
			break;
	}

	return qfalse;
}

static void R_X42ModelGetBounds( vec3_t bounds[2], const x42Model_t *m, const x42Pose_t * RESTRICT pose, trRefEntity_t *ent )
{
	uint i;
	vec3_t tmp;

	if( !m->header.numBones )
	{
		VectorSet( bounds[0], -32, -32, -32 );
		VectorSet( bounds[1], 32, 32, 32 );
	}
	
	tmp[0] = tmp[1] = tmp[2] = m->bones[0].extent;

	VectorSubtract( pose->boneMats[0].origin, tmp, bounds[0] );
	VectorAdd( pose->boneMats[0].origin, tmp, bounds[1] );
			
	for( i = 1; i < pose->numBones; i++ )
	{
		vec3_t vm, vM;

		tmp[0] = tmp[1] = tmp[2] = m->bones[i].extent;
		VectorSubtract( pose->boneMats[i].origin, tmp, vm );
		VectorAdd( pose->boneMats[i].origin, tmp, vM );

		AddPointToBounds( vm, bounds[0], bounds[1] );
		AddPointToBounds( vM, bounds[0], bounds[1] );
	}
}

static bool R_X42ModelInView( vec3_t bounds[2], const x42Model_t *m, const x42Pose_t * RESTRICT pose, trRefEntity_t *ent )
{
	vec3_t c;

	bounds[0][0] = -m->header.boundingBox.mins[0];
	bounds[0][1] = m->header.boundingBox.mins[2];
	bounds[0][2] = m->header.boundingBox.mins[1];

	bounds[1][0] = -m->header.boundingBox.maxs[0];
	bounds[1][1] = m->header.boundingBox.maxs[2];
	bounds[1][2] = m->header.boundingBox.maxs[1];

	//cull by AABB if we can
	switch( R_CullLocalBox( bounds ) )
	{
	case CULL_IN:
		return true;

	case CULL_CLIP:
		//do spheres
		break;

	case CULL_OUT:
	default:
		return false;
	}

	if( ent->e.nonNormalizedAxes )
		//can't do a good sphere test when an
		//arbitrary scale has been applied
		return true;

	c[0] = -m->header.boundingSphere.center[0];
	c[1] = m->header.boundingSphere.center[2];
	c[2] = m->header.boundingSphere.center[1];

	switch( R_CullLocalPointAndRadius( c, m->header.boundingSphere.radius ) )
	{
	case CULL_IN:
	case CULL_CLIP:
		return true;
	}

	return false;
}

static uint R_X42ComputeLOD( const vec3_t bounds[2], const x42Model_t *mod, trRefEntity_t *ent )
{
	float flod;
	
	if( mod->header.numLods < 2 )
	{
		// model has only 1 LOD level, skip computations and bias
		flod = 0;
	}
	else
	{
		// multiple LODs exist, so compute projected bounding sphere
		// and use that as a criteria for selecting LOD
		float radius = RadiusFromBounds( bounds[0], bounds[1] );
		float projectedRadius = R_ProjectRadius( radius, ent->e.origin );

		if( projectedRadius != 0 )
		{
			float res_factor = (float)(glConfig.vidWidth * glConfig.vidHeight) / (float)(1280 * 1024);
			float lodscale = r_lodscale->value;
			
			if( lodscale > 20 ) lodscale = 20;
			
			if( res_factor < 0 ) res_factor = 0;
			if( res_factor > 1 ) res_factor = 1;

			flod = 1.0f - projectedRadius * (lodscale * 1.4F) * res_factor;
		}
		else
		{
			// object intersects near view plane, e.g. view weapon
			flod = 0;
		}

		flod *= (float)(int)mod->header.numLods;
		
		if( flod < 0 )
		{
			flod = 0;
		}
		else if( flod >= mod->header.numLods )
		{
			flod = mod->header.numLods - 1;
		}
	}

	flod += r_lodbias->integer;
	
	if( flod < 0 )
	{
		flod = 0;
	}
	else if( flod >= mod->header.numLods )
	{
		flod = mod->header.numLods - 1;
	}

	return (uint)(int)flod;
}

void RB_SurfaceX42( x42Surface_t *s )
{
	uint i;
	uint bV, sV, bI, sI;
	bool needTanBasis, needNorm;

	if( !s->group->numElems )
		return;

	RB_CheckSurface( tess.shader, tess.fogNum, s->group->primType );
	RB_CHECKOVERFLOW( s->group->numVerts, s->group->numElems );

	bV = tess.numVertexes;

	needNorm = tess.shader->neededAttribs & VA_NORMAL;
	needTanBasis = tess.shader->neededAttribs & (VA_TANGENT | VA_BINORMAL);

	R_AnimateX42Group( s->model, s->group, s->pose, 
		tess.xyz + tess.numVertexes, sizeof( tess.xyz[0] ),
		needNorm ? tess.normal + tess.numVertexes : NULL, needNorm ? sizeof( tess.normal[0] ) : 0,
		needTanBasis ? tess.tangent + tess.numVertexes : NULL, needTanBasis ? sizeof( tess.tangent[0] ) : 0,
		needTanBasis ? tess.binormal + tess.numVertexes : NULL, needTanBasis ? sizeof( tess.binormal[0] ) : 0 );

	tess.numVertexes += s->group->numVerts;

	sV = s->group->firstVert;

	if( (tess.shader->neededAttribs & VA_TEXCOORD0) && s->model->vertTc )
	{
		for( i = 0; i < s->group->numVerts; i++ )
		{
			tess.texCoords[bV + i][0][0] = s->model->vertTc[sV + i][0];
			tess.texCoords[bV + i][0][1] = s->model->vertTc[sV + i][1];
		}
	}

	if( (tess.shader->neededAttribs & VA_COLOR) && s->model->vertCl )
	{
		for( i = 0; i < s->group->numVerts; i++ )
		{
			*(uint*)tess.vertexColors[bV + i] = *(uint*)s->model->vertCl[sV + i];
		}
	}

	bI = tess.numIndexes;
	sI = s->group->firstIndex;

	if( sI == X42_NO_INDICES )
	{
		//generate implicit indices
		for( i = 0; i < s->group->numElems; i++ )
			tess.indexes[bI + i] = (glIndex_t)(bV + i);
	}
	else
	{
		//copy the indices over
		for( i = 0; i < s->group->numElems; i++ )
			tess.indexes[bI + i] = bV + s->model->indices[sI + i];
	}

	tess.numIndexes += s->group->numElems;
}

void R_AddX42Surfaces( trRefEntity_t *ent )
{
	uint i;
	const x42Model_t *m;
	
	x42Pose_t *oldpose, *newpose;
	vec3_t bounds[2];
	
	uint iLod;
	x42lodRange_t *lod;

	if( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal )
		return;

	m = (const x42Model_t*)tr.currentModel->x42;

	if( !ent->e.poseData )
	{
		animGroupFrame_t frame;
		animGroupTransition_t trans;

		if( ent->e.renderfx & RF_WRAP_FRAMES )
		{
			ent->e.frame %= m->header.numFrames;
			ent->e.oldframe %= m->header.numFrames;
		}

		frame.animGroup = 0;
		frame.frame0 = ent->e.oldframe;
		frame.frame1 = ent->e.frame;
		frame.interp = 1.0F - ent->e.backlerp;
		frame.wrapFrames = (ent->e.renderfx & RF_WRAP_FRAMES) ? qtrue : qfalse;

		newpose = (x42Pose_t*)ri.Hunk_AllocateFrameMemory(
			sizeof( x42Pose_t ) + sizeof( affine_t ) * (m->header.numBones + m->header.numTags) );

		newpose->numBones = m->header.numBones;
		newpose->boneMats = (affine_t*)((byte*)newpose + sizeof( x42Pose_t ));
		newpose->numTags = m->header.numTags;
		newpose->tagMats = newpose->boneMats + newpose->numBones;

		trans.animGroup = 0;
		trans.interp = 0;

		R_x42PoseBuildPose( newpose, m, tr.frameCount, &frame, 1, NULL, 0, NULL, 0, NULL, 0, &trans, 1 );

		ent->e.poseData = (qhandle_t)newpose;
	}

	oldpose = (x42Pose_t*)ent->e.poseData;

	if( r_nocull->integer )
	{
		R_X42ModelGetBounds( bounds, m, oldpose, ent );
	}
	else if( !R_X42ModelInView( bounds, m, oldpose, ent ) )
	{
		return;
	}

	iLod = R_X42ComputeLOD( bounds, m, ent );
	lod = m->lods + iLod;

	R_SetupEntityLighting( &tr.refdef, ent );

	newpose = (x42Pose_t*)R_GetCommandMemory( sizeof( x42Pose_t ) );
	if( !newpose )
		return;

	newpose->boneMats = (affine_t*)R_GetCommandMemory( sizeof( affine_t ) * oldpose->numBones ); 
	newpose->tagMats = NULL;

	if( !newpose->boneMats )
		return;

	newpose->numBones = oldpose->numBones;
	newpose->numTags = 0;

	Com_Memcpy( newpose->boneMats, oldpose->boneMats, sizeof( affine_t ) * oldpose->numBones ); 

	for( i = lod->firstGroup; i < lod->firstGroup + lod->numGroups; i++ )
	{
		const x42group_t *g = m->groups + i;
		x42Surface_t *surf;
		shader_t *shader;

		if ( g->numElems == 0 )
			continue;

		surf = (x42Surface_t*)R_GetCommandMemory( sizeof( x42Surface_t ) );
		if( !surf )
			break;

		surf->surfType = SF_X42;
		surf->model = (x42Model_t*)m;
		surf->group = (x42group_t*)g;
		surf->pose = newpose;

		if( ent->e.customShader )
			shader = R_GetShaderByHandle( ent->e.customShader );
		else if( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins )
		{
			uint j;
			skin_t *skin;

			skin = R_GetSkinByHandle( ent->e.customSkin );

			// match the surface name to something in the skin file
			shader = tr.defaultShader;
			for( j = 0; j < skin->numSurfaces; j++ )
			{
				// the names have both been lowercased
				if( !strcmp( skin->surfaces[j]->name, m->strings + g->surfaceName ) )
				{
					shader = skin->surfaces[j]->shader;
					break;
				}
			}
		}
		else
			shader = R_GetShaderByHandle( m->groupMats[i] );

		if( shader->defaultShader )
			shader = tr.defaultShader;

		R_AddDrawSurf( (surfaceType_t*)surf, shader, 0, qfalse );
	}
}