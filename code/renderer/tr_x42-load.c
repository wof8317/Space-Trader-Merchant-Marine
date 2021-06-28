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

ID_INLINE void* HunkAllocAligned( size_t cb, size_t a )
{
	return AlignP( ri.Hunk_Alloc( cb + a, h_low ), a );
}

/*
	Persist flags (both for loading and saving):

	bits 0-8 = section persist flags
*/
#define X42_PERSIST_NORMALS				0x00000001	//load or save a file with normals
#define X42_PERSIST_TANGENT_BASIS		0x00000002	//load or save a file with the tangent basis
#define X42_PERSIST_TEXTURE_COORDINATES	0x00000004	//load or save a file with the texture coordinates
#define X42_PERSIST_COLORS				0x00000008

#define X42_PERSIST_EVERYTHING			0x000000FF	//load or save all file sections

#define const_cond_false (""[0])

static void fixup_persist_flags( uint *pf, const x42header_t *h )
{
	if( !(h->modelFlags & X42_MF_HAS_NORMALS) )
		*pf &= ~X42_PERSIST_NORMALS;
	if( !(h->modelFlags & X42_MF_HAS_TANGENT_BASIS) )
		*pf &= ~X42_PERSIST_TANGENT_BASIS;
	if( !(h->modelFlags &X42_MF_HAS_TEXTURE_COORDINATES) )
		*pf &= ~X42_PERSIST_TEXTURE_COORDINATES;
	if( !(h->modelFlags & X42_MF_HAS_COLORS) )
		*pf &= ~X42_PERSIST_COLORS;
}

static void x42_SetupBufferPointers( x42data_t *ret, const x42header_t *h, uint persist_flags )
{
	fixup_persist_flags( &persist_flags, h );

#define advance_out( a, cb ) HunkAllocAligned( cb, a )

	memcpy( &ret->header, h, sizeof( x42header_t ) );

	ret->keyStream		= (x42keyStreamEntry_t*)advance_out( sizeof( void* ),	sizeof( x42keyStreamEntry_t )*h->keyStreamLength );
	ret->animations		= (x42animation_t*)		advance_out( sizeof( void* ),	sizeof( x42animation_t )	* h->numAnims );

	ret->posValues		= (vec3_t*)				advance_out( sizeof( float ),	sizeof( vec3_t )			* h->numPosValues );
	ret->scaleValues	= (vec3_t*)				advance_out( sizeof( float ),	sizeof( vec3_t )			* h->numScaleValues );
	ret->rotValues		= (quat_t*)				advance_out( sizeof( float ),	sizeof( quat_t )			* h->numRotValues );

	ret->animGroups		= (x42animGroup_t*)		advance_out( sizeof( void* ),	sizeof( x42animGroup_t )	* h->numAnimGroups );
	ret->bones			= (x42bone_t*)			advance_out( sizeof( void* ),	sizeof( x42bone_t )			* h->numBones );
	ret->boneGroups		= (u8*)					advance_out( sizeof( void* ),	sizeof( u8 )				* h->numBones );

	ret->influences		= (x42influence_t*)		advance_out( sizeof( void* ),	sizeof( x42influence_t )	* h->numInfluences );
	ret->tags			= (x42tag_t*)			advance_out( sizeof( void* ),	sizeof( x42tag_t )			* h->numTags );

	ret->lods			= (x42lodRange_t*)		advance_out( sizeof( void* ),	sizeof( x42lodRange_t )		* h->numLods );
	ret->groups			= (x42group_t*)			advance_out( sizeof( void* ),	sizeof( x42group_t )		* h->numGroups );
	
	ret->vertPos		= (x42vertAnim_t*)		advance_out( sizeof( float[4] ),sizeof( x42vertAnim_t )		* h->numVerts );

	if( persist_flags & X42_PERSIST_NORMALS )
		ret->vertNorm	= (x42vertNormal_t*)	advance_out( sizeof( float[4] ),sizeof( x42vertNormal_t )	* h->numVerts );
	else
		ret->vertNorm = NULL;

	if( persist_flags & X42_PERSIST_TANGENT_BASIS )
		ret->vertTan	= (x42vertTangent_t*)	advance_out( sizeof( float[4] ),sizeof( x42vertTangent_t )	* h->numVerts );
	else
		ret->vertTan = NULL;

	if( persist_flags & X42_PERSIST_TEXTURE_COORDINATES )
		ret->vertTc = (vec2_t*)					advance_out( sizeof( float ),		sizeof( vec2_t )			* h->numVerts );
	else
		ret->vertTc = NULL;

	if( persist_flags & X42_PERSIST_COLORS )
		ret->vertCl = (rgba_t*)					advance_out( sizeof( rgba_t ),		sizeof( rgba_t )			* h->numVerts );
	else
		ret->vertCl = NULL;

	ret->indices = (x42index_t*)				advance_out( sizeof( x42index_t ),	sizeof( x42index_t )		* h->numIndices );

	ret->strings = (char*)						advance_out( sizeof( char ),		sizeof( char )				* h->nameBlobLen );
}

