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

#include "bg_spacetrader.h"


/*

SCHEMA

	commodities
	---------------------------
	id					INTEGER
	low					INTEGER
	high				INTEGER
	good				INTEGER
	abbreviation		STRING
	name				STRING




cl_commodities_text
	id
	name
	description

sv_events
	id
	chance
	planet_id

cl_events_text
	id
	name
	description

sv_commodities_events
	event_id
	commodity_id
	low
	hight

sv_dialogs
	id
	npc_id
	parent_id
	dialogtype		// ??
	planet_id
	priority

cl_dialogs_text
	id
	index
	text


sv_planets
	id
	dfs
	op
	size
	sa
	map
	visible
	music

cl_planets_text
	id
	name
	description
	icon
	overlay








*/


//
//	g_spacetrader.c -	this is the core rules engine.  all decisions concerning the trading
//						and scoring are accessed through here.
//



#define G_ST_JOBS_DIALOG_ID	-2
#define G_ST_TRADE_DIALOG_ID -3

static void		insert_purchase					( int client, int commodity_id, int qty, int price );
static void		G_ST_UpdateRanks				();
static void		returntohub						();
static void		G_ST_Moa_Lose					();


static int current_client;

static void G_ST_ClientCommand( int client, const char * cmd );
//	locally cached info

typedef struct
{
	int		reset;
	int		cargo_id;
	int		justloaded;

	struct
	{
		int		time;			// last time the objectives were tested
		int		scramble;
		
	} objectives;

	struct
	{
		int	count;
		struct
		{
			int		commodity_id;
			int		low_count;
			int		high_count;
		} list[ 32 ];
		int	total_spawned;
		int	stash_spawns;

	} stash;
	
	struct
	{
		int			dead_time;
		int			boss_escape_time;
		qboolean	failed;

		int			bounty;
		int			gang_left_easy;
		int			gang_left_medium;
		int			gang_left_hard;
		int			gang_count;

	} assassinate;
	struct
	{
		int			dead_time;
		int			cash;
	} moa;

	int		travelwarn;
} spacetraderInfo_t;

static spacetraderInfo_t st;

//
//	gs - GAME STATE		all global game state vars sit in this array.  anything in the array is propogated to all clients.
//						this is cleared to 0 each map restart.  These state variables change over the course of a turn. all
//						state that doesn't change over the course of the turn are stored in configstrings CS_SPACETRADER_*
//
globalState_t gs;

#define GS_COUNTDOWN			gs.COUNTDOWN
#define GS_MENU					gs.MENU
#define GS_INPLAY				gs.INPLAY

#define GS_SEED					gs.SEED
#define GS_TIME					gs.TIME
#define GS_TIMELEFT				gs.TIMELEFT
#define GS_LASTTIME				gs.LASTTIME
#define GS_TURN					gs.TURN
#define GS_PLANET				gs.PLANET

#define GS_ASSASSINATE			gs.ASSASSINATE
#define GS_BOSS					gs.BOSS
#define GS_BOARDING				gs.BOARDING


static qboolean players_connecting() {
	int i;
	for ( i=0; i<g_maxplayers.integer; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTING )
			return qtrue;
	}

	return qfalse;
}


//	player makes a contact to an npc
static void G_ST_AddContact( int client, int npc )
{
	if ( sql( "SELECT npc FROM contacts SEARCH npc ?2 WHERE player=?1 LIMIT 1;", "ii", client, npc ) )
	{
		sqldone();
		return;	// already added
	}

	sql( "INSERT INTO contacts(player,npc) VALUES(?,?);", "iie", client, npc );

	trap_SendServerCommand( client, va( "st_newcontact %i", npc ) );
}

//	all bots and players are connected, update the contacts for everyone
static void G_ST_UpdateContacts()
{
	while( sql( "SELECT client FROM players;", 0 ) )
	{
		current_client = sqlint(0);

		//	scan all the npcs on the planet
		while( sql( "SELECT id FROM npcs SEARCH planet_id ?1 WHERE eval( contact );", "i", GS_PLANET ) )
		{
			sql( "UPDATE OR INSERT contacts SET player=?1,npc=?2 SEARCH npc ?2 WHERE player=?1;", "iie", current_client, sqlint(0) );
		}
	}
}

static void G_ST_RevealBoss( int count )
{
	//	find the n 'cheapest' bosses
	while ( sql( "SELECT id, bounty FROM bosses SEARCH bounty WHERE planet_id=-1 AND captured=0 LIMIT ?;", "i", count ) )
	{
		int boss_id		= sqlint(0);
		int planet_id;
		int bounty		= sqlint(1);
		int value;
		int rate, player_count;

		//	pick one of his possible locations at random
		if ( !sql( "SELECT planet_id FROM dialogs SEARCH npc_id ?1 RANDOM 1;", "isId", boss_id, &planet_id ) )
			continue;

		sql( "SELECT SUM(cash+inventory), COUNT(*) FROM players;", "sIId", &value, &player_count );

		rate = BG_ST_GetMissionInt("bounty_rate");

		if (player_count != 0 )
			value = value / player_count;

		value = (value * Q_rrand(rate-3, rate+3) )/100;

		bounty = max(bounty,value);
		//	reveal him at that location
		sql( "UPDATE bosses SET planet_id=?1, bounty=?3 SEARCH id ?2", "iiie", planet_id, boss_id, bounty );
	}
}

static int G_ST_TeleportPlayerToNPC( int client, int bot, int eye_height )
{
	if ( bot >= 0 && bot < MAX_CLIENTS )
	{
		gentity_t * ent			= g_entities + client;
		gentity_t * target_bot	= g_entities + bot;
		vec3_t spawn;

		if ( FindSpawnNearOrigin( target_bot->client->ps.origin, spawn, 48, target_bot->r.mins, target_bot->r.maxs ) )
		{
			vec3_t bot_eye;
			vec3_t dir;
			vec3_t angles;

			VectorCopy( target_bot->client->ps.origin, bot_eye );
			bot_eye[2] -= eye_height;

			VectorSubtract( bot_eye, spawn, dir );
			vectoangles(dir, angles);

			TeleportPlayer( ent, spawn, angles, qfalse );
			return bot;
		}
	}

	return -1;
}

//
//	make bot say something to client.
//
static void  G_ST_SayDialog( int client, int bot, int npc_id, int dialog_id )
{
	sql( "UPDATE OR INSERT conversations SET player=?1, bot=?2, npc=?3, dialog_id=?4 SEARCH player ?1;", "iiiie", client, bot, npc_id, dialog_id );

	switch( dialog_id ) {
		case G_ST_TRADE_DIALOG_ID:
			trap_SendServerCommand( client, va( "st_trade %i %i ;", npc_id, bot ) );
			break;

		case G_ST_JOBS_DIALOG_ID:
			trap_SendServerCommand( client, va( "st_jobs %i %i ;", npc_id, bot ) );
			break;

		default:
			trap_SendServerCommand( client, "st_dialog open ;" );
			break;
	}
}

static void G_ST_UpdateNPCSprites() {
/*
	npcs_sprites
	------------
	npc
	player
	sprite
*/
	while( sql( "SELECT client FROM players;", 0 ) ) {
	
		current_client = sqlint(0);

		while( sql( "SELECT id, type like 'Merchant', good FROM npcs SEARCH planet_id GS_PLANET;", 0 ) ) {

			int	npc_id		= sqlint(0);
			int	best_sprite	= -1;
			int	best_score	= 0;

			while ( sql( "SELECT max(priority like '',eval( priority )), sprite FROM dialogs SEARCH name npcs.id[ ?1 ]^dialog;", "i", npc_id ) ) {

				int score = sqlint(0);
				
				if ( score > best_score )
				{
					best_score	= score;
					best_sprite	= sqlint(1);
				}
			}

			if ( best_score <= 0 )
			{
				int is_merchant = sqlint(1);

				if ( is_merchant == 1 )
				{
					// 1 - exclaim, 2 - question, 3 - good merchant, 4 - bad merchant, 5 - mechanic
					switch( sqlint(2) )
					{
					case 0:	best_sprite = 4; break;
					case 1: best_sprite = 3; break;
					case 2: best_sprite = 5; break;
					}
				}
			}

			if ( sql( "SELECT COUNT(*) FROM npcs_sprites SEARCH npc ?1 WHERE player=?2 AND sprite=?3;", "iii", npc_id, current_client, best_sprite ) ) {

				int n = sqlint(0);
				sqldone();

				if ( n==0 ) {
					sql( "UPDATE OR INSERT npcs_sprites SET npc=?1,player=?2,sprite=?3 SEARCH npc ?1 WHERE player=?2;", "iiie", npc_id, current_client, best_sprite );
				}
			}
		}
	}
}

static int G_ST_SayDialogByName( int client, int bot, int npc_id, const char * dialog )
{
	int		best_id		= -1;
	int		best_score	= 0;

	if ( !dialog || !dialog[0] )
		return 0;

	current_client = client;
	while ( sql( "SELECT id, max(priority like '',eval( priority )) FROM dialogs SEARCH name $1 WHERE (npc_id = ?2 OR ?2 = -1 OR npc_id < 0);", "ti", dialog, npc_id ) )
	{
		int score = sqlint(1);

		if ( score > best_score )
		{
			best_score	= score;
			best_id		= sqlint(0);
		}
	}

	if ( best_score <= 0 )
		return 0;

	G_ST_SayDialog( client, bot, npc_id, best_id );
	return 1;
}

//	evaluates to see if any npc wants to talk to the player
static int G_ST_SayStartDialog( int client )
{
	int		best_id		= -1;
	int		best_score	= 0;
	int		best_bot	= -1;
	int		best_npc	= -1;

	current_client = client;
	while ( sql( "SELECT id, npc_id, clients.id[npc_id].client, max(priority like '',eval( priority )) FROM dialogs SEARCH name 'start' WHERE planet_id = ?1;", "i", GS_PLANET ) )
	{
		int	npc_id		= sqlint(1);
		int client_id	= sqlint(2);
		int score		= sqlint(3);

		if ( client_id < 0 )
			continue;

		if ( score > best_score )
		{
			best_score	= score;
			best_id		= sqlint(0);
			best_bot	= client_id;
			best_npc	= npc_id;
		}
	}

	if ( best_score > 0 )
	{
		if ( G_ST_TeleportPlayerToNPC( client, best_bot, -4 ) )
		{
			gentity_t * target_bot = g_entities + best_bot;
			G_Say( g_entities + client, target_bot, SAY_TELL, va("dialog %s do start %d", target_bot->client->pers.netname, best_id ) );
			return 1;
		}
	}

	return 0;
}


//
//
//
static void G_ST_DoDialogCommand( int client, int bot, int npc_id, const char * cmd )
{
	const char * token;

	//
	//	execute dialog commands !!
	//
	while ( 1 )
	{
		token = COM_ParseExt( &cmd, qtrue );

		if ( !token || token[0] == 0 )
			break;

		switch( SWITCHSTRING( token ) )
		{
			case CS( 's','a','y',0):	// "st_say"
				{
					const char * dialog = COM_ParseExt( &cmd, qfalse );

					if ( !G_ST_SayDialogByName( client, bot, npc_id, dialog ) )
					{
						//	no dialog specified, see if any is hardcoded on npc
						if ( sql( "SELECT dialog, (type like 'Merchant') FROM npcs SEARCH id ?1 WHERE planet_id=?2 LIMIT 1;", "ii", npc_id, GS_PLANET ) )
						{
							const char 	*	dialog		= sqltext   ( 0 );
							int				isMerchant	= sqlint	( 1 );

							//	if npc has nothing to say and is a merchant, then go ahead and trade!
							if ( !G_ST_SayDialogByName( client, bot, npc_id, dialog ) && isMerchant )
							{
								G_ST_AddContact( client, npc_id );
								G_ST_SayDialog( client, bot, npc_id, G_ST_TRADE_DIALOG_ID );
							}

							sqldone();
						} else {
							G_ST_SayDialogByName( client, bot, npc_id, "guard" );
						}
					}
				} break;

			case CS('s','t','a','r'):	// "start"	- player asks npc to initiate dialog at start of game
				{
					int dialog_id = atoi( COM_ParseExt( &cmd, qfalse ) );
					G_ST_SayDialog( client, bot, npc_id, dialog_id );
				} break;

			case CS('b','y','e',0):		// "bye"
				trap_SendServerCommand( client, "st_dialog close ;" );
				G_ST_ClientCommand( client, va("endi %d ;", bot) );
				break;

			case CS('t','r','a','d'):	// "trade"
				{
					G_ST_SayDialog( client, bot, npc_id, G_ST_TRADE_DIALOG_ID );
					//flag this bot as being in dialog
				} break;

			case CS('j','o','b','s'):	// "jobs"
				{
					const char * sorry = COM_ParseExt( &cmd, qfalse );

					if ( !sorry || !sorry[0] )
						Com_Error( ERR_FATAL, "Error in dialog.  Command jobs needs default dialog.\n" );

					if ( sql( "SELECT COUNT(*) FROM bosses WHERE planet_id=? AND captured=0;", "i", GS_PLANET ) )
					{
						int	bossCount = sqlint(0);
						sqldone();

						if ( bossCount == 0 )
							//	there's no bosses on this planet, sorry
							G_ST_SayDialogByName( client, bot, npc_id, sorry );
						else if ( bossCount == 1 )
						{
							//	there's only 1 guy, cut to the chase and talk about him
							if ( sql( "SELECT id FROM bosses WHERE planet_id=GS_PLANET AND captured=0 LIMIT 1;", 0 ) ) {
								G_ST_DoDialogCommand( client, bot, npc_id, va( "job %d",sqlint(0) ) );
								sqldone();
							}
						} else
						{
							G_ST_SayDialog( client, bot, npc_id, G_ST_JOBS_DIALOG_ID );
						}
					}

				} break;
			case CS('j','o','b',0):
				{
					int boss_id = atoi( COM_ParseExt( &cmd, qfalse ) );
					
					//	find the dialog for this boss
					if ( sql( "SELECT id FROM dialogs SEARCH npc_id ? WHERE planet_id=GS_PLANET;", "i", boss_id ) )
					{
						int dialog_id = sqlint(0);
						sqldone();

						G_ST_SayDialog( client, bot, npc_id, dialog_id );
					}

				} break;

			case CS('r','e','v','e'):	// "reveal"
				{
					char * boss		= COM_Parse( &cmd );
					char * planet	= COM_Parse( &cmd );

					sql( "UPDATE bosses SET planet_id=planets.name[ $1 ].id SEARCH id npcs.name[ $2 ].id;", "tte", boss, planet );

				} break;

			case CS('a','s','s','a'):	// "assassinate"
				{
					trap_SendServerCommand( client, "st_dialog close ;" );
					CallVote( client, "assassinate", cmd + 1 );	 // the text after assassinate is the bosses name, so just push cmd forward by one for the space.
					
				} break;
			case CS('h','o','s','t'): // "hostage"
				{
				}break;

			case CS('g','i','v','e'):	// "give"
				{
					const char * commodity = COM_Parse( &cmd );
				
					if ( sql( "SELECT id FROM commodities WHERE name like $1 OR abbreviation like $1;", "t", commodity ) )
					{
						int commodity_id	= sqlint(0);
						sqldone();

						if ( sql( "SELECT min( eval($2), cargomax-cargo ) FROM players SEARCH client ?1;", "it", client, COM_Parse( &cmd ) ) ) {

							int qty = sqlint(0);
							sqldone();

							insert_purchase( client, commodity_id, qty, 0 );

							if ( qty > 0 && qty < 256 ) {
								G_AddEvent( g_entities + client, EV_ITEM_STASH, (qty<<8) + commodity_id );
							}

							if ( sql( "UPDATE missions SET value=players.client[?1].cargomax:plain SEARCH key 'max_cargo' WHERE atoi(value)<players.client[?1].cargomax;", "ie", client ) ) {
								trap_SendServerCommand( -1, "st_cp T_Upgrade_Cargo newcontact" );
							}
						}
					}
				} break;
			case CS('c','a','s','h'):	// "cash"
				{
					if ( sql( "SELECT eval($1);", "t", COM_Parse( &cmd ) ) ) {
						int amount = sqlint(0);
						sqldone();
					
						sql( "UPDATE players SET cash=cash+?2, networth=cash+debt+inventory SEARCH client ?1;", "iie", client, amount );
						trap_SendServerCommand( client, va("st_cash %d", amount ) ); 
						G_ST_UpdateRanks();
					}
				} break;
					
			case CS('c','o','n','t'):	// "contact"
				{
					const char * npc = COM_Parse( &cmd );

					if ( sql( "SELECT id FROM npcs WHERE name like $1;", "t", npc ) )
					{
						G_ST_AddContact( client, sqlint(0) );
						sqldone();
					}

				} break;

				//	flag
			case CS('$','f','l','a'):
				{
					sql( "UPDATE OR INSERT flags SET key=$1,value=value+1,player=?2 SEARCH key $1 WHERE player=?2;", "tie", token+6, client );
				} break;

				//	set
			case CS('$','s','e','t'):
				{
					current_client = client;
					sql( "UPDATE OR INSERT flags SET key=$1,value=eval($2),player=?3 SEARCH key $1 WHERE player=?3;", "ttie", token+5, COM_Parse( &cmd ), client );
				} break;

				//	$global_ flag
			case CS('$','g','l','o'):
				{
					current_client = client;
					sql( "UPDATE OR INSERT flags SET key=$1,value=eval($2),player=?3 SEARCH key $1 WHERE player=?3;", "ttie", token+8, COM_Parse( &cmd ), -1 );

				} break;

		}
	}
}


