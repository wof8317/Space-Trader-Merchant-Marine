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

//
//	ui_spacetrader.c -	this is the core rules engine.  all access to trading and rules info
//						is done through here
//

#include "../game/bg_spacetrader.h"


globalState_t			gsi;	// global state mirrored from server


extern menuStackDef_t	ui_menuStack;
extern menuStackDef_t	cg_menuStack;

extern vmCvar_t  ui_initialized;
extern vmCvar_t  ui_spacetrader;

static void st_shop_select_buy( void );
static void st_shop_select_sell( void );
static void st_shop_unselect_buy( void );
static void st_shop_unselect_sell( void );


#define MAX_PLANETS		16
#define MAX_CHALLENGES	256

//
//	locally cached info
//
#define MAX_ANSWERS 4

typedef struct
{
	int		commodity_id;
	void	*shape;
	int		low_time;	// x value when y is lowest
	int		low_price;	// the lowest y value
	int		high_time;	// x value when y is highest
	int		high_price;	// the highest y value

} graph_t;

#define MAX_GRAPHS 256
#define MAX_GRAPH_POINTS 256

#define MAX_NEWS_BLURBS 16
#define MAX_SHOP_LIST	64		// max amount of commodities in one buy/sell list

#define MAX_TALK_LINES	32

typedef struct
{
	int		npc;		// the npc id of the npc that this local client is interacting with
	int		npcIsGood;
	int		seed;		// game seed used for generating random numbers

	int		selectedprofile;
	int		gametype;

	qhandle_t	cp[ 8 ];

	int		clientselected;	// the client that the server has selected in the BeginTurn screen

	int		contact_index;

	int		MENU;		// menu state
	int		menus_init;

	struct
	{
		qhandle_t	graphShader;
		qhandle_t	lineshadow;
		qhandle_t	youarehere;
		qhandle_t	orbitShader;
		qhandle_t	decoration_accuracy;
		qhandle_t	decoration_boss;
		qhandle_t	decoration_money;
		qhandle_t	decoration_stash;
		qhandle_t	decoration_time;

		qhandle_t	arrow_past_head;
		qhandle_t	arrow_past_tail;
		qhandle_t	calendar;
		qhandle_t	arrow_left_head;
		qhandle_t	arrow_left_tail;

		qhandle_t	playerBust;

		effectDef_t *	ready;
		effectDef_t *	notready;
	} assets;

	struct
	{
		solarsystemInfo_t		ss;
		const planetInfo_t *	selected;		// the planet under the mouse cursor
		const planetInfo_t *	voted;			// the planet that this player wants to travel to
		const planetInfo_t *	current;

		int						selectLockTime;	// selection can't change before this timeout expires

		float	totalTime;

		enum
		{
			STZ_DEFAULT,
			STZ_OUT,
			STZ_IN,
		}						zoomState;

		solarsystemEye_t		eye[2];
		int						lerpTime[2];

		struct
		{
			int					stepTime;
			int					action;
		} showcase;

		int						readyTime;		// last time this user pressed the ready button

		qhandle_t				labels[ 16 ];
		effectDef_t *			label;

	} travel;

	struct
	{
		int		fromExplore;	// dialog with this bot was from explore mode

	} dialog;

	struct
	{
		lineInfo_t	lines[256];
		uint		lineCount;

		char		text[8 * 1024];
		uint		textLen;
	} chat;

	struct
	{
		int			count;
		struct
		{
			const char * text;
			qhandle_t icon;
		} blurbs[ MAX_NEWS_BLURBS ];

		int			tickerIndex;	// current news blurb on the ticker
		int			tickerTime;		// time the current blurb started showing

	} news;

	graph_t		graphs[MAX_GRAPHS];


	struct
	{
		int	bounty;
		char bossname[ MAX_NAME_LENGTH ];
	} assassinate;

	struct
	{
		int				client;		// the client that is under the crosshair
		qhandle_t		effect;

		effectDef_t *	ally;
		effectDef_t *	enemy;
	} crosshair;

	playerInfo_t modelinfo;

	sfxHandle_t		buySound;
	sfxHandle_t		cantbuySound;
	sfxHandle_t		sellSound;

} spacetraderInfo_t;

static spacetraderInfo_t st;

static void UI_ST_ScanForMissions( void );
static void UI_ST_SelectPlanet( const planetInfo_t * p, itemDef_t * item );

extern void _UI_DrawRect( float x, float y, float width, float height, float size, const float *color );


static int UI_ST_GetMissionInt(const char *key)
{
	int value;
	if (sql( "SELECT value FROM missions WHERE key LIKE $", "tsId", key, &value) ) {
		return value;
	} else {
		Com_Error(ERR_FATAL, "Key: %s not found in missions table", key );
		return 0;
	}
}

/*
static void UI_ST_MsgBox( const char * title, const char * text )
{
	trap_Cvar_Set( "ui_msgbox_title", title );
	trap_Cvar_Set( "ui_msgbox_text", text );
	Menus_ActivateByName( "msgbox" );
}
*/

static void cache_solarsystem( solarsystemInfo_t * ss )
{
	int i;

	ss->planetCount = trap_SQL_Select(	"SELECT "
											"id, "
											"planets_text.id[ id ]^name, "
											"IF (event_end>GS_TIME) THEN events_text.id[ event_id ]^name ELSE '', "
											"dfs, "
											"op, "
											"size, "
											"sa, "
											"travel_time, "
											"event_id, "
											"0,"
											"0,"
											"model ( 'models/' || name || '.x42' ), "
											"shader( 'ui/starmap/' || name ), "
											"shader( 'ui/starmap/' || name || 'atmosphere' ), "
											"shader( 'ui/planets/' || name ), "
											"0,"
											"events_icons.id[ event_id ].icon, "
											"is_visible, "
											"can_travel_to==0 AND id!=GS_PLANET, "
											"0, "
											"0, "
											"0, "
											"0 "
											"FROM planets;", (char*)ss->planets, sizeof(ss->planets) );

	st.travel.current = 0;

	//	default to planet model
	for ( i=0; i<ss->planetCount; i++ ) {
		if ( !ss->planets[ i ].model ) {
			ss->planets[ i ].model = trap_R_RegisterModel( ASSET_PLANETMODEL );
		}
	}

	if( sql( "SELECT # FROM planets SEARCH id ?;", "i", GS_PLANET ) )
	{
		st.travel.current = ss->planets + sqlint(0);
		sqldone();
	}
}

qhandle_t iconForModel( const char * path )
{
	char model[ MAX_QPATH ];
	char * skin;

	Q_strncpyz( model, path, sizeof(model) );
	
	skin = Q_strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	} else {
		skin = "default";
	}

	return trap_R_RegisterShaderNoMip( va("models/players/%s/icon_%s", model, skin ) );
}

static void cache_news( void )
{
	st.news.count = 0;

	if (sql( "SELECT value FROM missions_text WHERE key LIKE 'description'", 0 ))
	{
		st.news.blurbs[ st.news.count++ ].text = String_Alloc( sqltext(0) );
		sqldone();
	}

	/*
	for ( i=0; i<GS_PLANETCOUNT; i++ )
	{
		const char * blurb;

		if ( GS_PLANETS(i).event_end <= GS_TIME+GS_PLANETS(i).time )
			continue;

		if ( sql( "SELECT description,'ui/events/' || name FROM events_text WHERE id=?;", "i", GS_PLANETS(i).event_id ) )
		{
			blurb = String_Alloc( va("%s " S_COLOR_GREEN "%d" S_COLOR_WHITE " %s.\n", sqltext(0), GS_PLANETS(i).event_end-GS_TIME, st.travel.units ) );

			st.travel.ss.planets[ i ].disaster		= blurb;
			st.travel.ss.planets[ i ].disaster_icon = trap_R_RegisterShaderNoMip( sqltext(1) );

			if ( st.news.count < MAX_NEWS_BLURBS )
			{
				st.news.blurbs[ st.news.count ].icon = st.travel.ss.planets[ i ].disaster_icon;
				st.news.blurbs[ st.news.count++ ].text = blurb;
			}
			sqldone();
		}
	}
	*/
}

static void cache_icons()
{
	sql( "DELETE FROM icons;", "e" );
	sql( "INSERT INTO icons(client,npc,icon) SELECT client,id,portrait(model) FROM clients;", "e" );
	sql( "INSERT INTO icons(client,npc,icon) SELECT -1,id,portrait(model) FROM npcs WHERE clients.id[ id ].client=-1;", "e" );


	st.assets.playerBust = trap_R_RegisterShaderNoMip( va( "models/players/%s/bust_default", UI_Cvar_VariableString( "model" ) ) );
}

static void draw_pricegraph( itemDef_t * item, int commodity_id )
{
	//	icon water mark
	if ( sql( "SELECT icon,chart FROM commodities_text SEARCH id ?;", "i", commodity_id ) ) {

		float w = min( item->window.rect.w, item->window.rect.h ) * 0.75f;

		uiInfo.uiDC.setColor( item->window.foreColor );
		uiInfo.uiDC.drawHandlePic(	item->window.rect.x + (item->window.rect.w - w ) * 0.65f,
									item->window.rect.y + (item->window.rect.h - w ) * 0.5f,
									w,w,
									sqlint(0) );

		UI_Chart_Paint( item, sqlint(1) );

		sqldone();
	}
}

static void insert_prices( graphGen_t *gg, int x1, int x2, float price1, float price2, float range )
{
	int		x = x1 + ((x2-x1)>>1);
	float	y = max( 0.1f, price1 + ((price2-price1)*0.5f) + ((Q_random()-0.5f)*2.0f * range) );

	gg->pts[ x ][ 1 ] = y;

	gg->low		= min( gg->low, (int)y );
	gg->high	= max( gg->high, (int)y );

	if ( x1 != x )	insert_prices( gg, x1, x-1, price1, y, range * 0.64f );
	if ( x2 != x )	insert_prices( gg, x+1, x2, y, price2, range * 0.64f );
}

//
//	create graphs for all the commodities
//
static void cache_pricegraphs( int time1, int time2, int n )
{
	graphGen_t	gg;
	qhandle_t	chart;
	vec4_t		colorLow	= {0.9f, 0.7f, 0, 1};
	vec4_t		colorHigh	= {1, 1, 0, 1};

	while( sql( "SELECT id,low,high FROM commodities;", 0 ) ) {

		int commodity_id	= sqlint(0);
		int low				= sqlint(1);
		int high			= sqlint(2);
		int i,p1,p2,x1,x2;

		UI_Graph_Init( &gg );

		//	plot prices
		for ( i=x1=x2=p1=p2=0; sql( "SELECT time,price FROM prices SEARCH time ?2,?3 WHERE commodity_id=?1;", "iii", commodity_id, time1, time2 ); i++ ) {

			int time	= sqlint(0);

			x2 = ((time-time1)*(n-1)) / (time2-time1);
			p2 = sqlint(1);

			low = min( low, p2 );
			high = max( high, p2 );

			if ( i > 0 ) {
				gg.pts[ x1 ][ 1 ] =(float)p1;
				gg.pts[ x2 ][ 1 ] =(float)p2;
				insert_prices( &gg, x1+1, x2-1, (float)p1, (float)p2, fabsf( (float)(high-low) ) * 0.38f );
				x1 = x2+1;
			}

			p1 = p2;
			
		}

		//	uv coordinates and colors
		for ( i=0; i<n; i++ )
		{
			gg.pts[ i ][0] = (float)i;

			gg.uvs[ i ][0] = (float)i/(float)n;
			gg.uvs[ i ][1] = 0.0f;

			Vec4_Lrp( gg.colors[ i ], colorLow, colorHigh, (gg.pts[ i ][1]-gg.low) / (float)(gg.high-gg.low) );
		}

		gg.n = n;

		chart = UI_Chart_Create( commodity_id, 0, n, gg.low * 0.9f, gg.high * 1.1f );

		UI_Chart_AddGraph( chart, &gg, SHAPEGEN_LINE, st.assets.graphShader );

		for ( i=0; i<=4; i++ ) {
			float v = (((gg.high-gg.low)*i)/4.0f) + gg.low;
			UI_Chart_AddLabel( chart, 0, v, fn( (int)v, FN_CURRENCY ) );
		}

		sql( "UPDATE commodities_text SET chart=?2 SEARCH id ?1;", "iie", commodity_id, chart );
	}
}

static void cache_buylist()
{
	while( sql( "SELECT commodity_id, commodities_effects.commodity_id[ commodity_id ].buy_effect, todaysprices.commodity_id[ commodity_id ].range FROM commodities_npcs WHERE npc_id=?;", "i", st.npc ) )
	{
		int				id		= sqlint(0);
		qhandle_t		h		= sqlint(1);
		int				range	= sqlint(2);
		effectDef_t *	effect	= 0;

		UI_Effect_SetFlag( h, EF_DELETEME );

		if ( range < 2 )		effect = UI_Effect_Find( "buy_rockbottom" );
		else if ( range < 25 )	effect = UI_Effect_Find( "buy_cheap" );

		if ( effect ) {

			h = UI_Effect_SpawnText( 0, effect, 0, 0 );
			UI_Effect_SetFlag( h, EF_ONEFRAME_RECT );

			sql( "UPDATE OR INSERT commodities_effects SET buy_effect=?2,commodity_id=?1 SEARCH commodity_id ?1;", "iie", id, h );
		}
	}

	sql( "DELETE FROM buylist;", "e" );
	sql( "INSERT INTO buylist(id,avg_price,price,change,range) "
				"SELECT "
					"commodity_id, "
					"(commodities.id[ commodity_id ].high+commodities.id[ commodity_id ].low)/2, "
					"todaysprices.commodity_id[ commodity_id ].price, "
					"todaysprices.commodity_id[ commodity_id ].change, "
					"todaysprices.commodity_id[ commodity_id ].range "
				"FROM commodities_npcs SEARCH npc_id conversations.id[ 0 ].npc;", "e" );
}

static void cache_selllist()
{
	while( sql( "SELECT commodity_id, commodities_effects.commodity_id[ commodity_id ].sell_effect, todaysprices.commodity_id[ commodity_id ].range FROM commodities_npcs WHERE npc_id=?;", "i", st.npc ) )
	{
		int				id		= sqlint(0);
		qhandle_t		h		= sqlint(1);
		int				range	= sqlint(2);
		effectDef_t *	effect	= 0;

		UI_Effect_SetFlag( h, EF_DELETEME );

		if ( range > 95 )		effect = UI_Effect_Find( "sell_high" );
		else if ( range > 75 )	effect = UI_Effect_Find( "sell_ok" );

		if ( effect ) {

			h = UI_Effect_SpawnText( 0, effect, 0, 0 );
			UI_Effect_SetFlag( h, EF_ONEFRAME_RECT );

			sql( "UPDATE OR INSERT commodities_effects SET sell_effect=?2,commodity_id=?1 SEARCH commodity_id ?1;", "iie", id, h );
		}
	}

	st_shop_select_sell();
}
static void UI_ST_kickLaggingPlayers() {

	while ( sql( "SELECT client FROM clients WHERE lag=1;", 0 ) ) {

		trap_Cmd_ExecuteText( EXEC_APPEND, va("kick %d; wait;", sqlint(0) ) );
	}
}

