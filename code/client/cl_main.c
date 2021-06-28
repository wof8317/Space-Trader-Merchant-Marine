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
// cl_main.c  -- client main loop

#include "client.h"

cvar_t	*cl_nodelta;
cvar_t	*cl_debugMove;

cvar_t	*cl_noprint;
cvar_t	*cl_motd;

cvar_t	*rcon_client_password;
cvar_t	*rconAddress;

cvar_t	*cl_timeout;
cvar_t	*cl_maxpackets;
cvar_t	*cl_packetdup;
cvar_t	*cl_timeNudge;
cvar_t	*cl_showTimeDelta;
cvar_t	*cl_freezeDemo;

cvar_t	*cl_shownet;
cvar_t	*cl_showSend;
cvar_t	*cl_timedemo;
cvar_t	*cl_avidemo;
cvar_t	*cl_forceavidemo;

cvar_t	*cl_freelook;
cvar_t	*cl_sensitivity;

cvar_t	*cl_mouseAccel;
cvar_t	*cl_showMouseRate;

cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;
cvar_t	*m_filter;

cvar_t	*cl_activeAction;

cvar_t	*cl_motdString;

cvar_t	*cl_conXOffset;
cvar_t	*cl_conYOffset;
cvar_t	*cl_conWidth;
cvar_t	*cl_conHeight;

cvar_t	*cl_inGameVideo;

cvar_t	*cl_serverStatusResendTime;
cvar_t	*cl_trn;

cvar_t	*cl_authserver;
cvar_t	*cl_ircport;

cvar_t	*cl_lang;

clientActive_t		cl;
clientConnection_t	clc;
clientStatic_t		cls;
vm_t				*cgvm;

// Structure containing functions exported from refresh DLL
refexport_t	re;

typedef struct serverStatus_s
{
	char string[BIG_INFO_STRING];
	netadr_t address;
	int time, startTime;
	qboolean pending;
	qboolean print;
	qboolean retrieved;
} serverStatus_t;

serverStatus_t cl_serverStatusList[MAX_SERVERSTATUSREQUESTS];
int serverStatusCount;

extern void SV_BotFrame( int time );
void CL_CheckForResend( void );
void Q_EXTERNAL_CALL CL_ShowIP_f(void);
void Q_EXTERNAL_CALL CL_ServerStatus_f(void);
void CL_ServerStatusResponse( netadr_t from, msg_t *msg );

sqlInfo_t * sql_getclientdb( void ) { return &cl.db; }

/*
===============
CL_CDDialog

Called by Com_Error when a cd is needed
===============
*/
void CL_CDDialog( void ) {
	cls.cddialog = qtrue;	// start it next frame
}


/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

char * CL_GetReliableCommand( int index ) {
	char * cmd = clc.reliableCommands[ index & (MAX_RELIABLE_COMMANDS-1) ];
	return cmd?cmd:"";
}

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future usercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand( const char *cmd ) {
	int		index;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if ( clc.reliableSequence - clc.reliableAcknowledge > MAX_RELIABLE_COMMANDS ) {
		Com_Error( ERR_DROP, "Client command overflow" );
	}
	clc.reliableSequence++;

	index = clc.reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );

	clc.reliableCommands[ index ] = Q_strcpy_ringbuffer(	clc.reliableCommandBuffer,
															sizeof( clc.reliableCommandBuffer ),
															clc.reliableCommands[ (clc.reliableAcknowledge) & ( MAX_RELIABLE_COMMANDS - 1 ) ],
															clc.reliableCommands[ (clc.reliableSequence-1) & ( MAX_RELIABLE_COMMANDS - 1 ) ],
															cmd
														);

	if ( !clc.reliableCommands[ index ] ) {
		Com_Error( ERR_DROP, "Client command buffer overflow" );
	}
}

/*
=======================================================================

CLIENT SIDE DEMO RECORDING

=======================================================================
*/

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage ( msg_t *msg, int headerBytes ) {
	int		len, swlen;

	// write the packet sequence
	len = clc.serverMessageSequence;
	swlen = LittleLong( len );
	FS_Write (&swlen, 4, clc.demofile);

	// skip the packet sequencing information
	len = msg->cursize - headerBytes;
	swlen = LittleLong(len);
	FS_Write (&swlen, 4, clc.demofile);
	FS_Write ( msg->data + headerBytes, len, clc.demofile );
}


/*
====================
CL_StopRecording_f

stop recording a demo
====================
*/
void Q_EXTERNAL_CALL CL_StopRecord_f( void )
{
	int		len;

	if ( !clc.demorecording ) {
		Com_Printf ("Not recording a demo.\n");
		return;
	}

	// finish up
	len = -1;
	FS_Write (&len, 4, clc.demofile);
	FS_Write (&len, 4, clc.demofile);
	FS_FCloseFile (clc.demofile);
	clc.demofile = 0;
	clc.demorecording = qfalse;
	Com_Printf ("Stopped demo.\n");
}