//
//	get the total number of 'cdt' the client has in his inventory
//
static int Inv_GetItemQty( int client, int commodity_id )
{
	int t = 0;

	sql( "SELECT SUM(qty) FROM purchases SEARCH player ?1 WHERE commodity_id=?2;", "iisId", client, commodity_id, &t );

	return t;
}

static void G_ST_UpdateRanks() {
	if ( GS_INPLAY == 0 && GS_TURN == 1 ) {
		//	sort by client id until the game starts
		sql( "UPDATE players SET rank=1+@ SEARCH client;", "e" );
	} else {
		sql( "UPDATE players SET rank=1+@ SEARCH networth DESC;", "e" );
	}
}

static void G_ST_UpdateInventory( int client )
{
	if ( sql( "SELECT SUM(qty*todaysprices.commodity_id[ commodity_id ].price),SUM(qty) FROM purchases SEARCH player ?1;", "i", client ) ) {

		int	inventory	= sqlint(0);
		int	cargo_used	= sqlint(1);
		int	cargo_bought;
		sqldone();

		sql( "SELECT SUM(qty) FROM inventory SEARCH player ?1 WHERE commodity_id=commodities.abbreviation[ 'CH' ].id;", "isId", client, &cargo_bought );

		sql( "UPDATE players SET inventory=?2,cargo=?3,cargomax=cargomin+?4,networth=cash+debt+inventory SEARCH client ?1;", "iiiie", client, inventory, cargo_used-cargo_bought, cargo_bought );
		G_ST_UpdateRanks();
	}
}



//
//	insert_* -	these functions are responsible for updating the database with new records.  they
//				also keep track of sync the clients db
//

static void insert_purchase( int client, int commodity_id, int qty, int price )
{
	// add to front, remove from back.  stack ones that are equivalent price.
	if ( qty > 0 ) {

		sql( "UPDATE OR INSERT purchases SET player=?1,time=?2,commodity_id=?3,qty=qty+?4,price=?5 "
					"SEARCH time ?2 "
					"WHERE player=?1 AND commodity_id=?3 AND price=?5 LIMIT 1;",
					"iiiiie", client, GS_TIME, commodity_id, qty, price );

	} else if ( qty < 0 ) {

		sql( "UPDATE purchases SET qty=qty-(?3 REMOVE qty) SEARCH time DESC WHERE player=?1 AND commodity_id=?2;", "iiie", client, commodity_id, -qty );
		sql( "DELETE FROM purchases WHERE player=?1 AND qty<=0;", "ie", client );
	} else
		return;

	//	update the player's inventory
	if ( sql( "SELECT SUM(qty*price),SUM(qty) FROM purchases SEARCH player ?1 WHERE commodity_id=?2;", "ii", client, commodity_id ) ) {

		int qty		= sqlint(1);
		int cost	= sqlint(0);
		sqldone();

		if ( qty > 0 ) {

			sql( "UPDATE OR INSERT inventory SET player=?1,commodity_id=?2,qty=?3,cost=?4 "
					"SEARCH player ?1 "
					"WHERE commodity_id=?2;",
					"iiiie", client, commodity_id,qty,cost/qty );
		} else {

			sql( "DELETE FROM inventory WHERE player=?1 AND commodity_id=?2;", "iie", client, commodity_id );
		}
	}


	//	bookkeeping... update inventory total value and total cargo held
	G_ST_UpdateInventory( client );
}

static void insert_upgrade( int client, int time, int upgrade_id, int qty, int price )
{
	sql( "INSERT INTO upgrades(player,time,upgrade_id,qty,price) VALUES(?,?,?,?,?);", "iiiiie", client, time, upgrade_id, qty, price );
}

static void insert_debt( int client, int amount )
{
	if ( amount != 0 ) {
		sql( "UPDATE players SET debt=debt+?2,networth=cash+debt+inventory SEARCH client ?1;", "iie", client, amount );
		G_ST_UpdateRanks();
	}
}

static qboolean trade( int client, int commodity_id, int qty, int price )
{
	if ( qty > 0 )
	{
		if ( sql( "SELECT cash, cargomax, cargo FROM players SEARCH client ?1;", "i", client ) ) {

			int	cash		= sqlint(0);
			int cargo_max	= sqlint(1);
			int cargo_used	= sqlint(2);
			sqldone();

			//	player doesn't have enough cash to support trade
			if ( cash < qty*price )
				return qfalse;

			if ( commodity_id == st.cargo_id ) {

				//	cargo hold cannot be upgrade that far
				if ( qty > BG_ST_GetMissionInt( "max_cargo" ) - cargo_max )
					return qfalse;

			} else {

				//	player does not have enough room in cargo
				if ( qty > cargo_max-cargo_used )
					return qfalse;
			}
		}

	} else {

		//	player is giving inventory, make sure there is enough on hand to give
		if ( Inv_GetItemQty( client, commodity_id ) < -qty )
			return qfalse;

		//	if player is giving upgrades make sure they're not needed
		if ( commodity_id == st.cargo_id ) {
			if ( sql( "SELECT cargomax-cargo FROM players SEARCH client ?1;", "i", client ) ) {
				int cargo_free = sqlint(0);
				sqldone();

				if ( -qty > cargo_free )
					return qfalse;
			}
		}
	}

	//
	//	record inventory
	//
	insert_purchase( client, commodity_id, qty, price );

	//
	//	record cost
	//
	sql( "UPDATE players SET cash=cash-?2,networth=cash+debt+inventory SEARCH client ?1;", "iie", client, qty*price );
	G_ST_UpdateRanks();

	return qtrue;
}

int G_GlobalInt() {
	char token[ MAX_NAME_LENGTH ];
	trap_Argv( 0, token, sizeof(token) );

	switch( SWITCHSTRING(token) )
	{
		// "%client"
	case CS('c','l','i','e'):
		{
			return current_client;
		} break;
		// "%location"
	case CS( 'l','o','c','a' ):	return GS_PLANET;	
	case CS( 'd','e','b','t' ):
		{
			int debt = 0;
			sql( "SELECT debt FROM players SEARCH client ?;", "isId", current_client, &debt );
			return debt;
		} break;	// "%debt"
	case CS( 'c','a','s','h' ):
		{
			int cash = 0;
			sql( "SELECT cash FROM players SEARCH client ?;", "isId", current_client, &cash );
			return cash;
		} break;	// "%cash"
	case CS( 'i','n','v','i' ):
		{
			int inventory;
			sql( "SELECT inventory FROM players SEARCH client ?;", "isId", current_client, &inventory );
			return inventory;
		} break;	// "%inventory"
	case CS( 'n','e','t','w' ):
		{
			int networth;
			sql( "SELECT networth FROM players SEARCH client ?;", "isId", current_client, &networth );
			return networth;
		} break;	// "%networth"
	case CS('f','r',0,0):
		{
			int fr;
			sql( "SELECT fr FROM players SEARCH client ?;", "isId", current_client, &fr );
			return fr;
		} break;	// "%fr"	fugitive rating
	case CS( 'c','a','r','g'):
		{
			int cargo_left;
			sql( "SELECT cargomax-cargo FROM players SERACH client ?;", "isId", current_client, &cargo_left );
			return cargo_left;
		} break;	// "%cargoleft"
	case CS( 'c','l','o','s' ):
		{
			if ( sql( "SELECT min(travel_time) FROM planets WHERE can_travel_to AND is_visible;", 0 ) ) {
				int	t = sqlint(0);
				sqldone();

				return t;	// "%closest"
			}
		} break;
	case CS( 't','i','m','e' ): {
			switch( SWITCHSTRING( token + 4 ) ) {
			case 0:						return GS_TIME;		//	%time
			case CS('l','e','f','t'):	return GS_TIMELEFT;	//	%timeleft
			}
		} break;

		//	%global_
	case CS('g','l','o','b'):
		{
			int v = 0;
			sql( "SELECT value FROM flags SEARCH key $1 WHERE player=-1;", "tsId", token+7, &v );
			return v;
		} break;
	}

	//	player flags
	if (	token[ 0 ] == 'i' &&
			token[ 1 ] == 's' &&
			token[ 2 ] == '_' )
	{
		int v = 0;
		sql( "SELECT value FROM flags SEARCH key $ WHERE player=?;", "tisId", token+3, current_client, &v );
		return v;
	}


	//	inventory
	if ( sql( "SELECT id FROM commodities WHERE name like $1 OR abbreviation like $1;", "t", token ) )
	{
		int commodity_id = sqlint(0);
		sqldone();
		return Inv_GetItemQty( current_client, commodity_id );
	}

	return 0;
}


typedef struct
{
	int	dfs;
	int	op;
	int	sa;
} planet_t;

static void TravelPlanetLocation( planet_t * p, int t, float *x, float *y )
{
	if ( p->op > 0 && p->dfs > 0 )
	{
		float d = DEG2RAD(p->sa) + (((float)t / (float)p->op) * 2.0f * M_PI );

		*x = sinf( d ) * (float)p->dfs;
		*y = cosf( d ) * (float)p->dfs;
	} else
	{
		*x = 0.0f;
		*y = 0.0f;
	}
}

static int TravelTime( planet_t * dst, float sx, float sy, int starttime, float rate, int total )
{
	int t;
	float ex,ey;

	for ( t=1; t<=total; t++ )
	{
		float d;

		TravelPlanetLocation( dst, starttime + t, &ex, &ey );
		
		ex -= sx;
		ey -= sy;
		d = sqrtf( ex*ex + ey*ey );
        
		if ( d <= t * rate )
			return t;
	}

	return 0;
}

static void G_ST_UpdatePlanets()
{
	int	total_time	= BG_ST_GetMissionInt("time");
	int	ship_speed	= BG_ST_GetMissionInt("ship_speed");

	//	update visible and travel flags
	sql( "UPDATE planets SET is_visible=eval( visible ), can_travel_to=!(map like '-1' OR map like '') AND id!=GS_PLANET,travel_time=0;", "e" );

	//	calculate travel times
	if ( sql( "SELECT # FROM planets SEARCH id ?;", "i", GS_PLANET ) ) {

		planet_t	planets[ 32 ];
		float		sx,sy;
		
		int			planet_count;

		int			c = sqlint(0);
		sqldone();

		planet_count	= trap_SQL_Select( "SELECT dfs,op,sa FROM planets;", (char*)planets, sizeof(planets) );

		TravelPlanetLocation( planets + c, GS_TIME, &sx, &sy );

		while( sql( "SELECT #,id FROM planets WHERE can_travel_to=1;", 0 ) )
		{
			int i = sqlint(0);
			int t = TravelTime( planets + i, sx, sy, GS_TIME, (float)ship_speed, total_time );

			sql( "UPDATE planets SET travel_time=?2 SEARCH id ?1;", "iie", sqlint(1), t );
		}
	}
}

static void G_ST_SpawnDisasters()
{
	//	select 2 events at random from all the events that:
	//		1. are not on the current planet
	//		2. are on a planet that has no current event
	//		3. pass their chance roll
	while ( sql(	"SELECT "
						"planet_id, "
						"id "
					"FROM events "
					"WHERE	(planet_id!=GS_PLANET) AND "
					"		(planets.id[ planet_id ].event_end<GS_TIME) AND "
					"		(rnd( 0,100 ) < chance )"
					"RANDOM 2;", 0 ) )
	{
		//	set the planets event and ending time
		sql( "UPDATE planets SET event_id=?2,event_end=GS_TIME+travel_time+rnd( 3, travel_time ) SEARCH id ?1", "iie", sqlint(0), sqlint(1) );
	}
}

static void G_ST_RevealBosses( void )
{
	int c = 0;

	sql( "SELECT COUNT(*) FROM bosses WHERE planet_id >= 0 AND captured=0;", "sId", &c );

	if ( c < 3 )
		G_ST_RevealBoss( 3 );
}


