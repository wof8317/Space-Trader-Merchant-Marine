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

#include "cg_local.h"

// cg_weapons.c -- events and effects dealing with weapons

/*
==========================
CG_MachineGunEjectBrass
==========================
*/
static void CG_MachineGunEjectBrass( centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	float			waterScale = 1.0f;
	vec3_t			v[3];
	affine_t		lerped;

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (Q_rand()&15);

	AnglesToAxis( cent->lerpAngles, v );

	//adjust the offset based off where the tag_weapon is.
	trap_R_LerpTag( &lerped, cgs.clientinfo[cent->currentState.number].legsModel, cent->pe.legs.oldFrame, cent->pe.legs.frame, 1.0 - cent->pe.legs.backlerp, "tag_weapon");
	VectorCopy(lerped.origin, offset);
	offset[2] -= 20;
	
	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = Q_rand()&31;
	le->angles.trBase[1] = Q_rand()&31;
	le->angles.trBase[2] = Q_rand()&31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_ShotgunEjectBrass
==========================
*/
static void CG_ShotgunEjectBrass( centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	vec3_t			v[3];
	float			waterScale = 1.0f;
	affine_t		lerped;

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}


	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 60 + 60 * crandom();
	velocity[1] = 40 + 10 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer*3 + cg_brassTime.integer * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time;

	AnglesToAxis( cent->lerpAngles, v );

	//adjust the offset based off where the tag_weapon is.
	trap_R_LerpTag( &lerped, cgs.clientinfo[cent->currentState.number].legsModel, cent->pe.legs.oldFrame, cent->pe.legs.frame, 1.0 - cent->pe.legs.backlerp, "tag_weapon");
	VectorCopy(lerped.origin, offset);
	offset[2] -= 20;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );
	VectorCopy( re->origin, le->pos.trBase );
	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.shotgunBrassModel;
	le->bounceFactor = 0.3f;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = Q_rand()&31;
	le->angles.trBase[1] = Q_rand()&31;
	le->angles.trBase[2] = Q_rand()&31;
	le->angles.trDelta[0] = 1;
	le->angles.trDelta[1] = 0.5;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_SHELL;
	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_RocketTrail