/* 
================== 
CL_DemoFilename
================== 
*/  
void CL_DemoFilename( int number, char *fileName ) {
	int		a,b,c,d;

	if ( number < 0 || number > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "demo9999.tga" );
		return;
	}

	a = number / 1000;
	number -= a*1000;
	b = number / 100;
	number -= b*100;
	c = number / 10;
	number -= c*10;
	d = number;

	Com_sprintf( fileName, MAX_OSPATH, "demo%i%i%i%i"
		, a, b, c, d );
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
void Q_EXTERNAL_CALL CL_Record_f( void ) {
	char		name[MAX_OSPATH];
	byte		bufData[MAX_MSGLEN];
	char		demoName[MAX_QPATH];
	msg_t	buf;
	int			i;
	int			len;
	entityState_t	*ent;
	entityState_t	nullstate;
	char		*s;

	if ( Cmd_Argc() > 2 ) {
		Com_Printf ("record <demoname>\n");
		return;
	}

	if ( clc.demorecording ) {
		Com_Printf ("Already recording.\n");
		return;
	}

	if ( cls.state != CA_ACTIVE ) {
		Com_Printf ("You must be in a level to record.\n");
		return;
	}

  // sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
	if ( NET_IsLocalAddress( clc.serverAddress ) && !Cvar_VariableValue( "g_synchronousClients" ) ) {
		Com_Printf (S_COLOR_YELLOW "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");
	}

	if ( Cmd_Argc() == 2 ) {
		s = Cmd_Argv(1);
		Q_strncpyz( demoName, s, sizeof( demoName ) );
		Com_sprintf (name, sizeof(name), "demos/%s.dm_"PROTOCOL_VERSION, demoName );
	} else {
		int		number;

		// scan for a free demo name
		for ( number = 0 ; number <= 9999 ; number++ ) {
			CL_DemoFilename( number, demoName );
			Com_sprintf (name, sizeof(name), "demos/%s.dm_"PROTOCOL_VERSION, demoName );

			len = FS_ReadFile( name, NULL );
			if ( len <= 0 ) {
				break;	// file doesn't exist
			}
		}
	}

	// open the demo file

	Com_Printf ("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite( name );
	if ( !clc.demofile ) {
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}
	clc.demorecording = qtrue;

	Q_strncpyz( clc.demoName, demoName, sizeof( clc.demoName ) );

	// don't start saving messages until a non-delta compressed message is received
	clc.demowaiting = qtrue;

	// write out the gamestate message
	MSG_Init (&buf, bufData, sizeof(bufData));
	MSG_Bitstream(&buf);

	// NOTE, MRE: all server->client messages now acknowledge
	MSG_WriteLong( &buf, clc.reliableSequence );

	MSG_WriteByte (&buf, svc_gamestate);
	MSG_WriteLong (&buf, clc.serverCommandSequence );

	// configstrings
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( !cl.gameState.stringOffsets[i] ) {
			continue;
		}
		s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
		MSG_WriteByte (&buf, svc_configstring);
		MSG_WriteShort (&buf, i);
		MSG_WriteBigString (&buf, s);
	}

	// write sql tables
	MSG_WriteAllTables( &buf, &cl.db );

	// baselines
	Com_Memset (&nullstate, 0, sizeof(nullstate));
	for ( i = 0; i < MAX_GENTITIES ; i++ ) {
		ent = &cl.entityBaselines[i];
		if ( !ent->number ) {
			continue;
		}
		MSG_WriteByte (&buf, svc_baseline);		
		MSG_WriteDeltaEntity (&buf, &nullstate, ent, qtrue );
	}

	MSG_WriteByte( &buf, svc_EOF );
	
	// finished writing the gamestate stuff

	// write the client num
	MSG_WriteLong(&buf, clc.clientNum);
	// write the checksum feed
	MSG_WriteLong(&buf, clc.checksumFeed);

	// finished writing the client packet
	MSG_WriteByte( &buf, svc_EOF );

	// write it to the demo file
	len = LittleLong( clc.serverMessageSequence - 1 );
	FS_Write (&len, 4, clc.demofile);

	len = LittleLong (buf.cursize);
	FS_Write (&len, 4, clc.demofile);
	FS_Write (buf.data, buf.cursize, clc.demofile);

	// the rest of the demo file will be copied from net messages
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

/*
=================
CL_DemoCompleted
=================
*/
void CL_DemoCompleted( void ) {
	if (cl_timedemo && cl_timedemo->integer) {
		int	time;
		
		time = Sys_Milliseconds() - clc.timeDemoStart;
		if ( time > 0 ) {
			Com_Printf ("%i frames, %3.1f seconds: %3.1f fps\n", clc.timeDemoFrames,
			time/1000.0, clc.timeDemoFrames*1000.0 / time);
		}
	}

	CL_Disconnect( qtrue );
	CL_NextDemo();
}

/*
=================
CL_ReadDemoMessage
=================
*/
void CL_ReadDemoMessage( void ) {
	int			r;
	msg_t		buf;
	byte		bufData[ MAX_MSGLEN ];
	int			s;

	if ( !clc.demofile ) {
		CL_DemoCompleted ();
		return;
	}

	// get the sequence number
	r = FS_Read( &s, 4, clc.demofile);
	if ( r != 4 ) {
		CL_DemoCompleted ();
		return;
	}
	clc.serverMessageSequence = LittleLong( s );

	// init the message
	MSG_Init( &buf, bufData, sizeof( bufData ) );

	// get the length
	r = FS_Read (&buf.cursize, 4, clc.demofile);
	if ( r != 4 ) {
		CL_DemoCompleted ();
		return;
	}
	buf.cursize = LittleLong( buf.cursize );
	if ( buf.cursize == -1 ) {
		CL_DemoCompleted ();
		return;
	}
	if ( buf.cursize > buf.maxsize ) {
		Com_Error (ERR_DROP, "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN");
	}
	r = FS_Read( buf.data, buf.cursize, clc.demofile );
	if ( r != buf.cursize ) {
		Com_Printf( "Demo file was truncated.\n");
		CL_DemoCompleted ();
		return;
	}

	clc.lastPacketTime = cls.realtime;
	buf.readcount = 0;
	CL_ParseServerMessage( &buf );
}

/*
====================
CL_WalkDemoExt
====================
*/
static void CL_WalkDemoExt(char *arg, char *name, int *demofile)
{
	int i = 0;
	*demofile = 0;
	while(demo_protocols[i])
	{
		Com_sprintf (name, MAX_OSPATH, "demos/%s.dm_%d", arg, demo_protocols[i]);
		FS_FOpenFileRead( name, demofile, qtrue );
		if (*demofile)
		{
			Com_Printf("Demo file: %s\n", name);
			break;
		}
		else
			Com_Printf("Not found: %s\n", name);
		i++;
	}
}

/*
====================
CL_PlayDemo_f

demo <demoname>

====================
*/
void Q_EXTERNAL_CALL CL_PlayDemo_f( void )
{
	char		name[MAX_OSPATH];
	char		*arg, *ext_test;
	int			protocol, i;
	char		retry[MAX_OSPATH];

	if (Cmd_Argc() != 2) {
		Com_Printf ("playdemo <demoname>\n");
		return;
	}

	// make sure a local server is killed
	Cvar_Set( "sv_killserver", "1" );

	CL_Disconnect( qtrue );

	// open the demo file
	arg = Cmd_Argv(1);
	
	// check for an extension .dm_?? (?? is protocol)
	ext_test = arg + strlen(arg) - 6;
	if ((strlen(arg) > 6) && (ext_test[0] == '.') && ((ext_test[1] == 'd') || (ext_test[1] == 'D')) && ((ext_test[2] == 'm') || (ext_test[2] == 'M')) && (ext_test[3] == '_'))
	{
		protocol = atoi(ext_test+4);
		i=0;
		while(demo_protocols[i])
		{
			if (demo_protocols[i] == protocol)
				break;
			i++;
		}
		if (demo_protocols[i])
		{
			Com_sprintf (name, sizeof(name), "demos/%s", arg);
			FS_FOpenFileRead( name, &clc.demofile, qtrue );
		} else {
			Com_Printf("Protocol %d not supported for demos\n", protocol);
			Q_strncpyz(retry, arg, sizeof(retry));
			retry[strlen(retry)-6] = 0;
			CL_WalkDemoExt( retry, name, &clc.demofile );
		}
	} else {
		CL_WalkDemoExt( arg, name, &clc.demofile );
	}
	
	if (!clc.demofile) {
		Com_Error( ERR_DROP, "couldn't open %s", name);
		return;
	}
	Q_strncpyz( clc.demoName, Cmd_Argv(1), sizeof( clc.demoName ) );

	Con_Close();

	cls.state = CA_CONNECTED;
	clc.demoplaying = qtrue;
	Q_strncpyz( cls.servername, Cmd_Argv(1), sizeof( cls.servername ) );

	// read demo messages until connected
	while ( cls.state >= CA_CONNECTED && cls.state < CA_PRIMED ) {
		CL_ReadDemoMessage();
	}
	// don't get the first snapshot this frame, to prevent the long
	// time from the gamestate load from messing causing a time skip
	clc.firstDemoFrameSkipped = qfalse;
}


/*
====================
CL_StartDemoLoop

Closing the main menu will restart the demo loop
====================
*/
void CL_StartDemoLoop( void ) {
	// start the demo loop again
	Cbuf_AddText ("d1\n");
	cls.keyCatchers = 0;
}

/*
==================
CL_NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void CL_NextDemo( void ) {
	char	v[MAX_STRING_CHARS];

	Q_strncpyz( v, Cvar_VariableString ("nextdemo"), sizeof(v) );
	v[MAX_STRING_CHARS-1] = 0;
	Com_DPrintf("CL_NextDemo: %s\n", v );
	if (!v[0]) {
		CL_FlushMemory();
		return;
	}

	Cvar_Set ("nextdemo","");
	Cbuf_AddText (v);
	Cbuf_AddText ("\n");
	Cbuf_Execute();
}


//======================================================================

/*
=====================
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll( void )
{

	// clear sounds
	S_DisableSounds();
	// shutdown CGame
	CL_ShutdownCGame();
	// shutdown UI
	CL_ShutdownUI( qfalse );

	// shutdown the renderer
	if( re.Shutdown )
	{
		re.Shutdown( qfalse ); // don't destroy window or context
		cls.rendererStarted = qfalse;
	}

	cls.uiStarted = qfalse;
	cls.cgameStarted = qfalse;
	cls.soundRegistered = qfalse;
}

/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void CL_FlushMemory( void ) {

	// shutdown all the client stuff
	CL_ShutdownAll();

	// if not running a server clear the whole hunk
	if ( !com_sv_running->integer ) {
		// clear the whole hunk
		Hunk_Clear();
		CM_ClearPreloaderData();
		// clear collision map data
		CM_ClearMap();
	}
	else {
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	CL_StartHunkUsers();
}

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}

	Con_Close();
	cls.keyCatchers = 0;

	// if we are already connected to the local host, stay connected
	if ( cls.state >= CA_CONNECTED && !Q_stricmp( cls.servername, "localhost" ) ) {
		cls.state = CA_CONNECTED;		// so the connect screen is drawn
		Com_Memset( cls.updateInfoString, 0, sizeof( cls.updateInfoString ) );
		Com_Memset( clc.serverMessage, 0, sizeof( clc.serverMessage ) );
		Com_Memset( &cl.gameState, 0, sizeof( cl.gameState ) );
		clc.lastPacketSentTime = -9999;
		SCR_UpdateScreen();
	} else {
		// clear nextmap so the cinematic shutdown doesn't execute it
		Cvar_Set( "nextmap", "" );
		CL_Disconnect( qtrue );
		Q_strncpyz( cls.servername, "localhost", sizeof(cls.servername) );
		cls.state = CA_CHALLENGING;		// so the connect screen is drawn
		cls.keyCatchers = 0;
		SCR_UpdateScreen();
		clc.connectTime = -RETRANSMIT_TIMEOUT;
		NET_StringToAdr( cls.servername, &clc.serverAddress);
		// we don't need a challenge on the localhost

		CL_CheckForResend();
	}
}

static int sql_global_int( const char * name ) {
	if ( name )
	{
		switch( SWITCHSTRING( name ) )
		{
			case CS('c','l','i','e'):	return clc.clientNum;
			default:
				Cmd_TokenizeString( name );
				return VM_Call( uivm, UI_GLOBAL_INT );
		}
	}

	return 0;
}

static qhandle_t sql_portrait( const char * path )
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

	return re.RegisterShaderNoMip( va("models/players/%s/icon_%s", model, skin ) );
}

static qhandle_t sql_shader( const char * name ) {
	return re.RegisterShader( name );
}

static qhandle_t sql_sound( const char * name ) {
	return S_RegisterSound( name, qtrue );
}

static qhandle_t sql_model( const char * name ) {
	return re.RegisterModel( name );
}

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState (void) {

//	S_StopAllSounds();

	Com_Memset( &cl, 0, sizeof( cl ) );

	//	clear sql db
	sql_reset			( &cl.db, TAG_SQL_CLIENT );
	sql_assign_gs		( &cl.db, (int*)&cl.snap.ps.gs );
	cl.db.ps		= (int*)&cl.snap.ps;

	sql_exec( &cl.db,
		"CREATE TABLE servers"
		"("
			"addr STRING, "
			"clients INTEGER, "
			"hostname STRING, "
			"mapname STRING, "
			"sv_maxclients INTEGER, "
			"game STRING, "
			"gametype INTEGER, "
			"nettype INTEGER, "
			"minping INTEGER, "
			"maxping INTEGER, "
			"punkbuster INTEGER, "
			"ping INTEGER, "
			"start INTEGER, "
			"source INTEGER, "
			"lastupdate INTEGER, "
			"channel STRING "
		");"
		);

	cl.db.global_int		= sql_global_int;
	cl.db.portrait			= sql_portrait;
	cl.db.shader			= sql_shader;
	cl.db.sound				= sql_sound;
	cl.db.model				= sql_model;
}


/*
=====================
CL_Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect( qboolean showMainMenu ) {
	if ( !com_cl_running || !com_cl_running->integer ) {
		return;
	}

	// shutting down the client so enter full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	if ( clc.demorecording ) {
		CL_StopRecord_f ();
	}

	if ( clc.demofile ) {
		FS_FCloseFile( clc.demofile );
		clc.demofile = 0;
	}

	if ( uivm && showMainMenu ) {
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
	}

	SCR_StopCinematic ();
	S_ClearSoundBuffer();

	// send a disconnect message to the server
	// send it a few times in case one is dropped
	if ( cls.state >= CA_CONNECTED ) {
		CL_AddReliableCommand( "disconnect" );
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
	}
	
	CL_ClearState ();

	// wipe the client connection
	Com_Memset( &clc, 0, sizeof( clc ) );

	Cvar_Set( "cl_spacetrader", "0" );

	cls.state = CA_DISCONNECTED;

	// allow cheats locally
	Cvar_Set( "sv_cheats", "1" );

	// not connected to a pure server anymore
	cl_connectedToPureServer = qfalse;
}


/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer( const char *string ) {
	char	*cmd;

	cmd = Cmd_Argv(0);

	// ignore key up commands
	if ( cmd[0] == '-' ) {
		return;
	}

	if ( clc.demoplaying || cls.state < CA_CONNECTED || cmd[0] == '+' ) {
		Com_Printf ("Unknown command \"%s\"\n", cmd);
		return;
	}

	if ( Cmd_Argc() > 1 ) {
		CL_AddReliableCommand( string );
	} else {
		CL_AddReliableCommand( cmd );
	}
}

#ifdef USE_WEBAUTH

/*
===================
CL_Highscore_response

gets highscores results from the website
===================
*/
static int QDECL CL_Highscore_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		Cvar_Set( "st_postresults", buffer );
		VM_Call(uivm, UI_REPORT_HIGHSCORE_RESPONSE );
	}
	return 1;
}

