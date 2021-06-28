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

#include "ui_local.h"


#define UI_TIMER_GESTURE		2300
#define UI_TIMER_JUMP			1000
#define UI_TIMER_LAND			130
#define UI_TIMER_WEAPON_SWITCH	300
#define UI_TIMER_ATTACK			500
#define	UI_TIMER_MUZZLE_FLASH	20
#define	UI_TIMER_WEAPON_DELAY	250

#define JUMP_HEIGHT				56.0f

#define SWINGSPEED				0.3f

#define SPIN_SPEED				0.9f
#define COAST_TIME				1000


static int			dp_realtime;
static float		jumpHeight;
sfxHandle_t weaponChangeSound;


/*
===============
UI_PlayerInfo_SetWeapon
===============
*/
static void UI_PlayerInfo_SetWeapon( playerInfo_t *pi, weapon_t weaponNum ) {
	gitem_t *	item;
	char		path[MAX_QPATH];

	pi->currentWeapon = weaponNum;
tryagain:
	pi->realWeapon = weaponNum;
	pi->weaponModel = 0;
	pi->barrelModel = 0;
	pi->flashModel = 0;

	if ( weaponNum == WP_NONE ) {
		return;
	}

	for ( item = bg_itemlist + 1; item->classname ; item++ ) {
		if ( item->giType != IT_WEAPON ) {
			continue;
		}
		if ( item->giTag == weaponNum ) {
			break;
		}
	}

	if ( item->classname ) {
		pi->weaponModel = trap_R_RegisterModel( item->world_model[0] );
	}

	if( pi->weaponModel == 0 ) {
		if( weaponNum == WP_PISTOL ) {
			weaponNum = WP_NONE;
			goto tryagain;
		}
		weaponNum = WP_PISTOL;
		goto tryagain;
	}

	if ( weaponNum == WP_PISTOL || weaponNum == WP_GAUNTLET ) {
		strcpy( path, item->world_model[0] );
		COM_StripExtension( path, path, sizeof(path) );
		strcat( path, "_barrel.md3" );
		pi->barrelModel = trap_R_RegisterModel( path );
	}

	strcpy( path, item->world_model[0] );
	COM_StripExtension( path, path, sizeof(path) );
	strcat( path, "_flash.md3" );
	pi->flashModel = trap_R_RegisterModel( path );

	switch( weaponNum ) {
	case WP_GAUNTLET:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;

	case WP_PISTOL:
		MAKERGB( pi->flashDlightColor, 1, 1, 0 );
		break;

	case WP_SHOTGUN:
		MAKERGB( pi->flashDlightColor, 1, 1, 0 );
		break;

	case WP_GRENADE:
		MAKERGB( pi->flashDlightColor, 1, 0.7f, 0.5f );
		break;

    case WP_HIGHCAL:
        MAKERGB( pi->flashDlightColor, 1, 1, 0 );
        break;

	case WP_ASSAULT:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;

	default:
		MAKERGB( pi->flashDlightColor, 1, 1, 1 );
		break;
	}
}


/*
===============
UI_ForceLegsAnim
===============
*/
static void UI_ForceLegsAnim( playerInfo_t *pi, int anim ) {
	pi->legsAnim = anim;

	if ( anim == LEGS_JUMP ) {
		pi->legsAnimationTimer = UI_TIMER_JUMP;
	}
}

/*
===============
UI_ForceTorsoAnim
===============
*/
static void UI_ForceTorsoAnim( playerInfo_t *pi, int anim ) {
	pi->torsoAnim = anim;

	if ( anim == TORSO_GESTURE ) {
		pi->torsoAnimationTimer = UI_TIMER_GESTURE;
	}

	if ( anim == TORSO_ATTACK || anim == TORSO_ATTACK2 ) {
		pi->torsoAnimationTimer = UI_TIMER_ATTACK;
	}
}


/*
	UNREFERENCED LOCAL FUNCTION
static void UI_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							clipHandle_t parentModel, char *tagName ) {
	int				i;
	affine_t	lerped;
	
	// lerp the tag
	trap_CM_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0f - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// cast away const because of compiler problems
	MatrixMultiply( lerped.axis, ((refEntity_t*)parent)->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}
*/

/*
======================
UI_PositionRotatedEntityOnTag
======================
*/
static void UI_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							clipHandle_t parentModel, char *tagName ) {
	int				i;
	affine_t		lerped;
	vec3_t			tempAxis[3];

	// lerp the tag
	trap_CM_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0f - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// cast away const because of compiler problems
	MatrixMultiply( entity->axis, ((refEntity_t *)parent)->axis, tempAxis );
	MatrixMultiply( lerped.axis, tempAxis, entity->axis );
}


