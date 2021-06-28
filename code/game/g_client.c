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

// g_client.c -- client functions that don't happen every frame

static vec3_t	playerMins = {-15, -15, -24};
static vec3_t	playerMaxs = {15, 15, 32};

static void UseSpawn( gentity_t *self, gentity_t *other, gentity_t *activator) {
	if ( !(activator->r.svFlags & SVF_BOT) ){
		self->health = 0;
		level.progressive_spawn = qtrue;
	}
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}

	//Have the spawn point useable.  If it's disabled, then ignore it
	//when trying to find spawnpoints.
	ent->use = UseSpawn;
	G_SpawnInt( "disabled", "0", &i );
	ent->health = i;

	trap_LinkEntity(ent);
}
/*QUAKED info_bot_informant (1 0 1) (-16 -16 -24) (16 16 32)
spawn position for a bot.
"name" : set the name of the bot
*/
void SP_info_bot_start( gentity_t *ent ) {
    char *special;
    G_SpawnString( "special", "", &special);
    if (Q_stricmp(special,"exit_level")==0)
    {
        ent->spawnflags = BSF_END_LEVEL_ON_KILL;
    }
    G_SpawnInt( "skill", "2", &ent->count );
}
/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

}
/*QUAKED info_player_rank (1 0 1) (-16 -16 -24) (16 16 32)
The intermission should look at these.
"rank" set to 1 through 8
*/
void SP_info_player_rank( gentity_t *ent ) {
	G_SpawnInt( "rank", "1", &ent->count );
}
/*QUAKED info_player_exit (1 0 1) (-16 -16 -24) (16 16 32)
spawn position for a player exit.
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/mapobjects/podium/podium4.md3"
*/

void SP_info_player_exit(gentity_t *ent) {
	G_SetOrigin( ent, ent->s.origin );
	ent->s.modelindex = G_ModelIndex( SP_PODIUM_MODEL );
	ent->touch = PlayerExit_Trigger;
	ent->r.contents = CONTENTS_TRIGGER;
	VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
	VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );

	trap_LinkEntity(ent);
}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( vec3_t origin ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( origin, playerMins, mins );
	VectorAdd( origin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client) {
			return qtrue;
		}

	}

	return qfalse;
}
/*
========================

Find a spawn point near some origin
  * expensive, can do up to 8 trap_Traces
========================
*/

