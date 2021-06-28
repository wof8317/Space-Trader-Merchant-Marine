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
// client.h -- primary header for client

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../sql/sql.h"
#include "../renderer/tr_public.h"
#include "../ui/ui_public.h"
#include "keys.h"
#include "snd_public.h"
#include "../cgame/cg_public.h"
#include "../game/bg_public.h"

#define	RETRANSMIT_TIMEOUT	3000	// time between connection packet retransmits


// snapshots are a view of the server at a given time
typedef struct {
	qboolean		valid;			// cleared if delta parsing was invalid
	int				snapFlags;		// rate delayed and dropped commands

	int				serverTime;		// server time the message is valid for (in msec)

	int				messageNum;		// copied from netchan->incoming_sequence
	int				deltaNum;		// messageNum the delta is from
	int				ping;			// time from when cmdNum-1 was sent to time packet was reeceived
	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	int				cmdNum;			// the next cmdNum the server is expecting
	playerState_t	ps;						// complete information about the current player at this time

	int				numEntities;			// all of the entities that need to be presented
	int				parseEntitiesNum;		// at the time of this snapshot

	int				serverCommandNum;		// execute all commands up to this before
											// making the snapshot current
} clSnapshot_t;



/*
=============================================================================

the clientActive_t structure is wiped completely at every
new gamestate_t, potentially several times during an established connection

=============================================================================
*/

typedef struct {
	int		p_cmdNumber;		// cl.cmdNumber when packet was sent
	int		p_serverTime;		// usercmd->serverTime when packet was sent
	int		p_realtime;			// cls.realtime when packet was sent
} outPacket_t;

// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original 
#define	MAX_PARSE_ENTITIES	2048

typedef struct {
	int			timeoutcount;		// it requres several frames in a timeout condition
									// to disconnect, preventing debugging breaks from
									// causing immediate disconnects on continue
	clSnapshot_t	snap;			// latest received from server

	int			serverTime;			// may be paused during play
	int			oldServerTime;		// to prevent time from flowing bakcwards
	int			oldFrameServerTime;	// to check tournament restarts
	int			serverTimeDelta;	// cl.serverTime = cls.realtime + cl.serverTimeDelta
									// this value changes as net lag varies
	qboolean	extrapolatedSnapshot;	// set if any cgame frame has been forced to extrapolate
									// cleared when CL_AdjustTimeDelta looks at it
	qboolean	newSnapshots;		// set on parse of any valid packet

	gameState_t	gameState;			// configstrings
	char		mapname[MAX_QPATH];	// extracted from CS_SERVERINFO

	int			parseEntitiesNum;	// index (not anded off) into cl_parse_entities[]

	int			mouseX[2], mouseY[2];
	int			mouseDx[2], mouseDy[2];	// added to by mouse events
	int			mouseIndex;
	int			joystickAxis[MAX_JOYSTICK_AXIS];	// set by joystick events

	// cgame communicates a few values to the client system
	int			cgameUserCmdValue;	// current weapon to add to usercmd_t
	float		cgameSensitivity;

	// cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
	// properly generated command
	usercmd_t	cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int			cmdNumber;			// incremented each frame, because multiple
									// frames may need to be packed into a single packet

	outPacket_t	outPackets[PACKET_BACKUP];	// information about each packet we have sent out

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t		viewangles;

	int			serverId;			// included in each client message so the server
												// can tell if it is for a prior map_restart
	// big stuff at end of structure so most offsets are 15 bits or less
	clSnapshot_t	snapshots[PACKET_BACKUP];

	entityState_t	entityBaselines[MAX_GENTITIES];	// for delta compression when not in previous frame

	entityState_t	parseEntities[MAX_PARSE_ENTITIES];

	sqlInfo_t	db;

} clientActive_t;

extern	clientActive_t		cl;

/*
=============================================================================

the clientConnection_t structure is wiped when disconnecting from a server,
either to go to a full screen console, play a demo, or connect to a different server

A connection can be to either a server through the network layer or a
demo through a file.

=============================================================================
*/


typedef struct {

	int			clientNum;
	int			lastPacketSentTime;			// for retransmits during connection
	int			lastPacketTime;				// for timeouts

	netadr_t	serverAddress;
	int			connectTime;				// for connection retransmits
	int			connectPacketCount;			// for display on connection dialog
	char		serverMessage[MAX_STRING_TOKENS];	// for display on connection dialog

	int			challenge;					// from the server to use for connecting
	int			checksumFeed;				// from the server for checksum calculations

	// these are our reliable messages that go to the server
	int			reliableSequence;
	int			reliableAcknowledge;		// the last one the server has executed
	char *		reliableCommands[MAX_RELIABLE_COMMANDS];
	char		reliableCommandBuffer[ MAX_RELIABLE_BUFFER ];

	// server message (unreliable) and command (reliable) sequence
	// numbers are NOT cleared at level changes, but continue to
	// increase as long as the connection is valid

	// message sequence is used by both the network layer and the
	// delta compression layer
	int			serverMessageSequence;

	// reliable messages received from server
	int			serverCommandSequence;
	int			lastSnapServerCmdNum;
	int			lastExecutedServerCommand;		// last server command grabbed or executed with CL_GetServerCommand
	char *		serverCommands[MAX_RELIABLE_COMMANDS];
	char		serverCommandBuffer[ MAX_RELIABLE_BUFFER ];

	// file transfer from server
	qboolean	downloadRestart;	// if true, we need to do another FS_Restart because we downloaded a pak

	// demo information
	char		demoName[MAX_QPATH];
	qboolean	demorecording;
	qboolean	demoplaying;
	qboolean	demowaiting;	// don't record until a non-delta message is received
	qboolean	firstDemoFrameSkipped;
	fileHandle_t	demofile;

	int			timeDemoFrames;		// counter of rendered frames
	int			timeDemoStart;		// cls.realtime before first frame
	int			timeDemoBaseTime;	// each frame will be at this time + frameNum * 50

	// big stuff at end of structure so most offsets are 15 bits or less
	netchan_t	netchan;
} clientConnection_t;