/*
===============
UI_SetLerpFrameAnimation
===============
*/
static void UI_SetLerpFrameAnimation( playerInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;

	if ( newAnimation < 0 || newAnimation >= MAX_ANIMATIONS ) {
		trap_Error( ERR_DROP, va("Bad animation number: %i", newAnimation) );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
}


/*
===============
UI_RunLerpFrame
===============
*/
static qboolean UI_RunLerpFrame( playerInfo_t *ci, lerpFrame_t *lf ) {
	int			f;
	animation_t	*anim;

	REF_PARAM( ci );

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( dp_realtime >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( dp_realtime < lf->animationTime ) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		if ( f >= anim->numFrames ) {
			f -= anim->numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				return qfalse;/*
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = dp_realtime;*/
			}
		}
		lf->frame = anim->firstFrame + f;
		if ( dp_realtime > lf->frameTime ) {
			lf->frameTime = dp_realtime;
		}
	}

	if ( lf->frameTime > dp_realtime + 200 ) {
		lf->frameTime = dp_realtime;
	}

	if ( lf->oldFrameTime > dp_realtime ) {
		lf->oldFrameTime = dp_realtime;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0.0f;
	} else {
		lf->backlerp = 1.0f - (float)( dp_realtime - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
	return qtrue;
}
static int UI_BlendPlayerAnimation( playerInfo_t *pi, animNumber_t newAnimation, animNumber_t oldAnimation, lerpFrame_t *current, lerpFrame_t *new)
{
	// see if the animation sequence is switching
	if ( newAnimation != oldAnimation || !current->animation) {
		//CG_Printf("[NOTICE]Blending from %d to %d\n", oldAnimation, newAnimation);
		if (current->animation)
		{
			Com_Memcpy(new, current, sizeof(lerpFrame_t) );
			UI_SetLerpFrameAnimation( pi, current, newAnimation );
		} else {
			//transition from null animation to first animation.
			UI_SetLerpFrameAnimation( pi, current, newAnimation );
			Com_Memcpy(new, current, sizeof(lerpFrame_t) );
		}
		return ANIM_TRANSITION_TIME;

	}
	return 0;
}
static animNumber_t UI_AnimationPriorities( playerInfo_t *pi, animNumber_t anim ) {
	if ( pi->idleTimer <= uiInfo.uiDC.realTime ) {
		pi->idleTimer = uiInfo.uiDC.realTime + 5000 + Q_rrand(0,3000);
		if ( anim == LEGS_IDLE ) {
			switch ( Q_rand() % 3 ){
				case 0:
					return LEGS_IDLE2;
				case 1:
					return LEGS_IDLE3;
				case 2:
					return LEGS_IDLE4;
				default:
					return LEGS_IDLE;
			}
		
		} else {
			return LEGS_IDLE;
		}
	}
	return anim;
}

/*
===============
UI_PlayerAnimation
===============
*/
static void UI_PlayerAnimation( playerInfo_t *pi, refEntity_t *legs, refEntity_t *legs2, refEntity_t *torso, refEntity_t *torso2 ) {
	float			speedScale;
	animNumber_t	newAnimation;

	speedScale = 1.0f;

	newAnimation = UI_AnimationPriorities( pi, pi->legs.animationNumber );
	if ( pi->legsAnimationTimer <= 0 )
		pi->legsAnimationTimer = UI_BlendPlayerAnimation( pi, newAnimation, pi->legs.animationNumber, &pi->legs, &pi->legs2 );

	if (UI_RunLerpFrame( pi, &pi->legs ) == qfalse)
	{
		if ( !(pi->legs.animationNumber >= BOTH_DEATH1 && pi->legs.animationNumber <= BOTH_DEAD3) ) {
			int default_anim = LEGS_IDLE;//UI_AnimationPriorities( pi, pi->legs.animationNumber );
			if ( pi->legsAnimationTimer <= 0 )
			{
				pi->legsAnimationTimer = UI_BlendPlayerAnimation( pi, default_anim, pi->legs.animationNumber, &pi->legs, &pi->legs2  );
				UI_RunLerpFrame( pi, &pi->legs );	
			}
		}
	}
	legs->oldframe = pi->legs.oldFrame;
	legs->frame = pi->legs.frame;
	legs->backlerp = pi->legs.backlerp;
	if ( pi->legsAnimationTimer > 0 )
	{
		UI_RunLerpFrame( pi, &pi->legs2 );
		legs2->oldframe = pi->legs2.oldFrame;
		legs2->frame = pi->legs2.frame;
		legs2->backlerp = pi->legs2.backlerp;
		//if (cg_debugAnim.integer == 2 ) CG_Printf("Legs");
	}
	

	//newAnimation = UI_AnimationPriorities( pi, pi->torso.animationNumber );
	if ( pi->torsoAnimationTimer <= 0 )
		pi->torsoAnimationTimer = UI_BlendPlayerAnimation( pi, newAnimation, pi->torso.animationNumber, &pi->torso, &pi->torso2 );

	if (UI_RunLerpFrame( pi, &pi->torso ) == qfalse)
	{
		if ( !(pi->torso.animationNumber >= BOTH_DEATH1 && pi->torso.animationNumber <= BOTH_DEAD3) ) {
			animNumber_t default_anim = LEGS_IDLE;
			if ( pi->torsoAnimationTimer <= 0 )
			{
				pi->torsoAnimationTimer = UI_BlendPlayerAnimation( pi, default_anim, pi->torso.animationNumber, &pi->torso, &pi->torso2 );
				UI_RunLerpFrame( pi, &pi->torso );	
			}
		}
	}

	torso->oldframe = pi->torso.oldFrame;
	torso->frame = pi->torso.frame;
	torso->backlerp = pi->torso.backlerp;
	if ( pi->torsoAnimationTimer > 0 )
	{
		UI_RunLerpFrame( pi, &pi->torso2 );
		torso2->oldframe = pi->torso2.oldFrame;
		torso2->frame = pi->torso2.frame;
		torso2->backlerp = pi->torso2.backlerp;
	}
}


/*
==================
UI_SwingAngles
==================
*/
static void UI_SwingAngles( float destination, float swingTolerance, float clampTolerance,
					float speed, float *angle, qboolean *swinging ) {
	float	swing;
	float	move;
	float	scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return;
	}
	
	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabsf( swing );
	if ( scale < swingTolerance * 0.5f ) {
		scale = 0.5f;
	} else if ( scale < swingTolerance ) {
		scale = 1.0f;
	} else {
		scale = 2.0f;
	}

	// swing towards the destination angle
	if ( swing >= 0.0f ) {
		move = uiInfo.uiDC.frameTime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0.0f ) {
		move = uiInfo.uiDC.frameTime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - (clampTolerance - 1) );
	} else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + (clampTolerance - 1) );
	}
}


