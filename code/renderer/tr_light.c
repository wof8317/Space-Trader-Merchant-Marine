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

#define	DLIGHT_AT_RADIUS		16
// at the edge of a dlight's influence, this amount of light will be added

#define	DLIGHT_MINIMUM_RADIUS	16		
// never calculate a range less than this to prevent huge light numbers


/*
===============
R_TransformDlights

Transforms the origins of an array of dlights.
Used by both the front end (for DlightBmodel) and
the back end (before doing the lighting calculation)
===============
*/
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *or) {
	int		i;
	vec3_t	temp;

	for ( i = 0 ; i < count ; i++, dl++ ) {
		VectorSubtract( dl->origin, or->origin, temp );
		dl->transformed[0] = DotProduct( temp, or->axis[0] );
		dl->transformed[1] = DotProduct( temp, or->axis[1] );
		dl->transformed[2] = DotProduct( temp, or->axis[2] );
	}
}

/*
=============
R_DlightBmodel

Determine which dynamic lights may effect this bmodel
=============
*/
void R_DlightBmodel( bmodel_t *bmodel ) {
	int			i, j;
	dlight_t	*dl;
	int			mask;
	msurface_t	*surf;

	// transform all the lights
	R_TransformDlights( tr.refdef.num_dlights, tr.refdef.dlights, &tr.or );

	mask = 0;
	for ( i=0 ; i<tr.refdef.num_dlights ; i++ ) {
		dl = &tr.refdef.dlights[i];

		// see if the point is close enough to the bounds to matter
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( dl->transformed[j] - bmodel->bounds[1][j] > dl->radius ) {
				break;
			}
			if ( bmodel->bounds[0][j] - dl->transformed[j] > dl->radius ) {
				break;
			}
		}
		if ( j < 3 ) {
			continue;
		}

		// we need to check this light
		mask |= 1 << i;
	}

	tr.currentEntity->needDlights = (mask != 0);

	// set the dlight bits in all the surfaces
	for ( i = 0 ; i < bmodel->numSurfaces ; i++ ) {
		surf = tr.world->surfaces + bmodel->firstSurfaceNum + i;

		if ( *surf->data == SF_FACE ) {
			((srfSurfaceFace_t *)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		} else if ( *surf->data == SF_GRID ) {
			((srfGridMesh_t *)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		} else if ( *surf->data == SF_TRIANGLES ) {
			((srfTriangles_t *)surf->data)->dlightBits[ tr.smpFrame ] = mask;
		}
	}
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

extern	cvar_t	*r_ambientScale;
extern	cvar_t	*r_directedScale;
extern	cvar_t	*r_debugLight;

static int R_FindLightTreeLeaf16( int pos[3] )
{
	int xMin = 0;
	int xMax = tr.world->lightGridBounds[0];

	int yMin = 0;
	int yMax = tr.world->lightGridBounds[1];

	int zMin = 0;
	int zMax = tr.world->lightGridBounds[2];

	const ushort *tree = (short*)tr.world->lightGridData0;

	ushort idx = tree[0];

	while( idx & 0xE000 ) //while ! a leaf
	{
		int ofs = 0;
		int mul = 0;

		if( idx & 0x2000 ) //if splitting along Z
		{
			int zBound = (zMin + zMax) / 2;

			if( pos[2] < zBound )
			{
				ofs = 1;
				zMax = zBound;
			}
			else
			{
				zMin = zBound;
			}

			mul = 1;
		}

		if( idx & 0x4000 ) //if splitting along Y
		{
			int yBound = (yMin + yMax) / 2;

			if( pos[1] < yBound )
			{
				ofs |= 1 << mul;
				yMax = yBound;
			}
			else
			{
				yMin = yBound;
			}

			mul++;
		}

		if( idx & 0x8000 ) //if splitting along X
		{
			int xBound = (xMin + xMax) / 2;

			if( pos[0] < xBound )
			{
				ofs |= 1 << mul;
				xMax = xBound;
			}
			else
			{
				xMin = xBound;
			}
		}

		idx = tree[(idx & 0x1FFF) + ofs]; //idx & 0x1FFF = the index to the start of this node's children
	}

	return idx;
}

static int R_FindLightTreeLeaf32( int pos[3] )
{
	int xMin = 0;
	int xMax = tr.world->lightGridBounds[0];

	int yMin = 0;
	int yMax = tr.world->lightGridBounds[1];

	int zMin = 0;
	int zMax = tr.world->lightGridBounds[2];

	const uint *tree = (uint*)tr.world->lightGridData0;

	uint idx = tree[0];

	while( idx & 0xFC000000 ) //while ! a leaf
	{
		int ofs = 0;
		int mul = 0;

		int xSplit = (idx >> 30) & 0x3;
		int ySplit = (idx >> 28) & 0x3;
		int zSplit = (idx >> 26) & 0x3;

		if( zSplit )
		{
			int zBound = zMin + ((zMax - zMin) * zSplit) / 4;

			if( pos[2] < zBound )
			{
				ofs = 1;
				zMax = zBound;
			}
			else
			{
				zMin = zBound;
			}

			mul = 1;
		}

		if( ySplit )
		{
			int yBound = yMin + ((yMax - yMin) * ySplit) / 4;

			if( pos[1] < yBound )
			{
				ofs |= 1 << mul;
				yMax = yBound;
			}
			else
			{
				yMin = yBound;
			}

			mul++;
		}

		if( xSplit )
		{
			int xBound = xMin + ((xMax - xMin) * xSplit) / 4;

			if( pos[0] < xBound )
			{
				ofs |= 1 << mul;
				xMax = xBound;
			}
			else
			{
				xMin = xBound;
			}
		}

		idx = tree[(idx & 0x3FFFFFF) + ofs]; //idx & 0x3FFFFFF = the index to the start of this node's children
	}

	return idx;
}

static int R_FindLightTreeLeaf( int pos[3] )
{
	switch( tr.world->lightGridType )
	{
	case LIGHT_TREE_16:
		return R_FindLightTreeLeaf16( pos );

	case LIGHT_TREE_32:
		return R_FindLightTreeLeaf32( pos );

	NO_DEFAULT;
	}

	return 0; //shouldn't really ever get here
}

/*
=================
R_SetupEntityLightingTree

=================
*/

typedef struct lightTreeLeaf_t
{
	short	ambient, diffuse;
	byte	lon, lat;
} lightTreeLeaf_t;

static ID_INLINE void UnpackRGB565( byte rgb[3], ushort cl )
{
	byte r = (byte)((cl >> 11) & 0x1F);
	byte g = (byte)((cl >> 5) & 0x3F);
	byte b = (byte)(cl & 0x1F);

	rgb[0] = (r << 3) | (r >> 2); //multiply by 8.22 -> 8.25
	rgb[1] = (g << 2) | (g >> 4); //multiply by 4.047 -> 4.0625
	rgb[2] = (b << 3) | (b >> 2); //multiply by 8.22 -> 8.25
}

static void R_SetupEntityLightingTree( trRefEntity_t *ent )
{
	vec3_t	lightOrigin;
	int		pos[3];
	int		i, j;
	float	frac[3];
	vec3_t	direction;
	float	totalFactor;

	const lightTreeLeaf_t *tree;

	if( ent->e.renderfx & RF_LIGHTING_ORIGIN )
	{
		// seperate lightOrigins are needed so an object that is
		// sinking into the ground can still be lit, and so
		// multi-part models can be lit identically
		VectorCopy( ent->e.lightingOrigin, lightOrigin );
	}
	else
	{
		VectorCopy( ent->e.origin, lightOrigin );
	}

	VectorSubtract( lightOrigin, tr.world->lightGridOrigin, lightOrigin );
	for ( i = 0 ; i < 3 ; i++ ) {
		float	v;

		v = lightOrigin[i] * tr.world->lightGridInverseSize[i];

		pos[i] = floor( v );
		frac[i] = v - pos[i];
		
		if( pos[i] < 0 )
		{
			pos[i] = 0;
		}	   
		else if( pos[i] > tr.world->lightGridBounds[i] - 1 )
		{
			pos[i] = tr.world->lightGridBounds[i] - 1;
		}
	}

	VectorClear( ent->ambientLight );
	VectorClear( ent->directedLight );
	VectorClear( direction );

	tree = (lightTreeLeaf_t*)tr.world->lightGridData1;

	// trilerp the light value
	totalFactor = 0;
	for( i = 0; i < 8; i++ )
	{
		float factor;
		int lat, lng;
		vec3_t normal;
		
		int cpos[3], cidx;
		const lightTreeLeaf_t *leaf;

		byte rgb[3];

		factor = 1.0F;

		cpos[0] = pos[0]; cpos[1] = pos[1]; cpos[2] = pos[2];

		for( j = 0; j < 3; j++ )
		{
			if( i & (1 << j) )
			{
				factor *= frac[j];
				cpos[j]++;
			}
			else
			{
				factor *= 1.0F - frac[j];
			}
		}

		cidx = R_FindLightTreeLeaf( cpos );
		leaf = tree + cidx;

		if( !leaf->ambient )
			continue;	// ignore samples in walls

		totalFactor += factor;
		
		UnpackRGB565( rgb, leaf->ambient );

		ent->ambientLight[0] += factor * rgb[0];
		ent->ambientLight[1] += factor * rgb[1];
		ent->ambientLight[2] += factor * rgb[2];

		UnpackRGB565( rgb, leaf->diffuse );

		ent->directedLight[0] += factor * rgb[0];
		ent->directedLight[1] += factor * rgb[1];
		ent->directedLight[2] += factor * rgb[2];
		
		lat = leaf->lat;
		lng = leaf->lon;
		lat *= (FUNCTABLE_SIZE/256);
		lng *= (FUNCTABLE_SIZE/256);

		// decode X as cos( lat ) * sin( long )
		// decode Y as sin( lat ) * sin( long )
		// decode Z as cos( long )

		normal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
		normal[1] = tr.sinTable[lat] * tr.sinTable[lng];
		normal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

		VectorMA( direction, factor, normal, direction );
	}

	if ( totalFactor > 0 && totalFactor < 0.99 ) {
		totalFactor = 1.0f / totalFactor;
		VectorScale( ent->ambientLight, totalFactor, ent->ambientLight );
		VectorScale( ent->directedLight, totalFactor, ent->directedLight );
	}

	VectorScale( ent->ambientLight, r_ambientScale->value, ent->ambientLight );
	VectorScale( ent->directedLight, r_directedScale->value, ent->directedLight );

	VectorNormalize2( direction, ent->lightDir );
}

/*
=================
R_SetupEntityLightingGrid

=================
*/
static void R_SetupEntityLightingGrid( trRefEntity_t *ent ) {
	vec3_t	lightOrigin;
	int		pos[3];
	int		i, j;
	byte	*gridData;
	float	frac[3];
	int		gridStep[3];
	vec3_t	direction;
	float	totalFactor;

	if ( ent->e.renderfx & RF_LIGHTING_ORIGIN ) {
		// seperate lightOrigins are needed so an object that is
		// sinking into the ground can still be lit, and so
		// multi-part models can be lit identically
		VectorCopy( ent->e.lightingOrigin, lightOrigin );
	} else {
		VectorCopy( ent->e.origin, lightOrigin );
	}

	VectorSubtract( lightOrigin, tr.world->lightGridOrigin, lightOrigin );
	for ( i = 0 ; i < 3 ; i++ ) {
		float	v;

		v = lightOrigin[i]*tr.world->lightGridInverseSize[i];
		pos[i] = floor( v );
		frac[i] = v - pos[i];
		if ( pos[i] < 0 ) {
			pos[i] = 0;
		} else if ( pos[i] > tr.world->lightGridBounds[i] - 1 ) {
			pos[i] = tr.world->lightGridBounds[i] - 1;
		}
	}

	VectorClear( ent->ambientLight );
	VectorClear( ent->directedLight );
	VectorClear( direction );

	// trilerp the light value
	gridStep[0] = 8;
	gridStep[1] = 8 * tr.world->lightGridBounds[0];
	gridStep[2] = 8 * tr.world->lightGridBounds[0] * tr.world->lightGridBounds[1];
	gridData = (byte*)tr.world->lightGridData0 + pos[0] * gridStep[0]
		+ pos[1] * gridStep[1] + pos[2] * gridStep[2];

	totalFactor = 0;
	for ( i = 0 ; i < 8 ; i++ ) {
		float	factor;
		byte	*data;
		int		lat, lng;
		vec3_t	normal;
		#if idppc
		float d0, d1, d2, d3, d4, d5;
		#endif
		factor = 1.0;
		data = gridData;
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( i & (1<<j) ) {
				factor *= frac[j];
				data += gridStep[j];
			} else {
				factor *= (1.0f - frac[j]);
			}
		}

		if ( !(data[0]+data[1]+data[2]) ) {
			continue;	// ignore samples in walls
		}
		totalFactor += factor;
		#if idppc
		d0 = data[0]; d1 = data[1]; d2 = data[2];
		d3 = data[3]; d4 = data[4]; d5 = data[5];

		ent->ambientLight[0] += factor * d0;
		ent->ambientLight[1] += factor * d1;
		ent->ambientLight[2] += factor * d2;

		ent->directedLight[0] += factor * d3;
		ent->directedLight[1] += factor * d4;
		ent->directedLight[2] += factor * d5;
		#else
		ent->ambientLight[0] += factor * data[0];
		ent->ambientLight[1] += factor * data[1];
		ent->ambientLight[2] += factor * data[2];

		ent->directedLight[0] += factor * data[3];
		ent->directedLight[1] += factor * data[4];
		ent->directedLight[2] += factor * data[5];
		#endif
		lat = data[7];
		lng = data[6];
		lat *= (FUNCTABLE_SIZE/256);
		lng *= (FUNCTABLE_SIZE/256);

		// decode X as cos( lat ) * sin( long )
		// decode Y as sin( lat ) * sin( long )
		// decode Z as cos( long )

		normal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
		normal[1] = tr.sinTable[lat] * tr.sinTable[lng];
		normal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

		VectorMA( direction, factor, normal, direction );
	}

	if ( totalFactor > 0 && totalFactor < 0.99 ) {
		totalFactor = 1.0f / totalFactor;
		VectorScale( ent->ambientLight, totalFactor, ent->ambientLight );
		VectorScale( ent->directedLight, totalFactor, ent->directedLight );
	}

	VectorScale( ent->ambientLight, r_ambientScale->value, ent->ambientLight );
	VectorScale( ent->directedLight, r_directedScale->value, ent->directedLight );

	VectorNormalize2( direction, ent->lightDir );
}


/*
===============
LogLight
===============
*/
static void LogLight( trRefEntity_t *ent ) {
	int	max1, max2;

	if ( !(ent->e.renderfx & RF_FIRST_PERSON ) ) {
		return;
	}

	max1 = ent->ambientLight[0];
	if ( ent->ambientLight[1] > max1 ) {
		max1 = ent->ambientLight[1];
	} else if ( ent->ambientLight[2] > max1 ) {
		max1 = ent->ambientLight[2];
	}

	max2 = ent->directedLight[0];
	if ( ent->directedLight[1] > max2 ) {
		max2 = ent->directedLight[1];
	} else if ( ent->directedLight[2] > max2 ) {
		max2 = ent->directedLight[2];
	}

	ri.Printf( PRINT_ALL, "amb:%i  dir:%i\n", max1, max2 );
}

/*
=================
R_SetupEntityLighting

Calculates all the lighting values that will be used
by the Calc_* functions
=================
*/
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent )
{
	int				i;
	dlight_t		*dl;
	float			power;
	vec3_t			dir;
	float			d;
	vec3_t			lightDir;
	vec3_t			lightOrigin;

	// lighting calculations 
	if( ent->lightingCalculated )
		return;

	ent->lightingCalculated = qtrue;

	if( refdef->rdflags & RDF_SOLARLIGHT )
	{
		VectorSet( ent->directedLight, 0xFF, 0xFF, 0xFF );
		
		if( ent->e.renderfx & RF_LIGHTING_ORIGIN )
			VectorCopy( ent->e.lightingOrigin, lightDir );
		else
			VectorNegate( ent->e.origin, lightDir );

		VectorNormalize( lightDir );

		ent->lightDir[0] = DotProduct( lightDir, ent->e.axis[0] );
		ent->lightDir[1] = DotProduct( lightDir, ent->e.axis[1] );
		ent->lightDir[2] = DotProduct( lightDir, ent->e.axis[2] );

		if( ent->e.nonNormalizedAxes )
			VectorNormalize( ent->lightDir );

		VectorSet( ent->ambientLight, 0, 0, 0 );
		ent->ambientLightInt = 0;

		return;
	}

	//
	// trace a sample point down to find ambient light
	//
	if ( ent->e.renderfx & RF_LIGHTING_ORIGIN ) {
		// seperate lightOrigins are needed so an object that is
		// sinking into the ground can still be lit, and so
		// multi-part models can be lit identically
		VectorCopy( ent->e.lightingOrigin, lightOrigin );
	} else {
		VectorCopy( ent->e.origin, lightOrigin );
	}

	{
		bool useDefaultLighting = true;

		// if NOWORLDMODEL, only use dynamic lights (menu system, etc)
		if( (refdef->rdflags & RDF_NOWORLDMODEL) == 0 )
		{
			switch( tr.world->lightGridType )
			{
			case LIGHT_GRID:
				R_SetupEntityLightingGrid( ent );
				useDefaultLighting = false;
				break;

			case LIGHT_TREE_16:
			case LIGHT_TREE_32:
				R_SetupEntityLightingTree( ent );
				useDefaultLighting = false;
				break;

			default:
				break;
			}
		}
		
		if( useDefaultLighting )
		{
			ent->ambientLight[0] = ent->ambientLight[1] = 
				ent->ambientLight[2] = tr.identityLight * 150;
			ent->directedLight[0] = ent->directedLight[1] = 
				ent->directedLight[2] = tr.identityLight * 150;
			VectorCopy( tr.sunDirection, ent->lightDir );
		}
	}

	// bonus items and view weapons have a fixed minimum add
	if ( 1 /* ent->e.renderfx & RF_MINLIGHT */ ) {
		// give everything a minimum light add
		ent->ambientLight[0] += tr.identityLight * 32;
		ent->ambientLight[1] += tr.identityLight * 32;
		ent->ambientLight[2] += tr.identityLight * 32;
	}

	//
	// modify the light by dynamic lights
	//
	d = VectorLength( ent->directedLight );
	VectorScale( ent->lightDir, d, lightDir );

	for ( i = 0 ; i < refdef->num_dlights ; i++ ) {
		dl = &refdef->dlights[i];
		VectorSubtract( dl->origin, lightOrigin, dir );
		d = VectorNormalize( dir );

		power = DLIGHT_AT_RADIUS * ( dl->radius * dl->radius );
		if ( d < DLIGHT_MINIMUM_RADIUS ) {
			d = DLIGHT_MINIMUM_RADIUS;
		}
		d = power / ( d * d );

		VectorMA( ent->directedLight, d, dl->color, ent->directedLight );
		VectorMA( lightDir, d, dir, lightDir );
	}

	// clamp ambient
	for ( i = 0 ; i < 3 ; i++ ) {
		if ( ent->ambientLight[i] > tr.identityLightByte ) {
			ent->ambientLight[i] = tr.identityLightByte;
		}
	}

	if ( r_debugLight->integer ) {
		LogLight( ent );
	}

	// save out the byte packet version
	((byte *)&ent->ambientLightInt)[0] = (int)ent->ambientLight[0];
	((byte *)&ent->ambientLightInt)[1] = (int)ent->ambientLight[1];
	((byte *)&ent->ambientLightInt)[2] = (int)ent->ambientLight[2];
	((byte *)&ent->ambientLightInt)[3] = 0xff;
	
	// transform the direction to local space
	VectorNormalize( lightDir );
	ent->lightDir[0] = DotProduct( lightDir, ent->e.axis[0] );
	ent->lightDir[1] = DotProduct( lightDir, ent->e.axis[1] );
	ent->lightDir[2] = DotProduct( lightDir, ent->e.axis[2] );
}