qboolean FindSpawnNearOrigin(vec3_t origin, vec3_t dest, int dist, vec3_t mins, vec3_t maxs)
{
	int i,j;
	trace_t tr;
	vec3_t src, dest_trace;
	qboolean found = qfalse;
	
	for (i=-1; i < 2; i++)
	{
		for (j=-1; j < 2; j++)
		{
			float distance;
			if (i==0 && j==0) continue;
			VectorCopy(origin, src);
			src[0] += (i*dist);
			src[1] += (j*dist);
			
			VectorCopy(src, dest_trace);
			src[2] += 32;
			dest_trace[2] -= 4096;
			trap_Trace( &tr, src, mins, maxs, dest_trace, ENTITYNUM_NONE, MASK_PLAYERSOLID );

			// if the whole trace was a solid, dont use this one.
			if ( tr.allsolid ) continue;
			if ( SpotWouldTelefrag( tr.endpos ) ) continue;
			distance = DistanceSquared(origin, tr.endpos);
			//16384 = 128 units away
			if ( distance > 16384.0f ) continue;
			//if we didn't really go anywhere from our start to end, dont use this point.
			distance = DistanceSquared(src, tr.endpos);
			if ( distance < 1.0f ) continue;
			if (DistanceSquared(origin, tr.endpos) > DistanceSquared(origin, dest) ) continue;
			
			//trace from our end position, back to our start, a make sure
			//tr.startsolid is false;
			VectorCopy(tr.endpos, src);
			trap_Trace( &tr, src, mins, maxs, origin, ENTITYNUM_NONE, MASK_PLAYERSOLID );
			if ( tr.startsolid ) continue;

			VectorCopy(tr.endpos, dest);
			found = qtrue;
		}
	}
	return found;
}
/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot->s.origin ) ) {
			continue;
		}
		if ( spot->health != 0 ) continue;

		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}
	VectorCopy( nearestSpot->s.origin, origin );
	origin[2] += 9;
	VectorCopy (nearestSpot->s.angles, angles);
	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint( void ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot->s.origin ) ) {
			continue;
		}
		if ( spot->health != 0 ) continue;
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = Q_rrand( 0, count );
	return spots[ selection ];
}

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectRandomFurthestSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles, char *spawn_class ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[64];
	gentity_t	*list_spot[64];
	int			numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), spawn_class)) != NULL) {
		if ( SpotWouldTelefrag( spot->s.origin ) ) {
			continue;
		}
		if ( spot->health != 0 ) continue;
		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );
		for (i = 0; i < numSpots; i++) {
			if ( dist > list_dist[i] ) {
				if ( numSpots >= 64 )
					numSpots = 64-1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if (numSpots > 64)
					numSpots = 64;
				break;
			}
		}
		if (i >= numSpots && numSpots < 64) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots) {
		spot = G_Find( NULL, FOFS(classname), spawn_class);
		if (!spot)
			G_Error( "Couldn't find a spawn point" );
		VectorCopy (spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);
		return spot;
	}
	//we have X spots, but we want to do 0-(X-1)
	numSpots--;
	// select a random spot from the spawn points furthest away
	rnd = random() * (float)numSpots;

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
// old spawn code
//	return SelectRandomFurthestSpawnPoint( avoidPoint, origin, angles, "info_player_deathmatch" );
//	return SelectNearestDeathmatchSpawnPoint( avoidPoint, origin, angles );
	if (g_gametype.integer == GT_ASSASSINATE && level.progressive_spawn == qtrue) {
		return SelectNearestDeathmatchSpawnPoint( avoidPoint, origin, angles );
	} else {
		gentity_t *spot = SelectRandomDeathmatchSpawnPoint();
		VectorCopy (spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);
		return spot;
	}

	/* older spawn code
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( );
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( );
		}		
	}

	// find a single player start spot
	if (!spot) {
		G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
	*/
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t *SelectInitialSpawnPoint( vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

	spot = NULL;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( spot->spawnflags & 1 ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot->s.origin ) ) {
		return SelectSpawnPoint( vec3_origin, origin, angles );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}
/*
===========
SelectAISpawnPoint
===========
*/
gentity_t *SelectAISpawnPoint( vec3_t origin, vec3_t angles, char *bottype, char *name ){
	gentity_t	*spot = NULL;
	char *spawn_class;
	spawn_class = (bottype && bottype[0])?va("info_bot_%s",bottype):"info_player_deathmatch";
	spot = SelectRandomFurthestSpawnPoint( origin, origin, angles, spawn_class );

	//If we can't find a good spot first time around
	//look again but don't care about the names at the spawn points
	//this will let bots keep spawning once they were in the map once at their starting location
    if (!spot || SpotWouldTelefrag( spot->s.origin ))
    {
        spot = NULL;
        while ((spot = G_Find (spot, FOFS(classname), spawn_class)) != NULL) {
            //Checking to see if the spawn, 
            //and the gametype is not HUB, or the spawnpoint has not been already used.
            //this lets bots respawn in non hub games.
            if ( !(spot->flags & FL_ALREADY_SPAWNED) || g_gametype.integer != GT_HUB ) {
                break;
            }
        }
    }
	if ( !spot || SpotWouldTelefrag( spot->s.origin ) ) {
		return NULL;
	}
	//Set a flag here since we already spawned a bot here
    spot->flags |= FL_ALREADY_SPAWNED;
	VectorCopy (spot->s.origin, origin);
	// don't move the bot up 9 units when spawning
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;

}
/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > 6500 ) {
		// the body ques are never actually freed, they are just unlinked
		trap_UnlinkEntity( ent );
		ent->physicsObject = qfalse;
		if ( ent->r.svFlags & SVF_BOT && ent->dont_respawn == qtrue) {
			trap_DropClient(ent->r.ownerNum, "Done with this bot");
		}
		return;	
	}
	ent->nextthink = level.time + 100;
	ent->s.pos.trBase[2] -= 1;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue( gentity_t *ent ) {
	gentity_t		*body;
	int			contents;

	trap_UnlinkEntity (ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	trap_UnlinkEntity (body);

	body->s = ent->s;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc
	body->s.shields = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch ( body->s.legsAnim ) {
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + 5000;
	body->think = BodySink;
	
	body->dont_respawn = ent->dont_respawn;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}


	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap_LinkEntity (body);
}

//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

/*
================
respawn
================
*/
void respawn( gentity_t *ent ) {
	gentity_t	*tent;

	CopyToBodyQue (ent);
	ClientSpawn(ent, NULL);
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		// add a teleportation effect
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;
	}
}

