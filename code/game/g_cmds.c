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

#include "../ui/menudef.h"			// for the voice chats


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
}



/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return qfalse;
	}

	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You must be alive to use this command.\n\""));
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			trap_SendServerCommand( to-g_entities, va("print \"Bad client slot: %i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client %i is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to-g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (gentity_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	name = ConcatArgs( 1 );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_NUM_WEAPONS) - 1 - 
			( 1 << WP_GRAPPLING_HOOK ) - ( 1 << WP_NONE );
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = G_MaxTotalAmmo(i);
            ent->client->ammo_in_clip[i] = G_ClipAmountForWeapon(i);
		}
		ent->client->ammo_in_clip[WP_SHOTGUN] = 12;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		ent->client->ps.shields = 200;

		if (!give_all)
			return;
	}

	if (Q_stricmp(name, "excellent") == 0) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		if ( G_SpawnItem (it_ent, it) )
		{
			FinishSpawningItem(it_ent );
			memset( &trace, 0, sizeof( trace ) );
			Touch_Item (it_ent, ent, &trace);
			if (it_ent->inuse) {
				G_FreeEntity( it_ent );
			}
		} else
			G_FreeEntity( it_ent );

	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (gentity_t *ent)
{
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent-g_entities, 
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}



/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	if (ent->health <= 0) {
		return;
	}
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	/*		!! disabled for Space Traders !!
	if ( client->sess.sessionTeam == TEAM_RED ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the red team.\n\"",
			client->pers.netname) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the blue team.\n\"",
		client->pers.netname));
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the spectators.\n\"",
		client->pers.netname));
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the battle.\n\"",
		client->pers.netname));
	}
	*/
}

/*
=================
SetTeam
=================
*/
void SetTeam( gentity_t *ent, char *s ) {
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	// override decision if limiting the players
	if ( (g_gametype.integer == GT_TOURNAMENT)
		&& level.numNonSpectatorClients >= 2 ) {
		team = TEAM_SPECTATOR;
	} else if ( g_maxGameClients.integer > 0 && 
		level.numNonSpectatorClients >= g_maxGameClients.integer ) {
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return;
	}

	//
	// execute the team change
	//

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		CopyToBodyQue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		ent->client->pers.lives = 0;
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);

	}
	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		client->sess.spectatorTime = level.time;
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	client->sess.teamLeader = qfalse;
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

	BroadcastTeamChange( client, oldTeam );

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	ClientBegin( clientNum, NULL );
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent-g_entities, "print \"Blue team\n\"" );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent-g_entities, "print \"Red team\n\"" );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent-g_entities, "print \"Free team\n\"" );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent-g_entities, "print \"Spectator team\n\"" );
			break;
		}
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		trap_SendServerCommand( ent-g_entities, "print \"May not switch teams more than once per 5 seconds.\n\"" );
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s );

	ent->client->switchTeamTime = level.time + 5000;
}


/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int		clientnum;
	int		original;
	int		checks;

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	checks=0;
	do {
		clientnum += dir;
		checks++;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}
		if (checks >= level.maxclients) break;

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		
		// can't follow bots
		if ( clientnum >= 8 && ent->client->ps.clientNum < 8 ) {
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*
==================
G_Say
==================
*/

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ) {
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( other->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	if ( mode == SAY_TEAM  && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		return;
	}

	trap_SendServerCommand( other-g_entities, va("%s %d \"%s%c%c%s\"", 
		mode == SAY_TEAM ? "tchat" : "chat", ent->s.clientNum,
		name, Q_COLOR_ESCAPE, color, message));
}

#define EC		"\x19"

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%s)"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && g_gametype.integer >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"] (%s)"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		break;
	}

	Q_strncpyz( text, chatText, sizeof(text) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text );
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Say( ent, NULL, mode, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}

static void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
	int color;
	char *cmd;

	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( mode == SAY_TEAM && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if ( (g_gametype.integer == GT_TOURNAMENT )) {
		return;
	}

	if (mode == SAY_TEAM) {
		color = COLOR_CYAN;
		cmd = "vtchat";
	}
	else if (mode == SAY_TELL) {
		color = COLOR_MAGENTA;
		cmd = "vtell";
	}
	else {
		color = COLOR_GREEN;
		cmd = "vchat";
	}

	trap_SendServerCommand( other-g_entities, va("%s %d %d %d %s", cmd, voiceonly, ent->s.number, color, id));
}

