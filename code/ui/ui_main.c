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

/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

uiInfo_t uiInfo;




static const char *MonthAbbrev[] = {
	"Jan","Feb","Mar",
	"Apr","May","Jun",
	"Jul","Aug","Sep",
	"Oct","Nov","Dec"
};

#define MAX_MENU_STACKS 64

menuStackDef_t	ui_menuStack;
menuStackDef_t	cg_menuStack;


static void UI_StartServerRefresh( void );
static void UI_StopServerRefresh( void );
static void UI_DoServerRefresh( void );

extern void Save_Item( fileHandle_t f, itemDef_t * item );


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
vmCvar_t  ui_new;
vmCvar_t  ui_debug;
vmCvar_t  ui_initialized;
vmCvar_t  ui_spacetrader;
vmCvar_t  ui_tooltipmode;
vmCvar_t  ui_drawMenu;

void _UI_Init( qboolean );
void _UI_Shutdown( qboolean );
void _UI_KeyEvent( int key, qboolean down );
int _UI_MouseEvent( int dx, int dy );
void _UI_Refresh( int realtime );
qboolean _UI_IsFullscreen( void );

extern int debug_cgmenu;
extern int UI_GlobalInt();

int QDECL vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  )
{
	REF_PARAM( arg3 );
	REF_PARAM( arg4 );
	REF_PARAM( arg5 );
	REF_PARAM( arg6 );
	REF_PARAM( arg7 );
	REF_PARAM( arg8 );
	REF_PARAM( arg9 );
	REF_PARAM( arg10 );
	REF_PARAM( arg11 );

	if ( debug_cgmenu )
		UI_SetMenuStack( &cg_menuStack );
	else
		UI_SetMenuStack( &ui_menuStack );

  switch ( command ) {
	  case UI_GETAPIVERSION:
		  return UI_API_VERSION;

	  case UI_INIT:
		  _UI_Init(arg0);
		  uiInfo.clientNum = arg1;
		  return 0;

	  case UI_GAMEINIT:
		  {
			  UI_ST_exec( UI_ST_INIT_GAME );
		  } return 0;

	  case UI_SHUTDOWN:
		  _UI_Shutdown(arg0);
		  return 0;

	  case UI_KEY_EVENT:
		  _UI_KeyEvent( arg1, arg2 );
		  return 0;

	  case UI_MOUSE_EVENT:
		  return _UI_MouseEvent( arg1, arg2 );

	  case UI_REFRESH:
		  if (	(GS_INPLAY || !ui_spacetrader.integer) &&
					!(trap_Key_GetCatcher() & KEYCATCH_UI) &&
					!_UI_IsFullscreen() )
				UI_SetMenuStack( &cg_menuStack );

		  _UI_Refresh( arg1 );
		  return 0;

	  case UI_IS_FULLSCREEN:
		  return _UI_IsFullscreen();

	  case UI_SET_ACTIVE_MENU:
		  _UI_SetActiveMenu( arg0 );
		  return 0;

	  case UI_CONSOLE_COMMAND:
		  return UI_ConsoleCommand(arg0);

	  case UI_DRAW_CONNECT_SCREEN:
		  UI_DrawConnectScreen( arg0 );
		  return 0;
	  case UI_REPORT_HIGHSCORE_RESPONSE:
		  UI_ST_exec(UI_ST_REPORT_HIGHSCORE_RESPONSE, arg0);
		  return 0;
	  case UI_AUTHORIZED:
		  UI_ST_exec(UI_ST_AUTHORIZED, arg0, arg1);
		  return 0;
	  case UI_SERVER_ERRORMESSAGE:
		  UI_ST_exec(UI_ST_SERVER_ERRORMESSAGE, arg0 );
		  return 0;
	  case UI_GLOBAL_INT:
		return UI_GlobalInt();
	}

	return -1;
}



void AssetCache() {
	int n;

	for( n = 0; n < NUM_CROSSHAIRS; n++ ) {
		uiInfo.uiDC.Assets.crosshairShader[n] = trap_R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a' + n ) );
	}
	//REMOVE SOUND
	//uiInfo.newHighScoreSound = trap_S_RegisterSound("sound/feedback/voc_newhighscore.wav", qfalse);

    uiInfo.uiDC.Assets.shipModel = trap_R_RegisterModel( ASSET_PLANETMODEL );
	uiInfo.uiDC.Assets.planetModel = trap_R_RegisterModel( ASSET_PLANETMODEL );
	
	uiInfo.uiDC.Assets.orbit = trap_R_RegisterShaderNoMip( "ui/assets/orbit" );
	uiInfo.uiDC.Assets.orbitSel = trap_R_RegisterShaderNoMip( "ui/assets/orbitsel" );

	uiInfo.uiDC.Assets.star = trap_R_RegisterShaderNoMip( "ui/assets/star" );
	uiInfo.uiDC.Assets.menu = trap_R_RegisterShaderNoMip( "ui/assets/menubrushed" );
	uiInfo.uiDC.Assets.menubar = trap_R_RegisterShaderNoMip( "ui/assets/menubar" );
	uiInfo.uiDC.Assets.sbThumb = trap_R_RegisterShaderNoMip( "ui/assets/scrollarrows" );
	uiInfo.uiDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( "ui/assets/slider" );
	uiInfo.uiDC.Assets.lineshadow = trap_R_RegisterShaderNoMip( "ui/assets/pricegraph_shadow" );
	uiInfo.uiDC.Assets.redbar = trap_R_RegisterShaderNoMip( "ui/assets/redalpha" );
	uiInfo.uiDC.Assets.radar_player = trap_R_RegisterShaderNoMip( "radarPlayer" );
	uiInfo.uiDC.Assets.radar_bot = trap_R_RegisterShaderNoMip( "radarBot" );

}

void _UI_DrawSides(float x, float y, float w, float h, float size) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.glconfig.xscale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void _UI_DrawTopBottom(float x, float y, float w, float h, float size) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.glconfig.yscale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap_R_SetColor( color );

  _UI_DrawTopBottom(x, y, width, height, size);
  _UI_DrawSides(x, y, width, height, size);

	trap_R_SetColor( NULL );
}

/*
=================
_UI_Refresh
=================
*/

void UI_DrawCenteredPic(qhandle_t image, int w, int h) {
  int x, y;
  x = (SCREEN_WIDTH - w) >> 1;
  y = (SCREEN_HEIGHT - h) >> 1;
  UI_DrawHandlePic((float)x, (float)y, (float)w, (float)h, image);
}

int frameCount = 0;
int startTime;