/*
=================
R_LightForPoint
=================
*/
int Q_EXTERNAL_CALL R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir )
{
	trRefEntity_t ent;
	
	if( !tr.world )
		return qfalse;

	Com_Memset( &ent, 0, sizeof( ent ) );
	VectorCopy( point, ent.e.origin );

	switch( tr.world->lightGridType )
	{
	case LIGHT_GRID:
		R_SetupEntityLightingGrid( &ent );
		break;

	case LIGHT_TREE_16:
	case LIGHT_TREE_32:
		R_SetupEntityLightingTree( &ent );
		break;

	default:
		return qfalse;
	}
		
	VectorCopy(ent.ambientLight, ambientLight);
	VectorCopy(ent.directedLight, directedLight);
	VectorCopy(ent.lightDir, lightDir);

	return qtrue;
}

static ID_INLINE void vec3_to_bytes( byte out[3], const vec3_t in, bool scale )
{
	int c;

	float scale_fac = scale ? (float)0xFF : 1.0F;

	c = (int)(in[0] * scale_fac); if( c < 0 ) c = 0; else if( c > 0xFF ) c = 0xFF; out[0] = (byte)c;
	c = (int)(in[1] * scale_fac); if( c < 0 ) c = 0; else if( c > 0xFF ) c = 0xFF; out[1] = (byte)c;
	c = (int)(in[2] * scale_fac); if( c < 0 ) c = 0; else if( c > 0xFF ) c = 0xFF; out[2] = (byte)c;
}