/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount( int ignoreClientNum, int team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			count++;
		}
	}

	return count;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader( int team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			if ( level.clients[i].sess.teamLeader )
				return i;
		}
	}

	return -1;
}


/*
================
PickTeam

================
*/
team_t PickTeam( int ignoreClientNum ) {
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );

	if ( counts[TEAM_BLUE] > counts[TEAM_RED] ) {
		return TEAM_RED;
	}
	if ( counts[TEAM_RED] > counts[TEAM_BLUE] ) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] ) {
		return TEAM_RED;
	}
	return TEAM_BLUE;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = Q_strrchr(model, '/')) != 0) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}
*/

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum )
{
	gentity_t *ent;
	int		teamTask, teamLeader, team;
	char	*s;
	char	model[MAX_QPATH];
	char	headModel[MAX_QPATH];
	char	oldname[MAX_STRING_CHARS];
	gclient_t	*client;
	char	c1[MAX_INFO_STRING];
	char	c2[MAX_INFO_STRING];
	char	userinfo[MAX_INFO_STRING];
	int		i;
	gclient_t *tempClient;

	bool	changedUserInfo = false;

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate(userinfo) ) {
		strcpy (userinfo, "\\name\\badinfo");
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) ) {
		client->pers.localClient = qtrue;
	}

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) ) {
		client->pers.predictItemPickup = qfalse;
	} else {
		client->pers.predictItemPickup = qtrue;
	}

	// set name
	Q_strncpyz ( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey( userinfo, "name" );
	Q_CleanPlayerName( s, client->pers.netname, sizeof(client->pers.netname) );

	if( strcmp( client->pers.netname, oldname ) != 0 )
	{
		Info_SetValueForKey( userinfo, "name", client->pers.netname );
		changedUserInfo = true;
	}

	for (i=0; i < MAX_PLAYERS; i++)
	{
		tempClient = level.clients +i;
		if (tempClient != NULL && tempClient->pers.connected != CON_DISCONNECTED && client != tempClient) {
			if (strcmp(client->pers.netname, tempClient->pers.netname) == 0 ) {
				Q_strncpyz( client->pers.netname, va("%s(%d)", client->pers.netname, client-level.clients), sizeof(client->pers.netname));
			}
		}
	}

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			Q_strncpyz( client->pers.netname, "scoreboard", sizeof(client->pers.netname) );
		}
	}

	if ( client->pers.connected == CON_CONNECTED ) {
		if ( strcmp( oldname, client->pers.netname ) ) {
			trap_SendServerCommand( -1, va("print \"%s renamed to %s\n\"", oldname, 
				client->pers.netname) );
		}
	}

	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// set model
	Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );
	Q_strncpyz( headModel, Info_ValueForKey (userinfo, "headmodel"), sizeof( headModel ) );

	// bots set their team a few frames later
	if (g_gametype.integer >= GT_TEAM && g_entities[clientNum].r.svFlags & SVF_BOT) {
		s = Info_ValueForKey( userinfo, "team" );
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}
	}
	else {
		team = client->sess.sessionTeam;
	}

