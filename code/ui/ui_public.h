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
//
#ifndef __UI_PUBLIC_H__
#define __UI_PUBLIC_H__

#define UI_API_VERSION	7

typedef struct {
	connstate_t		connState;
	int				connectPacketCount;
	int				clientNum;
	char			servername[MAX_STRING_CHARS];
	char			updateInfoString[MAX_STRING_CHARS];
	char			messageString[MAX_STRING_CHARS];
} uiClientState_t;

typedef struct radarFilter_t
{
	int				type;
	int				offset_to_pos;
} radarFilter_t;

typedef struct radarInfo_t
{
	float			r;			// [0, 1] normalized into range
	float			theta;		// 0 = in front, + = clockwise

	entityState_t	ent;
} radarInfo_t;

typedef enum {
	UI_ERROR,
	UI_PRINT,
	UI_MILLISECONDS,
	UI_CVAR_SET,
	UI_CVAR_VARIABLEVALUE,
	UI_CVAR_VARIABLESTRINGBUFFER,
	UI_CVAR_VARIABLEINT,
	UI_CVAR_SETVALUE,
	UI_CVAR_RESET,
	UI_CVAR_CREATE,
	UI_CVAR_INFOSTRINGBUFFER,
	UI_ARGC,
	UI_ARGV,
	UI_ARGVI,
	UI_CMD_EXECUTETEXT,
	UI_FS_FOPENFILE,
	UI_FS_READ,
	UI_FS_WRITE,
	UI_FS_FCLOSEFILE,
	UI_FS_GETFILELIST,
	UI_R_REGISTERMODEL,
	UI_R_REGISTERSKIN,
	UI_R_REGISTERSHADER,
	UI_R_REGISTERSHADERNOMIP,
	UI_R_CLEARSCENE,
	UI_R_BUILDPOSE,
	UI_R_BUILDPOSE2,
	UI_R_BUILDPOSE3,
	UI_R_SHAPECREATE,
	UI_R_SHAPEDRAW,
	UI_R_GRAPHDRAW,
	UI_R_SHAPEDRAWMULTI,
	UI_R_ADDREFENTITYTOSCENE,
	UI_R_ADDPOLYTOSCENE,
	UI_R_ADDLIGHTTOSCENE,
	UI_R_RENDERSCENE,
	UI_R_SETCOLOR,
	UI_R_DRAWSTRETCHPIC,
	UI_R_DRAWSPRITE,
	UI_R_RENDERTEXT,
	UI_R_FLOWTEXT,
	UI_R_GETFONT,
	UI_R_ROUNDRECT,
	UI_UPDATESCREEN,
	UI_CM_LERPTAG,
	UI_CM_LOADMODEL,
	UI_S_REGISTERSOUND,
	UI_S_STARTLOCALSOUND,
	UI_KEY_KEYNUMTOSTRINGBUF,
	UI_KEY_GETBINDINGBUF,
	UI_KEY_SETBINDING,
	UI_KEY_ISDOWN,
	UI_KEY_GETOVERSTRIKEMODE,
	UI_KEY_SETOVERSTRIKEMODE,
	UI_KEY_CLEARSTATES,
	UI_KEY_GETCATCHER,
	UI_KEY_SETCATCHER,
	UI_KEY_GETKEY,
	UI_GETCLIPBOARDDATA,
	UI_GETGLCONFIG,
	UI_GETCLIENTSTATE,
	UI_GETCLIENTNUM,
	UI_GETCONFIGSTRING,
	UI_UPDATEGAMESTATE,
	_UI_LAN_GETPINGQUEUECOUNT,			//	unused
	_UI_LAN_CLEARPING,					//	unused
	_UI_LAN_GETPING,
	_UI_LAN_GETPINGINFO,
	UI_CVAR_REGISTER,
	UI_CVAR_UPDATE,
	UI_MEMORY_REMAINING,
	UI_R_REGISTERFONT,
	UI_R_GETFONTS,
	UI_R_MODELBOUNDS,
	UI_PC_ADD_GLOBAL_DEFINE,
	UI_PC_LOAD_SOURCE,
	UI_PC_FREE_SOURCE,
	UI_PC_READ_TOKEN,
	UI_PC_SOURCE_FILE_AND_LINE,
	UI_S_STOPBACKGROUNDTRACK,
	UI_S_STARTBACKGROUNDTRACK,
	UI_REAL_TIME,
	_UI_LAN_GETSERVERADDRESSSTRING,		//	unused
	_UI_LAN_GETSERVERINFO,				//	unused
	_UI_LAN_MARKSERVERVISIBLE,
	UI_LAN_UPDATEVISIBLEPINGS,
	UI_CIN_PLAYCINEMATIC,
	UI_CIN_STOPCINEMATIC,
	UI_CIN_RUNCINEMATIC,
	UI_CIN_DRAWCINEMATIC,
	UI_CIN_SETEXTENTS,
	UI_R_REMAP_SHADER,
	UI_LAN_SERVERSTATUS,
	UI_LAN_GETSERVERPING,				//	unused
	_UI_LAN_SERVERISVISIBLE,
	UI_LAN_COMPARESERVERS,				//	unused
	UI_GET_RADAR,
	// 1.32
	UI_FS_SEEK,
	UI_SET_PBCLSTATUS,

	UI_CON_PRINT,
	UI_CON_DRAW,
	UI_CON_HANDLEMOUSE,
	UI_CON_HANDLEKEY,
	UI_CON_CLEAR,
	UI_CON_RESIZE,
	UI_CON_GETTEXT,

	UI_MEMSET,
	UI_MEMCPY,
	UI_STRNCPY,
	UI_SIN,
	UI_COS,
	UI_ATAN2,
	UI_SQRT,
	UI_FLOOR,
	UI_CEIL,
	UI_ACOS,
	UI_LOG,
	UI_POW,
	UI_ATAN,
	UI_FMOD,
	UI_TAN,

	UI_Q_rand = 500,

	UI_SQL_LOADDB = 600,
	UI_SQL_EXEC,
	UI_SQL_PREPARE,
	UI_SQL_BIND,
	UI_SQL_BINDTEXT,
	UI_SQL_BINDINT,
	UI_SQL_BINDARGS,
	UI_SQL_STEP,
	UI_SQL_COLUMNCOUNT,
	UI_SQL_COLUMNASTEXT,
	UI_SQL_COLUMNASINT,
	UI_SQL_COLUMNNAME,
	UI_SQL_DONE,
	UI_SQL_SELECT,
	UI_SQL_COMPILE,
	UI_SQL_RUN,

	UI_SQL_SV_SELECT,


} uiImport_t;

