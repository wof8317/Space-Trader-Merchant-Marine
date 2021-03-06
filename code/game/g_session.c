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
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	const char	*var;

	// bk001205 - format
	int teamLeader;
	int spectatorState;
	int sessionTeam;

	var = va( "session%i", client - level.clients );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i",
		&sessionTeam,                 // bk010221 - format
		&client->sess.spectatorTime,
		&spectatorState,              // bk010221 - format
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader                   // bk010221 - format
		);

	// bk001205 - format issues
	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.teamLeader = (qboolean)teamLeader;
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo ) {
	clientSession_t	*sess = &client->sess;

	if ( g_gametype.integer == GT_HUB ) {
		// for hub maps all players and bots are on team red
		sess->sessionTeam = TEAM_RED;

	} else {
		// players vs npc's
		sess->sessionTeam = (client->ps.clientNum<MAX_PLAYERS)?TEAM_BLUE:TEAM_RED;
	}

	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorTime = level.time;
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void )
{
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int	i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED )
		{
			gclient_t * client = level.clients + i;
			const char	*s;
			const char	*var;

			s = va("%i %i %i %i %i %i %i", 
				client->sess.sessionTeam,
				client->sess.spectatorTime,
				client->sess.spectatorState,
				client->sess.spectatorClient,
				client->sess.wins,
				client->sess.losses,
				client->sess.teamLeader
				);

			var = va( "session%i", client - level.clients );
			trap_Cvar_Set( var, s );
		}
	}

	G_ST_exec( ST_SHUTDOWN_GAME );
}
