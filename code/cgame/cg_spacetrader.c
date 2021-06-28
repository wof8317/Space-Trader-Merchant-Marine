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

//	cg_spacetrader.c -	this is the core rules engine.  all access to trading and rules info
//						is done through here

#include "../game/bg_spacetrader.h"

#define	MAX_CP_LINES	8
#define FADE_CP_TIME	3000

globalState_t gsi;

#define GS_ASSASSINATE			gsi.ASSASSINATE
#define GS_BOSS					gsi.BOSS


void CG_UpdateGameState()
{
	trap_UpdateGameState( &gsi );
}
//
// cache any media we need later on that could cause a hitch
//
extern qboolean	CG_FindClientModelFile( char *filename, int length, clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *base, const char *ext );
static void CG_ST_CacheMedia()
{
	switch ( cgs.gametype )
	{
		case GT_ASSASSINATE:
		{
			if ( sql( "SELECT model FROM npcs SEARCH id ?;", "i", GS_BOSS ) ) {

				char filename[MAX_QPATH];
				if ( CG_FindClientModelFile( filename, sizeof(filename), 0, "", sqltext(0), "default", "char", "skin" ) ) {
					trap_R_RegisterSkin	( filename );
				}
				sqldone();
			}

		} break;
	}

}

//	commands that come from the local console
static int CG_ST_ConsoleCommand( const char * cmd )
{
	switch( SWITCHSTRING(cmd) )
	{
	//st_announcer
	case CS('a','n','n','o'):
		{
			if (sql( "SELECT snd FROM npcs_voices SEARCH cmd $1", "t", CG_Argv(1), 0) ){
				int time;
				if (trap_Argc() == 2) {
					time = 1000;
				} else {
					time = atoi(CG_Argv(2));
				}
				if (cg_announcer.integer == 1) {
					CG_AddBufferedSound(sqlint(0), time );
				}
				sqldone();
			}
		}
		break;
		//	st_lookat
	case CS('l','o','o','k'):
		//	st_visit
	case CS('v','i','s','i'):
		{
			// player wants to visit a contact.  we need to get the eye position
			// of that npc so we can tell the server to point the camera at it.
			int client_id	= trap_ArgvI( 1 );
			clientInfo_t *npc_ci = &cgs.clientinfo[ client_id ];
			int eye_height;
			//the magic 24 is a constant that q3 moved the md3's around by
			if ( (int)npc_ci->eye.origin[2] == 0 ) {
				eye_height = 0;
			} else {
				eye_height = DEFAULT_VIEWHEIGHT - ((int)npc_ci->eye.origin[2] - 24);
			}
			trap_Cmd_ExecuteText( EXEC_ONSERVER, va("st_%s %d %d ;", cmd, client_id, eye_height ) );
		} break;
	default:
		return 0;
	}

	return 1;
}

static void CG_ST_QueueDownloads() {

	char buffer[ MAX_INFO_STRING ];
	const char * maps;
	const char * map;

	//	create download queue
	while ( sql( "SELECT map FROM planets SEARCH travel_time WHERE can_travel_to;", 0 ) ) {

		map = sqltext(0);
		if ( trap_FS_FOpenFile( va("maps/%s.bsp", map), 0, FS_READ ) <= 0 ) {

			//	can't find the .map for the planet, try request the pak file that contains it
			sql( "INSERT INTO downloads(name) VALUES($);", "te", map );
		}
	}

	BG_ST_GetMissionStringBuffer( "combat_maps", buffer, sizeof(buffer) );

	maps = buffer;
	for( ;; ) {

		map = COM_Parse( &maps );
		if ( !map || !map[0] )
			break;

		if ( trap_FS_FOpenFile( va("maps/%s.bsp", map), 0, FS_READ ) <= 0 ) {

			sql( "INSERT INTO downloads(name) VALUES($);", "te", map );
		}
	}

}



extern vmCvar_t	cg_spacetrader;

