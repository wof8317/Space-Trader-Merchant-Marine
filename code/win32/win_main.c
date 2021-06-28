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
// win_main.c

#include "../client/client.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"
#include "resource.h"

#include "platform/pop_segs.h"
#include <setjmp.h>
#include "platform/push_segs.h"

#define	CD_BASEDIR	"quake3"
#define	CD_EXE		"quake3.exe"
#define	CD_BASEDIR_LINUX	"bin\\x86\\glibc-2.1"
#define	CD_EXE_LINUX "quake3"
#define MEM_THRESHOLD 96*1024*1024

static jmp_buf sys_exitframe;
static int sys_retcode;
static char sys_cmdline[BIG_INFO_STRING];
static char sys_exitstr[MAX_STRING_CHARS];

WinVars_t g_wv;
void* Q_EXTERNAL_CALL Sys_PlatformGetVars( void )
{
	return &g_wv;
}

qboolean Q_EXTERNAL_CALL Sys_LowPhysicalMemory( void )
{
	MEMORYSTATUS stat;
	GlobalMemoryStatus (&stat);
	return (stat.dwTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}

/*
==================
Sys_BeginProfiling
==================
*/
void Sys_BeginProfiling( void ) {
	// this is just used on the mac build
}

static void Sys_Exit( int retcode )
{
	sys_retcode = retcode;
	longjmp( sys_exitframe, -1 );
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void QDECL Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		text[4096];
    MSG        msg;

	va_start (argptr, error);
	vsnprintf (text, sizeof(text),error, argptr);
	va_end (argptr);

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

	Sys_SetErrorText( text );
	Sys_ShowConsole( 1, qtrue );

	timeEndPeriod( 1 );

	IN_Shutdown();

	// wait for the user to quit
	for( ; ; )
	{
		BOOL ret = GetMessage ( &msg, NULL, 0, 0 );
		if( ret == 0 || ret == -1 )
			Com_Quit_f();
		
		TranslateMessage( &msg );
      	DispatchMessage( &msg );
	}

	Sys_DestroyConsole();

	Q_strncpyz( sys_exitstr, text, sizeof( sys_exitstr ) ); 

	Sys_Exit( 1 );
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit( void )
{
	timeEndPeriod( 1 );
	IN_Shutdown();
	Sys_DestroyConsole();

	Sys_Exit( 0 );
}

void Host_RecordError( const char *msg );

void Sys_WriteDump( const char *fmt, ... )
{
#if defined( _WIN32 )

#ifndef DEVELOPER
	if( com_developer && com_developer->integer )
#endif
	{
		//this memory should live as long as the SEH is doing its thing...I hope
		static char msg[2048];
		va_list vargs;

		/*
			Do our own vsnprintf as using va's will change global state
			that might be pertinent to the dump.
		*/

		va_start( vargs, fmt );
		vsnprintf( msg, sizeof( msg ) - 1, fmt, vargs );
		va_end( vargs );

		msg[sizeof( msg ) - 1] = 0; //ensure null termination

		Host_RecordError( msg );
	}

#endif
}

/*
==============
Sys_Print
==============
*/
void Sys_Print( const char *msg ) {
	Conbuf_AppendText( msg );
}


/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir( const char *path ) {
	_mkdir (path);
}

/*
==============
Sys_Cwd
==============
*/
char * Sys_Cwd( char * path, int size ) {

	if ( _getcwd( path, size ) == NULL ) {
		Com_Error( ERR_FATAL, "couldn't find working directory.\n" );
	}

	return path;
}

/*
==============
Sys_DefaultCDPath
==============
*/
char *Sys_DefaultCDPath( char *path, int size )
{
#ifdef DEVELOPER
	path[ 0 ] = '\0';
	return path;
#else
	return Sys_Cwd( path, size );
#endif
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

void Sys_SplitPath( const char * fullname, char * path, int path_size, char * name, int name_size, char * ext, int ext_size )
{
	char _drive		[ _MAX_DRIVE ];
	char _dir		[ _MAX_DIR ];
	char _name		[ _MAX_FNAME ];
	char _ext		[ _MAX_EXT ];

	_splitpath( fullname, _drive, _dir, _name, _ext );

	if ( path ) {
		Q_strncpyz( path, _drive, path_size );
		Q_strcat( path, path_size, _dir );
	}

	if ( name )
		Q_strncpyz( name, _name, name_size );

	if ( ext )
		Q_strncpyz( ext, _ext, ext_size );
}


extern sqlInfo_t com_db;

void Sys_AddFiles( const char * base, const char * path, const char * ext ) {

	char				os_path[ MAX_OSPATH ];
	struct _finddata_t	findinfo;
	int					findhandle;

	Com_sprintf( os_path, sizeof(os_path), "%s%s*%s", base, path, (ext)?ext:"" );

	findhandle = _findfirst( os_path, &findinfo );
	if ( findhandle == -1 ) {
		return;
	}

	do {
		if ( !(findinfo.attrib & _A_HIDDEN) ) {

			if ( findinfo.attrib & _A_SUBDIR ) {
				if ( findinfo.name[ 0 ] != '.' ) {
					char sub_folder[ MAX_OSPATH ];
					Com_sprintf( sub_folder, sizeof(sub_folder), "%s%s/", path, findinfo.name );
					Sys_AddFiles( base, sub_folder, ext );
				}
			} else {

				char ext[ _MAX_EXT ];
				char name[ _MAX_FNAME ];

				_splitpath( findinfo.name, NULL, NULL,name, ext );

				sql_bindtext( &com_db, 2, path );
				sql_bindtext( &com_db, 3, name );
				sql_bindtext( &com_db, 4, ext );
				sql_bindint	( &com_db, 5, findinfo.size );
				sql_bindint	( &com_db, 6, 0 );
				sql_bindint	( &com_db, 7, -1 );

				sql_step	( &com_db );
			}
		}
	} while ( _findnext (findhandle, &findinfo) != -1 );
}


//========================================================


/*
================
Sys_ScanForCD

Search all the drives to see if there is a valid CD to grab
the cddir from
================
*/
qboolean Sys_ScanForCD( void ) {
	static char	cddir[MAX_OSPATH];
	char		drive[4];
	FILE		*f;
	char		test[MAX_OSPATH];
#if 0
	// don't override a cdpath on the command line
	if ( strstr( sys_cmdline, "cdpath" ) ) {
		return;
	}
#endif

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	// scan the drives
	for ( drive[0] = 'c' ; drive[0] <= 'z' ; drive[0]++ ) {
		if ( GetDriveType (drive) != DRIVE_CDROM ) {
			continue;
		}

		sprintf (cddir, "%s%s", drive, CD_BASEDIR);
		sprintf (test, "%s\\%s", cddir, CD_EXE);
		f = fopen( test, "r" );
		if ( f ) {
			fclose (f);
			return qtrue;
    } else {
      sprintf(cddir, "%s%s", drive, CD_BASEDIR_LINUX);
      sprintf(test, "%s\\%s", cddir, CD_EXE_LINUX);
  		f = fopen( test, "r" );
	  	if ( f ) {
		  	fclose (f);
			  return qtrue;
      }
    }
	}

	return qfalse;
}

/*
================
Sys_CheckCD

Return true if the proper CD is in the drive
================
*/
qboolean	Sys_CheckCD( void ) {
  // FIXME: mission pack
  return qtrue;
	//return Sys_ScanForCD();
}


/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void ) {
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 ) {
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 ) {
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 ) {
				data = Z_Malloc( GlobalSize( hClipboardData ) + 1 );
				Q_strncpyz( data, cliptext, GlobalSize( hClipboardData ) );
				GlobalUnlock( hClipboardData );
				
				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}


/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	if ( !FreeLibrary( dllHandle ) ) {
		Com_Error (ERR_FATAL, "Sys_UnloadDll FreeLibrary failed");
	}
}

/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine

TTimo: added some verbosity in debug
=================
*/
extern void FS_BuildOSPath( char * ospath, int size, const char *base, const char *game, const char *qpath );

// fqpath param added 7/20/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
// fqpath will be empty if dll not loaded, otherwise will hold fully qualified path of dll module loaded
// fqpath buffersize must be at least MAX_QPATH+1 bytes long
void * QDECL Sys_LoadDll( const char *name, char *fqpath , intptr_t (QDECL **entryPoint)(intptr_t, ...),
				  intptr_t (QDECL *systemcalls)(intptr_t, ...) ) {

	HINSTANCE	libHandle = NULL;
	void	(QDECL *dllEntry)( intptr_t (QDECL *syscallptr)(intptr_t, ...) );
	const char * filename = name;

#ifdef _DEBUG
	sql_prepare	( &com_db, "SELECT paths.id[ path_id ]^path || '/' || fullname FROM files SEARCH name ($1||'_debug') WHERE ext like '.dll';" );
#else
	sql_prepare	( &com_db, "SELECT paths.id[ path_id ]^path || '/' || fullname FROM files SEARCH name $1 WHERE ext like '.dll';" );
#endif
	sql_bindtext( &com_db, 1, name );

	if ( sql_step( &com_db ) ) {

		filename	= sql_columnastext( &com_db, 0 );
		libHandle	= LoadLibrary( filename );
	}

	sql_done( &com_db );

	if (!libHandle) {
		Com_Printf("LoadLibrary '%s' failed\n", filename );
		*fqpath = '\0';
		return NULL;
	}
	
	Com_Printf("LoadLibrary '%s' ok\n", filename );


	dllEntry = ( void (QDECL *)(intptr_t (QDECL *)( intptr_t, ... ) ) )GetProcAddress( libHandle, "dllEntry" ); 
	*entryPoint = (intptr_t (QDECL *)(intptr_t,...))GetProcAddress( libHandle, "vmMain" );
	if ( !*entryPoint || !dllEntry ) {
		FreeLibrary( libHandle );
		return NULL;
	}
	dllEntry( systemcalls );

	Q_strncpyz ( fqpath , filename , MAX_QPATH ) ;		// added 7/20/02 by T.Ray
	return libHandle;
}


/*
========================================================================

BACKGROUND FILE STREAMING

========================================================================
*/

#if 1

void Sys_InitStreamThread( void ) {
}

void Sys_ShutdownStreamThread( void )
{
	/*
		Note: if we crash during init this can get called
		before the stream thread is set up.
	*/
}

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
   return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
   FS_Seek( f, offset, origin );
}