#define	UI_FPS_FRAMES	4
void _UI_Refresh( int realtime )
{
	static int index;
	static int	previousTimes[UI_FPS_FRAMES];

	//if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
	//	return;
	//}

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if ( index > UI_FPS_FRAMES ) {
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < UI_FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		uiInfo.uiDC.FPS = (float)(1000 * UI_FPS_FRAMES / total);
	}

	UI_ST_exec( UI_ST_REFRESH );

	UI_UpdateCvars();

	UI_Effects_Update();

	if ( ui_drawMenu.integer ) {
		if (Menu_Count() > 0) {
			// paint all the menus
			Menu_PaintAll();
			// refresh server browser list
			UI_DoServerRefresh();
		}
	}

	UI_Effects_Draw( 0 );

	UI_ST_exec( UI_ST_PAINT );

	// tool tips
	if ( ui_tooltipmode.integer == 2 && UI_Effect_IsAlive( uiInfo.tooltip ) ) {

		float xb = DC->glconfig.xbias / DC->glconfig.xscale;
		rectDef_t r;
		effectDef_t * e = UI_Effect_GetEffect( uiInfo.tooltip );

		r.x = e->rect.x + (float)DC->cursorx;
		r.y = e->rect.y + (float)DC->cursory;
		r.w = e->rect.w;
		r.h = e->rect.h;

		//	don't let rect go off screen
		if ( r.x < -xb ) r.x = -xb;
		if ( r.y < 0.0f ) r.y = 0.0f;
		if ( r.x + r.w > 640.0f + xb ) r.x = 640.0f + xb - r.w;
		if ( r.y + r.h > 480.0f ) r.y = 480.0f - r.h;

		UI_Effect_SetJustRect( uiInfo.tooltip, &r );
	}
}

/*
=================
_UI_Shutdown
=================
*/
void _UI_Shutdown( qboolean save_menustack ) {

	if ( save_menustack )
	{
		int i;
		char info[ MAX_INFO_STRING ];
		info[ 0 ] = '\0';

		for ( i=0; i<ui_menuStack.count; i++ )
		{
			Q_strcat( info, sizeof(info), ui_menuStack.m[ i ]->window.name );
			Q_strcat( info, sizeof(info), " " );
		}

		trap_Cvar_Set( "ui_menustack", info );
	} else
		trap_Cvar_Set( "ui_menustack", "" );

}

static int QDECL SortEffects( const void *arg1, const void *arg2 ) {
	effectDef_t ** a = (effectDef_t**)arg1;
	effectDef_t ** b = (effectDef_t**)arg2;

	return Q_stricmp( (*a)->name, (*b)->name );
}

qboolean Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;
	cachedAssets_t * a = &uiInfo.uiDC.Assets;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}

	for( ; ; )
	{

		memset(&token, 0, sizeof(pc_token_t));

		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			break;
		}

		switch( SWITCHSTRING( token.string ) )
		{
		case CS('f','o','n','t'):
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			a->font = trap_R_RegisterFont(tempStr);
			break;

		case CS('m','e','n','u'):
			{
				switch( SWITCHSTRING( token.string + 4 ) )
				{
					//	menuEnterSound
				case CS('e','n','t','e'):
					if (!PC_String_Parse(handle, &tempStr)) {
						return qfalse;
					}
					a->menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
					break;

					//	menuExitSound
				case CS('e','x','i','t'):
					if (!PC_String_Parse(handle, &tempStr)) {
						return qfalse;
					}
					a->menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
					break;
					// menuBuzzSound
				case CS('b','u','z','z'):
					if (!PC_String_Parse(handle, &tempStr)) {
						return qfalse;
					}
					a->menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
					break;
				}
			} break;

		case CS('d','i','n','g'):
			{
				int i = token.string[ 4 ] - '0';
				if ( i < 0 || i > 4 )
					return qfalse;

				if (!PC_String_Parse(handle, &tempStr)) {
					return qfalse;
				}
				a->dings[ i ] = trap_S_RegisterSound( tempStr, qfalse );
			} break;
			//	itemFocusSound
		case CS('i','t','e','m'):
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			a->itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			break;

			//	cursor
		case CS('c','u','r','s'):
			if (!PC_String_Parse(handle, &a->cursorStr)) {
				return qfalse;
			}
			a->cursor = trap_R_RegisterShaderNoMip( a->cursorStr);
			break;

		case CS('f','a','d','e'):
			{
				switch( SWITCHSTRING( token.string + 4 ) )
				{
					//	fadeClamp
				case CS('c','l','a','m'):
					if (!PC_Float_Parse(handle, &a->fadeClamp)) {
						return qfalse;
					}
					break;
					//	fadeCycle
				case CS('c','y','c','l'):
					if (!PC_Int_Parse(handle, &a->fadeCycle)) {
						return qfalse;
					}
					break;
					//	fadeAmount
				case CS('a','m','o','u'):
					if (!PC_Float_Parse(handle, &a->fadeAmount)) {
						return qfalse;
					}
					break;
				}
			} break;
			// effectDef_t
		case CS('e','f','f','e'):
			{
				
				if ( a->effectCount < lengthof(a->effects) )
				{
					effectDef_t * effect = UI_Alloc( sizeof(effectDef_t) );
					a->effects[ a->effectCount++ ] = effect;

					if ( !Effect_Parse( handle, effect ) )
						return qfalse;
				}

			} break;
		}
	}

	qsort( a->effects, a->effectCount, sizeof(a->effects[0]), SortEffects );
	return qtrue;
}

void UI_ParseStrings( const char * stringFile ) {

	int			handle;

	handle = trap_PC_LoadSource( stringFile );
	if ( !handle ) {
		return;
	}

	for ( ;; )
	{
		const char * id;
		const char * src;
		int index;

		if ( !PC_String_Parse( handle, &id ) )
			break;

		if ( !PC_String_Parse( handle, &src ) )
			break;

		index = trap_SQL_Compile( src );

		sql( "INSERT INTO strings(index,name) VALUES(?1,$2);", "ite", index,id );
	}

	trap_PC_FreeSource(handle);
}

void UI_ParseMenu(const char *menuFile) {
	int handle;
	pc_token_t token;

	handle = trap_PC_LoadSource(menuFile);
	if (!handle) {
		return;
	}

	for( ; ; )
	{
		memset(&token, 0, sizeof(pc_token_t));
		if (!trap_PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle, menuFile );
		}
	}
	trap_PC_FreeSource(handle);
}

