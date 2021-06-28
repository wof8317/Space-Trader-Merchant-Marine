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

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	if ( ent->s.eFlags & EF_BOUNCE_HALF ) {
		VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) {
			G_SetOrigin( ent, trace->endpos );
			return;
		}
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	// splash damage
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent
			, ent->splashMethodOfDeath ) ) {
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
		}
	}

	trap_LinkEntity( ent );
}



/*
================
C4_Explode
================
*/
static void C4_Explode( gentity_t *c4 ) {
	G_ExplodeMissile( c4 );
	// if the prox mine has a trigger free it
	if (c4->activator) {
		G_FreeEntity(c4->activator);
		c4->activator = NULL;
	}
}

/*
================
C4_Die
================
*/
static void C4_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = C4_Explode;
	ent->nextthink = level.time + 1;
}

/*
================
C4_Trigger
================
*/
void C4_Trigger( gentity_t *trigger, gentity_t *other, trace_t *trace ) {
	vec3_t		v;
	gentity_t	*c4;

	if( !other->client ) {
		return;
	}

	// trigger is a cube, do a distance test now to act as if it's a sphere
	VectorSubtract( trigger->s.pos.trBase, other->s.pos.trBase, v );
	if( VectorLength( v ) > trigger->parent->splashRadius ) {
		return;
	}


	if ( g_gametype.integer >= GT_TEAM ) {
		// don't trigger same team mines
		if (trigger->parent->s.generic1 == other->client->sess.sessionTeam) {
			return;
		}
	}

	// ok, now check for ability to damage so we don't get triggered thru walls, closed doors, etc...
	if( !CanDamage( other, trigger->s.pos.trBase ) ) {
		return;
	}

	// trigger the mine!
	c4 = trigger->parent;
	c4->s.loopSound = 0;
	G_AddEvent( c4, EV_C4_TRIGGER, 0 );
	c4->nextthink = level.time + 500;

	G_FreeEntity( trigger );
}

/*
================
C4_Activate
================
*/
static void C4_Activate( gentity_t *ent ) {

	ent->think = C4_Explode;

	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = C4_Die;

	ent->s.loopSound = G_SoundIndex( "sound/weapons/c4/wstbtick.ogg" );
}

/*
================
C4_ExplodeOnPlayer
================
*/
static void C4_ExplodeOnPlayer( gentity_t *c4 ) {
	gentity_t	*player;

	player = c4->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

	G_SetOrigin( c4, player->s.pos.trBase );
	// make sure the explosion gets to the client
	c4->r.svFlags &= ~SVF_NOCLIENT;
	c4->splashMethodOfDeath = MOD_C4;
	G_ExplodeMissile( c4 );
}

/*
================
C4_Player
================
*/
static void C4_Player( gentity_t *c4, gentity_t *player ) {
	if( c4->s.eFlags & EF_NODRAW ) {
		return;
	}

	G_AddEvent( c4, EV_C4_STICK, 0 );

	if( player->s.eFlags & EF_TICKING ) {
		player->activator->splashDamage += c4->splashDamage;
		player->activator->splashRadius *= 1.50;
		c4->think = G_FreeEntity;
		c4->nextthink = level.time;
		return;
	}

	player->client->ps.eFlags |= EF_TICKING;
	player->activator = c4;

	c4->s.eFlags |= EF_NODRAW;
	c4->r.svFlags |= SVF_NOCLIENT;
	c4->s.pos.trType = TR_LINEAR;
	VectorClear( c4->s.pos.trDelta );

	c4->enemy = player;
	c4->think = C4_ExplodeOnPlayer;
	/*if ( player->client->invulnerabilityTime > level.time ) {
		c4->nextthink = level.time + 2 * 1000;
	}
	else {
		c4->nextthink = level.time + 10 * 1000;
	}*/
    c4->nextthink = level.time + 60 * 1000;
}


/*
================
G_MissileImpact
================
*/
void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
	
	other = &g_entities[trace->entityNum];

	// check for bounce
	if ( !other->takedamage &&
		( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
		G_BounceMissile( ent, trace );
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		return;
	}

	// impact damage
	if (other->takedamage) {
		// FIXME: wrong damage direction?
		if ( ent->damage ) {
			vec3_t	velocity;

			if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;	// stepped on a grenade
			}
			G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
				ent->s.origin, ent->damage, 
				0, ent->methodOfDeath, 0);
		}
	}

	if( ent->s.weapon == WP_C4 ) {
		if( ent->s.pos.trType != TR_GRAVITY ) {
			return;
		}

		// if it's a player, stick it on to them (flag them and remove this entity)
		if( other->s.eType == ET_PLAYER && other->health > 0 ) {
            if (ent->r.ownerNum != other->s.number){
			    C4_Player( ent, other );
            }
			return;
		}

		SnapVectorTowards( trace->endpos, ent->s.pos.trBase );
		G_SetOrigin( ent, trace->endpos );
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear( ent->s.pos.trDelta );

		G_AddEvent( ent, EV_C4_STICK, trace->surfaceFlags );

		ent->think = C4_Activate;
		ent->nextthink = level.time + 2000;

		vectoangles( trace->plane.normal, ent->s.angles );
		ent->s.angles[0] += 90;

		// link the c4 to the other entity
		ent->enemy = other;
		ent->die = C4_Die;
		VectorCopy(trace->plane.normal, ent->movedir);
		VectorSet(ent->r.mins, -4, -4, -4);
		VectorSet(ent->r.maxs, 4, 4, 4);
		trap_LinkEntity(ent);

		return;
	}

	if (!strcmp(ent->classname, "hook")) {
		gentity_t *nent;
		vec3_t v;

		nent = G_Spawn();
		if ( other->takedamage && other->client ) {

			G_AddEvent( nent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

			SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth
		} else {
			VectorCopy(trace->endpos, v);
			G_AddEvent( nent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
			ent->enemy = NULL;
		}

		SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth

		nent->freeAfterEvent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		G_SetOrigin( ent, v );
		G_SetOrigin( nent, v );

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity( ent );
		trap_LinkEntity( nent );

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, 
			other, ent->splashMethodOfDeath ) ) {
			if( !hitClient ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
			}
		}
	}

	trap_LinkEntity( ent );
}

