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

static float projMatrix[ 16 ];
static float viewMatrix[ 16 ];

void myGlMultMatrix( const float *a, const float *b, float *out ) {
	int		i, j;

	for ( i = 0 ; i < 4 ; i++ ) {
		for ( j = 0 ; j < 4 ; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
}

static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

static affine_t s_flipAffine =
{
	{
		{ -1, 0, 0 },
		{ 0, 0, 1 },
		{ 0, 1, 0 }
	},
	{ 0, 0, 0 }
};

static void SetupViewMatrix( vec3_t axis[3], vec3_t origin ) 
{
	float m[ 16 ];
	m[0] = axis[0][0];
	m[4] = axis[0][1];
	m[8] = axis[0][2];
	m[12] = -origin[0] * m[0] + -origin[1] * m[4] + -origin[2] * m[8];

	m[1] = axis[1][0];
	m[5] = axis[1][1];
	m[9] = axis[1][2];
	m[13] = -origin[0] * m[1] + -origin[1] * m[5] + -origin[2] * m[9];

	m[2] = axis[2][0];
	m[6] = axis[2][1];
	m[10] = axis[2][2];
	m[14] = -origin[0] * m[2] + -origin[1] * m[6] + -origin[2] * m[10];

	m[3] = 0;
	m[7] = 0;
	m[11] = 0;
	m[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix( m, s_flipMatrix, viewMatrix );
}

static void SetupProjMatrix( refdef_t * refdef, float projectionMatrix[16]  )
{
	float zNear, zFar, depth;

	zNear = 0.001F;
	zFar = 100000.0F;

	depth = zFar - zNear;

	projectionMatrix[0] = 1.0F / tanf( refdef->fov_x * M_PI / 360.0F );
	projectionMatrix[4] = 0;
	projectionMatrix[8] = 0;
	projectionMatrix[12] = 0;

	projectionMatrix[1] = 0;
	projectionMatrix[5] = 1.0F / tanf( refdef->fov_y * M_PI / 360.0F );
	projectionMatrix[9] = 0;
	projectionMatrix[13] = 0;

	projectionMatrix[2] = 0;
	projectionMatrix[6] = 0;
	projectionMatrix[10] = -(zFar + zNear) / depth;
	projectionMatrix[14] = -2 * zFar * zNear / depth;

	projectionMatrix[3] = 0;
	projectionMatrix[7] = 0;
	projectionMatrix[11] = -1;
	projectionMatrix[15] = 0;
}


static refdef_t refdef;

static void TransformPoint( vec4_t out, const float matrix[16], const vec4_t in )
{
	out[0] = matrix[0] * in[0] + matrix[4] * in[1] + matrix[8] * in[2] + matrix[12];
	out[1] = matrix[1] * in[0] + matrix[5] * in[1] + matrix[9] * in[2] + matrix[13];
	out[2] = matrix[2] * in[0] + matrix[6] * in[1] + matrix[10] * in[2] + matrix[14];
	out[3] = matrix[3] * in[0] + matrix[7] * in[1] + matrix[11] * in[2] + matrix[15];
}

static qboolean SolarSystem_WorldToScreen( const vec3_t p, float r, float *sx, float *sy, float *sz, float *srx, float *sry )
{
	vec4_t vp, pp, rp, tmp;

	tmp[0] = p[0];
	tmp[1] = p[1];
	tmp[2] = p[2];
	tmp[3] = 1.0F;

	TransformPoint( vp, viewMatrix, tmp );
	TransformPoint( pp, projMatrix, vp );

	if ( pp[ 0 ] < -pp[ 3 ] )	return qfalse;
	if ( pp[ 0 ] >  pp[ 3 ] )	return qfalse;
	if ( pp[ 1 ] < -pp[ 3 ] )	return qfalse;
	if ( pp[ 1 ] >  pp[ 3 ] )	return qfalse;
	if ( pp[ 2 ] < -pp[ 3 ] )	return qfalse;

	if( sx )
		*sx = pp[0] / pp[3];
	if( sy )
		*sy = pp[1] / pp[3];
	if ( sz )
		*sz = pp[2] / pp[3];
	
	tmp[0] = vp[0] + r;
	tmp[1] = vp[1] + r;
	tmp[2] = vp[2];
	tmp[3] = 1.0F;

	TransformPoint( rp, projMatrix, tmp );

	if( srx )
		*srx = rp[0] / rp[3] - pp[0] / pp[3];
	if( sry )
		*sry = rp[1] / rp[3] - pp[1] / pp[3];

	return qtrue;
}

float SolarSystem_PlanetTheta( const planetInfo_t *p, float t )
{
	 if ( p->dfs > 0 )
		return DEG2RAD( p->sa ) + (((float)t / p->op) * 2.0F * M_PI);
	else
		return 0;
}

void SolarSystem_PlanetPosition( const planetInfo_t * p, float t, vec3_t pos )
{
    if ( p->dfs > 0 )
    {
		float d = SolarSystem_PlanetTheta( p, t );

		pos[ 0 ] = sinf( d ) * p->dfs * 10.0f;
		pos[ 1 ] = cosf( d ) * p->dfs * 10.0f;
		pos[ 2 ] = 0.0f;

    } else
    {
        pos[ 0 ] = 0.0f;
        pos[ 1 ] = 0.0f;
		pos[ 2 ] = 0.0f;
    }
}

extern float fracf( float f );

static ID_INLINE NOGLOBALALIAS void Vec3_Cpy( vec3_t o, const vec3_t i ) { o[0] = i[0]; o[1] = i[1]; o[2] = i[2]; }

void SolarSystem_LookAtEx( solarsystemEye_t * eye, const planetInfo_t * p, const quat_t ofs, float time )
{
	quat_t q;

	if( p )
	{
		float theta, R;
		vec3_t planet_pos;

		theta = SolarSystem_PlanetTheta( p, time );

		R = p->dfs * 10.0F;

		planet_pos[0] = sinf( theta ) * R;
		planet_pos[1] = cosf( theta ) * R;
		planet_pos[2] = 0;

		Vec3_Cpy( eye->lookAt, planet_pos );
		
		{
			const vec3_t up = { 0, 0, 1 };

			Quat_AxisAngle( q, up, -theta + DEG2RAD( 90 ) );
		}

		eye->fov = DEG2RAD( 45 );
		eye->radius = (p->size * 5.0F) / tanf( 0.5F * eye->fov );
	}
	else
	{
		{
			vec3_t ax = { 0, 0, 1 };
			Quat_AxisAngle( q, ax, M_PI / 2 );
		}

		eye->radius = 50;
		VectorSet( eye->lookAt, 0, 0, 0 );
		eye->fov = DEG2RAD( 45 );
	}

	Quat_Mul( eye->direction, q, ofs );
}

void SolarSystem_LookAt( solarsystemEye_t * eye, const planetInfo_t * p, float time )
{
	if( p )
	{		
		const vec3_t up = { 0, 0, 1 };
		const vec3_t rt = { 0, 1, 0 };

		quat_t qa, qb, qo;

		Quat_AxisAngle( qa, up, DEG2RAD( 60 ) );
		Quat_AxisAngle( qb, rt, DEG2RAD( 20 ) );
		
		Quat_Mul( qo, qa, qb );

		SolarSystem_LookAtEx( eye, p, qo, time );
	}
	else
	{
		quat_t i;
		Quat_Ident( i );

		SolarSystem_LookAtEx( eye, p, i, time );
	}
}

void SolarSystem_TopDown( solarsystemEye_t *eye, const solarsystemInfo_t *ss, float t, const float angles[2], const float ofs[3], bool center_on_planets )
{
	int i;
	float r;

	vec3_t pos, center, delta;

	VectorClear( center );

#if 1
	if( center_on_planets )
	{
		vec3_t min, max;
		bool first;

		first = true;

		for( i = 0; i < ss->planetCount; i++ )
		{
			const planetInfo_t * p = ss->planets + i;
			int j;
			float s;

			if( !p->visible )
				continue;

			SolarSystem_PlanetPosition( p, t, pos );
			s = p->size * 2.0F;

			if( first )
			{
				for( j = 0; j < 3; j++ )
				{
					min[j] = pos[j] - s;
					max[j] = pos[j] + s;
				}

				first = false;
			}
			else
			{
				for( j = 0; j < 3; j++ )
				{
					if( pos[j] - s < min[j] )
						min[j] = pos[j] - s;

					if( pos[j] + s > max[j] )
						max[j] = pos[j] + s;
				}
			}
		}

		if( !first )
		{
			VectorAdd( min, max, center );
			VectorScale( center, 0.5F, center );
		}
	}
#endif

	VectorAdd( center, ofs, center );

	eye->fov = DEG2RAD( 65 );

	r = 0;
	for( i = 0; i < ss->planetCount; i++ )
	{
		const planetInfo_t * p = ss->planets + i;
		float s;

		if( !p->visible )
			continue;

		SolarSystem_PlanetPosition( p, t, pos );

		VectorSubtract( pos, center, delta );

		s = sqrtf( delta[0] * delta[0] + delta[1] * delta[1] ) + p->size * 2;
		if( s > r )
			r = s;
	}

	r *= 1.5F / tanf( 0.5F * eye->fov );

	{
		vec3_t ax;

		ax[0] = sinf( DEG2RAD( angles[0] ) );
		ax[1] = cosf( DEG2RAD( angles[0] ) );
		ax[2] = 0;

		Quat_AxisAngle( eye->direction, ax, DEG2RAD( 90 - angles[1] ) );
	}

	eye->radius = r;
	Vec3_Cpy( eye->lookAt, center );
}

qboolean SolarSystem_GetScreenRect( const itemDef_t * item, const vec3_t pos, float r, rectDef_t * rect, float * z )
{
	float sx, sy, rx, ry;
	float cw = item->window.rect.w * 0.5F;
	float ch = item->window.rect.h * 0.5F;
	float cx = item->window.rect.x + cw;
	float cy = item->window.rect.y + ch;

	if( SolarSystem_WorldToScreen( pos, r, &sx, &sy, z, &rx, &ry ) )
	{
		rect->x = cx + (sx - rx * 0.5F) * cw;
		rect->y = cy - (sy + ry * 0.5F) * ch;
		rect->w = rx * cw;
		rect->h = ry * ch;
		return qtrue;
	}

	return qfalse;
}

void SolarSystem_LerpEye( solarsystemEye_t * out, const solarsystemEye_t * a, const solarsystemEye_t * b, int rd, float t )
{
	Vec3_Lrp( out->lookAt, a->lookAt, b->lookAt, t );
	Quat_SleprEx( out->direction, a->direction, b->direction, t, rd );
	out->radius = a->radius * (1.0F - t) + b->radius * t;
	out->fov = a->fov * (1.0F - t) + b->fov * t;
}

#define PLANET_REST_SCALE 2.0F

void SolarSystem_Draw( solarsystemInfo_t * ss, rectDef_t * r, const planetInfo_t * selected,
	const planetInfo_t *current, float time, float zoomFade )
{
	float			x = r->x;
	float			y = r->y;
	float			w = r->w;
	float			h = r->h;
	int				i;
	vec3_t			eyeDir;
	affine_t		eyeMat;
	refEntity_t		re;
	qboolean		hassun;

	UI_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL | RDF_SOLARLIGHT;

	QuatToAffine( &eyeMat, ss->eye.direction );

	AxisCopy( eyeMat.axis, refdef.viewaxis );

	VectorSet( eyeDir, -1, 0, 0 );
	Affine_MulVec( eyeDir, &eyeMat, eyeDir );
	
	VectorMA( ss->eye.lookAt, ss->eye.radius, eyeDir, refdef.vieworg );
	
	refdef.x = (int)x;
	refdef.y = (int)y;
	refdef.width = (int)w;
	refdef.height = (int)h;

	if( w < h )
	{
		refdef.fov_x = RAD2DEG( ss->eye.fov );
		refdef.fov_y = RAD2DEG( 2 * atanf( (h / w) * tanf( 0.5F * ss->eye.fov ) ) );
	}
	else
	{
		refdef.fov_y = RAD2DEG( ss->eye.fov );
		refdef.fov_x = RAD2DEG( 2 * atanf( (w / h) * tanf( 0.5F * ss->eye.fov ) ) );
	}

	refdef.time = uiInfo.uiDC.realTime;

	trap_R_ClearScene();

	memset( &re, 0, sizeof( refEntity_t ) );
	re.reType = RT_SKYCUBE;
	re.radius = 3184.0f;
	re.customShader = trap_R_RegisterShaderNoMip( "ui/starmap/nightsky" );
	trap_R_AddRefEntityToScene( &re );

	hassun = qfalse;
	for( i = 0; i < ss->planetCount; i++ )
		if( Q_stricmp( ss->planets[i].name, "sun" ) == 0 )
		{
			hassun = qtrue;
			break;
		}

	for( i = 0; i < ss->planetCount; i++ )
	{
		float scale;
		planetInfo_t *p = ss->planets + i;
		
		//skip any invisible planets
		if( !p->visible )
			continue;

		memset( &re, 0, sizeof( refEntity_t ) );

		AxisClear( re.axis );

		SolarSystem_PlanetPosition( p, time, re.origin );
		VectorCopy ( re.origin, re.oldorigin );

		scale = PLANET_REST_SCALE;// + 0.2F * PLANET_REST_SCALE * (p == current ? selScale : 0);

		{
			vec3_t angles;
			//planet sphere
			re.reType = RT_MODEL;
			re.hModel = p->model;
			re.customShader = p->shader;
			angles[ PITCH ] = 0.0f;
			angles[ ROLL ] = 0.0f;
			angles[ YAW ] = fmodf( uiInfo.uiDC.realTime * 0.0025f, 360.0f );
			AnglesToAxis( angles, re.axis );

			if( !hassun )
			{
				re.renderfx |= RF_LIGHTING_ORIGIN;
				VectorSet( re.lightingOrigin, -0.5, -0.5, 0 );
				VectorNormalize( re.lightingOrigin );
			}
			else
				VectorClear( re.lightingOrigin );

			VectorScale( re.axis[0], (float)p->size * scale, re.axis[0] );
			VectorScale( re.axis[1], (float)p->size * scale, re.axis[1] );
			VectorScale( re.axis[2], (float)p->size * scale, re.axis[2] );

			re.nonNormalizedAxes = qtrue;

			trap_R_AddRefEntityToScene( &re );

			if( p->atmShader )
			{
				re.customShader = p->atmShader;
				re.shaderRGBA[3] = 0xFF;// (p == current || !ui_spacetrader.integer) ? 0xFF : 0x7F;

				trap_R_AddRefEntityToScene( &re );
			}
		}

		if( Q_stricmp( p->name, "sun" ) == 0 )
		{
			re.reType = RT_FLARE;
			re.customShader = trap_R_RegisterShaderNoMip( "ui/starmap/sun_flare" );

			re.radius = (float)p->size * scale;

			trap_R_AddRefEntityToScene( &re );
		}

		if( p->dfs > 0 )
		{
			re.reType = RT_ORBIT;
			re.radius = p->dfs * 10.0f;
			re.rotation = DEG2RAD( p->sa ) + (((float)time / p->op) * 2.0f * M_PI) - 0.3F * (float)p->size / (float)p->dfs;
			
			re.renderfx = RF_ORBIT_LINE;
			re.width = 3;
			re.hModel = 0;
			re.shaderRGBA[0] = 127;
			re.shaderRGBA[1] = 127;
			re.shaderRGBA[2] = 127;
			re.shaderRGBA[3] = ui_spacetrader.integer ? 0xFF : 0x70;
			re.lightingOrigin[0] = -50.0F * p->dfs;
			VectorClear( re.origin );
			trap_R_AddRefEntityToScene( &re );
		}

		if( p == selected || p == current )
		{
			re.reType = RT_ORBIT;
			re.radius = -(float)(p->size * scale * 1.3F);
			re.rotation = DEG2RAD( uiInfo.uiDC.realTime * 0.25f );
			re.renderfx = 0;
			re.width = 15;
			re.hModel = 0;
			re.lightingOrigin[0] = (float)p->size * 3.0f * M_PI;

			if( p == selected )
			{
				re.shaderRGBA[0] = 5;
				re.shaderRGBA[1] = 192;
				re.shaderRGBA[2] = 5;
			}
			else
			{
				re.shaderRGBA[0] = 192;
				re.shaderRGBA[1] = 192;
				re.shaderRGBA[2] = 5;
			}

			re.shaderRGBA[3] = (byte)((ui_spacetrader.integer ? 0xFF : 0x70) * zoomFade);
			
			SolarSystem_PlanetPosition( p, time, re.origin );
			trap_R_AddRefEntityToScene( &re );
		}
	}

	SetupViewMatrix( refdef.viewaxis, refdef.vieworg );
	SetupProjMatrix( &refdef, projMatrix );

	trap_R_RenderScene( &refdef );
}


planetInfo_t * SolarSystem_Select( solarsystemInfo_t * ss, const itemDef_t * item, float t, float x, float y )
{
	rectDef_t		rc;
	int				i;
	planetInfo_t *	closestPlanet = 0;
	float			closestDist = 1E6f;

	for ( i = 0; i < ss->planetCount; i++ )
	{
		vec3_t pos;
		float z;

		if( !ss->planets[i].visible || ss->planets[i].decoration )
			//skip this planet
			continue;

		SolarSystem_PlanetPosition( ss->planets + i, t, pos );

		if( SolarSystem_GetScreenRect( item, pos, (float)ss->planets[i].size * PLANET_REST_SCALE * 3 + 10, &rc, &z ) )
		{
			if( Rect_ContainsPoint( &rc, x,y ) || Rect_ContainsPoint( &ss->planets[ i ].textRect, x,y ) )
			{
				if ( z < closestDist )
				{
					closestPlanet	= ss->planets + i;
					closestDist		= z;
				}
			}
		}
	}

	return closestPlanet;
}	



/*
===============
UI_DrawPlanet
===============
*/
void UI_DrawPlanet( solarsystemInfo_t * ss, rectDef_t * r, int planet_id, float time )
{
	float			x = r->x;
	float			y = r->y;
	float			w = r->w;
	float			h = r->h;
	int				i;
	refEntity_t		re;
	float			fov;

	UI_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL | RDF_SOLARLIGHT;

	AxisClear( refdef.viewaxis );

	refdef.vieworg[0] = -10.0f;
	refdef.vieworg[1] = 0.0f;
	refdef.vieworg[2] = 0.0f;
	
	refdef.x = (int)x;
	refdef.y = (int)y;
	refdef.width = (int)w;
	refdef.height = (int)h;

	fov = 0.25f;

	if( w < h )
	{
		refdef.fov_x = RAD2DEG( fov );
		refdef.fov_y = RAD2DEG( 2 * atanf( (h / w) * tanf( 0.5F * fov ) ) );
	}
	else
	{
		refdef.fov_y = RAD2DEG( fov );
		refdef.fov_x = RAD2DEG( 2 * atanf( (w / h) * tanf( 0.5F * fov ) ) );
	}

	refdef.time = uiInfo.uiDC.realTime;

	trap_R_ClearScene();

	memset( &re, 0, sizeof( refEntity_t ) );
	/*
	re.reType = RT_SKYCUBE;
	re.radius = 3184.0f;
	re.customShader = trap_R_RegisterShaderNoMip( "ui/starmap/nightsky" );
	trap_R_AddRefEntityToScene( &re );
	*/

	for( i = 0; i < ss->planetCount; i++ )
	{
		planetInfo_t *p = ss->planets + i;
		
		//skip any invisible planets
		if( p->id != planet_id )
			continue;

		memset( &re, 0, sizeof( refEntity_t ) );

		AxisClear( re.axis );

		SolarSystem_PlanetPosition( p, time, re.origin );
		VectorSet( re.origin, 0.0f, 0.0f, 0.0f );
		VectorCopy ( re.origin, re.oldorigin );

		

		{
			vec3_t angles;
			//planet sphere
			re.reType = RT_MODEL;
			re.hModel = p->model;
			re.customShader = p->shader;
			angles[ PITCH ] = 0.0f;
			angles[ ROLL ] = 0.0f;
			angles[ YAW ] = fmodf( uiInfo.uiDC.realTime * 0.0025f, 360.0f );
			AnglesToAxis( angles, re.axis );

			re.renderfx |= RF_LIGHTING_ORIGIN;
			VectorSet( re.lightingOrigin, -0.5, -0.5, 0 );
			VectorNormalize( re.lightingOrigin );

			trap_R_AddRefEntityToScene( &re );

			if( p->atmShader )
			{
				re.customShader = p->atmShader;
				re.shaderRGBA[3] = 0xFF;// (p == current || !ui_spacetrader.integer) ? 0xFF : 0x7F;

				trap_R_AddRefEntityToScene( &re );
			}
		}
	}

	trap_R_RenderScene( &refdef );
}