extern	clientConnection_t clc;

/*
==================================================================

the clientStatic_t structure is never wiped, and is used even when
no client connection is active at all

==================================================================
*/

typedef struct {
	connstate_t	state;				// connection status
	int			keyCatchers;		// bit flags

	qboolean	cddialog;			// bring up the cd needed dialog next frame

	char		servername[MAX_OSPATH];		// name of server from original connect (used by reconnect)

	// when the server clears the hunk, all of these must be restarted
	qboolean	rendererStarted;
	qboolean	soundStarted;
	qboolean	soundRegistered;
	qboolean	uiStarted;
	qboolean	cgameStarted;

	int			framecount;
	int			frametime;			// msec since last frame

	int			realtime;			// ignores pause
	int			realFrametime;		// ignoring pause, so console always works

	int			broadcastTime;		// time when a broadcast was sent

	int masterNum;

	// update server info
	netadr_t	updateServer;
	char		updateChallenge[MAX_TOKEN_CHARS];
	char		updateInfoString[MAX_INFO_STRING];

	netadr_t	authorizeServer;

	// rendering info
	vidConfig_t	glconfig;
	qhandle_t	whiteShader;

	qhandle_t	font;
	qhandle_t	menu;
	
} clientStatic_t;

extern	clientStatic_t		cls;

//=============================================================================

extern	vm_t			*cgvm;	// interface to cgame dll or vm
extern	vm_t			*uivm;	// interface to ui dll or vm
extern	refexport_t		re;		// interface to refresh .dll


//
// cvars
//
extern	cvar_t	*cl_nodelta;
extern	cvar_t	*cl_debugMove;
extern	cvar_t	*cl_noprint;
extern	cvar_t	*cl_timegraph;
extern	cvar_t	*cl_maxpackets;
extern	cvar_t	*cl_packetdup;
extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_showSend;
extern	cvar_t	*cl_timeNudge;
extern	cvar_t	*cl_showTimeDelta;
extern	cvar_t	*cl_freezeDemo;

extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;
extern	cvar_t	*cl_run;
extern	cvar_t	*cl_anglespeedkey;

extern	cvar_t	*cl_sensitivity;
extern	cvar_t	*cl_freelook;

extern	cvar_t	*cl_mouseAccel;
extern	cvar_t	*cl_showMouseRate;

extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*m_forward;
extern	cvar_t	*m_side;
extern	cvar_t	*m_filter;

extern	cvar_t	*cl_timedemo;

extern	cvar_t	*cl_activeAction;

extern	cvar_t	*cl_conXOffset;
extern	cvar_t	*cl_conYOffset;
extern	cvar_t	*cl_conWidth;
extern	cvar_t	*cl_conHeight;
extern	cvar_t	*cl_inGameVideo;
extern	cvar_t	*cl_authserver;

//=================================================

//
// cl_main
//

void CL_Init (void);
void CL_FlushMemory(void);
void CL_ShutdownAll( void );
void CL_AddReliableCommand( const char *cmd );
char * CL_GetReliableCommand( int index );

void CL_StartHunkUsers( void );

void Q_EXTERNAL_CALL CL_Disconnect_f (void);
void CL_GetChallengePacket (void);
void Q_EXTERNAL_CALL CL_Vid_Restart_f( void );
void Q_EXTERNAL_CALL CL_Snd_Restart_f (void);
void CL_StartDemoLoop( void );
void CL_NextDemo( void );
void CL_ReadDemoMessage( void );

void CL_InitDownloads(void);

void CL_ShutdownRef( void );
void CL_InitRef( void );
int CL_ServerStatus( char *serverAddress, char *serverStatusString, int maxLen );

int CL_UpdateGameState( globalState_t * gs );

