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

#include "tr_local.h"

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

static	world_t		s_worldData;
static	byte		*fileBase;

static ID_INLINE void LittleVec2( float out[2], const float in[2] )
{
	out[0] = LittleFloat( in[0] );
	out[1] = LittleFloat( in[1] );
}

static ID_INLINE void LittleVec3( float out[3], const float in[3] )
{
	out[0] = LittleFloat( in[0] );
	out[1] = LittleFloat( in[1] );
	out[2] = LittleFloat( in[2] );
}

//===============================================================================

static void HSVtoRGB( float h, float s, float v, float rgb[3] )
{
	int i;
	float f;
	float p, q, t;

	h *= 5;

	i = floor( h );
	f = h - i;

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
static void R_ColorShiftLightingBytes( const byte in[4], byte out[4] )
{
	int shift, r, g, b;

	// shift the color data based on overbright range
	shift = r_mapOverBrightBits->integer - tr.overbrightBits;

	// shift the data based on overbright range
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;
	
	// normalize by color instead of saturating to white
	if( (r | g | b) > 255 )
	{
		int		max;

		max = r > g ? r : g;
		max = max > b ? max : b;

		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}

/*
===============
R_LoadLightmaps

===============
*/
#define	LIGHTMAP_SIZE	128

static void R_BspLoadLightMap( const byte *in, int index )
{
	int i;
	byte image[LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4];

	if( r_lightmap->integer == 2 )
	{	// color code by intensity as development tool	(FIXME: check range)
		for( i = 0; i < LIGHTMAP_SIZE * LIGHTMAP_SIZE; i++ )
		{
			float r = in[i * 3 + 0];
			float g = in[i * 3 + 1];
			float b = in[i * 3 + 2];

			float intensity;
			
			float out[3];

			intensity = 0.33F * r + 0.685F * g + 0.063F * b;

			if( intensity > 0xFF )
				intensity = 1.0F;
			else
				intensity /= (float)0xFF;

			HSVtoRGB( intensity, 1.00, 0.50, out );

			image[i * 4 + 0] = out[0] * 0xFF;
			image[i * 4 + 1] = out[1] * 0xFF;
			image[i * 4 + 2] = out[2] * 0xFF;
			image[i * 4 + 3] = 0xFF;
		}
	}
	else
	{
		for( i = 0; i < LIGHTMAP_SIZE * LIGHTMAP_SIZE; i++ )
		{
			R_ColorShiftLightingBytes( in + i * 3, image + i * 4 );
			image[i * 4 + 3] = 0xFF;
		}
	}

	tr.lightmaps[index] = R_CreateImage( va( "*lightmap%d", index ), image, 
		LIGHTMAP_SIZE, LIGHTMAP_SIZE, qtrue, ILF_NONE );
}

static void R_BspLoadDeluxeMap( const byte *in, int index )
{
	int i;
	byte image[LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4];

	if( !R_ShadeSupportsDeluxeMapping() )
		return;

	for( i = 0; i < LIGHTMAP_SIZE * LIGHTMAP_SIZE; i++ )
	{
		image[i * 4 + 0] = in[i * 3 + 0];
		image[i * 4 + 1] = in[i * 3 + 1];
		image[i * 4 + 2] = in[i * 3 + 2];
		image[i * 4 + 3] = 0xFF;
	}

	tr.deluxemaps[index] = R_CreateImage( va( "*deluxemap%d", index ), image, 
		LIGHTMAP_SIZE, LIGHTMAP_SIZE, qtrue, ILF_VECTOR_SB3 );
}

static void R_LoadLightmaps( lump_t *l )
{
	int i;
	byte *buf;
	
	// we are about to upload textures
	R_SyncRenderThread();

	if( !l->filelen )
	{
		//try to load external lightmaps
		char basePath[MAX_QPATH];
		COM_StripExtension( s_worldData.name, basePath, sizeof( basePath ) );

		for( ; ; )
		{
			char lp[MAX_QPATH];

			if( tr.numLightmaps == MAX_LIGHTMAPS )
			{
				ri.Printf( PRINT_WARNING, "WARNING: number of lightmaps > MAX_LIGHTMAPS\n" );
				break;
			}

			Com_sprintf( lp, sizeof( lp ), "%s/lm_%04i.tga", basePath, tr.numLightmaps );

			tr.lightmaps[tr.numLightmaps] = R_FindImageFile( lp, qtrue, ILF_NONE );

			if( !tr.lightmaps[tr.numLightmaps] )
				//couldn't load, assume that's all of them
				break;

			if( s_worldData.deluxeMapping )
			{
				Com_sprintf( lp, sizeof( lp ), "%s/dm_%04i.tga", basePath, tr.numLightmaps );
				tr.deluxemaps[tr.numLightmaps] = R_FindImageFile( lp, qtrue, ILF_VECTOR_SB3 );
			}

			tr.numLightmaps++;
		}

		return;
	}

	buf = fileBase + l->fileofs;

	// create all the lightmaps
	tr.numLightmaps = l->filelen / (LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3);

	// if we are in r_vertexLight mode, we don't need the lightmaps at all
	if( r_vertexLight->integer )
		return;

	if( s_worldData.deluxeMapping )
	{
		tr.numLightmaps /= 2;
		for( i = 0; i < tr.numLightmaps; i++ )
		{
			R_BspLoadLightMap( buf + (i * 2 + 0) * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3, i );

			if( s_worldData.exSurfs )
				//skip this if we don't have a tangent basis
				R_BspLoadDeluxeMap( buf + (i * 2 + 1) * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3, i );
		}
	}
	else
	{
		for( i = 0; i < tr.numLightmaps; i++ )
			//expand the 24 bit on-disk to 32 bit
			R_BspLoadLightMap( buf + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3, i );
	}
}

static void R_LoadLightmapsFast( void )
{
	lump_t dummy = { 0, 0 };
	R_LoadLightmaps( &dummy );
}


/*
=================
RE_SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void Q_EXTERNAL_CALL RE_SetWorldVisData( const byte *vis ) {
	tr.externalVisData = vis;
}


/*
=================
R_LoadVisibility
=================
*/
static void R_LoadVisibility( lump_t *l )
{
	int		len;
	byte	*buf;

	len = ( s_worldData.numClusters + 63 ) & ~63;
	s_worldData.novis = ri.Hunk_Alloc( len, h_low );
	Com_Memset( s_worldData.novis, 0xff, len );

    len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	s_worldData.numClusters = LittleLong( ((int *)buf)[0] );
	s_worldData.clusterBytes = LittleLong( ((int *)buf)[1] );

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if( tr.externalVisData )
	{
		s_worldData.vis = tr.externalVisData;
	}
	else
	{
		byte	*dest;

		dest = ri.Hunk_Alloc( len - 8, h_low );
		Com_Memcpy( dest, buf + 8, len - 8 );
		s_worldData.vis = dest;
	}
}

static void R_LoadVisibilityFast( const lump_t *l )
{
	int len;
	byte *buf;

	len = ( s_worldData.numClusters + 63 ) & ~63;
	s_worldData.novis = ri.Hunk_Alloc( len, h_low );
	Com_Memset( s_worldData.novis, 0xff, len );

    len = l->filelen;
	if ( !len ) {
		return;
	}
	buf = fileBase + l->fileofs;

	s_worldData.numClusters = LittleLong( ((int *)buf)[0] );
	s_worldData.clusterBytes = LittleLong( ((int *)buf)[1] );

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if( tr.externalVisData )
	{
		s_worldData.vis = tr.externalVisData;
	}
	else
	{
		s_worldData.vis = buf + 8;
	}
}

//===============================================================================


/*
===============
ShaderForShaderNum
===============
*/
static shader_t *ShaderForShaderNum( int shaderNum, int lightmapNum )
{
	shader_t	*shader;
	dshader_t	*dsh;

	if( s_worldData.deluxeMapping )
		lightmapNum /= 2;

	shaderNum = LittleLong( shaderNum );
	if( shaderNum < 0 || shaderNum >= s_worldData.numShaders )
		ri.Error( ERR_DROP, "ShaderForShaderNum: bad num %i", shaderNum );

	dsh = &s_worldData.shaders[shaderNum];

	if( r_vertexLight->integer )
		lightmapNum = LIGHTMAP_BY_VERTEX;

	if( r_fullbright->integer )
		lightmapNum = LIGHTMAP_WHITEIMAGE;

	shader = R_FindShader( dsh->shader, lightmapNum, qtrue );

	// if the shader had errors, just use default shader
	if( shader->defaultShader )
		return tr.defaultShader;

	return shader;
}

/*
===============
ParseFace
===============
*/
static void ParseFace( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indices )
{
	int i, j;
	srfSurfaceFace_t *cv;
	int numPoints, numIndexes;
	int lightmapNum;
	
	lightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum );
	if( r_singleShader->integer && !surf->shader->isSky )
		surf->shader = tr.defaultShader;

	numPoints = LittleLong( ds->numVerts );
	if( numPoints > MAX_FACE_POINTS )
	{
		ri.Printf( PRINT_WARNING, "WARNING: MAX_FACE_POINTS exceeded: %i\n", numPoints );
		numPoints = MAX_FACE_POINTS;
		surf->shader = tr.defaultShader;
	}

	numIndexes = LittleLong( ds->numIndexes );

	if( surf->shader->flareParams )
	{
		//parse as a flare
		vec3_t ptC;

		srfFlare_t *cf = (srfFlare_t*)ri.Hunk_Alloc( sizeof( srfFlare_t ), h_low );

		cf->surfaceType = SF_FLARE;
		
		ptC[0] = 0;
		ptC[1] = 0;
		ptC[2] = 0;

		verts += LittleLong( ds->firstVert );
		for( i = 0; i < numPoints; i++ )
		{
			for( j = 0 ; j < 3 ; j++ )
				ptC[j] += LittleFloat( verts[i].xyz[j] );
		}

		ptC[0] /= (float)numPoints;
		ptC[1] /= (float)numPoints;
		ptC[2] /= (float)numPoints;

		VectorCopy( ptC, cf->origin );

		// take the plane information from the lightmap vector
		LittleVec3( cf->normal, ds->lightmapVecs[2] );

		cf->color[0] = 1;
		cf->color[1] = 1;
		cf->color[2] = 1;

		surf->data = (surfaceType_t*)cf;

		return;
	}

	// create the srfSurfaceFace_t	
	cv = ri.Hunk_Alloc( sizeof( srfSurfaceFace_t ), h_low );

	cv->surfaceType = SF_FACE;
	cv->numVerts = numPoints;
	cv->numIndices = numIndexes;

	cv->verts = verts + LittleLong( ds->firstVert );
	cv->indices = indices + LittleLong( ds->firstIndex );

	// take the plane information from the lightmap vector
	for( i = 0; i < 3; i++ )
		cv->plane.normal[i] = LittleFloat( ds->lightmapVecs[2][i] );

	cv->plane.dist = DotProduct( cv->verts[0].xyz, cv->plane.normal );
	SetPlaneSignbits( &cv->plane );
	cv->plane.type = PlaneTypeForNormal( cv->plane.normal );

	surf->data = (surfaceType_t*)cv;
}


/*
===============
ParseMesh
===============
*/
static void ParseMesh( dsurface_t *ds, drawVert_t *verts, msurface_t *surf )
{
	srfGridMesh_t			*grid;
	int						width, height, numPoints;
	drawVert_t				points[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
	int						lightmapNum;
	vec3_t					bounds[2];
	vec3_t					tmpVec;

	static surfaceType_t	skipData = SF_SKIP;

	lightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum );
	
	if( r_singleShader->integer && !surf->shader->isSky )
	{
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if( s_worldData.shaders[LittleLong( ds->shaderNum )].surfaceFlags & SURF_NODRAW )
	{
		surf->data = &skipData;
		return;
	}

	width = LittleLong( ds->patchWidth );
	height = LittleLong( ds->patchHeight );

	verts += LittleLong( ds->firstVert );

	numPoints = width * height;
	Com_Memcpy( points, verts, sizeof( drawVert_t ) * numPoints );

	// pre-tesseleate
	grid = R_SubdividePatchToGrid( width, height, points );
	surf->data = (surfaceType_t*)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	LittleVec3( bounds[0], ds->lightmapVecs[0] );
	LittleVec3( bounds[1], ds->lightmapVecs[1] );

	VectorAdd( bounds[0], bounds[1], bounds[1] );
	VectorScale( bounds[1], 0.5F, grid->lodOrigin );
	VectorSubtract( bounds[0], grid->lodOrigin, tmpVec );
	grid->lodRadius = VectorLength( tmpVec );
}

/*
===============
ParseTriSurf
===============
*/
static void ParseTriSurf( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indices )
{
	srfTriangles_t	*tri;
	int				i;
	int				numVerts, numIndexes;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong( ds->numVerts );
	numIndexes = LittleLong( ds->numIndexes );

	tri = ri.Hunk_Alloc( sizeof( *tri ), h_low );
	tri->surfaceType = SF_TRIANGLES;
	tri->numVerts = numVerts;
	tri->numIndices = numIndexes;
	tri->verts = verts + LittleLong( ds->firstVert );
	tri->indexes = indices + LittleLong( ds->firstIndex );

	surf->data = (surfaceType_t*)tri;

	ClearBounds( tri->bounds[0], tri->bounds[1] );
	for( i = 0; i < numVerts; i++ )
		AddPointToBounds( tri->verts[i].xyz, tri->bounds[0], tri->bounds[1] );
}

/*
===============
ParseFlare
===============
*/
static void ParseFlare( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes ) {
	srfFlare_t		*flare;
	int				i;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	flare = ri.Hunk_Alloc( sizeof( *flare ), h_low );
	flare->surfaceType = SF_FLARE;

	surf->data = (surfaceType_t *)flare;

	for ( i = 0 ; i < 3 ; i++ ) {
		flare->origin[i] = LittleFloat( ds->lightmapOrigin[i] );
		flare->color[i] = LittleFloat( ds->lightmapVecs[0][i] );
		flare->normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
}


/*
=================
R_MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
int R_MergedWidthPoints(srfGridMesh_t *grid, int offset) {
	int i, j;

	for (i = 1; i < grid->width-1; i++) {
		for (j = i + 1; j < grid->width-1; j++) {
			if ( fabs(grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0]) > .1) continue;
			if ( fabs(grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1]) > .1) continue;
			if ( fabs(grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2]) > .1) continue;
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
R_MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
int R_MergedHeightPoints(srfGridMesh_t *grid, int offset) {
	int i, j;

	for (i = 1; i < grid->height-1; i++) {
		for (j = i + 1; j < grid->height-1; j++) {
			if ( fabs(grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0]) > .1) continue;
			if ( fabs(grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1]) > .1) continue;
			if ( fabs(grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2]) > .1) continue;
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
R_FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void R_FixSharedVertexLodError_r( int start, srfGridMesh_t *grid1 ) {
	int j, k, l, m, n, offset1, offset2, touch;
	srfGridMesh_t *grid2;

	for ( j = start; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) continue;
		// if the LOD errors are already fixed for this patch
		if ( grid2->lodFixed == 2 ) continue;
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) continue;
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) continue;
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) continue;
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) continue;
		//
		touch = qfalse;
		for (n = 0; n < 2; n++) {
			//
			if (n) offset1 = (grid1->height-1) * grid1->width;
			else offset1 = 0;
			if (R_MergedWidthPoints(grid1, offset1)) continue;
			for (k = 1; k < grid1->width-1; k++) {
				for (m = 0; m < 2; m++) {

					if (m) offset2 = (grid2->height-1) * grid2->width;
					else offset2 = 0;
					if (R_MergedWidthPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->width-1; l++) {
					//
						if ( fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
				for (m = 0; m < 2; m++) {

					if (m) offset2 = grid2->width-1;
					else offset2 = 0;
					if (R_MergedHeightPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->height-1; l++) {
					//
						if ( fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->widthLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		for (n = 0; n < 2; n++) {
			//
			if (n) offset1 = grid1->width-1;
			else offset1 = 0;
			if (R_MergedHeightPoints(grid1, offset1)) continue;
			for (k = 1; k < grid1->height-1; k++) {
				for (m = 0; m < 2; m++) {

					if (m) offset2 = (grid2->height-1) * grid2->width;
					else offset2 = 0;
					if (R_MergedWidthPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->width-1; l++) {
					//
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
				for (m = 0; m < 2; m++) {

					if (m) offset2 = grid2->width-1;
					else offset2 = 0;
					if (R_MergedHeightPoints(grid2, offset2)) continue;
					for ( l = 1; l < grid2->height-1; l++) {
					//
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1) continue;
						if ( fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1) continue;
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->heightLodError[k];
						touch = qtrue;
					}
				}
			}
		}
		if (touch) {
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r ( start, grid2 );
			//NOTE: this would be correct but makes things really slow
			//grid2->lodFixed = 1;
		}
	}
}

/*
=================
R_FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void R_FixSharedVertexLodError( void ) {
	int i;
	srfGridMesh_t *grid1;

	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		//
		grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid1->surfaceType != SF_GRID )
			continue;
		//
		if ( grid1->lodFixed )
			continue;
		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r( i + 1, grid1);
	}
}


/*
===============
R_StitchPatches
===============
*/
int R_StitchPatches( int grid1num, int grid2num ) {
	float *v1, *v2;
	srfGridMesh_t *grid1, *grid2;
	int k, l, m, n, offset1, offset2, row, column;

	grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	grid2 = (srfGridMesh_t *) s_worldData.surfaces[grid2num].data;
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = (grid1->height-1) * grid1->width;
		else offset1 = 0;
		if (R_MergedWidthPoints(grid1, offset1))
			continue;
		for (k = 0; k < grid1->width-2; k += 2) {

			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
									grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = grid1->width-1;
		else offset1 = 0;
		if (R_MergedHeightPoints(grid1, offset1))
			continue;
		for (k = 0; k < grid1->height-2; k += 2) {
			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
									grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
									grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = (grid1->height-1) * grid1->width;
		else offset1 = 0;
		if (R_MergedWidthPoints(grid1, offset1))
			continue;
		for (k = grid1->width-1; k > 1; k -= 2) {

			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
										grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k+1]);
					if (!grid2)
						break;
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	for (n = 0; n < 2; n++) {
		//
		if (n) offset1 = grid1->width-1;
		else offset1 = 0;
		if (R_MergedHeightPoints(grid1, offset1))
			continue;
		for (k = grid1->height-1; k > 1; k -= 2) {
			for (m = 0; m < 2; m++) {

				if ( grid2->width >= MAX_GRID_SIZE )
					break;
				if (m) offset2 = (grid2->height-1) * grid2->width;
				else offset2 = 0;
				for ( l = 0; l < grid2->width-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[(l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if (m) row = grid2->height-1;
					else row = 0;
					grid2 = R_GridInsertColumn( grid2, l+1, row,
										grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
			for (m = 0; m < 2; m++) {

				if (grid2->height >= MAX_GRID_SIZE)
					break;
				if (m) offset2 = grid2->width-1;
				else offset2 = 0;
				for ( l = 0; l < grid2->height-1; l++) {
				//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;

					v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) > .1)
						continue;
					if ( fabs(v1[1] - v2[1]) > .1)
						continue;
					if ( fabs(v1[2] - v2[2]) > .1)
						continue;
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
					if ( fabs(v1[0] - v2[0]) < .01 &&
							fabs(v1[1] - v2[1]) < .01 &&
							fabs(v1[2] - v2[2]) < .01)
						continue;
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if (m) column = grid2->width-1;
					else column = 0;
					grid2 = R_GridInsertRow( grid2, l+1, column,
										grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k+1]);
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[grid2num].data = (void *) grid2;
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/*
===============
R_TryStitchPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
int R_TryStitchingPatch( int grid1num ) {
	int j, numstitches;
	srfGridMesh_t *grid1, *grid2;

	numstitches = 0;
	grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	for ( j = 0; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) continue;
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) continue;
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) continue;
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) continue;
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) continue;
		//
		while (R_StitchPatches(grid1num, j))
		{
			numstitches++;
		}
	}
	return numstitches;
}

/*
===============
R_StitchAllPatches
===============
*/
void R_StitchAllPatches( void ) {
	int i, stitched, numstitches;
	srfGridMesh_t *grid1;

	numstitches = 0;
	do
	{
		stitched = qfalse;
		for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
			//
			grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
			// if this surface is not a grid
			if ( grid1->surfaceType != SF_GRID )
				continue;
			//
			if ( grid1->lodStitched )
				continue;
			//
			grid1->lodStitched = qtrue;
			stitched = qtrue;
			//
			numstitches += R_TryStitchingPatch( i );
		}
	}
	while (stitched);
	ri.Printf( PRINT_ALL, "stitched %d LoD cracks\n", numstitches );
}

/*
===============
R_MovePatchSurfacesToHunk
===============
*/
void R_MovePatchSurfacesToHunk( void )
{
	int i, size;
	srfGridMesh_t *grid, *hunkgrid;

	for( i = 0; i < s_worldData.numsurfaces; i++ )
	{
		//
		grid = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid->surfaceType != SF_GRID )
			continue;
		//
		size = (grid->width * grid->height - 1) * sizeof( drawVert_t ) + sizeof( *grid );
		hunkgrid = ri.Hunk_Alloc( size, h_low );
		Com_Memcpy( hunkgrid, grid, size );

		hunkgrid->widthLodError = ri.Hunk_Alloc( grid->width * 4, h_low );
		Com_Memcpy( hunkgrid->widthLodError, grid->widthLodError, grid->width * 4 );

		hunkgrid->heightLodError = ri.Hunk_Alloc( grid->height * 4, h_low );
		Com_Memcpy( grid->heightLodError, grid->heightLodError, grid->height * 4 );

		R_FreeSurfaceGridMesh( grid );

		s_worldData.surfaces[i].data = (void*)hunkgrid;
	}
}

/*
===============
R_LoadSurfaces
===============
*/
static void R_LoadSurfaces( const lump_t *lSurfs, const lump_t *lVerts, const lump_t *lIndices, bool fast )
{
	dsurface_t *in;
	msurface_t *out;

	int numVerts;
	int numIndices;
	drawVert_t *verts;
	int *indices;

	int count;
	int numFaces, numMeshes, numTriSurfs, numFlares;

	int i;

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;

	if( lSurfs->filelen % sizeof( *in ) )
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );

	in = (dsurface_t*)(fileBase + lSurfs->fileofs);
	count = lSurfs->filelen / sizeof( dsurface_t );

	if( lVerts->filelen % sizeof( drawVert_t ) )
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );

	verts = (drawVert_t*)(fileBase + lVerts->fileofs);
	numVerts = lVerts->filelen / sizeof( drawVert_t );

	if( lIndices->filelen % sizeof( int ) )
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );

	indices = (int*)(fileBase + lIndices->fileofs);
	numIndices = lIndices->filelen / sizeof( int );

	if( !fast )
	{
		int *oi;
		drawVert_t *ov;

		ov = (drawVert_t*)ri.Hunk_Alloc( lVerts->filelen, h_low );

#ifdef Q3_BIG_ENDIAN
		for( i = 0; i < numVerts; i++ )
		{
			ov[i].xyz[0] = SwapF( verts[i].xyz[0] );
			ov[i].xyz[1] = SwapF( verts[i].xyz[1] );
			ov[i].xyz[2] = SwapF( verts[i].xyz[2] );

			ov[i].normal[0] = SwapF( verts[i].normal[0] ); 
			ov[i].normal[1] = SwapF( verts[i].normal[1] );
			ov[i].normal[2] = SwapF( verts[i].normal[2] );

			ov[i].st[0] = SwapF( verts[i].st[0] );
			ov[i].st[1] = SwapF( verts[i].st[1] );

			ov[i].lightmap[0] = SwapF( verts[i].lightmap[0] );
			ov[i].lightmap[1] = SwapF( verts[i].lightmap[1] );

			*(int*)ov[i].color = *(int*)verts[i].color;
		}
#else
		Com_Memcpy( ov, verts, lVerts->filelen );
#endif
		verts = ov;

		oi = (int*)ri.Hunk_Alloc( lIndices->filelen, h_low );
#ifdef Q3_BIG_ENDIAN
		for( i = 0; i < numIndices; i++ )
			oi[i] = Swap4u( indices[i] );
#else
		Com_Memcpy( oi, indices, lIndices->filelen );
#endif
		indices = oi;
	}

	out = (msurface_t*)ri.Hunk_Alloc( sizeof( msurface_t ) * count, h_low );
	Com_Memset( out, 0, sizeof( msurface_t ) * count );

	s_worldData.surfaces = out;
	s_worldData.numsurfaces = count;

	for( i = 0; i < lVerts->filelen / sizeof( drawVert_t ); i++ )
		R_ColorShiftLightingBytes( verts[i].color, verts[i].color );

	for( i = 0; i < count; i++, in++, out++ )
	{
		switch( LittleLong( in->surfaceType ) )
		{
		case MST_PATCH:
			ParseMesh( in, verts, out );
			numMeshes++;
			break;

		case MST_TRIANGLE_SOUP:
			ParseTriSurf( in, verts, out, indices );
			numTriSurfs++;
			break;

		case MST_PLANAR:
			ParseFace( in, verts, out, indices );
			numFaces++;
			break;

		case MST_FLARE:
			ParseFlare( in, verts, out, indices );
			numFlares++;
			break;

		default:
			ri.Error( ERR_DROP, "Bad surfaceType" );
		}
	}

#ifdef PATCH_STITCHING
	R_StitchAllPatches();
#endif

	R_FixSharedVertexLodError();

#ifdef PATCH_STITCHING
	R_MovePatchSurfacesToHunk();
#endif

	ri.Printf( PRINT_ALL, "...loaded %d faces, %i meshes, %i trisurfs, %i flares\n", 
		numFaces, numMeshes, numTriSurfs, numFlares );
}

static void R_BspLoadExtSurfaces( const lump_t *surfsLump, const lump_t *vertLump, const lump_t *indexLump, bool fast )
{
	uint i;
	
	const dsurface_ext_t *inSurfs;
	const drawVert_ex_t *inVerts;
	const ushort *inIndices;

	uint numVerts, numIndices;

	drawVert_ex_t *outVerts;
	ushort *outIndices;

	if( surfsLump->filelen % sizeof( dsurface_ext_t ) )
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	if( vertLump->filelen % sizeof( drawVert_ex_t ) )
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	if( indexLump->filelen % sizeof( short ) )
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );

	if( !surfsLump->filelen )
		return;

	inSurfs = (dsurface_ext_t*)(fileBase + surfsLump->fileofs);
	inVerts = (drawVert_ex_t*)(fileBase + vertLump->fileofs);
	inIndices = (ushort*)(fileBase + indexLump->fileofs);

	numVerts = vertLump->filelen / sizeof( drawVert_ex_t );
	numIndices = indexLump->filelen / sizeof( ushort );

	s_worldData.numExSurfs = surfsLump->filelen / sizeof( dsurface_ext_t );
	s_worldData.exSurfs = (msurface_ex_t*)ri.Hunk_Alloc( sizeof( msurface_ex_t ) * s_worldData.numExSurfs, h_low );

	if( fast )
	{
		outVerts = (drawVert_ex_t*)inVerts;
		outIndices = (ushort*)inIndices;
	}
	else
	{
		outVerts = (drawVert_ex_t*)ri.Hunk_Alloc( sizeof( drawVert_ex_t ) * numVerts, h_low );
		outIndices = (ushort*)ri.Hunk_Alloc( sizeof( short ) * numIndices, h_low );

		for( i = 0; i < numVerts; i++ )
		{
			LittleVec3( outVerts[i].pos, inVerts[i].pos );
		
			LittleVec3( outVerts[i].norm, inVerts[i].norm );
			LittleVec3( outVerts[i].tan, inVerts[i].tan );
			LittleVec3( outVerts[i].bin, inVerts[i].bin );
			
			LittleVec2( outVerts[i].uvC, inVerts[i].uvC );
			LittleVec2( outVerts[i].uvL, inVerts[i].uvL );

			R_ColorShiftLightingBytes( inVerts[i].cl, outVerts[i].cl );
		}

		for( i = 0;	i < numIndices; i++ )
			outIndices[i] = LittleShort( inIndices[i] );
	}
	
	for( i = 0; i < s_worldData.numExSurfs; i++ )
	{
		const dsurface_ext_t *is = inSurfs + i;
		msurface_ex_t *es = s_worldData.exSurfs + i;

		es->surfType = SF_BSPEXMESH;

		switch( LittleLong( is->primType ) )
		{
		case SURF_PRIM_TRI_LIST:
			es->primType = GL_TRIANGLES;
			break;

		case SURF_PRIM_TRI_STRIP:
			es->primType = GL_TRIANGLE_STRIP;
			break;

		case SURF_PRIM_TRI_FAN:
			es->primType = GL_TRIANGLE_FAN;
			break;

		default:
			ri.Error( ERR_DROP, "Invalid primitive type." );
		}

		es->numVerts = LittleLong( is->numVerts );
		es->verts = outVerts + LittleLong( is->firstVert );
		es->numIndices = LittleLong( is->numIndices );
		es->indices = outIndices + LittleLong( is->firstIndex );
	}	
}

static void R_BspLoadExtSurfMap( const lump_t *l )
{
	int i;
	const uint *remap;

	if( !s_worldData.exSurfs )
		return;

	if( l->filelen % sizeof( uint ) )
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	if( l->filelen / sizeof( uint ) != (uint)s_worldData.numsurfaces )
		ri.Error( ERR_DROP, "LoadMap: wrong number of remap entries in %s", s_worldData.name );

	remap = (uint*)(fileBase + l->fileofs);

	for( i = 0; i < s_worldData.numsurfaces; i++ )
		if( LittleLong( remap[i] ) < s_worldData.numExSurfs )
			s_worldData.surfaces[i].redirect = s_worldData.exSurfs + LittleLong( remap[i] );
}

/*
=================
R_LoadSubmodels
=================
*/
static	void R_LoadSubmodels( lump_t *l )
{
	dmodel_t	*in;
	bmodel_t	*out;
	int			i, j, count;

	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);

	s_worldData.bmodels = out = ri.Hunk_Alloc( count * sizeof(*out), h_low );

	for ( i=0 ; i<count ; i++, in++, out++ ) {
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL );			// this should never happen

		model->type = MOD_BRUSH;
		model->bmodel = out;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );

		for (j=0 ; j<3 ; j++) {
			out->bounds[0][j] = LittleFloat (in->mins[j]);
			out->bounds[1][j] = LittleFloat (in->maxs[j]);
		}

		out->firstSurfaceNum = LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );
	}
}

static void R_LoadSubmodelsFast( const lump_t *lModels )
{
	cmodel_t *in;
	bmodel_t *out;
	int i, j, count;

	in = (void *)(fileBase + lModels->fileofs);
	if( lModels->filelen % sizeof( *in ) )
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	count = lModels->filelen / sizeof( *in );

	s_worldData.bmodels = out = ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	for( i = 0; i < count; i++, in++, out++ )
	{
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL ); //this should never happen

		model->type = MOD_BRUSH;
		model->bmodel = out;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );

		for( j = 0; j < 3; j++ )
		{
			out->bounds[0][j] = LittleFloat( in->leaf.mins[j] );
			out->bounds[1][j] = LittleFloat( in->leaf.maxs[j] );
		}

		out->firstSurfaceNum = s_worldData.marksurfaces[LittleLong( in->leaf.firstLeafSurface )];
		out->numSurfaces = LittleLong( in->leaf.numLeafSurfaces );
	}
}