/*
===================
CL_Highscore_f

asks for highscores from the website
===================
*/
static void Q_EXTERNAL_CALL CL_Highscore_f( void ) {

	if ( Cmd_Argc() != 7 ) {
		Com_Printf("Usage: highscore <score> <skill> <kills> <time> <game> <real time>\n");
		return;
	}

	HTTP_PostUrl(	va("http://%s/user/report_score",AUTHORIZE_SERVER_NAME), CL_Highscore_response, 0,
					"c[slot]=%d&h[score]=%d&h[skill]=%d&h[kills]=%d&h[time]=%d&h[game_id]=%d&h[real_time]=%d",
					Cvar_VariableIntegerValue("slot"),
					atoi(Cmd_Argv(1)),
					atoi(Cmd_Argv(2)),
					atoi(Cmd_Argv(3)),
					atoi(Cmd_Argv(4)),
					atoi(Cmd_Argv(5)),
					atoi(Cmd_Argv(6)) );
}

/*
===================
CL_Login_response

website's response to a users attempt to login
===================
*/
static int QDECL CL_Login_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		Cvar_Set( "cl_servermessage", Info_ValueForKey( buffer, "message" ) );

		switch( atoi( Info_ValueForKey( buffer, "status" ) ) )
		{
			case 1:		VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_OK );			break;
			case -1:	VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_NOTVERIFIED );	break;
			default:	VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_BAD );			break;
		}
	}

	//	VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_UNAVAILABLE);

	return length;
}

/*
==================
CL_Login_f
==================
*/
static void Q_EXTERNAL_CALL CL_Login_f( void )
{
	if ( Cmd_Argc() != 3) {
		Com_Printf( "usage: login user password\n");
		return;	
	}

	HTTP_PostUrl( va("http://%s/user/login", AUTHORIZE_SERVER_NAME), CL_Login_response, 0, "user[login]=%s&user[password]=%s&version=%d", Cmd_Argv(1), Cmd_Argv(2), 31 );
}

/*
==================
CL_ForgotPassword_f
==================
*/
void Q_EXTERNAL_CALL CL_ForgotPassword_f(void)
{
	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: forgotpassword email\n" );
		return;
	}

	HTTP_PostUrl( va("http://%s/user/forgot_password", AUTHORIZE_SERVER_NAME), 0, 0, "user[email]=%s", Cmd_Argv(1) );
}

/*
==================
CL_CreateCharacter_response
==================
*/
static int QDECL CL_CreateCharacter_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		Cvar_Set( "cl_servermessage", buffer );
		VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_CREATECHARACTER );
	}

	return 1;
}


/*
==================
CL_CreateCharacter_f
==================
*/
void Q_EXTERNAL_CALL CL_CreateCharacter_f(void)
{
	if ( Cmd_Argc() != 4 ) {
		Com_Printf( "usage: createcharacter <slot> <name> <model>\n" );
		return;
	}

	HTTP_PostUrl( va("http://%s/user/create_character", AUTHORIZE_SERVER_NAME), CL_CreateCharacter_response, 0, "char[slot]=%s&char[name]=%s&char[model]=%s", Cmd_Argv(1), Cmd_Argv(2), Cmd_Argv(3) );
}

/*
==================
CL_DeleteCharacter_response
==================
*/
static int QDECL CL_DeleteCharacter_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		if ( buffer && buffer[0] == '1' ) {
			VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_DELETECHARACTER );
		} else {
			VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_BAD);
		}
	}

	return 1;
}

/*
==================
CL_DeleteCharacter_f
==================
*/
void Q_EXTERNAL_CALL CL_DeleteCharacter_f(void)
{
	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: deletecharacter <slot>\n" );
		return;
	}

	HTTP_PostUrl( va("http://%s/user/delete_character", AUTHORIZE_SERVER_NAME), CL_DeleteCharacter_response, 0, "slot=%s", Cmd_Argv(1) );
}

/*
==================
CL_GetAccount_response
==================
*/
static int QDECL CL_GetAccount_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		Cvar_Set( "cl_servermessage", buffer );
		VM_Call( uivm, UI_AUTHORIZED, AUTHORIZE_ACCOUNTINFO );
	}

	return 1;
}

/*
==================				 `
CL_GetAccount_f
==================
*/void Q_EXTERNAL_CALL CL_GetAccount_f(void)
{
	if ( Cmd_Argc() != 1 ) {
		Com_Printf( "usage: getaccount\n" );
		return;
	}

	HTTP_PostUrl( va("http://%s/user/characters", AUTHORIZE_SERVER_NAME), CL_GetAccount_response, 0, 0 );
}

/*
==================
CL_GetHighScores_response
==================
*/
int QDECL CL_GetHighScores_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	//	ignore message from server if already in the game
	if ( code == HTTP_WRITE && uivm && Cvar_VariableIntegerValue( "cl_spacetrader" ) == 0 ) {

		if ( length > 1 ) {
			sql_prepare	( &cl.db, "UPDATE OR INSERT scores CS $2 SEARCH challenge ?1;" );
			sql_bindint	( &cl.db, 1, atoi( Info_ValueForKey(buffer,"challenge") ) );
			sql_bindtext( &cl.db, 2, buffer );
			sql_step	( &cl.db );
			sql_done	( &cl.db );
		}
	}

	return 1;
}

/*
==================
CL_GlobalHighScores_f
==================
*/
void Q_EXTERNAL_CALL CL_GlobalHighScores_f( void )
{
	if ( Cmd_Argc() != 3) {
		Com_Printf( "usage: globalhighscores [version] [char slot]\n");
		return;	
	}

	HTTP_PostUrl( va("http://%s/user/scores/version/%d/slot/%d",AUTHORIZE_SERVER_NAME, atoi(Cmd_Argv(1)), atoi(Cmd_Argv(2))), CL_GetHighScores_response, 0, 0 );
}

#endif


/*
==================
CL_UpdateServer_f
==================
*/
void Q_EXTERNAL_CALL CL_UpdateServer_f(void)
{
	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: updateserver <status>\n" );
		return;
	}
	switch(atoi(Cmd_Argv(1)))
	{
		case 2:
			{
#ifdef USE_IRC
			cvar_t *channel = Cvar_Get("sv_ircchannel", "", 0);
			Net_IRC_SendMessage(va("mode %s +s", channel->string) );
#endif
			}
			break;
	}
}

/*
==================
CL_GetServers_f
==================
*/
void Q_EXTERNAL_CALL CL_GetServers_f(void)
{
	if ( Cmd_Argc() != 1 ) {
		Com_Printf( "usage: getservers\n" );
		return;
	}
#ifdef USE_IRC
	Net_IRC_ListServers();
#endif

}


/*
==================
CL_OpenUrl_f
==================
*/
void Q_EXTERNAL_CALL CL_OpenUrl_f( void )
{
	const char *url;

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "Usage: openurl <url>\n" );
		return;
	}

	url = Cmd_Argv( 1 );
	
	{
		/*
			FixMe: URL sanity checks.

			Random sanity checks. Scott: if you've got some magic URL
			parsing and validating functions USE THEM HERE, this code
			is a placeholder!!!
		*/
		int i;
		const char *u;

		const char *allowPrefixes[] = { "http://", "https://", "" };
		const char *allowDomains[2] = { "www.playspacetrader.com", 0 };

#ifdef USE_WEBAUTH
		allowDomains[1] = AUTHORIZE_SERVER_NAME;
#endif

		u = url;
		for( i = 0; i < lengthof( allowPrefixes ); i++ )
		{
			const char *p = allowPrefixes[i];
			size_t len = strlen( p );
			if( Q_strncmp( u, p, len ) == 0 )
			{
				u += len;
				break;
			}
		}

		if( i == lengthof( allowPrefixes ) )
		{
			/*
				This really won't ever hit because of the "" at the end
				of the allowedPrefixes array. As I said above, placeholder
				code: fix it later!
			*/
			Com_Printf( "Invalid URL prefix.\n" );
			return;
		}

		for( i = 0; i < lengthof( allowDomains ); i++ )
		{
			size_t len;
			const char *d = allowDomains[i];
			if ( !d )
				break;

			len = strlen( d );
			if( Q_strncmp( u, d, len ) == 0 )
			{
				u += len;
				break;
			}
		}

		if( i == lengthof( allowDomains ) )
		{
			Com_Printf( "Invalid domain.\n" );
			return;
		}
		/* my kingdom for a regex */
		for (i=0; i < strlen(url); i++)
		{
			if ( !(
				(url[i] >= 'a' && url[i] <= 'z') || // lower case alpha 
				(url[i] >= 'A' && url[i] <= 'Z') || // upper case alpha
				(url[i] >= '0' && url[i] <= '9') || //numeric
				(url[i] == '/') || (url[i] == ':' ) || // / and : chars
				(url[i] == '.' ) || (url[i] == '&') || // . and & chars
				(url[i] == ';' )						// ; char
				) ) {
				Com_Printf("Invalid URL\n");
				return;
			}
		}
	}

	if( !Sys_OpenUrl( url ) )
		Com_Printf( "System error opening URL\n" );
}

#ifdef USE_AUTOPATCH
/*
==================
CL_GetVersion_response
==================
*/
static int QDECL CL_GetVersion_response( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_WRITE ) {
		char	key		[ MAX_INFO_KEY ];
		char	value	[ MAX_INFO_VALUE ];

		for ( ;; )
		{
			Info_NextPair( &buffer, key, value );
			if ( key[ 0 ] == '\0' )
				break;

			Cvar_Set( key, value );
		}

		Cbuf_ExecuteText( EXEC_INSERT, "sync_all 0\n" );
	}

	return 1;
}


/*
=================
CL_CheckVersion_f
=================
*/
void Q_EXTERNAL_CALL CL_CheckVersion_f(void)
{
	if ( com_webhost && com_webhost->string && com_webhost->string[ 0 ] ) {
		HTTP_GetUrl( va("%s/version/%s", com_webhost->string ,OS_STRING), CL_GetVersion_response, 0, 0);
	}
}

#endif


/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