static void UI_ST_IntegerPrint( int number, effectDef_t *fxd )
{
	rectDef_t r;
	r.x = 0.0F;
	r.y = 0.0F;
	r.w = 640.0F;
	r.h = 460.0F;
	UI_Effect_SpawnText( &r, fxd, va("%d", number), 0 );
}

static void UI_ST_CenterPrint( const char *text, effectDef_t *fxd )
{
	uint i;
	for( i = 0; i < lengthof( st.cp ); i++ )
	{
		uint c;

		if( i & 1 )
		{
			c = lengthof( st.cp ) / 2 + i / 2;
		}
		else
		{
			c = lengthof( st.cp ) / 2 - i / 2 - 1;
		}

		if( !UI_Effect_IsAlive( st.cp[c] ) )
		{
			rectDef_t r;
			
			r.x = -uiInfo.uiDC.glconfig.xbias;
			r.y = (480.0F - 32.0F * lengthof( st.cp )) * 0.5F + c * 32.0F;
			r.w = 640.0F + uiInfo.uiDC.glconfig.xbias*2.0f;
			r.h = 32.0F;

			st.cp[c] = UI_Effect_SpawnText( &r, fxd, text, 0 );
			break;
		}
	}
}

#if 0	// broken, was never actually used
// convert from a message sent from the server(from the map), to a string
static const char * UI_Hint( const char *hint )
{
	switch( SWITCHSTRING((const char*)hint) )
	{
		//	crouch
		case CS('c','r','o','u'):
		{
			int key;
			char keyname[32];
			
			key = trap_Key_GetKey("+movedown");
			trap_Key_KeynumToStringBuf( key, keyname, 32 );
			return va("Press '%s' to crouch", keyname);
		} break;
		//	jump
		case CS('j','u','m','p'):
		{
			int key;
			char keyname[32];
			
			key = trap_Key_GetKey("+moveup");
			trap_Key_KeynumToStringBuf( key, keyname, 32 );
			return va("Press '%s' to jump", keyname);
		} break;
		//	move
		case CS('m','o','v','e'):
		{
			return "Press the WASD keys to move";
		} break;
		//	mouse
		case CS('m','o','u','s'):
		{
			return "Use the mouse to look around";
		} break;
		//	chat
		case CS('c','h','a','t'):
		{
			int key;
			char keyname[32];
			
			key = trap_Key_GetKey("+attack");
			trap_Key_KeynumToStringBuf( key, keyname, 32 );
			return va("Press '%s' when facing someone to interact with them", keyname);
		} break;
		//	shoot
		case CS('s','h','o','o'):
		{
			int key;
			char keyname[32];
			
			key = trap_Key_GetKey("+attack");
			trap_Key_KeynumToStringBuf( key, keyname, 32 );
			return va("Press '%s' to attack", keyname);
		} break;
	}

	return hint;
}
#endif

static void UI_ST_ChatPrint( int client, const char *msg )
{
	const char *str;

	if( sql( "SELECT n FROM clients SEARCH client ?", "i", client ) )
	{
		str = va( "%s: %s\n", sqltext( 0 ), msg );

		trap_Con_Print( 1, str );

		sqldone();
	}
}


static void resize_contacts( menuDef_t * menu, int n ) {

	itemDef_t *r, *p, *c;
	itemDef_t * retire;
	float b;

	if ( !menu )
		return;

	r = Menu_FindItemByName( menu, "ready" );
	p = Menu_FindItemByName( menu, "otherplayers" );
	c = Menu_FindItemByName( menu, "contactlist" );
	retire = Menu_FindItemByName( menu, "retire" );

	if ( sql( "SELECT 1 FROM players SEARCH client %client WHERE objective=1;", 0 ) ) {
		sqldone();
		//	make room for the retire button
		b = retire->window.rect.y - 4.0f;
	} else
		b = retire->window.rect.y + retire->window.rect.h;

	if ( n > 1 ) {

		//	multiplayer, size player list to fit number of players. contact list gets the rest
		listBoxDef_t * lp = (listBoxDef_t*)p->typeData;
		p->window.rect.h = lp->titlebar + lp->elementHeight*(n+0.25f);

		b -= p->window.rect.h;
		p->window.rect.y = b;

		//	adjust ready button
		b -= (r->window.rect.h + 8.0f);
		r->window.rect.y = b;
		b -= 8.0f;

		p->window.rectClient.y = p->window.rect.y - menu->window.rect.y;
	}


	//	adjust height of contact list
	c->window.rect.h = b-c->window.rect.y;

	//	
	c->window.rectClient.y = c->window.rect.y - menu->window.rect.y;
	c->window.rectClient.h = c->window.rect.h;

}


#if 0
static void readyUpEffect() {

	int i;
	for ( i=0; i<GS_PLAYERCOUNT; i++ ) {

		int isReady = GS_READY&(1<<i);
		effectDef_t * def = (isReady)?st.assets.ready:0;
		qhandle_t	h = 0;

		sql( "SELECT ready_effect FROM clients_effects SEARCH client ?;", "isId", i,&h );


		//	if the current effect is not right then...
		if ( def != UI_Effect_GetEffect( h ) ) {
			//	kill old effect
			UI_Effect_SetFlag( h, EF_NOFOCUS );

			//	spawn new effect
			if ( def ) {
				h = UI_Effect_SpawnText( 0, def, 0, 0 );
			}
		}

		sql( "UPDATE OR INSERT clients_effects SET ready_icon=?2, ready_effect=?3, client=?1 SEARCH client ?1;",
				"iiie",
				i,
				isReady?st.assets.ready->shader:st.assets.notready->shader,
				h
			);

	}
}
#endif



//	a command has been sent from the server, try to process it
static int consolecommand( const char * cmd )
{
	switch( SWITCHSTRING((const char*)cmd) )
	{
	// st_newcontact < npc_id >
	case CS( 'n', 'e', 'w', 'c' ):			
		{
			effectDef_t *fxd = UI_Effect_Find( "newcontact" );

			if( !fxd )
				fxd = UI_Effect_Find( "cp" );

			if ( sql( "SELECT T_New_Contact;", 0 ) ) {
				UI_ST_CenterPrint( sqltext(0), fxd );
				sqldone();
			}
		}
		break;

		//	st_notime
		case CS('n','o','t','i'):
			{
				Menus_ActivateByName( "retireconfirm" );
			} break;
		//	st_trade <npc_id>
		case CS('t','r','a','d'):
			{
				// the server is telling the client to open a dialog box to begin trading with an npc
				menuDef_t * m = Menus_FindByName( "travel_menu" );
				if ( m )
				{
					int bot;
					//	active the travel menu
					Menus_Close( m );
					Menus_Activate( m );
					Menus_ActivateByName( "merchant_page" );
					Menus_ActivateByName( "shop_item_details" );

					//	remember who the client is trading with
					st.npc = trap_ArgvI( 1 );
					bot = trap_ArgvI( 2 );
					sql( "SELECT good FROM npcs WHERE id=?;", "isId", st.npc, &st.npcIsGood );

					//	cache this npc's goods
					cache_buylist();
					cache_selllist();

					//	initialize buying state
					st_shop_unselect_buy();
					st_shop_unselect_sell();

					trap_Cmd_ExecuteText( EXEC_NOW, va("st_lookat %d ;", bot) );
				}
			} break;

		//	st_awards
		//	the trading round has ended and the server is asking the client to display the awards
		//	for that round
		//
		case CS('a','w','a','r'):
			{
				switch ( st.gametype ) {
					case GT_HUB:
						Menus_FadeAllOutAndFadeIn( "trading_awards" );
						break;
					case GT_MOA:
					case GT_ASSASSINATE:
						Menus_FadeAllOutAndFadeIn( "assassinate_awards" );
						break;
				}
			} break;

		//	st_planet - the server's response from selecting a planet
		case CS('p','l','a','n'):
			{
				int resp;
				menuDef_t *m;

				m = Menus_FindByName( "travel_page" );

				if( !m || !(m->window.flags & WINDOW_VISIBLE) )
					break;

				resp = trap_ArgvI( 1 );

				if( resp >= 0 )
					Menus_ActivateByName( "travel_details" );

				switch( resp )
				{
				case 0:	Menus_ActivateByName( "travelconfirm" ); break;
				case 1:	Menus_ActivateByName( "travelnotallready" ); break;
				case 2:	Menus_ActivateByName( "traveltoofar" ); break;
				case 3: Menus_ActivateByName( "travelalreadythere" ); break;
				}
			} break;
		//	st_dialog <close/open>"
		case CS('d','i','a','l'):
			{
				menuDef_t * travel_menu = Menus_FindByName( "travel_menu" );
				menuDef_t * dialog_menu = Menus_FindByName( "dialog_menu" );

				switch( SWITCHSTRING( UI_Argv(1) ) )
				{
					case CS('c','l','o','s'):
						{
							Menus_Close( dialog_menu );

							if ( st.dialog.fromExplore )
								Menus_Close( travel_menu );

							st.MENU = 0;
							st.dialog.fromExplore = 0;
						} break;

					case CS('o','p','e','n'):
						{
							//	remember if the dialog was initiated from explore mode
							if ( !(trap_Key_GetCatcher() & KEYCATCH_UI) )
							{
								st.dialog.fromExplore = 1;
								if ( sql( "SELECT bot FROM conversations;", 0 ) ) {
									trap_Cmd_ExecuteText( EXEC_NOW, va( "st_lookat %d ;", sqlint(0) ) );
									sqldone();
								}
							}

							Menus_CloseAll();
							Menus_Activate( travel_menu );
							Menus_Activate( dialog_menu );

						} break;
				}

			} break;
		// st_jobs
		case CS('j','o','b','s'):
			{
				//int		bot_id		= trap_ArgvI( 1 );
				//int		npc_id		= trap_ArgvI( 2 );

				Menus_CloseAll();
				Menus_ShowByName( "travel_menu" );
				Menus_ShowByName( "jobs_menu" );

			} break;
		case CS('p','o','s','t'):	//	st_postgame
			{
				// store the summary so when we get kicked from the game
				// we still have the stats to show in the end.
				while ( sql(	"SELECT	'st_summary'||client:plain, "
										"'\\client\\' || client:plain || "
										"'\\place\\' || rank:plain || "
										"'\\networth\\' || networth:plain || "
										"'\\name\\' || clients.client[ client ]^n || "
										"'\\me\\' || (client==%client):plain "
								"FROM players;", 0 ) ) {

					trap_Cvar_Set( sqltext(0), sqltext(1) );
				}
				if ( sql( "SELECT value FROM missions SEARCH key 'number';", 0) ) {
					trap_Cvar_Set( "st_summary", sqltext(0) );
					sqldone();
				}

				// record a high score in local cvar's to unlock maps!!
				if ( sql( "SELECT networth, 'score_'||missions.key['number']^value, missions.key['par_score'].value, missions.key['number'].value FROM players SEARCH client %client;", 0 ) )
				{
					int			ui_scoreVal;

					int			score	= sqlint(0);
					const char*	var		= sqltext(1);
					const char* val		= sqltext(0);
					int			par		= sqlint(2);
					int			number	= sqlint(3);
					int			current;

					trap_Cvar_Register( NULL, var, "0", CVAR_ARCHIVE );

					current = trap_Cvar_VariableInt( var );
					if ( current < score ) {
						trap_Cvar_Set( var, val );
					}

					/*	current		old high score
						par			score to beat
						score		score you just got
					
						ui_score

						Add one element from each section:
						1			no new high score
						2			score is the same as the last high score
						3			player set a high score

						10			next challenge was and still is locked
						20			next challenge has just been unlocked
						30			next challenge was and still is unlocked
					*/

					ui_scoreVal = 0;
					if( score < current )
						ui_scoreVal += 1;
					else if( score > current )
						ui_scoreVal += 3;
					else //they are equal
						ui_scoreVal += 2;

					if ( number < 5 ) {

						if( current < par && score < par )
							ui_scoreVal += 10;
						else if( current < par && score >= par )
							ui_scoreVal += 20;
						else if( current >= par )
							ui_scoreVal += 30;
					}

					trap_Cvar_Set( "ui_score", va( "%d", ui_scoreVal ) );

					trap_Cvar_Set( "ui_best", fn( current, FN_CURRENCY ) );

					sqldone();
				}

			} break;
		case CS('s','c','o','r'):	//	score
			{
				trap_SQL_Prepare	( "INSERT INTO scores(player,networth,cash,debt) VALUES(?,?,?,?);" );
				trap_SQL_BindArgs	();
				trap_SQL_Step		();
				trap_SQL_Done		();
			} break;
		//	st_startgame - server is saying the game is starting...
		case CS('s','t','a','r'):
			{
				Menus_CloseByName( "WaitForPlayers" );
			} break;
		//	st_bank - bank transaction done, update the gui
		case CS('b','a','n','k'):
			{
				menuDef_t * menu = Menus_FindByName( "shop_item_details" );
				if ( menu )
				{
					itemDef_t * item = Menu_FindItemByName( menu, "buyQty" );
					// update the buy screen if it's visible
					if ( menu->window.flags & WINDOW_VISIBLE && item && item->window.flags & WINDOW_VISIBLE ) {
						st_shop_select_buy();
					}
				}
			} break;
		//	a table has changed
		//	st_table
		case CS('t','a','b','l'):	
			{
				switch( SWITCHSTRING( UI_Argv(1) ) )
				{
				case CS('i','n','v','e'):
					cache_selllist();
					break;
				case CS('c','o','n','t'):
					sql( "UPDATE feeders SET id=-1,sel=-1 SEARCH name 'contactlist';", "e" );
					break;
				}

			} break;
		//	st_moa - players have just finished battling the MoA
		case CS('m','o','a',0):
			{
				int	won = trap_ArgvI( 1 );

				Menus_CloseAll();
				Menus_OpenByName( won?"moa_won":"moa_lose" );
			} break;
		//	st_chat - someone's saying something at the connect screen
		case CS('c','h','a','t'):
			{
				int client = trap_ArgvI( 1 );
				const char *msg = UI_Argv( 2 );
				UI_ST_ChatPrint( client, msg );
			} break;
			//fallthrough to st_talk intended

		//	st_beginturn
		case CS('b','e','g','i'):
			{
				Menus_FadeAllOutAndFadeIn( "travel_menu" );
				Menus_FadeAllOutAndFadeIn( "status_page" );

				if ( GS_TURN == 1 ) {
					trap_Cmd_ExecuteText( EXEC_NOW, "updateserver 2" );
				}
#ifdef USE_DRM
				if ( trap_Cvar_VariableInt( "com_keyvalid" ) == 0 && GS_TURN > 1 ) {
					Menus_FadeAllOutAndFadeIn( "registergame" );
				}
#endif

			} break;

		//	st_won
		case CS('w','o','n',0):
			{
				Menus_ActivateByName( "missioncomplete" );
			} break;
		// st_lost
		case CS('l','o','s','t'):
			{
				Menus_ActivateByName( "missionfailed" );
			} break;
		// st_cash
		case CS('c','a','s','h'):
			{
				effectDef_t *effect = UI_Effect_Find( "stash" );
				int qty = trap_ArgvI( 1 );
				UI_ST_CenterPrint( fn( qty, FN_CURRENCY ), effect );
			} break;
		// st_lag
		case CS('l','a','g',0):
			{
				qboolean show_lag = trap_ArgvI( 1 );
				if ( show_lag == qtrue ) {
					Menus_ActivateByName( "net_dc" );
				} else {
					Menus_CloseByName( "net_dc" );
				}
			} break;
		// st_cp
		case CS('c','p',0,0 ):
			{
				effectDef_t *fxd;
				fxd = UI_Effect_Find( UI_Argc() > 2 ? UI_Argv( 2 ) : "cp" );

				if ( sql( "SELECT index FROM strings SEARCH name $;", "t", UI_Argv(1) ) ) {
					char t[ MAX_INFO_STRING ];
					trap_SQL_Run( t, sizeof(t), sqlint(0), trap_ArgvI(3),0,0 );
					UI_ST_CenterPrint( t, fxd );
					sqldone();
				} else {
					UI_ST_CenterPrint( UI_Argv( 1 ), fxd );
				}
			} break;
#if 0	// broken, was never actually used
		// st_hint
		case CS('h','i','n','t' ):
			{
				effectDef_t *fxd;
				const char *message;
				fxd = UI_Effect_Find( UI_Argc() > 2 ? UI_Argv( 2 ) : "hint" );
				message = UI_Hint( UI_Argv( 1 ) );
				Com_Printf( va("[NOTICE] %s\n", message ) );
				UI_ST_CenterPrint( message, fxd );
			} break;
#endif
		// st_int
		case CS('i','n','t',0 ):
			{
				effectDef_t *fxd;
				fxd = UI_Effect_Find( UI_Argc() > 2 ? UI_Argv( 2 ) : "int" );
				Com_Printf( va("[NOTICE] %d\n", atoi( UI_Argv( 1 ) ) ) );
				UI_ST_IntegerPrint( atoi(UI_Argv( 1 )), fxd );
			} break;
		//	st_crosshair
		case CS('c','r','o','s'):
			{
				effectDef_t * e;

				UI_Effect_SetFlag( st.crosshair.effect, EF_NOFOCUS );
				st.crosshair.client = trap_ArgvI( 1 );

				if ( (st.gametype == GT_FFA) || (st.gametype == GT_ASSASSINATE && st.crosshair.client >= MAX_PLAYERS) ) 
					e = st.crosshair.enemy;
				else
					e = st.crosshair.ally;

				if ( e->rect.x == 0.0f ) {
					e->rect.x = -uiInfo.uiDC.glconfig.xbias;
				}

				if ( e->rect.w == 640.0f ) {
					e->rect.w = 640.0f + uiInfo.uiDC.glconfig.xbias * 2.0f;
				}

				if ( sql( "SELECT n FROM clients SEARCH client ?;", "i", st.crosshair.client ) ) {
					st.crosshair.effect = UI_Effect_SpawnText( &e->rect, e, sqltext(0), 0 );
					sqldone();
				}

			} break;
		// st_lagging
		case CS('l','a','g','g'):
			{
				int lagging = 0;

				sql( "SELECT COUNT(*) FROM clients WHERE lag=1;", "sId", &lagging );

				if ( lagging ) {
					Menus_OpenByName( "waiting_for_players" );
				} else {
					Menus_CloseByName( "waiting_for_players" );
				}
			} break;
		// st_reportscore
		case CS('r','e','p','o'):
			{
				if (trap_Cvar_VariableInt("ui_logged_in") == 1 ) {
					trap_Cmd_ExecuteText( EXEC_NOW, va("highscore %d %d %d %d %d %d", atoi(UI_Argv(1)), atoi(UI_Argv(2)), atoi(UI_Argv(3)), atoi(UI_Argv(4)), atoi(UI_Argv(5)), atoi(UI_Argv(6))));
				}
			} break;
		// st_inspect
		case CS('i','n','s','p'):
			{
				Menus_ActivateByName( "cheat" );
			}break;
#if DEVELOPER
		// st_dumptext
		case CS('d','u','m','p'):
			{
				trap_Print( "Dialog\n" );
				while( sql( "SELECT id,name,voice,model,spawn FROM npcs SEARCH name;",0 ) ) {

					int				npc_id = sqlint(0);
					const char *	name = sqltext(1);
					const char *	voice = sqltext(2);
					const char *	model = sqltext(3);
					const char *	spawn = sqltext(4);
					
					trap_Print( va("[%3d] %s     (model:'%s' voice: '%s' spawn: '%s')\n\n", npc_id, name, model, voice, spawn ) );

					while ( sql( "SELECT id,dialogs_text.id[id]^text,priority FROM dialogs SEARCH npc_id ?;", "i", npc_id ) ) {

						int				dialog_id = sqlint(0);
						const char *	dialog_text = sqltext(1);
						const char *	priority = sqltext(2);

						if ( priority && priority[ 0 ] ) {
							trap_Print( va( "    %d. (%s)\n%s\n", dialog_id, priority, dialog_text ) );
						} else {
							trap_Print( va( "    %d. %s\n", dialog_id, dialog_text ) );
						}

						while ( sql( "SELECT line,text FROM dialogs_answers SEARCH id ? SORT 1;", "i", dialog_id ) ) {

							int	line = sqlint(0);
							const char * text = sqltext(1);

							/*
							if ( sql( "SELECT cmd FROM dialogs_cmds SEARCH id ?1 WHERE line=?2", "ii", dialog_id, line ) ) {

								const char * cmd = sqltext(0);
								int dialog_next = 0;
								const char * next = Q_strstr( cmd, 256, "say " );

								if ( next ) {

									if ( sql( "SELECT id FROM dialogs SEARCH name $;", "t", COM_Parse( &next ) ) ) {
										dialog_next = sqlint(0);
										sqldone();
									}
								}

								if ( dialog_next > 0 ) {
									trap_Print( va( "        %d. %s [%d]    (%s)\n", line, text, dialog_next, cmd ) );
								} else {
									trap_Print( va( "        %d. %s    (%s)\n", line, text, cmd ) );
								}

								sqldone();
							}
							*/
							trap_Print( va( "        %d. %s\n", line, text ) );
						}
						trap_Print( "\n" );
					}
				}
			}break;
#endif
		default:
			return 0; // unkown command
	}

	return 1; // command has been accepted
}

