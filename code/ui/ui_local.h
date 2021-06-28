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
#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#define Q_DO_NOT_DISABLE_USEFUL_WARNINGS

#include "../qcommon/q_shared.h"
#include "../renderer/tr_types.h"
#include "ui_public.h"
#include "keycodes.h"
#include "../game/bg_public.h"
#include "ui_shared.h"

// global display context

extern vmCvar_t	ui_botsFile;

extern vmCvar_t	ui_brassTime;
extern vmCvar_t	ui_drawCrosshair;
extern vmCvar_t	ui_drawCrosshairNames;
extern vmCvar_t	ui_marks;

extern vmCvar_t	ui_netSource;
extern vmCvar_t	ui_serverFilterType;
extern vmCvar_t	ui_currentMap;
extern vmCvar_t	ui_currentNetMap;
extern vmCvar_t	ui_singlePlayerActive;

extern vmCvar_t ui_spacetrader;

extern vmCvar_t	cl_cornersize;

//
// ui_qmenu.c
//

#define	MAX_EDIT_LINE			256

//
// ui_main.c
//
void UI_LoadMenus(const char *menuFile, qboolean reset);
void _UI_SetActiveMenu( uiMenuCommand_t menu );

//
//	ui_spacetrader.c
//

typedef enum
{
	UI_ST_INIT_FRONTEND,
	UI_ST_INIT_GAME,
	UI_ST_SHUTDOWN,
	UI_ST_MAINMENU,
	UI_ST_INGAMEMENU,
	UI_ST_CHATMENU,
	UI_ST_PAINT,
	UI_ST_REFRESH,
	UI_ST_OWNERDRAW_PAINT,
	UI_ST_OWNERDRAW_MOUSEMOVE,
	UI_ST_CONSOLECOMMAND,			//	process a command from the server
	UI_ST_SCRIPTCOMMAND,			//	process a script command from the menus
	UI_ST_CONFIGSTRING,
	UI_ST_HIGHSCORE_RESPONSE,		//  response to a highscore request
	UI_ST_REPORT_HIGHSCORE_RESPONSE,//  reponse to a highscore report
	UI_ST_AUTHORIZED,				//  we got a response to our authorize request
	UI_ST_REQUEST_HIGHSCORES,		// request highscores from the master server
	UI_ST_SERVER_ERRORMESSAGE,		// server got a error message it wants us to display

} spacetraderCmd_t;

extern int QDECL UI_ST_exec( int cmd, ... );

extern const char * UI_ST_getString( int id, ... );


//
//	ui_graph.c
//
#if 1
#define MAX_GRAPH_POINTS 256

typedef struct
{
	vec2_t		pts		[ MAX_GRAPH_POINTS ];
	vec2_t		uvs		[ MAX_GRAPH_POINTS ];
	vec4_t		colors	[ MAX_GRAPH_POINTS ];
	int			n;
	float		low, high;

} graphGen_t;

extern void UI_Graph_Init( graphGen_t * gg );
extern void UI_Graph_Plot( graphGen_t * gg, float p, vec4_t colorLow, vec4_t colorHigh );

extern qhandle_t UI_Chart_Find( int key );
extern qhandle_t UI_Chart_Create( int key, int start, int end, float low, float high );
extern void UI_Chart_AddGraph( qhandle_t h, graphGen_t * gg, shapeGen_t gen, qhandle_t shader );
extern void UI_Chart_AddLabel( qhandle_t h, int x, float y, const char * text );
extern void UI_Chart_Paint( itemDef_t * item, qhandle_t chart );
extern void UI_Graph_Line( graphGen_t * gg, int x1, int y1, int x2, int y2, vec4_t color );
#endif


typedef struct
{
	int				id;
	const char*		name;
	const char*		disaster_name;
    int             dfs;
    int             op;
	int				size;
    int             sa;
	int				time;
	int				event_id;
	void*			orbit;
	void*			course;
	qhandle_t		model;
	qhandle_t		shader;
	qhandle_t		atmShader;
	qhandle_t		icon;
	qhandle_t		overlay;
	qhandle_t		disaster_icon;
	qboolean		visible;
	qboolean		decoration;
	Rectangle		textRect;	// read only, this is a tight rect around the text. updated by paint

} planetInfo_t;

//
// ui_solarsystem.c
//
typedef struct
{
	/*
		Interpret this as follows:

		The eye is pointed at lookAt. Its location is the point (radius, 0, 0)
		rotated by direction and centered about lookAt.

		ArcBall camera control, anyone?
	*/

	vec3_t		lookAt;
	quat_t		direction;
	float		radius;

	float		fov;
} solarsystemEye_t;

