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
// common.c -- misc functions used in client and server

#include "q_shared.h"
#include "qcommon.h"
#include "../sql/sql.h"

#ifndef _WIN32
#include <netinet/in.h>
#include <sys/stat.h> // umask
#include <setjmp.h>
#endif

const int demo_protocols[] =
{ 66, 67, 68, 0 };

#define MAX_NUM_ARGVS	50

#define MIN_DEDICATED_COMHUNKMEGS 1
#define MIN_COMHUNKMEGS		12
#define DEF_COMHUNKMEGS		64
#define DEF_COMZONEMEGS		32
#define XSTRING(x)				STRING(x)
#define STRING(x)					#x
#define DEF_COMHUNKMEGS_S	XSTRING(DEF_COMHUNKMEGS)
#define DEF_COMZONEMEGS_S	XSTRING(DEF_COMZONEMEGS)

int		com_argc;
char	*com_argv[MAX_NUM_ARGVS+1];


int		com_randseed;

jmp_buf abortframe;		// an ERR_DROP occured, exit the entire frame

FILE *debuglogfile;
static fileHandle_t logfile;
fileHandle_t	com_journalFile;			// events are written here
fileHandle_t	com_journalDataFile;		// config files are written here

sqlInfo_t com_db;

sqlInfo_t * sql_getcommondb( void ) { return &com_db; }

cvar_t	*com_viewlog;
cvar_t	*com_speeds;
cvar_t	*com_developer;
cvar_t	*com_dedicated;
cvar_t	*com_timescale;
cvar_t	*com_fixedtime;
cvar_t	*com_dropsim;		// 0.0 to 1.0, simulated packet drops
cvar_t	*com_journal;
cvar_t	*com_maxfps;
cvar_t	*com_lowfps;
cvar_t	*com_altivec;
cvar_t	*com_timedemo;
cvar_t	*com_sv_running;
cvar_t	*com_cl_running;
cvar_t	*com_logfile;		// 1 = buffer log, 2 = flush after each print
cvar_t	*com_showtrace;
cvar_t	*com_version;
cvar_t	*com_blood;
cvar_t	*com_buildScript;	// for automated data building scripts
cvar_t	*com_introPlayed;
cvar_t	*cl_paused;
cvar_t	*sv_paused;
cvar_t	*com_cameraMode;
cvar_t	*com_heartbeat;
cvar_t	*com_renderer;
#ifdef USE_WEBHOST
cvar_t	*com_webhost;
#endif
cvar_t	*com_allowDownload;
cvar_t	*com_downloading;
#ifdef USE_WEBAUTH
cvar_t	*com_sessionid;
#endif
#ifdef USE_DRM
cvar_t	*com_keyvalid;
#endif
cvar_t	*com_cmdline;

#if defined(_DEBUG)
cvar_t	*com_noErrorInterrupt;
#endif

// com_speeds times
int		time_game;
int		time_frontend;		// renderer frontend time
int		time_backend;		// renderer backend time

int			com_frameTime;
int			com_frameMsec;
int			com_frameNumber;

qboolean	com_errorEntered;
qboolean	com_fullyInitialized;

void Q_EXTERNAL_CALL Com_WriteConfig_f( void );
void Q_EXTERNAL_CALL Com_Meminfo_f( void );
void CIN_CloseAllVideos( void );

//============================================================================

static char	*rd_buffer;
static int	rd_buffersize;
static void	(*rd_flush)( char *buffer );

void Com_BeginRedirect (char *buffer, int buffersize, void (*flush)( char *) )
{
	if (!buffer || !buffersize || !flush)
		return;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect (void)
{
	if ( rd_flush ) {
		rd_flush(rd_buffer);
	}

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void QDECL Com_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];
  static qboolean opening_qconsole = qfalse;


	va_start (argptr,fmt);
	
	Q_vsnprintf( msg, sizeof( msg ) - 1, fmt, argptr );
	msg[sizeof( msg ) - 1] = 0;

	va_end (argptr);

	if ( rd_buffer ) {
		if ((strlen (msg) + strlen(rd_buffer)) > (rd_buffersize - 1)) {
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		Q_strcat(rd_buffer, rd_buffersize, msg);
    // TTimo nooo .. that would defeat the purpose
		//rd_flush(rd_buffer);			
		//*rd_buffer = 0;
		return;
	}

	// echo to console if we're not a dedicated server
	if ( com_dedicated && !com_dedicated->integer ) {
		CL_ConsolePrint( msg );
	}

	// echo to dedicated console and early console
	Sys_Print( msg );

	// logfile
	if ( com_logfile && com_logfile->integer ) {
    // TTimo: only open the qconsole.log if the filesystem is in an initialized state
    //   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
		if ( !logfile && FS_Initialized() && !opening_qconsole) {
			struct tm *newtime;
			time_t aclock;

      opening_qconsole = qtrue;

			time( &aclock );
			newtime = localtime( &aclock );

			logfile = FS_FOpenFileWrite( "qconsole.log" );
			Com_Printf( "logfile opened on %s\n", asctime( newtime ) );
			if ( com_logfile->integer > 1 ) {
				// force it to not buffer so we get valid
				// data even if we are crashing
				FS_ForceFlush(logfile);
			}

      opening_qconsole = qfalse;
		}
		if ( logfile && FS_Initialized()) {
			FS_Write(msg, strlen(msg), logfile);
		}
	}
}


/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void QDECL Com_DPrintf( const char *fmt, ...) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];
		
	if ( !com_developer || !com_developer->integer ) {
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start (argptr,fmt);	
	Q_vsnprintf( msg, sizeof( msg ) - 1, fmt, argptr );
	msg[sizeof( msg ) - 1] = 0;
	va_end (argptr);
	
	Com_Printf ("%s", msg);
}

#ifdef USE_CALLHOME
const char* Q_EXTERNAL_CALL Con_GetText	( int console );
#endif

static int QDECL callhome( httpInfo_e code, const char * buffer, int length, void * notifyData )
{
	if ( code == HTTP_DONE || code == HTTP_FAILED ) {
		*(int*)notifyData = 1;
	}
	return 1;
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void QDECL Com_Error( int code, const char *fmt, ... ) {
	va_list		argptr;
	static int	lastErrorTime;
	static int	errorCount;
	int			currentTime;
	char		com_errorMessage[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsnprintf (com_errorMessage,sizeof(com_errorMessage),fmt, argptr);
	va_end (argptr);

#if defined(_DEBUG)
	if ( code != ERR_DISCONNECT && code != ERR_NEED_CD && com_errorMessage[0] != '\0' ) {
		if (com_noErrorInterrupt && !com_noErrorInterrupt->integer) {
			__debugbreak();
		}
	}
#endif

#ifdef USE_CALLHOME
	{
		int i = 0;
		HTTP_PostUrl( "http://www.playspacetrader.com/user/log", callhome, &i, "message=ERROR:%s\n%s\n", com_errorMessage, Con_GetText(0) );
		// spin for up to 5 seconds to allow bug to post
		while ( !i ) {
			Sys_Sleep( 10 );
			Net_HTTP_Pump();
		}
	}
#endif

	// when we are running automated scripts, make sure we
	// know if anything failed
	if ( com_buildScript && com_buildScript->integer ) {
		code = ERR_FATAL;
	}

	// if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
	currentTime = Sys_Milliseconds();
	if ( currentTime - lastErrorTime < 100 ) {
		if ( ++errorCount > 3 ) {
			code = ERR_FATAL;
		}
	} else {
		errorCount = 0;
	}
	lastErrorTime = currentTime;

	if ( com_errorEntered ) {
		Sys_Error( "recursive error after: %s", com_errorMessage );
	}
	com_errorEntered = qtrue;

	switch( code )
	{
	case ERR_FATAL:
	case ERR_DROP:
		Sys_WriteDump( "Debug Dump\nCom_Error: %s", com_errorMessage );
		break;
	}

	if ( code == ERR_SERVERDISCONNECT ) {
		CL_Disconnect( qtrue );
		CL_FlushMemory( );
		com_errorEntered = qfalse;
		Cvar_Set("com_errorMessage", com_errorMessage);
		longjmp (abortframe, -1);
	} else if ( code == ERR_DROP || code == ERR_DISCONNECT ) {
		Com_Printf ("********************\nERROR: %s\n********************\n", com_errorMessage);
		SV_Shutdown (va("Server crashed: %s\n",  com_errorMessage));
		CL_Disconnect( qtrue );
		CL_FlushMemory( );
		com_errorEntered = qfalse;
		Cvar_Set("com_errorMessage", com_errorMessage);
		longjmp (abortframe, -1);
	} else if ( code == ERR_NEED_CD ) {
		SV_Shutdown( "Server didn't have CD\n" );
		if ( com_cl_running && com_cl_running->integer ) {
			CL_Disconnect( qtrue );
			CL_FlushMemory( );
			com_errorEntered = qfalse;
			CL_CDDialog();
		} else {
			Com_Printf("Server didn't have CD\n" );
		}
		longjmp (abortframe, -1);
	} else {
		CL_Shutdown ();
		SV_Shutdown (va("Server fatal crashed: %s\n", com_errorMessage));
	}

	Com_Shutdown ();

	Sys_Error ("%s", com_errorMessage);
}


/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Q_EXTERNAL_CALL Com_Quit_f( void )
{
	// don't try to shutdown if we are in a recursive error
	if ( !com_errorEntered ) {
		SV_Shutdown ("Server quit\n");
		CL_Shutdown ();
		Com_Shutdown ();
		FS_Shutdown(qtrue);
	}
	Sys_Quit ();
}



/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define	MAX_CONSOLE_LINES	32
int		com_numConsoleLines;
char	*com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine( char *commandLine ) {
    int inq = 0;
    com_consoleLines[0] = commandLine;
    com_numConsoleLines = 1;

    while ( *commandLine ) {
        if (*commandLine == '"') {
            inq = !inq;
        }
        // look for a + seperating character
        // if commandLine came from a file, we might have real line seperators
        if ( (*commandLine == '+' && !inq) || *commandLine == '\n'  || *commandLine == '\r' ) {
            if ( com_numConsoleLines == MAX_CONSOLE_LINES ) {
                return;
            }

			while ( *commandLine == '+' || *commandLine == '\n'  || *commandLine == '\r' ) {
				*commandLine = 0;
				commandLine++;
			}
			
            com_consoleLines[com_numConsoleLines] = commandLine;
            com_numConsoleLines++;
        } else 
			commandLine++;
    }
}


/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of q3config.cfg
===================
*/
qboolean Com_SafeMode( void ) {
	int		i;

	for ( i = 0 ; i < com_numConsoleLines ; i++ ) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( !Q_stricmp( Cmd_Argv(0), "safe" )
			|| !Q_stricmp( Cmd_Argv(0), "cvar_restart" ) ) {
			com_consoleLines[i][0] = 0;
			return qtrue;
		}
	}
	return qfalse;
}


/*
===============
Com_StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets should
be after execing the config and default.
===============
*/
void Com_StartupVariable( const char *match ) {
	int		i;
	char	*s;
	cvar_t	*cv;

	for (i=0 ; i < com_numConsoleLines ; i++) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( strcmp( Cmd_Argv(0), "set" ) ) {
			continue;
		}

		s = Cmd_Argv(1);
		if ( !match || !strcmp( s, match ) ) {
			Cvar_Set( s, Cmd_Argv(2) );
			cv = Cvar_Get( s, "", 0 );
			cv->flags |= CVAR_USER_CREATED;
//			com_consoleLines[i] = 0;
		}
	}
}