typedef struct {
	char *key;
	char *value;
} missionDefaults_t;

//
//	initialize space trader for the very first time
//
static void G_ST_BeginGame( const char * mission )
{
	int i;
	missionDefaults_t defaults[] = { 
		{"max_cargo",	 	"250",					},
		{"time", 			"100",					},
		{"start_time",		"1",					},
		{"last_time",		"1",					},
		{"start_planet_id",	"1",					},
		{"stash_cheapest",	"250",					},
		{"stash_maxperpile","35",					},
		{"stash", 			"100",					},
		{"savings_rate", 	"50",					},
		{"interest_rate", 	"0",					},
		{"interest_period", "0",					},
		{"MoA_Audit_Chance","25",					},
		{"moa_guard_count",	"24",					},	//	number of extra guards that board to collect taxes if your 'fr' is 100%
		{"moa_guard_min",	"3",					},	//	minimum number of moa guards that board the ship to collect taxes
		{"moa_max_atonce",	"8",					},	//	do not spawn more than this at one time
		{"moa_fr_goods",	"15",					},
		{"moa_fr_kills",	"5",					},
		{"price_flux", 		"5",					},
		{"lowend_threshold","200",					},	//	the price at which commodities stop being considered 'easy, low end' commodities
		{"stash_bonus",		"2",					},
		{"speed_bonus",		"3",					},
		{"version", 		"0",					},
		{"next_challenge", 	NULL,					},
		{"moa_taxrate", 	"15",					},
		{"moa_mintax",		"250",					},
		{"wealth", 			"500",					},
		{"debt", 			"0",					},
		{"credit", 			"1000",					},
		{"cargo", 			"100",					},
		{"win_condition", 	NULL,					},
		{"fail_condition", 	NULL,					},
		{"scramble", 		"0",					},
		{"lives", 			"3",					},
		{"moa_music",		"music/minesdrum",		},
		{"ship_speed",		"6"						},
		{"combat_maps",		"",						},
		{"tradetime",		"120",					},
		{"moneybag_min",	"100",					},
		{"bounty_rate",		"23",					},
		{"turn",			"1",					},
		{"complete", 		"0"						},
		};

	// create clean empty db
	trap_SQL_Reset();

	//
	//	this step loads the mission's db into memory.  this will create all the necessary tables for
	//	static and read-only information.  this is done sepearately on the client.
	//
	trap_SQL_LoadDB( va("db/%s.mis", mission ) );

	trap_SQL_Exec( 
					//	contacts
					"CREATE TABLE IF NOT EXISTS contacts"
					"("
						"player INTEGER, "
						"npc INTEGER "
					");"
					"CREATE SYNC ON contacts SELECT npc AS npc FROM contacts WHERE player=? AND npcs.id[npc].planet_id=GS_PLANET;"


					//	prices
					"CREATE TABLE IF NOT EXISTS prices"
					"("
						"time INTEGER, "
						"commodity_id INTEGER, "
						"price INTEGER "
					");"
					"CREATE SYNC ON prices SELECT * FROM prices;"


					//	todaysprices
					"CREATE TABLE IF NOT EXISTS todaysprices"
					"("
						"commodity_id INTEGER, "
						"price INTEGER, "
						"change INTEGER, "
						"range INTEGER "
					");"
					"CREATE SYNC on todaysprices SELECT commodity_id AS commodity_id, price AS price, change AS change, range AS range FROM todaysprices;"

					//	todaysnews
					"CREATE TABLE IF NOT EXISTS todaysnews"
					"("
						"id INTEGER "
					");"
					"CREATE SYNC ON todaysnews SELECT id AS id FROM todaysnews;"


					//	purchases
					"CREATE TABLE IF NOT EXISTS purchases"
					"("
						"player	INTEGER, "
						"commodity_id INTEGER, "
						"time INTEGER, "
						"qty INTEGER, "
						"price INTEGER "
					");"

					//	inventory
					"CREATE TABLE IF NOT EXISTS inventory"
					"("
						"player INTEGER, "
						"commodity_id INTEGER, "
						"qty INTEGER, "
						"cost INTEGER "
					");"
					"CREATE SYNC ON inventory SELECT commodity_id AS commodity_id, qty AS qty, cost AS cost FROM inventory WHERE player=?;"


					//	flags
					"CREATE TABLE IF NOT EXISTS flags"
					"("
						"key	STRING, "
						"value	INTEGER, "
						"player	INTEGER "
					");"
					"CREATE SYNC ON flags SELECT key AS key, value AS value, player AS player FROM flags;"


					//	converations
					"CREATE TEMP TABLE conversations"
					"("
						"player		INTEGER, "
						"bot		INTEGER, "
						"npc		INTEGER, "
						"dialog_id	INTEGER "
					");"
					"CREATE INDEX i ON conversations(player);"
					"CREATE SYNC ON conversations SELECT bot AS bot, npc AS npc, dialog_id AS dialog_id, 0 AS id FROM conversations WHERE player=?;"

					//	npcs_sprites
					"CREATE TEMP TABLE npcs_sprites"
					"("
						"npc		INTEGER, "
						"player		INTEGER, "
						"sprite		INTEGER "
					");"
					"CREATE SYNC ON npcs_sprites SELECT npc AS npc,sprite AS sprite FROM npcs_sprites WHERE player=?;"

					//	travels
					"CREATE TABLE IF NOT EXISTS travels"
					"("
						"time		INTEGER, "
						"planet_id	INTEGER "
					");"
					"CREATE SYNC ON travels SELECT time AS time, planet_id AS planet_id FROM travels;"


					//	history
					"CREATE TABLE IF NOT EXISTS history"
					"("
						"client		INTEGER, "
						"time		INTEGER, "
						"networth	INTEGER "
					");"
					"CREATE SYNC ON history SELECT time AS time, networth AS networth FROM history WHERE client=?;"


					//	assassinate
					"CREATE TEMP TABLE assassinate"
					"("
						"key		STRING, "
						"value		INTEGER "
					");"
					"CREATE SYNC ON assassinate SELECT * FROM assassinate;"	// WHERE gametype == GT_ASSASSINATE

					//	boarding
					"CREATE TEMP TABLE boarding"
					"("
						"key		STRING, "
						"value		INTEGER "
					");"
					"CREATE SYNC ON boarding SELECT * FROM boarding;"	// WHERE gametype == GT_BOARDING


					//	players
					"CREATE TABLE IF NOT EXISTS players"
					"("
						"client		INTEGER, "
						"cash		INTEGER, "
						"debt		INTEGER, "
						"credit		INTEGER, "
						"inventory	INTEGER, "
						"networth	INTEGER, "
						"rank		INTEGER, "
						"cargo		INTEGER, "
						"cargomin	INTEGER, "
						"cargomax	INTEGER, "
						"vote		INTEGER, "
						"ready		INTEGER, "		//	ready to leave intermission screen
						"readytime	INTEGER, "		//	ready up to travel time
						"objective	INTEGER, "
						"stash		INTEGER, "
						"contacts	INTEGER, "
						"bonus		INTEGER, "
						"bossdamage	INTEGER, "
						"score		INTEGER, "
						"awards		INTEGER, "
						"fr			INTEGER, "		// fugitive rating
						"ts			INTEGER, "		// total stash collected in a turn
						"hint		INTEGER, "
						"guard		INTEGER "
					");"
					"CREATE INDEX i ON players(client);"
					"CREATE SYNC ON players SELECT * FROM players;"

					"CREATE TABLE IF NOT EXISTS stash"
					"("
						"id		INTEGER, "
						"used	INTEGER "
					");"

					//	bosses
					"CREATE SYNC ON bosses SELECT * FROM bosses;"

					//	planets
					"ALTER TABLE planets ADD can_travel_to INTEGER, is_visible INTEGER;"
					"CREATE SYNC ON planets SELECT * FROM planets;"
					
					//	mission settings
					"CREATE SYNC ON missions SELECT key AS key, value AS value FROM missions;"

					//	initialize some boss stuff
					"UPDATE bosses SET captured=0 WHERE captured=-1;"

#if 0
					"CREATE SYNC ON dialogs SELECT id AS id,name AS name,npc_id AS npc_id,priority AS priority FROM dialogs;"
#endif
					"CREATE TEMP TABLE spawnpoints"
					"("
						"name		STRING "
					");"
					"CREATE SYNC ON spawnpoints SELECT name AS name FROM spawnpoints;"
				);

	//
	// initialize default mission values
	//
	for ( i=0; i < lengthof(defaults); i++ ) {
		if ( sql("SELECT key FROM missions SEARCH key $;", "t", defaults[i].key ) ) {
			sqldone();
		} else {
			sql("INSERT INTO missions (key,value) VALUES( $, $ );", "tte", defaults[i].key, defaults[i].value );
		}
	}

	//
	//	initialize the time
	//
	GS_LASTTIME		= BG_ST_GetMissionInt( "last_time" );
	GS_TIME			= BG_ST_GetMissionInt( "start_time" );
	GS_TURN			= BG_ST_GetMissionInt( "turn" );
	GS_PLANET		= BG_ST_GetMissionInt( "start_planet_id" );
	GS_TIMELEFT		= (BG_ST_GetMissionInt("time") - GS_TIME)+1;

	st.reset		= (GS_TURN==1);
	GS_INPLAY		= (GS_TURN>1);
	st.justloaded	= 1;

	//
	//	this flag controls if the space trader rules engine is activated or not.  we set
	//	it to one to allow calls into the rules engine.
	//
	trap_Cvar_Set( "sv_spacetrader", "1" );
	trap_Cvar_Set( "g_missiontitle", BG_ST_GetMissionString( "number" ) );
	trap_Cvar_Set( "g_gametype", va( "%d", GT_HUB ) );
	trap_Cvar_Set( "sv_missionname", mission );
	trap_Cvar_Set( "st_starttime", va("%i", trap_Milliseconds() ) );

	//	cache cargo hold's id
	sql( "SELECT id FROM commodities SEARCH abbreviation 'CH';", "sId", &st.cargo_id );

	{
		char info[ MAX_INFO_STRING ];
		BG_ST_WriteState( info, sizeof(info), &gs );
		trap_Cvar_Set( "st_spacetrader", info );
	}

	//	mark this game as hidden, so when it is saved it doesn't show up in the map list
	sql( "UPDATE OR INSERT missions SET value=1,key='hidden' SEARCH key 'hidden';", "e" );
}

static int clamp( int low, int high, int v )
{
	if ( v <= low )	return low;
	if ( v >= high ) return high;
	return v;
}

static void cache_stash()
{
	//	create a list of all the types of stash that could spawn in this turn

	//	create a number to represent what a pile of stash should be worth
	//	a pile of stash is worth roughly 2% of overall networth but no less
	//	than a minimum amount.
	int	cheapest	= BG_ST_GetMissionInt( "stash_cheapest" );
	int	maxperpile	= BG_ST_GetMissionInt( "stash_maxperpile" );
	int	pile		= 0;

	//	1% of total assets is spawned in each stash
	sql( "SELECT SUM(cash+inventory) FROM players;", "sId", &pile );
	pile = (pile * 1) / 100;

	if ( pile < cheapest ) {
		pile = cheapest;
	}

	//	mission settings can scale the pile's worth
	pile = (pile * BG_ST_GetMissionInt( "stash" )) / 100;

	while ( sql( "SELECT commodity_id, price FROM todaysprices WHERE commodities.id[ commodity_id ].good<2", 0 ) )
	{
		int commodity_id	= sqlint(0);
		int price			= sqlint(1);

		//	if this commodity would take too many to make the pile worth it then skip
		if ( (pile / price) > maxperpile )
			continue;

		//	this commodity is too pricey for these players
		if ( price > pile )
			continue;

		//	commodity is within our range, record it
		st.stash.list[ st.stash.count ].commodity_id	= commodity_id;
		st.stash.list[ st.stash.count ].low_count		= clamp( 1, maxperpile-1, (pile/4)/price );
		st.stash.list[ st.stash.count ].high_count		= clamp( st.stash.list[ st.stash.count ].low_count+1, maxperpile, pile/price );
		st.stash.count++;
	}
}

// generate a random price at range between low and high that is at least offset away from price
static int generate_price( int low, int high, int price, int offset )
{
	int		p;

	p = abs(Q_rand()) % (high-low-offset*2);
	p += low;

	//	force the price to be outside of the invalid range
	if ( p > price-offset )
		p += offset*2;

	return p;
}

static void G_ST_GeneratePrices( int timeEnd, int rangeCount, int planet_id )
{
	int timeStart;
	int event_id =-1;
	int lowend_threshold = BG_ST_GetMissionInt( "lowend_threshold" );

	sql( "SELECT event_id FROM planets SEARCH id ?1 WHERE event_end>=GS_TIME;", "isId", planet_id, &event_id );

	if ( sql( "SELECT time FROM prices SEARCH time DESC LIMIT 1;", 0 ) ) {
		timeStart = sqlint(0)+1;
		sqldone();
	} else
		timeStart = ST_START_TIME;

	if ( timeStart == ST_START_TIME ) {
		int	cash = BG_ST_GetMissionInt("wealth");

		//	force half the good commodities that the player can actually buy to be a good buy, force the other half to be a good sell
		sql( "INSERT INTO todaysprices(commodity_id,price) SELECT id,IF (@<=(TOTAL/2)) THEN (low+rnd(0,(high-low)/4)) ELSE (high-rnd(0,(high-low)/4)) FROM commodities WHERE good=1 AND high<=?2 RANDOM TOTAL;", "ie", cash );

		//	generate prices for all the rest of the commodities
		sql( "INSERT INTO todaysprices(commodity_id,price) SELECT id,rnd(low,high) FROM commodities WHERE good!=1 OR high>?2;", "ie", cash );
	}

	while ( sql( "SELECT id,low,high FROM commodities;", 0 ) )
	{
		int id		= sqlint(0);
		int	low		= sqlint(1);
		int high	= sqlint(2);
		int price	= 0;
		int	newPrice;
		int price_low	= low;
		int price_high	= high;
		int rc;

		if ( event_id >= 0 && sql( "SELECT low,high FROM commodities_events WHERE event_id=? AND commodity_id=?", "ii", event_id, id ) ) {

			low		= sqlint(0);
			high	= sqlint(1);
			sqldone();
		} else {

			sql( "SELECT low, high FROM commodities_planets WHERE commodity_id=? AND planet_id=?;", "iisIId", id, planet_id, &low, &high );
		}

		if ( sql( "SELECT price FROM todaysprices SEARCH commodity_id ?;", "i", id ) )
		{
			price = sqlint(0);
			sqldone();
		}

		if ( ((high-low)/2)<lowend_threshold ) {
			//	if this is a low end commodity, then it will tend to be either low or high
			rc = 3;
		} else
			rc = rangeCount;

		newPrice = generate_price( low, high, price, (high-low)/rc );

		trap_SQL_Prepare( "INSERT INTO prices(commodity_id,time,price) VALUES(?,?,?);" );
		trap_SQL_BindInt( 1, id );

		if ( timeStart == ST_START_TIME ) {
			trap_SQL_BindInt( 2, timeStart );
			trap_SQL_BindInt( 3, price );
			trap_SQL_Step();
		}

		trap_SQL_BindInt( 2, timeEnd );
		trap_SQL_BindInt( 3, newPrice );
		trap_SQL_Step();

		trap_SQL_Done();

		sql(	"UPDATE todaysprices SET "
					"price=?2, "
					"change=?3, "
					"range=?4 "
				"SEARCH commodity_id ?1;",
				"iiiie", id, newPrice, newPrice-price,((newPrice - price_low) * 100) / (price_high - price_low ) );
	}
}