static void UI_ST_PlayerSelectSlot_SetModel( playerInfo_t *info, char * model )
{
	vec3_t	viewangles;
	vec3_t	moveangles;

	memset( info, 0, sizeof(playerInfo_t) );
		
	viewangles[YAW]   = 180.0f - 10.0f;
	viewangles[PITCH] = 0.0f;
	viewangles[ROLL]  = 0.0f;
	
	VectorClear( moveangles );
		
	UI_PlayerInfo_SetModel( info, model, "", 0 );
	UI_PlayerInfo_SetInfo( info, LEGS_IDLE, LEGS_IDLE, viewangles, vec3_origin, WP_PISTOL, qfalse );
}

static void Menu_Item_RunScript( const char * menuName, const char * itemName, const char * script )
{
	menuDef_t * menu = Menus_FindByName( menuName );
	if ( menu )
	{
		itemDef_t * item = Menu_FindItemByName( menu, itemName );
		if ( item )
		{
			Item_RunScript( item, script );
		}
	}
}

static ID_INLINE void SolarSystemEye_Cpy( solarsystemEye_t *o, const solarsystemEye_t *i )
{
	VectorCopy( i->lookAt, o->lookAt );
	Quat_Cpy( o->direction, i->direction );
	o->radius = i->radius;
	o->fov = i->fov;
}


void UI_ST_SolarSystemBeginTransfer( const solarsystemEye_t *dest, int time )
{
	SolarSystemEye_Cpy( &st.travel.eye[0], &st.travel.ss.eye );
	SolarSystemEye_Cpy( &st.travel.eye[1], dest );
										  
	st.travel.lerpTime[0] = uiInfo.uiDC.realTime;
	st.travel.lerpTime[1] = uiInfo.uiDC.realTime + time;
}

void UI_ST_SolarSystemSnapEye( const solarsystemEye_t *eye )
{
	SolarSystemEye_Cpy( &st.travel.eye[0], eye );
	SolarSystemEye_Cpy( &st.travel.eye[1], eye );
	SolarSystemEye_Cpy( &st.travel.ss.eye, eye );

	UI_ST_SolarSystemBeginTransfer( eye, 0 );
}

static void st_shop_unselect_buy( void ) {
	sql( "UPDATE feeders SET sel=-1, id=-1 SEARCH name 'buylist';", "e" );
	trap_Cvar_Set( "ui_buying", "0" );
}

static void st_shop_unselect_sell( void ) {
	sql( "UPDATE feeders SET sel=-1, id=-1 SEARCH name 'selllist';", "e" );
	trap_Cvar_Set( "ui_buying", "0" );
}

static void st_shop_select_buy( void ) {

	if ( sql( "SELECT feeders.name[ 'buylist' ].id;", 0 ) ) {
		int	id = sqlint(0);
		sqldone();
		if ( id == -1 ) {
			return;
		}
	}

	st_shop_unselect_sell();

	if ( sql( "SELECT "
				"todaysprices.commodity_id[ feeders.name[ 'buylist' ].id ].price, "
				"cargomax, "
				"cargo, "
				"cash, "
				"commodities.abbreviation[ 'CH' ].id = feeders.name[ 'buylist' ].id "
				"FROM players SEARCH client %client;", 0 ) ) {

		int price		= sqlint(0);
		int	cargo_max	= sqlint(1);
		int	cargo_used	= sqlint(2);
		int cash		= sqlint(3);
		int	cargo_hold	= sqlint(4);
		sqldone();

		if ( cargo_max-cargo_used == 0 && !cargo_hold ) {
			// player doesn't have any room in carg hold
			trap_Cvar_Set( "ui_buying", "4" );

		} else if ( price > cash ) {
			// player doesn't have enough money to buy any
			trap_Cvar_Set( "ui_buying", "3" );

		} else {
			//	adjust the item details to show selected item
			int	buyMax;
			
			trap_Cvar_Set( "ui_buying", "1" );
			if ( cargo_hold ) {
				buyMax = min( UI_ST_GetMissionInt( "max_cargo" ) - cargo_max, cash / price );
				if (buyMax == 0) {
					trap_Cvar_Set( "ui_buying", "7" );
				}
			} else {
				buyMax = min( cargo_max-cargo_used, cash / price );
			}
		
			UI_ActivateEditDlg( "shop_item_details", "buyQty", (float)buyMax, 1.0f, (float)buyMax, qfalse );
		}
	}
}

//	the feeder has something selected, do some house keeping to reflect what's selected
static void st_shop_select_sell( void ) {

	if ( sql( "SELECT feeders.name[ 'selllist' ].id;", 0 ) ) {

		int commodity_id	= sqlint(0);
		sqldone();

		//	nothing is selected
		if ( commodity_id == -1 ) {
			st_shop_unselect_sell();
			return;
		}

		if ( sql( "SELECT commodities.id[ ?1 ].good = npcs.id[ conversations.id[ 0 ].npc ].good, inventory.commodity_id[ ?1 ].qty;", "i", commodity_id ) )
		{
			int	willbuy = sqlint(0);
			int qty = sqlint(1);
			sqldone();

			//	something is selected on sell list, make sure buylist is not selected
			st_shop_unselect_buy();

			if ( !willbuy ) {
				//	merchant refuses to buy commodity
				trap_Cvar_Set( "ui_buying", "5" );
				return;
			}

			//	if cargo holds are selected, find out howmany can be sold, can't sell full ones
			if ( sql(	"SELECT min(cargomax-cargo,?2) FROM players "
						"SEARCH client %client "
						"WHERE commodities.abbreviation[ 'CH' ].id=?1;", "ii", commodity_id, qty ) )
			{
				qty = sqlint(0);
				sqldone();

				if ( qty == 0 ) {
					//	cargo holds in use
					trap_Cvar_Set( "ui_buying", "6" );
					return;
				}
			}

			UI_ActivateEditDlg( "shop_item_details", "sellQty", (float)qty, 1.0f, (float)qty, qfalse );
			trap_Cvar_Set( "ui_buying", "2" );
		}
	}
}