/*
=================
Com_AddStartupCommands

Adds command line parameters as script statements
Commands are seperated by + signs

Returns qtrue if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Com_AddStartupCommands( void ) {
	int		i;
	qboolean	added;

	added = qfalse;
	// quote every token, so args with semicolons can work
	for (i=0 ; i < com_numConsoleLines ; i++) {
		if ( !com_consoleLines[i] || !com_consoleLines[i][0] ) {
			continue;
		}

		// set commands won't override menu startup
		if ( Q_stricmpn( com_consoleLines[i], "set", 3 ) ) {
			added = qtrue;
		}
		Cbuf_AddText( com_consoleLines[i] );
		Cbuf_AddText( "\n" );
	}

	return added;
}


//============================================================================

void Info_Print( const char *s ) {
	char	key[512];
	char	value[512];
	char	*o;
	int		l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			Com_Memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Com_Printf ("%s", key);

		if (!*s)
		{
			Com_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Com_Printf ("%s\n", value);
	}
}

/*
============
Com_StringContains
============
*/
const char *Com_StringContains(const char *str1, const char *str2, int casesensitive) {
	int len, i, j;

	len = strlen(str1) - strlen(str2);
	for (i = 0; i <= len; i++, str1++) {
		for (j = 0; str2[j]; j++) {
			if (casesensitive) {
				if (str1[j] != str2[j]) {
					break;
				}
			}
			else {
				if (toupper(str1[j]) != toupper(str2[j])) {
					break;
				}
			}
		}
		if (!str2[j]) {
			return str1;
		}
	}
	return NULL;
}

/*
============
Com_Filter
============
*/
int Com_Filter(const char *filter, const char *name, int casesensitive)
{
	char buf[MAX_TOKEN_CHARS];
	const char *ptr;
	int i, found;

	while(*filter) {
		if (*filter == '*') {
			filter++;
			for (i = 0; *filter; i++) {
				if (*filter == '*' || *filter == '?') break;
				buf[i] = *filter;
				filter++;
			}
			buf[i] = '\0';
			if (strlen(buf)) {
				ptr = Com_StringContains(name, buf, casesensitive);
				if (!ptr) return qfalse;
				name = ptr + strlen(buf);
			}
		}
		else if (*filter == '?') {
			filter++;
			name++;
		}
		else if (*filter == '[' && *(filter+1) == '[') {
			filter++;
		}
		else if (*filter == '[') {
			filter++;
			found = qfalse;
			while(*filter && !found) {
				if (*filter == ']' && *(filter+1) != ']') break;
				if (*(filter+1) == '-' && *(filter+2) && (*(filter+2) != ']' || *(filter+3) == ']')) {
					if (casesensitive) {
						if (*name >= *filter && *name <= *(filter+2)) found = qtrue;
					}
					else {
						if (toupper(*name) >= toupper(*filter) &&
							toupper(*name) <= toupper(*(filter+2))) found = qtrue;
					}
					filter += 3;
				}
				else {
					if (casesensitive) {
						if (*filter == *name) found = qtrue;
					}
					else {
						if (toupper(*filter) == toupper(*name)) found = qtrue;
					}
					filter++;
				}
			}
			if (!found) return qfalse;
			while(*filter) {
				if (*filter == ']' && *(filter+1) != ']') break;
				filter++;
			}
			filter++;
			name++;
		}
		else {
			if (casesensitive) {
				if (*filter != *name) return qfalse;
			}
			else {
				if (toupper(*filter) != toupper(*name)) return qfalse;
			}
			filter++;
			name++;
		}
	}
	return qtrue;
}

/*
============
Com_FilterPath
============
*/
int Com_FilterPath(char *filter, char *name, int casesensitive)
{
	int i;
	char new_filter[MAX_QPATH];
	char new_name[MAX_QPATH];

	for (i = 0; i < MAX_QPATH-1 && filter[i]; i++) {
		if ( filter[i] == '\\' || filter[i] == ':' ) {
			new_filter[i] = '/';
		}
		else {
			new_filter[i] = filter[i];
		}
	}
	new_filter[i] = '\0';
	for (i = 0; i < MAX_QPATH-1 && name[i]; i++) {
		if ( name[i] == '\\' || name[i] == ':' ) {
			new_name[i] = '/';
		}
		else {
			new_name[i] = name[i];
		}
	}
	new_name[i] = '\0';
	return Com_Filter(new_filter, new_name, casesensitive);
}

/*
============
Com_HashKey
============
*/
int Com_HashKey(char *string, int maxlen) {
	int register hash, i;

	hash = 0;
	for (i = 0; i < maxlen && string[i] != '\0'; i++) {
		hash += string[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return hash;
}

/*
================
Com_RealTime
================
*/
int Com_RealTime(qtime_t *qtime) {
	time_t t;
	struct tm *tms;

	t = time(NULL);
	if (!qtime)
		return t;
	tms = localtime(&t);
	if (tms) {
		qtime->tm_sec = tms->tm_sec;
		qtime->tm_min = tms->tm_min;
		qtime->tm_hour = tms->tm_hour;
		qtime->tm_mday = tms->tm_mday;
		qtime->tm_mon = tms->tm_mon;
		qtime->tm_year = tms->tm_year;
		qtime->tm_wday = tms->tm_wday;
		qtime->tm_yday = tms->tm_yday;
		qtime->tm_isdst = tms->tm_isdst;
	}
	return t;
}


/*
==============================================================================

						ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

#define	ZONEID	0x1d4a11
#define MINFRAGMENT	64

typedef struct zonedebug_s {
	char *label;
	char *file;
	int line;
	int allocSize;
} zonedebug_t;

typedef struct memblock_s {
	int		size;           // including the header and possibly tiny fragments
	int     tag;            // a tag of 0 is a free block
	struct memblock_s       *next, *prev;
	int     id;        		// should be ZONEID
#ifdef ZONE_DEBUG
	zonedebug_t d;
#endif
} memblock_t;

typedef struct {
	int		size;			// total bytes malloced, including header
	int		used;			// total bytes used
	memblock_t	blocklist;	// start / end cap for linked list
	memblock_t	*rover;
} memzone_t;

// main zone for all "dynamic" memory allocation
memzone_t	*mainzone;
// we also have a small zone for small allocations that would only
// fragment the main zone (think of cvar and cmd strings)
memzone_t	*smallzone;

void Z_CheckHeap( void );

/*
========================
Z_ClearZone
========================
*/
void Z_ClearZone( memzone_t *zone, int size ) {
	memblock_t	*block;
	
	// set the entire zone to one free block

	zone->blocklist.next = zone->blocklist.prev = block =
		(memblock_t *)( (byte *)zone + sizeof(memzone_t) );
	zone->blocklist.tag = 1;	// in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->rover = block;
	zone->size = size;
	zone->used = 0;
	
	block->prev = block->next = &zone->blocklist;
	block->tag = 0;			// free block
	block->id = ZONEID;
	block->size = size - sizeof(memzone_t);
}

/*
========================
Z_AvailableZoneMemory
========================
*/
int Z_AvailableZoneMemory( memzone_t *zone ) {
	return zone->size - zone->used;
}

/*
========================
Z_AvailableMemory
========================
*/
int Z_AvailableMemory( void ) {
	return Z_AvailableZoneMemory( mainzone );
}

/*
========================
Z_Free
========================
*/
void Q_EXTERNAL_CALL Z_Free( void *ptr )
{
	memblock_t	*block, *other;
	memzone_t *zone;
	
	if (!ptr) {
		Com_Error( ERR_DROP, "Z_Free: NULL pointer" );
	}

	block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
	if (block->id != ZONEID) {
		Com_Error( ERR_FATAL, "Z_Free: freed a pointer without ZONEID" );
	}
	if (block->tag == 0) {
		Com_Error( ERR_FATAL, "Z_Free: freed a freed pointer" );
	}
	// if static memory
	if (block->tag == TAG_STATIC) {
		return;
	}

	// check the memory trash tester
	if ( *(int *)((byte *)block + block->size - 4 ) != ZONEID ) {
		Com_Error( ERR_FATAL, "Z_Free: memory block wrote past end" );
	}

	if (block->tag == TAG_SMALL) {
		zone = smallzone;
	}
	else {
		zone = mainzone;
	}

	zone->used -= block->size;
	// set the block to something that should cause problems
	// if it is referenced...
	Com_Memset( ptr, 0xaa, block->size - sizeof( *block ) );

	block->tag = 0;		// mark as free
	
	other = block->prev;
	if (!other->tag) {
		// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if (block == zone->rover) {
			zone->rover = other;
		}
		block = other;
	}

	zone->rover = block;

	other = block->next;
	if ( !other->tag ) {
		// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if (other == zone->rover) {
			zone->rover = block;
		}
	}
}


/*
================
Z_FreeTags
================
*/
void Z_FreeTags( int tag ) {
	int			count;
	memzone_t	*zone;

	if ( tag == TAG_SMALL ) {
		zone = smallzone;
	}
	else {
		zone = mainzone;
	}
	count = 0;
	// use the rover as our pointer, because
	// Z_Free automatically adjusts it
	zone->rover = zone->blocklist.next;
	do {
		if ( zone->rover->tag == tag ) {
			count++;
			Z_Free( (void *)(zone->rover + 1) );
			continue;
		}
		zone->rover = zone->rover->next;
	} while ( zone->rover != &zone->blocklist );
}