//==================================================================

/*
=================
R_SetParent
=================
*/
static	void R_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	R_SetParent (node->children[0], node);
	R_SetParent (node->children[1], node);
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static void R_LoadNodesAndLeafs( const lump_t *nodeLump, const lump_t *leafLump )
{
	int			i, j, p;
	dnode_t		*in;
	dleaf_t		*inLeaf;
	mnode_t 	*out;
	int			numNodes, numLeafs;

	in = (void *)(fileBase + nodeLump->fileofs);
	if (nodeLump->filelen % sizeof(dnode_t) ||
		leafLump->filelen % sizeof(dleaf_t) ) {
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	numNodes = nodeLump->filelen / sizeof(dnode_t);
	numLeafs = leafLump->filelen / sizeof(dleaf_t);

	out = ri.Hunk_Alloc ( (numNodes + numLeafs) * sizeof(*out), h_low);	

	s_worldData.nodes = out;
	s_worldData.numnodes = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// load nodes
	for ( i=0 ; i<numNodes; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleLong (in->mins[j]);
			out->maxs[j] = LittleLong (in->maxs[j]);
		}
	
		p = LittleLong(in->planeNum);
		out->plane = s_worldData.planes + p;

		out->contents = CONTENTS_NODE;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = s_worldData.nodes + p;
			else
				out->children[j] = s_worldData.nodes + numNodes + (-1 - p);
		}
	}
	
	// load leafs
	inLeaf = (void *)(fileBase + leafLump->fileofs);
	for ( i=0 ; i<numLeafs ; i++, inLeaf++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleLong (inLeaf->mins[j]);
			out->maxs[j] = LittleLong (inLeaf->maxs[j]);
		}

		out->cluster = LittleLong(inLeaf->cluster);
		out->area = LittleLong(inLeaf->area);

		if ( out->cluster >= s_worldData.numClusters ) {
			s_worldData.numClusters = out->cluster + 1;
		}

		out->firstmarksurface = s_worldData.marksurfaces +
			LittleLong( inLeaf->firstLeafSurface );
		out->nummarksurfaces = LittleLong(inLeaf->numLeafSurfaces);
	}	

	// chain decendants
	R_SetParent (s_worldData.nodes, NULL);
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
static void R_LoadShaders( lump_t *l )
{	
	int i, count;
	dshader_t *in, *out;
	
	in = (void*)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = ri.Hunk_Alloc ( count*sizeof(*out), h_low );

	s_worldData.shaders = out;
	s_worldData.numShaders = count;

	Com_Memcpy( out, in, count*sizeof(*out) );

	for ( i=0 ; i<count ; i++ ) {
		out[i].surfaceFlags = LittleLong( out[i].surfaceFlags );
		out[i].contentFlags = LittleLong( out[i].contentFlags );
	}
}