//
//	PROCESS MENU SCRIPT COMMANDS
//
static void scriptcommand( const char * cmd, const char ** args )
{
	switch( SWITCHSTRING( cmd ) )
	{
	//st_awards
	case CS( 'a','w','a','r'):
		{
			int players;
			sql("SELECT COUNT(*) FROM players;", "sId", &players);
			if (players > 1 ) {
				if ( sql( "SELECT client=%client FROM players SEARCH stash DESC LIMIT 1;", 0) ) {
					if (sqlint(0) == 1) {
						trap_Cmd_ExecuteText( EXEC_NOW, "st_announcer award_stash 1500\n" );
					}
					sqldone();
				}
				if ( sql( "SELECT client=%client FROM players SEARCH readytime WHERE readytime  < missions.key['tradetime'].value*1000 LIMIT 1;", 0 ) ) {
					if (sqlint(0) == 1) {
						trap_Cmd_ExecuteText( EXEC_NOW, "st_announcer award_fastest 1500\n" );
					}
					sqldone();
				}
				if ( sql( "SELECT rank:rank, rank==COUNT FROM players SEARCH client %client;", 0 ) ) {
					if (sqlint(1) == 1) {
						trap_Cmd_ExecuteText( EXEC_NOW, "st_announcer last 1500\n" );
					} else {
						trap_Cmd_ExecuteText( EXEC_NOW, va("st_announcer %s 1000\n", sqltext(0) ) );
					}
					sqldone();
				}
			}
		}
		break;
	// st_results
	case CS( 'r','e','s','u'):
		{
			int profit, networth;
			float change;
			char *sound;
			if ( sql("SELECT SUM((todaysprices.commodity_id[ commodity_id ].price-cost)*qty) FROM inventory;", 0) ){
				profit = sqlint(0);
				sql("SELECT max(cash+inventory,missions.key['wealth'].value) FROM players SEARCH client %client;", "sId", &networth);
				sqldone();
				networth -= profit;
				change = profit/(float)networth;
				if ( change > 2.0f ) {
					sound = "trade_mega";
				} else if ( change > 1.5f ) {
					sound = "trade_awesome";
				} else if ( change > 1.0f ) {
					sound = "trade_excellent";
				} else if ( change > 0.5f ) {
					sound = "trade_good";
				} else {
					sound = "trade_notgreat";
				}

				if ( sql( "SELECT snd FROM npcs_voices SEARCH cmd $1;", "t", sound ) ) {
					trap_S_StartLocalSound(sqlint(0), CHAN_ANNOUNCER);
				}
			}
		}
		break;
		//st_endofgame
	case CS( 'e', 'n', 'd', 'o' ):
		{
			int number = trap_Cvar_VariableInt("st_summary");

			if ( sql("SELECT networth, challenges.number[ ?1 ].par_score FROM summary;", "i", number ) ) {
				int networth, par_score;
				float ratio;
				char *sound;
				char *rank;
				networth = sqlint(0);
				par_score = sqlint(1);
				sqldone();
				
				ratio = networth/(float)par_score;
				if (ratio > 5.0f ) {
					sound = "rank_inside";		// 500% -
					rank = "Inside Trader";
				} else if ( ratio > 3.0f ) {
					sound = "rank_grandmaster";	// 300% - 500%
					rank = "Grandmaster Trader";
				} else if ( ratio > 1.0f ) {
					sound = "rank_master";		// 100% - 300%
					rank = "Master Trader";
				} else if ( ratio > 0.5f ) {
					sound = "rank_bluecollar";	// 50% - 100%
					rank = "Bluecollar Trader";
				} else if ( ratio > 0.25f ) {
					sound = "rank_hobo";		// 25% - 50%
					rank = "Hobo Trader";
				} else {
					sound = "rank_bum";			//	0 - 25%
					rank = "Bum Trader";
				}
				
				trap_Cvar_Set( "ui_rank", rank );
				trap_Cvar_Set( "ui_par", fn( par_score, FN_CURRENCY ) );
				trap_S_StartLocalSound(trap_S_RegisterSound( va("sound/voices/announcer/%s.ogg",sound), qfalse), CHAN_ANNOUNCER);
			}
		} break;
		//st_url
	case CS( 'u', 'r', 'l', 0 ):
		{
			const char *url;
			String_Parse( args, &url );

			if( trap_Cvar_VariableInt( "r_fullscreen" ) )
				trap_Cmd_ExecuteText( EXEC_INSERT, va( "set r_fullscreen 0; vid_restart; openurl \"%s\"", url ) );
			else
				trap_Cmd_ExecuteText( EXEC_INSERT, va( "openurl \"%s\"", url ) );

		}
		break;
		// st_resetLogin
	case CS('r','e','s','e'):
		{
			trap_Cvar_Set( "ui_logged_in", "-1" );
			trap_Cvar_Set( "com_sessionid", "" );
			uiInfo.charsLoaded = false;
		} break;
	case CS('s','h','o','p'):
		{
			switch( SWITCHSTRING( cmd+5 ) )
			{
			//	st_shop_buy
			case CS('b','u','y',0):
				{
					if ( trap_Cvar_VariableInt( "ui_buying" ) == 1 ) {

						//	get purchase quantity from ui dialog
						if ( sql( "SELECT feeders.name[ 'buylist' ].id, feeders.name[ 'buyqty' ].sel;", 0 ) ) {

							int commodity_id = sqlint(0);
							int bcnt = sqlint(1);

							//	send a command to server to process the transaction
							trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_buy %i %i;", commodity_id, bcnt ) );

							//	unselect commodity
							sql( "UPDATE feeders SET sel=-1,id=-1 SEARCH name 'buylist';", "e" );
							trap_Cvar_Set( "ui_buying", "0" );

							sqldone();

							trap_S_StartLocalSound( st.buySound, CHAN_LOCAL );
							break;
						}
					}

					trap_S_StartLocalSound( st.cantbuySound, CHAN_LOCAL );

				} break;
			//	st_shop_sell
			case CS('s','e','l','l'):
				{
					if ( trap_Cvar_VariableInt( "ui_buying" ) == 2 ) {

						if ( sql( "SELECT feeders.name[ 'selllist' ].id, feeders.name[ 'sellqty' ].sel;", 0 ) ) {

							//	get sale quantiy from ui dialog
							int commodity_id = sqlint(0);
							int scnt = sqlint(1);;

							//	send a command to server to process the transaction
							trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_sell %i %i;", commodity_id, scnt ) );
							sqldone();

							trap_S_StartLocalSound( st.sellSound, CHAN_LOCAL );
							break;
						}
					}

					trap_S_StartLocalSound( st.cantbuySound, CHAN_LOCAL );
				} break;
			//	st_shop_end
			case CS('e','n','d',0):
				{
					if ( sql( "SELECT client FROM clients WHERE id = ?;", "i", st.npc ) )
					{
						//	tell the server to release this merchant from shopping
						trap_Cmd_ExecuteText( EXEC_APPEND, va( "st_endi %i ;", sqlint(0) ) );
						sqldone();
					}
				} break;

			//	st_shop_select_
			case CS('s','e','l','e'):
				{
					switch (SWITCHSTRING( cmd+12 ) )
					{
					case CS('b','u','y',0):			st_shop_select_buy();	break;
					case CS('s','e','l','l'):		st_shop_select_sell();	break;
					}
				} break;

			}
		} break;
	//	st_planet
	case CS('p','l','a','n'):
		{
			menuDef_t * menu = Menus_FindByName( "travel_details" );

			if ( menu && menu->window.flags & WINDOW_VISIBLE ) {
				break;
			}

			// the player has selected a planet from the travel screen...
			if ( !gsi.MENU && st.travel.selected )
			{
				//	ask server if we can travel
				st.travel.voted = st.travel.selected;
				st.travel.selectLockTime = uiInfo.uiDC.realTime + 500;

				trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_planet %d ;", st.travel.selected->id ) );
				if (sql("SELECT snd FROM npcs_voices WHERE cmd = $1", "t", st.travel.selected->name) ) {
					trap_S_StartLocalSound( sqlint(0), CHAN_ANNOUNCER);
					sqldone();
				}
			}
		} break;
	// st_zoom
	case CS('z','o','o','m'):
		{
			switch ( SWITCHSTRING( cmd + 4 ) )
			{
			case CS('i','n',0,0):	// st_zoomin
				{
					if( !st.travel.selected )
						UI_ST_SelectPlanet( st.travel.voted, 0 );

					if( st.travel.selected && st.travel.zoomState != STZ_IN )
					{
						solarsystemEye_t eye;

						SolarSystem_LookAt( &eye, st.travel.selected, (float)GS_TIME );
						UI_ST_SolarSystemBeginTransfer( &eye, 1500 );

						st.travel.zoomState = STZ_IN;
					}
				} break;

				case CS('t','o',0,0): //st_zoomto
					{
						const char * s;
						String_Parse( args, &s );
						if( sql( "SELECT id FROM planets SEARCH name $;", "t", s ) )
						{
							int i;
							int planet_id = sqlint(0);
							for( i = 0; i < st.travel.ss.planetCount; i++ )
							{
								if( st.travel.ss.planets[i].id == planet_id )
								{
									solarsystemEye_t eye;

									SolarSystem_LookAt( &eye, st.travel.ss.planets + i, (float)GS_TIME );
									UI_ST_SolarSystemSnapEye( &eye );

									st.travel.zoomState = STZ_IN;
									break;
								}
							}

							sqldone();
						}
					} break;

			case CS('o','u','t',0):	// st_zoomout
				{
					float angles[2], ofs[3];
					solarsystemEye_t eye;

					Float_Parse( args, angles + 0 );
					Float_Parse( args, angles + 1 );

					Float_Parse( args, ofs + 0 );
					Float_Parse( args, ofs + 1 );
					Float_Parse( args, ofs + 2 );

					if( st.travel.zoomState == STZ_OUT )
						break;

					UI_ST_SelectPlanet( 0, 0 );

					SolarSystem_TopDown( &eye, &st.travel.ss, (float)GS_TIME, angles, ofs, true );
					UI_ST_SolarSystemBeginTransfer( &eye, 1000 );

					st.travel.zoomState = STZ_OUT;
				} break;

			case CS('s','t','a','r'):	// st_zoomstart
			case CS('i','n','i','t'):	// st_zoominit
				{
					solarsystemEye_t eye;

					if( st.travel.current )
					{
						SolarSystem_LookAt( &eye, st.travel.current, (float)GS_TIME );
						st.travel.zoomState = STZ_IN;
					}
					else
					{
						float angles[2] = { 0, 0 }, ofs[3] = { 0, 0, 0 };
						SolarSystem_TopDown( &eye, &st.travel.ss, (float)GS_TIME, angles, ofs, true );
						st.travel.zoomState = STZ_OUT;
					}
					UI_ST_SolarSystemSnapEye( &eye );
				} break;
			}
		} break;
	//	st_solarsystem
	case CS('s','o','l','a'):
		{
			cache_solarsystem( &st.travel.ss );
		} break;
	//	st_travel
	case CS('t','r','a','v'):
		{
			const planetInfo_t * p = st.travel.voted;

			if ( p && p->time > 0 )
			{
				if ( p->time <= GS_TIMELEFT )
				{
					if ( sql( "SELECT name FROM planets SEARCH id ?;", "i", p->id ) ) {
						trap_Cmd_ExecuteText( EXEC_INSERT, va( "callvote travel \"%s\" ;", sqltext(0) ) );
						sqldone();
					}
				}
			}
		} break;
	//	st_won
	case CS('w','o','n',0):
		{
			Menus_ActivateByName( "missioncomplete" );
		} break;
	//	st_server
	case CS('s','e','r','v'):
		{
			if ( sql( "SELECT 'flush ; map ' || challenges.id[ feeders.name['maplist'].id ]^name || '\n';", 0 ) ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, sqltext(0) );
				sqldone();
			}

		} break;
		//	st_visit
	case CS('v','i','s','i'):
		{
			//	visit contact if we're not already talking to them.
			if ( sql( "SELECT id FROM feeders SEARCH name 'contactlist' WHERE conversations.id[ 0 ].bot != id;", 0 ) )
			{
				//	kill the cross hair name effect, since we're leaving.
				UI_Effect_SetFlag( st.crosshair.effect, EF_DELETEME );

				trap_Cmd_ExecuteText( EXEC_NOW, va( "st_visit %d", sqlint(0) ) );
				sqldone();
			}
		} break;
	//	st_scanforchallenges
	case CS('s','c','a','n'):
		{
			int count;

			UI_ST_ScanForMissions();

			sql( "SELECT COUNT(*) FROM challenges;", "sId", &count );


			if ( count == 1 ) {
				//	if there's only one map in list, launch it
				if ( sql( "SELECT name FROM challenges;", 0 ) ) {
					trap_Cmd_ExecuteText( EXEC_APPEND, va("flush ; map %s\n", sqltext(0) ) );
					sqldone();
					break;
				}
			}

			//if ( trap_Cvar_VariableInt( "com_downloading" ) == 1 ) {
				//Menus_ActivateByName( "waitformission" );
			//} else {
				Menus_ActivateByName( "selectmission" );
			//}

		}	break;
	//	st_dialog_*
	case CS('d','i','a','l'):
		{
			switch ( SWITCHSTRING( cmd+7 ) )
			{
				//	st_dialog_done
			case CS('d','o','n','e'):
				{
					if ( sql( "SELECT bot FROM conversations;", 0 ) )
					{
						//	tell the server to release this merchant from shopping
						trap_Cmd_ExecuteText( EXEC_APPEND, va( "st_endi %i ;", sqlint(0) ) );
						sqldone();
					}

				} break;
				//	st_dialog_say
			case CS('s','a','y',0):
				{
					if ( sql( "SELECT bot, feeders.name['answerlist'].id FROM conversations LIMIT 1;", 0 ) ) {

						trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_dialog %d %d ;", sqlint(0), sqlint(1) ) );
						sqldone();
					}
				} break;
			}

		} break;
	//	st_jobs_day
	case CS('j','o','b','s'):
		{
			if ( sql( "SELECT bot, feeders.name['jobslist'].id FROM conversations LIMIT 1;", 0 ) ) {

				trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_dialog %d %d ;", sqlint(0), sqlint(1) ) );
				sqldone();
			}
		} break;
	//	st_restart
	case CS('r','e','s','t'):
		{
			Com_Error( ERR_DISCONNECT, "" );
		} break;
	//	st_cash_*
	case CS('c','a','s','h'):
		{
			switch ( SWITCHSTRING( cmd+5 ) )
			{
				//	st_cash_withdraw
			case CS('w','i','t','h'):
				{
					if ( sql( "SELECT sel FROM feeders SEARCH name 'withdraw_cash';", 0 ) ) {
						trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_withdraw %d ;", sqlint(0) ) );
						sqldone();
					}
				} break;
				//	st_cash_deposit
			case CS('d','e','p','o'):
				{
					if ( sql( "SELECT sel FROM feeders SEARCH name 'deposit_cash';", 0 ) ) {
						trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_deposit %d ;", sqlint(0) ) );
						sqldone();
					}

				} break;

				//st_cash_paydebt
			case CS( 'p', 'a', 'y', 'd' ):
				{
					if( sql(  "SELECT MIN( -debt, cash ) FROM players SEARCH client %client;", 0 ) )
					{
						trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_deposit %d ;", sqlint(0) ) );
						sqldone();
					}
				}
				break;

				//st_cash_drawbalance
			case CS( 'd', 'r', 'a', 'w' ):
				{
					if( sql(  "SELECT MIN( debt, credit ) FROM players SEARCH client %client;", 0 ) )
					{
						trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_withdraw %d ;", sqlint(0) ) );
						sqldone();
					}
				}
				break;
			}
		} break;
	case CS('c','a','r','g'):
		{
			switch( SWITCHSTRING( cmd+6 ) )
			{
			//	st_cargo_pay
			case CS('p','a','y',0):
				{
					int f = trap_Cvar_VariableInt( "ui_cargoupgradeamount" );
					trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_cargo_upgrade %d ;", f ) );
					trap_Cvar_SetValue( "ui_cargoupgradeamount", 1.0f );

				} break;
			}
		} break;
	//	st_chat
	case CS('c','h','a','t'):
		{
			char info[ MAX_INFO_STRING ];

			trap_Cvar_VariableStringBuffer( "ui_chatline", info, sizeof( info ) );
			trap_Cmd_ExecuteText( EXEC_ONSERVER, va( "st_chat \"%s\" ;", info ) ); 
		} break;
	//	st_ready
	case CS('r','e','a','d'):
		{
			if ( uiInfo.uiDC.realTime - st.travel.readyTime > 750 )
			{
				st.travel.readyTime = uiInfo.uiDC.realTime;	// spam protection
				trap_Cmd_ExecuteText( EXEC_ONSERVER, "st_ready" );
			}

		} break;
	//	st_intermission
	case CS('i','n','t','e'):
		{
			trap_Cmd_ExecuteText( EXEC_ONSERVER, "st_intermission ;" );
		} break;
	//	st_beginturn
	case CS('b','e','g','i'):
		{
			trap_Cmd_ExecuteText( EXEC_ONSERVER, "st_beginturn ;" );
		} break;
	case CS('p','l','a','y'):
		{
			switch( SWITCHSTRING(cmd+13) )
			{
			//	st_playerselect_btn
			case CS('b','t','n',0):
				{
					int slot;
					Int_Parse( args, &slot );

					st.selectedprofile = slot;

					//	create new player
					if ( uiInfo.characters[ slot ].name[ 0 ] == '\0' )
					{
						trap_Cvar_Set( "ui_playerselect_selected", va("%d",slot) );
						trap_Cvar_Set( "ui_model", "mpc1" );
						trap_Cvar_Set( "ui_name", "" );
						UI_ST_PlayerSelectSlot_SetModel( &uiInfo.characters[ slot ].info, "mpc1" );

						Menu_Item_RunScript( "selectplayer", va("st_playerselect_nameedit%d", slot ), "setFocus this ; editfield this" );

					} else
					{
						Menus_OpenByName( "deleteplayer_ask" );
					}
				} break;
			//	st_playerselect_create
			case CS('c','r','e','a'):
				{
					int		slot;
					char	playerName	[ MAX_NAME_LENGTH ];
					char	playerModel	[ MAX_QPATH ];

					Int_Parse( args, &slot );

					trap_Cvar_VariableStringBuffer( "ui_name",	playerName, sizeof(playerName) );
					trap_Cvar_VariableStringBuffer( "ui_model",	playerModel, sizeof(playerModel) );

					if ( playerName[ 0 ] && playerModel[ 0 ] ) {
						trap_Cmd_ExecuteText( EXEC_APPEND, va( "createcharacter %d \"%s\" \"%s\" ;", slot, playerName, playerModel ) );
					}
				} break;

			//	st_playerselect_delete
			case CS('d','e','l','e'):
				{
					int slot = st.selectedprofile;

					if ( slot >= 0 && slot < 5 )
					{
						trap_Cmd_ExecuteText( EXEC_APPEND, va( "deletecharacter %d ;", slot ) );
						uiInfo.characters[ slot ].name[ 0 ] = '\0';
						uiInfo.characters[ slot ].info.legsModel = 0;
					}
				} break;

			//	st_playerselect_load
			case CS('l','o','a','d'):
				{
					int i;

					if( !uiInfo.charsLoaded )
					{
						for( i=0; i<MAX_CHARACTERS; i++ )
							uiInfo.characters[ i ].name[ 0 ] = '\0';
					}

					trap_Cmd_ExecuteText( EXEC_APPEND, "getaccount ;" );

					uiInfo.charsLoaded = true;
				} break;
			//	st_playerselect_model
			case CS('m','o','d','e'):
				{
					int		slot;
					char	playerModel	[ MAX_QPATH ];

					Int_Parse( args, &slot );
					trap_Cvar_VariableStringBuffer( "ui_model",	playerModel, sizeof(playerModel) );

					UI_ST_PlayerSelectSlot_SetModel( &uiInfo.characters[ slot ].info, playerModel );
				} break;
				//	st_playerselect_cancel
			case CS('c','a','n','c'):
				{
					int		slot;
					Int_Parse( args, &slot );
					UI_ST_PlayerSelectSlot_SetModel( &uiInfo.characters[ slot ].info, "" );
				} break;
				//	st_playerselect_select
			case CS('s','e','l','e'):
				{
					int		slot;
					Int_Parse( args, &slot );

					st.selectedprofile = slot;

					if ( uiInfo.characters[ slot ].name[ 0 ] != '\0' )
					{
						trap_Cvar_Set( "name", uiInfo.characters[ slot ].name);
						trap_Cvar_Set( "model", uiInfo.characters[ slot ].model);
						trap_Cvar_Set( "slot", va("%d", slot) );
						Menus_CloseByName( "selectplayer" );
						if (sql("SELECT 1 FROM challenges SEARCH name 'continue' WHERE complete != 1", NULL) ) {
							Menus_ActivateByName( "sto_continue" );
							sqldone();
						} else {
							Menus_ActivateByName( "joinserver" );
						}
					}
				} break;
			}
		} break;
		//	st_forgotpassword
		case CS('f','o','r','g'):
			{
				char	email[ MAX_SAY_TEXT ];
				trap_Cvar_VariableStringBuffer( "ui_email", email, sizeof(email) );
				trap_Cmd_ExecuteText( EXEC_APPEND, va("forgotpassword %s ;", email ) );
			} break;
		//	st_lagkick
		case CS('l','a','g','k'):
			{
				UI_ST_kickLaggingPlayers();
			} break;
		//	st_model
		case CS('m','o','d','e'):
			{
				char model[ MAX_QPATH ];
				trap_Cvar_VariableStringBuffer( "model", model, sizeof(model) );
				UI_ST_PlayerSelectSlot_SetModel( &st.modelinfo, model );
			} break;
	}
}

