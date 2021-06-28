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

#include "g_local.h"

// g_weapon.c 
// perform the server side effects of a weapon firing

static	float	s_quadFactor;
static	vec3_t	forward, right, up;
static	vec3_t	muzzle;

#define NUM_NAILSHOTS 15

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


//to avoid a square root we store the square of the values here.
#define RUN_SPEED 40000.0f // 200 units/sec
//consider everything under 20 units/sec to be standing still for accuracy calculations
#define STOPPED_SPEED 400.0f // 20 units/sec
//Double the spread when running, half when crouched, 75% when standing still, default spread when walking.
int G_CalculateSpread(gentity_t *ent, int spread)
{
	float speed;
	
	speed = (ent->client->ps.velocity[0] * ent->client->ps.velocity[0]) + 
			(ent->client->ps.velocity[1] * ent->client->ps.velocity[1]);
	
	//player is ducked, and not in mid air.
	if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE && ent->client->ps.pm_flags & PMF_DUCKED) {
		return (int)(spread * 0.5f);
	}
	if (speed > RUN_SPEED) {
		//running
		return (int)(spread * 2.0f);
	} else if (speed < STOPPED_SPEED) {
		//still
		return (int)(spread * 0.75f);
	} else {
		//walking
		return (int)spread;
	}
}


/*
======================================================================

GAUNTLET

======================================================================
*/
#define GAUNTLET_DAMAGE 120

void Weapon_Gauntlet( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	vec3_t		angle;

	// set aiming directions
	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePointOrigin ( ent, ent->client->oldOrigin, forward, right, up, muzzle );
	muzzle[2] -= 10;

	VectorMA (muzzle, 64, forward, end);

	trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];
	if ( g_gametype.integer != GT_HUB ) {
		// send blood impact when we are not in the hub
		if ( traceEnt->takedamage && traceEnt->client ) {
			tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte( tr.plane.normal );
			tent->s.weapon = ent->s.weapon;
		}
	}

    if ( !traceEnt->takedamage) {
        return;
    }
	
	//Adjust the vector we are shooting around a bit.
	//do this after the trace so we still hit stuff right infront of us
	//but the vector they will be knocked back along has a 30deg upwards increase.
	VectorCopy(ent->client->ps.viewangles,angle);
	angle[PITCH] -= 30;
	// set aiming directions
	AngleVectors (angle, forward, right, up);
	
    G_Damage( traceEnt, ent, ent, forward, tr.endpos,
        GAUNTLET_DAMAGE, 0, MOD_GAUNTLET, 1 );
}

gentity_t * CheckTalkToBot ( gentity_t *ent)
{
 	trace_t		tr;
	vec3_t		end;
	gentity_t	*traceEnt;

	// set aiming directions
	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	VectorMA ( ent->r.currentOrigin, CHAT_DISTANCE, forward, end);

	trap_Trace (&tr, ent->r.currentOrigin, vec3_origin, vec3_origin, end, ent->s.number, CONTENTS_BODY);

	traceEnt = &g_entities[ tr.entityNum ];
    if (traceEnt->client && traceEnt->r.svFlags & SVF_BOT)
    {
		return traceEnt;
	}

	return NULL;
}

/*
======================================================================

MACHINEGUN

======================================================================
*/

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating 
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = (int)v[i];
		} else {
			v[i] = (int)v[i] + 1;
		}
	}
}

#define CHAINGUN_SPREAD		600
#define MACHINEGUN_SPREAD	1000
#define	MACHINEGUN_DAMAGE	15
#define	MACHINEGUN_TEAM_DAMAGE	5		// wimpier MG in teamplay

#define HIGHCAL_SPREAD   600
#define HIGHCAL_DAMAGE   40

#define ASSAULT_SPREAD   800
#define ASSAULT_DAMAGE   25