void G_ST_GenerateDebt()
{
	int		savings_rate		= BG_ST_GetMissionInt("savings_rate" );
	int		interest_rate		= BG_ST_GetMissionInt("interest_rate" );
	int		interest_period		= BG_ST_GetMissionInt("interest_period" );
	float	rate;

	// abort if no interest rate was specified
	if ( interest_rate == 0 || interest_period == 0 )
		return;

	while ( sql( "SELECT client,debt FROM players;", 0 ) )
	{
		int		client	= sqlint(0);
		float	debt	= (float)sqlint(1);
		int		j;
		float	interest = 0.0f;

		// convert rate from yearly interest into a percentage per day
		if ( debt > 0.0f ) {
			rate = savings_rate * (1.0f / (365.0f*100.0f));
		} else {
			rate = interest_rate * (1.0f / (365.0f*100.0f));
		}


		for ( j=GS_LASTTIME+1; j<=GS_TIME; j++ ) {
			interest += debt * rate;

			if ( (j % interest_period)==0 ) {
				debt += interest;
				interest = 0.0f;
			}
		}

		sql( "UPDATE players SET debt=?2,credit=max(credit,abs(debt)*2) SEARCH client ?1;", "iie", client, (int)debt );
	}
}

static int G_ST_WillBeStoppedByMoA( void )
{
	int r = 0;
	if ( sql( "SELECT eval( missions.key[ 'Moa_Audit_Chance' ]^value );", 0 ) ) {
		r = sqlint( 0 );
		sqldone();
	}

	return r;
}

static void G_ST_QueueIntermission( const char * script, int time, qboolean restart )
{
	sql( "UPDATE players SET ready=0;", "e" );

	level.exitTime					= 0;
	level.intermissionQueued		= time;		// start the intermission, game play is over
	level.intermission_dontrestart	= !restart;	// don't leave the map at the end of this intermission, there's still more to do

	trap_Cvar_Set( "nextmap", script );
}

static void G_ST_UpdateNews()
{
	sql( "DELETE FROM todaysnews;", "e" );
	sql( "INSERT INTO todaysnews SELECT id FROM news WHERE eval( value );", "e" ); 
}

static void G_ST_BeginTurn()
{
	//	record travel log
	sql( "INSERT INTO travels(time,planet_id) VALUES(GS_TIME,GS_PLANET);", "e" );

	//	generate a new random seed that the server and clients will use	for prices
	GS_TIMELEFT = (BG_ST_GetMissionInt("time") - GS_TIME)+1;

	// extract interest payments from all the players over the time period spent traveling
	G_ST_GenerateDebt();

	G_ST_SpawnDisasters();

	//	update the prices table, this is done independantly on the client
	G_ST_GeneratePrices( GS_TIME, BG_ST_GetMissionInt( "price_flux" ), GS_PLANET );

	//	the prices have changed, update everyone's new networth!
	while( sql( "SELECT client FROM players;", 0 ) ) {
		G_ST_UpdateInventory( sqlint(0) );
	}

	G_ST_UpdateRanks();

	G_ST_RevealBosses();

	//	record history
	sql( "INSERT INTO history(client,time,networth) SELECT client,GS_TIME,networth FROM players;", "e" );

	G_ST_QueueIntermission( "st_beginturn ;", 1, qfalse );

	sql( "DELETE FROM stash;", "e" );
	sql( "UPDATE players SET ts=0;", "e" );

	sql	( "UPDATE missions SET value=?2 SEARCH key $1;", "tie", "last_time", GS_LASTTIME );
	sql	( "UPDATE missions SET value=?2 SEARCH key $1;", "tie", "start_time", GS_TIME );
	sql	( "UPDATE missions SET value=?2 SEARCH key $1;", "tie", "turn", GS_TURN );
	sql	( "UPDATE missions SET value=?2 SEARCH key $1;", "tie", "start_planet_id", GS_PLANET );
}



//	at the end of each trading round after a new planet has been picked the trading
//	awards are displayed.
static void G_ST_Awards()
{
	int totalDamage;
	int		stashBonus = 0;
	int		speedBonus = 0;
	int		totalContacts = 0;
	
	
	if ( st.assassinate.failed == qfalse) { 
		sql( "SELECT SUM(bossdamage) FROM players;", "sId", &totalDamage );

		if( !totalDamage )
			//in case the boss somehow suicides
			totalDamage = 1;

		//	reward players for getting the boss
		sql( "UPDATE players SET "
				"bossdamage=(bossdamage*100)/?1, "			//	convert boss damage to a percentage
				"bonus=bonus + ?2 + (?2*bossdamage)/300;",	//	bonus as percentage of boss damage
				"iie", totalDamage, st.assassinate.bounty );
	}
	
	if ( sql( "SELECT SUM(cash+inventory) FROM players;", 0 ) ) {

		int totalAssets = sqlint(0);
		sqldone();
		stashBonus = (totalAssets * BG_ST_GetMissionInt( "stash_bonus" )) / 100;

		if ( g_gametype.integer == GT_HUB ) {
			speedBonus = (totalAssets * BG_ST_GetMissionInt( "speed_bonus" )) / 100;
		} else {
			speedBonus = 0;
		}
	}

	sql( "SELECT COUNT(*) FROM npcs SEARCH planet_id GS_PLANET WHERE type like 'Merchant';", "sId", &totalContacts );

	if ( totalContacts>0 ) {
		while( sql( "SELECT client FROM players;", 0 ) ) {
			int contacts = 0;
			int client = sqlint(0);
			sql( "SELECT COUNT(*) FROM contacts SEARCH player ?1 WHERE npcs.id[npc]^type like 'Merchant' AND npcs.id[npc].planet_id=GS_PLANET;", "isId", client, &contacts );
			sql( "UPDATE players SET contacts=?2 SEARCH client ?1;", "iie", client, (contacts * 100) / totalContacts );
		}
	}

	//	set the final time for any player that hasn't 'readied'
	sql( "UPDATE players SET readytime=? WHERE readytime=0;", "ie", level.time );


	//	award time and stash bonus
	sql(	"UPDATE players "
				"SET "
					"stash = (stash * 100) / ?1, "
					"bonus = bonus + "
						"((stash * ?2)/100) "					// stash bonus
			"SEARCH readytime;",
			"iie", max(1,st.stash.total_spawned), stashBonus ); 

	//	award time and stash bonus
	sql(	"UPDATE players "
				"SET "
					"bonus = bonus + "
						"(((TOTAL-@-1) * ?1) / TOTAL) "		// time bonus (( TOTALPLAYERS - CURRENTRANK - 1 ) * STASHBONUS / TOTALPLAYERS
			"SEARCH readytime 0,missions.key['tradetime'].value*1000-1;",
			"ie", speedBonus ); 


	sql( "UPDATE players SET cash=cash+bonus, networth=cash+debt+inventory;", "e" );
	G_ST_UpdateRanks();

	//trap_SendServerCommand( -1, "st_awards" );
	GS_MENU = CS('a','w','a','r');
}


static void G_ST_FinalScore( void )
{
	int version = BG_ST_GetMissionInt("number" );

	int	gametime = trap_Milliseconds() - trap_Cvar_VariableIntegerValue( "st_starttime" );

	while( sql( "SELECT client,networth,skill,score FROM players WHERE objective=1;", 0 ) ) {

		int client		= sqlint(0);
		int networth	= sqlint(1);
		int skill		= sqlint(2);
		int	score		= sqlint(3);

		// st_score <score> <skill> <kills> <time> <game> <real time>
		trap_SendServerCommand( client, va("st_reportscore %d %d %d %d %d %d ;",
													networth,
													skill,
													score,
													GS_TURN,
													version,
													gametime/1000
												) );
	}

	trap_SendServerCommand( -1, "st_postgame\n" );
}

//	the players have been stopped by the MoA.  They must vote if they will simply
//	pay the fine or try to fight it out.
static void G_ST_MoAAudit( void )
{
	//	players will have to vote if they want to pay the bill or fight. therefore,
	//	the vote parameters need to be set up
	G_Vote_SetPassedScript	( "st_tax ; st_travel 2 ;" );	// continue to next stage
	G_Vote_SetFailedScript	( "flush ; g_gametype %d; map ship;", GT_MOA );

	//	clear voting status
	sql( "UPDATE players SET vote=0;", "e" );

	level.voteTime = level.time;

	//	vote to be boarded
	//trap_SendServerCommand( -1, "ui_vote 1 moa ;" );
	GS_MENU = CS('m','o','a',0);
}

//	the players will reach their destination and get to see the results of the turn.
static void G_ST_ArriveSafely()
{
	//	travel!
	GS_INPLAY	= 0;
	GS_TURN++;

	if ( sql( "SELECT map FROM planets WHERE id=?", "i", GS_PLANET  ) )
	{
		//	the 'wait 250' is a sad attempt to avoid the problem where the map command is executed
		//	before all the client commands are sent out.  bug # 4073.  after a travel the client
		//	gets all the right data, and then on the first frame a rogue command comes in from the
		//	previous map and reverts data in the table.
		trap_SendConsoleCommand( EXEC_APPEND, va( "flush ; g_gametype %d; map %s;", GT_HUB, sqltext(0) ) );
		sqldone();

	} else
	{
		Com_Error( ERR_FATAL, "No map for planet '%d'.\n", GS_PLANET );
	}
}

#ifdef DEVELOPER
static void st_add_spawnpoints() {
	int i;
	for(i=0; i < MAX_GENTITIES; i++) {
		gentity_t *spawn = g_entities + i;
		if (spawn->inuse == qtrue && Q_stricmpn("info_bot_", spawn->classname, 9) == 0 ) {
			if (spawn->targetname) {
				sql("UPDATE OR INSERT spawnpoints SET name = $1 SEARCH name $1", "te", spawn->targetname);  
			}
		}
	}

}
#endif

static void st_bots_hub() {
	while( sql( "SELECT name, model, type, id, spawn FROM npcs WHERE planet_id = ? AND (type like 'Informant' OR type like 'Merchant');", "i", GS_PLANET) )
	{
		const char *	spawn_name	= sqltext(4);
		gentity_t *		npc_spawn	= G_Find( NULL, FOFS(targetname), spawn_name );
		gentity_t *		guard_spawn = NULL;

		//	spawn merchant or informant
		G_AddBot( sqltext(2), sqltext(1), sqltext(0), sqlint(3), g_spSkill.integer - 1.0f, "red", 0, npc_spawn );

		//	spawn merchants' guards
		while ( ( guard_spawn = G_Find(guard_spawn, FOFS(target), spawn_name ) ) != NULL)  {
			if ( Q_stricmp( guard_spawn->classname, "info_bot_guard") == 0)
				G_AddBot( "guard", "moasoldier", "Guard", -1, g_spSkill.integer, "red", 0, guard_spawn);
		}
	}
#ifdef DEVELOPER
	st_add_spawnpoints();
#endif
}

static void st_bots_boarding() {

	int		total;
	int		i,n;
	int		spawned=0;

	total = BG_ST_GetMissionInt( "moa_guard_min" );
	n = BG_ST_GetMissionInt( "moa_max_atonce" );


	//	number of moa guards depends on the fugitive rating.
	if ( sql( "SELECT SUM(fr), COUNT(*) FROM players;", 0 ) ) {
		int fr = sqlint(0);
		int clients = sqlint(1);
		sqldone();

		total *= clients;
		total += (BG_ST_GetMissionInt( "moa_guard_count" ) * fr) / 100;

		if ( fr == 0 ) {
			sql( "UPDATE players SET guard=?1/TOTAL;", "i", total );
		} else {
			sql( "UPDATE players SET guard=(fr*?1)/?2;", "ii", total, fr );
		}
	}

	if ( n > total ) {
		n = total;
	}

	//	spawn Auditors, 1/8th of all the guards are auditors
	for ( i=0; i<n>>3; i++ ) {
		G_AddBot( "guardhard", "moasoldier/auditor", "Auditor",	-1, g_spSkill.value + 1.0f, "red", 0, NULL );
	}

	//	spawn Analysts, 3/8ths of all the gaurds are analysts
	for ( i=0; i<(n*3)>>3; i++ ) {
		G_AddBot( "guard", "moasoldier", "Analyst",	-1, g_spSkill.value, "red", 0, NULL );
	}

	//	spawn Agents, 1/2 of all guards are agents
	for ( i=0; i<n>>1; i++ ) {
		G_AddBot( "guardeasy", "moasoldier/agent", "Agent", -1, g_spSkill.value - 1.0f, "red", 0, NULL );
	}


	sql( "SELECT COUNT(*) FROM clients SEARCH client ?1, ?2;", "iisId", g_maxplayers.integer, MAX_CLIENTS, &spawned );

	GS_ASSASSINATE.dead_countdown = 0;

	sql( "UPDATE OR INSERT boarding SET key=$1, value=?2 SEARCH key $1", "tie", "guards_left", total );
	sql( "UPDATE OR INSERT boarding SET key=$1, value=?2 SEARCH key $1", "tie", "spawn_queue", total-spawned );
}