/*
================
Z_TagMalloc
================
*/
#ifdef ZONE_DEBUG
void *Z_TagMallocDebug( int size, int tag, char *label, char *file, int line ) {
#else
void *Z_TagMalloc( int size, int tag ) {
#endif
	int		extra, allocSize;
	memblock_t	*start, *rover, *new, *base;
	memzone_t *zone;

	if (!tag) {
		Com_Error( ERR_FATAL, "Z_TagMalloc: tried to use a 0 tag" );
	}

	if ( tag == TAG_SMALL ) {
		zone = smallzone;
	}
	else {
		zone = mainzone;
	}

	allocSize = size;
	//
	// scan through the block list looking for the first free block
	// of sufficient size
	//
	size += sizeof(memblock_t);	// account for size of block header
	size += 4;					// space for memory trash tester
	size = PAD(size, sizeof(intptr_t));		// align to 32/64 bit boundary
	
	base = rover = zone->rover;
	start = base->prev;
	
	do {
		if (rover == start)	{
#ifdef ZONE_DEBUG
			Z_LogHeap();
#endif
			Com_Meminfo_f();

			// scaned all the way around the list
			Com_Error( ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes from the %s zone",
								size, zone == smallzone ? "small" : "main");
			return NULL;
		}
		if (rover->tag) {
			base = rover = rover->next;
		} else {
			rover = rover->next;
		}
	} while (base->tag || base->size < size);
	
	//
	// found a block big enough
	//
	extra = base->size - size;
	if (extra > MINFRAGMENT) {
		// there will be a free fragment after the allocated block
		new = (memblock_t *) ((byte *)base + size );
		new->size = extra;
		new->tag = 0;			// free block
		new->prev = base;
		new->id = ZONEID;
		new->next = base->next;
		new->next->prev = new;
		base->next = new;
		base->size = size;
	}
	
	base->tag = tag;			// no longer a free block
	
	zone->rover = base->next;	// next allocation will start looking here
	zone->used += base->size;	//
	
	base->id = ZONEID;

#ifdef ZONE_DEBUG
	base->d.label = label;
	base->d.file = file;
	base->d.line = line;
	base->d.allocSize = allocSize;
#endif

	// marker for memory trash testing
	*(int *)((byte *)base + base->size - 4) = ZONEID;

	return (void *) ((byte *)base + sizeof(memblock_t));
}

/*
========================
Z_Malloc
========================
*/
#ifdef ZONE_DEBUG
void *Z_MallocDebug( int size, char *label, char *file, int line ) {
#else
void *Z_Malloc( int size ) {
#endif
	void	*buf;
	
  //Z_CheckHeap ();	// _DEBUG

#ifdef ZONE_DEBUG
	buf = Z_TagMallocDebug( size, TAG_GENERAL, label, file, line );
#else
	buf = Z_TagMalloc( size, TAG_GENERAL );
#endif
	Com_Memset( buf, 0, size );

	return buf;
}

#ifdef ZONE_DEBUG
void *S_MallocDebug( int size, char *label, char *file, int line ) {
	return Z_TagMallocDebug( size, TAG_SMALL, label, file, line );
}
#else
void *S_Malloc( int size ) {
	return Z_TagMalloc( size, TAG_SMALL );
}
#endif

/*
========================
Z_CheckHeap
========================
*/
void Z_CheckHeap( void ) {
	memblock_t	*block;
	
	for (block = mainzone->blocklist.next ; ; block = block->next) {
		if (block->next == &mainzone->blocklist) {
			break;			// all blocks have been hit
		}
		if ( (byte *)block + block->size != (byte *)block->next)
			Com_Error( ERR_FATAL, "Z_CheckHeap: block size does not touch the next block\n" );
		if ( block->next->prev != block) {
			Com_Error( ERR_FATAL, "Z_CheckHeap: next block doesn't have proper back link\n" );
		}
		if ( !block->tag && !block->next->tag ) {
			Com_Error( ERR_FATAL, "Z_CheckHeap: two consecutive free blocks\n" );
		}
	}
}

/*
========================
Z_LogZoneHeap
========================
*/
void Z_LogZoneHeap( memzone_t *zone, char *name ) {
#ifdef ZONE_DEBUG
	char dump[32], *ptr;
	int  i, j;
#endif
	memblock_t	*block;
	char		buf[4096];
	int size, allocSize, numBlocks;

	if (!logfile || !FS_Initialized())
		return;
	size = allocSize = numBlocks = 0;
	Com_sprintf(buf, sizeof(buf), "\r\n================\r\n%s log\r\n================\r\n", name);
	FS_Write(buf, strlen(buf), logfile);
	for (block = zone->blocklist.next ; block->next != &zone->blocklist; block = block->next) {
		if (block->tag) {
#ifdef ZONE_DEBUG
			ptr = ((char *) block) + sizeof(memblock_t);
			j = 0;
			for (i = 0; i < 20 && i < block->d.allocSize; i++) {
				if (ptr[i] >= 32 && ptr[i] < 127) {
					dump[j++] = ptr[i];
				}
				else {
					dump[j++] = '_';
				}
			}
			dump[j] = '\0';
			Com_sprintf(buf, sizeof(buf), "size = %8d: %s, line: %d (%s) [%s]\r\n", block->d.allocSize, block->d.file, block->d.line, block->d.label, dump);
			FS_Write(buf, strlen(buf), logfile);
			allocSize += block->d.allocSize;
#endif
			size += block->size;
			numBlocks++;
		}
	}
#ifdef ZONE_DEBUG
	// subtract debug memory
	size -= numBlocks * sizeof(zonedebug_t);
#else
	allocSize = numBlocks * sizeof(memblock_t); // + 32 bit alignment
#endif
	Com_sprintf(buf, sizeof(buf), "%d %s memory in %d blocks\r\n", size, name, numBlocks);
	FS_Write(buf, strlen(buf), logfile);
	Com_sprintf(buf, sizeof(buf), "%d %s memory overhead\r\n", size - allocSize, name);
	FS_Write(buf, strlen(buf), logfile);
}

/*
========================
Z_LogHeap
========================
*/
void Q_EXTERNAL_CALL Z_LogHeap( void )
{
	Z_LogZoneHeap( mainzone, "MAIN" );
	Z_LogZoneHeap( smallzone, "SMALL" );
}

// static mem blocks to reduce a lot of small zone overhead
typedef struct memstatic_s {
	memblock_t b;
	byte mem[2];
} memstatic_t;

// bk001204 - initializer brackets
static const memstatic_t emptystring =
	{ {(sizeof(memblock_t)+2 + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'\0', '\0'} };
static const memstatic_t numberstring[] = {
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'0', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'1', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'2', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'3', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'4', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'5', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'6', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'7', '\0'} },
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'8', '\0'} }, 
	{ {(sizeof(memstatic_t) + 3) & ~3, TAG_STATIC, NULL, NULL, ZONEID}, {'9', '\0'} }
};

/*
========================
CopyString

 NOTE:	never write over the memory CopyString returns because
		memory from a memstatic_t might be returned
========================
*/
char *CopyString( const char *in ) {
	char	*out;

	if (!in[0]) {
		return ((char *)&emptystring) + sizeof(memblock_t);
	}
	else if (!in[1]) {
		if (in[0] >= '0' && in[0] <= '9') {
			return ((char *)&numberstring[in[0]-'0']) + sizeof(memblock_t);
		}
	}
	out = S_Malloc (strlen(in)+1);
	strcpy (out, in);
	return out;
}

/*
==============================================================================

Goals:
	reproducable without history effects -- no out of memory errors on weird map to map changes
	allow restarting of the client without fragmentation
	minimize total pages in use at run time
	minimize total pages needed during load time

  Single block of memory with stack allocators coming from both ends towards the middle.

  One side is designated the temporary memory allocator.

  Temporary memory can be allocated and freed in any order.

  A highwater mark is kept of the most in use at any time.

  When there is no temporary memory allocated, the permanent and temp sides
  can be switched, allowing the already touched temp memory to be used for
  permanent storage.

  Temp memory must never be allocated on two ends at once, or fragmentation
  could occur.

  If we have any in-use temp memory, additional temp allocations must come from
  that side.

  If not, we can choose to make either side the new temp side and push future
  permanent allocations to the other side.  Permanent allocations should be
  kept on the side that has the current greatest wasted highwater mark.

==============================================================================
*/


#define	HUNK_MAGIC	0x89537892
#define	HUNK_FREE_MAGIC	0x89537893

typedef struct {
	int		magic;
	int		size;
} hunkHeader_t;

typedef struct hunkblock_s {
	int size;
	byte printed;
	struct hunkblock_s *next;
	char *label;
	char *file;
	int line;
} hunkblock_t;

static struct
{
	hunkblock_t *blocks;

	byte	*mem, *original;
	size_t	memSize;

	size_t	permTop, permMax;
	size_t	tempTop, tempMax;

	size_t	maxEver;

	size_t	mark;
} s_hunk;

static	int		s_zoneTotal;
static	int		s_smallZoneTotal;


/*
=================
Com_Meminfo_f
=================
*/
void Q_EXTERNAL_CALL Com_Meminfo_f( void )
{
	memblock_t	*block;
	int			zoneBytes, zoneBlocks;
	int			smallZoneBytes, smallZoneBlocks;
	int			botlibBytes, rendererBytes, otherBytes;
	int			sqlclBytes, sqlsvBytes, sqlcomBytes;

	zoneBytes = 0;
	botlibBytes = 0;
	rendererBytes = 0;
	otherBytes = 0;
	sqlclBytes = 0;
	sqlsvBytes = 0;
	sqlcomBytes = 0;
	zoneBlocks = 0;
	for (block = mainzone->blocklist.next ; ; block = block->next) {
		if ( Cmd_Argc() != 1 ) {
			Com_Printf ("block:%p    size:%7i    tag:%3i\n",
				block, block->size, block->tag);
		}
		if ( block->tag ) {
			zoneBytes += block->size;
			zoneBlocks++;
			if ( block->tag == TAG_BOTLIB ) {
				botlibBytes += block->size;
			} else if ( block->tag == TAG_RENDERER ) {
				rendererBytes += block->size;
			} else if ( block->tag == TAG_SQL_CLIENT ) {
				sqlclBytes += block->size;
			} else if ( block->tag == TAG_SQL_SERVER ) {
				sqlsvBytes += block->size;
			} else if ( block->tag == TAG_SQL_COMMON ) {
				sqlcomBytes += block->size;
			} else
				otherBytes += block->size;
		}

		if (block->next == &mainzone->blocklist) {
			break;			// all blocks have been hit	
		}
		if ( (byte *)block + block->size != (byte *)block->next) {
			Com_Printf ("ERROR: block size does not touch the next block\n");
		}
		if ( block->next->prev != block) {
			Com_Printf ("ERROR: next block doesn't have proper back link\n");
		}
		if ( !block->tag && !block->next->tag ) {
			Com_Printf ("ERROR: two consecutive free blocks\n");
		}
	}

	smallZoneBytes = 0;
	smallZoneBlocks = 0;
	for (block = smallzone->blocklist.next ; ; block = block->next) {
		if ( block->tag ) {
			smallZoneBytes += block->size;
			smallZoneBlocks++;
		}

		if (block->next == &smallzone->blocklist) {
			break;			// all blocks have been hit	
		}
	}

	Com_Printf( "%8i K total hunk\n", s_hunk.memSize / 1024 );
	Com_Printf( "%8i K total zone\n", s_zoneTotal / 1024 );
	Com_Printf( "\n" );

	Com_Printf( "%8i K used hunk (permanent)\n", s_hunk.permTop / 1024 );
	Com_Printf( "%8i K used hunk (temp)\n", s_hunk.tempTop / 1024 );
	Com_Printf( "%8i K used hunk (TOTAL)\n", (s_hunk.permTop + s_hunk.tempTop) / 1024 );
	Com_Printf( "\n" );

	Com_Printf( "%8i K max hunk (permanent)\n", s_hunk.permMax / 1024 );
	Com_Printf( "%8i K max hunk (temp)\n", s_hunk.tempMax / 1024 );
	Com_Printf( "%8i K max hunk (TOTAL)\n", (s_hunk.permMax + s_hunk.tempMax) / 1024 );
	Com_Printf( "\n" );

	Com_Printf( "%8i K max hunk since last Hunk_Clear\n", s_hunk.maxEver / 1024 );
	Com_Printf( "%8i K hunk mem never touched\n", (s_hunk.memSize - s_hunk.maxEver) / 1024 );
	Com_Printf( "%8i hunk mark value\n", s_hunk.mark );
	Com_Printf( "\n" );
	
	Com_Printf( "\n" );
	Com_Printf( "%8i bytes in %i zone blocks\n", zoneBytes, zoneBlocks	);
	Com_Printf( "        %8i bytes in dynamic botlib\n", botlibBytes );
	Com_Printf( "        %8i bytes in dynamic renderer\n", rendererBytes );
	Com_Printf( "        %8i bytes in dynamic other\n", otherBytes );
	Com_Printf( "        %8i bytes in small Zone memory\n", smallZoneBytes );
	Com_Printf( "        %8i bytes in sql client memory\n", sqlclBytes );
	Com_Printf( "        %8i bytes in sql server memory\n", sqlsvBytes );
	Com_Printf( "        %8i bytes in sql common memory\n", sqlcomBytes );
}

/*
===============
Com_TouchMemory

Touch all known used data to make sure it is paged in
===============
*/
void Com_TouchMemory( void )
{
	int		start, end;
	int		i, j;
	int		sum;
	memblock_t	*block;

	Z_CheckHeap();

	start = Sys_Milliseconds();

	sum = 0;

	for (block = mainzone->blocklist.next ; ; block = block->next) {
		if ( block->tag ) {
			j = block->size >> 2;
			for ( i = 0 ; i < j ; i+=64 ) {				// only need to touch each page
				sum += ((int *)block)[i];
			}
		}
		if ( block->next == &mainzone->blocklist ) {
			break;			// all blocks have been hit	
		}
	}

	end = Sys_Milliseconds();

	Com_Printf( "Com_TouchMemory: %i msec\n", end - start );
}