void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
	int			j;
	gentity_t	*other;

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->pers.netname, id);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_VoiceTo( ent, other, mode, id, voiceonly );
	}
}

/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Voice( ent, NULL, mode, p, voiceonly );
}

/*
==================
Cmd_VoiceTell_f
==================
*/
static void Cmd_VoiceTell_f( gentity_t *ent, qboolean voiceonly ) {
	int			targetNum;
	gentity_t	*target;
	char		*id;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	id = ConcatArgs( 2 );

	G_LogPrintf( "vtell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, id );
	G_Voice( ent, target, SAY_TELL, id, voiceonly );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Voice( ent, ent, SAY_TELL, id, voiceonly );
	}
}


/*
==================
Cmd_VoiceTaunt_f
==================
*/
static void Cmd_VoiceTaunt_f( gentity_t *ent ) {
	gentity_t *who;
	int i;

	if (!ent->client) {
		return;
	}

	// insult someone who just killed you
	if (ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == ent->s.number) {
		// i am a dead corpse
		if (!(ent->enemy->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent->enemy, SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		if (!(ent->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent,        SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		ent->enemy = NULL;
		return;
	}
	// insult someone you just killed
	if (ent->client->lastkilled_client >= 0 && ent->client->lastkilled_client != ent->s.number) {
		who = g_entities + ent->client->lastkilled_client;
		if (who->client) {
			// who is the person I just killed
			if (who->client->lasthurt_mod == MOD_GAUNTLET) {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );	// and I killed them with a gauntlet
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );
				}
			} else {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );	// and I killed them with something else
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );
				}
			}
			ent->client->lastkilled_client = -1;
			return;
		}
	}

	if (g_gametype.integer >= GT_TEAM) {
		// praise a team mate who just got a reward
		for(i = 0; i < MAX_CLIENTS; i++) {
			who = g_entities + i;
			if (who->client && who != ent && who->client->sess.sessionTeam == ent->client->sess.sessionTeam) {
				if (who->client->rewardTime > level.time) {
					if (!(who->r.svFlags & SVF_BOT)) {
						G_Voice( ent, who, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					if (!(ent->r.svFlags & SVF_BOT)) {
						G_Voice( ent, ent, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					return;
				}
			}
		}
	}

	// just say something
	G_Voice( ent, NULL, SAY_ALL, VOICECHAT_TAUNT, qfalse );
}



static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order > sizeof(gc_orders)/sizeof(char *) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
}

void G_Vote_SetDescription( int client, const char * cmd, const char * arg, const char * arg2 )
{
	char		t[ MAX_INFO_STRING ];
	t[ 0 ] = '\0';

	Info_SetValueForKey( t, "c", va("%d",client) );
	Info_SetValueForKey( t, "t", cmd );
	Info_SetValueForKey( t, "a", arg );
	Info_SetValueForKey( t, "a2", arg2 );

	trap_SetConfigstring( CS_VOTE_STATE, t );
}

void G_Vote_SetPassedScript( const char * format, ... )
{
	va_list		argptr;

	va_start	( argptr, format );
	vsnprintf	( level.votePassedScript, sizeof(level.votePassedScript), format, argptr );
	va_end		( argptr );
}

void G_Vote_SetFailedScript( const char * format, ... )
{
	va_list		argptr;

	va_start	( argptr, format );
	vsnprintf	( level.voteFailedScript, sizeof( level.voteFailedScript), format, argptr );
	va_end		( argptr );
}




/*
==================
Cmd_CallVote_f
==================
*/
void CallVote( int client, const char* cmd, const char* arg )
{
	int votesleft;
	gentity_t *ent = g_entities + client;

	if( level.voteTime || level.voteExecuteTime )
	{
		trap_SendServerCommand( ent - g_entities, "ui_vote 2 vote_inprogress" );
		return;
	}
	
	if( ent->client->pers.voteCount >= MAX_VOTE_COUNT )
	{
		trap_SendServerCommand( ent - g_entities, "ui_vote 2 vote_maxvotes" );
		return;
	}

	if( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		trap_SendServerCommand( ent - g_entities, "ui_vote 2 vote_spectating" );
		return;
	}

	{
		int delay = level.voteFailedTime - level.time;
		if ( delay > 0 )
		{
			trap_SendServerCommand( ent - g_entities, va( "ui_vote 2 vote_soon %d", delay ) );
			return;
		}
	}

	if( strchr( cmd, ';' ) || strchr( arg, ';' ) )
	{
		trap_SendServerCommand( ent - g_entities, "ui_vote 2 vote_invalidstring" );
		return;
	}

	level.voteExecuteTime = 0;

	switch( SWITCHSTRING( cmd ) )
	{
	case CS('a','s','s','a'):	//	assassinate
		{
			int player_count = 0;
			sql( "SELECT COUNT(*) FROM players;", "sId", &player_count );
			level.voteExecuteTime = (player_count > 1) ? 1 : 0;
		}
		//fallthrough

	case CS('k','i','c','k'):	//	kick
	case CS('t','r','a','v'):	//	travel
	case CS('r','e','s','c'):	//	rescue
		{
			if( g_gametype.integer != GT_HUB )
			{
				trap_SendServerCommand( ent - g_entities, "ui_vote 2 vote_nothub" );
				return;
			}
		} break;

	}

	//	reset the votes
	sql( "UPDATE players SET vote=?1;", "i", GS_VOTENONE );


	switch( SWITCHSTRING( cmd ) )
	{
		
	case CS('k','i','c','k'):	//	kick
	case CS('r','e','s','c'):	//	rescue
	case CS('a','b','o','r'):	//	abort
		sql( "UPDATE players SET vote=?2 SEARCH client ?1;", "iie", client, GS_VOTEYES );
		G_Vote_SetDescription	( client, cmd, arg, "" );
		G_Vote_SetPassedScript	( "%s \"%s\"", cmd, arg );
		break;

	default:
		if ( !G_ST_exec( ST_VOTE, client, cmd, arg ) )
		{
			trap_SendServerCommand( ent - g_entities, "ui_vote 2 vote_invalidstring" );
			trap_SendServerCommand( ent - g_entities, "print \"Vote commands are: map_restart, nextmap, map <mapname>, g_gametype <n>, kick <player>, clientkick <clientnum>, g_doWarmup, timelimit <time>, fraglimit <frags>.\n\"" );
			return;
		}
	}


	//	force vote dialog up on all clients that haven't voted
	gs.MENU = SWITCHSTRING(cmd);

	votesleft=0;
	while ( sql( "SELECT client FROM players WHERE vote=?;", "i", GS_VOTENONE ) )
	{
		//int client = sqlint(0);
		//trap_SendServerCommand( client, va("ui_vote 1 %s",cmd) );	//	ask client to vote
		votesleft++;
	}

	if ( votesleft == 0 ) {
		// no one needs to vote, execute script right now
		trap_SendConsoleCommand( EXEC_NOW, level.votePassedScript );
		return;
	}

	//	start the voting, the caller automatically votes yes
	level.voteTime = level.time;

	//	tell the clients that have not voted yet to vote
	while( sql( "SELECT client FROM players WHERE vote=?;", "i", GS_VOTENONE ) )
	{
		trap_SendServerCommand( sqlint(0), "st_announcer vote_now 1000\n");
	}
}

void Cmd_CallVote_f( gentity_t *ent )
{
	char	cmd[ MAX_STRING_TOKENS ];
	char	arg[ MAX_STRING_TOKENS ];
	int		c = trap_Argc();
	int		i;

	trap_Argv( 1, cmd, sizeof( cmd ) );

	arg[ 0 ] = '\0';
	for ( i=2; i<c; i++ )
	{
		char tmp[ MAX_STRING_TOKENS ];
		trap_Argv( i, tmp, sizeof( tmp ) );
		Q_strcat( arg, sizeof(arg), va("\"%s\" ",tmp) );
	}

	CallVote( ent->client->ps.clientNum, cmd, arg );
}


/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent )
{
	char msg[64];
	int client = ent - g_entities;
	int vote;

	if( !level.voteTime )
	{
		trap_SendServerCommand( client, "st_cp T_No_Vote_In_Progress\n" );
		return;
	}

	if ( sql( "SELECT 1 FROM players SEARCH client ?1 WHERE vote!=?2;", "ii", client, GS_VOTENONE ) )
	{
		sqldone();
		trap_SendServerCommand( client, "st_cp T_Voted_Already\n" );
		return;
	}
	
	if( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		trap_SendServerCommand( ent - g_entities, "st_cp T_Not_Allowed\n" );
		return;
	}

	trap_Argv( 1, msg, sizeof( msg ) );

	switch( msg[ 0 ] )
	{
	case 'y':
	case 'Y':
	case '1':
		vote = GS_VOTEYES;
		break;

	case 'a':
	case 'A':
		vote = GS_VOTEABSTAIN;
		break;

	default:
		vote = GS_VOTENO;
		break;
	}

	sql( "UPDATE players SET vote=?2 SEARCH client ?1;", "iie", client, vote );

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles, qtrue );
}