/*
==================
CL_ForwardToServer_f
==================
*/
void Q_EXTERNAL_CALL CL_ForwardToServer_f( void )
{
	if ( cls.state != CA_ACTIVE || clc.demoplaying ) {
		Com_Printf ("Not connected to a server.\n");
		return;
	}
	
	// don't forward the first argument
	if ( Cmd_Argc() > 1 ) {
		CL_AddReliableCommand( Cmd_Args() );
	}
}

/*
==================
CL_Setenv_f

Mostly for controlling voodoo environment variables
==================
*/
void Q_EXTERNAL_CALL CL_Setenv_f( void ) {
	int argc = Cmd_Argc();

	if ( argc > 2 ) {
		char buffer[1024];
		int i;

		strcpy( buffer, Cmd_Argv(1) );
		strcat( buffer, "=" );

		for ( i = 2; i < argc; i++ ) {
			strcat( buffer, Cmd_Argv( i ) );
			strcat( buffer, " " );
		}

		putenv( buffer );
	} else if ( argc == 2 ) {
		char *env = getenv( Cmd_Argv(1) );

		if ( env ) {
			Com_Printf( "%s=%s\n", Cmd_Argv(1), env );
		} else {
			Com_Printf( "%s undefined\n", Cmd_Argv(1), env );
		}
	}
}


/*
==================
CL_Disconnect_f
==================
*/
void Q_EXTERNAL_CALL CL_Disconnect_f( void ) {
	SCR_StopCinematic();
	if ( cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC ) {
		Com_Error (ERR_DISCONNECT, "Disconnected from server");
	}
}


/*
================
CL_Reconnect_f

================
*/
void Q_EXTERNAL_CALL CL_Reconnect_f( void )
{
	if ( !strlen( cls.servername ) || !strcmp( cls.servername, "localhost" ) ) {
		Com_Printf( "Can't reconnect to localhost.\n" );
		return;
	}
	Cbuf_AddText( va("connect %s\n", cls.servername ) );
}

/*
================
CL_Connect_f

================
*/
void Q_EXTERNAL_CALL CL_Connect_f( void )
{
	char	*server;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: connect [server]\n");
		return;	
	}

	// clear any previous "server full" type messages
	clc.serverMessage[0] = 0;

	server = Cmd_Argv (1);

	if ( com_sv_running->integer && !strcmp( server, "localhost" ) ) {
		// if running a local server, kill it
		SV_Shutdown( "Server quit\n" );
	}

	sql_prepare( &cl.db, "SELECT channel FROM servers SEARCH addr $1 WHERE channel!-''");
	sql_bindtext( &cl.db, 1, server );
	if ( sql_step( &cl.db ) ) {
#ifdef USE_IRC
		Net_IRC_JoinChannel( sql_columnastext( &cl.db, 0 ) );
#endif
	}
	sql_done( &cl.db );

	// make sure a local server is killed
	Cvar_Set( "sv_killserver", "1" );
	SV_Frame( 0 );

	CL_Disconnect( qtrue );
	Con_Close();

	/* MrE: 2000-09-13: now called in CL_DownloadsComplete
	CL_FlushMemory( );
	*/

	Q_strncpyz( cls.servername, server, sizeof(cls.servername) );

	if (!NET_StringToAdr( cls.servername, &clc.serverAddress) ) {
		Com_Printf ("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		return;
	}
	if (clc.serverAddress.port == 0) {
		clc.serverAddress.port = BigShort( PORT_SERVER );
	}
	Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", cls.servername,
		clc.serverAddress.ip[0], clc.serverAddress.ip[1],
		clc.serverAddress.ip[2], clc.serverAddress.ip[3],
		BigShort( clc.serverAddress.port ) );

	// if we aren't playing on a lan, we need to authenticate
	// with the cd key
	if ( NET_IsLocalAddress( clc.serverAddress ) ) {
		cls.state = CA_CHALLENGING;
	} else {
		cls.state = CA_CONNECTING;
	}

	cls.keyCatchers = 0;
	clc.connectTime = -99999;	// CL_CheckForResend() will fire immediately
	clc.connectPacketCount = 0;

	// server connection string
	Cvar_Set( "cl_currentServerAddress", server );
}

#define MAX_RCON_MESSAGE 1024

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void Q_EXTERNAL_CALL CL_Rcon_f( void ) {
	char	message[MAX_RCON_MESSAGE];
	netadr_t	to;

	if ( !rcon_client_password->string ) {
		Com_Printf ("You must set 'rconpassword' before\n"
					"issuing an rcon command.\n");
		return;
	}

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;

	Q_strcat (message, MAX_RCON_MESSAGE, "rcon ");

	Q_strcat (message, MAX_RCON_MESSAGE, rcon_client_password->string);
	Q_strcat (message, MAX_RCON_MESSAGE, " ");

	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
	Q_strcat (message, MAX_RCON_MESSAGE, Cmd_Cmd()+5);

	if ( cls.state >= CA_CONNECTED ) {
		to = clc.netchan.remoteAddress;
	} else {
		if (!strlen(rconAddress->string)) {
			Com_Printf ("You must either be connected,\n"
						"or set the 'rconAddress' cvar\n"
						"to issue rcon commands\n");

			return;
		}
		NET_StringToAdr (rconAddress->string, &to);
		if (to.port == 0) {
			to.port = BigShort (PORT_SERVER);
		}
	}
	
	NET_SendPacket (NS_CLIENT, strlen(message)+1, message, to);
}

/*
=================
CL_SendPureChecksums
=================
*/
void CL_SendPureChecksums( void ) {
	const char *pChecksums;
	char cMsg[MAX_INFO_VALUE];
	int i;

	// if we are pure we need to send back a command with our referenced pk3 checksums
	pChecksums = "hello"; //FS_ReferencedPakPureChecksums();

	// "cp"
	// "Yf"
	Com_sprintf(cMsg, sizeof(cMsg), "Yf ");
	Q_strcat(cMsg, sizeof(cMsg), va("%d ", cl.serverId) );
	Q_strcat(cMsg, sizeof(cMsg), pChecksums);
	for (i = 0; i < 2; i++) {
		cMsg[i] += 10;
	}
	CL_AddReliableCommand( cMsg );
}