/*
================
G_RunMissile
================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			passent;

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
	// prox mines that left the owner bbox will attach to anything, even the owner
	else if (ent->s.weapon == WP_C4 && ent->count) {
		passent = ENTITYNUM_NONE;
	}
	else {
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	}
	// trace a line from the previous position to the current position
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );

	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask );
		tr.fraction = 0;
	}
	else {
		VectorCopy( tr.endpos, ent->r.currentOrigin );
	}

	trap_LinkEntity( ent );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
				ent->parent->client->hook = NULL;
			}
			G_FreeEntity( ent );
			return;
		}
		G_MissileImpact( ent, &tr );
		if ( ent->s.eType != ET_MISSILE ) {
			return;		// exploded
		}
	}

	// if the C4 wasn't yet outside the player body
	if (ent->s.weapon == WP_C4 && !ent->count) {
		// check if the C4 is outside the owner bbox
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask );
		if (!tr.startsolid || tr.entityNum != ent->r.ownerNum) {
			ent->count = 1;
		}
	}

	// check think function after bouncing
	G_RunThink( ent );
}
/*
================
G_RunModel
================
*/
void G_RunModel( gentity_t *ent ) {
	vec3_t		origin;

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	trap_LinkEntity( ent );
	// check think function after bouncing
	G_RunThink( ent );
}

void G_ShootGrenade( gentity_t *ent ) {
	vec3_t hand, forward, right, up;
	gentity_t *shooter;

	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	ent->think = G_ExplodeMissile;
	ent->s.eFlags |= EF_BOUNCE_HALF;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	ent->nextthink = level.time + 3300;
	
	shooter = &g_entities[ent->r.ownerNum];
	
	AngleVectors (shooter->client->ps.viewangles, forward, right, up);
	
	VectorCopy( shooter->s.pos.trBase, hand );

	VectorMA( hand, 10, forward, hand );
	VectorMA( hand, -14, right, hand );
	VectorMA( hand, shooter->client->ps.viewheight, up, hand );

	VectorCopy( hand, ent->s.pos.trBase );
	VectorScale( forward, 700, ent->s.pos.trDelta );
	SnapVector( ent->s.pos.trDelta );			// save net bandwidth

	VectorCopy (hand, ent->r.currentOrigin);

}

//=============================================================================
/*
=================
fire_grenade
=================
*/
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 700;
	bolt->think = G_ShootGrenade;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN | SVF_NOCLIENT;
	bolt->s.weapon = WP_GRENADE;
	bolt->s.eFlags |= EF_NODRAW;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 1000;
	bolt->splashDamage = 1000;
	bolt->splashRadius = 400;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SOLID;
	bolt->target_ent = NULL;

	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}


/*
=================
fire_grapple
=================
*/
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*hook;

	VectorNormalize (dir);

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = NULL;

	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	hook->s.otherEntityNum = self->s.number; // use to match beam in client
	VectorCopy( start, hook->s.pos.trBase );
	VectorScale( dir, 800, hook->s.pos.trDelta );
	SnapVector( hook->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, hook->r.currentOrigin);

	self->client->hook = hook;

	return hook;
}

/*
=================
fire_c4
=================
*/
gentity_t *fire_c4( gentity_t *self, vec3_t start, vec3_t dir ) {
	gentity_t	*c4;

	VectorNormalize (dir);

	c4 = G_Spawn();
	c4->classname = "c4";
	c4->nextthink = level.time + 3000;
	c4->think = G_ExplodeMissile;
	c4->s.eType = ET_MISSILE;
	c4->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	c4->s.weapon = WP_C4;
	c4->s.eFlags = 0;
	c4->r.ownerNum = self->s.number;
	c4->parent = self;
	c4->damage = 0;
	c4->splashDamage = 200;
	c4->splashRadius = 350;
	c4->methodOfDeath = MOD_C4;
	c4->splashMethodOfDeath = MOD_C4;
	c4->clipmask = MASK_SHOT;
	c4->target_ent = NULL;
	// count is used to check if the prox mine left the player bbox
	// if count == 1 then the prox mine left the player bbox and can attack to it
	c4->count = 0;

	//FIXME: we prolly wanna abuse another field
	c4->s.generic1 = self->client->sess.sessionTeam;

	c4->s.pos.trType = TR_GRAVITY;
	c4->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, c4->s.pos.trBase );
	
    VectorScale( dir, 200, c4->s.pos.trDelta );
	SnapVector( c4->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, c4->r.currentOrigin);

	return c4;
}