/*
=================
Com_InitZoneMemory
=================
*/
void Com_InitSmallZoneMemory( void ) {
	s_smallZoneTotal = 512 * 1024;
	// bk001205 - was malloc
	smallzone = calloc( s_smallZoneTotal, 1 );
	if ( !smallzone ) {
		Com_Error( ERR_FATAL, "Small zone data failed to allocate %1.1f megs", (float)s_smallZoneTotal / (1024*1024) );
	}
	Z_ClearZone( smallzone, s_smallZoneTotal );
	
	return;
}

void Com_InitZoneMemory( void ) {
	cvar_t	*cv;

	// allocate the random block zone
	cv = Cvar_Get( "com_zoneMegs", DEF_COMZONEMEGS_S, CVAR_INIT );

	s_zoneTotal = cv->integer * 1024 * 1024;

	// bk001205 - was malloc
	mainzone = calloc( s_zoneTotal, 1 );
	if ( !mainzone ) {
		Com_Error( ERR_FATAL, "Zone data failed to allocate %i megs", s_zoneTotal / (1024*1024) );
	}
	Z_ClearZone( mainzone, s_zoneTotal );

}

/*
=================
Hunk_Log
=================
*/
void Q_EXTERNAL_CALL Hunk_Log( void )
{
	hunkblock_t	*block;
	char		buf[4096];
	int size, numBlocks;

	if (!logfile || !FS_Initialized())
		return;
	size = 0;
	numBlocks = 0;
	Com_sprintf(buf, sizeof(buf), "\r\n================\r\nHunk log\r\n================\r\n");
	FS_Write(buf, strlen(buf), logfile);
	for (block = s_hunk.blocks; block; block = block->next) {
#ifdef HUNK_DEBUG
		Com_sprintf(buf, sizeof(buf), "size = %8d: %s, line: %d (%s)\r\n", block->size, block->file, block->line, block->label);
		FS_Write(buf, strlen(buf), logfile);
#endif
		size += block->size;
		numBlocks++;
	}
	Com_sprintf(buf, sizeof(buf), "%d Hunk memory\r\n", size);
	FS_Write(buf, strlen(buf), logfile);
	Com_sprintf(buf, sizeof(buf), "%d hunk blocks\r\n", numBlocks);
	FS_Write(buf, strlen(buf), logfile);
}

/*
=================
Hunk_SmallLog
=================
*/
void Q_EXTERNAL_CALL Hunk_SmallLog( void)
{
	hunkblock_t	*block, *block2;
	char		buf[4096];
	int size, locsize, numBlocks;

	if (!logfile || !FS_Initialized())
		return;
	for (block = s_hunk.blocks ; block; block = block->next) {
		block->printed = qfalse;
	}
	size = 0;
	numBlocks = 0;
	Com_sprintf(buf, sizeof(buf), "\r\n================\r\nHunk Small log\r\n================\r\n");
	FS_Write(buf, strlen(buf), logfile);
	for (block = s_hunk.blocks; block; block = block->next) {
		if (block->printed) {
			continue;
		}
		locsize = block->size;
		for (block2 = block->next; block2; block2 = block2->next) {
			if (block->line != block2->line) {
				continue;
			}
			if (Q_stricmp(block->file, block2->file)) {
				continue;
			}
			size += block2->size;
			locsize += block2->size;
			block2->printed = qtrue;
		}
#ifdef HUNK_DEBUG
		Com_sprintf(buf, sizeof(buf), "size = %8d: %s, line: %d (%s)\r\n", locsize, block->file, block->line, block->label);
		FS_Write(buf, strlen(buf), logfile);
#endif
		size += block->size;
		numBlocks++;
	}
	Com_sprintf(buf, sizeof(buf), "%d Hunk memory\r\n", size);
	FS_Write(buf, strlen(buf), logfile);
	Com_sprintf(buf, sizeof(buf), "%d hunk blocks\r\n", numBlocks);
	FS_Write(buf, strlen(buf), logfile);
}

/*
=================
Com_InitZoneMemory
=================
*/
void Com_InitHunkMemory( void )
{
	cvar_t	*cv;
	int nMinAlloc;
	char *pMsg = NULL;

	Com_Memset( &s_hunk, 0, sizeof( s_hunk ) );

	// make sure the file system has allocated and "not" freed any temp blocks
	// this allows the config and product id files ( journal files too ) to be loaded
	// by the file system without redunant routines in the file system utilizing different 
	// memory systems
	if (FS_LoadStack() != 0) {
		Com_Error( ERR_FATAL, "Hunk initialization failed. File system load stack not zero");
	}

	// allocate the stack based hunk allocator
	cv = Cvar_Get( "com_hunkMegs", DEF_COMHUNKMEGS_S, CVAR_LATCH | CVAR_ARCHIVE );

	// if we are not dedicated min allocation is 56, otherwise min is 1
	if (com_dedicated && com_dedicated->integer) {
		nMinAlloc = MIN_DEDICATED_COMHUNKMEGS;
		pMsg = "Minimum com_hunkMegs for a dedicated server is %i, allocating %i megs.\n";
	}
	else {
		nMinAlloc = MIN_COMHUNKMEGS;
		pMsg = "Minimum com_hunkMegs is %i, allocating %i megs.\n";
	}

	if( cv->integer < nMinAlloc )
	{
		s_hunk.memSize = 1024 * 1024 * nMinAlloc;
	    Com_Printf( pMsg, nMinAlloc, s_hunk.memSize / (1024 * 1024) );
	}
	else
	{
		s_hunk.memSize = cv->integer * 1024 * 1024;
	}


	// bk001205 - was malloc
	s_hunk.original = calloc( s_hunk.memSize + 31, 1 );
	if( !s_hunk.original ) {
		Com_Error( ERR_FATAL, "Hunk data failed to allocate %i megs", s_hunk.memSize / (1024*1024) );
	}
	// cacheline align
	s_hunk.mem = (byte *) ( ( (intptr_t)s_hunk.original + 31 ) & ~31 );

	Hunk_Clear();
	CM_ClearPreloaderData();

	Cmd_AddCommand( "meminfo", Com_Meminfo_f );
#ifdef ZONE_DEBUG
	Cmd_AddCommand( "zonelog", Z_LogHeap );
#endif
#ifdef HUNK_DEBUG
	Cmd_AddCommand( "hunklog", Hunk_Log );
	Cmd_AddCommand( "hunksmalllog", Hunk_SmallLog );
#endif
}

void Com_ReleaseMemory( void )
{
	if( s_hunk.original )
		free( s_hunk.original );
	memset( &s_hunk, 0, sizeof( s_hunk ) );

	if( smallzone )
	{
		free( smallzone );
		smallzone = 0;
	}

	if( mainzone )
	{
		free( mainzone );
		mainzone = 0;
	}
}

/*
====================
Hunk_MemoryRemaining
====================
*/
int	Hunk_MemoryRemaining( void )
{
	return s_hunk.memSize - s_hunk.permTop - s_hunk.tempTop;
}

/*
===================
Hunk_SetMark

The server calls this after the level and game VM have been loaded
===================
*/
void Hunk_SetMark( void )
{
	s_hunk.mark = s_hunk.permTop;
}

/*
=================
Hunk_ClearToMark

The client calls this before starting a vid_restart or snd_restart
=================
*/
void Hunk_ClearToMark( void )
{
	s_hunk.permTop = s_hunk.mark;
	s_hunk.permMax = s_hunk.permTop;

	s_hunk.tempMax = s_hunk.tempTop = 0;
}

/*
=================
Hunk_CheckMark
=================
*/
qboolean Hunk_CheckMark( void )
{
	return s_hunk.mark ? qtrue : qfalse;
}

void CL_ShutdownCGame( void );
void CL_ShutdownUI( qboolean );
void SV_ShutdownGameProgs( void );

/*
=================
Hunk_Clear

The server calls this before shutting down or loading a new map
=================
*/

static void Hunk_FrameInit( void );
static void Hunk_FrameKill( void );

void Hunk_Clear( void )
{

#ifndef DEDICATED
	CL_ShutdownCGame();
	CL_ShutdownUI( qfalse );
#endif
	SV_ShutdownGameProgs();
#ifndef DEDICATED
	CIN_CloseAllVideos();
#endif
	s_hunk.permTop = 0;
	s_hunk.permMax = 0;
	s_hunk.tempTop = 0;
	s_hunk.tempMax = 0;
	s_hunk.maxEver = 0;
	s_hunk.mark = 0;

	Com_Printf( "Hunk_Clear: reset the hunk ok\n" );
	VM_Clear();

#ifdef HUNK_DEBUG
	s_hunk.blocks = NULL;
#endif

	//stake out a chunk for the frame temp data
	Hunk_FrameInit();
}

static void Hunk_SwapBanks( void ) { }