/*
=================
Cmd_Stats_f
=================
*/
void Cmd_Stats_f( gentity_t *ent ) {
/*
	int max, n, i;

	max = trap_AAS_PointReachabilityAreaIndex( NULL );

	n = 0;
	for ( i = 0; i < max; i++ ) {
		if ( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
			n++;
	}

	//trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
	trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
*/
}


//
//	This is called to answer a bot's question from the Dialog box.
//
void Cmd_Holster_Weapon_f( gentity_t *ent )
{
    int weaponstate;
    weaponstate = ent->client->ps.weaponstate;
    if (ent->client->ps.weaponstate != WEAPON_HOLSTERED){
        ent->client->ps.weaponstate = WEAPON_HOLSTERING;
    }
}
void Cmd_UnHolster_Weapon_f( gentity_t *ent )
{
    int weaponstate;
    weaponstate = ent->client->ps.weaponstate;
    if (ent->client->ps.weaponstate == WEAPON_HOLSTERED){
        ent->client->ps.weaponstate = WEAPON_UNHOLSTERING;
    }
}

int ReloadTimeForWeapon(int weapon)
{
    switch(weapon)
    {
    case WP_HIGHCAL: return 1857;
    case WP_SHOTGUN: return 667;
    case WP_ASSAULT: return 2400;
	case WP_PISTOL: return 2000;
    default: return 2500;
    }
}
/*
==================
  Cmd_Reload
==================
*/