typedef struct
{
	int					planetCount;
	planetInfo_t		planets[ 96 ];	// a little over 8k used to as a buffer into a select statement

	solarsystemEye_t	eye;
} solarsystemInfo_t;

planetInfo_t * SolarSystem_Select( solarsystemInfo_t * ss, const itemDef_t * item, float t, float x, float y );
void SolarSystem_LookAt( solarsystemEye_t * eye, const planetInfo_t * p, float time );
void SolarSystem_TopDown( solarsystemEye_t *eye, const solarsystemInfo_t *ss, float t, const float angles[2], const float ofs[3], bool center_on_planets );

void SolarSystem_Draw( solarsystemInfo_t * ss, rectDef_t * r, const planetInfo_t * selected,
	const planetInfo_t *current, float time, float selScale );
void SolarSystem_LerpEye( solarsystemEye_t * out, const solarsystemEye_t * a, const solarsystemEye_t * b, int rd, float t );

qboolean SolarSystem_GetScreenRect( const itemDef_t * item, const vec3_t pos, float r, rectDef_t * rect, float * z );
void SolarSystem_PlanetPosition( const planetInfo_t * p, float t, vec3_t pos );

void UI_DrawPlanet( solarsystemInfo_t * ss, rectDef_t * r, int planet_id, float time );

//
// ui_menu.c
//
extern void MainMenu_Cache( void );
extern void UI_MainMenu(void);
extern void UI_RegisterCvars( void );
extern void UI_UpdateCvars( void );

//
// ui_connect.c
//
extern void UI_DrawConnectScreen( qboolean overlay );

//
// ui_players.c
//

//FIXME ripped from cg_local.h
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;

typedef struct {
	// model info
	qhandle_t		legsModel;
	qhandle_t		legsSkin;
	lerpFrame_t		legs;
	lerpFrame_t		legs2;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;
	lerpFrame_t		torso;
	lerpFrame_t		torso2;

	qhandle_t		headModel;
	qhandle_t		headSkin;

	animation_t		animations[MAX_TOTALANIMATIONS];

	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;
	vec3_t			flashDlightColor;
	int				muzzleFlashTime;

	// currently in use drawing parms
	vec3_t			viewAngles;
	vec3_t			moveAngles;
	weapon_t		currentWeapon;
	int				legsAnim;
	int				torsoAnim;

	// animation vars
	weapon_t		weapon;
	weapon_t		lastWeapon;
	weapon_t		pendingWeapon;
	int				weaponTimer;
	int				pendingLegsAnim;
	int				torsoAnimationTimer;
	int				pendingTorsoAnim;
	int				legsAnimationTimer;
	int				idleTimer;

	qboolean		chat;
	qboolean		newModel;

	qboolean		barrelSpinning;
	float			barrelAngle;
	int				barrelTime;

	int				realWeapon;
	
	modelType_t		modelType;
} playerInfo_t;

void UI_DrawPlayer( rectDef_t * r, playerInfo_t *pi, int time, float zoom );
void UI_PlayerInfo_SetModel( playerInfo_t *pi, const char *model, const char *headmodel, char *teamName );
void UI_PlayerInfo_SetInfo( playerInfo_t *pi, animNumber_t legsAnim, animNumber_t torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNum, qboolean chat );
qboolean UI_RegisterClientModelname( playerInfo_t *pi, const char *modelSkinName , const char *headName, const char *teamName);

//
// ui_atoms.c
//
void UI_ActivateEditDlg( const char* menuName, const char * itemName, float val, float minVal, float maxVal, qboolean activate );

// new ui stuff
#define MAX_PINGREQUESTS		32
#define MAX_ADDRESSLENGTH		64
#define MAX_SERVERSTATUS_LINES	128
#define MAX_SERVERSTATUS_TEXT	1024
#define MAX_FOUNDPLAYER_SERVERS	16
#define MAX_CHARACTERS		5


typedef struct
{
	char			name[ MAX_NAME_LENGTH ];
	char			model[ MAX_QPATH ];
	int				rank;
	playerInfo_t	info;
} characterInfo_t;

typedef struct serverStatus_s {
	qboolean refreshActive;
	int		refreshtime;
	int		numqueriedservers;
	int		currentping;
	int		currentServer;
	int		numPlayersOnServers;
	int		nextDisplayRefresh;
	int		nextSortTime;
	qhandle_t currentServerPreview;
	int		currentServerCinematic;
} serverStatus_t;

typedef struct {
	const char *modName;
	const char *modDescr;
} modInfo_t;