static void st_bots_assassinate() {
	gentity_t * spawn;

	for ( spawn=g_entities ; spawn < &g_entities[level.num_entities] ; spawn++ )
	{
		if ( !spawn->inuse ) {
			continue;
		}

		if ( Q_stricmpn( spawn->classname, "info_bot_guard", 14 ) ) {
			continue;
		}

		switch( SWITCHSTRING(spawn->classname+14) )
		{
		case 0:
			{
				if ( st.assassinate.gang_left_medium > 0 ) {
					G_AddBot( spawn->classname+9, "thug/medium", "Goon", -1, g_spSkill.integer - 1.0f , "red", 0, spawn );
					st.assassinate.gang_left_medium--;
				}
			} break;
		case CS('e','a','s','y'):
			{
				if ( st.assassinate.gang_left_easy > 0 ) {
					G_AddBot( spawn->classname+9, "thug/easy", "Oaf", -1, g_spSkill.integer - 2.0f, "red", 0, spawn );
					st.assassinate.gang_left_easy--;
				}
			} break;
		case CS('h','a','r','d'):
			{
				if ( st.assassinate.gang_left_hard > 0 ) {
					G_AddBot( spawn->classname+9, "thug/hard", "Enforcer", -1, g_spSkill.integer, "red", 0, spawn );
					st.assassinate.gang_left_hard--;
				}
			} break;
		}
	}

	if ( sql( "SELECT COUNT(*) FROM players;", 0 ) )
	{
		int c = sqlint(0);
		sqldone();
		st.assassinate.gang_left_medium += c/2;
		st.assassinate.gang_left_easy += c/2;
		st.assassinate.gang_left_hard += c/2;
	}
	
	GS_ASSASSINATE.dead_countdown = 0;
	
}


static int G_ST_ConsoleCommand( const char * cmd )
{
	switch( SWITCHSTRING( cmd ) )
	{

			//	st_beginturn - player is ready to start the new turn
		case CS('b','e','g','i'):
			{
				G_ST_UpdateContacts();

				//	check to see if there's an npc who wants to talk to the players straight away
				while( sql( "SELECT client FROM players;", 0 ) )
				{
					int client = sqlint(0);
					if ( !G_ST_SayStartDialog( client ) )
					{
						// if no npc wants to talk to the player send them to the information screen
						trap_SendServerCommand( client, "st_beginturn" );
					}
				}

				st.objectives.time = level.time;
				st.travelwarn = BG_ST_GetMissionInt( "tradetime" ) * 1000;
				if ( st.travelwarn ) {
					st.travelwarn += level.time;
				}

				GS_INPLAY = 1;

			} break;

		case CS('r','e','t','i'):	//	st_retire
			{
				const char * nextChallenge = BG_ST_GetMissionString( "next_challenge" );

				G_ST_Awards();

				if ( nextChallenge && Q_stricmp( nextChallenge, "0" ) )
				{
					//	start a new game of spacetrader at the end of the intermission
					G_ST_QueueIntermission( va("flush ; set sv_spacetrader 0 ; map %s", nextChallenge ), 1, qtrue );
					break;
				}
				trap_SendServerCommand( -1, "st_announcer award_completed 1500\n" );
				G_ST_FinalScore();

				sql("UPDATE OR INSERT missions SET value='1' SEARCH key 'complete';", "e" );
				trap_SendConsoleCommand( EXEC_APPEND, "sql_export \"db/continue.mis\"\n" );

				//	fire up the intermission screen to tell everyone the game is over, this
				//	also gives the clients a chance to get all the final info before the
				//	server goes down
				G_ST_QueueIntermission( "killserver", 1, qfalse );

			} break;

		case CS('a','b','o','r'):	//	st_abort
			{
				if ( g_gametype.integer == GT_ASSASSINATE ) {
					sql( "UPDATE bosses SET planet_id=-1 WHERE id=?;", "ie", GS_BOSS );	//	boss escapes...
					returntohub();	//	players go back to hub

				} else if ( g_gametype.integer == GT_MOA ) {

					G_ST_Moa_Lose();
				}
			} break;

			//
			//	st_travel
			//
			//	this command is executed after a succesful vote.
			//
		case CS('t','r','a','v'):
			{
				int	stage = trap_ArgvI( 1 );

				switch( stage )
				{
					//	successful vote has been made for a destination planet
				case 0:
					{
						//	on with the first stage, giving out rewards!
						G_ST_Awards();

						//	queue the intermission, when everyone has clicked ready the following
						//	command sends us to the next stage of traveling.
						G_ST_QueueIntermission( va("st_travel 1 %d ;",trap_ArgvI(2)), 1, qfalse );

					} break;

					//	everyone has viewed and clicked to leave the awards screen. the next thing
					//	to do is show the traveling animation and determing if the ship will be
					//	boarded or not.
				case 1:
					{
						//	record the destination planet, there's no way to change this now.  once we're done
						//	displaying the awards and travel animation we can use this to finish the travel
						//	calculations.
						if ( sql( "SELECT id, travel_time FROM planets SEARCH id ?;", "i", trap_ArgvI( 2 ) ) )
						{
							GS_PLANET	= sqlint(0);
							GS_LASTTIME	= GS_TIME;
							GS_TIME		= GS_TIME + sqlint(1);
							sqldone();
						}

						if ( G_ST_WillBeStoppedByMoA() )
						{
							G_ST_MoAAudit();

						} else
						{
							G_ST_ArriveSafely();
						}
						
					} break;

					//	players have finished dealing with the MoA and will now safely travel to destination planet
				case 2:
					{
						//	continue traveling
						G_ST_ArriveSafely();

					} break;
				}

			} break;

			//	st_assassinate
		case CS('a','s','s','a'):
			{
				int		id	= trap_ArgvI( 1 );
				char	map [ MAX_OSPATH ];
				
				if ( !id ) {
					char	boss[ MAX_OSPATH ];
					trap_Argv( 1, boss, sizeof( boss ) );
					sql( "SELECT id FROM npcs SEARCH name $1;", "tsId", boss, &id );
				}

				trap_Argv( 2, map, sizeof( map ) );

				GS_BOSS = id;
				GS_ASSASSINATE.boss_escaping 	= qfalse;
				GS_ASSASSINATE.boss_spawned		= 0;
				
				//	remember how much stash was picked up, so when we return to the hub
				trap_SendConsoleCommand( EXEC_INSERT, va( "flush ; g_gametype %d; map %s;",GT_ASSASSINATE, map ) );

			} break;
			//	st_tax
		case CS('t','a','x',0):
			{
				//	tax the players
				sql( "UPDATE players SET debt=debt-max(missions.key['moa_mintax'].value,((inventory+cash)*missions.key['moa_taxrate'].value)/100),networth=cash+debt+inventory;", "e" );
				G_ST_UpdateRanks();

			} break;
			//	st_bots - load the bots
		case CS('b','o','t','s'):
			{
				switch( g_gametype.integer )
				{
				case GT_HUB:			st_bots_hub();			break;
				case GT_MOA:			st_bots_boarding();		break;
				case GT_ASSASSINATE:	st_bots_assassinate();	break;
				}

			} break;
		default:
			return 0;
	}

	return 1;
}
//this may be removed once the FindSpawnNearOrigin code gets tested a bit more
static gentity_t *G_ST_FindSpawnNextToMerchant ( int merchant )
{
	gentity_t*	spot = NULL;

	if ( sql( "SELECT spawn FROM npcs WHERE id=?;", "i", merchant ) )
	{
		const char * spawn = sqltext(0);

		while ((spot = G_Find (spot, FOFS(classname), "info_notnull")) != NULL)
		{
			if ( Q_stricmp( spawn, spot->target ) )
				continue;

			if ( SpotWouldTelefrag( spot->s.origin ) ) {
				continue;
			}

			break;
		}

		sqldone();
	}

	return spot;
}


static void G_ST_WarnLastTrader( int client )
{
	if ( sql( "SELECT SUM(1), SUM(readytime==0) FROM players;", 0 ) ) {

		int	total		= sqlint(0);
		int	notready	= sqlint(1);
		sqldone();

		if ( total > 1 ) {

			if ( notready == 0 ) {
				trap_SendServerCommand( -1, "st_cp T_Everyone_Is_Ready newcontact" );

			} else if ( notready == 1 && total > 2 ) {

				if ( sql( "SELECT client FROM players WHERE readytime=0;", 0 ) ) {
					trap_SendServerCommand( sqlint(0), "st_cp T_You_Are_Last newcontact" );
					sqldone();
				}
			} else {

				if ( sql( "SELECT client FROM players SEARCH client ?1 WHERE readytime!=0;", "i", client ) ) {
					trap_SendServerCommand( -1, va( "st_cp T_Player_Is_Ready newcontact %i", sqlint(0) ) );
					sqldone();
				}
			}
		}
	}
}

static int G_ST_CanTravel( int client, int planet_id )
{
	if( GS_PLANET == planet_id )
		return 3;	// already there

	if ( sql( "SELECT client FROM players WHERE readytime=0 AND client!=?1;", "i", client ) ) {
		sqldone();
		return 1;	// not everyone ready
	}

	if ( sql( "SELECT travel_time,is_visible,can_travel_to FROM planets SEARCH id ?1;", "i", planet_id ) ) {

		int	travel_time = sqlint(0);
		int is_visible = sqlint(1);
		int can_travel_to = sqlint(2);
		sqldone();

		if ( travel_time > GS_TIMELEFT )
			return 2;	// too far

		if ( !is_visible || !can_travel_to )
			return -1;	// hidden
	}

	return 0;
}

#ifdef DEVELOPER
static char * Arg( int i ) {
	char buffer[ 255 ];
	char * s = buffer;
	trap_Argv( i, buffer, sizeof(buffer) );
	return COM_Parse( &s );
}
#endif

int ArgCommodity( int arg )
{
#ifdef DEVELOPER
	char * buffer = Arg( arg );
	int commodity_id = atoi( buffer );
	sql( "SELECT id FROM commodities WHERE name like $1 OR abbreviation like $1;", "tsId", buffer, &commodity_id );
	return commodity_id;
#else
	return trap_ArgvI( arg );
#endif
}

int ArgPlanet( int arg )
{
#ifdef DEVELOPER
	char * buffer = Arg( arg );
	int planet_id = atoi( buffer );
	sql( "SELECT id FROM planets WHERE name like $;", "tsId", buffer, &planet_id );
	return planet_id;
#else
	return trap_ArgvI( arg );
#endif
}

int ArgBot( int arg )
{
#ifdef DEVELOPER
	char * buffer = Arg( arg );
	int npc_id = atoi( buffer );
	sql( "SELECT clients.id[ id ].client FROM npcs WHERE name like $;", "tsId", buffer, &npc_id );
	return npc_id;
#else
	return trap_ArgvI( arg );
#endif
}


static void G_ST_ClientCommand( int client, const char * cmd )
{
	int cmd_id = SWITCHSTRING(cmd);

	// ignore all commands from the clients during an intermission, except for st_intermission
	if ( level.intermissionQueued || level.intermissiontime )
	{
		if ( cmd_id != CS('i','n','t','e') && cmd_id != CS('b','e','g','i') &&
			cmd_id != CS( 'c', 'h', 'a', 't' ) )
			return;
	}

	switch( SWITCHSTRING( cmd ) )
	{
			//	st_planet - client has selected a planet
		case CS('p','l','a','n'):
			{
				int		planet_id		= ArgPlanet( 1 );
				int		r				= G_ST_CanTravel( client, planet_id );

				trap_SendServerCommand( client, va( "st_planet %d %d ;", r, planet_id ) );

			} break;

			//	st_buy
		case CS('b','u','y',0 ):
			{
				int		commodity_id	= ArgCommodity( 1 );
				int		qty				= trap_ArgvI( 2 );

				if ( sql( "SELECT price FROM prices SEARCH time GS_TIME WHERE commodity_id=?;", "i", commodity_id ) ){
					trade( client, commodity_id, qty, sqlint(0) );
					sqldone();
				}

			} break;

			//	st_sell
		case CS('s','e','l','l'):
			{
				int		commodity_id	= ArgCommodity( 1 );
				int		qty				= trap_ArgvI( 2 );

				if ( sql( "SELECT price FROM prices SEARCH time GS_TIME WHERE commodity_id=?;", "i", commodity_id ) ){
					trade( client, commodity_id, -qty, sqlint(0) );
					sqldone();
				}

			} break;

			//	st_withdraw_cash
		case CS('w','i','t','h'):
			{
				int n = trap_ArgvI(1);

				sql(	"UPDATE players SET "
							"cash=cash+?2, "
							"credit=credit-?2, "
							"debt=debt-?2 "
						"SEARCH client ?1 "
						"WHERE credit>=?2;", "iie", client, n );
				trap_SendServerCommand( client, "st_bank;" );
			} break;
			//	st_deposit_cash
		case CS('d','e','p','o'):
			{
				int n = trap_ArgvI(1);

				sql(	"UPDATE players SET "
							"cash=cash-?2, "
							"credit=credit+?2, "
							"debt=debt+?2 "
						"SEARCH client ?1 "
						"WHERE cash>=?2;", "iie", client, n );
				trap_SendServerCommand( client, "st_bank;" );
			} break;
			// st_startgame - local client says its good to start the game, everyone's connected
		case CS( 's','t','a','r' ):
			{
				level.playerStartTime = level.time + 6000;
				// tell all the clients that they can start the game now
				trap_SendServerCommand( -1, "st_startgame ;" );
			} break;
			//	st_lookat
		case CS('l','o','o','k'):
			{
				int		bot			= trap_ArgvI( 1 );
				int		eye_height	= trap_ArgvI( 2 );

				gentity_t * ent			= g_entities + client;
				gentity_t * target_bot	= g_entities + bot;
				vec3_t bot_eye;
				vec3_t dir;
				vec3_t angles;

				VectorCopy( target_bot->client->ps.origin, bot_eye );
				bot_eye[2] -= eye_height;

				VectorSubtract( bot_eye, ent->client->ps.origin, dir );
				vectoangles(dir, angles);

				SetClientViewAngle( ent, angles );

			} break;
			//	st_visit
		case CS( 'v','i','s','i' ):
			{
				int		bot			= trap_ArgvI( 1 );
				int		eye_height	= trap_ArgvI( 2 );
				
				if ( bot != -1 && G_ST_TeleportPlayerToNPC( client, bot, eye_height ) )
				{
					gentity_t * target_bot = g_entities + bot;
					G_Say( g_entities + client, target_bot, SAY_TELL, va("dialog %s do say", target_bot->client->pers.netname) );
				}

			} break;

			//	st_endi
		case CS( 'e','n','d','i' ):
			{
				int			bot = ArgBot(1);
				gentity_t*	ent = g_entities + client;

				BotDoneTalking(bot, 1, g_entities + client );

				if ( sql( "UPDATE conversations SET bot=-1,dialog_id=-1 SEARCH player ? WHERE bot=?;", "iie", client, bot ) ) {
					G_Voice( &g_entities[bot], ent, SAY_TELL, "goodbye", qfalse);	// ?? this doesn't belong here :(
				}

			} break;

			//	st_ready - player is ready to travel or not
		case CS( 'r','e','a','d' ):
			{
				if ( !st.travelwarn || (level.time < st.travelwarn) ) {
					sql( "UPDATE players SET readytime=(readytime=0)*?2 SEARCH client ?1;", "iie", client, level.time );
					G_ST_WarnLastTrader(client);
				}

			} break;

			//	st_intermission - player is ready to end a turn
		case CS('b','e','g','i'):
		case CS('i','n','t','e'):
			{
				sql( "UPDATE players SET ready=1 SEARCH client ?1;", "ie", client );

			} break;


			//	st_dialog
			//
			//	this event gets called when a player is responds to a bot's dialog chain
			//
			//	parameters :
			//		client	- id of the player that is wanting to talk
			//		bot		- id of the bot the player wants to talk to
			//		answer	- text id of the response chosen by the player
			//
		case CS( 'd','i','a','l' ):
			{
				int		bot		= trap_ArgvI( 1 );
				int		answer	= trap_ArgvI( 2 );

				if ( sql( "SELECT dialog_id, npc FROM conversations SEARCH player ?1 WHERE bot=?2;", "ii", client, bot ) )
				{
					int dialog_id	= sqlint(0);
					int	npc_id		= sqlint(1);
					sqldone();

					if ( dialog_id == G_ST_JOBS_DIALOG_ID )
					{
						//G_Say(&g_entities[clientId], &g_entities[bot], SAY_TELL, va("dialog %s do job %d",level.clients[bot].pers.netname, answer));
						G_ST_DoDialogCommand( client, bot, npc_id, va( "job %d", answer ) );

					} else if ( sql( "SELECT cmd FROM dialogs_cmds SEARCH id ? WHERE line=?;", "ii", dialog_id, answer ) ) {

						//G_Say(&g_entities[client], &g_entities[bot], SAY_TELL, va("dialog %s do %s",level.clients[bot].pers.netname, cmd));
						G_ST_DoDialogCommand( client, bot, npc_id, (char*)sqltext(0) );
						sqldone();
					}
				}

			} break;

			//	st_chat - chatting at the connect screen
		case CS('c','h','a','t'):
			{
				char t[ MAX_SAY_TEXT ];
				trap_Argv( 1, t, sizeof(t) );

				// forward this to all other clients
				if ( t[0] != '\0' )
					trap_SendServerCommand( -1, va( "st_chat %i \"%s\" ;", client, t ) );
				

			} break;

			//	st_cargo_upgrade
		case CS('c','a','r','g'):
			{
				int	qty		= trap_ArgvI( 1 );

				if ( sql( "SELECT id, todaysprices.id[ id ].price FROM commodities SEARCH abbreviation 'CH';", 0 ) ) {

					int	cargo_id	= sqlint(0);
					int price		= sqlint(1);
					sqldone();

					trade( client, cargo_id, qty, price );
				}

			} break;
	}
}