static bool read_packed_floats( const byte **filebuf, size_t filesize,
	size_t *inPos, const vec2_t *bounds, uint components, float *dest,
	uint elements, size_t elemStride )
{
	uint i;
	float scale[4], bias[4];
	size_t cbElem;

	if( !elements )
		return true;

	for( i = 0; i < components; i++ )
	{
		bias[i] = bounds[i][0];
		scale[i] = (bounds[i][1] - bounds[i][0]) / 65530.0F;
	}

	cbElem = sizeof( u16 ) * components;

	if( *inPos + cbElem * elements > filesize )
		return false;

	for( i = 0; i < elements; i++ )
	{
		uint c;
		const u16 *buf = (const u16*)*filebuf;
		*filebuf += cbElem;

		for( c = 0; c < components; c++ )
			dest[c] = buf[c] * scale[c] + bias[c];

		dest = (float*)((byte*)dest + elemStride);
	}

	*inPos += cbElem * elements;
	return true;
}

static bool read_packed_floats_s16n( const byte **filebuf, size_t filesize,
	size_t *inPos, uint components, float *dest, uint elements, size_t elemStride )
{
	uint i;
	size_t cbElem;

	if( !elements )
		return true;

	cbElem = sizeof( s16 ) * components;

	if( *inPos + cbElem * elements > filesize )
		return false;

	for( i = 0; i < elements; i++ )
	{
		uint c;
		
		const s16 *buf = (const s16*)*filebuf;
		*filebuf += cbElem;

		for( c = 0; c < components; c++ )
			dest[c] = X42_FLOAT_S16N_UNPACK( buf[c] );

		dest = (float*)((byte*)dest + elemStride);
	}

	*inPos += cbElem * elements;
	return true;
}

static bool read_packed_floats_s8n( const byte **filebuf, size_t filesize,
	size_t *inPos, uint components, float *dest, uint elements, size_t elemStride )
{
	uint i;
	size_t cbElem;

	if( !elements )
		return true;

	cbElem = sizeof( s8 ) * components;

	if( *inPos + cbElem * elements > filesize )
		return false;

	for( i = 0; i < elements; i++ )
	{
		uint c;
		
		const s8 *buf = (const s8*)*filebuf;
		*filebuf += cbElem;

		for( c = 0; c < components; c++ )
			dest[c] = X42_FLOAT_S8N_UNPACK( buf[c] );

		dest = (float*)((byte*)dest + elemStride);
	}

	*inPos += cbElem * elements;
	return true;
}