/*
=================
R_LoadMarksurfaces
=================
*/
static void R_LoadMarksurfaces( const lump_t *l )
{	
	int i, count;
	int *in;
	int *out;
	
	in = (int*)(fileBase + l->fileofs);
	if( l->filelen % sizeof( int ) )
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	count = l->filelen / sizeof( int );

	out = (int*)ri.Hunk_Alloc( count * sizeof( int ), h_low );

	s_worldData.marksurfaces = out;
	s_worldData.nummarksurfaces = count;

	for( i = 0; i < count; i++ )
		out[i] = LittleLong( in[i] );
}


/*
=================
R_LoadPlanes
=================
*/
static void R_LoadPlanes( lump_t *l )
{
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	count = l->filelen / sizeof(*in);
	out = ri.Hunk_Alloc ( count*2*sizeof(*out), h_low);	
	
	s_worldData.planes = out;
	s_worldData.numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++) {
		bits = 0;
		for (j=0 ; j<3 ; j++) {
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0) {
				bits |= 1<<j;
			}
		}

		out->dist = LittleFloat (in->dist);
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}
}

/*
=================
R_LoadFogs

=================
*/
static	void R_LoadFogs( lump_t *l, lump_t *brushesLump, lump_t *sidesLump ) {
	int			i;
	fog_t		*out;
	dfog_t		*fogs;
	dbrush_t 	*brushes, *brush;
	dbrushside_t	*sides;
	int			count, brushesCount, sidesCount;
	int			sideNum;
	int			planeNum;
	shader_t	*shader;
	float		d;
	int			firstSide;

	fogs = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*fogs)) {
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	count = l->filelen / sizeof(*fogs);

	// create fog strucutres for them
	s_worldData.numfogs = count + 1;
	s_worldData.fogs = ri.Hunk_Alloc ( s_worldData.numfogs*sizeof(*out), h_low);
	out = s_worldData.fogs + 1;

	if ( !count ) {
		return;
	}

	brushes = (void *)(fileBase + brushesLump->fileofs);
	if (brushesLump->filelen % sizeof(*brushes)) {
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	brushesCount = brushesLump->filelen / sizeof(*brushes);

	sides = (void *)(fileBase + sidesLump->fileofs);
	if (sidesLump->filelen % sizeof(*sides)) {
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);
	}
	sidesCount = sidesLump->filelen / sizeof(*sides);

	for ( i=0 ; i<count ; i++, fogs++) {
		out->originalBrushNumber = LittleLong( fogs->brushNum );

		if ( (unsigned)out->originalBrushNumber >= brushesCount ) {
			ri.Error( ERR_DROP, "fog brushNumber out of range" );
		}
		brush = brushes + out->originalBrushNumber;

		firstSide = LittleLong( brush->firstSide );

			if ( (unsigned)firstSide > sidesCount - 6 ) {
			ri.Error( ERR_DROP, "fog brush sideNumber out of range" );
		}

		// brushes are always sorted with the axial sides first
		sideNum = firstSide + 0;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][0] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 1;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][0] = s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 2;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][1] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 3;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][1] = s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 4;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][2] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 5;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][2] = s_worldData.planes[ planeNum ].dist;

		// get information from the shader for fog parameters
		shader = R_FindShader( fogs->shader, LIGHTMAP_NONE, qtrue );
		Q_strncpyz( out->shader, shader->name, sizeof( out->shader ) );

		out->parms = shader->fogParms;

		out->colorInt = ColorBytes4 ( shader->fogParms.color[0] * tr.identityLight, 
			                          shader->fogParms.color[1] * tr.identityLight, 
			                          shader->fogParms.color[2] * tr.identityLight, 1.0 );

		d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / ( d * 8 );

		// set the gradient vector
		sideNum = LittleLong( fogs->visibleSide );

		if ( sideNum == -1 ) {
			out->hasSurface = qfalse;
		} else {
			out->hasSurface = qtrue;
			planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( vec3_origin, s_worldData.planes[ planeNum ].normal, out->surface );
			out->surface[3] = -s_worldData.planes[ planeNum ].dist;
		}

		out++;
	}

}