#ifdef DEVELOPER
int FindThing( const char * text, const char * tag, const char * markerBegin, const char * markerEnd, const char * thingname, const char ** begin, const char ** end )
{
	const char * def = 0;
	int braces=0;
	int found;
	int brace=0;
	const char * a = 0;
	const char * b = 0;
	char * t = 0;
	const char * last = 0;

	for ( found=0; !(found && brace==braces); )
	{
		last = text;
		t = COM_ParseExt( &text, qtrue );

		if ( t[ 0 ] == '\0' )
			break;

		if ( !Q_stricmp( t, tag ) )
		{
			t = COM_ParseExt( &text, qtrue );
			if ( t[ 0 ] != '{' )
				return 0;

			def		= text;
			brace	= ++braces;

			if ( !thingname ) // no name take first one
				found = 1;
		}

		if ( t[ 0 ] == '{' )
			braces++;

		if ( t[ 0 ] == '}' )
			braces--;

		if ( braces == brace+1 && !Q_stricmp( t, "name" ) )
		{
			t = COM_ParseExt( &text, qtrue );
			if ( !Q_stricmp( t, thingname ) )
				found = 1;
		}

		if ( found && thingname && !Q_stricmp( t, "itemDef" ) )
			break;
	}

	if ( found )
	{
		a = Q_strstr( def, text-def, markerBegin );
		b = Q_strstr( def, text-def, markerEnd );

		if ( a < def || a > text )
			a = last;

		if ( b >= def && b < text )
		{
			b += strlen( markerEnd );

		} else
			b = last;
			
		*begin = a;
		*end = b;

		return 1;
	}

	return 0;
}

static void writef( fileHandle_t f, const char * t ) {
	trap_FS_Write( t, strlen(t), f );
}

static void writef_rect( fileHandle_t f, const char * name, rectDef_t * r ) {
	writef( f, va("%s\t\t%d %d %d %d\n", name,	(int)r->x, (int)r->y, (int)r->w, (int)r->h ) );
}

static void writef_int( fileHandle_t f, const char * name, int i ) {
	writef( f, va("%s\t\t%d\n", name, i ) );
}


void Save_Menu( menuDef_t * menu, const char * filename )
{
	char buffer[ 65535 * 4 ];
	fileHandle_t f;
	const char * start; 
	const char * end;
	int i,len;
	const char * search;
	const char * menuMarkerBegin	= "\n\t\t//--editor.menu.begin--\n";
	const char * menuMarkerEnd		= "\t\t//--editor.menu.end--\n";
	const char * itemMarkerBegin	= "\n\t\t\t//--editor.item.begin--\n";
	const char * itemMarkerEnd		= "\t\t\t//--editor.item.end--\n";
	float x = uiInfo.uiDC.glconfig.xbias / uiInfo.uiDC.glconfig.xscale;

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( !f ) {
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return;
	}
	if ( len >= sizeof(buffer) ) {
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", filename, len, sizeof(buffer) ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buffer, len, f );
	buffer[len] = 0;
	if ( !FindThing( buffer, "menuDef", menuMarkerBegin, menuMarkerEnd, menu->window.name, &start, &end ) )
		return;

	trap_FS_FCloseFile( f );
	trap_FS_FOpenFile( filename, &f, FS_UPDATE );

	//	write everything previous to menu
	trap_FS_Write( buffer, start-buffer, f );

	//	write menu
	writef( f, menuMarkerBegin );
	menu->window.rectClient = menu->window.rect;

	if ( menu->window.flags & WINDOW_ALIGNLEFT ) {
		menu->window.rectClient.x += x;
	} else if ( menu->window.flags & WINDOW_ALIGNRIGHT ) {
		menu->window.rectClient.x -= x;
	} else if ( menu->window.flags & WINDOW_ALIGNWIDTH ) {
		float right = (menu->window.rectClient.x + menu->window.rectClient.w) - x*2.0f;
		menu->window.rectClient.x += x;
		menu->window.rectClient.w = right - menu->window.rectClient.x;
	}

	writef_rect( f, "\t\trect", &menu->window.rectClient );
	writef_int( f, "\t\tstyle", menu->window.style );
	writef( f, menuMarkerEnd );

	search = end;
	//	write items
	for ( i=0; i<menu->itemCount; i++ )
	{
		const char * itemBegin;
		const char * itemEnd;
		itemDef_t * item = menu->items[ i ];

		if ( !FindThing( search, "itemDef", itemMarkerBegin, itemMarkerEnd, 0, &itemBegin, &itemEnd ) )
		{
			trap_Print( va( S_COLOR_RED "couldn't finish saving menu file %s\n", filename ) );
			break;
		}

		//	write everything before item
		trap_FS_Write( search, itemBegin-search, f );

		writef( f, itemMarkerBegin );
		Save_Item( f, item );
		writef( f, itemMarkerEnd );

		search = itemEnd;
	}

	//	write everything after menu
	trap_FS_Write( search, len-(search-buffer), f );

	trap_FS_FCloseFile( f );
}

#endif


qboolean Load_Menu(int handle) {
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (token.string[0] != '{') {
		return qfalse;
	}

	for( ; ; )
	{

		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;
    
		if ( token.string[0] == 0 ) {
			return qfalse;
		}

		if ( token.string[0] == '}' ) {
			return qtrue;
		}

		UI_ParseMenu(token.string); 
	}
}

void UI_LoadMenus(const char *menuFile, qboolean reset) {
	pc_token_t token;
	int handle;
	int start;

	start = trap_Milliseconds();

	handle = trap_PC_LoadSource( menuFile );
	if (!handle) {
		trap_Error( ERR_DROP, va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );
		handle = trap_PC_LoadSource( "ui/menus.txt" );
		if (!handle) {
			trap_Error( ERR_DROP, va( S_COLOR_RED "default menu file not found: ui/menus.txt, unable to continue!\n", menuFile ) );
		}
	}

	ui_new.integer = 1;

	if (reset) {
		Menu_Reset();
	}

	for( ; ; )
	{
		if (!trap_PC_ReadToken(handle, &token))
			break;
		if( token.string[0] == 0 || token.string[0] == '}') {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "loadmenu") == 0) {
			if (Load_Menu(handle)) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_Printf("UI menu load time = %d milli seconds\n", trap_Milliseconds() - start);

	trap_PC_FreeSource( handle );
}


static void UI_DrawCrosshair( itemDef_t * item, vec4_t color )
{
	rectDef_t * rect = &item->window.rect;
	vec4_t 		hcolor;

	REF_PARAM( color );
	
	hcolor[0] = trap_Cvar_VariableValue("cg_crosshairColorR");
	hcolor[1] = trap_Cvar_VariableValue("cg_crosshairColorG");
	hcolor[2] = trap_Cvar_VariableValue("cg_crosshairColorB");
	hcolor[3] = trap_Cvar_VariableValue("cg_crosshairColorA");
	
 	trap_R_SetColor( hcolor );
	if (uiInfo.currentCrosshair < 0 || uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
		uiInfo.currentCrosshair = 0;
	}
	UI_DrawHandlePic( rect->x + item->textdivx + 8.0f, rect->y, rect->h, rect->h, uiInfo.uiDC.Assets.crosshairShader[uiInfo.currentCrosshair]);
 	trap_R_SetColor( NULL );
}


const char* UI_GetOwnerText( int id )
{
	if( id == 0 )
		return "";

	switch ( id )
	{
		case 0:					return 0;
		case BG_TOOLTIP:
			{
				int i;
				for ( i=ui_menuStack.count-1; i>=0; i-- )
				{
					menuDef_t * menu = ui_menuStack.m[ i ];
					if ( menu && menu->focusItem && menu->focusItem->tooltip ) {
						if ( sql( "SELECT index FROM strings SEARCH name $;", "t", menu->focusItem->tooltip ) ) {
							int tooltip = sqlint(0);
							sqldone();

							return UI_ST_getString( tooltip );
						}
					}
				}

				return 0;

			} break;
		case UI_KEYBINDSTATUS:			return (Display_KeyBindPending())?"Waiting for new key... Press ESCAPE to cancel":"Press ENTER or CLICK to change, Press BACKSPACE to clear";

		default:
			return UI_ST_getString( id );
	}
}

static void UI_OwnerDraw( itemDef_t * item, vec4_t color )
{
	switch( item->window.ownerDraw )
	{
	case UI_CROSSHAIR:
		UI_DrawCrosshair( item, color);
		break;
	default:
		UI_ST_exec( UI_ST_OWNERDRAW_PAINT, item );
		break;
	}

}

static qboolean UI_Crosshair_HandleKey( int flags, int key )
{
	REF_PARAM( flags );

	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		if (key == K_MOUSE2) {
			uiInfo.currentCrosshair--;
		} else {
			uiInfo.currentCrosshair++;
		}

		if (uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
			uiInfo.currentCrosshair = 0;
		} else if (uiInfo.currentCrosshair < 0) {
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}
		trap_Cvar_Set("cg_drawCrosshair", va("%d", uiInfo.currentCrosshair)); 
		return qtrue;
	}
	return qfalse;
}

/*
	UNREFERENCED LOCAL FUNCTION

static qboolean UI_NumericRange_HandleKey( int* val, float *special, int key,
										  int min, int max, int bigAmt )
{					
	if( key & K_CHAR_FLAG )
	{
		key &= ~K_CHAR_FLAG;

		if( key < '0' || key > '9' )
			return qfalse;

		if( *special == 0 )
		{
			*val = key - '0';
			*special = 1;
		}
		else
		{
			*val = atoi( va( "%i%c", *val, key ) );
		}
	}
	else
	{
		switch( key )
		{
		case K_BACKSPACE:
			if( *special != 0 )
			{
				char *s = va( "%i", *val );
				int l = strlen( s );

				if( l )
				{
					s[l - 1] = '\0';

					if( l == 1 )
						*special = 0;
				}

				*val = atoi( s );
			}
			break;

		case K_MWHEELUP:
		case K_UPARROW:
			(*val)++;
			*special = 0;
			break;

		case K_MWHEELDOWN:
		case K_DOWNARROW:
			(*val)--;
			*special = 0;
			break;

		case K_RIGHTARROW:
			*val += bigAmt;
			*special = 0;
			break;

		case K_LEFTARROW:
			*val -= bigAmt;
			*special = 0;
			break;

		case K_HOME:
			*val = min;
			*special = 0;
			break;

		case K_END:
			*val = max;
			*special = 0;
			break;

		default:
			return qfalse;
		}
	}

	return qtrue;
}
*/

static qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, int key)
{
	switch (ownerDraw)
	{
	case UI_CROSSHAIR:
		UI_Crosshair_HandleKey(flags, key);
		break;
	}

	return qfalse;
}