static bool R_LoadX42Direct( model_t *mod, const void *buffer, size_t header_size, size_t filesize, const char *name, const x42header_t *h )
{
	uint i;

	bool stat;
	x42PackHeader_v5_t pack;

	size_t inPos;
	const byte *inBuf;

	x42data_t *ret;

	const uint persist_flags = X42_PERSIST_EVERYTHING;

	ret = (x42data_t*)ri.Hunk_Alloc( sizeof( x42data_t ), h_low );
	x42_SetupBufferPointers( ret, h, persist_flags );

	inPos = sizeof( x42Header_ident_t );
	switch( h->ident.version )
	{
	case X42_VER_V5:
		inPos += sizeof( x42Header_v5_t );
		break;

	default:
		ri.Printf( PRINT_ERROR, "Invalid x42 file version in model '%s'\n", name );
		return false;
	}

	inBuf = (const byte*)buffer + inPos;

#define read_check() if( !stat ) return false; else (void)0

#define checked_read( buf, size )											\
	if( !const_cond_false )													\
	{																		\
		size_t cb = (size);													\
																			\
		stat = inPos + cb <= filesize;										\
		read_check();														\
																			\
		Com_Memcpy( buf, inBuf, cb );										\
		inBuf += cb;														\
																			\
		inPos += cb;														\
	}																		\
	else																	\
		(void)0

#define checked_align( a )													\
	if( !const_cond_false )													\
	{																		\
		size_t _a = (a);													\
																			\
		if( _a )															\
		{																	\
			size_t ofs = ((_a - (inPos % _a)) % _a);						\
																			\
			stat = inPos + ofs <= filesize;									\
			read_check();													\
																			\
			inBuf += ofs;													\
			inPos += ofs;													\
		}																	\
	}																		\
	else																	\
		(void)0

#define checked_read_a( buf, size, a )										\
	if( !const_cond_false )													\
	{																		\
		checked_align( a );													\
		checked_read( buf, size );											\
	}																		\
	else																	\
		(void)0

#define checked_skip( size, a )												\
	if( !const_cond_false )													\
	{																		\
		size_t cb = (size);													\
																			\
		checked_align( a );													\
		stat = inPos + cb <= filesize;										\
		read_check();														\
																			\
		inBuf += cb;														\
		inPos += cb;														\
	}																		\
	else																	\
		(void)0

	checked_read( &pack, sizeof( pack ) );

	checked_read_a( ret->bones, sizeof( x42Bone_v5_t ) * h->numBones, 8 );
	checked_read_a( ret->animGroups, sizeof( x42AnimGroup_v5_t ) * h->numAnimGroups, 8 );

	for( i = 0; i < h->numAnimGroups; i++ )
	{
		uint j;
		for( j = ret->animGroups[i].beginBone; j < ret->animGroups[i].endBone; j++ )
			ret->boneGroups[j] = (u8)i;
	}

	checked_align( 2 );
	stat = read_packed_floats( &inBuf, filesize, &inPos, pack.animPosPack, 3,
		(float*)ret->posValues, h->numPosValues, sizeof( vec3_t ) );
	read_check();

	if( h->modelFlags & X42_MF_UNIFORM_SCALE )
	{
		stat = read_packed_floats( &inBuf, filesize, &inPos, &pack.animScalePack, 1,
			(float*)ret->scaleValues, h->numScaleValues, sizeof( vec3_t ) );
		read_check();

		for( i = 0; i < h->numScaleValues; i++ )
		{
			ret->scaleValues[i][1] = ret->scaleValues[i][0];
			ret->scaleValues[i][2] = ret->scaleValues[i][0];
		}
	}
	else
	{
		vec2_t bdxyz[3];

		bdxyz[0][0] = bdxyz[1][0] = bdxyz[2][0] = pack.animScalePack[0];
		bdxyz[0][1] = bdxyz[1][1] = bdxyz[2][1] = pack.animScalePack[1];

		stat = read_packed_floats( &inBuf, filesize, &inPos, bdxyz, 3,
			(float*)ret->scaleValues, h->numScaleValues, sizeof( vec3_t ) );
		read_check();
	}
	
	stat = read_packed_floats_s16n( &inBuf, filesize, &inPos, 4,
		(float*)ret->rotValues, h->numRotValues, sizeof( quat_t ) );
	read_check();
	
	checked_read_a( ret->keyStream, sizeof( x42KeyStreamEntry_v5_t ) * h->keyStreamLength, 8 );
	checked_read_a( ret->animations, sizeof( x42Animation_v5_t ) * h->numAnims, 8 );
	checked_read_a( ret->tags, sizeof( x42Tag_v5_t ) * h->numTags, 8 );
	checked_read_a( ret->influences, sizeof( x42influence_t ) * h->numInfluences, 8 );
	checked_read_a( ret->lods, sizeof( x42LodRange_v5_t ) * h->numLods, 8 );
	checked_read_a( ret->groups, sizeof( x42Group_v5_t ) * h->numGroups, 8 );

	checked_align( 2 );
	stat = read_packed_floats( &inBuf, filesize, &inPos, pack.vertPosPack, 3,
		(float*)&ret->vertPos[0].pos, h->numVerts, sizeof( x42vertAnim_t ) );
	read_check();

	for( i = 0; i < h->numGroups; i++ )
	{
		uint j;

		const x42group_t *g = ret->groups + i;
		size_t cbElem;

		if( !g->maxVertInfluences )
			continue;

		cbElem = sizeof( byte ) * ((g->maxVertInfluences - 1) * 2 + 1);

		for( j = 0; j < g->numVerts; j++ )
		{
			uint k;
			u8 in[7];
			float sum;
			
			x42vertAnim_t *v;

			checked_read( in, cbElem );

			v = ret->vertPos + g->firstVert + j;

			sum = 0;
			for( k = 0; k < g->maxVertInfluences - 1U; k++ )
			{
				v->idx[k] = in[k * 2 + 0];
				v->wt[k] = X42_FLOAT_U8N_UNPACK( in[k * 2 + 1] );

				sum += v->wt[k];
			}

			v->idx[k] = in[k * 2 + 0];
			v->wt[k] = 1.0F - sum;
		}
	}
	
	if( ret->vertNorm )
	{
		stat = read_packed_floats_s8n( &inBuf, filesize, &inPos, 3,
			(float*)&ret->vertNorm[0].norm, h->numVerts, sizeof( x42vertNormal_t ) );
		read_check();
	}
	else if( h->modelFlags & X42_MF_HAS_NORMALS )
		checked_skip( sizeof( s8[3] ) * h->numVerts, 0 );

	if( ret->vertTan )
	{
		stat = read_packed_floats_s8n( &inBuf, filesize, &inPos, 7,
			(float*)&ret->vertTan[0].tan, h->numVerts, sizeof( x42vertTangent_t ) );
		read_check();

		for( i = 0; i < h->numVerts; i++ )
			ret->vertTan[i].nfac1 = ret->vertTan[i].nfac0;
	}
	else if( h->modelFlags & X42_MF_HAS_TANGENT_BASIS )
		checked_skip( sizeof( s8[7] ) * h->numVerts, 0 );

	if( ret->vertTc )
	{
		checked_align( 2 );
		stat = read_packed_floats( &inBuf, filesize, &inPos, pack.vertTcPack, 2,
			(float*)ret->vertTc, h->numVerts, sizeof( vec2_t ) );
		read_check();
	}
	else if( h->modelFlags & X42_MF_HAS_TEXTURE_COORDINATES )
		checked_skip( sizeof( u16[2] ) * h->numVerts, 2 );

	if( ret->vertCl )
	{
		checked_read_a( ret->vertCl, sizeof( rgba_t ) * h->numVerts, 4 );
	}
	else if( h->modelFlags & X42_MF_HAS_COLORS )
		checked_skip( sizeof( rgba_t ) * h->numVerts, 4 );

	checked_read_a( ret->indices, sizeof( x42index_t ) * h->numIndices, 4 );

	checked_read( (char*)ret->strings, sizeof( u8 ) * h->nameBlobLen );

#undef checked_skip
#undef checked_read_a
#undef checked_read
#undef checked_align
#undef read_check

	mod->type = MOD_X42;
	mod->x42 = ret;

	return true;
}