static void R_LoadFogsFast( const lump_t *l, const lump_t *brushesLump, const lump_t *sidesLump )
{
	int i;

	dbrush_t *brushes;
	dbrushside_t *sides;

	s_worldData.fogs = (fog_t*)(fileBase + l->fileofs);
	s_worldData.numfogs = l->filelen / sizeof( fog_t );

	if( l->filelen % sizeof( fog_t ) )
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );

	brushes = (dbrush_t*)(fileBase + brushesLump->fileofs);
	if( brushesLump->filelen % sizeof( *brushes ) )
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	sides = (dbrushside_t*)(fileBase + sidesLump->fileofs);
	if( sidesLump->filelen % sizeof( *sides ) )
		ri.Error (ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name);

	for( i = 1; i < s_worldData.numfogs; i++ )
	{
		fog_t *fog = s_worldData.fogs + i;

		float d;
		shader_t *shader;

		//get information from the shader for fog parameters
		shader = R_FindShader( fog->shader, LIGHTMAP_NONE, qtrue );

		fog->parms = shader->fogParms;

		fog->colorInt = ColorBytes4(
			shader->fogParms.color[0] * tr.identityLight, 
			shader->fogParms.color[1] * tr.identityLight,
			shader->fogParms.color[2] * tr.identityLight, 1.0 );

		d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		fog->tcScale = 1.0f / ( d * 8 );
	}
}