static qboolean UI_OwnerDrawHandleMouseMove( itemDef_t * item, float x, float y )
{
	switch( item->window.ownerDraw )
	{
	default:
		UI_ST_exec( UI_ST_OWNERDRAW_MOUSEMOVE, item, x,y );
		break;
	}

	return qfalse;
}

static bool Validate_Name( const char *s )
{
	if( !Info_Validate( s ) )
		return false;

	for( ; *s; s++ )
	{
		switch( *s )
		{
		case '\\':
			return false;
		}
	}

	return true;
}

static void UI_Login() {

	int logged_in = trap_Cvar_VariableInt( "ui_logged_in" );

	if ( logged_in != -2 && logged_in != 1 ) {

		char username[MAX_NAME_LENGTH];
		char password[MAX_NAME_LENGTH];
		
		trap_Cvar_VariableStringBuffer("ui_username", username, MAX_NAME_LENGTH);
		trap_Cvar_VariableStringBuffer("ui_password", password, MAX_NAME_LENGTH);

		if ( username[0] == '\0' || password[ 0 ] == '\0' )
		{
			trap_Cvar_Set( "ui_logged_in", "0" );
		} else
		{
			trap_Cvar_Set( "ui_logged_in", "-2" );
			trap_Cmd_ExecuteText( EXEC_APPEND, va("login \"%s\" \"%s\";\n", username, password) );
		}
	}
}