static bool read_packed_floats_swp( const byte **filebuf, size_t filesize,
	size_t *inPos, const vec2_t *bounds, uint components, float *dest,
	uint elements, size_t elemStride )
{
	uint i;
	float scale[4], bias[4];
	size_t cbElem;

	if( !elements )
		return true;

	for( i = 0; i < components; i++ )
	{
		bias[i] = bounds[i][0];
		scale[i] = (bounds[i][1] - bounds[i][0]) / 65530.0F;
	}

	cbElem = sizeof( u16 ) * components;

	if( *inPos + cbElem * elements > filesize )
		return false;

	for( i = 0; i < elements; i++ )
	{
		uint c;
		const u16 *buf = (const u16*)*filebuf;
		*filebuf += cbElem;

		for( c = 0; c < components; c++ )
			dest[c] = Swap2u( buf[c] ) * scale[c] + bias[c];

		dest = (float*)((byte*)dest + elemStride);
	}

	*inPos += cbElem * elements;
	return true;
}

static bool read_packed_floats_s16n_swp( const byte **filebuf, size_t filesize,
	size_t *inPos, uint components, float *dest, uint elements, size_t elemStride )
{
	uint i;
	size_t cbElem;

	if( !elements )
		return true;

	cbElem = sizeof( s16 ) * components;

	if( *inPos + cbElem * elements > filesize )
		return false;

	for( i = 0; i < elements; i++ )
	{
		uint c;
		
		const s16 *buf = (const s16*)*filebuf;
		*filebuf += cbElem;

		for( c = 0; c < components; c++ )
			dest[c] = X42_FLOAT_S16N_UNPACK( Swap2s( buf[c] ) );

		dest = (float*)((byte*)dest + elemStride);
	}

	*inPos += cbElem * elements;
	return true;
}

#define read_packed_floats_s8n_swp read_packed_floats_s8n

#define swap_u8( u ) (void)sizeof( u )
#define swap_rgba_t( cl ) (void)sizeof( cl )

#define swap_x42Index_t( idx ) *(idx) = Swap2u( *(idx) )

static void swap_x42PackHeader_v5_t( x42PackHeader_v5_t *pack )
{
	pack->animPosPack[0][0] = SwapF( pack->animPosPack[0][0] );
	pack->animPosPack[0][1] = SwapF( pack->animPosPack[0][1] );
	pack->animPosPack[1][0] = SwapF( pack->animPosPack[1][0] );
	pack->animPosPack[1][1] = SwapF( pack->animPosPack[1][1] );
	pack->animPosPack[2][0] = SwapF( pack->animPosPack[2][0] );
	pack->animPosPack[2][1] = SwapF( pack->animPosPack[2][1] );

	pack->animScalePack[0] = SwapF( pack->animScalePack[0] );
	pack->animScalePack[1] = SwapF( pack->animScalePack[1] );

	pack->vertPosPack[0][0] = SwapF( pack->vertPosPack[0][0] );
	pack->vertPosPack[0][1] = SwapF( pack->vertPosPack[0][1] );
	pack->vertPosPack[1][0] = SwapF( pack->vertPosPack[1][0] );
	pack->vertPosPack[1][1] = SwapF( pack->vertPosPack[1][1] );
	pack->vertPosPack[2][0] = SwapF( pack->vertPosPack[2][0] );
	pack->vertPosPack[2][1] = SwapF( pack->vertPosPack[2][1] );

	pack->vertTcPack[0][0] = SwapF( pack->vertTcPack[0][0] );
	pack->vertTcPack[0][1] = SwapF( pack->vertTcPack[0][1] );
	pack->vertTcPack[1][0] = SwapF( pack->vertTcPack[1][0] );
	pack->vertTcPack[1][1] = SwapF( pack->vertTcPack[1][1] );
}