static int R_BspCalcLightGridDimensions( world_t *w )
{
	int		i;
	vec3_t	maxs;
	int		numGridPoints;
	float	*wMins, *wMaxs;

	w->lightGridInverseSize[0] = 1.0F / w->lightGridSize[0];
	w->lightGridInverseSize[1] = 1.0F / w->lightGridSize[1];
	w->lightGridInverseSize[2] = 1.0F / w->lightGridSize[2];

	wMins = w->bmodels[0].bounds[0];
	wMaxs = w->bmodels[0].bounds[1];

	for( i = 0; i < 3; i++ )
	{
		w->lightGridOrigin[i] = w->lightGridSize[i] * ceil( wMins[i] / w->lightGridSize[i] );
		maxs[i] = w->lightGridSize[i] * floor( wMaxs[i] / w->lightGridSize[i] );
		w->lightGridBounds[i] = (maxs[i] - w->lightGridOrigin[i])/w->lightGridSize[i] + 1;
	}

	numGridPoints = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	return numGridPoints;
}

/*
================
R_LoadLightGrid

================
*/
static void R_BspLoadLightGrid( const lump_t *l, bool fast )
{
	int		i;
	int		numGridPoints;

	world_t	*w = &s_worldData;

	numGridPoints = R_BspCalcLightGridDimensions( w );

	if( l->filelen != numGridPoints * 8 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: light grid mismatch\n" );
		w->lightGridType = LIGHT_NONE;
		return;
	}

	w->lightGridType = LIGHT_GRID;

	if( fast )
	{
		w->lightGridData0 = fileBase + l->fileofs;
	}
	else
	{
		w->lightGridData0 = ri.Hunk_Alloc( l->filelen, h_low );
		Com_Memcpy( w->lightGridData0, (void*)(fileBase + l->fileofs), l->filelen );
	}

	{
		byte *data = (byte*)w->lightGridData0;
		// deal with overbright bits
		for( i = numGridPoints; i-- != 0; data += 8 )
		{
			R_ColorShiftLightingBytes( data + 0, data + 0 );
			R_ColorShiftLightingBytes( data + 3, data + 3 );
		}
	}
}

