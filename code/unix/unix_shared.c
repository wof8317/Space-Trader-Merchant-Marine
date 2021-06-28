/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <fts.h>

#include "../client/client.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

//=============================================================================

// Used to determine CD Path
static char cdPath[MAX_OSPATH];

// Used to determine local installation path
static char installPath[MAX_OSPATH];

// Used to determine where to store user-specific files
static char homePath[MAX_OSPATH];

/*
================
Sys_Milliseconds
================
*/
/* base time in seconds, that's our origin
   timeval:tv_sec is an int:
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
   using unsigned long data type to work right with Sys_XTimeToSysTime */
unsigned long sys_timeBase = 0;
/* current time in ms, using sys_timeBase as origin
   NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
     0x7fffffff ms - ~24 days
   although timeval:tv_usec is an int, I'm not sure wether it is actually used as an unsigned int
     (which would affect the wrap period) */
int curtime;
int Sys_Milliseconds (void)
{
	struct timeval tp;

	gettimeofday(&tp, NULL);

	if (!sys_timeBase)
	{
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	curtime = (tp.tv_sec - sys_timeBase)*1000 + tp.tv_usec/1000;

	return curtime;
}

#if defined(__linux__) && !defined(DEDICATED)
/*
================
Sys_XTimeToSysTime
sub-frame timing of events returned by X
X uses the Time typedef - unsigned long
disable with in_subframe 0

 sys_timeBase*1000 is the number of ms since the Epoch of our origin
 xtime is in ms and uses the Epoch as origin
   Time data type is an unsigned long: 0xffffffff ms - ~49 days period
 I didn't find much info in the XWindow documentation about the wrapping
   we clamp sys_timeBase*1000 to unsigned long, that gives us the current origin for xtime
   the computation will still work if xtime wraps (at ~49 days period since the Epoch) after we set sys_timeBase

================
*/
extern cvar_t *in_subframe;
int Sys_XTimeToSysTime (unsigned long xtime)
{
	int ret, time, test;

	if (!in_subframe->value)
	{
		// if you don't want to do any event times corrections
		return Sys_Milliseconds();
	}

	// test the wrap issue
#if 0
	// reference values for test: sys_timeBase 0x3dc7b5e9 xtime 0x541ea451 (read these from a test run)
	// xtime will wrap in 0xabe15bae ms >~ 0x2c0056 s (33 days from Nov 5 2002 -> 8 Dec)
	//   NOTE: date -d '1970-01-01 UTC 1039384002 seconds' +%c
	// use sys_timeBase 0x3dc7b5e9+0x2c0056 = 0x3df3b63f
	// after around 5s, xtime would have wrapped around
	// we get 7132, the formula handles the wrap safely
	unsigned long xtime_aux,base_aux;
	int test;
//	Com_Printf("sys_timeBase: %p\n", sys_timeBase);
//	Com_Printf("xtime: %p\n", xtime);
	xtime_aux = 500; // 500 ms after wrap
	base_aux = 0x3df3b63f; // the base a few seconds before wrap
	test = xtime_aux - (unsigned long)(base_aux*1000);
	Com_Printf("xtime wrap test: %d\n", test);
#endif

  // some X servers (like suse 8.1's) report weird event times
  // if the game is loading, resolving DNS, etc. we are also getting old events
  // so we only deal with subframe corrections that look 'normal'
  ret = xtime - (unsigned long)(sys_timeBase*1000);
  time = Sys_Milliseconds();
  test = time - ret;
  //printf("delta: %d\n", test);
  if (test < 0 || test > 30) // in normal conditions I've never seen this go above
  {
    return time;
  }

	return ret;
}
#endif

//#if 0 // bk001215 - see snapvector.nasm for replacement
#if !id386 // rcg010206 - using this for PPC builds...
long fastftol( float f ) { // bk001213 - from win32/win_shared.c
  //static int tmp;
  //	__asm fld f
  //__asm fistp tmp
  //__asm mov eax, tmp
  return (long)f;
}

void Sys_SnapVector( float *v ) { // bk001213 - see win32/win_shared.c
  // bk001213 - old linux
  v[0] = rint(v[0]);
  v[1] = rint(v[1]);
  v[2] = rint(v[2]);
}
#endif


void	Sys_Mkdir( const char *path )
{
    mkdir (path, 0777);
}

char *strlwr (char *s) {
  if ( s==NULL ) { // bk001204 - paranoia
    assert(0);
    return s;
  }
  while (*s) {
    *s = tolower(*s);
    s++;
  }
  return s; // bk001204 - duh
}

//============================================

extern sqlInfo_t com_db;
extern void FS_LoadZipFile( const char * filename, const char * basename, const char * ext );

void Sys_AddFiles( const char * base, const char * path, const char * ext_search ) {

	char				os_path[ MAX_OSPATH ];
	char				*argv[2] = {os_path, NULL};
	FTS					*findinfo;
	const int			baselen = strlen(base);
	
	Com_sprintf( os_path, sizeof(os_path), "%s%s", base, path );

	if (os_path[strlen(os_path)-1] == '/') 
		os_path[strlen(os_path)-1] = NULL;

	if ( (findinfo = fts_open( argv, FTS_PHYSICAL, NULL)) )
	{
		FTSENT *entry;
		while ( (entry = fts_read(findinfo)) ) {
			if (entry->fts_info & FTS_F) {
				if ( entry->fts_errno == 0 ) {
					char ext[ MAX_QPATH ];
					char name[ MAX_QPATH ];
					char dir[ MAX_OSPATH ];
					char pathname[ MAX_OSPATH ];
					
					Q_strncpyz(pathname, entry->fts_path, sizeof(pathname));
					Com_Memset(dir, 0, sizeof(dir));
					Com_Memset(name, 0, sizeof(name));
					Com_Memset(ext, 0, sizeof(ext));
					Sys_SplitPath( pathname, dir, sizeof(dir), name, sizeof(name), ext, sizeof(ext) );
		
					//sanity check
					ASSERT( strlen(dir) >= baselen );

					if ( !ext_search || !Q_stricmp( ext, ext_search ) ) {
					
						sql_bindtext( &com_db, 2, dir+baselen );
						sql_bindtext( &com_db, 3, name );
						sql_bindtext( &com_db, 4, ext );
						sql_bindint	( &com_db, 5, entry->fts_statp->st_size );
						sql_bindint	( &com_db, 6, 0 );
						sql_bindint	( &com_db, 7, -1 );
			
						sql_step	( &com_db );
					}
				}
			} else if ( entry->fts_info & FTS_D && entry->fts_name[0] == '.' ) {
				fts_set(findinfo, entry, FTS_SKIP);
			}
		}
	}
}


#define	MAX_FOUND_FILES	0x1000

// bk001129 - new in 1.26
void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles ) {
	char		search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char		filename[MAX_OSPATH];
	DIR			*fdir;
	struct dirent *d;
	struct stat st;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if (strlen(subdirs)) {
		Com_sprintf( search, sizeof(search), "%s/%s", basedir, subdirs );
	}
	else {
		Com_sprintf( search, sizeof(search), "%s", basedir );
	}

	if ((fdir = opendir(search)) == NULL) {
		return;
	}

	while ((d = readdir(fdir)) != NULL) {
		Com_sprintf(filename, sizeof(filename), "%s/%s", search, d->d_name);
		if (stat(filename, &st) == -1)
			continue;

		if (S_ISDIR(st.st_mode)) {
			if (Q_stricmp(d->d_name, ".") && Q_stricmp(d->d_name, "..")) {
				if (strlen(subdirs)) {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s/%s", subdirs, d->d_name);
				}
				else {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s", d->d_name);
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		Com_sprintf( filename, sizeof(filename), "%s/%s", subdirs, d->d_name );
		if (!Com_FilterPath( filter, filename, qfalse ))
			continue;
		list[ *numfiles ] = CopyString( filename );
		(*numfiles)++;
	}

	closedir(fdir);
}

// bk001129 - in 1.17 this used to be
// char **Sys_ListFiles( const char *directory, const char *extension, int *numfiles, qboolean wantsubs )
char **Sys_ListFiles( const char *directory, const char *extension, int *numfiles, qboolean wantsubs )
{
	struct dirent *d;
	// char *p; // bk001204 - unused
	DIR		*fdir;
	qboolean dironly = wantsubs;
	char		search[MAX_OSPATH];
	int			nfiles;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	//int			flag; // bk001204 - unused
	int			i;
	struct stat st;

	int			extLen;

	if ( !extension)
		extension = "";

	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		dironly = qtrue;
	}

	extLen = strlen( extension );

	// search
	nfiles = 0;

	if ((fdir = opendir(directory)) == NULL) {
		*numfiles = 0;
		return NULL;
	}

	while ((d = readdir(fdir)) != NULL) {
		Com_sprintf(search, sizeof(search), "%s/%s", directory, d->d_name);
		if (stat(search, &st) == -1)
			continue;
		if ((dironly && !(S_ISDIR(st.st_mode))) ||
			(!dironly && (S_ISDIR(st.st_mode))))
			continue;

		if (*extension) {
			if ( strlen( d->d_name ) < strlen( extension ) ||
				Q_stricmp(
					d->d_name + strlen( d->d_name ) - strlen( extension ),
					extension ) ) {
				continue; // didn't match
			}
		}

		if ( nfiles == MAX_FOUND_FILES - 1 )
			break;
		list[ nfiles ] = CopyString( d->d_name );
		nfiles++;
	}

	list[ nfiles ] = NULL;

	closedir(fdir);

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

void	Sys_FreeFileList( char **list ) {
	int		i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		Z_Free( list[i] );
	}

	Z_Free( list );
}

char *Sys_Cwd( char *path, int size )
{
	if ( getcwd( path, size ) == NULL) {
		Com_Error( ERR_FATAL, "couldn't find working directory.\n" );
	}

	return path;
}

void Sys_SetDefaultCDPath(const char *path)
{
	Q_strncpyz(cdPath, path, sizeof(cdPath));
}

char *Sys_DefaultCDPath(char *path, int size)
{
	Q_strncpyz(path, cdPath, size);
	return cdPath;
}

void Sys_SetDefaultInstallPath(const char *path)
{
	Q_strncpyz(installPath, path, sizeof(installPath));
}

char *Sys_DefaultInstallPath( char * path, int size )
{
	if (*installPath)
		return installPath;
	else
		return Sys_Cwd(path, size);
}

void Sys_SetDefaultHomePath(const char *path)
{
	Q_strncpyz(homePath, path, sizeof(homePath));
}

char *	Sys_DefaultHomePath( char * buffer, int size )
{
	char *p;

	if ( *homePath ) {
		Q_strncpyz(buffer, homePath, size);
		return buffer;
	}

	if ( ( p = getenv( "HOME" ) ) != NULL )
	{
		Q_strncpyz( buffer, p, size );
#ifdef MACOS_X
		Q_strcat( buffer, size, "/Library/Application Support/SpaceTrader" );
#else
		Q_strcat( buffer, size, "/." );
#endif
		Q_strcat( buffer, size, BASEGAME );
		if ( mkdir( buffer, 0777 ) )
		{
			if ( errno != EEXIST )
				Sys_Error( "Unable to create directory \"%s\", error is %s(%d)\n", buffer, strerror( errno ), errno );
		}
	}
	return buffer;
}
#ifdef __linux__
char* getexename ( char* buf, size_t size )
{
	char linkname[MAX_OSPATH]; /* /proc/<pid>/exe */
	pid_t pid;
	int ret;

	/* Get our PID and build the name of the link in /proc */
	pid = getpid();

	if ( snprintf ( linkname, sizeof ( linkname ), "/proc/%i/exe", pid ) < 0 )
	{
		/* This should only happen on large word systems. I'm not sure
		what the proper response is here.
		Since it really is an assert-like condition, aborting the
		program seems to be in order. */
		abort();
	}


	/* Now read the symbolic link */
	ret = readlink ( linkname, buf, size );

	/* In case of an error, leave the handling up to the caller */
	if ( ret == -1 )
		return NULL;

	/* Report insufficient buffer size */
	if ( ret >= size )
	{
		errno = ERANGE;
		return NULL;
	}

	/* Ensure proper NUL termination */
	buf[ret] = 0;

	return buf;
}


char * Sys_SecretSauce ( char * buffer, int size )
{
	char exename[MAX_OSPATH];
	char entrophy[1024];
	entrophy[0] = 0;
	if ( getexename ( exename, sizeof ( exename ) ) )
	{
		struct stat fileinfo;
		if ( stat ( exename, &fileinfo ) == 0 )
		{
			char minor, major;
			FILE *part;
			minor = fileinfo.st_dev & 255;
			major = ( fileinfo.st_dev >> 8 ) & 255;
			if ( part = fopen ( "/proc/partitions", "r" ) )
			{
				char buf[1024], cmp[11];
				Com_sprintf ( cmp, sizeof ( cmp ), "%4d  %4d", ( int ) major, ( int ) minor );
				while ( fgets ( buf, sizeof ( buf ), part ) )
				{
					if ( !Q_stricmpn ( cmp, buf, 10 ) )
					{
						Q_strncpyz ( entrophy, buf+10, sizeof ( entrophy ) );
						break;
					}
				}
				fclose ( part );
			}
		}

		gethostname ( entrophy+strlen ( entrophy ), sizeof ( entrophy ) - strlen ( entrophy ) );
		Com_MD5Buffer ( buffer, entrophy, size );
		return buffer;

	}
	return NULL;
}

#endif
//============================================

int Sys_GetProcessorId( void )
{
	return CPUID_GENERIC;
}

void Sys_ShowConsole( int visLevel, qboolean quitOnClose )
{
}

char *Sys_GetCurrentUser( char *buffer, int size )
{
	struct passwd *p;

	if ( (p = getpwuid( getuid() )) == NULL ) {
		Q_strncpyz(buffer, "player", size);
	} else {
		Q_strncpyz(buffer, p->pw_name, size);
	}
	return buffer;
}

#if defined(__linux__) || defined(__FreeBSD__)
// TTimo
// sysconf() in libc, POSIX.1 compliant
unsigned int Sys_ProcessorCount(void)
{
  return sysconf(_SC_NPROCESSORS_ONLN);
}
#endif