static void swap_x42KeyStreamEntry_v5_t( x42KeyStreamEntry_v5_t *ks )
{
	ks->type = Swap2u( ks->type );
	ks->bone = Swap2u( ks->bone );
	ks->frame = Swap2u( ks->frame );
	ks->value = Swap2u( ks->value );
}

static void swap_x42Animation_v5_t( x42Animation_v5_t *anim )
{
	anim->name = Swap2u( anim->name );
	anim->firstFrame = Swap2u( anim->firstFrame );
	anim->lastFrame = Swap2u( anim->lastFrame );
	anim->loopStart = Swap2u( anim->loopStart );
	anim->loopEnd = Swap2u( anim->loopEnd );
	anim->frameRate = Swap2u( anim->frameRate );
}

static void swap_x42AnimGroup_v5_t( x42AnimGroup_v5_t *group )
{
	group->name = Swap2u( group->name );
	group->beginBone = Swap2u( group->beginBone );
	group->endBone = Swap2u( group->endBone );
}

static void swap_x42Bone_v5_t( x42Bone_v5_t *bone )
{
	bone->name = Swap2u( bone->name );
	bone->flags = Swap2u( bone->name );
	bone->parentIdx = Swap2u( bone->parentIdx );
	bone->extent = SwapF( bone->extent );
}

static void swap_affine_t( affine_t *a )
{
	a->axis[0][0] = SwapF( a->axis[0][0] );
	a->axis[0][1] = SwapF( a->axis[0][1] );
	a->axis[0][2] = SwapF( a->axis[0][2] );

	a->axis[1][0] = SwapF( a->axis[1][0] );
	a->axis[1][1] = SwapF( a->axis[1][1] );
	a->axis[1][2] = SwapF( a->axis[1][2] );

	a->axis[2][0] = SwapF( a->axis[2][0] );
	a->axis[2][1] = SwapF( a->axis[2][1] );
	a->axis[2][2] = SwapF( a->axis[2][2] );

	a->origin[0] = SwapF( a->origin[0] );
	a->origin[1] = SwapF( a->origin[1] );
	a->origin[2] = SwapF( a->origin[2] );
}

static void swap_x42Influence_v5_t( x42Influence_v5_t *inf )
{
	inf->bone = Swap2u( inf->bone );
	swap_affine_t( &inf->meshToBone );
}

static void swap_x42Tag_v5_t( x42Tag_v5_t *tag )
{
	tag->name = Swap2u( tag->name );
	tag->bone = Swap2u( tag->bone );
	swap_affine_t( &tag->tagMatrix );
}

static void swap_x42Group_v5_t( x42Group_v5_t *g )
{
	uint i;

	g->material = Swap2u( g->material );
	g->surfaceName = Swap2u( g->surfaceName );

	g->maxVertInfluences = Swap2u( g->maxVertInfluences );
	g->numInfluences = Swap2u( g->numInfluences );

	for( i = 0; i < X42_MAX_INFLUENCES_PER_BATCH_V5; i++ )
		g->influences[i] = Swap2u( g->influences[i] );

	g->primType = Swap2u( g->primType );

	g->firstVert = Swap4u( g->firstVert );
	g->numVerts = Swap4u( g->numVerts );
	g->firstIndex = Swap4u( g->firstIndex );
	g->numElems = Swap4u( g->numElems );
}

static void swap_x42LodRange_v5_t( x42LodRange_v5_t *lod )
{
	lod->lodName = Swap2u( lod->lodName );
	lod->firstGroup = Swap2u( lod->firstGroup );
	lod->numGroups = Swap2u( lod->numGroups );
}