//
// cl_input
//
typedef struct {
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame if both a down and up happened
	qboolean	active;			// current state
	qboolean	wasPressed;		// set when down, not cleared when up
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_ClearState (void);
void CL_ReadPackets (void);

void CL_WritePacket( void );
void Q_EXTERNAL_CALL IN_CenterView (void);

void CL_VerifyCode( void );

float CL_KeyState (kbutton_t *key);
char *Key_KeynumToString (int keynum);

//
// cl_parse.c
//
extern int cl_connectedToPureServer;

void CL_SystemInfoChanged( void );
void CL_ParseServerMessage( msg_t *msg );
char * CL_GetReliableServerCommand( int index );

//====================================================================

void	CL_ServerInfoPacket( netadr_t from, msg_t *msg );
void	Q_EXTERNAL_CALL CL_LocalServers_f( void );
void	Q_EXTERNAL_CALL CL_GlobalServers_f( void );
void	Q_EXTERNAL_CALL CL_FavoriteServers_f( void );
void	Q_EXTERNAL_CALL CL_GetToken_f( void );
void	Q_EXTERNAL_CALL CL_Ping_f( void );
#ifdef USE_IRC
void	Q_EXTERNAL_CALL CL_Irc_f( void );
#endif
qboolean Q_EXTERNAL_CALL CL_UpdateVisiblePings_f( int source );
void	Q_EXTERNAL_CALL CL_DumpMapReferences( void );
void	CL_ServerAddToRefreshList( const char *addr, const char *channel );
//
// console
//
int			Con_Print		( int c, const char * text );
void		Con_Draw		( int c, float frac, bool scrollBar, bool background );
void		Con_HandleMouse	( int c, float x, float y );
void		Con_HandleKey	( int c, int key );
void		Con_Clear		( int c );
void		Con_Resize		( int c, int display, float x, float y, float w, float h );
const char* Q_EXTERNAL_CALL Con_GetText	( int console );

void Con_Init (void);
void Q_EXTERNAL_CALL Con_Clear_f (void);
void Q_EXTERNAL_CALL Con_ToggleConsole_f (void);
void Con_ClearNotify (void);
void Con_RunConsole (void);
void Con_DrawConsole (void);
void Con_PageUp( void );
void Con_PageDown( void );
void Con_Top( void );
void Con_Bottom( void );
void Con_Close( void );

//
// cl_scrn.c
//
void	SCR_Init (void);
void	SCR_UpdateScreen (void);

void	SCR_DebugGraph (float value, int color);

void	SCR_AdjustFrom640( float *x, float *y, float *w, float *h );
void	SCR_FillRect( float x, float y, float width, float height, const float *color, qhandle_t shader );
void	SCR_FillSquareRect( float x, float y, float width, float height, const float *color );
void	SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void	SCR_DrawNamedPic( float x, float y, float width, float height, const char *picname );

//
// cl_cin.c
//

void Q_EXTERNAL_CALL CL_PlayCinematic_f( void );
void SCR_DrawCinematic (void);
void SCR_RunCinematic (void);
void SCR_StopCinematic (void);
int Q_EXTERNAL_CALL CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status CIN_StopCinematic(int handle);
e_status Q_EXTERNAL_CALL CIN_RunCinematic (int handle);
void CIN_DrawCinematic (int handle);
void CIN_SetExtents (int handle, int x, int y, int w, int h);
void CIN_SetLooping (int handle, qboolean loop);
void Q_EXTERNAL_CALL CIN_UploadCinematic(int handle);
void CIN_CloseAllVideos(void);

//
// cl_cgame.c
//
void CL_InitCGame( void );
void CL_ShutdownCGame( void );
qboolean CL_GameCommand( void );
void CL_CGameRendering( stereoFrame_t stereo, qboolean uiFullScreen );
void CL_CGameRenderOverlays( stereoFrame_t stereo );
void CL_SetCGameTime( void );
void CL_FirstSnapshot( void );
void CL_ShaderStateChanged(void);

void CL_R_LoadWorld( const char *mapName );

//
// cl_ui.c
//
void CL_InitUI( void );
void CL_ShutdownUI( qboolean save_menustack );
int Key_GetCatcher( void );
void Key_SetCatcher( int catcher );
void LAN_LoadCachedServers();
void LAN_SaveServersToCache();
void CL_DownloadProgress( void );


//
// cl_net_chan.c
//
void CL_Netchan_Transmit( netchan_t *chan, msg_t* msg);	//int length, const byte *data );
void CL_Netchan_TransmitNextFragment( netchan_t *chan );
qboolean CL_Netchan_Process( netchan_t *chan, msg_t *msg );


//
// cl_text.c
//

int CL_RenderText_ExtractLines	(	fontInfo_t* font,
									float fontScale,
									float startColor[4],
									float baseColor[4],
									const char * text,
									int limit,
									int align,
									float width,
									lineInfo_t * lines,
									int line_offset,
									int line_limit
								);


void CL_RenderText_Line			(	lineInfo_t * line,
									fontInfo_t * font,
									float x,
									float y,
									float fontScale,
									float letterScaleX,
									float letterScaleY,
									int smallcaps,
									int cursorLoc,
									int cursorChar,
									qboolean colors
								);


void CL_RenderText				(	float*			rect,
									float			scale,
									vec4_t			color,
									const char*		text,
									int				limit,
									int				horiz,
									int				vert,
									int				style,
									int				cursor,
									int				cursorChar,
									qhandle_t		font,
									float*			textRect
								);