static void UI_Update( const char *name )
{
	int	val = (int)trap_Cvar_VariableValue( name );

	if (	name[ 0 ] == 'u' &&
			name[ 1 ] == 'i' &&
			name[ 2 ] == '_' )
	{
		switch( SWITCHSTRING( name+3 ) )
		{
			//	ui_sethost
		case CS('s','e','t','h'):
			{
				if (Info_Validate( UI_Cvar_VariableString( "ui_hostname" ) ) ) {
					trap_Cvar_Set( "sv_hostname", UI_Cvar_VariableString( "ui_hostname" ) );
				}
			} break;

			//	ui_gethost
		case CS('g','e','t','h'):
			{
				trap_Cvar_Set( "ui_hostname", UI_Cvar_VariableString("sv_hostname"));
			} break;

			//	ui_SetName
		case CS('S','e','t','N'):
			{
				if( Validate_Name( UI_Cvar_VariableString( "ui_Name" ) ) )
				{
					trap_Cvar_Set( "name", UI_Cvar_VariableString( "ui_Name" ) );
				}
				else
				{
					//keep name and ui_name in sync
					trap_Cvar_Set( "ui_Name", UI_Cvar_VariableString( "name" ) );
				}
			} break;

			//	ui_setRate
		case CS('s','e','t','r'):
			{
				float rate = trap_Cvar_VariableValue( "rate" );
				if( rate >= 5000 )
				{
					trap_Cvar_Set( "cl_maxpackets", "30" );
					trap_Cvar_Set( "cl_packetdup", "1" );
				}
				else if( rate >= 4000 )
				{
					trap_Cvar_Set( "cl_maxpackets", "15" );
					trap_Cvar_Set( "cl_packetdup", "2" );		// favor less prediction errors when there's packet loss
				}
				else
				{
					trap_Cvar_Set( "cl_maxpackets", "15" );
					trap_Cvar_Set( "cl_packetdup", "1" );		// favor lower bandwidth
				}
			} break;

			//	ui_GetName
		case CS('g','e','t','n'):
			{
				trap_Cvar_Set( "ui_Name", UI_Cvar_VariableString("name"));
			} break;

			//	ui_glCustom
		case CS('g','l','c','u'):
			{
				switch( val )
				{
					case 0:	// high quality
						trap_Cvar_SetValue( "r_fullScreen", 1 );
						trap_Cvar_SetValue( "r_subdivisions", 4 );
						trap_Cvar_SetValue( "r_vertexlight", 0 );
						trap_Cvar_SetValue( "r_lodbias", 0 );
						trap_Cvar_SetValue( "r_colorbits", 32 );
						trap_Cvar_SetValue( "r_depthbits", 24 );
						trap_Cvar_SetValue( "r_picmip", 0 );
						trap_Cvar_SetValue( "r_mode", 5 );
						trap_Cvar_SetValue( "r_fastSky", 0 );
						trap_Cvar_SetValue( "r_inGameVideo", 1 );
						trap_Cvar_SetValue( "cg_shadows", 1 );
						trap_Cvar_SetValue( "cg_brassTime", 2500 );
						trap_Cvar_Set( "r_texturemode", "LinearMipLinear" );
					break;
					case 1: // normal 
						trap_Cvar_SetValue( "r_fullScreen", 1 );
						trap_Cvar_SetValue( "r_subdivisions", 12 );
						trap_Cvar_SetValue( "r_vertexlight", 0 );
						trap_Cvar_SetValue( "r_lodbias", 0 );
						trap_Cvar_SetValue( "r_colorbits", 0 );
						trap_Cvar_SetValue( "r_depthbits", 24 );
						trap_Cvar_SetValue( "r_picmip", 1 );
						trap_Cvar_SetValue( "r_mode", 4 );
						trap_Cvar_SetValue( "r_fastSky", 0 );
						trap_Cvar_SetValue( "r_inGameVideo", 1 );
						trap_Cvar_SetValue( "cg_brassTime", 2500 );
						trap_Cvar_Set( "r_texturemode", "LinearMipLinear" );
						trap_Cvar_SetValue( "cg_shadows", 0 );
					break;
					case 2: // fast
						trap_Cvar_SetValue( "r_fullScreen", 1 );
						trap_Cvar_SetValue( "r_subdivisions", 8 );
						trap_Cvar_SetValue( "r_vertexlight", 0 );
						trap_Cvar_SetValue( "r_lodbias", 1 );
						trap_Cvar_SetValue( "r_colorbits", 0 );
						trap_Cvar_SetValue( "r_depthbits", 0 );
						trap_Cvar_SetValue( "r_picmip", 1 );
						trap_Cvar_SetValue( "r_mode", 3 );
						trap_Cvar_SetValue( "cg_shadows", 0 );
						trap_Cvar_SetValue( "r_fastSky", 1 );
						trap_Cvar_SetValue( "r_inGameVideo", 0 );
						trap_Cvar_SetValue( "cg_brassTime", 0 );
						trap_Cvar_Set( "r_texturemode", "LinearMipLinear" );
						trap_Cvar_Set( "r_fsaa", "0");
					break;
					case 3: // fastest
						trap_Cvar_SetValue( "r_fullScreen", 1 );
						trap_Cvar_SetValue( "r_subdivisions", 20 );
						trap_Cvar_SetValue( "r_vertexlight", 1 );
						trap_Cvar_SetValue( "r_lodbias", 2 );
						trap_Cvar_SetValue( "r_colorbits", 16 );
						trap_Cvar_SetValue( "r_depthbits", 16 );
						trap_Cvar_SetValue( "r_mode", 3 );
						trap_Cvar_SetValue( "r_picmip", 2 );
						trap_Cvar_SetValue( "cg_shadows", 0 );
						trap_Cvar_SetValue( "cg_brassTime", 0 );
						trap_Cvar_SetValue( "r_fastSky", 1 );
						trap_Cvar_SetValue( "r_inGameVideo", 0 );
						trap_Cvar_Set( "r_texturemode", "LinearMipNearest" );
						trap_Cvar_Set( "r_fsaa", "0");
					break;
				}
			} break;

			//	ui_mousePitch
		case CS('m','o','u','s'):
			{
				trap_Cvar_SetValue( "m_pitch", val ? -0.022F : 0.022F );
			} break;
		}

	} else if (	name[ 0 ] == 'r' &&
				name[ 1 ] == '_' )
	{
		switch( SWITCHSTRING( name+2 ) )
		{
			//	r_colorbits
		case CS('c','o','l','o'):
			{
				switch( val )
				{
					case 0:
						trap_Cvar_SetValue( "r_depthbits", 0 );
						trap_Cvar_SetValue( "r_stencilbits", 0 );
						break;

					case 16:
						trap_Cvar_SetValue( "r_depthbits", 16 );
						trap_Cvar_SetValue( "r_stencilbits", 0 );
						break;

					default:
						trap_Cvar_SetValue( "r_depthbits", 24 );
						break;

				}
			} break;
			//	r_lodbias
		case CS('l','o','d','b'):
			{
				switch( val )
				{
					case 0:
						trap_Cvar_SetValue( "r_subdivisions", 4 );
					break;
					case 1:
						trap_Cvar_SetValue( "r_subdivisions", 12 );
					break;
					case 2:
						trap_Cvar_SetValue( "r_subdivisions", 20 );
					break;
				}
			} break;
		}
	}
}