static bool R_LoadX42Swapped( model_t *mod, void *buffer, size_t header_size, int filesize, const char *name, const x42header_t *h )
{
	uint i;

	bool stat;
	x42PackHeader_v5_t pack;

	size_t inPos;
	const byte *inBuf;

	x42data_t *ret;

	const uint persist_flags = X42_PERSIST_EVERYTHING;

	ret = (x42data_t*)ri.Hunk_Alloc( sizeof( x42data_t ), h_low );
	x42_SetupBufferPointers( ret, h, persist_flags );

	inPos = sizeof( x42Header_ident_t );
	switch( h->ident.version )
	{
	case X42_VER_V5:
		inPos += sizeof( x42Header_v5_t );
		break;

	default:
		ri.Printf( PRINT_ERROR, "Invalid x42 file version in model '%s'\n", name );
		return false;
	}

	inBuf = (const byte*)buffer + inPos;

#define read_check() if( !stat ) return false; else (void)0

#define checked_read( buf, type, count )									\
	if( !const_cond_false )													\
	{																		\
		uint _i;															\
		size_t cb = sizeof( type ) * (count);								\
																			\
		stat = inPos + cb <= filesize;										\
		read_check();														\
																			\
		Com_Memcpy( buf, inBuf, cb );										\
		inBuf += cb;														\
		inPos += cb;														\
																			\
		for( _i = 0; _i < (count); _i++ )									\
			swap_##type( buf + _i );										\
	}																		\
	else																	\
		(void)0

#define checked_align( a )													\
	if( !const_cond_false )													\
	{																		\
		size_t _a = (a);													\
																			\
		if( _a )															\
		{																	\
			size_t ofs = ((_a - (inPos % _a)) % _a);						\
																			\
			stat = inPos + ofs <= filesize;									\
			read_check();													\
																			\
			inBuf += ofs;													\
			inPos += ofs;													\
		}																	\
	}																		\
	else																	\
		(void)0

#define checked_read_a( buf, type, count, a )								\
	if( !const_cond_false )													\
	{																		\
		checked_align( a );													\
		checked_read( buf, type, count );									\
	}																		\
	else																	\
		(void)0

#define checked_skip( size, a )												\
	if( !const_cond_false )													\
	{																		\
		size_t cb = (size);													\
																			\
		checked_align( a );													\
		stat = inPos + cb <= filesize;										\
		read_check();														\
																			\
		inBuf += cb;														\
		inPos += cb;														\
	}																		\
	else																	\
		(void)0

	checked_read( &pack, x42PackHeader_v5_t, 1 );

	checked_read_a( ret->bones, x42Bone_v5_t, h->numBones, 8 );
	checked_read_a( ret->animGroups, x42AnimGroup_v5_t, h->numAnimGroups, 8 );

	for( i = 0; i < h->numAnimGroups; i++ )
	{
		uint j;
		for( j = ret->animGroups[i].beginBone; j < ret->animGroups[i].endBone; j++ )
			ret->boneGroups[j] = (u8)i;
	}

	checked_align( 2 );
	stat = read_packed_floats_swp( &inBuf, filesize, &inPos, pack.animPosPack, 3,
		(float*)ret->posValues, h->numPosValues, sizeof( vec3_t ) );
	read_check();

	if( h->modelFlags & X42_MF_UNIFORM_SCALE )
	{
		stat = read_packed_floats_swp( &inBuf, filesize, &inPos, &pack.animScalePack, 1,
			(float*)ret->scaleValues, h->numScaleValues, sizeof( vec3_t ) );
		read_check();

		for( i = 0; i < h->numScaleValues; i++ )
		{
			ret->scaleValues[i][1] = ret->scaleValues[i][0];
			ret->scaleValues[i][2] = ret->scaleValues[i][0];
		}
	}
	else
	{
		vec2_t bdxyz[3];

		bdxyz[0][0] = bdxyz[1][0] = bdxyz[2][0] = pack.animScalePack[0];
		bdxyz[0][1] = bdxyz[1][1] = bdxyz[2][1] = pack.animScalePack[1];

		stat = read_packed_floats_swp( &inBuf, filesize, &inPos, bdxyz, 3,
			(float*)ret->scaleValues, h->numScaleValues, sizeof( vec3_t ) );
		read_check();
	}
	
	stat = read_packed_floats_s16n_swp( &inBuf, filesize, &inPos, 4,
		(float*)ret->rotValues, h->numRotValues, sizeof( quat_t ) );
	read_check();
	
	checked_read_a( ret->keyStream, x42KeyStreamEntry_v5_t, h->keyStreamLength, 8 );
	checked_read_a( ret->animations, x42Animation_v5_t, h->numAnims, 8 );
	checked_read_a( ret->tags, x42Tag_v5_t, h->numTags, 8 );
	checked_read_a( ret->influences, x42Influence_v5_t, h->numInfluences, 8 );
	checked_read_a( ret->lods, x42LodRange_v5_t, h->numLods, 8 );
	checked_read_a( ret->groups, x42Group_v5_t, h->numGroups, 8 );

	checked_align( 2 );
	stat = read_packed_floats_swp( &inBuf, filesize, &inPos, pack.vertPosPack, 3,
		(float*)&ret->vertPos[0].pos, h->numVerts, sizeof( x42vertAnim_t ) );
	read_check();

	for( i = 0; i < h->numGroups; i++ )
	{
		uint j;

		const x42group_t *g = ret->groups + i;
		size_t cbElem;

		if( !g->maxVertInfluences )
			continue;

		cbElem = sizeof( byte ) * ((g->maxVertInfluences - 1) * 2 + 1);

		for( j = 0; j < g->numVerts; j++ )
		{
			uint k;
			u8 in[7];
			float sum;
			
			x42vertAnim_t *v;

			checked_read( in, u8, cbElem );

			v = ret->vertPos + g->firstVert + j;

			sum = 0;
			for( k = 0; k < g->maxVertInfluences - 1U; k++ )
			{
				v->idx[k] = in[k * 2 + 0];
				v->wt[k] = X42_FLOAT_U8N_UNPACK( in[k * 2 + 1] );

				sum += v->wt[k];
			}

			v->idx[k] = in[k * 2 + 0];
			v->wt[k] = 1.0F - sum;
		}
	}
	
	if( ret->vertNorm )
	{
		stat = read_packed_floats_s8n_swp( &inBuf, filesize, &inPos, 3,
			(float*)&ret->vertNorm[0].norm, h->numVerts, sizeof( x42vertNormal_t ) );
		read_check();
	}
	else if( h->modelFlags & X42_MF_HAS_NORMALS )
		checked_skip( sizeof( s8[3] ) * h->numVerts, 0 );

	if( ret->vertTan )
	{
		stat = read_packed_floats_s8n_swp( &inBuf, filesize, &inPos, 7,
			(float*)&ret->vertTan[0].tan, h->numVerts, sizeof( x42vertTangent_t ) );
		read_check();

		for( i = 0; i < h->numVerts; i++ )
			ret->vertTan[i].nfac1 = ret->vertTan[i].nfac0;
	}
	else if( h->modelFlags & X42_MF_HAS_TANGENT_BASIS )
		checked_skip( sizeof( s8[7] ) * h->numVerts, 0 );

	if( ret->vertTc )
	{
		checked_align( 2 );
		stat = read_packed_floats_swp( &inBuf, filesize, &inPos, pack.vertTcPack, 2,
			(float*)ret->vertTc, h->numVerts, sizeof( vec2_t ) );
		read_check();
	}
	else if( h->modelFlags & X42_MF_HAS_TEXTURE_COORDINATES )
		checked_skip( sizeof( u16[2] ) * h->numVerts, 2 );

	if( ret->vertCl )
	{
		checked_read_a( ret->vertCl, rgba_t, h->numVerts, 4 );
	}
	else if( h->modelFlags & X42_MF_HAS_COLORS )
		checked_skip( sizeof( rgba_t ) * h->numVerts, 4 );

	checked_read_a( ret->indices, x42Index_t, h->numIndices, 4 );

	checked_read( (char*)ret->strings, u8, h->nameBlobLen );

#undef checked_skip
#undef checked_read_a
#undef checked_read
#undef checked_align
#undef read_check

	mod->type = MOD_X42;
	mod->x42 = ret;

	return true;
}