//
//	scans all the .mis files in the db/ folder and copies out all the missions rows from all the db's.
//
static void UI_ST_ScanForMissions( void )
{
	int		numfiles;
	char	buffer		[ 2048 ];
	char	filename	[ MAX_QPATH ];
	char	lang		[ MAX_NAME_LENGTH ];
	char**	files;
	int		i;
	int		n;

	trap_Cvar_VariableStringBuffer( "cl_lang", lang, sizeof(lang) );

	numfiles	= trap_FS_GetFileList( "db/", ".mis", buffer, sizeof(buffer) );
	files		= (char**)buffer;

	sql( "DELETE FROM challenges;", "e" );

	for ( i=0,n=0; i<numfiles; i++ )
	{
		trap_SQL_LoadDB( files[i], "sv_missions" );

		if ( sql( "SELECT name FROM common.files SEARCH fullname missions.key['db_name']^value;", 0 ) ) {

			trap_SQL_LoadDB( va("db/%s_%s.txt",sqltext(0),lang), "cl_missions_text" );	
			sqldone();
		} else {
			continue;
		}

		Q_strncpyz( filename, files[i] + 3, sizeof(filename) );
		filename[ strlen(filename)-4 ] = '\0';

		sql(	"INSERT INTO challenges(id,number,title,description,name,icon,time,time_units,par_score,hidden,version,complete) "
					"SELECT "
						"?1, "
						"missions.key['number'].value, "
						"missions_text.key['title']^value, "
						"missions_text.key['description']^value, "
						"$2, "
						"shader('ui/screenshots/'||$2), "
						"missions.key['time'].value, "
						"missions_text.key['time_units']^value, "
						"missions.key['par_score'].value, "
						"max(0,missions.key['hidden'].value), "
						"missions.key['version'].value, "
						"missions.key['complete'].value;", "ite", i, filename );
	}
}

static void UI_ST_InitFrontEnd( void )
{

	trap_SQL_Exec(
		"CREATE TABLE scores"
		"("
			"challenge		INTEGER, "
			"your_rank		INTEGER, "
			"your_score		INTEGER, "
			"first_rank		INTEGER, "
			"first_score	INTEGER, "
			"first_name		STRING "
		");"

		"CREATE TABLE stats"
		"("
//	\next_points\220\percentile\0.196723144834655\next_name\^2FrZ|^3Blaze\rank\3\points\0
			"key		STRING, "
			"value		STRING, "
			"integer	INTEGER "
		");"


		"CREATE TABLE challenges"
		"("
			"id INTEGER, "
			"number INTEGER, "
			"title STRING, "
			"description STRING, "
			"name STRING, "
			"icon INTEGER, "
			"par_score INTEGER, "
			"time INTEGER, "
			"time_units STRING, "
			"hidden INTEGER, "
			"version INTEGER, "
			"complete INTEGER "
		");"
	);

	//	initializing ui and not connected to a server, scan missions
	//	available for play
	UI_ST_ScanForMissions();

	trap_Cvar_Set( "ui_currentMap", "0" );
	trap_Cvar_Set( "ui_currentNetMap", "0" );
	trap_Cvar_Set( "ui_menustack", "" );
	trap_Cvar_Set( "ui_singlePlayerActive", "0" );

	st.chat.lineCount = trap_Cvar_VariableInt( "ui_chatlines" );

	trap_SQL_LoadDB( "ui/solarsystem.sql", 0 );
	cache_solarsystem( &st.travel.ss );
	
	trap_Con_Clear(1);
}

static qboolean UI_ST_InitGame( void )
{
	char info[ MAX_INFO_STRING ];

	//	all game types
	st.crosshair.ally = UI_Effect_Find( "ally" );
	st.crosshair.enemy = UI_Effect_Find( "enemy" );

	trap_GetConfigString( CS_SPACETRADER, info, sizeof(info) );

	//	check to make sure we're connecting to a space trader game
	if ( info[ 0 ] == '\0' )
		return qfalse;

	uiInfo.maxplayers	= 8;

	BG_ST_ReadState( info, &gsi );

	trap_UpdateGameState( &gsi );

	st.seed				= 1;	// this is supposed to come from the server

	st.chat.lineCount	= trap_Cvar_VariableInt( "ui_chatlines" );

	//	cache disaster icons
	sql( "INSERT INTO events_icons SELECT id,shader('ui/events/' || icon) FROM events_text;", "e" );

	cache_solarsystem( &st.travel.ss );
	cache_news();
	
	st.assets.graphShader	= trap_R_RegisterShaderNoMip( "ui/assets/pricegraph" );
	st.assets.youarehere	= trap_R_RegisterShaderNoMip( "ui/assets/youarehere" );
	st.assets.orbitShader	= trap_R_RegisterShaderNoMip( "ui/assets/orbit" );

	st.assets.decoration_accuracy = trap_R_RegisterShaderNoMip( "ui/assets/decoration_accuracy" );
	st.assets.decoration_money = trap_R_RegisterShaderNoMip( "ui/assets/decoration_money" );
	st.assets.decoration_stash = trap_R_RegisterShaderNoMip( "ui/assets/decoration_stash" );
	st.assets.decoration_time = trap_R_RegisterShaderNoMip( "ui/assets/decoration_time" );
	st.assets.decoration_boss = trap_R_RegisterShaderNoMip( "ui/assets/decoration_boss" );

	st.assets.arrow_past_head = trap_R_RegisterShaderNoMip( "ui/starmap/arrowpasthead" );
	st.assets.arrow_past_tail = trap_R_RegisterShaderNoMip( "ui/starmap/arrowpasttail" );
	st.assets.arrow_left_head = trap_R_RegisterShaderNoMip( "ui/starmap/arrowlefthead" );
	st.assets.arrow_left_tail = trap_R_RegisterShaderNoMip( "ui/starmap/arrowlefttail" );
//	st.assets.calendar = trap_R_RegisterShaderNoMip( "ui/assets/calendar" );

	st.assets.ready = UI_Effect_Find( "ready" );
	st.assets.notready = UI_Effect_Find( "notready" );

	st.gametype = trap_Cvar_VariableInt("g_gametype");
	st.travel.label = UI_Effect_Find( "planet_label" );

	st.travel.totalTime = (float)UI_ST_GetMissionInt( "time" );

	// cache commodity icons
	sql( "UPDATE commodities_text SET icon=shader('ui/commodity/' || commodities.id[ id ]^abbreviation);", "e" );
	sql( "UPDATE commodities_text SET icon=shader('ui/commodity/MG') WHERE icon=0;", "e" );

	cache_pricegraphs( GS_TIME+(ST_START_TIME-1), GS_TIME, 96 );

	if ( st.gametype == GT_ASSASSINATE ) {

		if ( sql( "SELECT npcs.id[ ?1 ]^name, bounty FROM bosses WHERE id=?1;", "i", GS_BOSS ) )
		{
			Q_strncpyz( st.assassinate.bossname, sqltext(0), sizeof(st.assassinate.bossname) );
			st.assassinate.bounty = sqlint(1);
			sqldone();
		}
	}


	cache_icons();

	st.buySound = trap_S_RegisterSound( "sound/misc/stash_pickup", qtrue );
	st.cantbuySound = trap_S_RegisterSound( "sound/misc/stash_full", qtrue );
	st.sellSound = trap_S_RegisterSound( "sound/ui/pop", qtrue );

	return qtrue;
}

static void UI_ST_UpdateSounds()
{
#if 0
	int i;
	for ( i=0; i<lengthof( gsInfo ); i++ )
	{
		int j;
		for ( j=0; j<gsInfo[ i ].count; j++ )
		{
			gsInfo_t * gs = gsInfo[ i ].v + j;

			if ( gs->flags & GE_TICK )
				uiInfo.uiDC.startLocalSound( uiInfo.uiDC.Assets.dings[ 0 ], CHAN_LOCAL_SOUND );

			if ( gs->flags & GE_LASTFRAME )
			{
				uiInfo.uiDC.startLocalSound( uiInfo.uiDC.Assets.dings[ 0 ], CHAN_LOCAL_SOUND );
				uiInfo.uiDC.startLocalSound( uiInfo.uiDC.Assets.dings[ 1 ], CHAN_LOCAL_SOUND );
				uiInfo.uiDC.startLocalSound( uiInfo.uiDC.Assets.dings[ 2 ], CHAN_LOCAL_SOUND );
				uiInfo.uiDC.startLocalSound( uiInfo.uiDC.Assets.dings[ 3 ], CHAN_LOCAL_SOUND );
				continue;
			}
		}
	}
#endif
}

static void UI_ST_SelectPlanet( const planetInfo_t * p, itemDef_t * item ) {
	int i;

	//	a different planet is selected
	if ( p == st.travel.selected )
		return;

	if ( item ) {
		trap_SQL_Prepare	( "UPDATE feeders SET sel=?1,id=?2 SEARCH name $3;" );
		trap_SQL_BindInt	( 1, (p)?p-st.travel.ss.planets:-1 );
		trap_SQL_BindInt	( 2, (p)?p->id:0 );
		trap_SQL_BindText	( 3, item->window.name );
		trap_SQL_Step		();
		trap_SQL_Done		();
	}

	
	// kill old labels
	for ( i=0; i<lengthof( st.travel.labels ); i++ ) {
		UI_Effect_SetFlag( st.travel.labels[i], EF_NOFOCUS );
	}

	// make new labels
	if ( p && !Q_stricmp(((menuDef_t*)item->parent)->window.name, "travel_page") ) {
		int i = 0;

		//	name
		st.travel.labels[ i++ ] = UI_Effect_SpawnText( 0, st.travel.label, p->name, p->icon );

		// travel time
		if ( p->time ) {
			if ( sql( "SELECT index FROM strings SEARCH name $;", "t", "T_Travel_Time" ) ) {

				int index = sqlint(0);
				char t[ MAX_NAME_LENGTH ];
				trap_SQL_Run( t, sizeof(t), index, p->time, 0,0 );
				st.travel.labels[ i++ ] = UI_Effect_SpawnText( 0, st.travel.label, t, 0 );
				sqldone();
			}
		}

		// disaster
		if( p->disaster_name && p->disaster_name[0]  ) {
			st.travel.labels[ i++ ] = UI_Effect_SpawnText( 0, st.travel.label, p->disaster_name, p->disaster_icon );
		}

		// bounties
		while ( sql( "SELECT npcs.id[ id ]^name || '\n' || (bounty:currency), icons.npc[ id ].icon FROM bosses WHERE planet_id=? AND captured=0;", "i", p->id ) ) {

			st.travel.labels[ i++ ] = UI_Effect_SpawnText( 0, st.travel.label, sqltext(0), sqlint(1) );
		}
	}


	st.travel.selected = p;
}