#else

typedef struct {
	fileHandle_t	file;
	byte	*buffer;
	qboolean	eof;
	qboolean	active;
	int		bufferSize;
	int		streamPosition;	// next byte to be returned by Sys_StreamRead
	int		threadPosition;	// next byte to be read from file
} streamsIO_t;

typedef struct {
	HANDLE				threadHandle;
	int					threadId;
	CRITICAL_SECTION	crit;
	streamsIO_t			sIO[MAX_FILE_HANDLES];
} streamState_t;

streamState_t	stream;

/*
===============
Sys_StreamThread

A thread will be sitting in this loop forever
================
*/
void Sys_StreamThread( void ) {
	int		buffer;
	int		count;
	int		readCount;
	int		bufferPoint;
	int		r, i;

	while (1) {
		Sleep( 10 );
//		EnterCriticalSection (&stream.crit);

		for (i=1;i<MAX_FILE_HANDLES;i++) {
			// if there is any space left in the buffer, fill it up
			if ( stream.sIO[i].active  && !stream.sIO[i].eof ) {
				count = stream.sIO[i].bufferSize - (stream.sIO[i].threadPosition - stream.sIO[i].streamPosition);
				if ( !count ) {
					continue;
				}

				bufferPoint = stream.sIO[i].threadPosition % stream.sIO[i].bufferSize;
				buffer = stream.sIO[i].bufferSize - bufferPoint;
				readCount = buffer < count ? buffer : count;

				r = FS_Read( stream.sIO[i].buffer + bufferPoint, readCount, stream.sIO[i].file );
				stream.sIO[i].threadPosition += r;

				if ( r != readCount ) {
					stream.sIO[i].eof = qtrue;
				}
			}
		}
//		LeaveCriticalSection (&stream.crit);
	}
}