static void R_BspLoadLightTree( const lump_t *lInt, int interiorBits, const lump_t *lLeaves, bool fast )
{
	int numGridPoints;

	world_t	*w = &s_worldData;

	numGridPoints = R_BspCalcLightGridDimensions( w );

	switch( interiorBits )
	{
	case 16:
		w->lightGridType = LIGHT_TREE_16;
		break;

	case 32:
		w->lightGridType = LIGHT_TREE_32;
		break;

	default:
		ri.Printf( PRINT_WARNING, "WARNING: invalid light tree interior type\n" );
		w->lightGridType = LIGHT_NONE;
		return;
	}
	
	if( fast )
	{
		w->lightGridData0 = fileBase + lInt->fileofs;

		w->lightGridData1 = fileBase + lLeaves->fileofs;
	}
	else
	{
		w->lightGridData0 = ri.Hunk_Alloc( lInt->filelen, h_low );
		Com_Memcpy( w->lightGridData0, (void*)(fileBase + lInt->fileofs), lInt->filelen );

		w->lightGridData1 = ri.Hunk_Alloc( lLeaves->filelen, h_low );
		Com_Memcpy( w->lightGridData1, (void*)(fileBase + lLeaves->fileofs), lLeaves->filelen );
	}
#ifdef Q3_BIG_ENDIAN
	{
		int i,c;
		uint *p32;
		ushort *p16;
		switch( interiorBits )
		{
			case 16:
				c = lInt->filelen/sizeof(ushort);
				p16 = (ushort *)w->lightGridData0;
				for (i = 0; i < c; i++ ) {
					p16[i] = Swap2u(p16[i]);
				}
				break;
				
			case 32:
				c = lInt->filelen/sizeof(uint);
				p32 = (uint *)w->lightGridData0;
				for (i = 0; i < c; i++ ) {
					p32[i] = Swap4u(p32[i]);
				}
				break;
				
			NO_DEFAULT;
		}
		c = lLeaves->filelen/sizeof(ushort);
		p16 = (ushort *)w->lightGridData1;
		for (i = 0; i < c; i++ ) {
			p16[i] = Swap2u(p16[i]);
		}
	}
	
#endif
}