static void UI_ST_DrawPlanetLabels( rectDef_t * r, planetInfo_t * p, itemDef_t * item, float a )
{
	int i;
	float ts;
	rectDef_t t;
	float rgba[4];
	//float theta, radius;

	//ts = (item->textscale * r->w) / 36.0f;
	ts = item->textscale;

	rgba[0] = item->window.foreColor[0];
	rgba[1] = item->window.foreColor[1];
	rgba[2] = item->window.foreColor[2];
	rgba[3] = item->window.foreColor[3] * a;

	//theta = DEG2RAD( 90 );

	trap_R_SetColor( rgba );

	if ( st.travel.label ) {
		t = st.travel.label->rect;
		for ( i=0; i<lengthof( st.travel.labels ); i++ ) {

			if ( !UI_Effect_IsAlive( st.travel.labels[ i ] ) )
				break;

			UI_Effect_SetRect( st.travel.labels[ i ], &t, item );
			t.y += t.h;
		}
	}


	//planet name
	t.x = r->x;
	t.y = r->y - 6.0f;
	t.w = r->w;
	t.h = 1;

	trap_R_RenderText( &t, ts, rgba, p->name, -1, TEXT_ALIGN_RIGHT | TEXT_ALIGN_NOCLIP,
		ITEM_ALIGN_CENTER, item->textStyle | TEXT_STYLE_OUTLINED, item->font, -1, 0 );

	//travel time
	if ( p->time > 0 )
	{
		qhandle_t font = trap_R_RegisterFont( "digital" );
		t.x = t.x + t.w + 8.0f;
		trap_R_SetColor( colorGreen );
		trap_R_RenderText( &t, ts*1.5f, rgba, va("%i",p->time), -1, TEXT_ALIGN_LEFT | TEXT_ALIGN_NOCLIP, TEXT_ALIGN_CENTER, TEXT_STYLE_OUTLINED, font, -1, 0 );
	}
	trap_R_SetColor( NULL );
}

static void Menus_SetState( int state ) {

	if ( st.MENU == state )
		return;

	UI_SetMenuStack( &ui_menuStack );

	st.MENU = state;

	switch( st.MENU ) {

		//	st_awards
		//	the trading round has ended and the server is asking the client to display the awards
		//	for that round
		//
		case CS('a','w','a','r'):
			{
				switch ( st.gametype ) {
					case GT_HUB:
						Menus_FadeAllOutAndFadeIn( "trading_awards" );
						break;
					case GT_MOA:
					case GT_ASSASSINATE:
						Menus_FadeAllOutAndFadeIn( "assassinate_awards" );
						break;
				}
			} break;

		//	st_moa - players have just finished battling the MoA
		case CS('m','o','a','w'):
			Menus_CloseAll();
			Menus_OpenByName( "moa_won" );
			break;
		case CS('m','o','a','f'):
			Menus_CloseAll();
			Menus_OpenByName( "moa_lose" );
			break;

		case 0:
			break;

		//	ass u me its a vote menu
		default:
			{
				char cmd[ 5 ];
				menuDef_t * menu;
				
				cmd[ 0 ] = (char)(st.MENU)&0xFF;
				cmd[ 1 ] = (char)(st.MENU>>8)&0xFF;
				cmd[ 2 ] = (char)(st.MENU>>16)&0xFF;
				cmd[ 3 ] = (char)(st.MENU>>24)&0xFF;
				cmd[ 4 ] = '\0';

				menu = Menus_FindByName( va( "%svote", cmd ) );
				if( !menu )
					menu = Menus_FindByName( "vote" );

				Menus_Activate( menu );
				Menus_ActivateByName( "watchvote" );
			}
			break;

	}
}

static void UI_ST_InitMenus()
{
	//
	//	Initialize CG MenuStack
	//
	UI_SetMenuStack( &cg_menuStack );
	Menus_CloseAll();

	switch( st.gametype )
	{
	case GT_FFA:
		Menus_ActivateByName( "dashboard_ffa" );
		break;

	case GT_HUB:
		Menus_ActivateByName( "dashboard" );
		//Menus_ActivateByName( "dashboard_trade" );
		Menus_ActivateByName( "multiplayer_scores" );
		break;

	case GT_ASSASSINATE:
		Menus_ActivateByName( "dashboard" );
		Menus_ActivateByName( "combat" );
		Menus_ActivateByName( "combat-assassinate" );
		Menus_ActivateByName( "multiplayer_scores" );
		break;

	case GT_MOA:
		Menus_ActivateByName( "dashboard" );
		Menus_ActivateByName( "combat" );
		Menus_ActivateByName( "combat-moaboarding" );
		Menus_ActivateByName( "multiplayer_scores" );
		break;

	default:
		Menus_ActivateByName( "player" );
	}

	//
	//	Initialize UI MenuStack
	//
	UI_SetMenuStack( &ui_menuStack );
	Menus_CloseAll();

	if ( st.gametype == GT_HUB )
	{
		if ( GS_INPLAY == 0 )
		{
			Menus_FadeAllOutAndFadeIn( 0 );
			Menus_ActivateByName( "BeginTurn" );

			if ( ui_singlePlayerActive.integer == 0 )
			{
				//	first turn + local server means this client is the host...
				if ( GS_TURN == 1 && trap_Cvar_VariableInt( "sv_running" ) )
				{
					Menus_ActivateByName( "BeginTurn_Server" );
				} else
					Menus_ActivateByName( "BeginTurn_Client" );

				Menus_ActivateByName( "BeginTurn_Chat" );
			} else {
				if ( sql( "select id from tips_text random 1;", 0 ) ) {
					trap_Cvar_SetValue( "ui_professio_tip", (float)sqlint(0) );
					sqldone();
				}
			}

			if ( GS_TURN == 1 )
			{

			} else
			{
				Menus_ActivateByName( "BeginTurn_Results" );
			}

		} else {
			Menus_SetState( gsi.MENU );
#ifdef USE_DRM
			if ( trap_Cvar_VariableInt( "com_keyvalid" ) == 0 && GS_TURN > 1 ) {
				Menus_ActivateByName( "registergame" );
			}
#endif
		}
	}

	st.menus_init = 1;
}

float UI_ST_SolarSystemAdjustCameraCurve( float transfer_time )
{
#ifndef M_LN2
#define M_LN2 0.69314718055994530941723212145818F
#endif

	return 1.0F - ((1 - cosf((1 - transfer_time) * M_PI)) * 0.5F) *
		powf( (1.0F / M_LN2) * logf( 2 - transfer_time ), 1.65F );
}

void UI_ST_DrawSolarSystemShowcase( itemDef_t *item, float transfer_time )
{
	float time = (float)uiInfo.uiDC.realTime * 1e-7f;

	static int dir = -1;

	if( st.travel.showcase.stepTime - uiInfo.uiDC.realTime <= 0 )
	{
		dir = -1;

		if( !st.travel.selected )
		{
			st.travel.selected = st.travel.ss.planets - 1; //not a valid planet but the do loop will ++ it right away
			dir = 1;
		}

		do
		{
			st.travel.selected++;

			if( st.travel.selected == st.travel.ss.planets + st.travel.ss.planetCount )
			{
				st.travel.selected = st.travel.ss.planets + 0;
				dir = -1;
			}
		} while( st.travel.selected->decoration || !st.travel.selected->visible );

		{
			solarsystemEye_t eye;
			SolarSystem_LookAt( &eye, st.travel.selected, time );

			UI_ST_SolarSystemBeginTransfer( &eye, 20000 );
		}

		st.travel.showcase.stepTime = uiInfo.uiDC.realTime + 22000;
		
		transfer_time = 0;
	}

	transfer_time = UI_ST_SolarSystemAdjustCameraCurve( transfer_time );
	SolarSystem_LerpEye( &st.travel.ss.eye, &st.travel.eye[0], &st.travel.eye[1], dir, transfer_time );

	SolarSystem_Draw( &st.travel.ss, &item->window.rect, NULL, NULL, time, 0 );
}

static qhandle_t build_sniperzoom( void )
{
#define cR 50
#define iD 0.02F

	int i, t;

	float rx, ry;

	shapeGen_t gen[4] = { SHAPEGEN_FILL_USE_INDICES, SHAPEGEN_FILL_USE_INDICES, SHAPEGEN_FILL_USE_INDICES, SHAPEGEN_FILL_USE_INDICES };
	curve_t cv[4];

	vec2_t pts[4][(cR + 1) * 2];
	
	short iidxs[(cR + 4) * 3];

	vec4_t cls[(cR + 1) * 2];
	short idxs[cR * 2 * 3];

	//get the resolution multipliers
	ry = 0.4F;
	rx = ry * ((480 * uiInfo.uiDC.glconfig.yscale) / (640 * uiInfo.uiDC.glconfig.xscale)) *
		(640 * uiInfo.uiDC.glconfig.xscale) / (640 * uiInfo.uiDC.glconfig.xscale + 2 * uiInfo.uiDC.glconfig.xbias);

	//set up the curve_t records

	for( i = 0; i < 4; i++ )
	{
		Com_Memset( cv + i, 0, sizeof( curve_t ) );
		cv[i].pts = pts[i];
	}

	cv[2].colors = cv[3].colors = cls;
	cv[2].indices = cv[3].indices = idxs;

	//left half outer fill

	for( i = 0; i < cR + 1; i++ )
	{
		float t = M_PI + (i * M_PI) / cR;
		float x = sinf( t );
		float y = -cosf( t );

		cv[0].pts[i][0] = 0.5F + x * rx;
		cv[0].pts[i][1] = 0.5F + y * ry;
	}

	cv[0].pts[cR + 1 + 0][0] = 0.5F;
	cv[0].pts[cR + 1 + 0][1] = 0;

	cv[0].pts[cR + 1 + 1][0] = 0;
	cv[0].pts[cR + 1 + 1][1] = 0;

	cv[0].pts[cR + 1 + 2][0] = 0;
	cv[0].pts[cR + 1 + 2][1] = 1;

	cv[0].pts[cR + 1 + 3][0] = 0.5F;
	cv[0].pts[cR + 1 + 3][1] = 1;

	cv[0].numPts = cR + 1 + 4;

	//right half outer fill

	for( i = 1; i < cR + 1; i++ )
	{
		float t = M_PI + (i * M_PI) / cR;
		float x = -sinf( t );
		float y = cosf( t );

		cv[1].pts[i][0] = 0.5F + x * rx;
		cv[1].pts[i][1] = 0.5F + y * ry;
	}

	//weld verts
	cv[1].pts[cR][0] = cv[0].pts[0][0];
	cv[1].pts[cR][1] = cv[0].pts[0][1];

	cv[1].pts[0][0] = cv[0].pts[cR][0];
	cv[1].pts[0][1] = cv[0].pts[cR][1];

	cv[1].pts[cR + 1 + 0][0] = 0.5F;
	cv[1].pts[cR + 1 + 0][1] = 1;

	cv[1].pts[cR + 1 + 1][0] = 1;
	cv[1].pts[cR + 1 + 1][1] = 1;

	cv[1].pts[cR + 1 + 2][0] = 1;
	cv[1].pts[cR + 1 + 2][1] = 0;

	cv[1].pts[cR + 1 + 3][0] = 0.5F;
	cv[1].pts[cR + 1 + 3][1] = 0;

	cv[1].numPts = cR + 1 + 4;

	//outer fill indices
	t = 0;

	for( i = 0; i <= cR / 2 + 1; i++ )
	{
		iidxs[t * 3 + 0] = cR + 3;
		iidxs[t * 3 + 1] = (short)i;
		iidxs[t * 3 + 2] = (short)(i ? i - 1 : cR + 4);

		t++;
	}

	for( i = cR; i >= cR / 2 + 1; i-- )
	{
		iidxs[t * 3 + 0] = cR + 2;
		iidxs[t * 3 + 1] = (short)(i + 1);
		iidxs[t * 3 + 2] = (short)i;

		t++;
	}

	iidxs[t * 3 + 0] = cR / 2 + 1;
	iidxs[t * 3 + 1] = cR + 3;
	iidxs[t * 3 + 2] = cR + 2;
	t++;

	cv[0].indices = iidxs;
	cv[0].numIndices = t * 3;
	cv[1].indices = iidxs;
	cv[1].numIndices = t * 3;

	//inner gradient setup
	for( i = 0; i < cR + 1; i++ )
	{
		//outside verts
		VectorSet( cls[i], 1, 1, 1 );
		cls[i][3] = 1;

		//inside verts
		VectorSet( cls[cR + 1 + i], 1, 1, 1 );
		cls[cR + 1 + i][3] = 0;
	}

	for( i = 0; i < cR; i++ )
	{
		idxs[i * 6 + 0] = (short)i;
		idxs[i * 6 + 1] = (short)(i + 1);
		idxs[i * 6 + 2] = (short)(cR + 1 + i);

		idxs[i * 6 + 3] = (short)(cR + 1 + i);
		idxs[i * 6 + 4] = (short)(i + 1);
		idxs[i * 6 + 5] = (short)(cR + 1 + i + 1);
	}

	//left half inner gradient
	for( i = 0; i < cR + 1; i++ )
	{
		//outside verts
		cv[2].pts[i][0] = cv[0].pts[i][0];
		cv[2].pts[i][1] = cv[0].pts[i][1];

		//inside verts
		cv[2].pts[cR + 1 + i][0] = 0.5F * iD + (1.0F - iD) * cv[0].pts[i][0];
		cv[2].pts[cR + 1 + i][1] = 0.5F * iD + (1.0F - iD) * cv[0].pts[i][1];
	}

	cv[2].numPts = (cR + i) * 2;
	cv[2].numIndices = cR * 2 * 3;

	//right half inner gradient
	for( i = 0; i < cR + 1; i++ )
	{
		//outside verts
		cv[3].pts[i][0] = cv[1].pts[i][0];
		cv[3].pts[i][1] = cv[1].pts[i][1];

		//inside verts
		cv[3].pts[cR + 1 + i][0] = 0.5F * iD + (1.0F - iD) * cv[1].pts[i][0];
		cv[3].pts[cR + 1 + i][1] = 0.5F * iD + (1.0F - iD) * cv[1].pts[i][1];
	}

	//outside verts are automatically welded by virtue of where they come from
	//make sure the inside ones are also exact:

	cv[3].pts[cR + 1 + 0][0] = cv[2].pts[cR + 1 + cR][0];
	cv[3].pts[cR + 1 + 0][1] = cv[2].pts[cR + 1 + cR][1];

	cv[3].pts[cR + 1 + cR][0] = cv[2].pts[cR + 1 + 0][0];
	cv[3].pts[cR + 1 + cR][1] = cv[2].pts[cR + 1 + 0][1];

	cv[3].numPts = (cR + i) * 2;
	cv[3].numIndices = cR * 2 * 3;

	return trap_R_ShapeCreate( cv, gen, 4 );
	
#undef iD
#undef cR
}

static void UI_ST_WealthColor( int wealth, int minWealth, int maxWealth, float rgba[4] )
{
	if( wealth >= 0 )
	{
		Vec4_Lrp( rgba, g_color_table[COLOR_YELLOW], g_color_table[COLOR_GREEN],
			Com_Clamp( 0, 1, (float)wealth / (float)maxWealth ) );
	}
	else
	{
		Vec4_Lrp( rgba, g_color_table[COLOR_YELLOW], g_color_table[COLOR_RED],
			Com_Clamp( 0, 1, (float)wealth / (float)minWealth ) );
	}
}

static ID_INLINE qboolean PackedBits_Get( const uint *bits, uint idx )
{
	return ((bits[idx >> 5] & (uint)(1 << (idx & 0x1F))) != 0) ? qtrue : qfalse;
}

static ID_INLINE void PackedBits_Set( uint *bits, uint idx, qboolean value )
{
	if( value )
		bits[idx >> 5] |= (uint)(1 << (idx & 0x1F));
	else
		bits[idx >> 5] &= ~(uint)(1 << (idx & 0x1F));
}