/*
=================
Hunk_Alloc

Allocate permanent (until the hunk is cleared) memory
=================
*/
#ifdef HUNK_DEBUG
void* Q_EXTERNAL_CALL Hunk_AllocDebug( int size, ha_pref preference, char *label, char *file, int line ) {
#else
void* Q_EXTERNAL_CALL Hunk_Alloc( int size, ha_pref preference ) {
#endif
	void	*buf;

	if ( s_hunk.mem == NULL)
	{
		Com_Error( ERR_FATAL, "Hunk_Alloc: Hunk memory system not initialized" );
	}

#ifdef HUNK_DEBUG
	size += sizeof( hunkblock_t );
#endif

	// round to cacheline
	size = (size + 31) & ~31;

	if( s_hunk.permTop + s_hunk.tempTop + size > s_hunk.memSize )
	{
#ifdef HUNK_DEBUG
		Hunk_Log();
		Hunk_SmallLog();
#endif
		Com_Error( ERR_DROP, "Hunk_Alloc failed on %i", size );
	}

	buf = s_hunk.mem + s_hunk.permTop;
	s_hunk.permTop += size;

	if( s_hunk.permTop > s_hunk.permMax )
		s_hunk.permMax = s_hunk.permTop;

	if( s_hunk.permTop + s_hunk.tempTop > s_hunk.maxEver )
		s_hunk.maxEver = s_hunk.permTop + s_hunk.tempTop;

	Com_Memset( buf, 0, size );

#ifdef HUNK_DEBUG
	{
		hunkblock_t *block;

		block = (hunkblock_t *) buf;
		block->size = size - sizeof(hunkblock_t);
		block->file = file;
		block->label = label;
		block->line = line;
		block->next = s_hunk.blocks;
		s_hunk.blocks = block;
		buf = ((byte *) buf) + sizeof(hunkblock_t);
	}
#endif

	return buf;
}

/*
=================
Hunk_AllocateTempMemory

This is used by the file loading system.
Multiple files can be loaded in temporary memory.
When the files-in-use count reaches zero, all temp memory will be deleted
=================
*/
void* Q_EXTERNAL_CALL Hunk_AllocateTempMemory( int size )
{
	void		*buf;
	hunkHeader_t	*hdr;

	// return a Z_Malloc'd block if the hunk has not been initialized
	// this allows the config and product id files ( journal files too ) to be loaded
	// by the file system without redunant routines in the file system utilizing different 
	// memory systems
	if ( s_hunk.mem == NULL )
	{
		return Z_Malloc(size);
	}

	Hunk_SwapBanks();

	size = PAD( size, sizeof( intptr_t ) ) + sizeof( hunkHeader_t );

	if( s_hunk.permTop + s_hunk.tempTop + size > s_hunk.memSize )
	{
		Com_Error( ERR_DROP, "Hunk_AllocateTempMemory: failed on %i", size );
	}

	s_hunk.tempTop += size;
	buf = s_hunk.mem + s_hunk.memSize - s_hunk.tempTop;

	if( s_hunk.tempTop > s_hunk.tempMax )
		s_hunk.tempMax = s_hunk.tempTop;

	if( s_hunk.permTop + s_hunk.tempTop > s_hunk.maxEver )
		s_hunk.maxEver = s_hunk.permTop + s_hunk.tempTop;

	hdr = (hunkHeader_t*)buf;
	buf = (void*)(hdr + 1);

	hdr->magic = HUNK_MAGIC;
	hdr->size = size;

	// don't bother clearing, because we are going to load a file over it
	return buf;
}


/*
==================
Hunk_FreeTempMemory
==================
*/
void Q_EXTERNAL_CALL Hunk_FreeTempMemory( void *buf )
{
	hunkHeader_t	*hdr;

	  // free with Z_Free if the hunk has not been initialized
	  // this allows the config and product id files ( journal files too ) to be loaded
	  // by the file system without redunant routines in the file system utilizing different 
	  // memory systems
	if( !s_hunk.mem )
	{
		Z_Free( buf );
		return;
	}


	hdr = (hunkHeader_t*)buf - 1;
	if( hdr->magic != HUNK_MAGIC )
	{
		Com_Error( ERR_FATAL, "Hunk_FreeTempMemory: bad magic" );
	}

	hdr->magic = HUNK_FREE_MAGIC;

	// this only works if the files are freed in stack order,
	// otherwise the memory will stay around until Hunk_ClearTempMemory
	if( (byte*)hdr == s_hunk.mem + s_hunk.memSize - s_hunk.tempTop )
	{
		s_hunk.tempTop -= hdr->size;
	}
}


/*
=================
Hunk_ClearTempMemory

The temp space is no longer needed.  If we have left more
touched but unused memory on this side, have future
permanent allocs use this side.
=================
*/
void Hunk_ClearTempMemory( void )
{
	if( s_hunk.mem )
	{
		s_hunk.tempTop = 0;
		s_hunk.tempMax = 0;
	}
}

static byte *s_frameStackLoc = 0;
static byte *s_frameStackBase = 0;
static byte *s_frameStackEnd = 0;

static void Hunk_FrameInit( void )
{
	int megs = Cvar_Get( "com_hunkFrameMegs", "1", CVAR_LATCH | CVAR_ARCHIVE )->integer;
	size_t cb;

	if( megs < 1 )
		megs = 1;

	cb = 1024 * 1024 * megs;

	s_frameStackBase = Hunk_Alloc( cb, h_low );
	s_frameStackEnd = s_frameStackBase + cb;

	s_frameStackLoc = s_frameStackBase;
}

static void Hunk_FrameKill( void )
{
	s_frameStackBase = 0;
	s_frameStackEnd = 0;

	s_frameStackLoc = 0;
}

void* Q_EXTERNAL_CALL Hunk_FrameAlloc( size_t cb )
{
	void *ret;

	if( s_frameStackLoc + cb >= s_frameStackEnd )
		//out of frame stack memory
		return 0;

	ret = s_frameStackLoc;
	s_frameStackLoc += cb;

	//Com_Memset( ret, 0, cb );
	return ret;
}

void Hunk_FrameReset( void )
{
	s_frameStackLoc = s_frameStackBase;
}

/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

// bk001129 - here we go again: upped from 64
// FIXME TTimo blunt upping from 256 to 1024
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=5
#define	MAX_PUSHED_EVENTS	            1024
// bk001129 - init, also static
static int com_pushedEventsHead = 0;
static int com_pushedEventsTail = 0;
// bk001129 - static
static sysEvent_t	com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling( void ) {
	
	int i;

	Com_StartupVariable( "journal" );
	com_journal = Cvar_Get ("journal", "0", CVAR_INIT);
	if ( !com_journal->integer ) {
		if ( com_journal->string && com_journal->string[ 0 ] == '_' ) {
			Com_Printf( "Replaying journaled events\n");
			FS_FOpenFileRead( va("journal%s.dat",com_journal->string), &com_journalFile, qtrue );
			FS_FOpenFileRead( va("journal_data%s.dat",com_journal->string), &com_journalDataFile, qtrue );
			com_journal->integer = 2;
		} else
			return;
	} else {
		for ( i=0; i <= 9999 ; i++ ) {
			char f[MAX_OSPATH];
			Com_sprintf( f, sizeof(f), "journal_%04d.dat", i );
			if( !FS_FileExists( f ) )
				break;
		}

		if ( com_journal->integer == 1 ) {
			Com_Printf( "Journaling events\n");
			com_journalFile		= FS_FOpenFileWrite( va("journal_%04d.dat",i) );
			com_journalDataFile	= FS_FOpenFileWrite( va("journal_data_%04d.dat",i) );
		} else if ( com_journal->integer == 2 ) {
			i--;
			Com_Printf( "Replaying journaled events\n");
			FS_FOpenFileRead( va("journal_%04d.dat",i), &com_journalFile, qtrue );
			FS_FOpenFileRead( va("journal_data_%04d.dat",i), &com_journalDataFile, qtrue );
		}
	}

	if ( !com_journalFile || !com_journalDataFile ) {
		Cvar_Set( "journal", "0" );
		com_journalFile = 0;
		com_journalDataFile = 0;
		Com_Printf( "Couldn't open journal files\n" );
	}
}

/*
=================
Com_GetRealEvent
=================
*/
sysEvent_t	Com_GetRealEvent( void ) {
	int			r;
	sysEvent_t	ev;

	// either get an event from the system or the journal file
	if ( com_journal->integer == 2 ) {
		r = FS_Read( &ev, sizeof(ev), com_journalFile );
		if ( r != sizeof(ev) ) {
			//Com_Error( ERR_FATAL, "Error reading from journal file" );
			com_journal->integer = 0;
			ev.evType = SE_NONE;
		}
		if ( ev.evPtrLength ) {
			ev.evPtr = Z_Malloc( ev.evPtrLength );
			r = FS_Read( ev.evPtr, ev.evPtrLength, com_journalFile );
			if ( r != ev.evPtrLength ) {
				//Com_Error( ERR_FATAL, "Error reading from journal file" );
				com_journal->integer = 0;
				ev.evType = SE_NONE;
			}
		}
	} else {
		ev = Sys_GetEvent();

		// write the journal value out if needed
		if ( com_journal->integer == 1 ) {
			r = FS_Write( &ev, sizeof(ev), com_journalFile );
			if ( r != sizeof(ev) ) {
				Com_Error( ERR_FATAL, "Error writing to journal file" );
			}
			if ( ev.evPtrLength ) {
				r = FS_Write( ev.evPtr, ev.evPtrLength, com_journalFile );
				if ( r != ev.evPtrLength ) {
					Com_Error( ERR_FATAL, "Error writing to journal file" );
				}
			}
		}
	}

	return ev;
}


/*
=================
Com_InitPushEvent
=================
*/
// bk001129 - added
void Com_InitPushEvent( void ) {
  // clear the static buffer array
  // this requires SE_NONE to be accepted as a valid but NOP event
  memset( com_pushedEvents, 0, sizeof(com_pushedEvents) );
  // reset counters while we are at it
  // beware: GetEvent might still return an SE_NONE from the buffer
  com_pushedEventsHead = 0;
  com_pushedEventsTail = 0;
}


/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent( sysEvent_t *event ) {
	sysEvent_t		*ev;
	static int printedWarning = 0; // bk001129 - init, bk001204 - explicit int

	ev = &com_pushedEvents[ com_pushedEventsHead & (MAX_PUSHED_EVENTS-1) ];

	if ( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS ) {

		// don't print the warning constantly, or it can give time for more...
		if ( !printedWarning ) {
			printedWarning = qtrue;
			Com_Printf( "WARNING: Com_PushEvent overflow\n" );
		}

		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		com_pushedEventsTail++;
	} else {
		printedWarning = qfalse;
	}

	*ev = *event;
	com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t	Com_GetEvent( void ) {
	if ( com_pushedEventsHead > com_pushedEventsTail ) {
		com_pushedEventsTail++;
		return com_pushedEvents[ (com_pushedEventsTail-1) & (MAX_PUSHED_EVENTS-1) ];
	}
	return Com_GetRealEvent();
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket( netadr_t *evFrom, msg_t *buf ) {
	int		t1, t2, msec;

	t1 = 0;

	if ( com_speeds->integer ) {
		t1 = Sys_Milliseconds ();
	}

	SV_PacketEvent( *evFrom, buf );

	if ( com_speeds->integer ) {
		t2 = Sys_Milliseconds ();
		msec = t2 - t1;
		if ( com_speeds->integer == 3 ) {
			Com_Printf( "SV_PacketEvent time: %i\n", msec );
		}
	}
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/
int Com_EventLoop( void ) {
	sysEvent_t	ev;
	netadr_t	evFrom;
	byte		bufData[MAX_MSGLEN];
	msg_t		buf;

	MSG_Init( &buf, bufData, sizeof( bufData ) );

	while ( 1 ) {
		ev = Com_GetEvent();

		// if no more events are available
		if ( ev.evType == SE_NONE ) {
			// manually send packet events for the loopback channel
			while ( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) ) {
				CL_PacketEvent( evFrom, &buf );
			}

			while ( NET_GetLoopPacket( NS_SERVER, &evFrom, &buf ) ) {
				// if the server just shut down, flush the events
				if ( com_sv_running->integer ) {
					Com_RunAndTimeServerPacket( &evFrom, &buf );
				}
			}

			return ev.evTime;
		}


		switch ( ev.evType ) {
		default:
		  // bk001129 - was ev.evTime
			Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evType );
			break;
        case SE_NONE:
            break;
		case SE_KEY:
			CL_KeyEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_CHAR:
			CL_CharEvent( ev.evValue );
			break;
		case SE_MOUSE_ABS:
			CL_MouseEvent( ev.evValue, ev.evValue2, true );
			break;
		case SE_MOUSE_REL:
			CL_MouseEvent( ev.evValue, ev.evValue2, false );
			break;
		case SE_JOYSTICK_AXIS:
			CL_JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_CONSOLE:
			Cbuf_AddText( (char *)ev.evPtr );
			Cbuf_AddText( "\n" );
			break;
		case SE_PACKET:
			// this cvar allows simulation of connections that
			// drop a lot of packets.  Note that loopback connections
			// don't go through here at all.
			if ( com_dropsim->value > 0 ) {

				if ( Rand_NextInt32InRange( &com_db.rand, 1, 0xFFFF ) < com_dropsim->value ) {
					break;		// drop this packet
				}
			}

			evFrom = *(netadr_t *)ev.evPtr;
			buf.cursize = ev.evPtrLength - sizeof( evFrom );

			// we must copy the contents of the message out, because
			// the event buffers are only large enough to hold the
			// exact payload, but channel messages need to be large
			// enough to hold fragment reassembly
			if ( (unsigned)buf.cursize > buf.maxsize ) {
				Com_Printf("Com_EventLoop: oversize packet\n");
				continue;
			}
			Com_Memcpy( buf.data, (byte *)((netadr_t *)ev.evPtr + 1), buf.cursize );
			if ( com_sv_running->integer ) {
				Com_RunAndTimeServerPacket( &evFrom, &buf );
			} else {
				CL_PacketEvent( evFrom, &buf );
			}
			break;
		case SE_RANDSEED:
			com_randseed = ev.evValue;
			break;
		}

		// free any block data
		if ( ev.evPtr ) {
			Z_Free( ev.evPtr );
		}
	}

	return 0;	// never reached
}

/*
================
Com_Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int Com_Milliseconds (void) {
	sysEvent_t	ev;

	// get events and push them until we get a null event with the current time
	do {

		ev = Com_GetRealEvent();
		if ( ev.evType != SE_NONE ) {
			Com_PushEvent( &ev );
		}
	} while ( ev.evType != SE_NONE );
	
	return ev.evTime;
}

//============================================================================

extern int cmd_waitfordownload;

#ifndef DEDICATED
extern void CL_DownloadsComplete( void );
#endif


static void download_complete() {

	//	see how many downloads remain
	sql_prepare	( &com_db, "SELECT COUNT(*) FROM openfiles WHERE pak=1;" );
	if ( sql_step( &com_db ) ) {
		int	n = sql_columnasint( &com_db, 0 );
		if ( n == 0 ) {
			cmd_waitfordownload = 0;
			Cvar_Set( "com_downloading", "0" );
#ifndef DEDICATED
			if ( com_cl_running->integer ) {
				CL_DownloadsComplete();
			}
#endif
		}
	}
	sql_done	( &com_db );
}

/*
================
================
*/
static int QDECL Com_wget_write( httpInfo_e code, const char * buffer, int length, void * notifyData ) {

	static int		lasttime;
	fileHandle_t	f		= (fileHandle_t)notifyData;
	int				time	= Sys_Milliseconds();


	switch( code )
	{
	case HTTP_LENGTH:
		{
			int length = atoi(buffer);
			//	record the length of this file so we can tell if we have a partial download
			sql_prepare	( &com_db, "UPDATE openfiles SET length=xfer+?2 SEARCH f ?1;" );
			sql_bindint	( &com_db, 1, f );
			sql_bindint	( &com_db, 2, length );
			sql_step	( &com_db );
			sql_done	( &com_db );

		} break;

	case HTTP_WRITE:
		{
			if ( time - lasttime > 2000 ) {
				lasttime = time;

				sql_prepare	( &com_db, "SELECT 'downloading ' || name || ' @ ' || (xfer/1024)/((SYS_TIME-time)/1000):plain || ' kb/s\n',(xfer/1024)/((SYS_TIME-time)/1000):plain FROM openfiles SEARCH f ?1 WHERE (SYS_TIME-time)>1500;" );
				sql_bindint	( &com_db, 1, f );

				if ( sql_step( &com_db ) ) {
					Com_Printf	( sql_columnastext( &com_db, 0 ) );
					Cvar_Set	( "com_downloaded_rate", sql_columnastext( &com_db, 1 ) );
				}

				sql_done	( &com_db );
			}

			FS_Write( buffer, length, f );
		} break;

	case HTTP_FAILED:
		if ( cmd_waitfordownload ) {
			Com_Error( ERR_FATAL, "HTTP Error: (%d): %s\n", length, buffer );
		}

		Com_Printf( "Transfer Failed!\n" );

		//	fall throught
	case HTTP_DONE:
		{
			sql_prepare	( &com_db, "SELECT name,xfer,((SYS_TIME-time)/1000) FROM openfiles SEARCH f ?1;" );
			sql_bindint	( &com_db, 1, f );
			if ( sql_step( &com_db ) ) {
				const char *	name = sql_columnastext	( &com_db, 0 );
				int				xfer = sql_columnasint	( &com_db, 1 );
				int				time = sql_columnasint	( &com_db, 2 );

				Com_Printf	( "'%s' done downloading! %d bytes\n", name, xfer );
				if ( time > 0 ) {
					Com_Printf( "%d kb/s\n", (xfer/1024)/time );
				}
			}

			sql_done	( &com_db );

			FS_FCloseFile( f );

			download_complete();
			
		} break;
	}

	return length;
}

#ifdef USE_WEBHOST
/*
================
sync
================
*/
static int sync( const char * filename )
{
	fileHandle_t	f;
	int				length;
	int				sv_length;
	char			sv_md5[ 33 ];
	int				pure;
	const char *	info;
	char *			tmp;

	//	check if pak file is ok
	pure = FS_PakIsPure( filename );

	//	pak file passes test, nothing to do
	if ( pure == 1 ) {
		return 0;
	}

	//	not allowed to download, pak doesn't exist.
	if ( com_allowDownload->integer == 0 || !com_webhost->string[0] ) {
		return -1;
	}

	info = FS_GetMD5AndLength( filename );

	//	get the MD5 of the pak on the website
	Q_strncpyz( sv_md5, COM_Parse( &info ), sizeof(sv_md5) );
	sv_length = atoi( COM_Parse( &info ) );


	//	make sure file is not already downloading
	sql_prepare	( &com_db, "SELECT f FROM openfiles WHERE name like $1||'~'||$2;" );
	sql_bindtext( &com_db, 1, filename );
	sql_bindtext( &com_db, 2, sv_md5 );
	if ( sql_step( &com_db ) ) {
		sql_done( &com_db );
		return 1;
	}
	sql_done	( &com_db );

	//	create a tmp name using the md5 string
	tmp = va("~/%s~%s",filename,sv_md5);

	//	open the tmp file to write
	length = FS_FOpenFileByMode( tmp, &f, FS_APPEND );

	//	if the temp file is larger than the destination must start over
	if ( length >= sv_length ) {
		FS_FCloseFile( f );
		length = FS_FOpenFileByMode( tmp, &f, FS_WRITE );
	}

	if ( !f ) {
		Com_Printf( "sync: couldn't open '%s' for writing.\n", filename );
		return -1;
	}

	//	mark this openfile as a pak download
	sql_prepare	( &com_db, "UPDATE openfiles SET pak=1 SEARCH f ?1;" );
	sql_bindint	( &com_db, 1, f );
	sql_step	( &com_db );
	sql_done	( &com_db );

	//	download the rest of it
	Com_Printf( "downloading '%s'\n", filename );
	HTTP_GetUrl( va("%s/%s", com_webhost->string, filename), Com_wget_write, (void*)f, length );

	Cvar_Set( "com_downloading", "1" );

	return 1;
}

/*
================
Com_sync_f
================
*/
static void Q_EXTERNAL_CALL Com_sync_f( void ) {

	char *			filename;

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "Usages: sync <filename> <wait>\n" );
		return;
	}

	filename = Cmd_Argv(1);

	switch( sync( filename ) )
	{
	case -1:
		Com_Error( ERR_DROP, "Unable to sync '%s'.\n", filename );
		break;

	case 0:
		Cbuf_ExecuteText( EXEC_NOW, "donedl" );
		break;

	case 1:
		if ( Cmd_Argc() == 3 ) {
			cmd_waitfordownload = atoi(Cmd_Argv(2));
		}
		break;
	}
}