static int G_ST_CheckIntermissionExit( void ) {

	// don't exit too soon
	if ( level.time < level.intermissiontime + 500 ) {
		return 0;
	}

	if ( sql( "SELECT SUM(ready>0),COUNT(*) FROM players;", 0 ) )
	{
		int	ready	= sqlint(0);
		int	total	= sqlint(1);
		sqldone();

		// if no one is ready then wait
		if ( ready == 0 || total == 0 )
			return 0;

		// if everyone wants to go, go now
		if ( ready == total ) {
			return 1;
		}

		//	if at connect screen then go immediately
		if ( GS_INPLAY == 0 && GS_TURN == 1 ) {
			return 1;
		}
		
		//	the first person to ready starts the timer
		if ( !level.exitTime ) {
			level.exitTime = level.time;
		}

		// if we have waited long enough since at least one player
		// wanted to exit, go ahead and exit now
		if ( level.time - level.exitTime > 3000 ) {
			return 1;
		}
	}

	return 0;
}


static void G_ST_InitPlayer( int client )
{
	int		start_cash		= BG_ST_GetMissionInt("wealth");
	int		start_debt		= -BG_ST_GetMissionInt("debt" );
	int		start_credit	= BG_ST_GetMissionInt("credit" );
	int		start_cargomax	= BG_ST_GetMissionInt("cargo" );

	// clear all info for this client from the db, a player may have disconnected and then reconnected...
	sql( "DELETE FROM contacts WHERE player=?;", "ie", client );

	sql(	"UPDATE OR INSERT players SET "
				"client = ?1, "
				"cash = ?2, "
				"debt = ?3, "
				"cargomin = ?4, "
				"cargomax = ?4, "
				"credit = ?5, "
				"networth = cash+debt+inventory "
			"SEARCH client ?1;",
			"iiiiiie",
			client,
			start_cash,
			start_debt,
			start_cargomax,
			start_credit
		);

	// give the player any starting commodities
	while( sql( "SELECT commodity_id, amount FROM resources", 0) ) {
		insert_purchase( client, sqlint( 0 ), sqlint( 1 ), 0 );
	}
}

static void G_ST_Moa_Win()
{
	int moa_fr_goods;
	int moa_fr_kills;
	int total_kills=0;
	int total_goods=0;

	//	the players have successfully escaped the MoA.	Show the result screen and then continue
	//	on to traveling to the destination planet

	//	queue the intermission, when everyone has clicked ready the following
	//	command sends us to the next stage of traveling.
	G_ST_QueueIntermission( "st_travel 2 ;", 1, qfalse );


	moa_fr_goods = BG_ST_GetMissionInt( "moa_fr_goods" );
	moa_fr_kills = BG_ST_GetMissionInt( "moa_fr_kills" );

	sql( "SELECT SUM(score) FROM players;", "sId", &total_kills );
	sql( "SELECT SUM(commodities.id[commodity_id].good==0) FROM inventory;", "sId", &total_goods );

	while( sql( "SELECT client,score FROM players;", 0 ) ) {

		int client = sqlint(0);
		int score = sqlint(1);

		//	fugitive ranking goes up based on the amount of illegal goods in the inventory
		if ( sql( "SELECT SUM(commodities.id[commodity_id].good==0) FROM inventory SEARCH player ?1;", "i", client ) )
		{
			int goods	= sqlint( 0 );
			int fr		= Q_rrand( 2,5 );
			sqldone();

			if ( score && total_kills ) {
				fr += (moa_fr_kills * score) / total_kills;
			}

			if ( goods && total_goods ) {
				fr += (moa_fr_goods * goods) / total_goods;
			}

			//	increase the fugitive rankings of each player
			sql( "UPDATE players SET fr=fr+?2 SEARCH client ?1;", "iie", client,fr );
		}
	}

	//	show the MoA won screen
	//trap_SendServerCommand( -1, "st_moa 1 ;" );
	GS_MENU = CS('m','o','a','w');
}

static void G_ST_Moa_Lose()
{
	G_ST_QueueIntermission( "st_travel 2;", 1, qfalse );

	//	add taxes penalty, tax_rate + 3 percent boarding + 2 percent per soldier
	if ( sql( "SELECT missions.key['moa_taxrate'].value, missions.key['moa_mintax'].value;", 0 ) ) {
		int taxrate = sqlint(0);
		int mintax = sqlint(1);
		sqldone();

		sql( "UPDATE players SET bonus=-(max(?2,((cash+inventory)*?1)/100) + (((cash+inventory)*(3+score))/100)  ),debt=debt+bonus,networth=debt+cash+inventory;", "iie", taxrate, mintax );
	}

	G_ST_UpdateRanks();

	//	show the MoA failed screen
	//trap_SendServerCommand( -1, "st_moa 0 ;" );
	GS_MENU = CS('m','o','a','f');

	//	add time penalty
//	enable this when the gui displays it
	//	sql( "SELECT SUM(credit+inventory) FROM players;", "sId", &totalassets );
	//	GS_TIME += totalassets / 100000;
}

static void returntohub() {
	//	the mission has ended, go back to hub
	if ( sql( "SELECT map FROM planets WHERE id=?;", "i", GS_PLANET ) )
	{
		G_ST_Awards();
		G_ST_QueueIntermission( va( "flush ; g_gametype %d; map %s;", GT_HUB, sqltext(0) ), 1, qtrue );
		sqldone();
	}
}