/*
================
R_LoadEntities
================
*/
void R_LoadEntities( const lump_t *l, bool fastLoad )
{
	char *p, *token, *s;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	world_t	*w;

	w = &s_worldData;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	p = (char *)(fileBase + l->fileofs);

	// store for reference by the cgame
	if( fastLoad )
	{
		w->entityString = p;
	}
	else
	{
		w->entityString = ri.Hunk_Alloc( l->filelen + 1, h_low );
		strcpy( w->entityString, p );
	}

	w->entityParsePoint = w->entityString;

	token = COM_ParseExt( &p, qtrue );
	if (!*token || *token != '{') {
		return;
	}

	// only parse the world spawn
	for( ; ; )
	{	
		// parse key
		token = COM_ParseExt( &p, qtrue );

		if( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "Unexpected ene of entities while parsing worldspawn.\n" );
			break;
		}

		if( token[0] == '{' )
			continue;

		if( token[0] == '}' )
			break;

		Q_strncpyz( keyname, token, sizeof( keyname ) );

		// parse value
		token = COM_ParseExt( &p, qtrue );

		if( token[0] == 0 )
			continue;

		Q_strncpyz( value, token, sizeof( value ) );

		if( Q_stricmp( keyname, "classname" ) == 0 )
		{
			if( Q_stricmp( value, "worldspawn" ) != 0 )
				ri.Printf(PRINT_WARNING, "WARNING: expected worldspawn found '%s'\n", value);

			continue;
		}

		// check for remapping of shaders for vertex lighting
		if( Q_stricmp( keyname, "vertexremapshader" ) == 0 )
		{
			s = strchr(value, ';');
			if( !s )
			{
				ri.Printf( PRINT_WARNING, "WARNING: no semicolon in vertexshaderremap '%s'\n", value );
				break;
			}
			
			*s++ = 0;
			
			if( r_vertexLight->integer )
				R_RemapShader( value, s, "0" );

			continue;
		}
		
		// check for remapping of shaders
		if( Q_stricmp( keyname, "remapshader" ) == 0 )
		{
			s = strchr(value, ';');
			if( !s )
			{
				ri.Printf( PRINT_WARNING, "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}
			
			*s++ = 0;
			
			R_RemapShader( value, s, "0" );
			
			continue;
		}

		// check for a different grid size
		if( Q_stricmp( keyname, "gridsize" ) == 0 )
		{
			sscanf(value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2] );
			continue;
		}

		if( Q_stricmp( keyname, "deluxeMapping" ) == 0 )
		{
			s_worldData.deluxeMapping = atoi( value ) ? qtrue : qfalse;
			continue;
		}
	}
}

/*
=================
R_GetEntityToken
=================
*/
qboolean Q_EXTERNAL_CALL R_GetEntityToken( char *buffer, int size )
{
	const char	*s;

	s = COM_Parse( &s_worldData.entityParsePoint );
	Q_strncpyz( buffer, s, size );

	if( !s_worldData.entityParsePoint || !s[0] )
	{
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qfalse;
	}
	
	return qtrue;
}

//from tr_lightmaphack.c
void R_RemapDeluxeImages( world_t *world );

/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void Q_EXTERNAL_CALL RE_LoadWorldMap( const char *name )
{
	int			i;
	dheader_t	*header;
	dheader_ex_t *exHead;
	byte		*buffer;
	byte		*startMarker;

	if( tr.worldMapLoaded )
		ri.Error( ERR_DROP, "ERROR: attempted to redundantly load world map\n" );

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[0] = 0.45F;
	tr.sunDirection[1] = 0.3F;
	tr.sunDirection[2] = 0.9F;

	VectorNormalize( tr.sunDirection );

	{
		//try bspf
		int length = ri.FS_ReadFile( va( "%sf", name ), (void**)&buffer );
		if( buffer )
		{
			byte *d = ri.Hunk_Alloc( length, h_low );
			Com_Memcpy( d, buffer, length );
			RE_LoadWorldMapFast( d, name );
			ri.FS_FreeFile( buffer );

			return;
		}
	}

	tr.worldMapLoaded = qtrue;

	// load it
    ri.FS_ReadFile( name, (void**)&buffer );
	if( !buffer )
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s not found", name );

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	Com_Memset( &s_worldData, 0, sizeof( s_worldData ) );
	Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );

	Q_strncpyz( s_worldData.baseName, COM_SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	COM_StripExtension( s_worldData.baseName, s_worldData.baseName, sizeof( s_worldData.baseName ) );

	startMarker = ri.Hunk_Alloc( 0, h_low );

	header = (dheader_t*)buffer;
	fileBase = (byte*)header;

	i = LittleLong (header->version);
	if( i != BSP_VERSION )
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i)", name, i, BSP_VERSION );

	// swap all the lumps
#ifdef Q3_BIG_ENDIAN
	for( i = 0; i < sizeof( dheader_t ) / 4; i++ )
		((int*)header)[i] = LittleLong( ((int*)header)[i] );