/*
================
Com_SyncAll
================
*/
int Com_SyncAll( void ) {

	int w = 0;
	char * info = Cvar_VariableString( "sv_paks" );

	for ( ;; )
	{
		const char * name = COM_Parse( &info );
		if ( name[0] == '\0' )
			break;

		if ( sync( name ) )
			w = 1;
		COM_Parse( &info );	// skip md5
		COM_Parse( &info );	// skip length
	}

	return w;
}

/*
================
Com_sync_all_f
================
*/
static void Q_EXTERNAL_CALL Com_sync_all_f( void ) {

	char * arg = Cmd_Argv(1);

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "Usages: sync_all <wait>|cancel\n" );
		return;
	}

	if ( !Q_stricmp( arg, "cancel" ) ) {

		sql_prepare	( &com_db, "SELECT f FROM openfiles;" );
		while ( sql_step( &com_db ) ) {
			FS_FCloseFile( sql_columnasint( &com_db, 0 ) );
		}
		sql_done	( &com_db );

		download_complete();

	} else {
		if ( Com_SyncAll() ) {
			if ( Cmd_Argc() == 2 ) {
				cmd_waitfordownload = atoi(Cmd_Argv(1));
			}
		}
	}
}
#endif


/*
================
Com_warmboot_f
================
*/
static void Q_EXTERNAL_CALL Com_warmboot_f( void ) {

	if ( Cvar_VariableIntegerValue( "fs_restart" ) == 1 ) {
		FS_ConditionalRestart( 0 );
		CL_Disconnect( qtrue );
		CL_FlushMemory( );
		longjmp(abortframe, -1);
	}
}

#ifdef USE_WEBHOST
static void Q_EXTERNAL_CALL Com_StopDownloads_f(void) {
	Net_HTTP_Kill();
	Net_HTTP_Init();
	Cvar_Set("com_downloading", "0");
	Cvar_Set("sv_paks", "");
	Cbuf_ExecuteText(EXEC_INSERT, "sync_all cancel\n");
}
#endif

static void Q_EXTERNAL_CALL Com_CheckGameVersion_f(void) {
	Sys_SwapVersion();
}
#ifndef DEDICATED
#ifdef USE_DRM
static void Q_EXTERNAL_CALL Com_Register_f(void) {

	int r;
	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "Usages: register <cd key>\n" );
		Cvar_Set( "com_registermessage", "7" );
		return;
	}
	r = DRM_Register(Cmd_Argv(1));

	Cvar_Set( "com_keyvalid", va("%d", r==4) );
	Cvar_Set( "com_registermessage", va("%d",r) );
}

static void Q_EXTERNAL_CALL Com_Unregister_f(void) {
	DRM_Cancel();
	Cvar_Set( "com_keyvalid", va("%d", 0) );
	Com_Error( ERR_NEED_CD, "Invalid Key" );
}