/*	NOTE: all client side now

	// team
	switch( team ) {
	case TEAM_RED:
		ForceClientSkin(client, model, "red");
//		ForceClientSkin(client, headModel, "red");
		break;
	case TEAM_BLUE:
		ForceClientSkin(client, model, "blue");
//		ForceClientSkin(client, headModel, "blue");
		break;
	}
	// don't ever use a default skin in teamplay, it would just waste memory
	// however bots will always join a team but they spawn in as spectator
	if ( g_gametype.integer >= GT_TEAM && team == TEAM_SPECTATOR) {
		ForceClientSkin(client, model, "red");
//		ForceClientSkin(client, headModel, "red");
	}
*/

	if (g_gametype.integer >= GT_TEAM) {
		client->pers.teamInfo = qtrue;
	} else {
		s = Info_ValueForKey( userinfo, "teamoverlay" );
		if ( ! *s || atoi( s ) != 0 ) {
			client->pers.teamInfo = qtrue;
		} else {
			client->pers.teamInfo = qfalse;
		}
	}
	/*
	s = Info_ValueForKey( userinfo, "cg_pmove_fixed" );
	if ( !*s || atoi( s ) == 0 ) {
		client->pers.pmoveFixed = qfalse;
	}
	else {
		client->pers.pmoveFixed = qtrue;
	}
	*/

	// team task (0 = none, 1 = offence, 2 = defence)
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// colors
	strcpy(c1, Info_ValueForKey( userinfo, "color1" ));
	strcpy(c2, Info_ValueForKey( userinfo, "color2" ));

	if ( ent->r.svFlags & SVF_BOT )
		ent->client->pers.npc_id = atoi( Info_ValueForKey( userinfo, "id" ) );
	else
		ent->client->pers.npc_id = -1;

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	if ( ent->r.svFlags & SVF_BOT ) {
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d\\id\\%d\\client\\%d",
			client->pers.netname, team, model, headModel, c1, c2, 
			client->pers.maxHealth, client->sess.wins, client->sess.losses,
			Info_ValueForKey( userinfo, "skill" ), teamTask, teamLeader, client->pers.npc_id,
			clientNum );
	} else {
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d\\b\\-1\\client\\%d",
			client->pers.netname, client->sess.sessionTeam, model, headModel, c1, c2, 
			client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader,
			clientNum);
	}

	trap_SetConfigstring( CS_PLAYERS+clientNum, s );

	// Guard this so we dont blow up with a Z_malloc error
	sql( "UPDATE OR INSERT clients CS $ WHERE client=?;", "ti", s, clientNum );

	// this is not the userinfo, more like the configstring actually
	G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );

	if( changedUserInfo )
		trap_SetUserinfo( clientNum, userinfo );
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char		*value;
//	char		*areabits;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	gentity_t	*ent;

	ent = &g_entities[ clientNum ];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

 	// IP filtering
 	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
 	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
 	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	if ( G_FilterPacket( value ) ) {
		return "You are banned from this server.";
	}

  // we don't check password for bots and local client
  // NOTE: local client <-> "ip" "localhost"
  //   this means this client is not running in our current process
	if ( !( ent->r.svFlags & SVF_BOT ) && (strcmp(value, "localhost") != 0)) {

		// only check if it's your first time to connect
		if ( firstTime && BG_GameInProgress( &gs ) )
			return "This game is already in progress.";

		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) &&
			strcmp( g_password.string, value) != 0) {
			return "Invalid password";
		}
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