/*
===============
Sys_InitStreamThread

================
*/
void Sys_InitStreamThread( void ) {
	int i;

	InitializeCriticalSection ( &stream.crit );

	// don't leave the critical section until there is a
	// valid file to stream, which will cause the StreamThread
	// to sleep without any overhead
//	EnterCriticalSection( &stream.crit );

	stream.threadHandle = CreateThread(
	   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
	   0,		// DWORD cbStack,
	   (LPTHREAD_START_ROUTINE)Sys_StreamThread,	// LPTHREAD_START_ROUTINE lpStartAddr,
	   0,			// LPVOID lpvThreadParm,
	   0,			//   DWORD fdwCreate,
	   &stream.threadId);
	for(i=0;i<MAX_FILE_HANDLES;i++) {
		stream.sIO[i].active = qfalse;
	}
}

/*
===============
Sys_ShutdownStreamThread

================
*/
void Sys_ShutdownStreamThread( void ) {
}


/*
===============
Sys_BeginStreamedFile

================
*/
void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
	if ( stream.sIO[f].file ) {
		Sys_EndStreamedFile( stream.sIO[f].file );
	}

	stream.sIO[f].file = f;
	stream.sIO[f].buffer = Z_Malloc( readAhead );
	stream.sIO[f].bufferSize = readAhead;
	stream.sIO[f].streamPosition = 0;
	stream.sIO[f].threadPosition = 0;
	stream.sIO[f].eof = qfalse;
	stream.sIO[f].active = qtrue;

	// let the thread start running
//	LeaveCriticalSection( &stream.crit );
}

/*
===============
Sys_EndStreamedFile

================
*/
void Sys_EndStreamedFile( fileHandle_t f ) {
	if ( f != stream.sIO[f].file ) {
		Com_Error( ERR_FATAL, "Sys_EndStreamedFile: wrong file");
	}
	// don't leave critical section until another stream is started
	EnterCriticalSection( &stream.crit );

	stream.sIO[f].file = 0;
	stream.sIO[f].active = qfalse;

	Z_Free( stream.sIO[f].buffer );

	LeaveCriticalSection( &stream.crit );
}


/*
===============
Sys_StreamedRead

================
*/
int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
	int		available;
	int		remaining;
	int		sleepCount;
	int		copy;
	int		bufferCount;
	int		bufferPoint;
	byte	*dest;

	if (stream.sIO[f].active == qfalse) {
		Com_Error( ERR_FATAL, "Streamed read with non-streaming file" );
	}

	dest = (byte *)buffer;
	remaining = size * count;

	if ( remaining <= 0 ) {
		Com_Error( ERR_FATAL, "Streamed read with non-positive size" );
	}

	sleepCount = 0;
	while ( remaining > 0 ) {
		available = stream.sIO[f].threadPosition - stream.sIO[f].streamPosition;
		if ( !available ) {
			if ( stream.sIO[f].eof ) {
				break;
			}
			if ( sleepCount == 1 ) {
				Com_DPrintf( "Sys_StreamedRead: waiting\n" );
			}
			if ( ++sleepCount > 100 ) {
				Com_Error( ERR_FATAL, "Sys_StreamedRead: thread has died");
			}
			Sleep( 10 );
			continue;
		}

		EnterCriticalSection( &stream.crit );

		bufferPoint = stream.sIO[f].streamPosition % stream.sIO[f].bufferSize;
		bufferCount = stream.sIO[f].bufferSize - bufferPoint;

		copy = available < bufferCount ? available : bufferCount;
		if ( copy > remaining ) {
			copy = remaining;
		}
		memcpy( dest, stream.sIO[f].buffer + bufferPoint, copy );
		stream.sIO[f].streamPosition += copy;
		dest += copy;
		remaining -= copy;

		LeaveCriticalSection( &stream.crit );
	}

	return (count * size - remaining) / size;
}