void Bullet_Fire (gentity_t *ent, int spread, int damage, int mod ) {
	trace_t		tr;
	vec3_t		end;
	float		r;
	float		u;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			i, passent;

	damage *= s_quadFactor;

	spread = G_CalculateSpread(ent, spread);

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * spread * 16;
	r = cos(r) * crandom() * spread * 16;
	VectorMA (muzzle, 8192*16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);

	passent = ent->s.number;
	for (i = 0; i < 10; i++) {

		trap_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( tr.endpos, muzzle );

		// send bullet impact
		if ( traceEnt->takedamage && traceEnt->client ) {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
			tent->s.eventParm = traceEnt->s.number;
			if( LogAccuracyHit( traceEnt, ent ) ) {
				ent->client->accuracy_hits++;
			}
		} else {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
			tent->s.eventParm = DirToByte( tr.plane.normal );
		}
		tent->s.otherEntityNum = ent->s.number;

		if ( traceEnt->takedamage) {
			G_Damage( traceEnt, ent, ent, forward, tr.endpos,
				damage, 0, mod, 0);
		}
		break;
	}
}

/*
======================================================================

SHOTGUN

======================================================================
*/

// DEFAULT_SHOTGUN_SPREAD and DEFAULT_SHOTGUN_COUNT	are in bg_public.h, because
// client predicts same spreads
#define	DEFAULT_SHOTGUN_DAMAGE	17

qboolean ShotgunPellet( vec3_t start, vec3_t end, gentity_t *ent ) {
	trace_t		tr;
	int			damage, i, passent;
	gentity_t	*traceEnt;
	vec3_t		tr_start, tr_end;

	passent = ent->s.number;
	VectorCopy( start, tr_start );
	VectorCopy( end, tr_end );
	for (i = 0; i < 10; i++) {
		trap_Trace (&tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT);
		traceEnt = &g_entities[ tr.entityNum ];

		// send bullet impact
		if (  tr.surfaceFlags & SURF_NOIMPACT ) {
			return qfalse;
		}

		if ( traceEnt->takedamage) {
			damage = DEFAULT_SHOTGUN_DAMAGE * s_quadFactor;
			G_Damage( traceEnt, ent, ent, forward, tr.endpos,
				damage, 0, MOD_SHOTGUN, 0);
			if( LogAccuracyHit( traceEnt, ent ) ) {
				return qtrue;
			}
		}
		return qfalse;
	}
	return qfalse;
}

// this should match CG_ShotgunPattern
void ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent ) {
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;
	qboolean	hitClient = qfalse;

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
		if( ShotgunPellet( origin, end, ent ) && !hitClient ) {
			hitClient = qtrue;
			ent->client->accuracy_hits++;
		}
	}
}


void weapon_supershotgun_fire (gentity_t *ent) {
	gentity_t		*tent;

	// send shotgun blast
	tent = G_TempEntity( muzzle, EV_SHOTGUN );
	VectorScale( forward, 4096, tent->s.origin2 );
	SnapVector( tent->s.origin2 );
	tent->s.eventParm = Q_rand() & 255;		// seed for spread pattern
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern( tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent );
}


/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire (gentity_t *ent) {
	gentity_t	*m;
	vec3_t hand;

	// extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize( forward );

	VectorCopy( ent->s.pos.trBase, hand );

	m = fire_grenade (ent, hand, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}



/*
======================================================================

GRAPPLING HOOK

======================================================================
*/

void Weapon_GrapplingHook_Fire (gentity_t *ent)
{
	if (!ent->client->fireHeld && !ent->client->hook)
		fire_grapple (ent, muzzle, forward);

	ent->client->fireHeld = qtrue;
}

void Weapon_HookFree (gentity_t *ent)
{
	ent->parent->client->hook = NULL;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity( ent );
}

void Weapon_HookThink (gentity_t *ent)
{
	if (ent->enemy) {
		vec3_t v, oldorigin;

		VectorCopy(ent->r.currentOrigin, oldorigin);
		v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5f;
		v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5f;
		v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5f;
		SnapVectorTowards( v, oldorigin );	// save net bandwidth

		G_SetOrigin( ent, v );
	}

	VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);
}

/*
======================================================================

C4

======================================================================
*/