//
//	The Space Trader rules engine
//
int QDECL G_ST_exec( spacetraderCmd_t cmd, ... )
{
	va_list		argptr;
	va_start(argptr, cmd);

	if ( g_spacetrader.integer == 0 && cmd != ST_BOOT_GAME )
		return 0;

	switch ( cmd )
	{
		case ST_BOOT_GAME:
		{
			char			mission[ MAX_QPATH ];
			fileHandle_t	f;
			int i;

			trap_Cvar_VariableStringBuffer( "mapname", mission, sizeof(mission) ); 

			if ( trap_FS_FOpenFile( va( "db/%s.mis", mission ), &f, FS_READ ) > 0 )
			{
				trap_FS_FCloseFile( f );

				G_ST_BeginGame( mission );

				if ( sql( "SELECT map FROM planets WHERE id=?;", "i", GS_PLANET ) )
				{
					// set serverinfo visible name
					trap_Cvar_Set( "mapname", sqltext(0) );
					sqldone();
				}

			}

			// if space trader is not running...
			if ( !trap_Cvar_VariableIntegerValue("sv_spacetrader") ) {

				trap_SQL_Reset();

				//	let clients know what the server coded weapon info is
				trap_SQL_Exec(	"CREATE TABLE IF NOT EXISTS missions ( key STRING, value STRING );"
								"CREATE SYNC ON missions SELECT key AS key, value AS value FROM missions;" );
				trap_SQL_Prepare( "UPDATE OR INSERT missions SET key=$1,value=$2 SEARCH key $1;" );
				for ( i=0; i < WP_NUM_WEAPONS; i++ ) {
					trap_SQL_BindText( 1, va("clipsize%i",i) );
					trap_SQL_BindText( 2, fn( G_ClipAmountForWeapon( i ), FN_PLAIN ) );
					trap_SQL_Step();
				}
				trap_SQL_Done();
			}


		} break;

		//
		//	called everytime the game module is initialized
		//
		case ST_INIT_GAME:
		{
			char info[ MAX_INFO_STRING ];

			//	game is reset every map change, restore the game state from the cvar so
			//	we can continue playing
			trap_Cvar_VariableStringBuffer( "st_spacetrader", info, sizeof(info) );

			BG_ST_ReadState( info, &gs );

			sql( "SELECT id FROM commodities SEARCH abbreviation 'CH';", "sId", &st.cargo_id );

			sql( "UPDATE players SET readytime=0,bossdamage=0,bonus=0,vote=0,ready=0,score=0;", "e" );

			//	update bosses first, because updateplanets checks status of bosses.
			if ( GS_INPLAY == 0 ) {
				G_ST_RevealBosses();
			}

			//	calculate travel times to all other planets, this is done every map
			//	load since a new planet could be revealed from a combat mission
			G_ST_UpdatePlanets();

			//	make sure the clients don't start to play until its time
			if ( GS_INPLAY == 0 ) {
				G_ST_BeginTurn();
			}

			//	start stash count from last time at this hub
			if ( g_gametype.integer == GT_HUB ) {
				sql( "UPDATE players SET stash=ts;", "e" );
			} else {
				sql( "UPDATE players SET ts=stash, stash=0;", "e" );
			}

			G_ST_UpdateNews();

			GS_COUNTDOWN = 0;
			GS_MENU = 0;

			//	going to and coming from a combat mission, create a countdown
			//	when there's more than one player.
			if ( GS_INPLAY == 1 ) {
				if ( sql("SELECT COUNT(*) FROM players;", 0 ) ) {
					int player_count = sqlint(0);
					sqldone();
					if ( player_count > 1 )
						GS_COUNTDOWN = 3;
				}
			}

			if ( g_gametype.integer != GT_MOA )
				cache_stash();
			else {
				//	2% of total assets is spawned in each stash
				int total = 1;
				sql( "SELECT SUM(cash+inventory),COUNT(*) FROM players;", "sIId", &st.moa.cash, &total );
				st.moa.cash /= total;
			}

			if ( g_gametype.integer == GT_ASSASSINATE ) {

				GS_ASSASSINATE.health = GS_ASSASSINATE.maxhealth = 100;

				sql( "SELECT SUM(count) FROM henchmen WHERE boss_id=? AND type=0;", "isId", GS_BOSS, &st.assassinate.gang_left_easy );
				sql( "SELECT SUM(count) FROM henchmen WHERE boss_id=? AND type=1;", "isId", GS_BOSS, &st.assassinate.gang_left_medium );
				sql( "SELECT SUM(count) FROM henchmen WHERE boss_id=? AND type=2;", "isId", GS_BOSS, &st.assassinate.gang_left_hard );

				st.assassinate.gang_count =
					st.assassinate.gang_left_easy +
					st.assassinate.gang_left_medium +
					st.assassinate.gang_left_hard;
					
				st.assassinate.dead_time = 0;

				sql( "SELECT bounty FROM bosses WHERE id=?;", "isId", GS_BOSS, &st.assassinate.bounty );

				if ( sql( "SELECT npcs.id[ GS_BOSS ]^name;", 0 ) ) {
					trap_Cvar_Set( "boss", sqltext(0) );
				}
			} else {
				trap_Cvar_Set( "boss", "" );
			}

			

			BG_ST_WriteState( info, sizeof(info), &gs );

			//	pass game state to clients via configstring
			trap_SetConfigstring( CS_SPACETRADER, info );

			//	start music
			switch( g_gametype.integer )
			{
			case GT_ASSASSINATE:
				if ( sql( "SELECT music FROM bosses WHERE id=?;", "i", GS_BOSS ) ) {
					trap_SetConfigstring( CS_MUSICTRACK, sqltext(0) );
					sqldone();
				}
				break;
			case GT_MOA:
				trap_SetConfigstring( CS_MUSICTRACK, BG_ST_GetMissionString( "moa_music" ) );
				trap_SendServerCommand( -1, "st_announcer tip_boarding 3500" );
				break;
			default:
				if ( sql( "SELECT music FROM planets WHERE id=?", "i", GS_PLANET ) ) {
					trap_SetConfigstring( CS_MUSICTRACK, sqltext(0) );
					sqldone();
				}
			}

			//	clear clients table
			sql( "DELETE FROM clients;", "e" );

			st.objectives.scramble = BG_ST_GetMissionInt( "scramble" );


			if ( GS_TURN > 1 ) {
				if ( st.justloaded ) {
					G_ST_ConsoleCommand( "beginturn" );
				} else {
					if ( trap_Cvar_VariableValue( "ui_netSource" ) == -1 || 
							(trap_Cvar_VariableValue( "ui_netSource" ) == 1 && trap_Cvar_VariableValue( "ui_singlePlayerActive" ) == 1  ) ) {
						trap_SendConsoleCommand( EXEC_APPEND, "sql_export \"db/continue.mis\"\n" );
						trap_SendServerCommand( -1, "st_cp T_Progress_Saved newcontact" );
					}
				}
			}

		} break;

		case ST_SHUTDOWN_GAME:
			{
				char info[ MAX_INFO_STRING ];
				BG_ST_WriteState( info, sizeof(info), &gs );
				trap_Cvar_Set( "st_spacetrader", info );
			} break;

		case ST_INTERMISSION:
			return G_ST_CheckIntermissionExit();

		//
		// A player has (re)connected, update their debt, inventory worth, cash on hand.
		case ST_SPAWN_PLAYER:
		{
			int client = va_arg( argptr, int );

			if ( st.reset )
				G_ST_InitPlayer( client );

			G_ST_UpdateRanks();

			g_entities[ client ].health =
			level.clients[ client ].ps.stats[STAT_HEALTH] =
			level.clients[ client ].ps.stats[STAT_MAX_HEALTH] =
			level.clients[ client ].pers.maxHealth = 100;
			level.clients[ client ].ps.stats[STAT_MAX_HEALTH] = 200;

			// no shields in hub
			if ( g_gametype.integer == GT_HUB ) {
				level.clients[ client ].ps.shields = 0;
			} else {
				level.clients[ client ].ps.shields = 100;
			}

#if 0
			if( trap_Cvar_VariableIntegerValue( "com_developer" ) && g_gametype.integer == GT_HUB )
			{
				sql( "UPDATE npcs SET planet_id=?1 WHERE id=?;", "iie", 1, 84 );
				trap_SendConsoleCommand( EXEC_APPEND, "wait ; wait ; st_assassinate 84" );
			}
#endif
			
		} break;
		case ST_SPAWN_BOT:
		{
			int client = va_arg( argptr, int );
			gentity_t *ent = g_entities + client;
			gentity_t *spawn = va_arg( argptr, gentity_t *);
			int health, armor;
			//	a bot has spawned into a hub, update all the client's contact info
			health=armor=100;

			switch( g_gametype.integer )
			{
			case GT_HUB:
			{
				if (spawn && spawn->message) {
					G_Say(ent, ent, SAY_TELL,va("%s %s", ent->client->pers.netname, spawn->message) );
				}
			} break;
			case GT_ASSASSINATE:
			{
				if ( ent->client->pers.npc_id == GS_BOSS )
				{
					sql( "SELECT health,armor FROM bosses SEARCH id ?1;", "isIId", GS_BOSS, &health, &armor );
					if ( st.assassinate.dead_time == 0 )
						GS_ASSASSINATE.health = GS_ASSASSINATE.maxhealth = health;
				}

				//We may have a message from the spawnpoint to send to the player, so check, and send it to the clients team.
				if (GS_ASSASSINATE.boss_spawned && BotGetType(client) != BOT_BOSS ) {
					G_Say(ent, ent, SAY_TELL,va("%s guard boss", ent->client->pers.netname) );
				} else if (spawn && spawn->message) {
					G_Say(ent, ent, SAY_TELL,va("%s %s", ent->client->pers.netname, spawn->message) );
				}

			} break;
			case GT_MOA:
			{
				if ( sql( "SELECT client FROM players WHERE guard>0 RANDOM 1;", 0 ) ) {

					int client = sqlint(0);
					sqldone();

					sql( "UPDATE players SET guard=guard-1 SEARCH client ?1;", "ie", client );
					G_Say(ent, ent, SAY_TELL, va("kill %s", level.clients[client].pers.netname) );
				}
				
			}break;
			}

			switch ( BotGetType(client) )
			{
			case BOT_GUARD_EASY:
				health = 75;
				armor = 0;
				break;
			case BOT_GUARD_MEDIUM:
				health = 100;
				armor = 0;
				break;
			case BOT_GUARD_HARD:
				health = 100;
				armor = 75;
				break;
			}

			if ( sql( "SELECT COUNT(*) FROM players;", 0 ) ) {
				int client_count = sqlint(0);
				sqldone();
				if (client_count < 1) client_count = 1;
				health += (health * (client_count-1))/4;
				armor += (armor * (client_count-1))/4;

				g_entities[ client ].health =
				level.clients[ client ].ps.stats[STAT_HEALTH] =
				level.clients[ client ].ps.stats[STAT_MAX_HEALTH] =
				level.clients[ client ].pers.maxHealth = health;
				// no shields in hub
				if ( g_gametype.integer != GT_HUB )
				{
					level.clients[ client ].ps.shields = armor;
				} else {
					level.clients[ client ].ps.shields = 0;
				}
			}

		} break;

		//
		//	this event gets called when a player is attempting to interact with a bot
		//
		//	parameters :
		//		clientID	- id of the player that is wanting to talk
		//		botID		- id of the bot the player wants to talk to
		//		botname		- name of the bot player wants to talk to
		//
		case ST_BOT_INTERACT:
		{
			int				client	= va_arg( argptr, int );
			int				bot		= va_arg( argptr, int );
			int				npc_id	= va_arg( argptr, int );
			char *			message	= va_arg( argptr, char * );

			G_ST_DoDialogCommand( client, bot, npc_id, message );

		} break;

			// ( int gametype, int status )
		case ST_TEST_OBJECTIVES:
			{
				int gametype	= g_gametype.integer;

				//	don't check objectives until the turn has started
				if ( GS_INPLAY == 0 || GS_COUNTDOWN > 0 )
					break;

				switch ( gametype )
				{
				// check if mission objectives have been met
				case GT_HUB:
					{
						if ( level.time - st.objectives.time > 1000 )
						{
							if ( st.travelwarn && st.objectives.time < st.travelwarn && level.time >= st.travelwarn ) {
								sql( "UPDATE players SET readytime=?1 WHERE readytime=0;", "ie", level.time );
								trap_SendServerCommand( -1, "st_cp T_Time_To_Travel newcontact" );
								st.travelwarn = level.time + BG_ST_GetMissionInt("tradetime") *1000;
							}

							st.objectives.time = level.time;

							G_ST_UpdateNPCSprites();

							while( sql( "SELECT client FROM players WHERE objective=0;", 0 ) )
							{
								current_client = sqlint(0);

								if ( sql( "SELECT eval( missions.key[ 'win_condition' ]^value ), eval( missions.key[ 'fail_condition' ]^value );", 0 ) )
								{
									int won		= sqlint(0);
									int lost	= sqlint(1);
									sqldone();

									if ( won )
									{
										sql( "UPDATE players SET objective=?2 SEARCH client ?1;", "iie", current_client, OBJECTIVE_WON );
										if ( !st.objectives.scramble )
											trap_SendServerCommand( current_client, "st_won" );

									} else if ( lost )
									{
										sql( "UPDATE players SET objective=?2 SEARCH client ?1;", "iie", current_client, OBJECTIVE_LOST );
										if ( !st.objectives.scramble )
											trap_SendServerCommand( current_client, "st_lost" );
									}
								}
							}

							if ( st.objectives.scramble ) {

								if ( sql( "SELECT SUM(objective=?1)SUM(,objective=?2),COUNT(*) FROM players;", "ii", OBJECTIVE_WON, OBJECTIVE_LOST ) ) {

									int won		= sqlint(0);
									int lost	= sqlint(1);
									int	total	= sqlint(2);
									sqldone();

									//	if at least one person has won, and the game type is scramble then
									//	force the game to end
									if ( won > 0 || lost == total ) {

										//	submit final scores to all the clients and the master server
										G_ST_FinalScore();

										//	kill the game and send everyone to the post screen
										trap_SendConsoleCommand( EXEC_APPEND, "wait 30 ; killserver ;" );
									}
								}
							}
						}

					} break;
				case GT_ASSASSINATE:
					{
						if ( st.assassinate.dead_time > 0 )
						{
							GS_ASSASSINATE.dead_countdown = 10 - ( level.time - st.assassinate.dead_time )/1000;
			
							if ( GS_ASSASSINATE.dead_countdown == 0 ) {
								returntohub();
							}

						} else if ( st.assassinate.failed == qtrue ) {
							returntohub();
						} else if ( st.assassinate.boss_escape_time > 0 && (level.time - st.assassinate.boss_escape_time) > 60000) {
							sql( "UPDATE bosses SET planet_id=-1 WHERE id=?;", "ie", GS_BOSS );
							returntohub();
						} else if ( TeamCount(-1, TEAM_BLUE) == 0 && TeamCount(-1, TEAM_RED) > 0) {
							returntohub();
						}

						// handle abort mission
						//if ( status == OBJECTIVE_LOST && GS_ASSASSINATE.dead_countdown <= 0 ) {
						//	GS_ASSASSINATE.dead_countdown = 5;
						//}
					} break;
				case GT_MOA:
					{
						if (st.moa.dead_time > 0)
						{
							GS_ASSASSINATE.dead_countdown = 10 - ( level.time - st.moa.dead_time ) / 1000;
							if ( GS_ASSASSINATE.dead_countdown == 0 ) {
								G_ST_Moa_Win();
							}
						}
					} break;
				}
				

				return 0;

			} break;

		case ST_TIMER:
		{
			if ( players_connecting() )
				break;

			if ( GS_COUNTDOWN >= 0 && GS_COUNTDOWN <= 10 ) {
				GS_COUNTDOWN--;
			}

		} break;
		case ST_TOSS_STASH:
			{
				int			clientId	= va_arg( argptr, int );
				gentity_t*	ent			= g_entities + clientId;
				gentity_t*	stash;
				gitem_t*	item;
				int angle = 35;
				
				//players shouldn't drop stash/cash
				if ( clientId < MAX_PLAYERS )
					break;
				
				// moa soldiers dont drop stash, and never drop money in hubs
				if ( g_gametype.integer == GT_HUB ) break;

				if ( ent->client->pers.npc_id == GS_BOSS ) 
				{
					int i;
					for ( item = bg_itemlist + 1 ; item->classname ; item++) {
						if ( item->giType == IT_MONEYBAG ) {
							// create one stash bag per player.
							if ( sql( "SELECT COUNT(*) FROM players;", 0 ) ) {
								int player_count = sqlint(0);
								int amount = (st.assassinate.bounty / 3) / player_count;
								sqldone();

								for (i = 0; i < player_count; i++ ){
									stash = Drop_Item( ent, item, angle );
									stash->stash_qty = amount + Q_rrand( -(amount>>3), +(amount>>3) );	// +-12.5%
									stash->nextthink = level.time + 8000;
									// make this moneybag 3 times bigger
									stash->s.generic1 = 3;
									angle += 35;
								}
							}
							break;
						} else if ( item->giType == IT_STASH ) {
							for (i=0; i < st.assassinate.gang_count; i++) {
								if ( st.stash.count )
								{
									int	j = Q_rrand( 0, st.stash.count );
									stash = Drop_Item( ent, item, angle );
									stash->stash_commodity_id	= st.stash.list[ j ].commodity_id;
									stash->stash_qty			= Q_rrand( st.stash.list[ j ].low_count, st.stash.list[j].high_count );
									angle += 35;
									//Increase the total amount of stash spawned when bots drop stash
									st.stash.total_spawned++;
								}
							}
						}
					}
				} else {
					for ( item = bg_itemlist + 1 ; item->classname ; item++) {
						if ( item->giType == IT_MONEYBAG ) {
							stash = Drop_Item( ent, item, angle );
							if ( stash )
							{
								int amount;
								if (g_gametype.integer == GT_ASSASSINATE) {
									amount = (st.assassinate.bounty / 3);
									if ( st.assassinate.gang_count > 0 )
										amount /= st.assassinate.gang_count;
									amount = max(amount, BG_ST_GetMissionInt("moneybag_min"));
									amount += Q_rrand( -(amount>>3), +(amount>>3) );
								} else {
									amount = Q_rrand( st.moa.cash/400, st.moa.cash/100 );
								}
								stash->stash_qty = amount;
								stash->nextthink = level.time + 16000;
								//dont scale the moneybag
								stash->s.generic1 = 1;
							}
							break;
						}
					}
				}
			} break;
		case ST_CLIENT_DEATH:
			{
				int			clientId	= va_arg( argptr, int );
				int			meansOfDeath = va_arg( argptr, int );
				gentity_t*	ent			= g_entities + clientId;

				switch ( g_gametype.integer )
				{
				case GT_ASSASSINATE:
					{
						if ( ent->r.svFlags & SVF_BOT )
						{
							if ( BotGetType( clientId ) == BOT_BOSS )
							{
								if ( st.assassinate.dead_time == 0 )
								{
									//	mark this boss as captured
									sql( "UPDATE bosses SET captured=GS_TURN WHERE id=?;", "ie", GS_BOSS );

									trap_SendServerCommand( -1, va("st_cp T_Boss_Captured cp %d\n", ent->client->lasthurt_client) );
									st.assassinate.dead_time = level.time;
									//cant escape when you are dead!
									GS_ASSASSINATE.boss_escaping = qfalse;

									//	start the countdown
									GS_ASSASSINATE.dead_countdown = 10;
									trap_SendServerCommand( -1, "st_cp T_Go_Collect_Stash\n" );
									trap_SendServerCommand( -1, "st_announcer tip_captured 5000\n");
									trap_SendServerCommand( -1, "st_announcer 5 1000\n");
									trap_SendServerCommand( -1, "st_announcer 4 1000\n");
									trap_SendServerCommand( -1, "st_announcer 3 1000\n");
									trap_SendServerCommand( -1, "st_announcer 2 1000\n");
									trap_SendServerCommand( -1, "st_announcer 1 1000\n");

								}
							} else
							{
								if ( ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == clientId )
								{
									if ( st.assassinate.gang_count > 0 )
									{
										int reward = (st.assassinate.bounty / st.assassinate.gang_count) / 3;
										sql( "UPDATE players SET bonus=bonus+?2 SEARCH client ?1;", "iie", clientId, reward );
									}
								}
							}
							G_Voice( &g_entities[clientId], ent->enemy, SAY_TELL, "death", qfalse);
							return 0;
						} else {
							level.clients[ clientId ].pers.lives--;
							if ( level.clients[ clientId ].pers.lives <= 0 ) {
								if ( TeamCount( -1, TEAM_BLUE ) > 1 )
								{
									trap_SendServerCommand( clientId, "st_cp T_No_More_Chances\n" );
									trap_SendServerCommand( clientId, "st_announcer tip_nexttime 1500\n");
								} else if ( st.assassinate.dead_time == 0 ) { // make sure the boss has not been killed
									sql( "UPDATE bosses SET planet_id=-1 WHERE id=?;", "ie", GS_BOSS );

									//	launch the failed screen
									trap_SendServerCommand( -1, "st_combatfailed" );
									trap_SendServerCommand( -1, "st_cp T_Mission_Failed\n" );
									trap_SendServerCommand( -1, "st_announcer tip_escaped 1500" );
									st.assassinate.failed = qtrue;
								}
							}
						}
					} break;
				case GT_HOSTAGE:
					if ( ent->r.svFlags & SVF_BOT )
					{
						return 0;
					}
					break;
				case GT_MOA:
				case GT_PIRATE:
					{
						if ( clientId >= MAX_PLAYERS )
						{
							//	A Bot Has Died
							if (meansOfDeath != MOD_SUICIDE)
							{
								//	see if guard should respawn
								if ( !sql( "UPDATE boarding SET value=value-1 SEARCH key $ WHERE value>0;", "te", "spawn_queue" ) ) {
									ent->dont_respawn = 1;
								}

								//	see if all guards are defeated
								if ( !sql( "UPDATE boarding SET value=value-1 SEARCH key $ WHERE value>1;", "te", "guards_left" ) ) {
									GS_ASSASSINATE.dead_countdown = 10;
									st.moa.dead_time = level.time;
									//we get a +1 stuck on the number of guards left from the UI side
									sql( "UPDATE boarding SET value=0 SEARCH key $;", "te", "guards_left" );
									//G_ST_Moa_Win();
								}
							}
						} else
						{
							//	A Player Has Died
							int alive_players=0;
							while ( sql( "SELECT client FROM players;", 0 ) )
							{
								int client = sqlint(0);
								gclient_t player;
								if ( client == clientId) continue;
								player = level.clients[ client ];
								if ( player.ps.stats[STAT_HEALTH] > 0 && player.sess.sessionTeam != TEAM_SPECTATOR && !(player.ps.pm_flags & PMF_FOLLOW) ){
									alive_players++;
								}
							}
							trap_SendServerCommand( clientId, "st_announcer tip_nexttime 1500" );
							if ( alive_players == 0 )
							{
								G_ST_Moa_Lose();
							}
						}
					}
					break;
				}

			} break;

			// return 0 to allow this client to spawn, return 1 to stop respawning
		case ST_CLIENT_RESPAWN:
			{
				int clientId = va_arg( argptr, int );
				int	spawns = va_arg( argptr, int );
				gentity_t * ent = g_entities + clientId;
				
				// dont respawn if we are in an intermission
				if ( level.intermissionQueued ) {
					return 1;
				}
				
				switch ( g_gametype.integer )
				{
					case GT_ASSASSINATE:
					case GT_HOSTAGE:
						if ( ent->r.svFlags & SVF_BOT )
						{
							//spawns = 1 on first spawn
							if (spawns == 1) return 0;
							if ( ( level.teamScores[TEAM_BLUE] > st.assassinate.gang_count/2 || level.time - level.startTime > (1000 * 60 * 1) ) && (GS_ASSASSINATE.boss_spawned == qfalse) )
							{
								int i, guarding_bots=0;
								if ( sql( "SELECT spawn, name, model FROM npcs SEARCH id $", "i", GS_BOSS) ) {

									trap_SendServerCommand( -1, "st_cp T_Boss_Arrived newcontact\n" );
									G_AddBot("boss", sqltext(2), sqltext(1), GS_BOSS, g_spSkill.integer +1.0f, "red", 0, NULL);
									GS_ASSASSINATE.boss_spawned = qtrue;
									for (i=MAX_PLAYERS; i< MAX_CLIENTS; i++)
									{
										if (BotGetType(i) != BOT_BOSS && guarding_bots < 6) {
											gentity_t *bot;
											bot = g_entities + i;
											if (bot->client->ps.stats[STAT_HEALTH] > 0 ) {
												guarding_bots++;
												G_Say( bot, bot, SAY_TELL, va("%s cover %s", bot->client->pers.netname, sqltext(1) ) );
											}
										}
									}
									sqldone();
								}
							}

							switch ( BotGetType( clientId ) )
							{
							case BOT_GUARD_EASY:
								if ( st.assassinate.gang_left_easy <= 0 )
								{
									return 1;
								} else {
									st.assassinate.gang_left_easy--;
									return 0;
								}
							case BOT_GUARD_MEDIUM:
								if ( st.assassinate.gang_left_medium <= 0 )
								{
									return 1;
								} else {
									st.assassinate.gang_left_medium--;
									return 0;
								}
							case BOT_GUARD_HARD:
								if ( st.assassinate.gang_left_hard <= 0 )
								{
									return 1;
								} else {
									st.assassinate.gang_left_hard--;
									return 0;
								}
							case BOT_BOSS:
								return 1;
							}
						} else {
							if (level.clients[ clientId ].pers.lives == 2 && st.assassinate.dead_time == 0 ) {
								trap_SendServerCommand( clientId, "st_announcer tip_2chances 1500\n");
							} else if (level.clients[ clientId ].pers.lives == 1 && st.assassinate.dead_time == 0) {
								trap_SendServerCommand( clientId, "st_announcer tip_lastchance 1500\n");
							}
							if ( level.clients[ clientId ].pers.lives <= 0 ) {
								SetTeam( ent, "follow1" );
								return 1;
							} else {
								return 0;
							}
						}
						break;

					case GT_MOA:
					{
						if ( ent->r.svFlags & SVF_BOT )
						{
							if (ent->dont_respawn == qtrue ) {
								return 1;
							}
							return 0; //respawn
						} else {
							// always let them spawn the first time through
							if (spawns == 1) return 0;
							SetTeam( ent, "follow1" );
							return 1;
						}
					}
					case GT_PIRATE:
						return 1;
					default:
						return 0;
				}
				return 0;
			} break;
		case ST_SCORE:
			{
				gentity_t *	ent		= va_arg( argptr, gentity_t * );
				int score 			= va_arg( argptr, int );

				//	don't count suicides
				if ( score > 0 ) {
					sql( "UPDATE players SET score=score+?2 SEARCH client ?1;", "iie", ent->s.clientNum, score );
				}

			} break;
		case ST_SPAWN_ITEM:
			{
				gentity_t *	ent		= va_arg( argptr, gentity_t * );
				gitem_t *	item	= va_arg( argptr, gitem_t * );
				int i;

				//	only allow stash in hubs
				if ( item->giType != IT_STASH && item->giType != IT_MONEYBAG ) {
					if ( g_gametype.integer == GT_HUB )
						return 0;
					else
						return 1;
				}

				if ( st.stash.count == 0 ) {
					return 0;
				}

				i = Q_rrand( 0, st.stash.count );
				ent->stash_commodity_id = st.stash.list[ i ].commodity_id;
				ent->stash_qty			= Q_rrand( st.stash.list[ i ].low_count, st.stash.list[i].high_count );
				ent->stash_id			= st.stash.stash_spawns++;


				//	check if this item has spawned before...
				if ( g_gametype.integer == GT_HUB && GS_INPLAY == 1 ) {
					if ( sql( "SELECT used FROM stash SEARCH id ?1;", "i", ent->stash_id ) ) {

						int picked_up = sqlint(0);
						sqldone();

						st.stash.total_spawned++;

						return !picked_up;
					}
				}

				//	roll to see if stash will spawn
				if ( !st.stash.count || Q_rrand( 0, 100 ) < 20 ) {
					return 0;
				}

					
				st.stash.total_spawned++;
				if ( g_gametype.integer == GT_HUB ) {
					sql( "INSERT INTO stash(id,used) VALUES(?1,0);", "ie", ent->stash_id );  
				}

				return 1;	// allow this item to spawn

			}break;
			//can be stash, or moneybag
		case ST_PICKUP_STASH:
			{
				gentity_t *	ent		= va_arg( argptr, gentity_t * );
				int 		client	= va_arg( argptr, int );
				// bots can't pickup stash
				if (client >= MAX_PLAYERS ) return 0;
				if ( !ent->item ) return 0;
				if ( ent->item->giType == IT_STASH )
				{
					if ( sql( "UPDATE players SET stash=stash+1 SEARCH client ?1 WHERE cargomax-cargo>=?2", "iie", client, ent->stash_qty ) > 0 ) {

						if ( g_gametype.integer == GT_HUB ) {
							sql( "UPDATE stash SET used=1 SEARCH id ?1;", "ie", ent->stash_id );
						}

						G_AddEvent( g_entities + client, EV_ITEM_STASH, ((ent->stash_qty) << 8) + ent->stash_commodity_id );
						insert_purchase( client, ent->stash_commodity_id, ent->stash_qty, 0 );
						return 1;
					}
	
					if (level.clients[ client ].stashTouchTime + 3000 < level.time )
					{
						G_AddEvent( g_entities + client, EV_ITEM_STASH, 0 );
						level.clients[ client ].stashTouchTime = level.time;
					}
				} else if ( ent->item->giType == IT_MONEYBAG ) {
					trap_SendServerCommand( client, va("st_cash %d", ent->stash_qty ) ); 
					sql( "UPDATE players SET cash=cash+?2,networth=cash+debt+inventory SEARCH client ?1;", "iie", client, ent->stash_qty );
					G_ST_UpdateRanks();
					return 1;
				}
				return 0;
			
			} break;
		//
		//	process commands sent by the client.  any st_* commands are sent here
		//
		case ST_CLIENTCOMMAND:
			{
				int				clientId	= va_arg( argptr, int );
				const char *	cmd			= va_arg( argptr, const char * );

				G_ST_ClientCommand( clientId, cmd );

			} break;

		case ST_CONSOLECOMMAND:
			{
				return G_ST_ConsoleCommand( va_arg( argptr, const char * ) );

			} break;

		case ST_CLIENT_CONNECT:
			{
				int client = va_arg( argptr, int );

				if ( client >= 0 && client < MAX_PLAYERS )
					G_ST_UpdateRanks();

				if ( g_gametype.integer == GT_ASSASSINATE)
				{
					//assign the player their lives.
					level.clients[ client ].pers.lives = BG_ST_GetMissionInt( "lives" );
				}
			} break;

		case ST_CLIENT_DISCONNECT:
			{
				int client = va_arg( argptr, int );

				if ( client >= 0 && client <MAX_PLAYERS ) {
					sql( "DELETE FROM players WHERE client=?;", "ie", client );
					G_ST_UpdateRanks();
				}

			} break;

		case ST_DEAL_DAMAGE:
			{
				gentity_t	* target	= g_entities + va_arg( argptr, int );
				gentity_t	* attacker	= g_entities + va_arg( argptr, int );
				int	amount = va_arg( argptr, int );

				if ( target->client->pers.npc_id == GS_BOSS )
				{
					//Run forest run
					if ( GS_ASSASSINATE.boss_escaping == qfalse && target->client->ps.shields <= 0 )
					{
						GS_ASSASSINATE.boss_escaping = qtrue;
						st.assassinate.boss_escape_time = level.time;
						trap_SendServerCommand(-1, "st_cp T_Boss_Escaping\n");
						G_Say( target, target, SAY_TELL, va("%s camp exit", target->client->pers.netname) );
					}

					// only update the bosses health when he is still alive
					if ( GS_ASSASSINATE.dead_countdown == 0 && st.assassinate.dead_time == 0 )
						GS_ASSASSINATE.health = target->health;

					sql( "UPDATE players SET bossdamage=bossdamage+?2 SEARCH client ?1;", "iie", attacker->s.clientNum, amount );
				}
			}
			break;
#if 0	// was actually broken and never used
		case ST_PRINT_HINT:
			{
				int clientId	= va_arg( argptr, int );
				char *message	= va_arg( argptr, char * );

				if ( clientId < MAX_PLAYERS ) {
					if ( sql( "UPDATE players SET hint=?2 SEARCH client ?1 WHERE hint+4000<?2;", "iie", clientId, level.time ) ) {
						trap_SendServerCommand( clientId, va("st_hint \"%s\"", message ));
					}
				}

			} break;
#endif
		case ST_VOTE:
			{
				int		clientId	= va_arg( argptr, int );
				char *	cmd			= va_arg( argptr, char * );
				const char *	arg	= va_arg( argptr, char * );

				switch( SWITCHSTRING( cmd ) )
				{
					case CS('t','r','a','v'):	//	travel
						{
							if ( sql( "SELECT id FROM planets WHERE name like $;", "t", COM_Parse( &arg ) ) )
							{
								int	planet_id = sqlint(0);
								sqldone();

								switch( G_ST_CanTravel( clientId, planet_id ) )
								{
								case 1:	trap_SendServerCommand( clientId, "ui_vote 2 travel_notready" ); return 1;
								case 2:	trap_SendServerCommand( clientId, "ui_vote 2 travel_notime" ); return 1;
								case 3: trap_SendServerCommand( clientId, "ui_vote 2 travel_invalid"); return 1;
								}

								G_Vote_SetDescription	( clientId, "travel", va("%d",planet_id), "" );
								G_Vote_SetPassedScript	( "st_%s 0 %d ;", cmd, planet_id );
								G_Vote_SetFailedScript	( "" );

								sql( "UPDATE OR INSERT missions SET key=$1,value=?2:plain SEARCH key $1;", "tie", "voter", clientId );
								sql( "UPDATE OR INSERT missions SET key=$1,value=?2:plain SEARCH key $1;", "tie", "voteplanet", planet_id );

								//	the client that called the vote, automatically votes yes
								sql( "UPDATE players SET vote=?2 SEARCH client ?1;", "iie", clientId, GS_VOTEYES );

							} else
							{
								trap_SendServerCommand( clientId, "ui_vote 2 travel_invalid" ); return 1;
							}

						} break;

					case CS('a','s','s','a'):	//	assassinate <boss name> <map name>
						{
							if ( sql( "SELECT id SEARCH name ? FROM npcs WHERE type='Boss';", "t", COM_Parse( &arg ) ) )
							{
								int boss_id = sqlint(0);
								sqldone();

								G_Vote_SetPassedScript	( "st_assa %d %s", boss_id, COM_Parse( &arg ) );
								G_Vote_SetFailedScript	( "" );

								sql( "UPDATE OR INSERT assassinate SET key=$1,value=?2 SEARCH key $1;", "tie", "voter", clientId );
								sql( "UPDATE OR INSERT assassinate SET key=$1,value=?2 SEARCH key $1;", "tie", "boss", boss_id );
							}

							//	the client that called the vote, automatically votes yes
							sql( "UPDATE players SET vote=?2 SEARCH client ?1;", "iie", clientId, GS_VOTEYES );

						} break;

					case CS('r','e','t','i'):	//	retire

						G_Vote_SetDescription	( clientId, cmd, "", "" );
						G_Vote_SetPassedScript	( "st_%s", cmd );
						G_Vote_SetFailedScript	( "" );

						//	the client that called the vote, automatically votes yes
						sql( "UPDATE players SET vote=?2 SEARCH client ?1;", "iie", clientId, GS_VOTEYES );

						break;

					default:
						return 0;
				}

				return 1;

			} break;
	}

	return 1;
}