/*
===============
Sys_StreamSeek

================
*/
void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {

	// halt the thread
	EnterCriticalSection( &stream.crit );

	// clear to that point
	FS_Seek( f, offset, origin );
	stream.sIO[f].streamPosition = 0;
	stream.sIO[f].threadPosition = 0;
	stream.sIO[f].eof = qfalse;

	// let the thread start running at the new position
	LeaveCriticalSection( &stream.crit );
}

#endif

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_t	eventQue[MAX_QUED_EVENTS];
int			eventHead, eventTail;
byte		sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t	*ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];
	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf("Sys_QueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	if ( time == 0 ) {
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent

================
*/

sysEvent_t Sys_GetEvent( void )
{
	MSG			msg;

	sysEvent_t	ev;
	char		*s;
	msg_t		netmsg;
	netadr_t	adr;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	//force the internal queuing system to poll the game timer
	//we don't want to use the message's time since windows is allowed
	//to reorder the message queue, and it can easily sneak messages
	//past GetMessage or PeekMessage...
	//											- PD

	g_wv.sysMsgTime = 0;

	// pump the message loop
	while( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
	{
		if( msg.message == WM_QUIT )
			Com_Quit_f();

		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = strlen( s ) + 1;
		b = Z_Malloc( len );
		Q_strncpyz( b, s, len-1 );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	adr.type = NA_IP;
	if ( Sys_GetPacket ( &adr, &netmsg ) ) {
		netadr_t		*buf;
		int				len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
		buf = Z_Malloc( len );
		*buf = adr;
		memcpy( buf+1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = timeGetTime();

	return ev;
}

//================================================================

void Sys_Sleep( int msec )
{
	if( msec > 2 )
		MsgWaitForMultipleObjectsEx( 0, NULL, msec - 2, QS_ALLEVENTS, MWMO_INPUTAVAILABLE );
}

qboolean Sys_OpenUrl( const char *url )
{
	return ((int)ShellExecute( NULL, NULL, url, NULL, NULL, SW_SHOWNORMAL ) > 32) ? qtrue : qfalse; 
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Q_EXTERNAL_CALL Sys_In_Restart_f( void ) {
	IN_Shutdown();
	IN_Init();
}


/*
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
void Q_EXTERNAL_CALL Sys_Net_Restart_f( void ) {
	NET_Restart();
}


/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
#define OSR2_BUILD_NUMBER 1111
#define WIN98_BUILD_NUMBER 1998

void Sys_Init( void ) {
	int cpuid;
	char tmp[ MAX_NAME_LENGTH ];

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);

	g_wv.osversion.dwOSVersionInfoSize = sizeof( g_wv.osversion );

	if (!GetVersionEx (&g_wv.osversion))
		Sys_Error ("Couldn't get OS info");

	if (g_wv.osversion.dwMajorVersion < 4)
		Sys_Error ("Quake3 requires Windows version 4 or greater");
	if (g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("Quake3 doesn't run on Win32s");

	if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		Cvar_Set( "arch", "winnt" );
	}
	else if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
	{
		if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= WIN98_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win98" );
		}
		else if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win95 osr2.x" );
		}
		else
		{
			Cvar_Set( "arch", "win95" );
		}
	}
	else
	{
		Cvar_Set( "arch", "unknown Windows variant" );
	}

	// save out a couple things in rom cvars for the renderer to access
	Cvar_Get( "win_hinstance", va("%i", (int)g_wv.hInstance), CVAR_ROM );
	Cvar_Get( "win_wndproc", va("%i", (int)MainWndProc), CVAR_ROM );

	//
	// figure out our CPU
	//
	Cvar_Get( "sys_cpustring", "detect", 0 );
	if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring"), "detect" ) )
	{
		Com_Printf( "...detecting CPU, found " );

		cpuid = Sys_GetProcessorId();

		switch ( cpuid )
		{
		case CPUID_GENERIC:
			Cvar_Set( "sys_cpustring", "generic" );
			break;
		case CPUID_INTEL_UNSUPPORTED:
			Cvar_Set( "sys_cpustring", "x86 (pre-Pentium)" );
			break;
		case CPUID_INTEL_PENTIUM:
			Cvar_Set( "sys_cpustring", "x86 (P5/PPro, non-MMX)" );
			break;
		case CPUID_INTEL_MMX:
			Cvar_Set( "sys_cpustring", "x86 (P5/Pentium2, MMX)" );
			break;
		case CPUID_INTEL_KATMAI:
			Cvar_Set( "sys_cpustring", "Intel Pentium III" );
			break;
		case CPUID_AMD_3DNOW:
			Cvar_Set( "sys_cpustring", "AMD w/ 3DNow!" );
			break;
		case CPUID_AXP:
			Cvar_Set( "sys_cpustring", "Alpha AXP" );
			break;
		default:
			Com_Error( ERR_FATAL, "Unknown cpu type %d\n", cpuid );
			break;
		}
	}
	else
	{
		Com_Printf( "...forcing CPU type to " );
		if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "generic" ) )
		{
			cpuid = CPUID_GENERIC;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "x87" ) )
		{
			cpuid = CPUID_INTEL_PENTIUM;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "mmx" ) )
		{
			cpuid = CPUID_INTEL_MMX;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "3dnow" ) )
		{
			cpuid = CPUID_AMD_3DNOW;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "PentiumIII" ) )
		{
			cpuid = CPUID_INTEL_KATMAI;
		}
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "axp" ) )
		{
			cpuid = CPUID_AXP;
		}
		else
		{
			Com_Printf( "WARNING: unknown sys_cpustring '%s'\n", Cvar_VariableString( "sys_cpustring" ) );
			cpuid = CPUID_GENERIC;
		}
	}
	Cvar_SetValue( "sys_cpuid", cpuid );
	Com_Printf( "%s\n", Cvar_VariableString( "sys_cpustring" ) );

	Cvar_Set( "username", Sys_GetCurrentUser( tmp, sizeof(tmp) ) );

	IN_Init();		// FIXME: not in dedicated?
}


qboolean Sys_DetectAltivec( void )
{
    return qfalse;  // never altivec on Windows...at least for now.  :)
}



//=======================================================================

int	totalMsec, countMsec;

/*****************************************
	Ye Olde Linker Hackery

*/

#include "platform/pop_segs.h"

static bool did_hack_hack;
static void *data_seg_cpy;
static uint data_seg_len;

#include "platform/push_segs.h"

#pragma data_seg( push )
#pragma data_seg( ".hwdata$a" )

__declspec( allocate( ".hwdata$a" ) ) static int data_seg_a = 0;

#pragma data_seg( ".hwdata$z" )

__declspec( allocate( ".hwdata$z" ) ) static int data_seg_z = 0;

#pragma data_seg( pop )

#pragma bss_seg( push )
#pragma bss_seg( ".hwbss$a" )

__declspec( allocate( ".hwbss$a" ) )static int bss_seg_a;

#pragma bss_seg( ".hwbss$z" )

__declspec( allocate( ".hwbss$z" ) )static int bss_seg_z;

#pragma data_seg( pop )

static void Hack_Segs( void )
{
	if( !did_hack_hack )
	{
		did_hack_hack = true;

		//first time in copy out the data segs
		data_seg_len = (&data_seg_z - (&data_seg_a + 1)) * sizeof( int );

		data_seg_cpy = malloc( data_seg_len );		
		memcpy( data_seg_cpy, &data_seg_a + 1, data_seg_len );
	}
	else
	{
		//next time in copy the data seg back and zero the bss seg
		memcpy( &data_seg_a + 1, data_seg_cpy, data_seg_len );
		memset( &bss_seg_a + 1, 0, (&bss_seg_z - (&bss_seg_a + 1)) * sizeof( int ) );
	}
}

int Win_Main( HINSTANCE hInstance, HWND hWnd, LPSTR lpCmdLine, int nCmdShow )
{
	char		cwd[ MAX_OSPATH ];
	int			startTime, endTime;

	Hack_Segs();

	if( setjmp( sys_exitframe ) )
	{
		//make sure that any subsystems that may have spawned threads go down
		__try
		{
			S_Shutdown();
			CL_ShutdownRef();
			Sys_ShutdownStreamThread();
		}
		__finally //wheeeee...
		{
			Com_ReleaseMemory();
		}

		return sys_retcode;
	}

	g_wv.hInstance = hInstance;
	g_wv.hWndHost = hWnd;
	Q_strncpyz( sys_cmdline, lpCmdLine, sizeof( sys_cmdline ) );

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	// get the initial time base
	Sys_Milliseconds();

	Sys_QueEvent( 0, SE_RANDSEED, timeGetTime(), 0, 0, 0 );

#if 0
	// if we find the CD, add a +set cddir xxx command line
	Sys_ScanForCD();
#endif

	Sys_InitStreamThread();

	
	Com_Init( sys_cmdline );
	NET_Init();
#ifdef USE_IRC
	Net_IRC_Init();
#endif

	_getcwd ( cwd, sizeof( cwd ) );
	Com_Printf( "Working directory: %s\n", cwd );

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if( !com_dedicated->integer && !com_viewlog->integer )
	{
		Sys_ShowConsole( 0, qfalse );
	} else {
		Sys_ShowConsole( 1, qfalse );
	}

    // main game loop
	for( ; ; )
	{
		// if not running as a game client, sleep a bit
		if( g_wv.isMinimized || ( com_dedicated && com_dedicated->integer ) )
		{
			Sleep( 5 );
		}

		// set low precision every frame, because some system calls
		// reset it arbitrarily
		//_controlfp( _PC_24, _MCW_PC );
		//_controlfp( -1, _MCW_EM  ); // no exceptions, even if some crappy
                                // syscall turns them back on!

		startTime = Sys_Milliseconds();

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();

		endTime = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		countMsec++;
	}

	// never gets here
}

/*
======================
	ReportCrash
======================
*/

#include <DbgHelp.h>

typedef struct 
{
	DWORD excThreadId;

	DWORD excCode;
	PEXCEPTION_POINTERS pExcPtrs;

	int numIgnoreThreads;
	DWORD ignoreThreadIds[16];

	int numIgnoreModules;
	HMODULE ignoreModules[16];
} dumpParams_t;

typedef struct dumpCbParams_t
{
	dumpParams_t	*p;
	HANDLE			hModulesFile;
} dumpCbParams_t;

static BOOL CALLBACK MiniDumpCallback( PVOID CallbackParam, const PMINIDUMP_CALLBACK_INPUT CallbackInput,
	PMINIDUMP_CALLBACK_OUTPUT CallbackOutput )
{
	const dumpCbParams_t *params = (dumpCbParams_t*)CallbackParam;
	
	switch( CallbackInput->CallbackType )
	{
	case IncludeThreadCallback:
		{
			int i;

			for( i = 0; i < params->p->numIgnoreThreads; i++ )
				if( CallbackInput->IncludeThread.ThreadId == params->p->ignoreThreadIds[i] )
					return FALSE;
		}
		return TRUE;

	case IncludeModuleCallback:
		{
			int i;

			for( i = 0; i < params->p->numIgnoreModules; i++ )
				if( (HMODULE)CallbackInput->IncludeModule.BaseOfImage == params->p->ignoreModules[i] )
					return FALSE;
		}
		return TRUE;

	case ModuleCallback:
		if( params->hModulesFile )
		{
			PWCHAR packIncludeMods[] = { L"/oh/noes" };//{ L"SpaceTrader.exe", L"render-base.dll", L"cgame.dll", L"game.dll", L"ui.dll" };

			wchar_t *path = CallbackInput->Module.FullPath;
			size_t len = wcslen( path );

			size_t i;
			size_t c;

			//path is fully qualified - this won't trash
			for( i = len; i--; )
				if( path[i] == L'\\' )
					break;

			c = i + 1;

			for( i = 0; i < lengthof( packIncludeMods ); i++ )
				if( wcsicmp( path + c, packIncludeMods[i] ) == 0 )
				{
					DWORD dummy;

					WriteFile( params->hModulesFile, path, sizeof( wchar_t ) * len, &dummy, NULL );
					WriteFile( params->hModulesFile, L"\n", sizeof( wchar_t ), &dummy, NULL );
					break;
				}
		}
		return TRUE;

	default:
		return TRUE;
	}
}

typedef BOOL (WINAPI *MiniDumpWriteDumpFunc_t)(
	IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
);

static void CreatePath( char *OSPath )
{
	char *ofs;

	for( ofs = OSPath + 1; *ofs; ofs++ )
	{
		if( *ofs == '\\' || *ofs == '/' )
		{
			// create the directory
			*ofs = 0;
			
			CreateDirectory( OSPath, NULL );

			*ofs = '\\';
		}
	}

	if( ofs[-1] != '\\' )
		CreateDirectory( OSPath, NULL );
}

static DWORD WINAPI DoGenerateCrashDump( LPVOID pParams )
{
	dumpParams_t *params = (dumpParams_t*)pParams;
	
	static int nDump = 0;

	HANDLE hFile, hIncludeFile;
	DWORD dummy;
	char basePath[MAX_PATH], path[MAX_PATH];

	MiniDumpWriteDumpFunc_t MiniDumpWriteDump;

	{
		HMODULE hDbgHelp = LoadLibrary( "DbgHelp.dll" );

		if( !hDbgHelp )
			return 1;

		MiniDumpWriteDump = (MiniDumpWriteDumpFunc_t)GetProcAddress( hDbgHelp, "MiniDumpWriteDump" );

		if( !MiniDumpWriteDump )
			return 2;

		params->ignoreModules[params->numIgnoreModules++] = hDbgHelp;
	}

	{
		char homePath[ MAX_OSPATH ];
		Sys_DefaultHomePath( homePath, sizeof(homePath) );
		if( !homePath[0] )
			return 3;

		Com_sprintf( basePath, sizeof( basePath ), "%s/st/bugs", homePath );
	}

#define BASE_DUMP_FILE_NAME "dump%04i"

	CreatePath( basePath );

	for( ; ; )
	{
		DWORD err;

		Com_sprintf( path, sizeof( path ), "%s/"BASE_DUMP_FILE_NAME".dmp", basePath, nDump );
		hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0 );

		if( hFile != INVALID_HANDLE_VALUE )
			break;

		err = GetLastError();

		switch( err )
		{
		case ERROR_FILE_EXISTS:
		case ERROR_ALREADY_EXISTS:
			break;

		case 0:
			return (DWORD)-1;

		default:
			return 0x80000000 | err;
		}

		nDump++;
	}

	Com_sprintf( path, sizeof( path ), "%s/"BASE_DUMP_FILE_NAME".include", basePath, nDump );
	hIncludeFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );

	{
		dumpCbParams_t cbParams;

		MINIDUMP_EXCEPTION_INFORMATION excInfo;
		MINIDUMP_CALLBACK_INFORMATION cbInfo;

		excInfo.ThreadId = params->excThreadId;
		excInfo.ExceptionPointers = params->pExcPtrs;
		excInfo.ClientPointers = TRUE;

		cbParams.p = params;
		cbParams.hModulesFile = hIncludeFile;

		cbInfo.CallbackParam = &cbParams;
		cbInfo.CallbackRoutine = MiniDumpCallback;

		MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hFile,
			MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory,
			&excInfo, NULL, &cbInfo );
	}

	CloseHandle( hFile );

	{
		/*
			Write any additional include files here. Remember,
			the include file is unicode (wchar_t), has one
			file per line, and lines are seperated by a single LF (L'\n').
		*/
	}
	CloseHandle( hIncludeFile );

	Com_sprintf( path, sizeof( path ), "%s/"BASE_DUMP_FILE_NAME".build", basePath, nDump );
	hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );

	{
		HMODULE hMod;

		HRSRC rcVer, rcBuild, rcMachine;
		HGLOBAL hgVer, hgBuild, hgMachine;

		DWORD lVer, lBuild, lMachine;
		char *pVer, *pBuild, *pMachine;

		hMod = GetModuleHandle( NULL );

		rcVer = FindResource( hMod, MAKEINTRESOURCE( IDR_INFO_SVNSTAT ), "INFO" );
		rcBuild = FindResource( hMod, MAKEINTRESOURCE( IDR_INFO_BUILDCONFIG ), "INFO" );
		rcMachine = FindResource( hMod, MAKEINTRESOURCE( IDR_INFO_BUILDMACHINE ), "INFO" );

		hgVer = LoadResource( hMod, rcVer );
		hgBuild = LoadResource( hMod, rcBuild );
		hgMachine = LoadResource( hMod, rcMachine );

		lVer = SizeofResource( hMod, rcVer );
		pVer = (char*)LockResource( hgVer );

		lBuild = SizeofResource( hMod, rcBuild );
		pBuild = (char*)LockResource( hgBuild );

		lMachine = SizeofResource( hMod, rcMachine );
		pMachine = (char*)LockResource( hgMachine );

		WriteFile( hFile, pBuild, lBuild, &dummy, NULL );
		WriteFile( hFile, pMachine, lMachine, &dummy, NULL );
		WriteFile( hFile, pVer, lVer, &dummy, NULL );
	}

	CloseHandle( hFile );

	Com_sprintf( path, sizeof( path ), "%s/"BASE_DUMP_FILE_NAME".info", basePath, nDump );
	hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );

	{
		/*
			The .info contains one line of bug title and the rest is all bug description.
		*/

		const char *msg;
		switch( params->excCode )
		{
			/*
		case SEH_CAPTURE_EXC:
			msg = (const char*)params->pExcPtrs->ExceptionRecord->ExceptionInformation[0];
			break;
			*/

		default:
			msg = "Crash Dump\nSpace Trader crashed, see the attached dump for more information.";
			break;
		}

		WriteFile( hFile, msg, strlen( msg ), &dummy, NULL );
	}

	CloseHandle( hFile );

	Com_sprintf( path, sizeof( path ), "%s/"BASE_DUMP_FILE_NAME".con", basePath, nDump );
	hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );

	{
		const char *conDump = Con_GetText( 0 );	
		WriteFile( hFile, conDump, strlen( conDump ), &dummy, NULL );
	}

	CloseHandle( hFile );

#undef BASE_DUMP_FILE_NAME

	return 0;
}