static void Q_EXTERNAL_CALL Com_RegistrationStatus_f(void) {
	int reg_status;
	reg_status = DRM_Validate();
	Com_Printf("Registration Status: %d\n", reg_status);
}
#endif
#endif
/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Q_EXTERNAL_CALL Com_Error_f( void )
{
	if ( Cmd_Argc() > 1 ) {
		Com_Error( ERR_DROP, "Testing drop error" );
	} else {
		Com_Error( ERR_FATAL, "Testing fatal error" );
	}
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Q_EXTERNAL_CALL Com_Freeze_f( void )
{
	float	s;
	int		start, now;

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "freeze <seconds>\n" );
		return;
	}
	s = atof( Cmd_Argv(1) );

	start = Com_Milliseconds();

	while ( 1 ) {
		now = Com_Milliseconds();
		if ( ( now - start ) * 0.001 > s ) {
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force an access violation for development reasons
=================
*/
static void Q_EXTERNAL_CALL Com_Crash_f( void )
{
	*(int*)0 = 0x12345678;
}


static void Com_DetectAltivec(void)
{
	// Only detect if user hasn't forcibly disabled it.
	if (com_altivec->integer) {
		static qboolean altivec = qfalse;
		static qboolean detected = qfalse;
		if (!detected) {
			altivec = Sys_DetectAltivec();
			detected = qtrue;
		}

		if (!altivec) {
			Cvar_Set( "com_altivec", "0" );  // we don't have it! Disable support!
		}
	}
}

/*
=================
Com_Init
=================
*/
void Com_Init( char *commandLine ) {
	char	*s;

	Com_Printf( "%s %s %s\n%s\n", Q3_VERSION, PLATFORM_STRING, __DATE__, commandLine );

#ifndef DEDICATED
#ifdef USE_DRM
	//initialize the DRM
	DRM_Init();
#endif
#endif

	if ( setjmp (abortframe) ) {
		Sys_Error ("Error during initialization");
	}

	// bk001129 - do this before anything else decides to push events
	Com_InitPushEvent();

	Com_InitSmallZoneMemory();

	Cvar_Init ();

	// Save the command line if we swap the exe
	com_cmdline = Cvar_Get("com_cmdline", commandLine, CVAR_ROM);


	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	Com_ParseCommandLine( commandLine );

//	Swap_Init ();
	Cbuf_Init ();

	Com_StartupVariable( "com_zoneMegs" );

	Com_InitZoneMemory();
	Cmd_Init ();

	// override anything from the config files with command line args
	Com_StartupVariable( NULL );

	// get the developer cvar set as early as possible
	Com_StartupVariable( "developer" );

	// done early so bind command exists
	CL_InitKeyCommands();

	// init the sql db, stores file info
	sql_reset( &com_db, TAG_SQL_COMMON );

	// com_allowDownload determines the install path
	Com_StartupVariable( "com_allowDownload" );

	FS_InitFilesystem ();

	Com_InitJournaling();

	Cbuf_AddText ("exec default.cfg\n");

	// skip the q3config.cfg if "safe" is on the command line
	if ( !Com_SafeMode() ) {
		Cbuf_AddText ("exec st.cfg\n");
	}

	Cbuf_AddText ("exec autoexec.cfg\n");

	Cbuf_Execute ();

	// override anything from the config files with command line args
	Com_StartupVariable( NULL );

  // get dedicated here for proper hunk megs initialization
#ifdef DEDICATED
	com_dedicated = Cvar_Get ("dedicated", "1", CVAR_ROM);
#else
	com_dedicated = Cvar_Get ("dedicated", "0", CVAR_LATCH);
#endif

	// allocate the stack based hunk allocator
	Com_InitHunkMemory();

	// if any archived cvars are modified after this, we will trigger a writing
	// of the config file
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	//
	// init commands and vars
	//
	com_altivec = Cvar_Get ("com_altivec", "1", CVAR_ARCHIVE);
	com_maxfps = Cvar_Get ("com_maxfps", "85", CVAR_ARCHIVE);
	com_lowfps = Cvar_Get( "com_lowfps", "0", 0 );
	com_blood = Cvar_Get ("com_blood", "1", CVAR_ARCHIVE);

	com_developer = Cvar_Get ("developer", "0", CVAR_TEMP );
	com_logfile = Cvar_Get ("logfile", "0", CVAR_TEMP );

	com_timescale = Cvar_Get ("timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO );
	com_fixedtime = Cvar_Get ("fixedtime", "0", CVAR_CHEAT);
	com_showtrace = Cvar_Get ("com_showtrace", "0", CVAR_CHEAT);
	com_dropsim = Cvar_Get ("com_dropsim", "0", CVAR_CHEAT);
	com_viewlog = Cvar_Get( "viewlog", "0", CVAR_CHEAT );
	com_speeds = Cvar_Get ("com_speeds", "0", 0);
	com_timedemo = Cvar_Get ("timedemo", "0", CVAR_CHEAT);
	com_cameraMode = Cvar_Get ("com_cameraMode", "0", CVAR_CHEAT);
	com_heartbeat = Cvar_Get ("com_heartbeat", "1", CVAR_CHEAT);
	
	cl_paused = Cvar_Get ("cl_paused", "0", CVAR_ROM);
	sv_paused = Cvar_Get ("sv_paused", "0", CVAR_ROM);
	com_sv_running = Cvar_Get ("sv_running", "0", CVAR_ROM);
	com_cl_running = Cvar_Get ("cl_running", "0", CVAR_ROM);
	com_buildScript = Cvar_Get( "com_buildScript", "0", 0 );
	com_introPlayed = Cvar_Get( "com_introplayed", "0", CVAR_ARCHIVE);
#ifdef _MSC_VER
	com_renderer = Cvar_Get( "com_renderer", "render-base", CVAR_ARCHIVE );
#else
	com_renderer = Cvar_Get( "com_renderer", "librender-base", CVAR_ARCHIVE );
#endif
#ifdef USE_WEBHOST
	com_webhost				= Cvar_Get( "com_webhost", "http://www.playspacetrader.com", CVAR_INIT | CVAR_ARCHIVE | CVAR_SYSTEMINFO );
#endif
	com_allowDownload		= Cvar_Get( "com_allowDownload", "1", CVAR_INIT );
	com_downloading			= Cvar_Get( "com_downloading", "0", 0 );
#ifdef USE_WEBAUTH
	com_sessionid			= Cvar_Get( "com_sessionid", "", CVAR_INIT );
#endif
#ifndef DEDICATED
#ifdef USE_DRM
	com_keyvalid			= Cvar_Get( "com_keyvalid", "0", CVAR_ROM );
	if ( DRM_Validate() == 4 ) {
		Cvar_Set( "com_keyvalid", "1" );
	}
#endif
#endif


#if defined(_DEBUG)
	com_noErrorInterrupt = Cvar_Get( "com_noErrorInterrupt", "0", 0 );
#endif

	if ( com_dedicated->integer ) {
		if ( !com_viewlog->integer ) {
			Cvar_Set( "viewlog", "1" );
		}
	}

#ifndef _DEBUG
	if( com_developer && com_developer->integer )
	{
#endif
		Cmd_AddCommand( "error", Com_Error_f );
		Cmd_AddCommand( "crash", Com_Crash_f );
		Cmd_AddCommand( "freeze", Com_Freeze_f );
#ifndef _DEBUG
	}
#endif
	Cmd_AddCommand( "quit", Com_Quit_f );
	Cmd_AddCommand( "changeVectors", MSG_ReportChangeVectors_f );
	Cmd_AddCommand( "writeconfig", Com_WriteConfig_f );
#ifdef USE_WEBHOST
	Cmd_AddCommand( "sync", Com_sync_f );
	Cmd_AddCommand( "sync_all", Com_sync_all_f );
	Cmd_AddCommand( "stop_downloads", Com_StopDownloads_f );
#endif
	Cmd_AddCommand( "warmboot", Com_warmboot_f );
	Cmd_AddCommand( "swap_version", Com_CheckGameVersion_f );
#ifndef DEDICATED
#ifdef USE_DRM
	Cmd_AddCommand( "register", Com_Register_f );
	Cmd_AddCommand( "unregister", Com_Unregister_f );
	Cmd_AddCommand( "validate", Com_RegistrationStatus_f );
#endif
#endif

	s = va("%s %s %s", Q3_VERSION, PLATFORM_STRING, __DATE__ );
	com_version = Cvar_Get ("version", s, CVAR_ROM | CVAR_SERVERINFO );

	Sys_Init();
	Netchan_Init( Com_Milliseconds() & 0xffff );	// pick a port value that should be nice and random
	VM_Init();
	SV_Init();
	Net_HTTP_Init();

	com_dedicated->modified = qfalse;
	if ( !com_dedicated->integer ) {
		CL_Init();
		Sys_ShowConsole( com_viewlog->integer, qfalse );
	}

	// set com_frameTime so that if a map is started on the
	// command line it will still be able to count on com_frameTime
	// being random enough for a serverid
	com_frameTime = Com_Milliseconds();

	// add + commands from command line
	if ( !Com_AddStartupCommands() ) {
		// if the user didn't give any commands, run default action
		if ( !com_dedicated->integer ) {
//			Cbuf_AddText ("cinematic idlogo.RoQ\n");
			Cbuf_AddText ("cinematic hermitworks.avi\n");
			if( !com_introPlayed->integer ) {
				Cvar_Set( com_introPlayed->name, "1" );
//				Cvar_Set( "nextmap", "cinematic intro.RoQ" );
			}
		}
	}

	// start in full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	CL_StartHunkUsers();

	com_fullyInitialized = qtrue;

	// always set the cvar, but only print the info if it makes sense.
	Com_DetectAltivec();
	#if idppc
	Com_Printf ("Altivec support is %s\n", com_altivec->integer ? "enabled" : "disabled");
	#endif

	Com_Printf ("--- Common Initialization Complete ---\n");
}

//==================================================================

void Com_WriteConfigToFile( const char *filename ) {
	fileHandle_t	f;

	f = FS_FOpenFileWrite( filename );
	if ( !f ) {
		Com_Printf ("Couldn't write %s.\n", filename );
		return;
	}

	FS_Printf (f, "// generated by spacetrader, do not modify\n");
	Key_WriteBindings (f);
	Cvar_WriteVariables (f);
	FS_FCloseFile( f );
}


/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration( void ) {
	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if ( !com_fullyInitialized ) {
		return;
	}

	if ( !(cvar_modifiedFlags & CVAR_ARCHIVE ) ) {
		return;
	}
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	if ( Cvar_VariableIntegerValue( "fs_noconfig" ) == 0 ) {
		Com_WriteConfigToFile( "st.cfg" );
	}
}


/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Q_EXTERNAL_CALL Com_WriteConfig_f( void )
{
	char	filename[MAX_QPATH];

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "Usage: writeconfig <filename>\n" );
		return;
	}

	Q_strncpyz( filename, Cmd_Argv(1), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
	Com_Printf( "Writing %s.\n", filename );
	Com_WriteConfigToFile( filename );
}

/*
================
Com_ModifyMsec
================
*/
int Com_ModifyMsec( int msec ) {
	int		clampTime;

	//
	// modify time for debugging values
	//
	if ( com_fixedtime->integer ) {
		msec = com_fixedtime->integer;
	} else if ( com_timescale->value ) {
		msec *= com_timescale->value;
	} else if (com_cameraMode->integer) {
		msec *= com_timescale->value;
	}
	
	// don't let it scale below 1 msec
	if ( msec < 1 && com_timescale->value) {
		msec = 1;
	}

	if ( com_dedicated->integer ) {
		// dedicated servers don't want to clamp for a much longer
		// period, because it would mess up all the client's views
		// of time.
		if ( msec > 500 ) {
			Com_Printf( "Hitch warning: %i msec frame time\n", msec );
		}
		clampTime = 5000;
	} else 
	if ( !com_sv_running->integer ) {
		// clients of remote servers do not want to clamp time, because
		// it would skew their view of the server's time temporarily
		clampTime = 5000;
	} else {
		// for local single player gaming
		// we may want to clamp the time to prevent players from
		// flying off edges when something hitches.
		clampTime = 200;
	}

	if ( msec > clampTime ) {
		msec = clampTime;
	}

	return msec;
}

/*
=================
Com_Frame
=================
*/
void Com_Frame( void )
{

	int msec, minMsec;
	static int lastTime;
	int key;
	int timeBeforeFirstEvents;
	int timeBeforeServer;
	int timeBeforeEvents;
	int timeBeforeClient;
	int timeAfter;

	if( setjmp( abortframe ) )
		return;			// an ERR_DROP was thrown

	// bk001204 - init to zero.
	//  also:  might be clobbered by `longjmp' or `vfork'
	timeBeforeFirstEvents =0;
	timeBeforeServer =0;
	timeBeforeEvents =0;
	timeBeforeClient = 0;
	timeAfter = 0;


	// old net chan encryption key
	key = 0x87243987;

	// write config file if anything changed
	Com_WriteConfiguration(); 

	// if "viewlog" has been modified, show or hide the log console
	if ( com_viewlog->modified ) {
		if ( !com_dedicated->value ) {
			Sys_ShowConsole( com_viewlog->integer, qfalse );
		}
		com_viewlog->modified = qfalse;
	}

	//
	// main event loop
	//
	if( com_speeds->integer )
		timeBeforeFirstEvents = Sys_Milliseconds();
	
	// clamp the framerate
	if( !com_dedicated->integer && !com_timedemo->integer )
	{
		int targetFps;

		if( com_maxfps->integer > 0 && com_maxfps->integer <= 1000 )
			targetFps = com_maxfps->integer;
		else
			targetFps = 1000;
		
		if( com_lowfps->integer > 0 && com_lowfps->integer < targetFps )
			targetFps = com_lowfps->integer;

		if( !Sys_IsForeground() )
			targetFps = 24;

		minMsec = 1000 / targetFps;
	}
	else
		minMsec = 1;

	com_frameTime = Com_EventLoop();
	if( lastTime > com_frameTime )
		lastTime = com_frameTime;		// possible on first frame
	msec = com_frameTime - lastTime;

	while( msec < minMsec )
	{
		//give cycles back to the OS
		Sys_Sleep( minMsec - msec );

		com_frameTime = Com_EventLoop();	
		msec = com_frameTime - lastTime;
	}
		
	Cbuf_Execute();

	if (com_altivec->modified)
	{
		Com_DetectAltivec();
		com_altivec->modified = qfalse;
	}

	lastTime = com_frameTime;

	// mess with msec if needed
	com_frameMsec = msec;
	msec = Com_ModifyMsec( msec );

	//
	// server side
	//
	if ( com_speeds->integer ) {
		timeBeforeServer = Sys_Milliseconds ();
	}

	SV_Frame( msec );

	// if "dedicated" has been modified, start up
	// or shut down the client system.
	// Do this after the server may have started,
	// but before the client tries to auto-connect
	if ( com_dedicated->modified ) {
		// get the latched value
		Cvar_Get( "dedicated", "0", 0 );
		com_dedicated->modified = qfalse;
		if ( !com_dedicated->integer ) {
			CL_Init();
			Sys_ShowConsole( com_viewlog->integer, qfalse );
		} else {
			CL_Shutdown();
			Sys_ShowConsole( 1, qtrue );
		}
	}

	//
	// client system
	//
	if ( !com_dedicated->integer ) {
		//
		// run event loop a second time to get server to client packets
		// without a frame of latency
		//
		if ( com_speeds->integer ) {
			timeBeforeEvents = Sys_Milliseconds ();
		}
		Com_EventLoop();
		Cbuf_Execute ();


		//
		// client side
		//
		if ( com_speeds->integer ) {
			timeBeforeClient = Sys_Milliseconds ();
		}

		CL_Frame( msec );

		if ( com_speeds->integer ) {
			timeAfter = Sys_Milliseconds ();
		}
	}

	//
	// report timing information
	//
	if ( com_speeds->integer ) {
		int			all, sv, ev, cl;

		all = timeAfter - timeBeforeServer;
		sv = timeBeforeEvents - timeBeforeServer;
		ev = timeBeforeServer - timeBeforeFirstEvents + timeBeforeClient - timeBeforeEvents;
		cl = timeAfter - timeBeforeClient;
		sv -= time_game;
		cl -= time_frontend + time_backend;

		Com_Printf ("frame:%i all:%3i sv:%3i ev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n", 
					 com_frameNumber, all, sv, ev, cl, time_game, time_frontend, time_backend );
	}	

	//
	// trace optimization tracking
	//
	if ( com_showtrace->integer ) {
	
		extern	int c_traces, c_brush_traces, c_patch_traces;
		extern	int	c_pointcontents;

		Com_Printf ("%4i traces  (%ib %ip) %4i points\n", c_traces,
			c_brush_traces, c_patch_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces = 0;
		c_patch_traces = 0;
		c_pointcontents = 0;
	}

	//Check to make sure we don't have any http data waiting
	// comment this out until I get things going better under win32
	Net_HTTP_Pump();
	
#ifndef DEDICATED
#ifdef USE_IRC
	Net_IRC_Pump();
#endif
#endif

	// old net chan encryption key
	key = lastTime * 0x87243987;

	com_frameNumber++;

	//reset the frame memory stack
	Hunk_FrameReset();
}

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown (void) {

	if (logfile) {
		FS_FCloseFile (logfile);
		logfile = 0;
	}

	if ( com_journalFile ) {
		FS_FCloseFile( com_journalFile );
		com_journalFile = 0;
	}
	
	Net_HTTP_Kill();
#ifndef DEDICATED
#ifdef USE_IRC
	Net_IRC_Shutdown("Com_Shutdown");
#endif
#endif
#ifndef DEDICATED
#ifdef USE_DRM
	DRM_Kill();
#endif
#endif
}

//------------------------------------------------------------------------


/*
===========================================
command line completion
===========================================
*/

/*
==================
Field_Clear
==================
*/
void Field_Clear( field_t *edit ) {
  memset(edit->buffer, 0, MAX_EDIT_LINE);
	edit->cursor = 0;
	edit->scroll = 0;
}

static const char *completionString;
static char shortestMatch[MAX_TOKEN_CHARS];
static int	matchCount;
// field we are working on, passed to Field_AutoComplete(&g_consoleCommand for instance)
static field_t *completionField;

/*
===============
FindMatches

===============
*/
static void FindMatches( const char *s ) {
	int		i;

	if ( Q_stricmpn( s, completionString, strlen( completionString ) ) ) {
		return;
	}
	matchCount++;
	if ( matchCount == 1 ) {
		Q_strncpyz( shortestMatch, s, sizeof( shortestMatch ) );
		return;
	}

	// cut shortestMatch to the amount common with s
	for ( i = 0 ; shortestMatch[i] ; i++ ) {
		if ( i >= strlen( s ) ) {
			shortestMatch[i] = 0;
			break;
		}

		if ( tolower(shortestMatch[i]) != tolower(s[i]) ) {
			shortestMatch[i] = 0;
		}
	}
}

/*
===============
PrintMatches

===============
*/
static void PrintMatches( const char *s ) {
	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) ) {
		Com_Printf( "    %s\n", s );
	}
}

/*
===============
PrintCvarMatches

===============
*/
static void PrintCvarMatches( const char *s ) {
	char value[ TRUNCATE_LENGTH ];

	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) ) {
		Com_TruncateLongString( value, Cvar_VariableString( s ) );
		Com_Printf( "    %s = \"%s\"\n", s, value );
	}
}