static void UI_RunMenuScript( const char **args )
{
	const char *name, *name2;

	if( String_Parse( args, &name ) )
	{
		if ( name[ 0 ] == 's' && name[ 1 ] == 't' && name[ 2 ] == '_' )
		{
			//	menu script commands prefixed with 'st_' are destined for rules engine...
			UI_ST_exec( UI_ST_SCRIPTCOMMAND, name+3, args );
			return;

		}
		else if( Q_stricmp( name, "resetDefaults" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
			trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
			Controls_SetDefaults();
			trap_Cvar_Set("com_introPlayed", "1" );
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
		}
		else if( Q_stricmp( name, "saveControls" ) == 0 )
		{
			Controls_SetConfig(qtrue);
		}
		else if( Q_stricmp( name, "loadControls" ) == 0 )
		{
			Controls_GetConfig();
		}
		else if( Q_stricmp( name, "clearError" ) == 0 )
		{
			trap_Cvar_Set("com_errorMessage", "");
		}
		else if( Q_stricmp( name, "RefreshServers" ) == 0 )
		{
			UI_StartServerRefresh();
		}
		else if( Q_stricmp( name, "RefreshFilter" ) == 0 )
		{
			UI_StartServerRefresh();
		}
		else if( Q_stricmp( name, "StopRefresh" ) == 0 )
		{
			UI_StopServerRefresh();
		}
		else if( Q_stricmp( name, "UpdateFilter" ) == 0 )
		{
			if (ui_netSource.integer == AS_LOCAL) {
				UI_StartServerRefresh();
			}
		}
		else if( Q_stricmp( name, "JoinServer" ) == 0 )
		{
			if ( sql( "SELECT addr FROM servers WHERE #=feeders.name[ 'serverlist' ].id;", 0 ) ) {

				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", sqltext(0) ) );
				sqldone();
			}
		}
		else if( Q_stricmp( name, "Quit" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_NOW, "quit" );
		}
		else if( Q_stricmp( name, "Leave" ) == 0 )
		{
			if ( trap_Cvar_VariableInt("sv_running") == 0) {
				trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
			} else {
				trap_Cmd_ExecuteText( EXEC_APPEND, "killserver\n");

			}
			Menus_CloseAll();
		}
		else if( Q_stricmp( name, "glCustom" ) == 0 )
		{
			trap_Cvar_Set( "ui_glCustom", "4" );
		}
		else if( Q_stricmp( name, "update" ) == 0 )
		{
			if( String_Parse( args, &name2 ) )
				UI_Update( name2 );
		}
		else if( Q_stricmp( name, "setPbClStatus" ) == 0 )
		{
			int stat;
			if ( Int_Parse( args, &stat ) )
				trap_SetPbClStatus( stat );
		}
		else if ( Q_stricmp( name, "gethighscores") == 0 )
		{
			UI_ST_exec( UI_ST_REQUEST_HIGHSCORES );
		}
		else if( Q_stricmp( name, "login" ) == 0 )
		{
			UI_Login();
		}
		else if( Q_stricmp( name, "closebugreport" ) == 0 )
		{
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_BUG );
		}
		else if ( Q_stricmp( name, "registergame" ) == 0 )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va("register %s\n", UI_Cvar_VariableString( "ui_licensekey" ) ) );
		}
		else if ( Q_stricmp( name, "checkupdates" ) == 0 )
		{
			trap_Cvar_Set( "com_changelog", "Space Trader is up to date." );
			trap_Cvar_Set( "com_downloadbar_x", COM_Parse( args ) );
			trap_Cvar_Set( "com_downloadbar_y", COM_Parse( args ) );
			trap_Cvar_Set( "com_downloadbar_w", COM_Parse( args ) );
			trap_Cvar_Set( "com_downloadbar_h", COM_Parse( args ) );
			trap_Cmd_ExecuteText( EXEC_APPEND, "check_updates" );
		}
#ifdef DEVELOPER
		else if ( Q_stricmp( name, "editorupdate" ) == 0 )
		{
			UI_Editor_UpdateItem();
		}
#endif
		else
		{
			Com_Printf( "unknown UI script %s\n", name );
		}
	}
}

qboolean UI_hasSkinForBase(const char *base, const char *team) {
	char	test[1024];
	
	Com_sprintf( test, sizeof( test ), "models/players/%s/%s/lower_default.skin", base, team );

	if (trap_FS_FOpenFile(test, 0, FS_READ)) {
		return qtrue;
	}
	Com_sprintf( test, sizeof( test ), "models/players/characters/%s/%s/lower_default.skin", base, team );

	if (trap_FS_FOpenFile(test, 0, FS_READ)) {
		return qtrue;
	}
	return qfalse;
}

static int UI_PlayCinematic(const char *name, float x, float y, float w, float h) {
  return trap_CIN_PlayCinematic( name, (int)x, (int)y, (int)w, (int)h, (CIN_loop | CIN_silent) );
}

static void UI_StopCinematic(int handle) {
	if ( handle >= 0 )
	{
	  trap_CIN_StopCinematic(handle);
	}
}

static void UI_DrawCinematic( int handle, float x, float y, float w, float h )
{
	trap_CIN_SetExtents( handle, (int)x, (int)y, (int)w, (int)h );
	trap_CIN_DrawCinematic(handle);
}

static void UI_RunCinematicFrame( int handle )
{
	trap_CIN_RunCinematic( handle );
}


void UI_Cvar_Set( const char* cvar, const char* value )
{
	trap_Cvar_Set( cvar, value );
}