image_t* R_MakeImage( const char *name );

static image_t* R_LightUploadLightImage( const char *name, int w, int h, int d,
	GLenum uploadFormat, const void *data )
{
	image_t *img = R_MakeImage( "*lightGridAmbient" );
	
	img->uploadWidth = w;
	img->uploadHeight = h;
	img->uploadDepth = d;

	img->width = w;
	img->height = h;
	img->depth = d;

	img->internalFormat = uploadFormat;
	img->hasAlpha = qfalse;

	img->mipmap = qfalse;

	img->texTarget = GL_TEXTURE_3D_EXT;
	img->addrMode = TAM_Normalized;

	R_StateSetTexture( img, GL_TEXTURE0_ARB );
	if( GLEW_EXT_texture3D )
	{
		glTexImage3DEXT( GL_TEXTURE_3D_EXT, 0, uploadFormat, w, h, d,
			0, GL_RGB, GL_UNSIGNED_BYTE, data );
	}
#if GL_VERSION_1_2
	else if( GLEW_VERSION_1_2 )
	{
		glTexImage3D( GL_TEXTURE_3D, 0, uploadFormat, w, h, d,
			0, GL_RGB, GL_UNSIGNED_BYTE, data );
	}
#endif

	R_StateSetTexture( NULL, GL_TEXTURE0_ARB );

	return img;
}