/*
===============
Field_FindFirstSeparator
===============
*/
static char *Field_FindFirstSeparator( char *s )
{
	int i;

	for( i = 0; i < strlen( s ); i++ )
	{
		if( s[ i ] == ';' )
			return &s[ i ];
	}

	return NULL;
}

/*
===============
Field_CompleteFilename
===============
*/
static int Field_CompleteFilename( const char *dir,
		const char *ext, qboolean stripExt )
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	FS_FilenameCompletion( dir, ext, stripExt, FindMatches );

	if( matchCount == 0 )
		return 0;

	Q_strcat( completionField->buffer, sizeof( completionField->buffer ),
			shortestMatch + strlen( completionString ) );
	completionField->cursor = strlen( completionField->buffer );

	if( matchCount == 1 )
	{
		Q_strcat( completionField->buffer, sizeof( completionField->buffer ), " " );
		completionField->cursor++;
		return 1;
	}

	Com_Printf( "]%s\n", completionField->buffer );
	
	FS_FilenameCompletion( dir, ext, stripExt, PrintMatches );

	return 0;
}

/*
===============
Field_CompleteCommand
===============
*/
static void Field_CompleteCommand( char *cmd,
		qboolean doCommands, qboolean doCvars )
{
	int		completionArgument = 0;
	char	*p;

	// Skip leading whitespace and quotes
	cmd = Com_SkipCharset( cmd, " \"" );

	Cmd_TokenizeStringIgnoreQuotes( cmd );
	completionArgument = Cmd_Argc( );

	// If there is trailing whitespace on the cmd
	if( *( cmd + strlen( cmd ) - 1 ) == ' ' )
	{
		completionString = "";
		completionArgument++;
	}
	else
		completionString = Cmd_Argv( completionArgument - 1 );

	if( completionArgument > 1 )
	{
		const char *baseCmd = Cmd_Argv( 0 );


//Scott: Why did they ever do this, now tab completion works from the st console.
/* #ifndef DEDICATED
		// If the very first token does not have a leading \ or /,
		// refuse to autocomplete
		if( cmd == completionField->buffer )
		{
			if( baseCmd[ 0 ] != '\\' && baseCmd[ 0 ] != '/' )
				return;

			baseCmd++;
		}
#endif */
		//Scott: if we start with a slash, remove it for the compares below
		if( baseCmd[ 0 ] == '\\' || baseCmd[ 0 ] == '/' ) baseCmd++;
		
		if( ( p = Field_FindFirstSeparator( cmd ) ) != NULL )
		{
			// Compound command
			Field_CompleteCommand( p + 1, qtrue, qtrue );
		}
		else
		{
			// FIXME: all this junk should really be associated with the respective
			// commands, instead of being hard coded here
			if( ( !Q_stricmp( baseCmd, "map" ) ||
						!Q_stricmp( baseCmd, "devmap" ) ||
						!Q_stricmp( baseCmd, "spmap" ) ||
						!Q_stricmp( baseCmd, "spdevmap" ) ) &&
					completionArgument == 2 )
			{
				if ( Field_CompleteFilename( "maps/", ".bsp", qtrue ) != 1 )
					Field_CompleteFilename( "db/", ".mis", qtrue );
			}
			else if( ( !Q_stricmp( baseCmd, "exec" ) ||
						!Q_stricmp( baseCmd, "writeconfig" ) ) &&
					completionArgument == 2 )
			{
				Field_CompleteFilename( "", ".cfg", qfalse );
			}
			else if( !Q_stricmp( baseCmd, "condump" ) &&
					completionArgument == 2 )
			{
				Field_CompleteFilename( "", ".txt", qfalse );
			}
			else if( !Q_stricmp( baseCmd, "demo" ) && completionArgument == 2 )
			{
				Field_CompleteFilename( "demos/", ".dm_"PROTOCOL_VERSION, qtrue );
			}
			else if( ( !Q_stricmp( baseCmd, "toggle" ) ||
						!Q_stricmp( baseCmd, "vstr" ) ||
						!Q_stricmp( baseCmd, "set" ) ||
						!Q_stricmp( baseCmd, "seta" ) ||
						!Q_stricmp( baseCmd, "setu" ) ||
						!Q_stricmp( baseCmd, "sets" ) ) &&
					completionArgument == 2 )
			{
				// Skip "<cmd> "
				p = Com_SkipTokens( cmd, 1, " " );

				if( p > cmd )
					Field_CompleteCommand( p, qfalse, qtrue );
			}
			else if( !Q_stricmp( baseCmd, "rcon" ) && completionArgument == 2 )
			{
				// Skip "rcon "
				p = Com_SkipTokens( cmd, 1, " " );

				if( p > cmd )
					Field_CompleteCommand( p, qtrue, qtrue );
			}
			else if( !Q_stricmp( baseCmd, "bind" ) && completionArgument >= 3 )
			{
				// Skip "bind <key> "
				p = Com_SkipTokens( cmd, 2, " " );

				if( p > cmd )
					Field_CompleteCommand( p, qtrue, qtrue );
			}
		}
	}
	else
	{
		if( completionString[0] == '\\' || completionString[0] == '/' )
			completionString++;

		matchCount = 0;
		shortestMatch[ 0 ] = 0;

		if( strlen( completionString ) == 0 )
			return;

		if( doCommands )
			Cmd_CommandCompletion( FindMatches );

		if( doCvars )
			Cvar_CommandCompletion( FindMatches );

		if( matchCount == 0 )
			return;	// no matches

		if( cmd == completionField->buffer )
		{
#ifndef DEDICATED
			Com_sprintf( completionField->buffer,
					sizeof( completionField->buffer ), "\\%s", shortestMatch );
#else
			Com_sprintf( completionField->buffer,
					sizeof( completionField->buffer ), "%s", shortestMatch );
#endif
		}
		else
		{
			Q_strcat( completionField->buffer, sizeof( completionField->buffer ),
					shortestMatch + strlen( completionString ) );
		}

		completionField->cursor = strlen( completionField->buffer );

		if( matchCount == 1 )
		{
			Q_strcat( completionField->buffer, sizeof( completionField->buffer ), " " );
			completionField->cursor++;
			return;
		}

		Com_Printf( "]%s\n", completionField->buffer );

		// run through again, printing matches
		if( doCommands )
			Cmd_CommandCompletion( PrintMatches );

		if( doCvars )
			Cvar_CommandCompletion( PrintCvarMatches );
	}
}

/*
===============
Field_AutoComplete

Perform Tab expansion
===============
*/
void Field_AutoComplete( field_t *field )
{
	completionField = field;

	Field_CompleteCommand( completionField->buffer, qtrue, qtrue );
}