/*
=================
UI_Init
=================
*/
void _UI_Init( qboolean inGameLoad ) {
	int start;

	//uiInfo.inGameLoad = inGameLoad;

	UI_RegisterCvars();
	UI_InitMemory();
	UI_Effect_Init();

	// cache redundant calulations
	trap_GetGlconfig( &uiInfo.uiDC.glconfig );


  //UI_Load();
	uiInfo.uiDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	uiInfo.uiDC.setColor = &trap_R_SetColor;
	uiInfo.uiDC.drawHandlePic = &UI_DrawHandlePic;
	uiInfo.uiDC.drawStretchPic = &trap_R_DrawStretchPic;
	uiInfo.uiDC.registerModel = &trap_R_RegisterModel;
	uiInfo.uiDC.modelBounds = &trap_R_ModelBounds;
	uiInfo.uiDC.fillRect = &trap_R_RenderRoundRect;
	uiInfo.uiDC.drawRect = &_UI_DrawRect;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.drawTopBottom = &_UI_DrawTopBottom;
	uiInfo.uiDC.clearScene = &trap_R_ClearScene;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene = &trap_R_RenderScene;
	uiInfo.uiDC.registerFont = &trap_R_RegisterFont;
	uiInfo.uiDC.ownerDrawItem = &UI_OwnerDraw;
	uiInfo.uiDC.runScript = &UI_RunMenuScript;
	uiInfo.uiDC.setCVar = UI_Cvar_Set;
	uiInfo.uiDC.getCVarString = trap_Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue = trap_Cvar_VariableValue;
	uiInfo.uiDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound = &trap_S_StartLocalSound;
	uiInfo.uiDC.ownerDrawHandleKey = &UI_OwnerDrawHandleKey;
	uiInfo.uiDC.ownerDrawHandleMouseMove = &UI_OwnerDrawHandleMouseMove;
	uiInfo.uiDC.setBinding = &trap_Key_SetBinding;
	uiInfo.uiDC.getBindingBuf = &trap_Key_GetBindingBuf;
	uiInfo.uiDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	uiInfo.uiDC.executeText = &trap_Cmd_ExecuteText;
	uiInfo.uiDC.Error = &Com_Error; 
	uiInfo.uiDC.Print = &Com_Printf; 
	uiInfo.uiDC.registerSound = &trap_S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	uiInfo.uiDC.playCinematic = &UI_PlayCinematic;
	uiInfo.uiDC.stopCinematic = &UI_StopCinematic;
	uiInfo.uiDC.drawCinematic = &UI_DrawCinematic;
	uiInfo.uiDC.runCinematicFrame = &UI_RunCinematicFrame;
	uiInfo.uiDC.getOwnerText = &UI_GetOwnerText;
	uiInfo.uiDC.getClientNum = &trap_GetClientNum;
	uiInfo.uiDC.toolTip = &UI_Effects_UpdateToolTip;

	Init_Display(&uiInfo.uiDC);

	memset( &ui_menuStack, 0, sizeof(ui_menuStack) );
	String_Init();
  
	uiInfo.uiDC.whiteShader = trap_R_RegisterShaderNoMip( "*white" );

	AssetCache();

	start = trap_Milliseconds();

    uiInfo.traveltime = 0;
	uiInfo.maxplayers = 8;

	{
		char tmp[ MAX_INFO_STRING ];
		trap_GetConfigString( CS_SERVERINFO, tmp, sizeof(tmp) );
		uiInfo.maxplayers = atoi( Info_ValueForKey( tmp, "sv_maxplayers" ) );
	}

	trap_SQL_Exec(
			"CREATE TABLE feeders"
			"("
				"name	STRING, "
				"sel	INTEGER, "
				"id		INTEGER, "
				"enabled INTEGER "
			");"

			"CREATE TABLE sliders"
			"("
				"name	STRING, "
				"value	INTEGER "
			");"

			"CREATE TABLE IF NOT EXISTS strings"
			"("
				"index	INTEGER, "
				"name	STRING "
			");"

			"CREATE TABLE ui"
			"("
				"key	STRING, "
				"value	INTEGER "
			");"

			);



#ifdef USE_DRM
	trap_PC_AddGlobalDefine( "USE_DRM" );
#endif
#ifdef DEMO
	trap_PC_AddGlobalDefine( "DEMO" );
#endif

	//	check to see if 'cl_lang' is valid, default to english.
	if ( sql( "SELECT 1 FROM common.files SEARCH fullname 'ui/strings_'||$1||'.txt';", "t", UI_Cvar_VariableString("cl_lang") ) ) {
		sqldone();
	} else {
		UI_Cvar_Set( "cl_lang", "en-US" );
	}

	trap_PC_AddGlobalDefine( va("LANG \"%s\"",UI_Cvar_VariableString("cl_lang")) ); 

	{
		char tmp[ 64 ];
		trap_SQL_Run( tmp, sizeof(tmp), 0, 0,0,0 );
		if ( tmp[ 0 ] == '\0' ) {
			trap_SQL_Compile( "1" );
			UI_ParseStrings( va("ui/strings_%s.txt",UI_Cvar_VariableString("cl_lang")) );
		}
	}

	if ( inGameLoad )
		UI_LoadMenus( "ui/ingame.txt", qtrue );
	else
		UI_LoadMenus( "ui/menus.txt", qtrue);
	
	uiInfo.tooltipDef1 = UI_Effect_Find( "tooltip1" );
	uiInfo.tooltipDef2 = UI_Effect_Find( "tooltip2" );

#ifdef DEVELOPER
	UI_Editor_Init();
#endif

	Menus_CloseAll();

	// initialize rules engine
	if ( !inGameLoad )
		UI_ST_exec( UI_ST_INIT_FRONTEND );

	UI_LoadBots();

	// sets defaults for ui temp cvars
	uiInfo.currentCrosshair = (int)trap_Cvar_VariableValue("cg_drawCrosshair");
	trap_Cvar_Set("ui_mousePitch", (trap_Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");

	uiInfo.serverStatus.currentServerCinematic = -1;

	trap_Cvar_Register(NULL, "debug_protocol", "", 0 );

	uiInfo.missionStatus = 0;	// the mission is in progress
	sql( "SELECT index FROM strings SEARCH name $;", "tsId", "T_Yes", &uiInfo.T_Yes );
	sql( "SELECT index FROM strings SEARCH name $;", "tsId", "T_No", &uiInfo.T_No );

	trap_Key_SetCatcher( 0 );
}


/*
=================
UI_KeyEvent
=================
*/
void _UI_KeyEvent( int key, qboolean down )
{
	Display_HandleKey( key, down, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory );

	if ( !Menu_GetFocused() )
	{
		trap_Key_ClearStates();
		trap_Cvar_Set( "cl_paused", "0" );
	}
}


/*
=================
UI_MouseEvent
=================
*/
int _UI_MouseEvent( int dx, int dy )
{
	UI_AdjustTo640( &dx, &dy );

	// update mouse screen position
	uiInfo.uiDC.cursorx = dx;
	uiInfo.uiDC.cursory = dy;

	return Display_MouseMove( uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory );
}

void _UI_SetActiveMenu( uiMenuCommand_t menu )
{
	char buf[256];

	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	if( Menu_Count() > 0 )
	{
		vec3_t v;
		v[0] = v[1] = v[2] = 0;
		switch( menu )
		{
		case UIMENU_NONE:
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();

			return;

		case UIMENU_MAIN:

			UI_SetMenuStack( &cg_menuStack );
			Menus_CloseAll();
			UI_SetMenuStack( &ui_menuStack );
			Menus_CloseAll();

			if ( !GS_INPLAY && trap_Cvar_VariableInt( "com_downloading" ) == 1 ) {
				Menus_ActivateByName( "downloading" );
			} else {

				Menus_ActivateByName( "welcome" );

				UI_ST_exec( UI_ST_MAINMENU );
			}

			trap_Cvar_VariableStringBuffer( "com_errorMessage", buf, sizeof( buf ) );
			if( strlen( buf ) )
			{
				if( Q_stricmp( "Disconnected from server", buf ) == 0 )
				{
					trap_Cvar_Set( "com_errorMessage", "" );
				}
				else
				{
					trap_Cvar_Set( "ui_errorMessage", buf );
					Menus_ActivateByName( "error_popmenu" );
				}
			}
			return;
		
		case UIMENU_TEAM:
			Menus_ActivateByName("team");
			return;
		
		case UIMENU_NEED_CD:
			Menus_ActivateByName( "header" );
			Menus_ActivateByName( "registergame" );
			return;
		
		case UIMENU_BAD_CD_KEY:
			return;

		case UIMENU_INGAME:
			UI_ST_exec( UI_ST_INGAMEMENU );
			return;

		case UIMENU_CHAT:
			UI_ST_exec( UI_ST_CHATMENU );
			return;

		}
	}
}

qboolean _UI_IsFullscreen( void ) {
	return Menus_AnyFullScreenVisible();
}


/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
void UI_DrawConnectScreen( qboolean overlay )
{
	char text[ MAX_INFO_STRING ];
	uiClientState_t cstate;
	rectDef_t r;

	REF_PARAM( overlay );

	//
	// see what information we should display
	//
	trap_GetClientState( &cstate );
	text[0] = '\0';

	if (!Q_stricmp(cstate.servername,"localhost"))
	{
		Q_strcat( text, sizeof(text), "Loading...\n" );

	} else {
		Q_strcat( text, sizeof(text), va("Connecting to %s\n", cstate.servername));

		switch ( cstate.connState )
		{
			case CA_CONNECTING:		Q_strcat( text, sizeof(text), va("Awaiting connection...%i\n", cstate.connectPacketCount) );	break;
			case CA_CHALLENGING:	Q_strcat( text, sizeof(text), va("Awaiting challenge...%i\n", cstate.connectPacketCount) );	break;
			case CA_CONNECTED:		Q_strcat( text, sizeof(text), "Awaiting gamestate...\n" ); break;
			case CA_UNINITIALIZED:
			case CA_DISCONNECTED:
			case CA_AUTHORIZING:
			case CA_LOADING:
			case CA_PRIMED:
			case CA_ACTIVE:
			case CA_CINEMATIC:
				break;
		}
	}

	r.x = 50.0f;
	r.y = 0.0f;
	r.w = 540.0f;
	r.h = 450.0f;
	trap_R_RenderText	(	&r,
							0.35f,
							colorWhite,
							text,
							sizeof(text),
							TEXT_ALIGN_RIGHT,
							TEXT_ALIGN_RIGHT,
							ITEM_TEXTSTYLE_SHADOWED | ITEM_TEXTSTYLE_ITALIC,
							uiInfo.uiDC.Assets.font,
							-1,
							0
						);

	// password required / connection rejected information goes here
}


/*
================
cvars
================
*/

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

vmCvar_t	ui_botsFile;

vmCvar_t	ui_brassTime;
vmCvar_t	ui_drawCrosshair;
vmCvar_t	ui_drawCrosshairNames;
vmCvar_t	ui_marks;

vmCvar_t	ui_gameType;
vmCvar_t	ui_netSource;
vmCvar_t	ui_serverFilterType;
vmCvar_t	ui_currentMap;
vmCvar_t	ui_currentNetMap;
vmCvar_t	ui_singlePlayerActive;
vmCvar_t	ui_username;
vmCvar_t	ui_password;
vmCvar_t	ui_crosshairColorR;
vmCvar_t	ui_crosshairColorG;
vmCvar_t	ui_crosshairColorB;
vmCvar_t	ui_crosshairColorA;

vmCvar_t	cl_cornersize;

// bk001129 - made static to avoid aliasing
static cvarTable_t		cvarTable[] = {
	{ &ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM },

	{ &ui_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE },
	{ &ui_drawCrosshair, "cg_drawCrosshair", "2", CVAR_ARCHIVE },
	{ &ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &ui_marks, "cg_marks", "1", CVAR_ARCHIVE },

	{ &ui_new, "ui_new", "0", CVAR_TEMP },
	{ &ui_debug, "ui_debug", "0", CVAR_TEMP },
	{ &ui_initialized, "ui_initialized", "0", CVAR_TEMP },
	{ &ui_spacetrader, "cl_spacetrader", "0", CVAR_TEMP },
	{ &ui_netSource, "ui_netSource", "0", CVAR_ARCHIVE },
	{ &ui_currentMap, "ui_currentMap", "0", CVAR_ARCHIVE },
	{ &ui_currentNetMap, "ui_currentNetMap", "0", CVAR_ARCHIVE },
	{ &ui_singlePlayerActive, "ui_singlePlayerActive", "0", 0},
	{ &ui_tooltipmode, "ui_tooltipmode", "0", CVAR_ARCHIVE},
	{ &ui_drawMenu, "ui_drawMenu", "1", CVAR_TEMP },
	{ &ui_username, "ui_username", "", CVAR_ARCHIVE},
	{ &ui_password, "ui_password", "", CVAR_ARCHIVE},
	{ &ui_crosshairColorR, "cg_crosshairColorR", "1.0", CVAR_ARCHIVE},
	{ &ui_crosshairColorG, "cg_crosshairColorG", "1.0", CVAR_ARCHIVE},
	{ &ui_crosshairColorB, "cg_crosshairColorB", "1.0", CVAR_ARCHIVE},
	{ &ui_crosshairColorA, "cg_crosshairColorA", "1.0", CVAR_ARCHIVE},
	{ &cl_cornersize, "cl_cornersize", "", 0},
};

// bk001129 - made static to avoid aliasing
static int		cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);


/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}
}