typedef struct {
	displayContextDef_t uiDC;
	int newHighScoreTime;
	int newBestTime;
	int showPostGameTime;
	qboolean newHighScore;
	qboolean demoAvailable;
	qboolean soundHighScore;

    int traveltime;
    
	int timeLeft;
    int totalTime; //stores the max length of time the game lasts.

    int connectMenu; //store the connect menu we should use at the next connect screen

	int redBlue;
	int skillIndex;

	bool charsLoaded;
	characterInfo_t		characters[ MAX_CHARACTERS ];

	serverStatus_t serverStatus;

	int currentCrosshair;
	int startPostGameTime;
	sfxHandle_t newHighScoreSound;

	int				scoresSortKey;

	int				missionStatus;		// -1 failed, 0 in progress, 1 completed   - updated by CS_OBJECTIVES

	int				clientNum;
	qhandle_t		tooltip;
	effectDef_t *	tooltipDef1;
	effectDef_t *	tooltipDef2;

	int				maxplayers;

	int	T_Yes;
	int T_No;

}	uiInfo_t;

extern uiInfo_t uiInfo;

#define MAX_PLAYERS uiInfo.maxplayers



extern void			UI_Init( void );
extern void			UI_Shutdown( void );
extern void			UI_KeyEvent( int key );
extern void			UI_MouseEvent( int dx, int dy );
extern void			UI_Refresh( int realtime );
extern qboolean		UI_ConsoleCommand( int realTime );
extern float		UI_ClampCvar( float min, float max, float value );
extern void			UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ); 
extern void			UI_UpdateScreen( void );
extern void			UI_LerpColor(vec4_t a, vec4_t b, vec4_t c, float t);
extern qboolean 	UI_CursorInRect (int x, int y, int width, int height);
extern void			UI_AdjustFrom640( float *x, float *y, float *w, float *h );
extern void			UI_AdjustTo640( int *x, int *y );
extern qboolean		UI_IsFullscreen( void );
extern void			UI_SetActiveMenu( uiMenuCommand_t menu );
extern void			UI_PopMenu (void);
extern char			*UI_Argv( int arg );
extern int			UI_Argc( void );
extern char			*UI_Cvar_VariableString( const char *var_name );
extern void			UI_Refresh( int time );
extern void			UI_KeyEvent( int key );

//
// ui_syscalls.c
//
void			trap_Print( const char *string );
void			trap_Error( int level, const char *string );
int				trap_Milliseconds( void );
void			trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void			trap_Cvar_Update( vmCvar_t *vmCvar );
void			trap_Cvar_Set( const char *var_name, const char *value );
float			trap_Cvar_VariableValue( const char *var_name );
int				trap_Cvar_VariableInt( const char * var_name );
void			trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void			trap_Cvar_SetValue( const char *var_name, float value );
void			trap_Cvar_Reset( const char *name );
void			trap_Cvar_Create( const char *var_name, const char *var_value, int flags );
void			trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize );
int				trap_Argc( void );
void			trap_Argv( int n, char *buffer, int bufferLength );
int				trap_ArgvI( int n );
void			trap_Cmd_ExecuteText( int exec_when, const char *text );	// don't use EXEC_NOW!
int				trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void			trap_FS_Read( void *buffer, int len, fileHandle_t f );
void			trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void			trap_FS_FCloseFile( fileHandle_t f );
int				trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
int				trap_FS_Seek( fileHandle_t f, long offset, int origin ); // fsOrigin_t
qhandle_t		trap_R_RegisterModel( const char *name );
qhandle_t		trap_R_RegisterSkin( const char *name );
qhandle_t		trap_R_RegisterShader( const char *name );
qhandle_t		trap_R_RegisterShaderNoMip( const char *name );
void			trap_R_ClearScene( void );

qhandle_t		trap_R_BuildPose	( qhandle_t h, const animGroupFrame_t *f, uint numGroupFrames, boneOffset_t * b, uint numBoneOffsets );
qhandle_t		trap_R_BuildPose2	( qhandle_t h, const animGroupFrame_t *f0, const boneOffset_t *b0, const animGroupFrame_t *g1, const boneOffset_t *b1, const animGroupTransition_t *frameLerps );
qhandle_t		trap_R_BuildPose3	( qhandle_t h, const animGroupFrame_t *f0, const animGroupFrame_t *f1, const animGroupTransition_t *frameLerps );

void			trap_R_AddRefEntityToScene( const refEntity_t *re );
void			trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts );
void			trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void			trap_R_RenderScene( const refdef_t *fd );