static bool R_LoadX42Header( x42header_t *h, size_t *header_size, const void *buffer, bool swap )
{
	const x42Header_ident_t *inIdent = (x42Header_ident_t*)buffer;

	Com_Memset( h, 0, sizeof( x42header_t ) );

	h->ident = *inIdent;
	if( swap )
	{
		h->ident.ident = Swap4u( h->ident.ident );
		h->ident.version = Swap4u( h->ident.version );
	}

	switch( h->ident.version )
	{
	case X42_VER_V5:
		{
			const x42Header_v5_t *inH = (x42Header_v5_t*)(inIdent + 1);
			*header_size = sizeof( x42Header_ident_t ) + sizeof( x42Header_v5_t ); 

			h->modelFlags = inH->modelFlags;
			
			h->baseFrame = inH->baseFrame;
			h->numFrames = inH->numFrames;

			h->nameBlobLen = inH->nameBlobLen;

			h->numBones = inH->numBones;
			h->numAnimGroups = inH->numAnimGroups;

			h->numPosValues = inH->numPosValues;
			h->numScaleValues = inH->numScaleValues;
			h->numRotValues = inH->numRotValues;

			h->keyStreamLength = inH->keyStreamLength;
			h->numAnims = inH->numAnims;

			h->numTags = inH->numTags;
			h->numInfluences = inH->numInfluences;

			h->numLods = inH->numLods;
			h->numGroups = inH->numGroups;

			h->numVerts = inH->numVerts;
			h->numIndices = inH->numIndices;

			h->boundingBox = inH->boundingBox;
			h->boundingSphere = inH->boundingSphere;
		}
		break;

	default:
		return false;
	}

	if( swap )
	{	
		h->modelFlags = Swap2u( h->modelFlags );

		h->baseFrame = Swap2s( h->baseFrame );
		h->numFrames = Swap2u( h->numFrames );

		h->nameBlobLen = Swap2u( h->nameBlobLen );

		h->numBones = Swap2u( h->numBones );
		h->numAnimGroups = Swap2u( h->numAnimGroups );

		h->numPosValues = Swap2u( h->numPosValues );
		h->numScaleValues = Swap2u( h->numScaleValues );
		h->numRotValues = Swap2u( h->numRotValues );

		h->keyStreamLength = Swap2u( h->keyStreamLength );
		h->numAnims = Swap2u( h->numAnims );

		h->numTags = Swap2u( h->numTags );
		h->numInfluences = Swap2u( h->numInfluences );

		h->numLods = Swap2u( h->numLods );
		h->numGroups = Swap2u( h->numGroups );

		h->numVerts = Swap4u( h->numVerts );
		h->numIndices = Swap4u( h->numIndices );

		h->boundingBox.mins[0] = SwapF( h->boundingBox.mins[0] );
		h->boundingBox.mins[1] = SwapF( h->boundingBox.mins[1] );
		h->boundingBox.mins[2] = SwapF( h->boundingBox.mins[2] );

		h->boundingBox.maxs[0] = SwapF( h->boundingBox.maxs[0] );
		h->boundingBox.maxs[1] = SwapF( h->boundingBox.maxs[1] );
		h->boundingBox.maxs[2] = SwapF( h->boundingBox.maxs[2] );

		h->boundingSphere.center[0] = SwapF( h->boundingSphere.center[0] );
		h->boundingSphere.center[1] = SwapF( h->boundingSphere.center[1] );
		h->boundingSphere.center[2] = SwapF( h->boundingSphere.center[2] );

		h->boundingSphere.radius = SwapF( h->boundingSphere.radius );
	}

	return true;
}