void Cmd_Reload_finish( gentity_t *ent )
{
	int weapon;

	weapon = ent->client->ps.weapon;

	if ( ent->client->ps.ammo[weapon] == 0) return;

	if ( (weapon == WP_SHOTGUN && ent->client->ammo_in_clip[weapon] < 12) || ( ent->client->ammo_in_clip[weapon] < G_ClipAmountForWeapon(weapon)  ) )
	{
		// Don't actually remove ammo if it's a bot
		if ( !(ent->r.svFlags & SVF_BOT) ) {
			//Remove the ammo from bag
			ent->client->ps.ammo[weapon] -= G_ClipAmountForWeapon( weapon );
			if ( ent->client->ps.ammo[weapon] < 0) ent->client->ps.ammo[weapon] = 0;
		}
		//Add the ammo to weapon
		ent->client->ammo_in_clip[weapon] += G_ClipAmountForWeapon(weapon);
		if (ent->client->ammo_in_clip[weapon] > G_ClipAmountForWeapon(weapon) && G_ClipAmountForWeapon(weapon) > 1) ent->client->ammo_in_clip[weapon] = G_ClipAmountForWeapon(weapon);
	}
}

void Cmd_Reload_f( gentity_t *ent, qboolean first )	{
	int weapon;

	weapon = ent->client->ps.weapon;

    if ( (ent->client->ps.weaponstate == WEAPON_RELOADING || ent->client->ps.weaponstate == WEAPON_RELOADING_QUICK ) && weapon != WP_SHOTGUN)
    {
        //If we are already reloading, don't let them reload again.
        return;
    }

	if ( ( weapon != WP_SHOTGUN && ent->client->ammo_in_clip[weapon] >= G_ClipAmountForWeapon(weapon) ) || 
		(weapon == WP_SHOTGUN && ent->client->ammo_in_clip[weapon] >= 12 ) )	{
		ent->client->ps.weaponstate = WEAPON_READY;
		trap_SendServerCommand( ent-g_entities,
			va("print \"No need to reload.\n\""));
		return;
	}
	if (ent->client->ps.ammo[weapon] == 0)	{
		trap_SendServerCommand( ent-g_entities,
			va("print \"You are out of clips.\n\""));
		return;
	}

	if (ent->client->ps.ammo[weapon] == 0 || (ent->client->ps.weaponstate != WEAPON_EMPTY && weapon == WP_SHOTGUN && ent->client->ps.weaponTime > 0) ) return;

	//if its the assault rifle, then reset our zoom
	if ( ent->client->ps.weapon == WP_ASSAULT )
	{
    	ent->client->weaponStates[WP_ASSAULT] = qfalse;
    	ent->client->ps.stats[STAT_MODE] = qfalse;
	}

    
    //ent->client->ps.torsoAnim = TORSO_DROP;
	//If we are not doing quick reloads, or the gun is full, do the full reload animation
	if ( weapon != WP_SHOTGUN ) {
		ent->client->ps.weaponTime += ReloadTimeForWeapon(weapon);
		ent->client->ps.weaponstate = WEAPON_RELOADING;
	} else {
		if ( (ent->client->ammo_in_clip[weapon] == 11 || ent->client->ps.ammo[weapon] == 1) && (ent->client->ps.weaponstate != WEAPON_RELOADING && ent->client->ps.weaponstate != WEAPON_RELOADING_QUICK)) {
			ent->client->ps.weaponstate = WEAPON_RELOADING;
			ent->client->ps.weaponTime = 1833;
		} else {
			ent->client->ps.weaponstate = WEAPON_RELOADING_QUICK;
			//700 ms per shell we add
			ent->client->ps.weaponTime += 667;
		}
		//Atomic reloads with the shotgun, each reload is only one shell, so do a reload finish for it
		Cmd_Reload_finish( ent );
		if ( ent->client->ammo_in_clip[weapon] == 11 && ent->client->ps.weaponstate != WEAPON_RELOADING) {
			ent->client->ps.weaponTime = 1833;
		}
	}
    
}