void			trap_R_SetColor( const float *rgba );
void			trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void			trap_R_DrawSprite( float x, float y, float w, float h, float mat[2][3], vec4_t uv, qhandle_t hShader );
void			trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );

/*
	These are calls to the equivalent STRender functions.
	See tr_public.h for more info.
*/
qhandle_t		trap_R_ShapeCreate( const curve_t *curves, const shapeGen_t *genTypes, int numShapes );
void			trap_R_ShapeDraw( qhandle_t shape, qhandle_t shader, float * m );
void			trap_R_GraphDraw( rectDef_t * r, int * graph, int n, qhandle_t shader );
void			trap_R_ShapeDrawMulti( void **shapes, unsigned int numShapes, qhandle_t shader );

void			DrawShape( rectDef_t * r, qhandle_t shape, qhandle_t shader );

void			trap_UpdateScreen( void );
int				trap_CM_LerpTag( affine_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName );
void			trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
sfxHandle_t		trap_S_RegisterSound( const char *sample, qboolean compressed );
void			trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void			trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void			trap_Key_SetBinding( int keynum, const char *binding );
qboolean		trap_Key_IsDown( int keynum );
qboolean		trap_Key_GetOverstrikeMode( void );
void			trap_Key_SetOverstrikeMode( qboolean state );
void			trap_Key_ClearStates( void );
int				trap_Key_GetCatcher( void );
void			trap_Key_SetCatcher( int catcher );
int				trap_Key_GetKey( const char *binding );

void			trap_GetClipboardData( char *buf, int bufsize );
void			trap_GetClientState( uiClientState_t *state );
int				trap_GetClientNum( void );
void			trap_GetGlconfig( vidConfig_t *glconfig );
int				trap_GetConfigString( int index, char* buff, int buffsize );
int				trap_UpdateGameState( globalState_t * gs );
int				trap_GetPlayerStat( int stat );
qboolean		trap_LAN_UpdateVisiblePings( int source );
int				trap_MemoryRemaining( void );
void			trap_GetCDKey( char *buf, int buflen );
void			trap_SetCDKey( char *buf );
qhandle_t		trap_R_RegisterFont(const char *pFontname );
void			trap_R_GetFonts( char * buffer, int size );
void			trap_S_StopBackgroundTrack( void );
void			trap_S_StartBackgroundTrack( const char *intro, const char *loop);
int				trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status		trap_CIN_StopCinematic(int handle);
e_status		trap_CIN_RunCinematic (int handle);
void			trap_CIN_DrawCinematic (int handle);
void			trap_CIN_SetExtents (int handle, int x, int y, int w, int h);
int				trap_RealTime(qtime_t *qtime);
void			trap_R_RemapShader( const char *oldShader, const char *newShader );

void			trap_SetPbClStatus( int status );

int				trap_Con_Print		( int c, const char * text );
void			trap_Con_Draw		( int c, float frac );
void			trap_Con_HandleMouse( int c, float x, float y );
void			trap_Con_HandleKey	( int c, int key );
void			trap_Con_Clear		( int c );
void			trap_Con_Resize		( int c, int display, float x, float y, float w, float h );
void			trap_Con_GetText	( char *buf, int buf_size, int c );

int				trap_Get_Radar		( radarInfo_t *out_ents, int maxEnts, const radarFilter_t *filters, int numFilters, float range );


//
//	ui_gameinfo.c
//
void UI_LoadBots( void );

//ui_vote.c
extern void UI_Vote( int voteCmd, const char * cmd );


extern globalState_t gsi;

#include "ui_anim.h"


#define GS_RANK(client)			gsi.RANKS[ client ]

#define GS_PLANETS(planet)		gsi.PLANETS[ planet ]

#define GS_DISASTERS(disaster)	gsi.DISASTERS[ disaster ]

#define GS_COUNTDOWN			gsi.COUNTDOWN
#define GS_INPLAY				gsi.INPLAY

#define GS_TIME					gsi.TIME
#define GS_LASTTIME				gsi.LASTTIME
#define GS_TIMELEFT				gsi.TIMELEFT
#define GS_TURN					gsi.TURN
#define GS_PLANET				gsi.PLANET

#define GS_DISASTERCOUNT		gsi.DISASTERCOUNT
#define GS_ASSASSINATE			gsi.ASSASSINATE
#define GS_BOSS					gsi.BOSS
#define GS_BOARDING				gsi.BOARDING

extern int			sqldone		( void );
extern const char *	sqltext		( int column );
extern int			sqlint		( int column );
extern int			sqlstep		( void );
extern int			sql			( const char * stmt, const char * argdef, ... );

#endif