/*
=================
CL_ResetPureClientAtServer
=================
*/
void CL_ResetPureClientAtServer( void ) {
	CL_AddReliableCommand( va("vdr") );
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/
void Q_EXTERNAL_CALL CL_Vid_Restart_f( void ) {

	// don't let them loop during the restart
	S_StopAllSounds();
	// shutdown the UI
	CL_ShutdownUI( qtrue );
	// shutdown the CGame
	CL_ShutdownCGame();
	// shutdown the renderer and clear the renderer interface
	CL_ShutdownRef();
	// client is no longer pure untill new checksums are sent
	CL_ResetPureClientAtServer();
	// reinitialize the filesystem if the game directory or checksum has changed
	FS_ConditionalRestart( clc.checksumFeed );

	cls.rendererStarted = qfalse;
	cls.uiStarted = qfalse;
	cls.cgameStarted = qfalse;
	cls.soundRegistered = qfalse;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set( "cl_paused", "0" );

	// if not running a server clear the whole hunk
	if ( !com_sv_running->integer ) {
		// clear the whole hunk
		Hunk_Clear();
		CM_ClearPreloaderData();
	}
	else {
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	// initialize the renderer interface
	CL_InitRef();

	// startup all the client stuff
	CL_StartHunkUsers();

	// start the cgame if connected
	if ( cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC ) {
		cls.cgameStarted = qtrue;
		CL_InitCGame();
		// send pure checksums
		CL_SendPureChecksums();
	}
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
void Q_EXTERNAL_CALL CL_Snd_Restart_f( void ) {
	S_Shutdown();
	S_Init();

	CL_Vid_Restart_f();
}

/*
==================
CL_Configstrings_f
==================
*/
void Q_EXTERNAL_CALL CL_Configstrings_f( void )
{
	int		i;
	int		ofs;

	if ( cls.state != CA_ACTIVE ) {
		Com_Printf( "Not connected to a server.\n");
		return;
	}

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		ofs = cl.gameState.stringOffsets[ i ];
		if ( !ofs ) {
			continue;
		}
		Com_Printf( "%4i: %s\n", i, cl.gameState.stringData + ofs );
	}
}

/*
==============
CL_Clientinfo_f
==============
*/
void Q_EXTERNAL_CALL CL_Clientinfo_f( void )
{
	Com_Printf( "--------- Client Information ---------\n" );
	Com_Printf( "state: %i\n", cls.state );
	Com_Printf( "Server: %s\n", cls.servername );
	Com_Printf ("User info settings:\n");
	Info_Print( Cvar_InfoString( CVAR_USERINFO ) );
	Com_Printf( "--------------------------------------\n" );
}


//====================================================================

/*
=================
CL_DownloadsComplete

Called when all downloading has been completed
=================
*/
void CL_DownloadsComplete( void ) {

	if ( cls.state < CA_CONNECTING ) {
		//	a download has completed outside of a game
		return;
	}

	if ( cls.state == CA_ACTIVE ) {
		//	a download has completed while the game is playing
		//	inform the client that its download is complete
		Cbuf_ExecuteText( EXEC_INSERT, "donedl" );
		return;
	}

	// if we downloaded files we need to restart the file system
	if (clc.downloadRestart) {
		clc.downloadRestart = qfalse;

		FS_Restart(clc.checksumFeed); // We possibly downloaded a pak, restart the file system to load it

		// inform the server so we get new gamestate info
		CL_AddReliableCommand( "donedl" );

		// by sending the donedl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}

	// let the client game init and load data
	cls.state = CA_LOADING;

	// Pump the loop, this may change gamestate!
	Com_EventLoop();

	// if the gamestate was changed by calling Com_EventLoop
	// then we loaded everything already and we don't want to do it again.
	if ( cls.state != CA_LOADING ) {
		return;
	}

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// flush client memory and start loading stuff
	// this will also (re)load the UI
	// if this is a local client then only the client part of the hunk
	// will be cleared, note that this is done after the hunk mark has been set
	CL_FlushMemory();

	// initialize the CGame
	cls.cgameStarted = qtrue;
	CL_InitCGame();

	// set pure checksums
	CL_SendPureChecksums();

	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
}

/*
=================
CL_InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void CL_InitDownloads(void) {
		
#if 0
// if connecting to a server, force all paks to download first.
#ifdef USE_WEBHOST
	if ( !com_sv_running->integer || Cvar_VariableIntegerValue( "ui_singlePlayerActive" )==0  ) {

		if ( Com_SyncAll() ) {
			// if autodownloading is not enabled on the server
			cls.state = CA_CONNECTED;
			clc.downloadRestart = qtrue;
			return;
		}
	}
#endif
#endif

	// let the client game init and load data
	cls.state = CA_LOADING;

	CL_DownloadsComplete();
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend( void ) {
	int		port, i;
	char	info[MAX_INFO_STRING];
	char	data[MAX_INFO_STRING];

	// don't send anything if playing back a demo
	if ( clc.demoplaying ) {
		return;
	}

	// resend if we haven't gotten a reply yet
	if ( cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING ) {
		return;
	}

	if ( cls.realtime - clc.connectTime < RETRANSMIT_TIMEOUT ) {
		return;
	}

	clc.connectTime = cls.realtime;	// for retransmit requests
	clc.connectPacketCount++;


	switch ( cls.state ) {
	case CA_CONNECTING:
		// requesting a challenge
		//Always request auth
		//if ( !Sys_IsLANAddress( clc.serverAddress ) ) {
			//CL_RequestAuthorization();
		//}
		NET_OutOfBandPrint(NS_CLIENT, clc.serverAddress, "getchallenge");
		break;
		
	case CA_CHALLENGING:
		// sending back the challenge
		port = Cvar_VariableValue ("net_qport");

		Q_strncpyz( info, Cvar_InfoString( CVAR_USERINFO ), sizeof( info ) );
		Info_SetValueForKey( info, "protocol", PROTOCOL_VERSION );
		Info_SetValueForKey( info, "qport", va("%i", port ) );
		Info_SetValueForKey( info, "challenge", va("%i", clc.challenge ) );
		
		strcpy(data, "connect ");
    // TTimo adding " " around the userinfo string to avoid truncated userinfo on the server
    //   (Com_TokenizeString tokenizes around spaces)
    data[8] = '"';

		for(i=0;i<strlen(info);i++) {
			data[9+i] = info[i];	// + (clc.challenge)&0x3;
		}
    data[9+i] = '"';
		data[10+i] = 0;

    // NOTE TTimo don't forget to set the right data length!
		NET_OutOfBandData( NS_CLIENT, clc.serverAddress, (byte *) &data[0], i+10 );
		// the most current userinfo has been sent, so watch for any
		// newer changes to userinfo variables
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;

	default:
		Com_Error( ERR_FATAL, "CL_CheckForResend: bad cls.state" );
	}
}

/*
===================
CL_DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void CL_DisconnectPacket( netadr_t from ) {
	if ( cls.state < CA_AUTHORIZING ) {
		return;
	}

	// if not from our server, ignore it
	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) ) {
		return;
	}

	// if we have received packets within three seconds, ignore it
	// (it might be a malicious spoof)
	if ( cls.realtime - clc.lastPacketTime < 3000 ) {
		return;
	}

	// drop the connection
	Com_Printf( "Server disconnected for unknown reason\n" );
	Cvar_Set("com_errorMessage", "Server disconnected for unknown reason\n" );
	CL_Disconnect( qtrue );
}


/*
===================
CL_MotdPacket

===================
*/
void CL_MotdPacket( netadr_t from ) {
	char	*challenge;
	char	*info;

	// if not from our server, ignore it
	if ( !NET_CompareAdr( from, cls.updateServer ) ) {
		return;
	}

	info = Cmd_Argv(1);

	// check challenge
	challenge = Info_ValueForKey( info, "challenge" );
	if ( strcmp( challenge, cls.updateChallenge ) ) {
		return;
	}

	challenge = Info_ValueForKey( info, "motd" );

	Q_strncpyz( cls.updateInfoString, info, sizeof( cls.updateInfoString ) );
	Cvar_Set( "cl_motdString", challenge );
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket( netadr_t from, msg_t *msg ) {
	char	*s;
	char	*c;

	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );	// skip the -1

	s = MSG_ReadStringLine( msg );

	Cmd_TokenizeString( s );

	c = Cmd_Argv(0);

	Com_DPrintf ("CL packet %s: %s\n", NET_AdrToString(from), c);

	// challenge from the server we are connecting to
	if ( !Q_stricmp(c, "challengeResponse") ) {
		if ( cls.state != CA_CONNECTING ) {
			Com_Printf( "Unwanted challenge response received.  Ignored.\n" );
		} else {
			// start sending challenge repsonse instead of challenge request packets
			clc.challenge = atoi(Cmd_Argv(1));
			cls.state = CA_CHALLENGING;
			clc.connectPacketCount = 0;
			clc.connectTime = -99999;

			// take this address as the new server address.  This allows
			// a server proxy to hand off connections to multiple servers
			clc.serverAddress = from;
			Com_DPrintf ("challengeResponse: %d\n", clc.challenge);
		}
		return;
	}

	// server connection
	if ( !Q_stricmp(c, "connectResponse") ) {
		if ( cls.state >= CA_CONNECTED ) {
			Com_Printf ("Dup connect received.  Ignored.\n");
			return;
		}
		if ( cls.state != CA_CHALLENGING ) {
			Com_Printf ("connectResponse packet while not connecting.  Ignored.\n");
			return;
		}
		if ( !NET_CompareBaseAdr( from, clc.serverAddress ) ) {
			Com_Printf( "connectResponse from a different address.  Ignored.\n" );
			Com_Printf( "%s should have been %s\n", NET_AdrToString( from ), 
				NET_AdrToString( clc.serverAddress ) );
			return;
		}
		Netchan_Setup (NS_CLIENT, &clc.netchan, from, Cvar_VariableValue( "net_qport" ) );
		cls.state = CA_CONNECTED;
		clc.lastPacketSentTime = -9999;		// send first packet immediately
		return;
	}

	// server responding to an info broadcast
	if ( !Q_stricmp(c, "infoResponse") ) {
		CL_ServerInfoPacket( from, msg );
		return;
	}

	// server responding to a get playerlist
	if ( !Q_stricmp(c, "statusResponse") ) {
		CL_ServerStatusResponse( from, msg );
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if (!Q_stricmp(c, "disconnect")) {
		CL_DisconnectPacket( from );
		return;
	}

	// echo request from server
	if ( !Q_stricmp(c, "echo") ) {
		NET_OutOfBandPrint( NS_CLIENT, from, "%s", Cmd_Argv(1) );
		return;
	}

	// cd check
	if ( !Q_stricmp(c, "keyAuthorize") ) {
		// we don't use these now, so dump them on the floor
		return;
	}

	// global MOTD from id
	if ( !Q_stricmp(c, "motd") ) {
		CL_MotdPacket( from );
		return;
	}

	// echo request from server
	if ( !Q_stricmp(c, "print") ) {
		s = MSG_ReadString( msg );
		Q_strncpyz( clc.serverMessage, s, sizeof( clc.serverMessage ) );
		Com_Printf( "%s", s );
		return;
	}

	// error message from server
	if ( !Q_stricmp( c, "error" ) ) {
		s = MSG_ReadString( msg );
		if ( uivm )
			VM_Call( uivm, UI_SERVER_ERRORMESSAGE, s );
		return;
	}

	Com_DPrintf ("Unknown connectionless packet command.\n");
}


/*
=================
CL_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CL_PacketEvent( netadr_t from, msg_t *msg ) {
	int		headerBytes;

	clc.lastPacketTime = cls.realtime;

	if ( msg->cursize >= 4 && *(int *)msg->data == -1 ) {
		CL_ConnectionlessPacket( from, msg );
		return;
	}

	if ( cls.state < CA_CONNECTED ) {
		return;		// can't be a valid sequenced packet
	}

	if ( msg->cursize < 4 ) {
		Com_Printf ("%s: Runt packet\n",NET_AdrToString( from ));
		return;
	}

	//
	// packet from server
	//
	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) ) {
		Com_DPrintf ("%s:sequenced packet without connection\n"
			,NET_AdrToString( from ) );
		// FIXME: send a client disconnect?
		return;
	}

	if (!CL_Netchan_Process( &clc.netchan, msg) ) {
		return;		// out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in 
	// client messages, allowing the server to detect a dropped
	// gamestate
	clc.serverMessageSequence = LittleLong( *(int *)msg->data );

	clc.lastPacketTime = cls.realtime;
	CL_ParseServerMessage( msg );

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if ( clc.demorecording && !clc.demowaiting ) {
		CL_WriteDemoMessage( msg, headerBytes );
	}
}

/*
==================
CL_CheckTimeout

==================
*/
void CL_CheckTimeout( void )
{
	//
	// check timeout
	//
	if ( ( !cl_paused->integer || !sv_paused->integer ) 
		&& cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC 
		&& cls.realtime - clc.lastPacketTime > cl_timeout->value*1000) { 
		if (++cl.timeoutcount > 5) {    // timeoutcount saves debugger 
				Com_Printf ("\nServer connection timed out.\n"); 
				CL_Disconnect( qtrue ); 
				return; 
		}
	} else {
		cl.timeoutcount = 0;
	}
}


//============================================================================

/*
==================
CL_CheckUserinfo

==================
*/
void CL_CheckUserinfo( void ) {
	// don't add reliable commands when not yet connected
	if ( cls.state < CA_CHALLENGING ) {
		return;
	}
	// don't overflow the reliable command buffer when paused
	if ( cl_paused->integer ) {
		return;
	}
	// send a reliable userinfo update if needed
	if( cvar_modifiedFlags & CVAR_USERINFO )
	{
		char *info = Cvar_InfoString( CVAR_USERINFO );
		char cleanName[MAX_NAME_LENGTH];

		if( Q_CleanPlayerName( Info_ValueForKey( info, "name" ), cleanName, sizeof( cleanName ) ) )
			Info_SetValueForKey( info, "name", cleanName );

		CL_AddReliableCommand( va("userinfo \"%s\"", info ) );

		cvar_modifiedFlags &= ~CVAR_USERINFO;
	}

}

/*
==================
CL_Frame

==================
*/
void CL_Frame ( int msec ) {

	if ( !com_cl_running->integer ) {
		return;
	}

	if ( uivm ) {
		if ( cls.cddialog ) {
			// bring up the cd error dialog if needed
			cls.cddialog = qfalse;
			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NEED_CD );
		} else	if ( cls.state == CA_DISCONNECTED && !( cls.keyCatchers & KEYCATCH_UI )
			&& !com_sv_running->integer ) {
			// if disconnected, bring up the menu
			S_StopAllSounds();
			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
		}
	}

	// if recording an avi, lock to a fixed fps
	if ( cl_avidemo->integer && msec) {
		// save the current screen
		if ( cls.state == CA_ACTIVE || cl_forceavidemo->integer) {
			Cbuf_ExecuteText( EXEC_NOW, "screenshot silent\n" );
		}
		// fixed time for next frame'
		msec = (1000 / cl_avidemo->integer) * com_timescale->value;
		if (msec == 0) {
			msec = 1;
		}
	}
	
	// save the msec before checking pause
	cls.realFrametime = msec;

	// decide the simulation time
	cls.frametime = msec;

	cls.realtime += cls.frametime;

	if ( cl_timegraph->integer ) {
		SCR_DebugGraph ( cls.realFrametime * 0.25, 0 );
	}

	// see if we need to update any userinfo
	CL_CheckUserinfo();

	// if we haven't gotten a packet in a long time,
	// drop the connection
	CL_CheckTimeout();

	// send intentions now
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();

	// decide on the serverTime to render
	CL_SetCGameTime();

	// update the screen
	SCR_UpdateScreen();

	// update audio
	S_Update();

	// advance local effects for next frame
	SCR_RunCinematic();

	Con_RunConsole();

	cls.framecount++;
}


//============================================================================

/*
================
CL_RefPrintf

DLL glue
================
*/
void Q_EXTERNAL_CALL_VA CL_RefPrintf( int print_level, const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start (argptr,fmt);
	
	Q_vsnprintf( msg, sizeof( msg ) - 1, fmt, argptr );
	msg[sizeof( msg ) - 1] = 0;

	va_end (argptr);

	if ( print_level == PRINT_ALL ) {
		Com_Printf ("%s", msg);
	} else if ( print_level == PRINT_WARNING ) {
		Com_Printf (S_COLOR_YELLOW "%s", msg);		// yellow
	} else if ( print_level == PRINT_DEVELOPER ) {
		Com_DPrintf (S_COLOR_RED "%s", msg);		// red
	}
}



/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef( void ) {
	if( !re.Shutdown )
		return;

	re.Shutdown( qtrue );
	cls.rendererStarted = qfalse;
	Com_Memset( &re, 0, sizeof( re ) );
}

/*
============
CL_InitRenderer
============
*/
void CL_InitRenderer( void )
{
	if( cls.rendererStarted )
		return;

	cls.rendererStarted = qtrue;

	// this sets up the renderer and calls R_Init
	re.BeginRegistration( &cls.glconfig );
	Con_Resize( 0, -1, 0, 0, 0, 0 );
	Con_Resize( 1, -1, 0, 0, 0, 0 );

	// load character sets
	cls.whiteShader = re.RegisterShader( "*white" );
	cls.menu		= re.RegisterShader( "ui/assets/menubrushed" );
	cls.font		= re.RegisterFont( "profont" );

#ifdef USE_CALLHOME
	HTTP_PostUrl( "http://www.playspacetrader.com/user/log", 0, 0, 
				"message="
				"[%s] %s(%s)\n"
				, Cvar_VariableString( "sys_osstring" )
				, cls.glconfig.renderer_string
				, cls.glconfig.version_string );
#endif
}

/*
============================
CL_StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void CL_StartHunkUsers( void ) {
	if (!com_cl_running) {
		return;
	}

	if ( !com_cl_running->integer ) {
		return;
	}

	if( !cls.rendererStarted )
	{
		CL_InitRenderer();
		cls.rendererStarted = qtrue;
	}

	if ( !cls.soundStarted ) {
		cls.soundStarted = qtrue;
		S_Init();
	}

	if ( !cls.soundRegistered ) {
		cls.soundRegistered = qtrue;
		S_BeginRegistration();
	}

	if ( !cls.uiStarted ) {
		cls.uiStarted = qtrue;
		CL_InitUI();
	}
}

/*
============
CL_RefMalloc
============
*/
void* Q_EXTERNAL_CALL CL_RefMalloc( int size )
{
	return Z_TagMalloc( size, TAG_RENDERER );
}

int Q_EXTERNAL_CALL CL_ScaledMilliseconds( void )
{
	return Sys_Milliseconds()*com_timescale->value;
}

// be_aas_main.c
void Q_EXTERNAL_CALL AAS_DrawDebugSurface( void (Q_EXTERNAL_CALL *drawPoly)( int color, int numPoints, float *points ) );


/*
============
CL_InitRef
============
*/

void CL_InitRef( void )
{
#ifdef DEVELOPER
	intptr_t (QDECL *entryPoint)( int callNum, ... ) = 0;
	char dllPath[ MAX_QPATH ];
#endif
	refimport_t	ri;

	Com_Printf( "----- Initializing Renderer ----\n" );

#ifdef DEVELOPER
	Sys_LoadDll( com_renderer->string, dllPath, &entryPoint, 0 );
#endif

	ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ExecuteText = Cbuf_ExecuteText;
	ri.Printf = CL_RefPrintf;
	ri.Error = Com_Error;
	ri.Milliseconds = CL_ScaledMilliseconds;
	ri.Malloc = CL_RefMalloc;
	ri.Free = Z_Free;
#ifdef HUNK_DEBUG
	ri.Hunk_AllocDebug = Hunk_AllocDebug;
#else
	ri.Hunk_Alloc = Hunk_Alloc;
#endif
	ri.Hunk_AllocateTempMemory = Hunk_AllocateTempMemory;
	ri.Hunk_FreeTempMemory = Hunk_FreeTempMemory;
	ri.Hunk_AllocateFrameMemory = Hunk_FrameAlloc;
	ri.CM_DrawDebugSurface = CM_DrawDebugSurface;
	ri.AAS_DrawDebugSurface = AAS_DrawDebugSurface;
	
	ri.FS_ReadFile = FS_ReadFile;
	ri.FS_FreeFile = FS_FreeFile;

	ri.FS_WriteFile = FS_WriteFile;
	
	ri.FS_FOpenFile = FS_FOpenFileByMode;
	ri.FS_Read = FS_Read;
	ri.FS_Write = FS_Write;
	ri.FS_Seek = FS_Seek;
	ri.FS_FCloseFile = FS_FCloseFile;
	
	ri.FS_FreeFileList = FS_FreeFileList;
	ri.FS_ListFiles = FS_ListFiles;
	ri.FS_FileExists = FS_FileExists;
	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;

	// cinematic stuff

	ri.CIN_UploadCinematic = CIN_UploadCinematic;
	ri.CIN_PlayCinematic = CIN_PlayCinematic;
	ri.CIN_RunCinematic = CIN_RunCinematic;

	ri.CM_ClusterPVS = CM_ClusterPVS;
	ri.PlatformGetVars = Sys_PlatformGetVars;
	ri.Sys_LowPhysicalMemory = Sys_LowPhysicalMemory;
	ri.Con_GetText = Con_GetText;

#ifdef DEVELOPER
	if( entryPoint )
	{
		if ( !entryPoint( 1, REF_API_VERSION, &ri, &re ) )
			Com_Error (ERR_FATAL, "Couldn't initialize refresh" );
	}
	else
#endif
	{
		RE_Startup( REF_API_VERSION, &re, &ri );
	}

	Com_Printf( "-------------------------------\n");

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set( "cl_paused", "0" );
}


//===========================================================================================


void Q_EXTERNAL_CALL CL_SetModel_f( void )
{
	char	*arg;
	char	name[256];

	arg = Cmd_Argv( 1 );
	if (arg[0]) {
		Cvar_Set( "model", arg );
		Cvar_Set( "headmodel", arg );
	} else {
		Cvar_VariableStringBuffer( "model", name, sizeof(name) );
		Com_Printf("model is set to %s\n", name);
	}
}

#ifdef DEVELOPER
static void Q_EXTERNAL_CALL CL_sql_f( void )
{
	sql_prompt( &cl.db, Cmd_Argv(1) );
}
#endif

static void Q_EXTERNAL_CALL CL_sql_export_f( void ) {
	if ( Cmd_Argc() == 1 ) {
		sql_prepare( &cl.db, "SELECT value FROM missions SEARCH key 'db_name';" );
		if ( sql_step( &cl.db ) ) {
			sql_export( sql_columnastext( &cl.db, 0) );
		}
		sql_done( &cl.db);
	} else {
		sql_export( Cmd_Argv(1) );
	}
}
#ifdef DEVELOPER
static void Q_EXTERNAL_CALL CL_sql_save_f( void ) {
	if ( Cmd_Argc() == 1 ) {
		sql_prepare( &cl.db, "SELECT value FROM missions SEARCH key 'db_name';" );
		if ( sql_step( &cl.db ) ) {
			sql_save( sql_columnastext( &cl.db, 0) );
		}
		sql_done( &cl.db);
	} else {
		sql_save( Cmd_Argv(1) );
	}
}
#endif
/*
====================
CL_Init
====================
*/
void CL_Init( void ) {
	Com_Printf( "----- Client Initialization -----\n" );

	Con_Init ();	

	CL_ClearState ();

	cls.state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED

	cls.realtime = 0;

	CL_InitInput ();

	//
	// register our variables
	//
	cl_noprint = Cvar_Get( "cl_noprint", "0", 0 );
	cl_motd = Cvar_Get ("cl_motd", "1", 0);
	cl_timeout = Cvar_Get( "cl_timeout", "200", 0 );
	cl_timeNudge = Cvar_Get ("cl_timeNudge", "30", CVAR_TEMP );
	cl_shownet = Cvar_Get ("cl_shownet", "0", CVAR_TEMP );
	cl_showSend = Cvar_Get ("cl_showSend", "0", CVAR_TEMP );
	cl_showTimeDelta = Cvar_Get ("cl_showTimeDelta", "0", CVAR_TEMP );
	cl_freezeDemo = Cvar_Get ("cl_freezeDemo", "0", CVAR_TEMP );
	rcon_client_password = Cvar_Get ("rconPassword", "", CVAR_TEMP );
	cl_activeAction = Cvar_Get( "activeAction", "", CVAR_TEMP );

	cl_timedemo = Cvar_Get ("timedemo", "0", 0);
	cl_avidemo = Cvar_Get ("cl_avidemo", "0", 0);
	cl_forceavidemo = Cvar_Get ("cl_forceavidemo", "0", 0);

	rconAddress = Cvar_Get ("rconAddress", "", 0);

	cl_yawspeed = Cvar_Get ("cl_yawspeed", "140", CVAR_ARCHIVE);
	cl_pitchspeed = Cvar_Get ("cl_pitchspeed", "140", CVAR_ARCHIVE);
	cl_anglespeedkey = Cvar_Get ("cl_anglespeedkey", "1.5", 0);

	cl_maxpackets = Cvar_Get ("cl_maxpackets", "30", CVAR_ARCHIVE );
	cl_packetdup = Cvar_Get ("cl_packetdup", "1", CVAR_ARCHIVE );

	cl_run = Cvar_Get ("cl_run", "1", CVAR_ARCHIVE);
	cl_sensitivity = Cvar_Get ("sensitivity", "5", CVAR_ARCHIVE);
	cl_mouseAccel = Cvar_Get ("cl_mouseAccel", "0", CVAR_ARCHIVE);
	cl_freelook = Cvar_Get( "cl_freelook", "1", CVAR_ARCHIVE );

	cl_showMouseRate = Cvar_Get ("cl_showmouserate", "0", 0);

	cl_conXOffset = Cvar_Get ("cl_conXOffset", "0", 0);
	cl_conYOffset = Cvar_Get ("cl_conYOffset", "0", 0);
	cl_conWidth = Cvar_Get ("cl_conWidth", "640", 0);
	cl_conHeight = Cvar_Get ("cl_conHeight", "32", 0);
#ifdef MACOS_X
        // In game video is REALLY slow in Mac OS X right now due to driver slowness
	cl_inGameVideo = Cvar_Get ("r_inGameVideo", "0", CVAR_ARCHIVE);
#else
	cl_inGameVideo = Cvar_Get ("r_inGameVideo", "1", CVAR_ARCHIVE);
#endif

	cl_serverStatusResendTime = Cvar_Get ("cl_serverStatusResendTime", "750", 0);

#ifdef USE_WEBAUTH
	cl_authserver = Cvar_Get( "cl_authserver", "www.playspacetrader.com", CVAR_INIT );
#endif
	cl_ircport = Cvar_Get( "cl_ircport", "27399", CVAR_INIT );

	cl_lang = Cvar_Get( "cl_lang", "en-US", CVAR_ARCHIVE | CVAR_LATCH );

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	Cvar_Get ("cg_autoswitch", "1", CVAR_ARCHIVE);
	
#ifdef USE_WEBAUTH
	// Initialize ui_logged_in to -1(not logged in)
	// -2 - login in progress
	// 0 - invalid login
	// 1 - logged in
	Cvar_Get ("ui_logged_in", "-1", CVAR_ROM);
	//force ui_logged in to -1 so we can't reset it on the command line
	Cvar_Set( "ui_logged_in", "-1" );
#endif

	m_pitch = Cvar_Get ("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw = Cvar_Get ("m_yaw", "0.022", CVAR_ARCHIVE);
	m_forward = Cvar_Get ("m_forward", "0.25", CVAR_ARCHIVE);
	m_side = Cvar_Get ("m_side", "0.25", CVAR_ARCHIVE);
#ifdef MACOS_X
        // Input is jittery on OS X w/o this
	m_filter = Cvar_Get ("m_filter", "1", CVAR_ARCHIVE);
#else
	m_filter = Cvar_Get ("m_filter", "0", CVAR_ARCHIVE);
#endif

	cl_motdString = Cvar_Get( "cl_motdString", "", CVAR_ROM );

	Cvar_Get( "cl_maxPing", "800", CVAR_ARCHIVE );


	// userinfo
	Cvar_Get ("name", "UnnamedPlayer", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("rate", "10000", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("model", "mpc1", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("headmodel", "mpc1", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("color1",  "4", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("color2", "5", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("teamtask", "0", CVAR_USERINFO );
	Cvar_Get ("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE );

	Cvar_Get ("password", "", CVAR_USERINFO);
	Cvar_Get ("cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE );
	
	Cvar_Get ("slot", "0", CVAR_USERINFO );


	// cgame might not be initialized before menu is used
	Cvar_Get ("cg_viewsize", "100", CVAR_ARCHIVE );

	//
	// register our commands
	//
	Cmd_AddCommand ("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand ("configstrings", CL_Configstrings_f);
	Cmd_AddCommand ("clientinfo", CL_Clientinfo_f);
	Cmd_AddCommand ("snd_restart", CL_Snd_Restart_f);
	Cmd_AddCommand ("vid_restart", CL_Vid_Restart_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("demo", CL_PlayDemo_f);
	Cmd_AddCommand ("cinematic", CL_PlayCinematic_f);
	Cmd_AddCommand ("stoprecord", CL_StopRecord_f);
	Cmd_AddCommand ("connect", CL_Connect_f);
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);
	Cmd_AddCommand ("localservers", CL_LocalServers_f);
#ifdef USE_WEBAUTH
	Cmd_AddCommand ("login", CL_Login_f);
	Cmd_AddCommand ("highscore", CL_Highscore_f);
	Cmd_AddCommand ("forgotpassword", CL_ForgotPassword_f);
	Cmd_AddCommand ("createcharacter", CL_CreateCharacter_f);
	Cmd_AddCommand ("deletecharacter", CL_DeleteCharacter_f);
	Cmd_AddCommand ("getaccount", CL_GetAccount_f);
	Cmd_AddCommand ("globalhighscores", CL_GlobalHighScores_f);
#endif
	Cmd_AddCommand ("updateserver", CL_UpdateServer_f);
	Cmd_AddCommand ("getservers", CL_GetServers_f);
	Cmd_AddCommand ("openurl", CL_OpenUrl_f );
#ifdef USE_AUTOPATCH
	Cmd_AddCommand ("check_updates", CL_CheckVersion_f);
#endif
#ifdef USE_IRC
	Cmd_AddCommand ("irc", CL_Irc_f);
#endif
//	Cmd_AddCommand ("getauth", CL_RequestAuthorization);
	Cmd_AddCommand ("rcon", CL_Rcon_f);
	Cmd_AddCommand ("setenv", CL_Setenv_f );
	Cmd_AddCommand ("ping", CL_Ping_f );
	Cmd_AddCommand ("serverstatus", CL_ServerStatus_f );
	Cmd_AddCommand ("showip", CL_ShowIP_f );
	Cmd_AddCommand ("model", CL_SetModel_f );
	Cmd_AddCommand ("dumpreferences", CL_DumpMapReferences );
#ifdef DEVELOPER
	Cmd_AddCommand ("sql_c", CL_sql_f );
	Cmd_AddCommand ("sql_save", CL_sql_save_f );
#endif
	Cmd_AddCommand ("sql_export", CL_sql_export_f );

	CL_InitRef();

	SCR_Init ();

	Cbuf_Execute ();

	Cvar_Set( "cl_running", "1" );

	Com_Printf( "language: %s\n", cl_lang->string );
	Com_Printf( "----- Client Initialization Complete -----\n" );
}


/*
===============
CL_Shutdown

===============
*/
void CL_Shutdown( void ) {
	static qboolean recursive = qfalse;
	
	Com_Printf( "----- CL_Shutdown -----\n" );

	if ( recursive ) {
		printf ("recursive shutdown\n");
		return;
	}
	recursive = qtrue;

	CL_Disconnect( qtrue );

	
	CL_ShutdownRef();
	
	CL_ShutdownUI( qfalse );

	S_Shutdown();
	
	Cmd_RemoveCommand ("cmd");
	Cmd_RemoveCommand ("configstrings");
	Cmd_RemoveCommand ("userinfo");
	Cmd_RemoveCommand ("snd_restart");
	Cmd_RemoveCommand ("vid_restart");
	Cmd_RemoveCommand ("disconnect");
	Cmd_RemoveCommand ("record");
	Cmd_RemoveCommand ("demo");
	Cmd_RemoveCommand ("cinematic");
	Cmd_RemoveCommand ("stoprecord");
	Cmd_RemoveCommand ("connect");
	Cmd_RemoveCommand ("localservers");
	Cmd_RemoveCommand ("globalservers");
	Cmd_RemoveCommand ("rcon");
	Cmd_RemoveCommand ("setenv");
	Cmd_RemoveCommand ("ping");
	Cmd_RemoveCommand ("serverstatus");
	Cmd_RemoveCommand ("showip");
	Cmd_RemoveCommand ("model");

	Cvar_Set( "cl_running", "0" );

	recursive = qfalse;

	Com_Memset( &cls, 0, sizeof( cls ) );

	Com_Printf( "-----------------------\n" );

}

void CL_ServerAddToRefreshList( const char * info, const char * channel ) {

	char * addr = Info_ValueForKey( info, "addr" );
	netadr_t	to;
	
	Com_Memset( &to, 0, sizeof(netadr_t) );
	
	if ( !NET_StringToAdr( addr, &to ) ) {
		return;
	}
	
	NET_OutOfBandPrint( NS_CLIENT, to, "getinfo xxx" );

	//	update server's info
	sql_prepare	( &cl.db, "UPDATE OR INSERT servers CS $1 SEARCH addr $2;" );
	sql_bindtext( &cl.db, 1, va("%s\\channel\\%s", info, channel) );
	sql_bindtext( &cl.db, 2, addr );
	sql_step	( &cl.db );
	sql_done	( &cl.db );
}

/*
===================
CL_ServerInfoPacket
===================
*/
void CL_ServerInfoPacket( netadr_t from, msg_t *msg ) {
	const char * info = MSG_ReadString( msg );
	const char * addr = NET_AdrToString( from );

	// if this isn't the correct protocol version, ignore it
	if ( Q_stricmp( Info_ValueForKey( info, "protocol" ), PROTOCOL_VERSION ) ) {
		Com_DPrintf( "Different protocol info packet: %s\n", info );
		return;
	}

	//	attempt to update the server
	sql_prepare ( &cl.db, "UPDATE servers SET ping=IF (start>0 ) THEN max(1,?2-start) ELSE ping,start=0,lastupdate=?2 SEARCH addr $1;" );
	sql_bindtext( &cl.db, 1, addr );
	sql_bindint ( &cl.db, 2, cls.realtime );
	sql_step	( &cl.db );

	if ( sql_done ( &cl.db ) == 0 ) {

		// new server acquired from a broadcast...
		sql_prepare ( &cl.db, "INSERT INTO servers(addr,ping,lastupdate) VALUES($1,?2,?3);" );
		sql_bindtext( &cl.db, 1, addr );
		sql_bindint ( &cl.db, 2, cls.realtime - cls.broadcastTime );
		sql_bindint ( &cl.db, 3, cls.realtime );
		sql_step	( &cl.db );
		sql_done	( &cl.db );
	}

	//	update server's info
	sql_prepare	( &cl.db, "UPDATE servers CS $1 SEARCH addr $2;" );
	sql_bindtext( &cl.db, 1, info );
	sql_bindtext( &cl.db, 2, addr );
	sql_step	( &cl.db );
	sql_done	( &cl.db );
}

/*
===================
CL_GetServerStatus
===================
*/
serverStatus_t *CL_GetServerStatus( netadr_t from ) {
	serverStatus_t *serverStatus;
	int i, oldest, oldestTime;

	serverStatus = NULL;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if ( NET_CompareAdr( from, cl_serverStatusList[i].address ) ) {
			return &cl_serverStatusList[i];
		}
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if ( cl_serverStatusList[i].retrieved ) {
			return &cl_serverStatusList[i];
		}
	}
	oldest = -1;
	oldestTime = 0;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if (oldest == -1 || cl_serverStatusList[i].startTime < oldestTime) {
			oldest = i;
			oldestTime = cl_serverStatusList[i].startTime;
		}
	}
	if (oldest != -1) {
		return &cl_serverStatusList[oldest];
	}
	serverStatusCount++;
	return &cl_serverStatusList[serverStatusCount & (MAX_SERVERSTATUSREQUESTS-1)];
}

/*
===================
CL_ServerStatus
===================
*/
int CL_ServerStatus( char *serverAddress, char *serverStatusString, int maxLen ) {
	int i;
	netadr_t	to;
	serverStatus_t *serverStatus;

	// if no server address then reset all server status requests
	if ( !serverAddress ) {
		for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
			cl_serverStatusList[i].address.port = 0;
			cl_serverStatusList[i].retrieved = qtrue;
		}
		return qfalse;
	}
	// get the address
	if ( !NET_StringToAdr( serverAddress, &to ) ) {
		return qfalse;
	}
	serverStatus = CL_GetServerStatus( to );
	// if no server status string then reset the server status request for this address
	if ( !serverStatusString ) {
		serverStatus->retrieved = qtrue;
		return qfalse;
	}

	// if this server status request has the same address
	if ( NET_CompareAdr( to, serverStatus->address) ) {
		// if we recieved an response for this server status request
		if (!serverStatus->pending) {
			Q_strncpyz(serverStatusString, serverStatus->string, maxLen);
			serverStatus->retrieved = qtrue;
			serverStatus->startTime = 0;
			return qtrue;
		}
		// resend the request regularly
		else if ( serverStatus->startTime < Com_Milliseconds() - cl_serverStatusResendTime->integer ) {
			serverStatus->print = qfalse;
			serverStatus->pending = qtrue;
			serverStatus->retrieved = qfalse;
			serverStatus->time = 0;
			serverStatus->startTime = Com_Milliseconds();
			NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );
			return qfalse;
		}
	}
	// if retrieved
	else if ( serverStatus->retrieved ) {
		serverStatus->address = to;
		serverStatus->print = qfalse;
		serverStatus->pending = qtrue;
		serverStatus->retrieved = qfalse;
		serverStatus->startTime = Com_Milliseconds();
		serverStatus->time = 0;
		NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );
		return qfalse;
	}
	return qfalse;
}

/*
===================
CL_ServerStatusResponse
===================
*/
void CL_ServerStatusResponse( netadr_t from, msg_t *msg ) {
	char	*s;
	char	info[MAX_INFO_STRING];
	int		i, l, score, ping;
	int		len;
	serverStatus_t *serverStatus;

	serverStatus = NULL;
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if ( NET_CompareAdr( from, cl_serverStatusList[i].address ) ) {
			serverStatus = &cl_serverStatusList[i];
			break;
		}
	}
	// if we didn't request this server status
	if (!serverStatus) {
		return;
	}

	s = MSG_ReadStringLine( msg );

	len = 0;
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "%s", s);

	if (serverStatus->print) {
		Com_Printf("Server settings:\n");
		// print cvars
		while (*s) {
			for (i = 0; i < 2 && *s; i++) {
				if (*s == '\\')
					s++;
				l = 0;
				while (*s) {
					info[l++] = *s;
					if (l >= MAX_INFO_STRING-1)
						break;
					s++;
					if (*s == '\\') {
						break;
					}
				}
				info[l] = '\0';
				if (i) {
					Com_Printf("%s\n", info);
				}
				else {
					Com_Printf("%-24s", info);
				}
			}
		}
	}

	len = strlen(serverStatus->string);
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "\\");

	if (serverStatus->print) {
		Com_Printf("\nPlayers:\n");
		Com_Printf("num: score: ping: name:\n");
	}
	for (i = 0, s = MSG_ReadStringLine( msg ); *s; s = MSG_ReadStringLine( msg ), i++) {

		len = strlen(serverStatus->string);
		Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "\\%s", s);

		if (serverStatus->print) {
			score = ping = 0;
			sscanf(s, "%d %d", &score, &ping);
			s = strchr(s, ' ');
			if (s)
				s = strchr(s+1, ' ');
			if (s)
				s++;
			else
				s = "unknown";
			Com_Printf("%-2d   %-3d    %-3d   %s\n", i, score, ping, s );
		}
	}
	len = strlen(serverStatus->string);
	Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string)-len, "\\");

	serverStatus->time = Com_Milliseconds();
	serverStatus->address = from;
	serverStatus->pending = qfalse;
	if (serverStatus->print) {
		serverStatus->retrieved = qtrue;
	}
}

/*
==================
CL_LocalServers_f
==================
*/
void Q_EXTERNAL_CALL CL_LocalServers_f( void )
{
	char		*message;
	int			i, j;
	netadr_t	to;

	cls.broadcastTime = cls.realtime;

	Com_Printf( "Scanning for servers on the local network...\n");

	Com_Memset( &to, 0, sizeof( to ) );

	// The 'xxx' in the message is a challenge that will be echoed back
	// by the server.  We don't care about that here, but master servers
	// can use that to prevent spoofed server responses from invalid ip
	message = "\377\377\377\377getinfo xxx";

	// send each message twice in case one is dropped
	for ( i = 0 ; i < 2 ; i++ ) {
		// send a broadcast packet on each server port
		// we support multiple server ports so a single machine
		// can nicely run multiple servers
		for ( j = 0 ; j < NUM_SERVER_PORTS ; j++ ) {
			to.port = BigShort( (short)(PORT_SERVER + j) );

			to.type = NA_BROADCAST;
			NET_SendPacket( NS_CLIENT, strlen( message ), message, to );

			to.type = NA_BROADCAST_IPX;
			NET_SendPacket( NS_CLIENT, strlen( message ), message, to );
		}
	}
}

#ifdef USE_IRC
/*
==================
CL_Irc_f
==================
*/
void Q_EXTERNAL_CALL CL_Irc_f ( void )
{
	Net_IRC_SendMessage(Cmd_Argv(1));
}
#endif
/*
==================
CL_Ping_f
==================
*/
void Q_EXTERNAL_CALL CL_Ping_f( void )
{
	netadr_t	to;
	char*		server;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: ping [server]\n");
		return;	
	}

	Com_Memset( &to, 0, sizeof(netadr_t) );

	server = Cmd_Argv(1);

	if ( !NET_StringToAdr( server, &to ) ) {
		return;
	}

	NET_OutOfBandPrint( NS_CLIENT, to, "getinfo xxx" );

	sql_prepare ( &cl.db, "UPDATE OR INSERT servers SET addr=$1,start=?2 SEARCH addr $1" );
	sql_bindtext( &cl.db, 1, server );
	sql_bindint ( &cl.db, 2, cls.realtime );
	sql_step	( &cl.db );
	sql_done	( &cl.db );
}

/*
==================
CL_UpdateVisiblePings_f
==================
*/
qboolean Q_EXTERNAL_CALL CL_UpdateVisiblePings_f( int source )
{
	//	spam servers that haven't responded in a while
	sql_prepare ( &cl.db, "SELECT addr FROM servers WHERE lastupdate < ?1-1000" );
	sql_bindint	( &cl.db, 1, cls.realtime );
	while( sql_step( &cl.db ) ){

		netadr_t adr;

		NET_StringToAdr(sql_columnastext( &cl.db, 0 ), &adr);
		NET_OutOfBandPrint( NS_CLIENT, adr, "getinfo xxx" );
	}
	sql_done	( &cl.db );

	sql_prepare ( &cl.db, "UPDATE servers SET start=?1 WHERE lastupdate < ?1-1000" );
	sql_bindint ( &cl.db, 1, cls.realtime );
	sql_step	( &cl.db );
	sql_done	( &cl.db );

	return qtrue;
}

/*
==================
CL_ServerStatus_f
==================
*/
void Q_EXTERNAL_CALL CL_ServerStatus_f( void )
{
	netadr_t	to;
	char		*server;
	serverStatus_t *serverStatus;

	Com_Memset( &to, 0, sizeof(netadr_t) );

	if ( Cmd_Argc() != 2 ) {
		if ( cls.state != CA_ACTIVE || clc.demoplaying ) {
			Com_Printf ("Not connected to a server.\n");
			Com_Printf( "Usage: serverstatus [server]\n");
			return;	
		}
		server = cls.servername;
	}
	else {
		server = Cmd_Argv(1);
	}

	if ( !NET_StringToAdr( server, &to ) ) {
		return;
	}

	NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );

	serverStatus = CL_GetServerStatus( to );
	serverStatus->address = to;
	serverStatus->print = qtrue;
	serverStatus->pending = qtrue;
}

/*
==================
CL_ShowIP_f
==================
*/
void Q_EXTERNAL_CALL CL_ShowIP_f( void )
{
	Sys_ShowIP();
}