typedef enum uiMenuCommand_t
{
	UIMENU_NONE,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_CHAT,
	UIMENU_NEED_CD,
	UIMENU_BAD_CD_KEY,
	UIMENU_TEAM,
} uiMenuCommand_t;

typedef enum {
	UI_GETAPIVERSION = 0,	// system reserved

	UI_INIT,
	UI_GAMEINIT,
//	void	UI_Init( void );

	UI_SHUTDOWN,
//	void	UI_Shutdown( void );

	UI_KEY_EVENT,
//	void	UI_KeyEvent( int gui, int key );

	UI_MOUSE_EVENT,
//	void	UI_MouseEvent( int gui, int dx, int dy );

	UI_REFRESH,
//	void	UI_Refresh( int gui, int time );

	UI_IS_FULLSCREEN,
//	qboolean UI_IsFullscreen( void );

	UI_SET_ACTIVE_MENU,
//	void	UI_SetActiveMenu( uiMenuCommand_t menu );

	UI_CONSOLE_COMMAND,
//	qboolean UI_ConsoleCommand( int realTime );

	UI_DRAW_CONNECT_SCREEN,
//	void	UI_DrawConnectScreen( qboolean overlay );

// if !overlay, the background will be drawn, otherwise it will be
// overlayed over whatever the cgame has drawn.
// a GetClientState syscall will be made to get the current strings

	// void UI_ReportHighScoreResponse( void );
	UI_REPORT_HIGHSCORE_RESPONSE,

//When the client has been authorized, send a response, and the token
//	void	UI_Authorized( int response )	
	UI_AUTHORIZED,

// when the client gets an error message from the server
	UI_SERVER_ERRORMESSAGE,


// sql is calling to determine the value of an indentifier
	UI_GLOBAL_INT,

} uiExport_t;

#endif