void Cmd_Hostage_f( gentity_t * ent )
{
}

void Cmd_Location_f( gentity_t *ent )
{
    //G_Printf("Location: [%.1f,%.1f,%.1f]\n", ent->client->ps.origin[0],ent->client->ps.origin[1],ent->client->ps.origin[2]);
    //G_Printf("Angle: [%.1f,%.1f,%.1f]\n", ent->client->ps.viewangles[0],ent->client->ps.viewangles[1],ent->client->ps.viewangles[2]);

	G_Printf( "\"origin\" \"%.1f %.1f %.1f\"\n\"angle\" \"%.1f\"\n", ent->client->ps.origin[0],ent->client->ps.origin[1],ent->client->ps.origin[2],ent->client->ps.viewangles[1] );

}

void Cmd_ChangeWeaponMode_f( gentity_t *ent ) 
{
    int weapon;
    weapon = ent->client->ps.weapon;

	if ( weapon != WP_ASSAULT ) return;

	if (ent->client->weaponStates[weapon] == qtrue) {
    	ent->client->ps.weaponstate = WEAPON_RAISING;
		if (ent->client->ps.weapon == WP_C4){
			ent->client->ps.weaponAnim = WEAPON_LOWER;
		}
		ent->client->ps.weaponTime += 500;
    	//Toggle the value
    	ent->client->weaponStates[weapon] = qfalse;
    	ent->client->ps.stats[STAT_MODE] = qfalse;
    } else {
    	ent->client->ps.weaponstate = WEAPON_CHANGING;
		//ent->client->ps.torsoAnim = ( ( ent->client->ps.torsoAnim & ANIM_TOGGLEBIT )
	//		^ ANIM_TOGGLEBIT )	| TORSO_DROP;
		if (ent->client->ps.weapon == WP_C4){
			ent->client->ps.weaponAnim = WEAPON_MODE;
		}
		ent->client->ps.weaponTime += 500;
    	//Toggle the value
    	ent->client->weaponStates[weapon] = qtrue;
    	ent->client->ps.stats[STAT_MODE] = qtrue;
    }
}

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum, const char * cmd ) {
	gentity_t*	ent;
	int			part1,part2;

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;		// not fully in game yet
	}

	// all client commands prefixed with 'st_' are destined for the
	// rules engine.
	if ( cmd[ 0 ] == 's' && cmd[ 1 ] == 't' && cmd[ 2 ] == '_' )
	{
		G_ST_exec( ST_CLIENTCOMMAND, clientNum, cmd+3 );
		return;
	}

	part1 = SWITCHSTRING(cmd);
	part2 = SWITCHSTRING(cmd+4);

	switch( part1 )
	{
	case CS('s','a','y',0):		Cmd_Say_f (ent, SAY_ALL, qfalse);	return;		//	say
	case CS('s','a','y','_'):	
		if (clientNum > 8) {//don't let clients do say_team
			Cmd_Say_f (ent, SAY_TEAM, qfalse);
		}
		return;		//	say_team
	case CS('t','e','l','l'):	Cmd_Tell_f ( ent );	return;						//	tell
	case CS('v','s','a','y'):	
		{
			switch( part2 )
			{
				case 0:						Cmd_Voice_f (ent, SAY_ALL, qfalse, qfalse);		return;		//	vsay
				case CS('_','t','e','a'):	Cmd_Voice_f (ent, SAY_TEAM, qfalse, qfalse);	return;		//	vsay_team
			}
		} break;
	case CS('v','t','e','l'):	Cmd_VoiceTell_f ( ent, qfalse );	return;		//	vtell
	case CS('v','o','s','a'):
		{
			switch( part2 )
			{
				case CS('y',0,0,0):			Cmd_Voice_f (ent, SAY_ALL, qfalse, qtrue);	return;	//	vosay
				case CS('y','_','t','e'):	Cmd_Voice_f (ent, SAY_TEAM, qfalse, qtrue);	return;	//	vosay_team
			}
		} break;
	case CS('v','o','t','e'):
		{
			switch( part2 )
			{
				case CS('l','l',0,0):			Cmd_VoiceTell_f ( ent, qtrue );	return;	//	votell
			}
		} break;
	case CS('v','t','a','u'):	Cmd_VoiceTaunt_f ( ent );	return;	//	vtaunt
	case CS('s','c','o','r'):	Cmd_Score_f (ent);	return;	//score
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime) {
		Cmd_Say_f (ent, qfalse, qtrue);
		return;
	}

	switch( part1 )
	{
	case CS('g','i','v','e'):	Cmd_Give_f (ent);		break;	//	give
	case CS('g','o','d',0):		Cmd_God_f (ent);		break;	//	god
	case CS('n','o','t','a'):	Cmd_Notarget_f (ent);	break;	//	notarget
	case CS('n','o','c','l'):	Cmd_Noclip_f (ent);		break;	//	noclip
	case CS('k','i','l','l'):	Cmd_Kill_f (ent);		break;	//	kill
	case CS('l','e','v','e'):	Cmd_LevelShot_f (ent);	break;	//	levelshot
	case CS('f','o','l','l'):
		{
			switch( part2 )
			{
			case CS('o','w',0,0):		Cmd_Follow_f (ent);	break;			//	follow
			case CS('o','w','n','e'):	Cmd_FollowCycle_f (ent, 1); break;	//	follownext
			case CS('o','w','p','r'):	Cmd_FollowCycle_f (ent, -1); break;	//	followprev
			}
		} break;

	case CS( 't','e','a','m' ):	Cmd_Team_f (ent);			break;	//	team
	case CS( 'w','h','e','r' ):	Cmd_Where_f (ent);			break;	//	where
	case CS( 'c','a','l','l' ):	Cmd_CallVote_f (ent);		break;	//	callvote
	case CS( 'v','o','t','e' ):	Cmd_Vote_f (ent);			break;	//	vote
	case CS( 'g','c',0,0):		Cmd_GameCommand_f( ent );	break;	//	gc
	case CS( 's','e','t','v' ):	Cmd_SetViewpos_f( ent );	break;	//	setviewpos
	case CS( 's','t','a','t' ):	Cmd_Stats_f( ent );			break;	//	stats
	case CS( 'l','o','c',0 ):	Cmd_Location_f( ent );		break;	//	loc
	case CS( 'h','o','l','s' ):	Cmd_Holster_Weapon_f( ent );break;	//	holster
	case CS( 'u','n','h','o' ):	Cmd_UnHolster_Weapon_f( ent ); break;//	unholster
	case CS( 'r','e','l','o' ):	Cmd_Reload_f( ent, qtrue );	break;	//	reload
	case CS( 'h','o','s','t' ):	Cmd_Hostage_f( ent );		break;	//	hostage
	case CS( 'm','o','d','e' ):	Cmd_ChangeWeaponMode_f( ent );	break;
	case CS( 'g','r','e','n' ):	Cmd_GrenadeFire_f( ent );	break;	//	grenade
	default:
		trap_SendServerCommand( clientNum, va("print \"unknown cmd %s\n\"", cmd ) );
		}
}