static void UI_DrawStretchPic( float x, float y, float w, float h, qhandle_t shader )
{
	UI_AdjustFrom640( &x, &y, &w, &h );
	uiInfo.uiDC.drawStretchPic( x, y, w, h, 0, 0, 1, 1, shader );
}

static void UI_ST_DrawTravelRecordBar( itemDef_t *item )
{
	static int numTravels = 0;
	
	static struct
	{
		int				time;
		planetInfo_t	*planet;
	} ti[32]; //MAX 32 travels!!

	static qhandle_t graphShape;

	if( numTravels == 0 )
	{
		//load it up

		while( sql( "SELECT time, planet_id FROM travels SEARCH time;", NULL ) )
		{
			int i;

			int time = sqlint( 0 );
			int planet_id = sqlint( 1 );
			//int netWorth = sqlint( 2 );

			if( time < 0 )
				continue;

			ti[numTravels].time = time;
			//ti[numTravels].netWorth = netWorth;

			for( i = 0; i < st.travel.ss.planetCount; i++ )
			{
				if( st.travel.ss.planets[i].id == planet_id )
				{
					ti[numTravels].planet = st.travel.ss.planets + i;
					break;
				}
			}

			numTravels++;
		}

		if( !numTravels )
		{
			//if there's some SQL error then draw nothing and don't spam the DB
			numTravels = -1;
			return;
		}

		{
			int i, n;
			
			curve_t cv;
			shapeGen_t gen = SHAPEGEN_LINE;

			vec2_t pts[512];
			uint ptSet[lengthof( pts ) / 32 + 1]; //these bits be tightly packed bools...n'yarr!
			vec2_t uvs[lengthof( pts )];
			vec4_t cls[lengthof( pts )];

			int low, high;
			
			
			low = -100;
			high = 5000;

			n = -1;
			Com_Memset( ptSet, 0, sizeof( ptSet ) );

			while( sql( "SELECT time, networth FROM history SEARCH time;", 0 ) )
			{
				int time = sqlint( 0 ) - 1; //table time is 1-based
				int worth = sqlint( 1 );

				if( time < 0 || time > lengthof( pts ) )
					continue;

				pts[time][0] = time / st.travel.totalTime;
				pts[time][1] = (float)worth;

				PackedBits_Set( ptSet, time, qtrue );

				if( worth < low ) low = worth;
				if( worth > high ) high = worth;

				if( n < time ) n = time;
			}

			n++;

			for( i = 0; i < n; i++ )
			{
				int worth;

				/*
					If the point wasn't in the table (for whatever reason),
					just interpolate it *by ommitting it from the curve*.
				*/

				if( !PackedBits_Get( ptSet, i ) )
				{
					int j;
					for( j = i; j < n - 1; j++ )
					{
						pts[i][0] = pts[i + 1][0];
						pts[i][1] = pts[i + 1][1];
						PackedBits_Set( ptSet, i, PackedBits_Get( ptSet, i + 1 ) );
					}

					//adjust the count
					n -= 1;

					//do the loop again
					i -= 1; //cancel the i++ as we want to check the new element at this index
					continue;
				}

				worth = (int)pts[i][1];

				//normalize the vertical value and invert since screen space y increases downward
				pts[i][1] = 1.0F - Com_Clamp( 0.0f, 1.0f, (pts[i][1] - low) / (float)(high - low) );

				uvs[i][0] = pts[i][0];
				uvs[i][1] = 0.0f;

				UI_ST_WealthColor( worth, -10000, 100000, cls[i] );
			}

			Com_Memset( &cv, 0, sizeof( curve_t ) );

			cv.pts = pts;
			cv.uvs = uvs;
			cv.colors = cls;

			cv.numPts = n;

			graphShape = trap_R_ShapeCreate( &cv, &gen, 1 );
		}
	}

	if( numTravels > 0 )
	{
		int i;

		int totalTime = UI_ST_GetMissionInt( "time" );
		float w = min( item->window.rect.h, 64 );
		float h = w;

		float xScale = (item->window.rect.w-w) / (float)totalTime;

		for( i = 0; i < numTravels; i++ )
		{
			float x = (float)ti[i].time * xScale;
			float y = 0.0f;
			
			x += item->window.rect.x;
			y = item->window.rect.y + (item->window.rect.h - h) * 0.5F;

			trap_R_SetColor( g_color_table[ColorIndex( COLOR_WHITE )] );
			UI_DrawStretchPic( x+2.0f, y+2.0f, w-4.0f, h-4.0f, ti[i].planet->icon );

			if( i > 0 )
			{
				float xe = (float)ti[i].time * xScale;
				float xs = (float)ti[i - 1].time * xScale + w;

				float d = min( item->window.rect.h, 16 );
				float y = item->window.rect.y + (item->window.rect.h - d) * 0.5F;

				rectDef_t r;

				if( xe <= xs )
					//planets overlap, can't draw between them
					continue;

				xe += item->window.rect.x;
				xs += item->window.rect.x;

				UI_DrawStretchPic( xe - d, y, d, d, st.assets.arrow_past_head );
				UI_DrawStretchPic( xs, y, xe - xs - d, d, st.assets.arrow_past_tail );

				r.x = xs;
				r.y = item->window.rect.y;
				r.w = xe - xs;
				r.h = item->window.rect.h;

				if( r.w > d * 2.0f )
					r.w -= d;

				//UI_DrawStretchPic( r.x + r.w*0.5f - 14.0f, r.y, 28.0f, r.h, st.assets.calendar );
				trap_R_SetColor( item->window.backColor );
				trap_R_RenderText( &r, item->textscale, colorYellow, va( "%i", ti[i].time - ti[i - 1].time ),
					-1, TEXT_ALIGN_CENTER | TEXT_ALIGN_NOCLIP, ITEM_ALIGN_CENTER, item->textStyle, item->font, -1, 0 );
			}
		}

		//draw in the last arrow
		{
			float xe = item->window.rect.w;
			float xs = (float)ti[numTravels - 1].time * xScale + w * 0.5F;

			float d = min( item->window.rect.h, 16 );
			float y = item->window.rect.y + (item->window.rect.h - d) * 0.5F;

			rectDef_t r;

			if( xe > xs )
			{
				xe += item->window.rect.x;
				xs += item->window.rect.x;

				UI_DrawStretchPic( xe - d, y, d, d, st.assets.arrow_left_head );
				UI_DrawStretchPic( xs, y, xe - xs - d, d, st.assets.arrow_left_tail );

				r.x = xs;
				r.y = item->window.rect.y;
				r.w = xe - xs;
				r.h = item->window.rect.h;

				if( r.w > d * 2.0f )
					r.w -= d;

				//UI_DrawStretchPic( r.x + r.w*0.5f - 14.0f, r.y, 28.0f, r.h, st.assets.calendar );
				trap_R_SetColor( item->window.backColor );
				trap_R_RenderText( &r, item->textscale, colorGreen, va( "%i", totalTime - ti[numTravels - 1].time ),
					-1, TEXT_ALIGN_CENTER | TEXT_ALIGN_NOCLIP, ITEM_ALIGN_CENTER, item->textStyle, item->font, -1, 0 );
			}
		}

		DrawShape( &item->window.rect, graphShape, st.assets.graphShader );
	}

#if 0
	if ( trap_Key_IsDown( K_MOUSE1 ) )
	{
		float x = (float)uiInfo.uiDC.cursorx;
		float y = (float)uiInfo.uiDC.cursory;
		
		if ( Rect_ContainsPoint( &item->window.rect, x,y ) ) {
			rectDef_t r;
			float t = ((x-item->window.rect.x)/(item->window.rect.w)) * st.travel.totalTime;
			r.y = item->window.rect.y - 12.0f;
			r.h = 12.0f;
			r.x = x - 32.0f;
			r.w = 64.0f;
			trap_R_SetColor( colorGreen );
			trap_R_RenderText( &r, item->textscale, item->window.foreColor, fn( (int)t, FN_PLAIN ), -1, item->textalignx, item->textaligny, item->textStyle, item->font, -1, 0 );
		}
	}
#endif
}

static void UI_InsertMasterServerText( const char * info ) {

	char	key		[ MAX_INFO_KEY ];
	char	value	[ MAX_INFO_VALUE ];

	trap_SQL_Prepare( "UPDATE OR INSERT stats SET key=$1,value=$2,integer=?3 SEARCH key $1;" );
	for ( ;; )
	{
		Info_NextPair( &info, key, value );
		if ( key[ 0 ] == '\0' )
			break;

		trap_SQL_BindText	( 1, key );
		trap_SQL_BindText	( 2, value );
		trap_SQL_BindInt	( 3, atoi(value) );
		trap_SQL_Step		();
	}
	trap_SQL_Done();
}