/*
======================
UI_MovedirAdjustment
======================
*/
static float UI_MovedirAdjustment( playerInfo_t *pi ) {
	vec3_t		relativeAngles;
	vec3_t		moveVector;

	VectorSubtract( pi->viewAngles, pi->moveAngles, relativeAngles );
	AngleVectors( relativeAngles, moveVector, NULL, NULL );
	if ( Q_fabs( moveVector[0] ) < 0.01 ) {
		moveVector[0] = 0.0;
	}
	if ( Q_fabs( moveVector[1] ) < 0.01 ) {
		moveVector[1] = 0.0;
	}

	if ( moveVector[1] == 0 && moveVector[0] > 0 ) {
		return 0;
	}
	if ( moveVector[1] < 0 && moveVector[0] > 0 ) {
		return 22;
	}
	if ( moveVector[1] < 0 && moveVector[0] == 0 ) {
		return 45;
	}
	if ( moveVector[1] < 0 && moveVector[0] < 0 ) {
		return -22;
	}
	if ( moveVector[1] == 0 && moveVector[0] < 0 ) {
		return 0;
	}
	if ( moveVector[1] > 0 && moveVector[0] < 0 ) {
		return 22;
	}
	if ( moveVector[1] > 0 && moveVector[0] == 0 ) {
		return  -45;
	}

	return -22;
}