static INT_PTR CALLBACK DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_COMMAND:
		switch( LOWORD( wParam ) )
		{
		case IDOK:
			PostQuitMessage( 0 );
			break;
		}
		return TRUE;

	default:
		return FALSE;
	}
}

static DWORD WINAPI DoReportCrashGUI( LPVOID pParams )
{
	dumpParams_t *params = (dumpParams_t*)pParams;

	HWND dlg;
	HANDLE h;
	DWORD tid;
	MSG msg;
	BOOL mRet;
	BOOL ended;

	if( IsDebuggerPresent() )
	{
		if( MessageBox( NULL, "An exception has occurred: press Yes to debug, No to create a crash dump.",
			"Space Trader Error", MB_ICONERROR | MB_YESNO | MB_DEFBUTTON1 ) == IDYES )
			return EXCEPTION_CONTINUE_SEARCH; 
	}

	h = CreateThread( NULL, 0, DoGenerateCrashDump, pParams, CREATE_SUSPENDED, &tid );
	params->ignoreThreadIds[params->numIgnoreThreads++] = tid;

	dlg = CreateDialog( g_wv.hInstance, MAKEINTRESOURCE( IDD_CRASH_REPORT ), NULL, DialogProc );
	ShowWindow( dlg, SW_SHOWNORMAL );

	ended = FALSE;
	ResumeThread( h );

	while( (mRet = GetMessage( &msg, 0, 0, 0 )) != 0 )
	{
		if( mRet == -1 )
			break;

		TranslateMessage( &msg );
		DispatchMessage( &msg );

		if( !ended && WaitForSingleObject( h, 0 ) == WAIT_OBJECT_0 )
		{
			HWND btn = GetDlgItem( dlg, IDOK );

			ended = TRUE;

			if( btn )
			{
				EnableWindow( btn, TRUE );
				UpdateWindow( btn );
				InvalidateRect( btn, NULL, TRUE );
			}
			else
				break;
		}
	}

	if( !ended )
		WaitForSingleObject( h, INFINITE );

	{
		DWORD hRet;
		GetExitCodeThread( h, &hRet );

		if( hRet == 0 )
		{
			MessageBox( NULL, "There was an error. Please submit a bug report.",
				"Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1 );
		}
		else
		{
			char msg[1024];

			_snprintf( msg, sizeof( msg ),
				"There was an error. Please submit a bug report.\n\n"
				"No error info was saved (code:0x%X).", hRet );

			MessageBox( NULL, msg,
				"Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1 );
		}
	}

	DestroyWindow( dlg );

	switch( params->excCode )
	{
		/*
	case SEH_CAPTURE_EXC:
		return (DWORD)EXCEPTION_CONTINUE_EXECUTION;
		*/

	default:
		return (DWORD)EXCEPTION_EXECUTE_HANDLER;
	}
}

static int ReportCrash( DWORD excCode, PEXCEPTION_POINTERS pExcPtrs )
{	
#ifdef _DEBUG
	
	return EXCEPTION_CONTINUE_SEARCH;

#else

	/*
		Launch off another thread to get the crash dump.

		Note that this second thread will pause this thread so *be certain* to
		keep all GUI stuff on it and *not* on this thread.
	*/

	dumpParams_t params;

	DWORD tid, ret;
	HANDLE h;

	Com_Memset( &params, 0, sizeof( params ) );
	params.excThreadId = GetCurrentThreadId();
	params.excCode = excCode;
	params.pExcPtrs = pExcPtrs;

	h = CreateThread( NULL, 0, DoReportCrashGUI, &params, CREATE_SUSPENDED, &tid );
	params.ignoreThreadIds[params.numIgnoreThreads++] = tid;

	ResumeThread( h );
	WaitForSingleObject( h, INFINITE );

	GetExitCodeThread( h, &ret );
	return (int)ret;
#endif
}

int Game_Main( HINSTANCE hInstance, HWND hWnd, LPSTR lpCmdLine, int nCmdShow )
{
	int retcode;

	__try
	{
		retcode = Win_Main( hInstance, hWnd, lpCmdLine, nCmdShow );
	}
	__except( ReportCrash( GetExceptionCode(), GetExceptionInformation() ) )
	{
		retcode = -1;
	}

	return retcode;
}

void Game_GetExitState( int *retCode, char *exit_msg, size_t msg_buff_len )
{
	if( retCode )
		*retCode = sys_retcode;

	if( exit_msg )
		strncpy( exit_msg, sys_exitstr, msg_buff_len );
}

/*
===============
Sys_SwapVersion
 - check to see if we are running the highest
   numbered exe, and switch to it if we are not
===============
*/

static int GetSTExeVersion( const char *path )
{
	int len, i;
	char buf[16];
	const char *basePart, *extPart;

	basePart = strstr( path, PLATFORM_EXE_BASE );
	if( !basePart )
		return -1;

	extPart = strstr( path, PLATFORM_EXE_EXT );
	if( !extPart )
		return -1;

	basePart += strlen( PLATFORM_EXE_BASE );;

	if( basePart >= extPart )
		return -1;

	len = extPart - basePart;
	if( len > sizeof( buf ) - 1 )
		return -1;

	strncpy( buf, basePart, len );

	for( i = 0; i < len; i++ )
	{
		if( buf[i] < '0' || buf[i] > '9' )
			return -1;
	}

	return atoi( buf );
}

void Sys_SwapVersion( void )
{
	int maxVer;

	HANDLE hFind;
	WIN32_FIND_DATA find;
	char osPath[MAX_PATH], osDir[MAX_PATH], findPath[MAX_PATH];

	Q_strncpyz( osDir, Cvar_VariableString( "fs_basepath" ), sizeof( osDir ) );
	Q_strcat( osDir, sizeof( osDir ), "\\"BASEGAME );
	//Sys_DefaultInstallPath( osDir, sizeof( osDir ) );

	_snprintf( findPath, sizeof( findPath ), "%s\\%s", osDir, PLATFORM_EXE_BASE"*"PLATFORM_EXE_EXT );

	hFind = FindFirstFileEx( findPath, FindExInfoStandard, &find, FindExSearchNameMatch, NULL, 0 );
	if( hFind == INVALID_HANDLE_VALUE )
		return;

	maxVer = -1;
	do
	{
		int iVer;

		if( find.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY) )
			continue;

		iVer = GetSTExeVersion( find.cFileName );
		if( iVer > maxVer )
			maxVer = iVer;
	} while( FindNextFile( hFind, &find ) );


	FindClose( hFind );

	if( maxVer != -1 )
	{
		int c;
		char cwd[MAX_PATH];
	
		_snprintf( osPath, sizeof( osPath ), "%s\\"PLATFORM_EXE_NAME, osDir, maxVer );
		GetCurrentDirectory( sizeof( cwd ), cwd );
		
		c = (int)ShellExecute( NULL, NULL, osPath, sys_cmdline, cwd, SW_SHOW );

		if( c > 32 )
			Com_Quit_f();
	}
}