//	areabits = client->areabits;

	memset( client, 0, sizeof(*client) );

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if ( firstTime ) {
		G_InitSessionData( client, userinfo );
	}

	if ( !isBot )
		G_ReadSessionData( client );

	//Overwrite their teams depending on game type
	switch ( g_gametype.integer )
	{
	case GT_MOA:
	case GT_PIRATE:
	case GT_ASSASSINATE:
		client->sess.sessionTeam = (isBot)?TEAM_RED:TEAM_BLUE;
		break;
	case GT_HUB:
		client->sess.sessionTeam = TEAM_RED;
		break;
	case GT_HOSTAGE:
    	//if it's not a bot, or if it's a hostage, put it on the blue team.
    	if ( !isBot || Q_stricmp(Info_ValueForKey(userinfo, "bottype"), "hostage") == 0 ){
    		client->sess.sessionTeam = TEAM_BLUE;
    	} else {
    		client->sess.sessionTeam = TEAM_RED;
    	}
    	break;
    }

	if( isBot ){
	    if (!g_botsReconnect.integer && !firstTime) return "";
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if( !G_BotConnect( clientNum, !firstTime ) ) {
			return "BotConnectfailed";
		}
	}
	G_ST_exec( ST_CLIENT_CONNECT, clientNum );
	
	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	ClientUserinfoChanged( clientNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime && !(ent->r.svFlags & SVF_BOT) )
	{
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname) );
	}

	if ( g_gametype.integer >= GT_TEAM &&
		client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	// for statistics
//	client->areabits = areabits;
//	if ( !client->areabits )
//		client->areabits = G_Alloc( (trap_AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 );
	return NULL;
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum, gentity_t *spawn ) {
	gentity_t	*ent;
	gclient_t	*client;
	//gentity_t	*tent;
	int			flags, score;

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	//Also, keep the players score
	score = client->ps.persistant[PERS_SCORE];
	
	
	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;
    client->ps.persistant[PERS_SCORE] = score;

	// locate ent at a spawn point
	ClientSpawn( ent, spawn );

	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		// send event
		//tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		//tent->s.clientNum = ent->s.clientNum;

		if ( !(ent->r.svFlags & SVF_BOT) )
		{
			trap_SendServerCommand( -1, va("print \"" S_COLOR_WHITE "%s has arrived.\n\"", client->pers.netname) );
		}
	}
	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn(gentity_t *ent, gentity_t *spawn) {
	int		index;
	vec3_t	spawn_origin, spawn_angles, dest;
	gclient_t	*client;
	int		i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int		persistant[MAX_PERSISTANT];
	gentity_t	*spawnPoint;
	int		flags;
	int		savedPing;
//	char	*savedAreaBits;
	int		accuracy_hits, accuracy_shots;
	int		eventSequence;
	char	userinfo[MAX_INFO_STRING];
	int 	health;
	trace_t	tr;
	int width;


	index = ent - g_entities;
	client = ent->client;

	trap_GetUserinfo( index, userinfo, sizeof(userinfo) );
	
	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( spawn ) {
		spawnPoint = spawn;
		if (!SpotWouldTelefrag( spawn->s.origin ) ) {
			VectorCopy( spawn->s.origin, spawn_origin );
			spawn_origin[2] += 9;
			VectorCopy( spawn->s.angles, spawn_angles );
		} else {
			Com_Error( ERR_DROP, "Spawn point specified[%s], but %s would cause a telefrag\n", spawn->targetname, Info_ValueForKey(userinfo,"name") );
		}
		
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		spawnPoint = SelectSpectatorSpawnPoint ( 
						spawn_origin, spawn_angles);
	} 
	//If we are using map spawn points and it's a bot that is spawning
	else if ( index >= MAX_PLAYERS && g_usemapspawns.integer == 1 )
	{
	   spawnPoint = SelectAISpawnPoint(spawn_origin, spawn_angles, Info_ValueForKey(userinfo,"bottype"), Info_ValueForKey(userinfo,"name"));
	} 
	else /* if (g_gametype.integer >= GT_CTF ) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint ( 
						client->sess.sessionTeam, 
						client->pers.teamState.state, 
						spawn_origin, spawn_angles);
	} else */{
		do {
			// the first spawn should be at a good looking spot
				// don't spawn near existing origin if possible
			spawnPoint = SelectSpawnPoint ( 
				client->ps.origin, 
				spawn_origin, spawn_angles);

			// Tim needs to prevent bots from spawning at the initial point
			// on q3dm0...
			if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}
			// just to be symetric, we have a nohumans option...
			if ( ( spawnPoint->flags & FL_NO_HUMANS ) && !( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}

			break;

		} while ( 1 );
	}
	if ( spawnPoint == NULL && client->sess.sessionTeam != TEAM_SPECTATOR ) return;

	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	flags = ent->client->ps.eFlags & EF_TELEPORT_BIT;
	flags ^= EF_TELEPORT_BIT;

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = client->ps.persistant[i];
	}
	eventSequence = client->ps.eventSequence;

	memset (client, 0, sizeof(*client)); // bk FIXME: Com_Memset?

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->lastkilled_client = -1;
	
	if (g_gametype.integer != GT_HUB )
		client->ps.pm_flags |= PMF_ANGRY;  //set the angry bit if we are not in a hub

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
    client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;
	

	client->airOutTime = level.time + 12000;

	health = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( BotGetType( index ) != BOT_BOSS) {
		client->pers.maxHealth = health;
	}
	if ( (client->pers.maxHealth < 1 || client->pers.maxHealth > 100) && ( ent->r.svFlags & SVF_BOT ) ) {
		client->pers.maxHealth = 100;
	}
	// set max health
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	
	client->ps.eFlags = flags;

	if ( client->sess.sessionTeam != TEAM_SPECTATOR && G_ST_exec( ST_CLIENT_RESPAWN, ent->s.number, client->ps.persistant[PERS_SPAWN_COUNT]) == 1 ) {
		SetTeam( ent, "s");
		return;
	}


	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;
	
	
	width = atoi( Info_ValueForKey( userinfo, "width" ) );
	client->ps.persistant[PERS_WIDTH]=width;
	if ( width == 0.0f ) {
		VectorCopy (playerMins, ent->r.mins);
		VectorCopy (playerMaxs, ent->r.maxs);
	} else {
		VectorCopy (playerMins, ent->r.mins);
		VectorCopy (playerMaxs, ent->r.maxs);
		ent->r.mins[0] = -1 * width;
		ent->r.mins[1] = -1 * width;
		
		ent->r.maxs[0] = width;
		ent->r.maxs[1] = width;
	}

	client->ps.clientNum = index;

	if (g_gametype.integer == GT_HUB)
	{
	    client->ps.stats[STAT_WEAPONS] = WP_NONE;
	}else{
	    client->ps.stats[STAT_WEAPONS] = ( 1 << WP_HIGHCAL );
	    client->ps.ammo[WP_HIGHCAL] = G_MaxTotalAmmo(WP_HIGHCAL);
	    client->ammo_in_clip[WP_HIGHCAL] = G_ClipAmountForWeapon(WP_HIGHCAL);

		if ( ent->r.svFlags & SVF_BOT ) 
		{
			switch ( BotGetType(ent-g_entities) )
			{
			case BOT_BOSS:
				client->ps.shields = client->ps.stats[STAT_MAX_SHIELDS];
			case BOT_GUARD_HARD:
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SHOTGUN );
				client->ps.ammo[WP_SHOTGUN] = G_MaxTotalAmmo(WP_SHOTGUN);
				client->ammo_in_clip[WP_SHOTGUN] = 12;
			case BOT_GUARD_MEDIUM:
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_ASSAULT );
				client->ps.ammo[WP_ASSAULT] = G_MaxTotalAmmo(WP_ASSAULT);
				client->ammo_in_clip[WP_ASSAULT] = G_ClipAmountForWeapon(WP_ASSAULT);
			case BOT_GUARD_EASY:
				client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_PISTOL );
				client->ps.ammo[WP_PISTOL] = G_MaxTotalAmmo(WP_PISTOL);
				client->ammo_in_clip[WP_PISTOL] = G_ClipAmountForWeapon(WP_PISTOL);
			}
		}

		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_GAUNTLET );
		client->ps.ammo[WP_GAUNTLET] = G_MaxTotalAmmo(WP_GAUNTLET);
		client->ammo_in_clip[WP_GAUNTLET] = -1;
		client->ps.ammo[WP_GRAPPLING_HOOK] = G_MaxTotalAmmo(WP_GRAPPLING_HOOK);;
	}
	
	client->ps.stats[STAT_HANDICAP] = atoi( Info_ValueForKey( userinfo, "handicap" ) );

	VectorSet( dest, spawn_origin[0], spawn_origin[1], spawn_origin[2] - 4096 );
	trap_Trace( &tr, spawn_origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_PLAYERSOLID );

	if (tr.startsolid == qfalse && tr.allsolid == qfalse ) {
		G_SetOrigin( ent, tr.endpos );
		VectorCopy( tr.endpos, client->ps.origin );
	} else {
		G_SetOrigin( ent, spawn_origin );
		VectorCopy( spawn_origin, client->ps.origin );
	}

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );
	if (ent->r.svFlags & SVF_BOT)
	{
		BotSetIdealViewAngle( index, spawn_angles );
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {

	} else {
		G_KillBox( ent );
		trap_LinkEntity (ent);

		// force the base weapon up
		client->ps.weapon = WP_PISTOL;
		client->ps.weaponstate = WEAPON_READY;
		client->ps.stats[STAT_AMMO] = client->ammo_in_clip[WP_PISTOL];

	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	// set default animations
	
	if (g_gametype.integer != GT_HUB) {
		client->ps.torsoAnim = TORSO_STAND;
	} else {
		client->ps.torsoAnim = LEGS_IDLE;
	}
	client->ps.legsAnim = LEGS_IDLE;
	
	if ( level.intermissiontime && gs.INPLAY ) {
		MoveClientToIntermission( ent );
	} else {
		// fire the targets of the spawn point
		G_UseTargets( spawnPoint, ent );

		// select the highest weapon number available, after any
		// spawn given items have fired
		if (g_gametype.integer == GT_HUB){
		    client->ps.weapon = WP_NONE;
		}else{
		    client->ps.weapon = WP_GAUNTLET;
		}
		for ( i = WP_NUM_WEAPONS - 1 ; i > 0 ; i-- ) {
			if ( client->ps.stats[STAT_WEAPONS] & ( 1 << i ) ) {
				client->ps.weapon = i;
				break;
			}
		}
	}

	G_ST_exec( (ent->r.svFlags & SVF_BOT )?ST_SPAWN_BOT:ST_SPAWN_PLAYER, index, spawnPoint );

	//verify the health, G_ST_exec cuts out firhg away if the game isnt' loaded
	if ( client->pers.maxHealth  <= 0 || client->ps.stats[STAT_HEALTH] <= 0 || client->ps.stats[STAT_MAX_HEALTH] <= 0 )
	{
		ent->health =
		client->ps.stats[STAT_HEALTH] =
		client->ps.stats[STAT_MAX_HEALTH] =
		client->pers.maxHealth = 100;
	}
	
	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities );

	// positively link the client, even if the command times are weird
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
	
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED 
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR && !(ent->r.svFlags & SVF_BOT) ) {
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = ent->s.clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems( ent );

	}

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

	// if we are playing in tourney mode and losing, give a win to the other player
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& !level.intermissiontime
		&& !level.warmupTime && level.sortedClients[1] == clientNum ) {
		level.clients[ level.sortedClients[0] ].sess.wins++;
		ClientUserinfoChanged( level.sortedClients[0] );
	}

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring( CS_PLAYERS + clientNum, "");
	sql( "DELETE FROM clients WHERE client=?;", "ie", clientNum );

	CalculateRanks();

	if ( ent->r.svFlags & SVF_BOT ) {
		BotAIShutdownClient( clientNum, qfalse );
	}

	G_ST_exec( ST_CLIENT_DISCONNECT, clientNum );
}