==========================
*/
static void CG_RocketTrail( centity_t *ent, const weaponInfo_t *wi )
{
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;

	if( cg_noProjectileTrail.integer )
		return;

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if( es->pos.trType == TR_STATIONARY )
	{
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	{
		if( contents & lastContents & CONTENTS_WATER )
			CG_BubbleTrail( lastPos, origin, 8 );

		return;
	}

	for( ; t <= ent->trailTime; t += step )
	{
		BG_EvaluateTrajectory( &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up, 
					  wi->trailRadius, 
					  1, 1, 1, 0.33f,
					  wi->wiTrailTime, 
					  t,
					  0,
					  0, 
					  cgs.media.smokePuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}

/*
==========================
CG_GrappleTrail
==========================
*/
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi )
{
	vec3_t	origin;
	entityState_t	*es;
	vec3_t			forward, up;
	refEntity_t		beam;

	REF_PARAM( wi );

	es = &ent->currentState;

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	ent->trailTime = cg.time;

	memset( &beam, 0, sizeof( beam ) );
	//FIXME adjust for muzzle position
	VectorCopy ( cg_entities[ ent->currentState.otherEntityNum ].lerpOrigin, beam.origin );
	beam.origin[2] += 26;
	AngleVectors( cg_entities[ ent->currentState.otherEntityNum ].lerpAngles, forward, NULL, up );
	VectorMA( beam.origin, -6, up, beam.origin );
	VectorCopy( origin, beam.oldorigin );

	if (Distance( beam.origin, beam.oldorigin ) < 64 )
		return; // Don't draw if close

	beam.reType = RT_SPRITE;
	beam.customShader = cgs.media.lightningShader;

	AxisClear( beam.axis );
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	trap_R_AddRefEntityToScene( &beam );
}

/*
	UNREFERENCED LOCAL FUNCTION
static void CG_GrenadeTrail( centity_t *ent, const weaponInfo_t *wi ) {
	CG_RocketTrail( ent, wi );
}
*/


/*
======================
CG_ParseWeaponAnimationFile

Read a configuration file containing animation counts and rates
models/weapons2/sniper/animation.cfg, etc
======================
*/
static qboolean	CG_ParseWeaponAnimationFile( const char *filename, weaponInfo_t *wi ) {
	const char	*text_p;
	int			len;
	char		*token;
	float		fps;
	int			skip;
	int			anim;
	char		text[20000];
	fileHandle_t	f;
	animation_t *animations;

	animations = wi->animations;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning
	for( ; ; )
	{
		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}

		if ( (anim = BG_FindWeaponAnimationByName( token ) ) >= 0 )
		{
			token = COM_Parse( &text_p );
			if ( !*token ) break;
			animations[anim].firstFrame = atoi( token );
			token = COM_Parse( &text_p );
			if ( !*token ) break;
			animations[anim].numFrames = atoi( token );
			token = COM_Parse( &text_p );
			if ( !*token ) break;
			animations[anim].loopFrames = atoi( token );
			token = COM_Parse( &text_p );
			if ( !*token ) break;
			fps = atoi( token );
			if ( fps == 0 ) {
				fps = 1;
			}
			animations[anim].frameLerp = 1000 / fps;
			animations[anim].initialLerp = 1000 / fps;
			
			animations[anim].reversed = qfalse;
			animations[anim].flipflop = qfalse;
			// if numFrames is negative the animation is reversed
			if (animations[anim].numFrames < 0) {
				animations[anim].numFrames = -animations[anim].numFrames;
				animations[anim].reversed = qtrue;
			}
			continue;
		} else if ( !text_p) {
			break;
		}
	}
	return qtrue;
}
static qboolean	CG_ParseWeaponSoundFile( const char *filename, weaponInfo_t *wi ) {
	const char	*text_p;
	int			len;
	int			i;
	char		*token;
	int			skip;
	char		text[20000];
	fileHandle_t	f;
	sfxWeaponAnimationSound_t *sounds;

	sounds = wi->animationSounds;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	// read in the sound info.
	// it's in frame# path_to_sound format.
	// MAX_WEAPON_ANIMATIONS is re-used since we have a 1:1 mapping of animations, to sounds.
	for ( i = 0 ; i < MAX_WEAPON_ANIMATIONS ; i++ ) {

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		sounds[i].frame = atoi( token );

		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		sounds[i].sound = trap_S_RegisterSound(token, qfalse );
		sounds[i].playFrame = -1;
	}
	return qtrue;
}
/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum )
{
	int i;
	vec3_t mins, maxs;
	gitem_t *item, *ammo;
	weaponInfo_t *weaponInfo;
	char path[MAX_QPATH], path2[MAX_QPATH];

	weaponInfo = &cg_weapons[weaponNum];

	if( weaponNum == 0 )
		return;

	if( weaponInfo->registered )
		return;

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for( item = bg_itemlist + 1 ; item->classname ; item++ )
	{
		if( item->giType == IT_WEAPON && item->giTag == weaponNum )
		{
			weaponInfo->item = item;
			break;
		}
	}

	if( !item->classname )
		CG_Error( "Couldn't find weapon %i", weaponNum );
	
	CG_RegisterItemVisuals( item - bg_itemlist );
	if (item->world_model[0]) 
	{
		clientInfo_t *ci;
		strcpy( path, item->world_model[0] );
		COM_StripExtension( path, path, sizeof(path) );
			
		Com_sprintf( path2, sizeof( path2 ), "%s.x42", path );
		weaponInfo->weaponModel = trap_R_RegisterModel( path2 );
		if( weaponInfo->weaponModel )
			weaponInfo->weaponModelType = MT_X42;
		else
		{
			// load cmodel before model so filecache works
			weaponInfo->weaponModel = trap_R_RegisterModel( item->world_model[0] );
			weaponInfo->weaponModelType = MT_MD3;
		}
	
		// calc midpoint for rotation
		trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
		for( i = 0 ; i < 3 ; i++ )
			weaponInfo->weaponMidpoint[i] = mins[i] + 0.5F * ( maxs[i] - mins[i] );
	
		weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );
		weaponInfo->ammoIcon = trap_R_RegisterShader( item->icon );
	
		for( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ )
			if( ammo->giType == IT_AMMO && ammo->giTag == weaponNum )
				break;
	
		if( ammo->classname && ammo->world_model[0] )
			weaponInfo->ammoModel = trap_R_RegisterModel( ammo->world_model[0] );
	
		Com_sprintf( path2, sizeof( path2 ), "%s_flash.x42", path );
		weaponInfo->flashModel = trap_R_RegisterModel( path2 );
		if( weaponInfo->flashModel )
			weaponInfo->flashModelType = MT_X42;
		else
		{
			Com_sprintf( path2, sizeof( path2 ), "%s_flash.md3", path );
			weaponInfo->flashModel = trap_R_RegisterModel( path2 );
			weaponInfo->flashModelType = MT_MD3;
		}

		ci = &cgs.clientinfo[ cg.clientNum ];
		if (ci->gender != GENDER_FEMALE) {
			Com_sprintf( path2, sizeof( path2 ), "%s_1st_male.x42", path );
		} else {
			Com_sprintf( path2, sizeof( path2 ), "%s_1st.x42", path );
		}
		weaponInfo->firstpersonModel = trap_R_RegisterModel( path2 );
		if( weaponInfo->firstpersonModel )
			weaponInfo->firstpersonModelType = MT_X42;
		else
		{
			Com_sprintf( path2, sizeof( path2 ), "%s_1st.md3", path );
			weaponInfo->firstpersonModel = trap_R_RegisterModel( path2 );
			weaponInfo->firstpersonModelType = MT_MD3;
		}
	
		weaponInfo->loopFireSound = qfalse;
		
		//Parse the config file for the weapons animations
		strcpy( path, item->world_model[0] );
		
		COM_StripExtension( path, path, sizeof(path) );
		for( i = strlen( path ); i >= 0; i-- )
		if( path[i] == '/' )
			break;
		if (ci->gender != GENDER_FEMALE) {
			weaponInfo->customSkin = trap_R_RegisterSkin( va( "models/weapons/%s/%s_male.skin", path + i + 1, path + i + 1 ) );
		} else {
			weaponInfo->customSkin = trap_R_RegisterSkin( va( "models/weapons/%s/%s.skin", path + i + 1, path + i + 1 ) );
		}
		
		CG_ParseWeaponAnimationFile( va( "models/weapons/%s/animation.cfg", path + i + 1 ), weaponInfo );
		CG_ParseWeaponSoundFile( va( "models/weapons/%s/sounds.cfg", path + i + 1 ), weaponInfo );
	}
	switch( weaponNum )
	{
	case WP_GAUNTLET:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/melee/melee.ogg", qfalse );
		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->missileModel = trap_R_RegisterModel( "models/ammo/rocket/rocket.md3" );
		weaponInfo->missileTrailFunc = CG_GrappleTrail;
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;
		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		break;

	case WP_PISTOL:
		MAKERGB( weaponInfo->flashDlightColor, 0xFF/255.0f, 0xC2/255.0f, 0x1E/255.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/machinegun/machgf1b.ogg", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;

	case WP_SHOTGUN:
		MAKERGB( weaponInfo->flashDlightColor, 0xFF/255.0f, 0xC2/255.0f, 0x1E/255.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/shotgun/sshotf1b.ogg", qfalse );
		weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrass;
		break;

	case WP_C4:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weapons/c4/c4.x42" );
		weaponInfo->customSkin = trap_R_RegisterSkin("models/weapons/c4/c4.skin" );
		//weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/c4/wstbfire.ogg", qfalse );
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader( "grenadeExplosion" );
		break;


	case WP_GRENADE:
		weaponInfo->missileModel = trap_R_RegisterModel( "models/weapons/grenades/grenades.md3" );
		//weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/grenade/grenlf1a.ogg", qfalse );
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader( "grenadeExplosion" );
		break;

	case WP_ASSAULT:
		MAKERGB( weaponInfo->flashDlightColor, 0xFF/255.0f, 0xC2/255.0f, 0x1E/255.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/assault/fire1.ogg", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;

	case WP_HIGHCAL:
		MAKERGB( weaponInfo->flashDlightColor, 0xFF/255.0f, 0xC2/255.0f, 0x1E/255.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/highcal/fire1.ogg", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;
	
	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.ogg", qfalse );
		break;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum )
{
	itemInfo_t *itemInfo;
	gitem_t *item;
	
	if( itemNum < 0 || itemNum >= bg_numItems )
		CG_Error( "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );

	itemInfo = cg_items + itemNum ;
	if( itemInfo->registered )
		return;

	item = &bg_itemlist[itemNum];
	
	memset( itemInfo, 0, sizeof( &itemInfo ) );
	itemInfo->registered = qtrue;

	if( item->world_model[0] )
	{
		itemInfo->models[0] = trap_R_RegisterModel( item->world_model[0] );
		if( !itemInfo->models[0] )
		{
			char p0[MAX_QPATH], p1[MAX_QPATH];
			COM_StripExtension( item->world_model[0], p0, sizeof( p0 ) );
			Com_sprintf( p1, sizeof( p1 ), "%s.x42", p0 );

			itemInfo->models[0] = trap_R_RegisterModel( p1 );
		}
	}

	if( item->icon )
		itemInfo->icon = trap_R_RegisterShader( item->icon );

	if( item->giType == IT_WEAPON )
		CG_RegisterWeapon( item->giTag );

	// powerups have an accompanying ring or sphere
	if( (item->giType == IT_HEALTH || 
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE) &&
		item->world_model[1] )
	{
		itemInfo->models[1] = trap_R_RegisterModel( item->world_model[1] );

		if( !itemInfo->models[1] )
		{
			char p0[MAX_QPATH], p1[MAX_QPATH];
			COM_StripExtension( item->world_model[1], p0, sizeof( p0 ) );
			Com_sprintf( p1, sizeof( p1 ), "%s.x42", p0 );

			itemInfo->models[1] = trap_R_RegisterModel( p1 );
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame ) {

    //Scott: commented this code out since we have a reload animation to play
	// change weapon
	/*if ( frame >= ci->animations[TORSO_DROP].firstFrame 
		&& frame < ci->animations[TORSO_DROP].firstFrame + 9 ) {
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;
	}*/

	// stand attack
	if ( frame >= ci->animations[TORSO_ATTACK].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if ( frame >= ci->animations[TORSO_ATTACK2].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK2].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	}
	
	return 0;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange*0.25 * 
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin( cg.time * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}

/*
	UNREFERENCED LOCAL FUNCTION
#define		SPIN_SPEED	0.9
#define		COAST_TIME	1000
static float	CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
	}

	return angle;
}
*/

/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups )
{
	REF_PARAM( powerups );

	// add powerup effects
	trap_R_AddRefEntityToScene( gun );
}
/*
========================
CG_WeaponAnimationSound
  Checks the array of sounds, and starts playing that sound if it is supposed to.
========================
*/

static void CG_WeaponAnimationSound( centity_t *cent, weaponInfo_t *weapon)
{
	int i;
	for (i=0; i < MAX_WEAPON_ANIMATIONS; i++)
	{
		if (weapon->animationSounds[i].sound )
		{
			if ( cent->pe.weapon.oldFrame < weapon->animationSounds[i].frame && cent->pe.weapon.frame >= weapon->animationSounds[i].frame && (cent->pe.weapon.frame - cent->pe.weapon.oldFrame < 10)  )
			{
				if ( cent->pe.weapon.frame != weapon->animationSounds[i].playFrame )
				{
					trap_S_StartLocalSound(weapon->animationSounds[i].sound, CHAN_WEAPON);
					weapon->animationSounds[i].playFrame = cent->pe.weapon.frame;
				}
			} else {
				weapon->animationSounds[i].playFrame = -1;
			}
		}
	}
}

/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team )
{
	refEntity_t gun;
	refEntity_t gun2;
	refEntity_t flash;
	refEntity_t c4;
	vec3_t angles;
	weapon_t weaponNum;
	weaponInfo_t *weapon;
	centity_t *nonPredictedCent;

	REF_PARAM( team );

	weaponNum = cent->currentState.weapon;

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	memset( &gun2, 0, sizeof( gun2 ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

    if( weapon->customSkin )
        gun.customSkin = weapon->customSkin;

	// set custom shading for railgun refire rate
	if( ps )
	{
		gun.shaderRGBA[0] = 255;
		gun.shaderRGBA[1] = 255;
		gun.shaderRGBA[2] = 255;
		gun.shaderRGBA[3] = 255;

		gun.hModel = weapon->firstpersonModel ? weapon->firstpersonModel : weapon->weaponModel;
	}
	else
	   gun.hModel = weapon->weaponModel;
		
	if( !gun.hModel )
		return;

	if( !ps )
	{
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if( (cent->currentState.eFlags & EF_FIRING) && weapon->firingSound )
		{
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
			cent->pe.lightningFiring = qtrue;
		}
		else if( weapon->readySound )
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
	}

	if( !ps ) 
		CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon", qtrue );
	else
	{
		animGroupFrame_t groups;
		animGroupFrame_t groups2;
		animGroupTransition_t transition;

		CG_WeaponAnimation( cent, &gun, &gun2 );

		groups.animGroup = 0;
		groups.frame0 = gun.oldframe;
		groups.frame1 = gun.frame;
		groups.interp = 1.0F - gun.backlerp;
		groups.wrapFrames = gun.renderfx & RF_WRAP_FRAMES;

		//blend groups
		if (cent->pe.weaponTransTime > 0 )
		{
			groups2.animGroup = 0;
			groups2.frame0 = gun2.oldframe;
			groups2.frame1 = gun2.frame;
			groups2.interp = 1.0F - gun2.backlerp;
			groups2.wrapFrames = gun2.renderfx & RF_WRAP_FRAMES;

			transition.animGroup = 0;
			transition.interp = (float)(ANIM_TRANSITION_TIME - cent->pe.weaponTransTime) / ANIM_TRANSITION_TIME;
		} else {
			groups2.animGroup = 0;
			groups2.frame0 = gun.oldframe;
			groups2.frame1 = gun.frame;
			groups2.interp = 1.0F - gun.backlerp;
			groups2.wrapFrames = gun.renderfx & RF_WRAP_FRAMES;
		}

		if ( cent->pe.weaponTransTime > 0 )
		{
			gun.poseData = trap_R_BuildPose3( gun.hModel, &groups2, &groups, &transition );

			cent->pe.weaponTransTime -= cg.frametime;
		} else {
			gun.poseData = trap_R_BuildPose( gun.hModel, &groups, 1, NULL, 0 );
		}
		
		CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon", qfalse );
		CG_WeaponAnimationSound( cent, weapon );
	}

	// don't draw the gun while zoomed in
	if ( !(ps && ps->weapon == WP_ASSAULT && cg.snap->ps.stats[STAT_MODE] == qtrue) ) {
		CG_AddWeaponWithPowerups( &gun, cent->currentState.shields );
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( (nonPredictedCent - cg_entities) != cent->currentState.clientNum )
		nonPredictedCent = cent;
	
	if (ps && weaponNum == WP_C4 && ps->stats[STAT_AMMO] != 0 ) {
		memset( &c4, 0, sizeof( c4 ) );

		if( gun.renderfx & RF_LIGHTING_ORIGIN )
		{
			VectorCopy( gun.lightingOrigin, c4.lightingOrigin );
			c4.renderfx = gun.renderfx;
		}
		else
		{
			VectorCopy( gun.origin, c4.lightingOrigin );
			c4.renderfx = gun.renderfx | RF_LIGHTING_ORIGIN;
		}

		c4.shadowPlane = gun.shadowPlane;
		c4.customSkin = weapon->customSkin;
		c4.hModel = weapon->weaponModel;
		if (!c4.hModel) {
			return;
		}

		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = crandom() * 10;
		AnglesToAxis( angles, flash.axis );
		
		CG_PositionEntityOnTag( &c4,&gun, weapon->firstpersonModel, "tag_c4", qfalse );
		c4.nonNormalizedAxes = qtrue;
		
		trap_R_AddRefEntityToScene( &c4 );
	}


	// add the flash
	if( weaponNum != WP_GRAPPLING_HOOK || !(nonPredictedCent->currentState.eFlags & EF_FIRING) )
	{
		// impulse flash
		if( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME && !cent->pe.railgunFlash )
			return;
	}

	memset( &flash, 0, sizeof( flash ) );
	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	if (!flash.hModel) {
		return;
	}
	
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis( angles, flash.axis );

    if( ps )
        CG_PositionRotatedEntityOnTag( &flash, 0, &gun, weapon->firstpersonModel, "tag_flash" );
    else
        CG_PositionRotatedEntityOnTag( &flash, 0, &gun, weapon->weaponModel, "tag_flash" );
    
	trap_R_AddRefEntityToScene( &flash );

	if( ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum )
	{
		if( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] )
			trap_R_AddAdditiveLightToScene( flash.origin, 300 + (Q_rand() & 31), weapon->flashDlightColor[0],
				weapon->flashDlightColor[1], weapon->flashDlightColor[2] );
			//trap_R_AddLightToScene( flash.origin, 300 + (Q_rand() & 31), weapon->flashDlightColor[0],
				//weapon->flashDlightColor[1], weapon->flashDlightColor[2] );
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t	hand;
	centity_t	*cent;
	clientInfo_t	*ci;
	float		fovOffset;
	vec3_t		angles;
	weaponInfo_t	*weapon;
    float fudge_x, fudge_y, fudge_z;

	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// no gun if in third person view or a camera is active
	//if ( cg.renderingThirdPerson || cg.cameraMode) {
	if ( cg.renderingThirdPerson ) {
		return;
	}

	if ( cg_cubemap.integer != 0 )
	{
		return;
	}

	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
		vec3_t		origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			// special hack for lightning gun...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	// drop gun lower at higher fov
	if ( cg_fov.integer > 90 ) {
		fovOffset = -0.2 * ( cg_fov.integer - 90 );
	} else {
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );
    
    //FIXME: TODO: ETC: this needs to be updated once we get better player animations in.
    switch(ps->weapon){
    case WP_HIGHCAL:
        fudge_x = 14.0f; fudge_y = -1.0f; fudge_z = 2.0f;
        break;
    case WP_SHOTGUN:
        fudge_x = 11.0f; fudge_y = -2.0f; fudge_z = -1.0f;
        break;
    case WP_ASSAULT:
        fudge_x = 11.0f; fudge_y = fudge_z = -1.0f;
        break;
    case WP_PISTOL:
        fudge_x = 19.0f; fudge_y = -2.0f; fudge_z = -1.0f;
        break;
    default:
        fudge_x = 4.0f; fudge_y = 0.0f; fudge_z = -1.0f;
        break;
    }
	fudge_x -= 2.0f;
	fudge_y -= 2.0f;
	fudge_z -= 7.0f;

	VectorMA( hand.origin, (cg_gun_x.value+fudge_x), cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, (cg_gun_y.value+fudge_y), cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gun_z.value+fovOffset+fudge_z), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	// map torso animations to weapon animations
	if ( cg_gun_frame.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	} else {
		// get clientinfo for animation map
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
		hand.backlerp = cent->pe.torso.backlerp;
	}

	//hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM] );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void ) {
	int		i;
	int		bits;
	int		count;
	int		x, y;
	char	*name;
	float	*color;

	if( cgs.gametype == GT_HUB )
		return;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
	if ( !color ) {
		return;
	}
	trap_R_SetColor( color );

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of weapons owned
	bits = cg.snap->ps.stats[ STAT_WEAPONS ];
	count = 0;
	for ( i = 1 ; i < 16 ; i++ ) {
		if ( bits & ( 1 << i ) ) {
			count++;
		}
	}

	x = 320 - count * 20;
	y = 380;

	for ( i = 1 ; i < 16 ; i++ ) {
		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}
		
		CG_RegisterWeapon( i );
		
		// can't select grenades directly.
		if (i == WP_GRENADE || i == WP_GAUNTLET) continue;

		// draw weapon icon
		CG_DrawPic( x, y, 32, 32, cg_weapons[i].weaponIcon );

		// draw selection marker
		if ( i == cg.weaponSelect ) {
			CG_DrawPic( x-4, y-4, 40, 40, cgs.media.selectShader );
		}
		// no ammo cross on top  
		/*if ( !(cg.snap->ps.stats[ STAT_AMMO_LEFT ] & ( 1 << i ) ) ) {  
			CG_DrawPic( x, y, 32, 32, cgs.media.noammoShader );  
		}*/
		x += 40;
	}

	// draw the selected name
	if ( cg_weapons[ cg.weaponSelect ].item )
	{
		name = cg_weapons[ cg.weaponSelect ].item->pickup_name;
		if ( sql( "SELECT index FROM strings SEARCH name $;", "t", name ) )
		{
			rectDef_t r;
			char t[ 255 ];
			r.x = 0.0f;
			r.y = y-22.0f;
			r.w = 640.0f;
			r.h = 22.0f;

			trap_SQL_Run( t, sizeof(t), sqlint(0), 0,0,0 );
			sqldone();

			trap_R_RenderText(	&r,
								0.26f,
								color,
								t,
								-1,
								TEXT_ALIGN_CENTER,
								TEXT_ALIGN_CENTER,
								TEXT_STYLE_SHADOWED,
								cgs.media.font,
								-1,
								0 );
		}
	}

	trap_R_SetColor( NULL );
}


/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {
	if ( cg.predictedPlayerState.weaponstate == WEAPON_HOLSTERED ) return qfalse;
/*	if ( ! (cg.snap->ps.stats[ STAT_AMMO_LEFT] & ( 1 << i ) ) ) {
		return qfalse;
	}*/
	if ( ! (cg.snap->ps.stats[ STAT_WEAPONS ] & ( 1 << i ) ) ) {
		return qfalse;
	}
	//Can't directly select grenades
	if ( i == WP_GRENADE || i == WP_GAUNTLET ) return qfalse;

	return qtrue;
}

/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < 16 ; i++ ) {
		cg.weaponSelect++;
		if ( cg.weaponSelect == 16 ) {
			cg.weaponSelect = 0;
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == 16 ) {
		cg.weaponSelect = original;
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < 16 ; i++ ) {
		cg.weaponSelect--;
		if ( cg.weaponSelect == -1 ) {
			cg.weaponSelect = 15;
		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == 16 ) {
		cg.weaponSelect = original;
	}
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > 15 ) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) ) {
		return;		// don't have the weapon
	}
	//Can't select grenades, they get thrown offhand.
	if ( num == WP_GRENADE || num == WP_GAUNTLET) return;
	cg.weaponSelect = num;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( void ) {
	int		i;

	if ( cg.snap->ps.weapon == WP_C4)
	{
		trap_SendClientCommand("mode");
		return;
	}

	if (cg.snap->ps.ammo[cg.snap->ps.weapon] > 0 && cg_autoreload.integer)
	{
		trap_SendClientCommand("reload");
		return;
	}
	if (cg_autoswitch.integer)
	{
		cg.weaponSelectTime = cg.time;
	
		for ( i = 15 ; i > 0 ; i-- ) {
			if ( CG_WeaponSelectable( i ) ) {
				cg.weaponSelect = i;
				break;
			}
		}
	}
}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent ) {
	entityState_t *ent;
	int				c;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	// unless it's a grenade, then never do a flash
	if ( ent->weapon != WP_GRENADE )
		cent->muzzleFlashTime = cg.time;

	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !weap->flashSound[c] ) {
			break;
		}
	}
	if ( c > 0 ) {
		c = Q_rand() % c;
		if ( weap->flashSound[c] )
		{
			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
		}
	}

	// do brass ejection
	if ( weap->ejectBrassFunc && cg_brassTime.integer > 0 ) {
		weap->ejectBrassFunc( cent );
	}
}


/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType )
{
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vec3_t			lightColor;
	localEntity_t	*le;
	int				r;
	qboolean		isSprite;
	int				duration;

	REF_PARAM( clientNum );
	REF_PARAM( soundType );

	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch ( weapon ) {
	default:
	case WP_C4:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_proxexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
	case WP_GRENADE:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_grenexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		CG_BigExplode(origin, cgs.media.shrapnelModel);

		break;
	case WP_ASSAULT:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		radius = 16;
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		duration = 200;
		sfx = 0;
		radius = 4;
		break;
	case WP_PISTOL:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		duration = 200;
		r = Q_rand() & 3;
		if ( r == 0 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 1 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;
	case WP_HIGHCAL:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;

		r = Q_rand() & 3;
		if ( r == 0 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 1 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;
	}

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir, 
							   mod,	shader,
							   duration, isSprite );
		le->light = light;
		VectorCopy( lightColor, le->lightColor );
	}

	//
	// impact mark
	//
	CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, qfalse, radius, qfalse );
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum ) {
	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_GRENADE:
	case WP_C4:
		CG_MissileHitWall( weapon, 0, origin, dir, IMPACTSOUND_FLESH );
		break;
	default:
		break;
	}
}