void Weapon_C4_Fire (gentity_t *ent) {
	gentity_t	*m;
	
    // extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize( forward );

	m = fire_c4 (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

void Weapon_C4_Detonate (gentity_t *ent) {
    //Go throug the c4 ents, and blow up the ones that belong to our client.
    gentity_t *c4;
    c4=NULL;
    while ( (c4 = G_Find(c4,FOFS(classname), "c4")) != NULL )
    {
        if (c4->r.ownerNum == ent->s.number){
            c4->nextthink = level.time+5;
        }
    }
}

//======================================================================


/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if( !target->client ) {
		return qfalse;
	}

	if( !attacker->client ) {
		return qfalse;
	}

	if( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
CalcMuzzlePoint

set muzzle location relative to the center of the body
===============
*/
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

/*
===============
CalcMuzzlePointOrigin

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePointOrigin ( gentity_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

void Cmd_GrenadeFire_f( gentity_t *ent )
{
	if (ent->client->ammo_in_clip[WP_GRENADE] > 0) {
		ent->s.weapon = WP_GRENADE;
		FireWeapon(ent);
	}
}

/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent ) {
	//Remove Ammo if not infinite
    //but the C4 will handle it's removal of ammo itself since depeding on the weapon mode it may not actually be shooting
	if ( ent->s.weapon != WP_C4 &&
         ent->client->ammo_in_clip[ ent->s.weapon ] != -1 ) {
		ent->client->ammo_in_clip[ ent->s.weapon ]--;
	}
	if (ent->s.weapon == WP_GRENADE ) {
		ent->client->ps.ammo[WP_GRENADE]--;
	}

	s_quadFactor = 1;

	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
	if( ent->s.weapon != WP_GRAPPLING_HOOK ) {
		ent->client->accuracy_shots++;
	}

	// set aiming directions
	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePointOrigin ( ent, ent->client->oldOrigin, forward, right, up, muzzle );

	// fire the specific weapon
	switch( ent->s.weapon ) {
	case WP_GAUNTLET:
		Weapon_Gauntlet( ent );
		break;
	case WP_SHOTGUN:
		weapon_supershotgun_fire( ent );
		break;
	case WP_PISTOL:
		Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_DAMAGE, MOD_PISTOL );
		break;
	case WP_GRENADE:
		weapon_grenadelauncher_fire( ent );
		break;
	case WP_ASSAULT:
		if (ent->client->ps.stats[STAT_MODE] == qtrue) {
			Bullet_Fire( ent, ASSAULT_SPREAD/4, ASSAULT_DAMAGE*1.50f, MOD_ASSAULT);
		} else {
			Bullet_Fire( ent, ASSAULT_SPREAD, ASSAULT_DAMAGE, MOD_ASSAULT);
		}
		break;
	case WP_GRAPPLING_HOOK:
		Weapon_GrapplingHook_Fire( ent );
		break;
    case WP_HIGHCAL:
        Bullet_Fire( ent, HIGHCAL_SPREAD, HIGHCAL_DAMAGE, MOD_HIGHCAL);
        break;
	case WP_C4:
        if (ent->client->weaponStates[WP_C4] == qfalse)
        {
            ent->client->ammo_in_clip[ ent->s.weapon ]--;
		    Weapon_C4_Fire( ent );
        } else {
            Weapon_C4_Detonate( ent );
        }
		break;

	default:
// FIXME		G_Error( "Bad ent->s.weapon" );
		break;
	}
}


/*
==================
  BG_ClipAmountForWeapon
==================
*/
int G_ClipAmountForWeapon( int weapon )
{
    switch(weapon)
    {
	case WP_PISTOL: return 40;
	case WP_HIGHCAL: return 9;
	case WP_SHOTGUN: return 1;
	case WP_ASSAULT: return 35;
	case WP_GRENADE: return 0;
	case WP_C4: return 4;
    default: return 0;
    }
}

/*
==================
  MaxTotalAmmo
==================
*/
int G_MaxTotalAmmo( int weapon )
{
    switch(weapon)
    {
	case WP_PISTOL: return 120;
	case WP_HIGHCAL: return 36;
	case WP_SHOTGUN: return 40;
	case WP_ASSAULT: return 140;
	case WP_GRENADE: return 4;
	case WP_C4: return 4;
	case WP_GAUNTLET: return -1;
	case WP_GRAPPLING_HOOK: return -1;
    default: return 0;
    }
}