int QDECL CG_ST_exec( int cmd, ... )
{
	int			i=0;
	va_list		argptr;
	va_start(argptr, cmd);


	//
	//	Initialize 
	//
	if ( cmd == CG_ST_INIT )
	{
		char lang[ MAX_NAME_LENGTH ];
		const char * info = CG_ConfigString( CS_SPACETRADER );
		const char * mission;

		if ( !info || info[ 0 ] == '\0' )
			return 0;

		BG_ST_ReadState( info, &gsi );

		cg_spacetrader.integer = 1;

		trap_Cvar_VariableStringBuffer( "cl_lang", lang, sizeof(lang) );
		mission = Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ), "sv_missionname" );

		// copy initial database from disk into memory
		trap_SQL_LoadDB( va("db/%s.mis",mission) );	

		if ( sql( "SELECT name FROM common.files SEARCH fullname missions.key['db_name']^value;", 0 ) ) {

			mission = sqltext(0);
			sqldone();

			trap_SQL_LoadDB( va("db/%s_%s.txt",mission,lang) );	
			trap_SQL_LoadDB( va("db/voices_%s.txt",lang) );
			trap_SQL_LoadDB( va("db/tips_%s.txt",lang) );
		}

		trap_SQL_Exec(

				//	clients_effects
				"CREATE TABLE clients_effects"
				"("
					"client	INTEGER, "
					"bubble	INTEGER, "
					"ready_effect INTEGER, "
					"ready_icon INTEGER "
				");"

				"CREATE TABLE events_icons"
				"("
					"id INTEGER, "
					"icon INTEGER "
				");"

				"CREATE TABLE icons"
				"("
					"client INTEGER, "
					"npc INTEGER,"
					"icon INTEGER"
				");"
				"CREATE INDEX i ON icons(npc);"

				"CREATE TABLE commodities_effects"
				"("
					"commodity_id INTEGER, "
					"buy_effect INTEGER, "
					"sell_effect INTEGER "
				");"
				"CREATE INDEX i ON commodities_effects(commodity_id);"

				"CREATE TABLE downloads"
				"("
					"name STRING"
				");"

				"CREATE TABLE buylist"
				"("
					"id INTEGER, "
					"avg_price INTEGER, "
					"price INTEGER, "
					"change INTEGER, "
					"range INTEGER "
				");"

				"ALTER TABLE npcs_voices ADD snd INTEGER;"
				"ALTER TABLE commodities_text ADD chart INTEGER;"

				);

		trap_Cvar_Set( "cl_spacetrader", va("%d", cg_spacetrader.integer ) );	// turn the rules engine on, its show time!

		trap_SQL_Exec(
					"CREATE INDEX i ON prices(time);"
					"CREATE INDEX i ON missions(key);"
					"CREATE INDEX i ON commodities(id);"
					"CREATE INDEX i ON bosses(bounty);"
					"CREATE INDEX i ON travels(time);"
					"CREATE INDEX i ON npcs(id);"
					"CREATE INDEX i ON clients(npc);"
					"CREATE INDEX i ON planets(id);"
					"CREATE INDEX i ON history(time);"
					"CREATE INDEX i ON planets_text(id);"
					);



		//	ok, db is ready at this point
		if ( sql( "SELECT name, bounty FROM npcs WHERE id=?", "i", GS_BOSS ) )
		{
			Q_strncpyz( cg.bossName, sqltext(0), sizeof(cg.bossName) );
			sqldone();
		}

		//	register voices
		sql( "UPDATE npcs_voices SET snd=sound('sound/voices/'||file||'.ogg');", "e" );

		CG_ST_CacheMedia();

		CG_ST_QueueDownloads();

		trap_Cmd_ExecuteText( EXEC_NOW, "donedl" );
	}

	if ( cg_spacetrader.integer == 0 )
		return 0;

	switch ( cmd )
	{
	case CG_ST_CONSOLECOMMAND:
		{
			return CG_ST_ConsoleCommand( va_arg( argptr, const char * ) );
		} break;

	case CG_ST_SHIELD:
		{
			refEntity_t *ent = va_arg( argptr, refEntity_t * );
			entityState_t *state = va_arg( argptr, entityState_t * );
			if ( state->shields > 0 ) {
				ent->customShader = cgs.media.shieldShader;
				ent->shaderRGBA[3] = 255 * (state->shields/200.0f);
			}
			return 0;
		} break;
	case CG_ST_SERVERCOMMAND:
		{
			return CG_ST_ConsoleCommand( va_arg( argptr, const char * ) );
		} break;
	}

	return i;
}

void BG_ST_Allsolid(int clientNum)
{
}

#if (__LINE__>4096 )
#error "sql query table too small."
#endif