static void R_x42TransformStaticGroupsIntoQ3Space( x42data_t *x42 )
{
	uint i;

	for( i = 0; i < x42->header.numGroups; i++ )
	{
		uint j;
		uint lastVert;

		const x42group_t *g = x42->groups + i;

		if( g->maxVertInfluences )
			//not a static group
			continue;

		lastVert = g->firstVert + g->numVerts;

		for( j = g->firstVert; j < lastVert; j++ )
		{
			Affine_MulPos( x42->vertPos[j].pos, &x42_m2q3, x42->vertPos[j].pos );

			if( x42->vertNorm )
			{
				Affine_MulPos( x42->vertNorm[j].norm, &x42_m2q3, x42->vertNorm[j].norm );
			}

			if( x42->vertTan )
			{
				Affine_MulPos( x42->vertTan[j].tan, &x42_m2q3, x42->vertTan[j].tan );
				Affine_MulPos( x42->vertTan[j].bit, &x42_m2q3, x42->vertTan[j].bit );
			}
		}
	}
}

static void R_x42LoadDefaultShaders( x42data_t *x42 )
{
	uint i;

	x42->groupMats = (int*)ri.Hunk_Alloc( sizeof( int ) * x42->header.numGroups, h_low );
	for( i = 0; i < x42->header.numGroups; i++ )
	{
		shader_t *shader = R_FindShader( x42->strings + x42->groups[i].material, LIGHTMAP_NONE, qtrue );
		x42->groupMats[i] = shader->defaultShader ? 0 : shader->index;

		Q_strlwr( (char*)x42->strings + x42->groups[i].surfaceName );
	}
}

bool R_LoadX42( model_t *mod, void *buffer, int filesize, const char *name )
{
	x42header_t h;
	size_t header_size;

	bool swapped;
	
	const x42Header_ident_t *inIdent = (x42Header_ident_t*)buffer;

	if( inIdent->ident != X42_IDENT && Swap4u( inIdent->ident ) != X42_IDENT )
	{
		ri.Printf( PRINT_WARNING, "R_LoadX42: %s has an invalid ident\n", name );
		return false;
	}

	swapped = inIdent->ident != X42_IDENT;

	if( !R_LoadX42Header( &h, &header_size, buffer, swapped ) )
		return false;

	if( !swapped )
	{
		if( !R_LoadX42Direct( mod, buffer, header_size, filesize, name, &h ) )
			return false;
	}
	else
	{
		if( !R_LoadX42Swapped( mod, buffer, header_size, filesize, name, &h ) )
			return false;
	}

	mod->numLods = h.numLods;

	R_x42LoadDefaultShaders( mod->x42 );
	R_x42TransformStaticGroupsIntoQ3Space( mod->x42 );
	R_x42PoseInitKsStarts( mod->x42 );

	return true;
}