/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
================
CG_ShotgunPellet
================
*/
static void CG_ShotgunPellet( vec3_t start, vec3_t end, int skipNum ) {
	trace_t		tr;
	int sourceContentType, destContentType;

	CG_Trace( &tr, start, NULL, NULL, end, skipNum, MASK_SHOT );

	sourceContentType = trap_CM_PointContents( start, 0 );
	destContentType = trap_CM_PointContents( tr.endpos, 0 );

	// FIXME: should probably move this cruft into CG_BubbleTrail
	if ( sourceContentType == destContentType ) {
		if ( sourceContentType & CONTENTS_WATER ) {
			CG_BubbleTrail( start, tr.endpos, 32 );
		}
	} else if ( sourceContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( start, trace.endpos, 32 );
	} else if ( destContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( tr.endpos, trace.endpos, 32 );
	}

	if (  tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	if ( cg_entities[tr.entityNum].currentState.eType == ET_PLAYER ) {
		CG_MissileHitPlayer( WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum );
	} else {
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// SURF_NOIMPACT will not make a flame puff or a mark
			return;
		}
		if ( tr.surfaceFlags & SURF_METALSTEPS ) {
			CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL );
		} else {
			CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT );
		}
	}
}

/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/
static void CG_ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, int otherEntNum ) {
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// generate the "random" spread pattern
	for ( i = 0 ; i < DEFAULT_SHOTGUN_COUNT ; i++ ) {
		r = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
		u = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
		VectorMA( origin, 8192 * 16, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);

		CG_ShotgunPellet( origin, end, otherEntNum );
	}
}

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire( entityState_t *es ) {
	vec3_t up, v;
	int contents;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );
	
	contents = trap_CM_PointContents( es->pos.trBase, 0 );
	if( !(contents & CONTENTS_WATER) )
	{
		VectorSet( up, 0, 0, 8 );
		CG_SmokePuff( v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
	}

	CG_ShotgunPattern( es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum );
}