#endif
	//ToDo: byte swap (bother?)
	exHead = (dheader_ex_t*)(header + 1);
	if( LittleLong( exHead->ident ) == BSP_IDENT && LittleLong( exHead->version ) == BSP_EX_VERSION && LittleLong( exHead->numLumps ) >= 4 )
	{
#ifdef Q3_BIG_ENDIAN
		//swap the extended header
		for( i = 0; i < LittleLong( exHead->numLumps ); i++ )
		{
			exHead->lumps[i].filelen = Swap4u( exHead->lumps[i].filelen );
			exHead->lumps[i].fileofs = Swap4u( exHead->lumps[i].fileofs );
		}
#endif
		R_BspLoadExtSurfaces( exHead->lumps + LUMP_EXT_SURFS, exHead->lumps + LUMP_EXT_VERTS, exHead->lumps + LUMP_EXT_INDICES, false );
	}
	else
		exHead = NULL;

	// load into heap
	R_LoadEntities( header->lumps + LUMP_ENTITIES, false );
	
	R_LoadSubmodels( header->lumps + LUMP_MODELS );

	if( exHead && LittleLong(exHead->numLumps) > LUMP_EXT_LIGHT_TREE_LEAVES )
	{
		if( exHead->lumps[LUMP_EXT_LIGHT_TREE_16].filelen )
		{
			R_BspLoadLightTree( exHead->lumps + LUMP_EXT_LIGHT_TREE_16, 16, exHead->lumps + LUMP_EXT_LIGHT_TREE_LEAVES, false );
		}
		else if( exHead->lumps[LUMP_EXT_LIGHT_TREE_32].filelen )
		{
			R_BspLoadLightTree( exHead->lumps + LUMP_EXT_LIGHT_TREE_32, 32, exHead->lumps + LUMP_EXT_LIGHT_TREE_LEAVES, false );
		}
		else
			R_BspLoadLightGrid( header->lumps + LUMP_LIGHTGRID, false );
	}
	else
		R_BspLoadLightGrid( header->lumps + LUMP_LIGHTGRID, false );

	R_LoadShaders( header->lumps + LUMP_SHADERS );
	R_LoadLightmaps( header->lumps + LUMP_LIGHTMAPS );

	R_LoadPlanes( header->lumps + LUMP_PLANES );
	
	R_LoadFogs( header->lumps + LUMP_FOGS, header->lumps + LUMP_BRUSHES, header->lumps + LUMP_BRUSHSIDES );
	R_LoadSurfaces( header->lumps + LUMP_SURFACES, header->lumps + LUMP_DRAWVERTS, header->lumps + LUMP_DRAWINDEXES, false );

	if( s_worldData.exSurfs )
		R_BspLoadExtSurfMap( exHead->lumps + LUMP_EXT_SURFS_MAP );

	R_LoadMarksurfaces( header->lumps + LUMP_LEAFSURFACES );
	R_LoadNodesAndLeafs( header->lumps + LUMP_NODES, header->lumps + LUMP_LEAFS );
	R_LoadVisibility( header->lumps + LUMP_VISIBILITY );

	s_worldData.dataSize = (byte *)ri.Hunk_Alloc( 0, h_low ) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

    ri.FS_FreeFile( buffer );

	//if we loaded q3map2 deluxe maps we need to convert them into tangent space
	if( tr.world->deluxeMapping && header->lumps[LUMP_LIGHTMAPS].filelen )
		R_RemapDeluxeImages( tr.world );
}

/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void Q_EXTERNAL_CALL RE_LoadWorldMapFast( void *pMapData, const char *name )
{
	const fbspHeader_t *header;
	byte *buffer;
	byte *startMarker;

	if( tr.worldMapLoaded )
		ri.Error( ERR_DROP, "ERROR: attempted to redundantly load world map\n" );

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[0] = 0.45F;
	tr.sunDirection[1] = 0.3F;
	tr.sunDirection[2] = 0.9F;

	VectorNormalize( tr.sunDirection );

	tr.worldMapLoaded = qtrue;

	buffer = (byte*)pMapData;

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	Com_Memset( &s_worldData, 0, sizeof( s_worldData ) );
	Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );

	Q_strncpyz( s_worldData.baseName, COM_SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	COM_StripExtension( s_worldData.baseName, s_worldData.baseName, sizeof( s_worldData.baseName ) );

	startMarker = ri.Hunk_Alloc( 0, h_low );

	header = (fbspHeader_t*)buffer;
	fileBase = (byte*)header;

	if( header->version != FBSP_VERSION )
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (should be %i)", name, FBSP_VERSION );

	R_LoadEntities( header->lumps + FLUMP_ENTITIES, true );

	s_worldData.planes = (cplane_t*)(fileBase + header->lumps[FLUMP_PLANES].fileofs);
	s_worldData.numplanes = header->lumps[FLUMP_PLANES].filelen / sizeof( cplane_t );

	s_worldData.shaders = (dshader_t*)(fileBase + header->lumps[FLUMP_SHADERS].fileofs);
	s_worldData.numShaders = header->lumps[FLUMP_SHADERS].filelen / sizeof( dshader_t );

	s_worldData.marksurfaces = (int*)(fileBase + header->lumps[FLUMP_LEAFSURFACES].fileofs);
	s_worldData.nummarksurfaces = header->lumps[FLUMP_LEAFSURFACES].filelen / sizeof( int );

	s_worldData.marksurfaces = (int*)(fileBase + header->lumps[FLUMP_LEAFSURFACES].fileofs);
	s_worldData.nummarksurfaces = header->lumps[FLUMP_LEAFSURFACES].filelen / sizeof( int );

	R_LoadSubmodelsFast( header->lumps + FLUMP_MODELS );

	R_LoadLightmapsFast();
	R_LoadNodesAndLeafs( header->lumps + FLUMP_NODES, header->lumps + FLUMP_LEAFS );
	R_LoadFogsFast( header->lumps + FLUMP_FOGS, header->lumps + FLUMP_BRUSHES, header->lumps + FLUMP_BRUSHSIDES );
	
	if( header->lumps[FLUMP_EXT_LIGHT_TREE_16].filelen )
	{
		R_BspLoadLightTree( header->lumps + FLUMP_EXT_LIGHT_TREE_16, 16, header->lumps + FLUMP_EXT_LIGHT_TREE_LEAVES, true );
	}
	else if( header->lumps[FLUMP_EXT_LIGHT_TREE_32].filelen )
	{
		R_BspLoadLightTree( header->lumps + FLUMP_EXT_LIGHT_TREE_32, 32, header->lumps + FLUMP_EXT_LIGHT_TREE_LEAVES, true );
	}
	else
		R_BspLoadLightGrid( header->lumps + LUMP_LIGHTGRID, true );

	R_LoadVisibilityFast( header->lumps + FLUMP_VISIBILITY );
	
	R_BspLoadExtSurfaces( header->lumps + FLUMP_EXT_SURFS, header->lumps + FLUMP_EXT_VERTS, header->lumps + FLUMP_EXT_INDICES, true );
	R_LoadSurfaces( header->lumps + FLUMP_SURFACES, header->lumps + FLUMP_DRAWVERTS, header->lumps + FLUMP_DRAWINDEXES, true );
	R_BspLoadExtSurfMap( header->lumps + FLUMP_EXT_SURFS_MAP );

	s_worldData.dataSize = (byte *)ri.Hunk_Alloc( 0, h_low ) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;
}