/*
=================
ArenaServers_StopRefresh
=================
*/
static void UI_StopServerRefresh( void )
{
	if (!uiInfo.serverStatus.refreshActive) {
		// not currently refreshing
		return;
	}
	uiInfo.serverStatus.refreshActive = qfalse;
}

/*
=================
UI_DoServerRefresh
=================
*/
static void UI_DoServerRefresh( void )
{
	if (!uiInfo.serverStatus.refreshActive) {
		return;
	}

	if (uiInfo.uiDC.realTime > uiInfo.serverStatus.nextDisplayRefresh ) {
		UI_StartServerRefresh();
		return;
	}

	if (uiInfo.uiDC.realTime < uiInfo.serverStatus.refreshtime) {
		return;
	}

	// if still trying to retrieve pings
	if (trap_LAN_UpdateVisiblePings(ui_netSource.integer)) {
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	}
}

/*
=================
UI_StartServerRefresh
=================
*/
static void UI_StartServerRefresh()
{
	qtime_t q;
	int delay;
	trap_RealTime(&q);
	trap_Cvar_Set( va("ui_lastServerRefresh_%i", ui_netSource.integer), va("%s-%i, %i at %i:%i", MonthAbbrev[q.tm_mon],q.tm_mday, 1900+q.tm_year,q.tm_hour,q.tm_min));

	//
	if( ui_netSource.integer == AS_LOCAL ) {
		// clear number of displayed servers
		sql( "DELETE FROM servers WHERE lastupdate < ?1;", "ie", uiInfo.uiDC.realTime-2000 );
		trap_Cmd_ExecuteText( EXEC_NOW, "localservers\n" );
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
		delay = 2500;
	} else {
		// clear number of displayed servers
		//sql( "DELETE FROM servers WHERE lastupdate < ?1;", "ie", uiInfo.uiDC.realTime-10000 );

		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;
		if( ui_netSource.integer == AS_GLOBAL ) {
			trap_Cmd_ExecuteText( EXEC_NOW, "getservers" );
		}
		delay = 10000;
	}
	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + delay;
}

void UI_Effects_UpdateToolTip( itemDef_t * item )
{
	item;
#if 0
	effectDef_t * e;

	//	kill old tooltip
	if ( uiInfo.tooltip )
		UI_Effect_SetFlag( uiInfo.tooltip, EF_NOFOCUS );

	if ( ui_tooltipmode.integer == 0 )
		return;

	e = (ui_tooltipmode.integer==2)?uiInfo.tooltipDef2:uiInfo.tooltipDef1;

	if ( item && item->tooltip && e )
	{
		char t[ EFFECT_TEXT_LENGTH ];
		rectDef_t r;
		r.x = e->rect.x + (float)uiInfo.uiDC.cursorx;
		r.y = e->rect.x + (float)uiInfo.uiDC.cursory;
		r.w = e->rect.w;
		r.h = e->rect.h;
			
		trap_SQL_Run( t, sizeof(t), item->tooltip, 0,0,0 );
		uiInfo.tooltip = UI_Effect_SpawnText( 0, e, t, 0 );
		UI_Effect_SetRect( uiInfo.tooltip, &r, item );
	}
#endif
}