/*
============================================================================

BULLETS

============================================================================
*/


/*
===============
CG_Tracer
===============
*/
void CG_Tracer( vec3_t source, vec3_t dest ) {
	vec3_t		forward, right;
	polyVert_t	verts[4];
	vec3_t		line;
	float		len, begin, end;
	vec3_t		start, finish;
	vec3_t		midpoint;

	// tracer
	VectorSubtract( dest, source, forward );
	len = VectorNormalize( forward );

	// start at least a little ways from the muzzle
	if ( len < 100 ) {
		return;
	}
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if ( end > len ) {
		end = len;
	}
	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( finish, cg_tracerWidth.value, right, verts[0].xyz );
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA( finish, -cg_tracerWidth.value, right, verts[1].xyz );
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA( start, -cg_tracerWidth.value, right, verts[2].xyz );
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA( start, cg_tracerWidth.value, right, verts[3].xyz );
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );

	midpoint[0] = ( start[0] + finish[0] ) * 0.5;
	midpoint[1] = ( start[1] + finish[1] ) * 0.5;
	midpoint[2] = ( start[2] + finish[2] ) * 0.5;

	// add the tracer sound
	trap_S_StartSound( midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound );

}


/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean	CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum ) {
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t		start;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if ( sourceEntityNum >= 0 && cg_tracerChance.value > 0 ) {
		if ( CG_CalcMuzzlePoint( sourceEntityNum, start ) ) {
			sourceContentType = trap_CM_PointContents( start, 0 );
			destContentType = trap_CM_PointContents( end, 0 );

			// do a complete bubble trail if necessary
			if ( ( sourceContentType == destContentType ) && ( sourceContentType & CONTENTS_WATER ) ) {
				CG_BubbleTrail( start, end, 32 );
			}
			// bubble trail from water into air
			else if ( ( sourceContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( start, trace.endpos, 32 );
			}
			// bubble trail from air into water
			else if ( ( destContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( trace.endpos, end, 32 );
			}

			// draw a tracer
			if ( random() < cg_tracerChance.value ) {
				CG_Tracer( start, end );
			}
		}
	}

	// impact splash and mark
	if ( flesh ) {
		CG_Bleed( end, fleshEntityNum );
	} else {
		CG_MissileHitWall( WP_PISTOL, 0, end, normal, IMPACTSOUND_DEFAULT );
	}

}