/*
===============
UI_PlayerAngles
===============
*/
static void UI_PlayerAngles( playerInfo_t *pi, vec3_t legsAngles, vec3_t legs[3], vec3_t torsoAngles, vec3_t torso[3], vec3_t headAngles, vec3_t head[3] ) {
	float		dest;
	float		adjust;

	VectorCopy( pi->viewAngles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( pi->legsAnim != LEGS_IDLE 
		|| pi->torsoAnim != TORSO_STAND  ) {
		// if not standing still, always point all in the same direction
		pi->torso.yawing = qtrue;	// always center
		pi->torso.pitching = qtrue;	// always center
		pi->legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	adjust = UI_MovedirAdjustment( pi );
	legsAngles[YAW] = headAngles[YAW] + adjust;
	torsoAngles[YAW] = headAngles[YAW] + 0.25f * adjust;


	// torso
	UI_SwingAngles( torsoAngles[YAW], 25.0f, 90.0f, SWINGSPEED, &pi->torso.yawAngle, &pi->torso.yawing );
	UI_SwingAngles( legsAngles[YAW], 40.0f, 90.0f, SWINGSPEED, &pi->legs.yawAngle, &pi->legs.yawing );

	torsoAngles[YAW] = pi->torso.yawAngle;
	legsAngles[YAW] = pi->legs.yawAngle;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if ( headAngles[PITCH] > 180 ) {
		dest = (-360.0f + headAngles[PITCH]) * 0.75f;
	} else {
		dest = headAngles[PITCH] * 0.75f;
	}
	UI_SwingAngles( dest, 15.0f, 30.0f, 0.1f, &pi->torso.pitchAngle, &pi->torso.pitching );
	torsoAngles[PITCH] = pi->torso.pitchAngle;

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}

/*
===============
UI_PlayerFloatSprite
===============
*/
static void UI_PlayerFloatSprite( playerInfo_t *pi, vec3_t origin, qhandle_t shader )
{
	refEntity_t		ent;

	REF_PARAM( pi );
	
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( origin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = 0;
	trap_R_AddRefEntityToScene( &ent );
}


/*
======================
UI_MachinegunSpinAngle
======================
*/
float	UI_MachinegunSpinAngle( playerInfo_t *pi ) {
	int		delta;
	float	angle;
	float	speed;
	int		torsoAnim;

	delta = dp_realtime - pi->barrelTime;
	if ( pi->barrelSpinning ) {
		angle = pi->barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5f * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / ((float)COAST_TIME) );
		angle = pi->barrelAngle + delta * speed;
	}

	torsoAnim = pi->torsoAnim;
	if( torsoAnim == TORSO_ATTACK2 ) {
		torsoAnim = TORSO_ATTACK;
	}
	if ( pi->barrelSpinning == !(torsoAnim == TORSO_ATTACK) ) {
		pi->barrelTime = dp_realtime;
		pi->barrelAngle = AngleMod( angle );
		pi->barrelSpinning = !!(torsoAnim == TORSO_ATTACK);
	}

	return angle;
}


/*
===============
UI_DrawPlayer
===============
*/
void UI_DrawPlayer( rectDef_t * r, playerInfo_t *pi, int time, float zoom ) {
	refdef_t		refdef;
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	refEntity_t		legs2;
	refEntity_t		torso2;
	refEntity_t		head2;
	vec3_t			origin;
	int				renderfx;
	vec3_t			mins = {-16, -16, -18};
	vec3_t			maxs = {16, 16, 34};
	float			len;
	float			xx;
	float			x = r->x;
	float			y = r->y;
	float			w = r->w;
	float			h = r->h;
	vec3_t			legsAngles, torsoAngles, headAngles;

	if( (pi->modelType == MT_X42 && !pi->legsModel) ||
		(pi->modelType == MT_MD3 && (!pi->torsoModel || !pi->headModel || !pi->animations[0].numFrames)) )
		return;

	// this allows the ui to cache the player model on the main menu
	if( w == 0 || h == 0 )
		return;

	dp_realtime = time;

	if( pi->pendingWeapon != -1 && dp_realtime > pi->weaponTimer )
	{
		pi->weapon = pi->pendingWeapon;
		pi->lastWeapon = pi->pendingWeapon;
		pi->pendingWeapon = -1;
		pi->weaponTimer = 0;
		if( pi->currentWeapon != pi->weapon )
			trap_S_StartLocalSound( weaponChangeSound, CHAN_LOCAL );
	}

	UI_AdjustFrom640( &x, &y, &w, &h );

	y -= jumpHeight;

	memset( &refdef, 0, sizeof( refdef ) );
	memset( &legs, 0, sizeof( legs ) );
	memset( &torso, 0, sizeof( torso ) );
	memset( &head, 0, sizeof( head ) );
	memset( &legs2, 0, sizeof( legs2 ) );
	memset( &torso2, 0, sizeof( torso2 ) );
	memset( &head2, 0, sizeof( head2 ) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.x = (int)x;
	refdef.y = (int)y;
	refdef.width = (int)w;
	refdef.height = (int)h;

	refdef.fov_x = floorf( (float)refdef.width / 640.0f * 80.0F );
	xx = refdef.width / tanf( refdef.fov_x / 360.0f * M_PI );
	refdef.fov_y = atan2f( refdef.height, xx );
	refdef.fov_y *= ( 360.0f / M_PI );

	// calculate distance so the player nearly fills the box
	len = (0.45f + ((1.0f - zoom) * 0.1f)) * ( maxs[2] - mins[2] );		
	origin[0] = len / tanf( DEG2RAD( refdef.fov_x ) * 0.5f );
	origin[1] = 0.5f * ( mins[1] + maxs[1] );
	origin[2] = -0.5f * ( mins[2] + maxs[2] );

	refdef.time = dp_realtime;

	trap_R_ClearScene();

	// get the rotation information
	UI_PlayerAngles( pi, legsAngles, legs.axis, torsoAngles, torso.axis, headAngles, head.axis );
	
	// get the animation state (after rotation, to allow feet shuffle)
	UI_PlayerAnimation( pi, &legs, &legs2, &torso, &torso2 );

	renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

	//
	// add the legs
	//
	legs.hModel = pi->legsModel;
	legs.customSkin = pi->legsSkin;

	VectorCopy( origin, legs.origin );

	VectorCopy( origin, legs.lightingOrigin );
	legs.renderfx = renderfx;
	VectorCopy ( legs.origin, legs.oldorigin );

	if( pi->modelType == MT_X42 )
	{
		animGroupFrame_t groups[2];
		boneOffset_t offsets[2];
		animGroupTransition_t transitions[2];
		
		animGroupFrame_t groups2[2];
		
		groups[0].animGroup = 0;
		groups[0].frame0 = legs.oldframe;
		groups[0].frame1 = legs.frame;
		groups[0].interp = 1.0F - legs.backlerp;
		groups[0].wrapFrames = legs.renderfx & RF_WRAP_FRAMES;

		groups[1].animGroup = 1;
		groups[1].frame0 = torso.oldframe;
		groups[1].frame1 = torso.frame;
		groups[1].interp = 1.0F - torso.backlerp;
		groups[1].wrapFrames = torso.renderfx & RF_WRAP_FRAMES;
		
		//blend groups
		if (pi->legsAnimationTimer > 0 )
		{
			groups2[0].animGroup = 0;
			groups2[0].frame0 = legs2.oldframe;
			groups2[0].frame1 = legs2.frame;
			groups2[0].interp = 1.0F - legs2.backlerp;
			groups2[0].wrapFrames = legs2.renderfx & RF_WRAP_FRAMES;
		} else {
			groups2[0].animGroup = 0;
			groups2[0].frame0 = legs.oldframe;
			groups2[0].frame1 = legs.frame;
			groups2[0].interp = 1.0F - legs.backlerp;
			groups2[0].wrapFrames = legs.renderfx & RF_WRAP_FRAMES;
		}
		//blend groups
		if (pi->torsoAnimationTimer > 0 )
		{
			groups2[1].animGroup = 1;
			groups2[1].frame0 = torso2.oldframe;
			groups2[1].frame1 = torso2.frame;
			groups2[1].interp = 1.0F - torso2.backlerp;
			groups2[1].wrapFrames = torso2.renderfx & RF_WRAP_FRAMES;
		} else {
			groups2[1].animGroup = 1;
			groups2[1].frame0 = torso.oldframe;
			groups2[1].frame1 = torso.frame;
			groups2[1].interp = 1.0F - torso.backlerp;
			groups2[1].wrapFrames = torso.renderfx & RF_WRAP_FRAMES;
		}

		memset( offsets, 0, sizeof( offsets ) );

		Q_strncpyz( offsets[0].boneName, "spine_base", sizeof( offsets[0].boneName ) );

		Quat_FromEulerAngles( offsets[0].rot, torsoAngles );
		
		Q_strncpyz( offsets[1].boneName, "neck_base", sizeof( offsets[1].boneName ) );
		Quat_FromEulerAngles( offsets[1].rot, headAngles );

			
		if ( pi->legsAnimationTimer > 0 || pi->torsoAnimationTimer > 0 )
		{
			if ( pi->legsAnimationTimer > 0 ) {
				transitions[0].animGroup = 0;
				transitions[0].interp = (float)(ANIM_TRANSITION_TIME - pi->legsAnimationTimer) / ANIM_TRANSITION_TIME;
				pi->legsAnimationTimer  -= uiInfo.uiDC.frameTime;;
			} else {
				transitions[0].animGroup = 0;
				transitions[0].interp = 1.0f;
			}
			if ( pi->torsoAnimationTimer > 0 ) {
				transitions[1].animGroup = 1;
				transitions[1].interp = (float)(ANIM_TRANSITION_TIME - pi->torsoAnimationTimer) / ANIM_TRANSITION_TIME;
				pi->torsoAnimationTimer -= uiInfo.uiDC.frameTime;;
			} else {
				transitions[1].animGroup = 1;
				transitions[1].interp = 1.0f;
			}
			/*if (transitions[0].interp >= 1.0f) transitions[0].interp = 1.0f;
			if (transitions[1].interp >= 1.0f) transitions[1].interp = 1.0f;
			if (transitions[0].interp <= 0.0f) transitions[0].interp = 0.0f;
			if (transitions[1].interp <= 0.0f) transitions[1].interp = 0.0f;*/

			//CG_Printf("[NOTICE]Torso: [%f] [%d,%d] [%d->%d]\n", transitions[1].interp, cent->pe.torsoTransTime, cg.frametime, groups2[1].frame0, groups[1].frame0);
			legs.poseData = trap_R_BuildPose2( legs.hModel, groups2, offsets, groups, offsets, transitions);

		} else {
			legs.poseData = trap_R_BuildPose( legs.hModel,	groups, lengthof( groups ), offsets, lengthof( offsets ) );
		}

		//x42 characters have their feet at zero,
		//but MD3s are set up on this odd offset thing: adjust
		legs.origin[2] -= 24;
	}
	trap_R_AddRefEntityToScene( &legs );

	if( !legs.hModel )
		return;

	if( pi->modelType == MT_MD3 )
	{
		//
		// add the torso
		//
		torso.hModel = pi->torsoModel;
		if( !torso.hModel )
			return;
	
		torso.customSkin = pi->torsoSkin;
	
		VectorCopy( origin, torso.lightingOrigin );
	
		UI_PositionRotatedEntityOnTag( &torso, &legs, pi->legsModel, "tag_torso");
	
		torso.renderfx = renderfx;
	
		trap_R_AddRefEntityToScene( &torso );
	
		//
		// add the head
		//
		head.hModel = pi->headModel;
		if (!head.hModel) {
			return;
		}
		head.customSkin = pi->headSkin;
	
		VectorCopy( origin, head.lightingOrigin );
	
		UI_PositionRotatedEntityOnTag( &head, &torso, pi->torsoModel, "tag_head");
	
		head.renderfx = renderfx;
	
		trap_R_AddRefEntityToScene( &head );
	}
	//
	// add the gun
	//
	/*if ( pi->currentWeapon != WP_NONE ) {
		memset( &gun, 0, sizeof(gun) );
		gun.hModel = pi->weaponModel;
		VectorCopy( origin, gun.lightingOrigin );
		UI_PositionEntityOnTag( &gun, &torso, pi->torsoModel, "tag_weapon");
		gun.renderfx = renderfx;
		trap_R_AddRefEntityToScene( &gun );
	}*/

	//
	// add the chat icon
	//
	if( pi->chat )
		UI_PlayerFloatSprite( pi, origin, trap_R_RegisterShaderNoMip( "sprites/balloon3" ) );

	//
	// add an accent light
	//
	origin[0] -= 200;	// + = behind, - = in front
	origin[1] += 200;	// + = left, - = right
	origin[2] += 100;	// + = above, - = below
	trap_R_AddLightToScene( origin, 400.0f, 0.2f, 0.2f, 1.0f );

	origin[0] -= 200;
	origin[1] -= 200;
	origin[2] += 100;
	trap_R_AddLightToScene( origin, 400.0f, 1.0f, 0.5f, 0.2f );

	trap_R_RenderScene( &refdef );
}

/*
==========================
UI_FileExists
==========================
*/
static qboolean	UI_FileExists(const char *filename) {
	int len;

	len = trap_FS_FOpenFile( filename, 0, FS_READ );
	if (len>0) {
		return qtrue;
	}
	return qfalse;
}

/*
==========================
UI_FindClientHeadFile
==========================
*/
static qboolean	UI_FindClientHeadFile( char *filename, int length, const char *teamName, const char *headModelName, const char *headSkinName, const char *base, const char *ext ) {
	char *team, *headsFolder;
	int i;

	team = "default";

	if ( headModelName[0] == '*' ) {
		headsFolder = "heads/";
		headModelName++;
	}
	else {
		headsFolder = "";
	}
	for( ; ; )
	{
		for( i = 0; i < 2; i++ )
		{
			if ( i == 0 && teamName && *teamName ) {
				Com_sprintf( filename, length, "models/players/%s%s/%s/%s%s_%s.%s", headsFolder, headModelName, headSkinName, teamName, base, team, ext );
			}
			else {
				Com_sprintf( filename, length, "models/players/%s%s/%s/%s_%s.%s", headsFolder, headModelName, headSkinName, base, team, ext );
			}
			if ( UI_FileExists( filename ) ) {
				return qtrue;
			}
			if ( i == 0 && teamName && *teamName ) {
				Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, headSkinName, ext );
			}
			else {
				Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headModelName, base, headSkinName, ext );
			}
			if ( UI_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( headsFolder[0] ) {
			break;
		}
		headsFolder = "heads/";
	}

	return qfalse;
}

static qboolean	UI_FindClientModelFile( char *filename, int length, playerInfo_t *pi, const char *teamName, const char *modelName, const char *skinName, const char *base, const char *ext )
{
	char *team, *charactersFolder;
	int i;

	REF_PARAM( pi );

	team = "default";
	charactersFolder = "";
	for( ; ; )
	{
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				//								"models/players/characters/james/stroggs/lower_lily_red.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, team, ext );
			}
			else {
				//								"models/players/characters/james/lower_lily_red.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s_%s_%s.%s", charactersFolder, modelName, base, skinName, team, ext );
			}
			if ( UI_FileExists( filename ) ) {
				return qtrue;
			}
			if ( i == 0 && teamName && *teamName ) {
				//								"models/players/characters/james/stroggs/lower_lily.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, ext );
			}
			else {
				//								"models/players/characters/james/lower_lily.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelName, base, skinName, ext );
			}
			if ( UI_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( charactersFolder[0] ) {
			break;
		}
		charactersFolder = "characters/";
	}

	return qfalse;
}

/*
==========================
UI_RegisterClientSkin
==========================
*/
static qboolean	UI_RegisterClientSkin( playerInfo_t *pi, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName , const char *teamName) {
	char		filename[MAX_QPATH*2];

	switch( pi->modelType )
	{
	case MT_X42:
		if( UI_FindClientModelFile( filename, sizeof( filename ), pi, teamName, modelName, skinName, "char", "skin" ) )
			pi->legsSkin = trap_R_RegisterSkin( filename );
		if( pi->legsSkin )
			return qtrue;
		else
			Com_Printf( "Character skin load failure: %s\n", filename );
		break;
	case MT_MD3:
		if (teamName && *teamName) {
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/%s/lower_%s.skin", modelName, teamName, skinName );
		} else {
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower_%s.skin", modelName, skinName );
		}
		pi->legsSkin = trap_R_RegisterSkin( filename );
		if (!pi->legsSkin) {
			if (teamName && *teamName) {
				Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/%s/lower_%s.skin", modelName, teamName, skinName );
			} else {
			Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/lower_%s.skin", modelName, skinName );
			}
			pi->legsSkin = trap_R_RegisterSkin( filename );
		}
	
		if (teamName && *teamName) {
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/%s/upper_%s.skin", modelName, teamName, skinName );
		} else {
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper_%s.skin", modelName, skinName );
		}
		pi->torsoSkin = trap_R_RegisterSkin( filename );
		if (!pi->torsoSkin) {
			if (teamName && *teamName) {
				Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/%s/upper_%s.skin", modelName, teamName, skinName );
			} else {
				Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/upper_%s.skin", modelName, skinName );
			}
			pi->torsoSkin = trap_R_RegisterSkin( filename );
		}

		if ( UI_FindClientHeadFile( filename, sizeof(filename), teamName, headModelName, headSkinName, "head", "skin" ) ) {
			pi->headSkin = trap_R_RegisterSkin( filename );
		}
	
		if ( !pi->legsSkin || !pi->torsoSkin || !pi->headSkin ) {
			return qfalse;
		}
		break;
	}
	return qtrue;
}


/*
======================
UI_ParseAnimationFile
======================
*/
static qboolean UI_ParseAnimationFile( const char *filename, animation_t *animations ) {
	const char	*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
	int			anim;
	char		text[20000];
	fileHandle_t	f;

	memset( animations, 0, sizeof( animation_t ) * MAX_ANIMATIONS );

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= ( sizeof( text ) - 1 ) ) {
		Com_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	COM_Compress(text);

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	// read optional parameters
	for( ; ; )
	{
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}
		if ( !Q_stricmp( token, "footsteps" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			continue;
		} else if ( !Q_stricmp( token, "headoffset" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !token ) {
					break;
				}
			}
			continue;
		} else if ( !Q_stricmp( token, "sex" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			continue;
		} else if ( (anim = BG_FindAnimationByName( token ) ) >= 0 )
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
			fps = (float)atoi( token );
			if ( fps == 0 ) {
				fps = 1;
			}
			animations[anim].frameLerp = (int)(1000 / fps);
			animations[anim].initialLerp = (int)(1000 / fps);
			
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

/*
==========================
UI_RegisterClientModelname
==========================
*/
qboolean UI_RegisterClientModelname( playerInfo_t *pi, const char *modelSkinName, const char *headModelSkinName, const char *teamName ) {
	char		modelName[MAX_QPATH];
	char		skinName[MAX_QPATH];
	char		headModelName[MAX_QPATH];
	char		headSkinName[MAX_QPATH];
	char		filename[MAX_QPATH];
	char		*slash;

	pi->torsoModel = 0;
	pi->headModel = 0;
	pi->legsModel = 0;

	if ( !modelSkinName[0] ) {
		return qfalse;
	}

	Q_strncpyz( modelName, modelSkinName, sizeof( modelName ) );

	slash = strchr( modelName, '/' );
	if ( !slash ) {
		// modelName did not include a skin name
		Q_strncpyz( skinName, "default", sizeof( skinName ) );
	} else {
		Q_strncpyz( skinName, slash + 1, sizeof( skinName ) );
		*slash = '\0';
	}

	Q_strncpyz( headModelName, headModelSkinName, sizeof( headModelName ) );
	slash = strchr( headModelName, '/' );
	if ( !slash ) {
		// modelName did not include a skin name
		Q_strncpyz( headSkinName, "default", sizeof( skinName ) );
	} else {
		Q_strncpyz( headSkinName, slash + 1, sizeof( skinName ) );
		*slash = '\0';
	}

	// load cmodels before models so filecache works

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/char_hub.x42", modelName );
	pi->legsModel = trap_R_RegisterModel( filename );
	if( !pi->legsModel )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/char.x42", modelName );
		pi->legsModel = trap_R_RegisterModel( filename );
	}
	if( pi->legsModel )
		pi->modelType = MT_X42;
	else
	{
		pi->modelType = MT_MD3;
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", modelName );
		pi->legsModel = trap_R_RegisterModel( filename );
		if( !pi->legsModel )
		{
			Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/lower.md3", modelName );
			pi->legsModel = trap_R_RegisterModel( filename );
			if( !pi->legsModel )
			{
				Com_Printf( "Failed to load model file %s\n", filename );
				return qfalse;
			}
		}

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", modelName );
		pi->torsoModel = trap_R_RegisterModel( filename );
		if ( !pi->torsoModel ) {
			Com_sprintf( filename, sizeof( filename ), "models/players/characters/%s/upper.md3", modelName );
			pi->torsoModel = trap_R_RegisterModel( filename );
			if ( !pi->torsoModel ) {
				Com_Printf( "Failed to load model file %s\n", filename );
				return qfalse;
			}
		}

		if (headModelName && headModelName[0] == '*' ) {
			Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/%s.md3", &headModelName[1], &headModelName[1] );
		}
		else {
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/head.md3", headModelName );
		}
		pi->headModel = trap_R_RegisterModel( filename );
		if ( !pi->headModel && headModelName[0] != '*') {
			Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/%s.md3", headModelName, headModelName );
			pi->headModel = trap_R_RegisterModel( filename );
		}
	
		if (!pi->headModel) {
			Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}
	
	// if any skins failed to load, fall back to default
	if ( !UI_RegisterClientSkin( pi, modelName, skinName, headModelName, headSkinName, teamName) ) {
		if ( !UI_RegisterClientSkin( pi, modelName, "default", headModelName, "default", teamName ) ) {
			Com_Printf( "Failed to load skin file: %s : %s\n", modelName, skinName );
			return qfalse;
		}
	}

	// load the animations
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation_hub.cfg", modelName );
	if ( !UI_ParseAnimationFile( filename, pi->animations ) ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", modelName );
		if ( !UI_ParseAnimationFile( filename, pi->animations ) ) {
			Com_Printf( "Failed to load animation file %s\n", filename );
			return qfalse;
		}
	}

	return qtrue;
}


/*
===============
UI_PlayerInfo_SetModel
===============
*/
void UI_PlayerInfo_SetModel( playerInfo_t *pi, const char *model, const char *headmodel, char *teamName ) {
	memset( pi, 0, sizeof(*pi) );
	UI_RegisterClientModelname( pi, model, headmodel, teamName );
	pi->weapon = WP_PISTOL;
	pi->currentWeapon = pi->weapon;
	pi->lastWeapon = pi->weapon;
	pi->pendingWeapon = -1;
	pi->weaponTimer = 0;
	pi->chat = qfalse;
	pi->newModel = qtrue;
	UI_PlayerInfo_SetWeapon( pi, pi->weapon );
}


/*
===============
UI_PlayerInfo_SetInfo
===============
*/
void UI_PlayerInfo_SetInfo( playerInfo_t *pi, animNumber_t legsAnim, animNumber_t torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNumber, qboolean chat ) {
	int			currentAnim;
	weapon_t	weaponNum;

	pi->chat = chat;
	pi->idleTimer = 0;//uiInfo.uiDC.realTime + 2000 + ((Q_rand()/3)%2000);


	// view angles
	VectorCopy( viewAngles, pi->viewAngles );

	// move angles
	VectorCopy( moveAngles, pi->moveAngles );

	if ( pi->newModel ) {
		pi->newModel = qfalse;

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim( pi, legsAnim );
		pi->legs.yawAngle = viewAngles[YAW];
		pi->legs.yawing = qfalse;

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim( pi, torsoAnim );
		pi->torso.yawAngle = viewAngles[YAW];
		pi->torso.yawing = qfalse;

		if ( weaponNumber != -1 ) {
			pi->weapon = weaponNumber;
			pi->currentWeapon = weaponNumber;
			pi->lastWeapon = weaponNumber;
			pi->pendingWeapon = -1;
			pi->weaponTimer = 0;
			UI_PlayerInfo_SetWeapon( pi, pi->weapon );
		}

		return;
	}

	// weapon
	if ( weaponNumber == -1 ) {
		pi->pendingWeapon = -1;
		pi->weaponTimer = 0;
	}
	else if ( weaponNumber != WP_NONE ) {
		pi->pendingWeapon = weaponNumber;
		pi->weaponTimer = dp_realtime + UI_TIMER_WEAPON_DELAY;
	}
	weaponNum = pi->lastWeapon;
	pi->weapon = weaponNum;

	if ( torsoAnim == BOTH_DEATH1 || legsAnim == BOTH_DEATH1 ) {
		torsoAnim = legsAnim = LEGS_IDLE;
		pi->weapon = pi->currentWeapon = WP_NONE;
		UI_PlayerInfo_SetWeapon( pi, pi->weapon );

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim( pi, legsAnim );

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim( pi, torsoAnim );

		return;
	}

	// leg animation
	currentAnim = pi->legsAnim;
	if ( legsAnim != LEGS_JUMP && ( currentAnim == LEGS_JUMP || currentAnim == LEGS_LAND ) ) {
		pi->pendingLegsAnim = legsAnim;
	}
	else if ( legsAnim != currentAnim ) {
		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim( pi, legsAnim );
	}

	// torso animation
	if ( torsoAnim == TORSO_STAND || torsoAnim == TORSO_STAND2 ) {
		if ( weaponNum == WP_NONE || weaponNum == WP_GAUNTLET ) {
			torsoAnim = TORSO_STAND2;
		}
		else {
			torsoAnim = TORSO_STAND;
		}
	}

	if ( torsoAnim == TORSO_ATTACK || torsoAnim == TORSO_ATTACK2 ) {
		if ( weaponNum == WP_NONE || weaponNum == WP_GAUNTLET || weaponNum == WP_HIGHCAL ) {
			torsoAnim = TORSO_ATTACK2;
		}
		else {
			torsoAnim = TORSO_ATTACK;
		}
		pi->muzzleFlashTime = dp_realtime + UI_TIMER_MUZZLE_FLASH;
		//FIXME play firing sound here
	}

	currentAnim = pi->torsoAnim;

	if ( weaponNum != pi->currentWeapon || currentAnim == TORSO_RAISE || currentAnim == TORSO_DROP ) {
		pi->pendingTorsoAnim = torsoAnim;
	}
	else if ( ( currentAnim == TORSO_GESTURE || currentAnim == TORSO_ATTACK ) && ( torsoAnim != currentAnim ) ) {
		pi->pendingTorsoAnim = torsoAnim;
	}
	else if ( torsoAnim != currentAnim ) {
		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim( pi, torsoAnim );
	}
}