int QDECL UI_ST_exec( int cmd, ... )
{
	va_list		argptr;
	va_start(argptr, cmd);

	switch ( cmd )
	{
		case UI_ST_INIT_FRONTEND:
			{
				UI_ST_InitFrontEnd();
			} break;

		case UI_ST_INIT_GAME:
			{
				UI_ST_InitGame();
				UI_ST_InitMenus();
			} break;

		case UI_ST_SHUTDOWN:
			{

			} break;

		case UI_ST_PAINT:
			{
				UI_ST_UpdateSounds();
			} break;

		case UI_ST_REFRESH:
			{
				if ( trap_UpdateGameState( &gsi ) ) {

					menuDef_t * menu;

					if ( st.menus_init ) {
						Menus_SetState( gsi.MENU );
					}

					//	hack to resize buttons on travel bar
					menu = Menus_FindByName( "travel_menu" );
					if ( menu && menu->window.flags&WINDOW_VISIBLE ) {
						if ( sql( "SELECT COUNT(*) FROM players;", 0 ) ) {
							resize_contacts( menu, sqlint(0) );
							sqldone();
						}
					}
				}

			} break;

		case UI_ST_MAINMENU:
			{
				if ( trap_Cvar_VariableInt( "st_summary" ) != 0 ) {
					//	we're back to the main menu with scores in tact, we must have just
					//	finished a game. show the summary.
					char summary[ MAX_INFO_STRING ];
					int i;

					trap_SQL_Exec(	"CREATE TABLE summary"
									"("
										"client INTEGER, "
										"place INTEGER, "
										"name STRING, "
										"networth INTEGER, "
										"me INTEGER "
									");"
									);

					trap_SQL_Prepare( "UPDATE OR INSERT summary CS $2 WHERE client=?1;" );

					for ( i=0; i < 8; i++ ) {

						trap_Cvar_VariableStringBuffer( va( "st_summary%d", i ), summary, sizeof(summary) );

						if ( summary[0] != '\0' )
						{
							trap_SQL_BindInt	( 1, i );
							trap_SQL_BindText	( 2, summary );
							trap_SQL_Step		();
						}
					}

					trap_SQL_Done();

					{
						char info[ MAX_INFO_STRING ];
						sql( "DELETE FROM stats;", "e" );
						trap_Cvar_VariableStringBuffer( "st_postresults", info, sizeof(info) );
						UI_InsertMasterServerText( info );
					}

					Menus_ActivateByName("endofgame");

					//	the client is disconnected via Com_Error, since we don't actually want to display
					//	the message we can trash it here and nothing will pop up on the user.
					trap_Cvar_Set("com_errorMessage", "" );
					trap_Cvar_Set("st_summary", "0" );
				}

			} break;

		case UI_ST_INGAMEMENU:
			{
				if( st.gametype == GT_HUB )
				{
					Menus_ActivateByName("travel_menu");
					Menus_ActivateByName("status_page");
				}
				else
					Menus_ActivateByName("escape_menu");

			} break;

		case UI_ST_CHATMENU:
			{
				if( GS_INPLAY && !trap_Cvar_VariableInt( "ui_singlePlayerActive" ) )
				{
					menuDef_t *m = Menus_FindByName( "Ingame_Chat" );
					if( m && !(m->window.flags & WINDOW_VISIBLE) )
						Menus_Activate( m );
				}
			}
			break;

		case UI_ST_OWNERDRAW_PAINT:
			{
				itemDef_t * item = va_arg( argptr, itemDef_t * );

				switch( item->window.ownerDraw )
				{
				case CG_SNIPER_ZOOM:
					{
						static qhandle_t sniperZoom = 0;

						if( !sniperZoom )
							sniperZoom = build_sniperzoom();

						DrawShape( &item->window.rect, sniperZoom, item->window.background );
					}
					break;

				case UI_TRAVEL_RECORD_BAR:
					{
						UI_ST_DrawTravelRecordBar( item );
					}
					break;

				case UI_TRAVELSCREEN:
					{
						int i;

						float a;
						float sel_scale = 1.0F;
						float time = (float)GS_TIME;
						float transfer_time = Com_Clamp( 0, 1,
							(float)(uiInfo.uiDC.realTime - st.travel.lerpTime[0]) /
							(float)(st.travel.lerpTime[1] - st.travel.lerpTime[0]) );

						//	we are in the front end, animating show case behind guis...
						if( ui_spacetrader.integer == 0 )
						{
							UI_ST_DrawSolarSystemShowcase( item, transfer_time );
							break;
						}

						switch( st.travel.zoomState )
						{
						case STZ_IN:
							a = 1.0F - Com_Clamp( 0, 1, transfer_time * 4 );
							
							transfer_time = UI_ST_SolarSystemAdjustCameraCurve( transfer_time );
							sel_scale = 1.0F - transfer_time;
							break;

						case STZ_OUT:
							a = 1.0F - Com_Clamp( 0, 1, (1.0F - transfer_time) * 4 );

							transfer_time = UI_ST_SolarSystemAdjustCameraCurve( transfer_time );
							sel_scale = transfer_time;
							break;

						default:
							a = 1.0F;
							break;
						}

#if 0
						if( ui_spacetrader.integer == 1 && trap_Key_IsDown( K_MOUSE1 ) )
						{
							menuDef_t * menu = Menus_FindByName( "travel_page" );
							if ( menu && menu->window.flags&WINDOW_VISIBLE ) {
								itemDef_t * item = Menu_FindItemByName( menu, "recordbar" );
								if ( item ) {
									float x = (float)uiInfo.uiDC.cursorx;
									float y = (float)uiInfo.uiDC.cursory;
									if ( Rect_ContainsPoint( &item->window.rect, x,y ) ) {
										time = ((x-item->window.rect.x)/(item->window.rect.w)) * st.travel.totalTime;
									}
								}
							}
						}
#endif

						SolarSystem_LerpEye( &st.travel.ss.eye, &st.travel.eye[0], &st.travel.eye[1], 1, transfer_time );
						SolarSystem_Draw( &st.travel.ss, &item->window.rect,
							(item->window.flags & WINDOW_DECORATION) ? NULL : st.travel.selected,
							(item->window.flags & WINDOW_DECORATION) ? NULL : st.travel.current,
							time, sel_scale );

						if( a < 0.001f )
							break;

						for( i = 0; i < st.travel.ss.planetCount; i++ )
						{
							planetInfo_t * p = st.travel.ss.planets + i;
							vec3_t pos;
							rectDef_t r;

							// skip any invisible planets
							if( !p->visible || p->decoration )
								continue;

							SolarSystem_PlanetPosition( p, (float)GS_TIME, pos );
							if( SolarSystem_GetScreenRect( item, pos, (float)p->size * 4 + 10, &r, 0 ) )
							{
								UI_ST_DrawPlanetLabels( &r, p, item, a );
							}
						}
					} break;
				case UI_SHOPBUY_PRICEGRAPH:
					{
						if ( sql( "SELECT feeders.name[ 'buylist' ].id, feeders.name[ 'selllist' ].id;", 0 ) ) {

							int buy		= sqlint(0);
							int sell	= sqlint(1);
							int id;
							sqldone();

							if ( buy >= 0 ) {
								id = buy;
							} else if ( sell >= 0 ) {
								id = sell;
							} else
								break;

							draw_pricegraph( item, id );

						}
					} break;
				case UI_UPGRADE_PRICEGRAPH:
					//draw_pricegraph( item, GS_CARGO_ID, GS_TIME-120, GS_TIME );
					break;
				case UI_PLAYERSELECT_SLOT0_MODEL:
				case UI_PLAYERSELECT_SLOT1_MODEL:
				case UI_PLAYERSELECT_SLOT2_MODEL:
				case UI_PLAYERSELECT_SLOT3_MODEL:
				case UI_PLAYERSELECT_SLOT4_MODEL:
					{
						menuDef_t *parent = (menuDef_t*)item->parent;
						float zoom = min( 1.0f, (uiInfo.uiDC.realTime - item->focusTime)/200.0f );
						
						if ( item != parent->focusItem )
							zoom = 1.0f - zoom;

						zoom *= zoom;

						UI_DrawPlayer	(	&item->window.rect,
											&uiInfo.characters[ item->window.ownerDraw-UI_PLAYERSELECT_SLOT0_MODEL ].info,
											uiInfo.uiDC.realTime,
											zoom
										);

					} break;

				case UI_PLAYER_MODEL:
					{
						UI_DrawPlayer	(	&item->window.rect,
											&uiInfo.characters[ st.selectedprofile ].info,
											uiInfo.uiDC.realTime, 1.0f );


					} break;

				case UI_CURRENT_MODEL:
					{
						UI_DrawPlayer	(	&item->window.rect,
											&st.modelinfo,
											uiInfo.uiDC.realTime, 1.0f );
					} break;

				case UI_DRAW_PLANET:
					{
						int planet_id = GS_PLANET;

						if ( item->values ) {
							if ( trap_SQL_Prepare( item->values ) ) {
								trap_SQL_Step();
								planet_id	= trap_SQL_ColumnAsInt( 0 );
								trap_SQL_Done();
							}
						}

						UI_DrawPlanet	( &st.travel.ss, &item->window.rect, planet_id, (float)GS_TIME );
					} break;

				case UI_NEWS_TICKER:
					{
						if( st.news.count > 0 )
						{
							char ticker[100];
							int t = uiInfo.uiDC.realTime;
							int o = (t - st.news.tickerTime) / 100;
							int i;

							for( i = 0; i < sizeof( ticker ); i++ )
							{
								int x = o + i - sizeof( ticker );

								if( x >= 0 )
								{
									char c;

									if( Q_IsColorString( st.news.blurbs[st.news.tickerIndex].text + x ) )
									{
										o += 2;
										i--;
										continue;
									}

									c = st.news.blurbs[st.news.tickerIndex].text[x];

									ticker[i] = c;

									if( !c )
										break;

									continue;
								}

								ticker[i] = ' ';
							}

							trap_R_RenderText( &item->window.rect, item->textscale, item->window.foreColor,
								ticker, -2, item->textalignx, item->textaligny, item->textStyle, item->font,
								-1, 0 );

							if( i == 0 )
							{
								st.news.tickerTime	= uiInfo.uiDC.realTime;
								st.news.tickerIndex = (st.news.tickerIndex + 1) % st.news.count;
							}
						}

					} break;

				case UI_RADAR:
					{
						int n;
						int i;
						rectDef_t rc;
						const rectDef_t *r = &item->window.rect;
						radarInfo_t	units[ 64 ];
						float cx, cy, R;

#define FIELD_OFFSET( T, f ) (int)(&((T*)0)->f)

						radarFilter_t filters[] =
						{
							{ ET_PLAYER, FIELD_OFFSET( entityState_t, pos.trBase ) },
							{ ET_ITEM, FIELD_OFFSET( entityState_t, origin ) },
						};

						R = min( r->w, r->h );
						n = trap_Get_Radar( units, lengthof( units ), filters, lengthof( filters ), 1500.0f );

						R *= 0.5F;

						cx = (r->x + r->x + r->w) * 0.5F;
						cy = (r->y + r->y + r->h) * 0.5F;

						trap_R_SetColor( colorWhite );
						for( i = 0; i < n; i++ )
						{
							qhandle_t t;
							const radarInfo_t *ri = units + i;

							//no they aren't backwards, check out the definition of ri->theta
							float x = sinf( ri->theta ) * ri->r * R;
							float y = cosf( ri->theta ) * ri->r * R;
							
							rc.x = cx + x - 8.0F;
							rc.y = cy + y - 8.0F;
							rc.w = 16.0F;
							rc.h = 16.0F;

							switch( ri->ent.eType )
							{
							case ET_PLAYER:	 {
								if (!(ri->ent.eFlags & EF_DEAD)) {
									t = ( ri->ent.clientNum < MAX_PLAYERS )?uiInfo.uiDC.Assets.radar_player:uiInfo.uiDC.Assets.radar_bot;
								} else {
									t = 0;
								}
							}	break;
							case ET_ITEM:	t = trap_R_RegisterShaderNoMip( bg_itemlist[ri->ent.modelindex].radar_effect ); break;
							default:		t = 0;
							}

							if ( t ) {
								UI_DrawHandlePic( rc.x, rc.y, rc.w, rc.h, t );
							}
						}						
					}
					break;
				case UI_BOSSHEALTH:
					{
						rectDef_t * r = &item->window.rect;

						if ( sql( "SELECT icon FROM icons SEARCH npc ?;", "i", GS_BOSS ) ) {

							trap_R_SetColor( item->window.backColor );
							uiInfo.uiDC.drawHandlePic( r->x, r->y, r->w, r->h, sqlint(0) );
							if ( GS_ASSASSINATE.maxhealth > 0 ) {
							float x,y,w;
								float h = r->h * (float)GS_ASSASSINATE.health / GS_ASSASSINATE.maxhealth;
								x = r->x;
								y = r->y + h;
								w = r->w;
								h = r->h - h;
								UI_AdjustFrom640( &x, &y, &w, &h );
								uiInfo.uiDC.drawStretchPic( x, y, w, h, 0, 0, 1, 1, uiInfo.uiDC.Assets.redbar );
							}
							sqldone();
						}
					} break;

				case UI_TRAVEL_DISASTERICON:
					{
						const planetInfo_t * p = st.travel.selected;

						if ( p && p->disaster_name && p->disaster_name[0] && p->disaster_icon ) {

							rectDef_t * r = &item->window.rect;
							uiInfo.uiDC.setColor( colorWhite );
							UI_DrawHandlePic( r->x, r->y, r->w, r->h, st.travel.selected->disaster_icon );
						}
					} break;
				}
			} break;
		case UI_ST_OWNERDRAW_MOUSEMOVE:
			{
				if ( ui_spacetrader.integer != 0 )
				{
					menuDef_t * menu = Menus_FindByName( "travel_details" );
					itemDef_t * item = va_arg( argptr, itemDef_t * );
					float x = (float)va_arg( argptr, double );   // todo : pass floats instead of promoting.
					float y = (float)va_arg( argptr, double );

					if ( menu && menu->window.flags & WINDOW_VISIBLE ) {
						break;
					}

					switch( item->window.ownerDraw )
					{
					case UI_TRAVELSCREEN:
						{
							//_UI_DrawRect( x-4, y-4, 8, 8, 2.0f, colorRed );
							if( uiInfo.uiDC.realTime >= st.travel.selectLockTime )
								UI_ST_SelectPlanet( SolarSystem_Select( &st.travel.ss, item, (float)GS_TIME, x,y ), item );
							
						} break;
					}
				}
			} break;
		case UI_ST_CONSOLECOMMAND:
			{
				return consolecommand( va_arg( argptr, const char * ) );

			} break;

		case UI_ST_SCRIPTCOMMAND:
			{
				const char * cmd	= va_arg( argptr, const char * );
				const char ** args	= va_arg( argptr, const char** );

				scriptcommand( cmd, args );
			} break;

		case UI_ST_CONFIGSTRING:
			{
				int num = va_arg( argptr, int );

				if ( num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS && trap_Cvar_VariableInt( "cl_spacetrader" ) == 1 ) {
					sql( "UPDATE icons SET icon=portrait(clients.client[client]^model) SEARCH client ?;", "ie", num-CS_PLAYERS );
				}

			} break;
		case UI_ST_REQUEST_HIGHSCORES:
			{
				while( sql( "SELECT Title FROM missions;", 0 ) )
				{
					trap_Cmd_ExecuteText( EXEC_APPEND, va( "globalhighscores \"%s\" \"%s\";", sqltext(0), UI_Cvar_VariableString("name") ) );
				}
			}
			break;

		case UI_ST_REPORT_HIGHSCORE_RESPONSE:
			{
				// \next_points\220\percentile\0.196723144834655\next_name\^2FrZ|^3Blaze\rank\3\points\0
				if ( trap_Cvar_VariableInt( "cl_spacetrader" ) == 0 ) {
					char info[ MAX_INFO_STRING ];
					trap_Cvar_VariableStringBuffer( "st_postresults", info, sizeof(info) );
					UI_InsertMasterServerText( info );
				}

			} break;

		case UI_ST_AUTHORIZED:
			{
				int		code		= va_arg( argptr, int );
				char	info[ MAX_INFO_STRING ];

				trap_Cvar_VariableStringBuffer( "cl_servermessage", info, sizeof(info) );

				switch(code)
				{
				case AUTHORIZE_OK:
					{
						menuDef_t * m = Menus_FindByName( "login" );
						//clear out old characters
						memset(uiInfo.characters, 0, sizeof(uiInfo.characters));

						if ( m && m->window.flags & WINDOW_VISIBLE )
						{
							Menus_Close( m );
							Menus_OpenByName("selectplayer");
						}

						trap_Cvar_Set( "ui_logged_in", "1" );

					} break;
				case AUTHORIZE_NOTVERIFIED:
					{
						trap_Cvar_Set( "ui_password", "" );
						trap_Cvar_Set( "ui_logged_in", "2" );
					}
					break;
				case AUTHORIZE_BAD:
					{
						trap_Cvar_Set( "ui_password", "" );
						trap_Cvar_Set( "ui_logged_in", "0" );

					} break;
				case AUTHORIZE_UNAVAILABLE:
					{
						menuDef_t * m = Menus_FindByName( "login" );
						if ( m && m->window.flags & WINDOW_VISIBLE )
						{
							Menus_Close( m );
							Menus_OpenByName("noauth");
						}

						trap_Cvar_Set( "ui_logged_in", "0" );

					} break;
				case AUTHORIZE_DELETECHARACTER:
					{
					} break;

				case AUTHORIZE_CREATECHARACTER: // flow through
					{
						int error_code = atoi(info);
						if (error_code == -1)
						{
							int i;
							for (i=0; i < MAX_CHARACTERS; i++) {
								if (uiInfo.characters[i].name[0] == '\0') {
									uiInfo.characters[i].info.legsModel = 0;
								}
							}
							if ( sql( "SELECT index FROM strings SEARCH name 'TT_PLAYER_INVALIDNAME';", 0 ) ) {
								char t[ MAX_INFO_STRING ];
								trap_SQL_Run( t, sizeof(t), sqlint(0), 0,0,0 );
								sqldone();

								trap_Cvar_Set( "ui_errorMessage", t );
								Menus_OpenByName( "error_popmenu" );
								break;
							}
						}
					}
				case AUTHORIZE_ACCOUNTINFO:
					{
						int i;
						for ( i=0; i<MAX_CHARACTERS; i++ )
						{
							const char * s = Info_ValueForKey( info, va("%d", i ) );

							if ( s && s[0] )
							{
								characterInfo_t * c = uiInfo.characters + i;

								Q_strncpyz( c->name, COM_Parse( &s ), sizeof(c->name) );
								Q_strncpyz( c->model, COM_Parse( &s ), sizeof(c->model) );
								
								c->rank = atoi(COM_Parse( &s ));

								UI_ST_PlayerSelectSlot_SetModel( &c->info, c->model );
							} else
								uiInfo.characters[ i ].name[ 0 ] = '\0';
						}

					} break;
				}
			}
			break;
		case UI_ST_SERVER_ERRORMESSAGE:
			{
				char *message = va_arg( argptr, char * );
				Com_Error( ERR_SERVERDISCONNECT, message);
			} break;
	}

	return 1;
}

int UI_GlobalInt() {
	char token[ MAX_NAME_LENGTH ];
	trap_Argv( 0, token, sizeof(token) );

	switch( SWITCHSTRING(token) )
	{
		// "%taxes"
	case CS( 't','a','x','e'):
		{
			int tax = 0;
			sql( "SELECT max(missions.key['moa_mintax'].value,((inventory+cash)*missions.key['moa_taxrate'].value)/100) FROM players SEARCH client %client;", "sId", &tax );
			return tax;
		} break;
	case CS( 'c','a','r','g'):
		{
			int cargo_left;
			sql( "SELECT cargomax-cargo FROM players SERACH client %client;", "sId",  &cargo_left );
			return cargo_left;
		} break;	// "%cargoleft"
	}

	return 0;
}

const char * UI_ST_getString( int id, ... )
{
	va_list		argptr;
	va_start(argptr, id);

	switch( id )
	{
	case UI_PLAYERSELECT_SLOT0_BTN:
	case UI_PLAYERSELECT_SLOT1_BTN:
	case UI_PLAYERSELECT_SLOT2_BTN:
	case UI_PLAYERSELECT_SLOT3_BTN:
	case UI_PLAYERSELECT_SLOT4_BTN:
		{
			return (uiInfo.characters[ id-UI_PLAYERSELECT_SLOT0_BTN ].name[0])?"DELETE":"CREATE";

		} break;

	case UI_PLAYERSELECT_SLOT0_NAME:
	case UI_PLAYERSELECT_SLOT1_NAME:
	case UI_PLAYERSELECT_SLOT2_NAME:
	case UI_PLAYERSELECT_SLOT3_NAME:
	case UI_PLAYERSELECT_SLOT4_NAME:
		{
			characterInfo_t * c = uiInfo.characters + id-UI_PLAYERSELECT_SLOT0_NAME;

			if ( c->name[ 0 ] == '\0' )
				return 0;

			if ( c->rank == 0 )
				return c->name;

			return va( "%s^-\n%s PLACE", c->name, fn(c->rank,FN_RANK) );
		} break;

	case UI_PLAYER_NAME:
		{
			return (st.selectedprofile>=0)?uiInfo.characters[ st.selectedprofile ].name:0;
		} break;
		
	default:
		{
			char t[ 2048 ];
			trap_SQL_Run( t, sizeof(t), id, 0,0,0 );
			return va( "%s", t );

		} break;
	}
}
