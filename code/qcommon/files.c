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
/*****************************************************************************
 * name:		files.c
 *
 * desc:		handle based filesystem for Quake III Arena 
 *
 * $Archive: /MissionPack/code/qcommon/files.c $
 *
 *****************************************************************************/


#include "q_shared.h"
#include "qcommon.h"
#include "unzip.h"

#include "../sql/sql.h"

extern sqlInfo_t com_db;

/*
=============================================================================

QUAKE3 FILESYSTEM

All of Quake's data access is through a hierarchical file system, but the contents of 
the file system can be transparently merged from several sources.

A "qpath" is a reference to game file data.  MAX_ZPATH is 256 characters, which must include
a terminating zero. "..", "\\", and ":" are explicitly illegal in qpaths to prevent any
references outside the quake directory system.

The "base path" is the path to the directory holding all the game directories and usually
the executable.  It defaults to ".", but can be overridden with a "+set fs_basepath c:\quake3"
command line to allow code debugging in a different directory.  Basepath cannot
be modified at all after startup.  Any files that are created (demos, screenshots,
etc) will be created reletive to the base path, so base path should usually be writable.

The "cd path" is the path to an alternate hierarchy that will be searched if a file
is not located in the base path.  A user can do a partial install that copies some
data to a base path created on their hard drive and leave the rest on the cd.  Files
are never writen to the cd path.  It defaults to a value set by the installer, like
"e:\quake3", but it can be overridden with "+set fs_cdpath g:\quake3".

If a user runs the game directly from a CD, the base path would be on the CD.  This
should still function correctly, but all file writes will fail (harmlessly).

The "home path" is the path used for all write access. On win32 systems we have "base path"
== "home path", but on *nix systems the base installation is usually readonly, and
"home path" points to ~/.q3a or similar

The user can also install custom mods and content in "home path", so it should be searched
along with "home path" and "cd path" for game content.


The "base game" is the directory under the paths where data comes from by default, and
can be either "baseq3" or "demoq3".

The "current game" may be the same as the base game, or it may be the name of another
directory under the paths that should be searched for files before looking in the base game.
This is the basis for addons.

Clients automatically set the game directory after receiving a gamestate from a server,
so only servers need to worry about +set fs_game.

No other directories outside of the base game and current game will ever be referenced by
filesystem functions.

To save disk space and speed loading, directory trees can be collapsed into zip files.
The files use a ".pk3" extension to prevent users from unzipping them accidentally, but
otherwise the are simply normal uncompressed zip files.  A game directory can have multiple
zip files of the form "pak0.pk3", "pak1.pk3", etc.  Zip files are searched in decending order
from the highest number to the lowest, and will always take precedence over the filesystem.
This allows a pk3 distributed as a patch to override all existing data.

Because we will have updated executables freely available online, there is no point to
trying to RESTRICT demo / oem versions of the game with code changes.  Demo / oem versions
should be exactly the same executables as release versions, but with different data that
automatically restricts where game media can come from to prevent add-ons from working.

After the paths are initialized, quake will look for the product.txt file.  If not
found and verified, the game will run in restricted mode.  In restricted mode, only 
files contained in demoq3/pak0.pk3 will be available for loading, and only if the zip header is
verified to not have been modified.  A single exception is made for q3config.cfg.  Files
can still be written out in restricted mode, so screenshots and demos are allowed.
Restricted mode can be tested by setting "+set fs_restrict 1" on the command line, even
if there is a valid product.txt under the basepath or cdpath.

If not running in restricted mode, and a file is not found in any local filesystem,
an attempt will be made to download it and save it under the base path.

If the "fs_copyfiles" cvar is set to 1, then every time a file is sourced from the cd
path, it will be copied over to the base path.  This is a development aid to help build
test releases and to copy working sets over slow network links.

File search order: when FS_FOpenFileRead gets called it will go through the fs_searchpaths
structure and stop on the first successful hit. fs_searchpaths is built with successive
calls to FS_AddGameDirectory

Additionaly, we search in several subdirectories:
current game is the current mode
base game is a variable to allow mods based on other mods
(such as baseq3 + missionpack content combination in a mod for instance)
BASEGAME is the hardcoded base game ("baseq3")

e.g. the qpath "sound/newstuff/test.ogg" would be searched for in the following places:

home path + current game's zip files
home path + current game's directory
base path + current game's zip files
base path + current game's directory
cd path + current game's zip files
cd path + current game's directory

home path + base game's zip file
home path + base game's directory
base path + base game's zip file
base path + base game's directory
cd path + base game's zip file
cd path + base game's directory

home path + BASEGAME's zip file
home path + BASEGAME's directory
base path + BASEGAME's zip file
base path + BASEGAME's directory
cd path + BASEGAME's zip file
cd path + BASEGAME's directory

server download, to be written to home path + current game's directory


The filesystem can be safely shutdown and reinitialized with different
basedir / cddir / game combinations, but all other subsystems that rely on it
(sound, video) must also be forced to restart.

Because the same files are loaded by both the clip model (CM_) and renderer (TR_)
subsystems, a simple single-file caching scheme is used.  The CM_ subsystems will
load the file with a request to cache.  Only one file will be kept cached at a time,
so any models that are going to be referenced by both subsystems should alternate
between the CM_ load function and the ref load function.

TODO: A qpath that starts with a leading slash will always refer to the base game, even if another
game is currently active.  This allows character models, skins, and sounds to be downloaded
to a common directory no matter which game is active.

How to prevent downloading zip files?
Pass pk3 file names in systeminfo, and download before FS_Restart()?

Aborting a download disconnects the client from the server.

How to mark files as downloadable?  Commercial add-ons won't be downloadable.

Non-commercial downloads will want to download the entire zip file.
the game would have to be reset to actually read the zip in

Auto-update information

Path separators

Casing

  separate server gamedir and client gamedir, so if the user starts
  a local game after having connected to a network game, it won't stick
  with the network game.

  allow menu options for game selection?

Read / write config to floppy option.

Different version coexistance?

When building a pak file, make sure a q3config.cfg isn't present in it,
or configs will never get loaded from disk!

  todo:

  downloading (outside fs?)
  game directory passing and restarting

=============================================================================

*/


#define	DEMOGAME			"demo"

// every time a new demo pk3 file is built, this checksum must be updated.
// the easiest way to get it is to just run the game and see what it spits out
#define	DEMO_PAK_CHECKSUM	1656324795u

// if this is defined, the executable positively won't work with any paks other
// than the demo pak, even if productid is present.  This is only used for our
// last demo release to prevent the mac and linux users from using the demo
// executable with the production windows pak before the mac/linux products
// hit the shelves a little later
// NOW defined in build files
//#define PRE_RELEASE_DEMO

#define MAX_ZPATH			256


static	char		fs_gamedir[MAX_OSPATH];	// this will be a single file name with no separators
static	cvar_t		*fs_debug;
static	cvar_t		*fs_homepath;
static	cvar_t		*fs_basepath;
static	cvar_t		*fs_basegame;
static	cvar_t		*fs_cdpath;
static	cvar_t		*fs_gamedirvar;
static	cvar_t		*fs_restart;
static	cvar_t		*fs_restrict;
static	int			fs_initialized;
static	int			fs_readCount;			// total bytes read
static	int			fs_loadCount;			// total files read
static	int			fs_loadStack;			// total files in memory
static	int			fs_packFiles;			// total number of files in packs

static int fs_checksumFeed;

typedef union qfile_gus {
	FILE*		o;
	unzFile		z;
} qfile_gut;

typedef struct qfile_us {
	qfile_gut	file;
	qboolean	unique;
} qfile_ut;

typedef struct {
	int			id;
	qfile_ut	handleFiles;
	int			zipFilePos;
	qboolean	zipFile;
} fileHandleData_t;

static fileHandleData_t	fsh[MAX_FILE_HANDLES];

void FS_AddZipFile( const char * filename, int length, qboolean mount );

// productId: This file is copyright 2007 HermitWorks Entertainment Corporation, and may not be duplicated except during a licensed installation of the full commercial version of Space Trader
static byte fs_scrambledProductId[177] = {
220, 129, 255, 108, 244, 163, 171, 55, 133, 65, 199, 36, 140, 222, 53, 99, 65, 171, 175, 232, 236, 193, 210, 249, 160, 97, 233, 231, 20, 200, 248, 238, 129, 189, 161, 144, 70, 206, 81, 27, 5, 47, 122, 82, 126, 233, 219, 154, 246, 212, 67, 1, 144, 181, 17, 196, 130, 65, 81, 213, 221, 249, 131, 12, 38, 133, 118, 190, 250, 225, 162, 118, 193, 88, 78, 121, 3, 9, 58, 177, 157, 185, 226, 58, 52, 25, 219, 232, 49, 101, 251, 227, 60, 8, 50, 32, 205, 249, 194, 159, 144, 16, 144, 146, 110, 102, 238, 150, 236, 49, 19, 208, 61, 23, 149, 74, 192, 117, 123, 5, 195, 133, 159, 11, 16, 44, 222, 74, 103, 7, 54, 240, 50, 101, 54, 179, 5, 193, 72, 162, 64, 81, 250, 240, 215, 52, 43, 106, 118, 86, 27, 42, 124, 241, 40, 34, 174, 94, 99, 108, 6, 105, 25, 25, 148, 118, 210, 218, 163, 164, 174, 227, 254, 124, 94, 22, 106, 
};

/*
==============
FS_Initialized
==============
*/

qboolean FS_Initialized( void ) {
	return fs_initialized;
}

/*
=================
FS_LoadStack
return load stack
=================
*/
int FS_LoadStack( void )
{
	return fs_loadStack;
}
                      
static fileHandle_t	FS_HandleForFile(void) {
	int		i;

	for ( i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		if ( fsh[i].handleFiles.file.o == NULL ) {
			return i;
		}
	}
	Com_Error( ERR_DROP, "FS_HandleForFile: none free" );
	return 0;
}

static FILE	*FS_FileForHandle( fileHandle_t f ) {
	if ( f < 0 || f > MAX_FILE_HANDLES ) {
		Com_Error( ERR_DROP, "FS_FileForHandle: out of reange" );
	}
	if (fsh[f].zipFile == qtrue) {
		Com_Error( ERR_DROP, "FS_FileForHandle: can't get FILE on zip file" );
	}
	if ( ! fsh[f].handleFiles.file.o ) {
		Com_Error( ERR_DROP, "FS_FileForHandle: NULL" );
	}
	
	return fsh[f].handleFiles.file.o;
}

void	FS_ForceFlush( fileHandle_t f ) {
	FILE *file;

	file = FS_FileForHandle(f);
	setvbuf( file, NULL, _IONBF, 0 );
}

/*
====================
FS_ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
static char * FS_ReplaceSeparators( char *path ) {
	char	*s;

	for ( s = path ; *s ; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			*s = PATH_SEP;
		}
	}

	return path;
}

/*
===================
FS_BuildOSPath

Qpath may have either forward or backwards slashes
===================
*/
void FS_BuildOSPath( char * ospath, int size, const char *base, const char *game, const char *qpath ) {

	if( !game || !game[0] ) {
		game = fs_gamedir;
	}

	Com_sprintf( ospath, size, "%s/%s/%s", base, game, qpath );
	FS_ReplaceSeparators( ospath );	
}

/*
=====================
FS_BuildOSHomePath

  * return a path to a file in the users homepath
=====================
*/
void FS_BuildOSHomePath( char * ospath, int size, const char *qpath ) {
	Com_sprintf( ospath, size, "%s/%s/%s", fs_homepath->string, fs_gamedir, qpath );
	FS_ReplaceSeparators( ospath );	
}

/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
static qboolean FS_CreatePath (char *OSPath) {
	char	*ofs;
	
	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) ) {
		Com_Printf( "WARNING: refusing to create relative path \"%s\"\n", OSPath );
		return qtrue;
	}

	for (ofs = OSPath+1 ; *ofs ; ofs++) {
		if (*ofs == PATH_SEP) {	
			// create the directory
			*ofs = 0;
			Sys_Mkdir (OSPath);
			*ofs = PATH_SEP;
		}
	}
	return qfalse;
}


/*
================
FS_FileExists

Tests if the file exists in the current gamedir, this DOES NOT
search the paths.  This is to determine if opening a file to write
(which always goes into the current gamedir) will cause any overwrites.
NOTE TTimo: this goes with FS_FOpenFileWrite for opening the file afterwards
================
*/
qboolean Q_EXTERNAL_CALL FS_FileExists( const char *file )
{
	FILE *f;
	char testpath[ MAX_OSPATH ];

	FS_BuildOSPath( testpath, sizeof(testpath), fs_homepath->string, fs_gamedir, file );

	f = fopen( testpath, "rb" );
	if (f) {
		fclose( f );
		return qtrue;
	}
	return qfalse;
}

static void fs_db_open( fileHandle_t f, const char * fullname, const char * filename, int resume_from ) {

	int path_id = -1;

	//	attempt to find the path id
	sql_prepare	( &com_db, "SELECT id FROM paths WHERE (path||$2) like ($1);" );
	sql_bindtext( &com_db, 1, fullname );
	sql_bindtext( &com_db, 2, filename );

	if ( sql_step( &com_db ) ) {
		path_id = sql_columnasint( &com_db, 0 );
	}

	sql_done	( &com_db );


	sql_prepare	( &com_db, "INSERT INTO openfiles(f,path,time,xfer,name) VALUES(?,?,SYS_TIME,?,$);" );
	sql_bindint	( &com_db, 1, f );
	sql_bindint	( &com_db, 2, path_id );
	sql_bindint	( &com_db, 3, resume_from );
	sql_bindtext( &com_db, 4, filename );
	sql_step	( &com_db );
	sql_done	( &com_db );
}

static void fs_db_close( fileHandle_t f ) {

	sql_prepare	( &com_db, "SELECT name, path FROM openfiles SEARCH f ?1 WHERE pak==0;" );
	sql_bindint	( &com_db, 1, f );

	//	check to see if the file being closed was being written to and is not a pak file
	if ( sql_step( &com_db ) ) {

		const char * fullname = sql_columnastext( &com_db, 0 );
		char path		[ MAX_QPATH ];
		char name		[ MAX_QPATH ];
		char ext		[ 16 ];
		int	length;
		int	path_id = sql_columnasint( &com_db, 1 );

		Sys_SplitPath( fullname, path, sizeof(path), name, sizeof(name), ext, sizeof(ext) );

		fseek( fsh[f].handleFiles.file.o, 0, SEEK_END );
		length = ftell( fsh[f].handleFiles.file.o );

		//	update the fat to reflect new file
		sql_prepare	( &com_db, "UPDATE OR INSERT files SET id=#+1, path_id=?5, path=$1,name=$2,ext=$3, length=?4, pak_id=-1, fullname=path||name||ext SEARCH name $2 WHERE path like $1 AND ext like $3;" );
		sql_bindtext( &com_db, 1, path );
		sql_bindtext( &com_db, 2, name );
		sql_bindtext( &com_db, 3, ext );
		sql_bindint	( &com_db, 4, length );
		sql_bindint	( &com_db, 5, path_id );
		sql_step	( &com_db );
		sql_done	( &com_db );

		sql_prepare	( &com_db, "DELETE FROM openfiles WHERE f = ?1;" );
		sql_bindint	( &com_db, 1, f );
		sql_step	( &com_db );
		sql_done	( &com_db );
	}

	sql_done( &com_db );
}

/*
==============
FS_FCloseFile

If the FILE pointer is an open pak file, leave it open.

For some reason, other dll's can't just cal fclose()
on files returned by FS_FOpenFile...
==============
*/
void Q_EXTERNAL_CALL FS_FCloseFile( fileHandle_t f )
{
	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if (fsh[f].zipFile == qtrue) {
		unzCloseCurrentFile( fsh[f].handleFiles.file.z );
		if ( fsh[f].handleFiles.unique ) {
			unzClose( fsh[f].handleFiles.file.z );
		}
		Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
		return;
	}

	// we didn't find it as a pak, so close it as a unique file
	if (fsh[f].handleFiles.file.o) {

		fs_db_close( f );

		fclose (fsh[f].handleFiles.file.o);

		//	check if this was a temporary file
		sql_prepare	( &com_db, "SELECT name,xfer,length,pak FROM openfiles SEARCH f ?1 WHERE pak=1;" );
		sql_bindint	( &com_db, 1, f );
		if ( sql_step( &com_db ) ) {
			char source[ MAX_OSPATH ];
			char target[ MAX_OSPATH ];
			int	xfer	= sql_columnasint( &com_db, 1 );
			int	length	= sql_columnasint( &com_db, 2 );

			FS_BuildOSPath( source, sizeof(source), fs_basepath->string, fs_gamedir, sql_columnastext( &com_db, 0 ) );

			if ( xfer > 0 ) {

				//	if transfer was completed
				if ( xfer == length ) {

					char *t;
					Q_strncpyz( target, source, sizeof(target) );
					t = strstr( target, "~" );
					if ( t ) {
						*t = 0;
					}

					remove( target );
				
					if ( rename( source, target ) != 0 ) {
						Com_Error( ERR_FATAL, "could not update pak file %s. File may be in use.\n", target );
					}

					fs_restart->integer = 1;

					
					//  auto load paks.
					if ( sql_columnasint( &com_db, 3 ) == 1 ) {
						FS_AddZipFile( target, length, qtrue );
					}
				}
			} else {
				//	transfer did nothing, remove temp file
				remove( source );
			}

			sql_prepare	( &com_db, "DELETE FROM openfiles WHERE f = ?1;" );
			sql_bindint	( &com_db, 1, f );
			sql_step	( &com_db );
			sql_done	( &com_db );
		}
		sql_done	( &com_db );
	}
	Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
}

/*
===========
FS_FOpenFileWrite

===========
*/
fileHandle_t Q_EXTERNAL_CALL FS_FOpenFileWrite( const char *filename )
{
	char			ospath[ MAX_OSPATH ];
	fileHandle_t	f;

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile	= qfalse;

	if ( filename[ 0 ] == '~' && filename[ 1 ] == '/' ) {
		filename += 2;
		FS_BuildOSPath( ospath, sizeof(ospath), fs_basepath->string, fs_gamedir, filename );
	} else {
		FS_BuildOSPath( ospath, sizeof(ospath), fs_homepath->string, fs_gamedir, filename );
	}

	fs_db_open( f, ospath, filename, 0 );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileWrite: %s\n", ospath );
	}

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[f].handleFiles.file.o = fopen( ospath, "wb" );

	if (!fsh[f].handleFiles.file.o) {
		return 0;
	}

	return f;
}

/*
===========
FS_FOpenFileWrite

===========
*/
int Q_EXTERNAL_CALL FS_FOpenFileDirect( const char *filename, fileHandle_t * f )
{
	int r;
	char			ospath[ MAX_OSPATH ];

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	*f = FS_HandleForFile();
	fsh[*f].zipFile	= qfalse;

	FS_BuildOSPath( ospath, sizeof(ospath), fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileDirect: %s\n", ospath );
	}

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[*f].handleFiles.file.o = fopen( ospath, "rb" );

	if (!fsh[*f].handleFiles.file.o) {
		*f = 0;
		return 0;
	}

	fseek( fsh[*f].handleFiles.file.o, 0, SEEK_END );
	r = ftell( fsh[*f].handleFiles.file.o );
	fseek( fsh[*f].handleFiles.file.o, 0, SEEK_SET );

	return r;
}

/*
===========
FS_FOpenFileAppend

===========
*/
int FS_FOpenFileAppend( const char *filename, fileHandle_t *f ) {
	char	ospath[ MAX_OSPATH ];
	int		r;

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	*f = FS_HandleForFile();
	fsh[*f].zipFile = qfalse;

	// don't let sound stutter
	S_ClearSoundBuffer();

	if ( filename[ 0 ] == '~' && filename[ 1 ] == '/' ) {
		filename += 2;
		FS_BuildOSPath( ospath, sizeof(ospath), fs_basepath->string, fs_gamedir, filename );
	} else {
		FS_BuildOSPath( ospath, sizeof(ospath), fs_homepath->string, fs_gamedir, filename );
	}

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileAppend: %s\n", ospath );
	}

	if( FS_CreatePath( ospath ) ) {
		*f = 0;
		return 0;
	}

	fsh[*f].handleFiles.file.o = fopen( ospath, "ab" );
	if (!fsh[*f].handleFiles.file.o) {
		*f = 0;
		return 0;
	}

	fseek( fsh[*f].handleFiles.file.o, 0, SEEK_END );
	r = ftell( fsh[*f].handleFiles.file.o );

	fs_db_open( *f, ospath, filename, r );

	return r;
}

#ifdef DEVELOPER
/*
===========
FS_FOpenFileUpdate

===========
*/
fileHandle_t FS_FOpenFileUpdate( const char *filename, int * length ) {
	char			ospath[ MAX_OSPATH ];
	fileHandle_t	f;

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	FS_BuildOSPath( ospath, sizeof(ospath), fs_basepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileWrite: %s\n", ospath );
	}

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

	fs_db_open( f, ospath, filename, 0 );

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[f].handleFiles.file.o = fopen( ospath, "wb" );

	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
	return f;
}

#endif

/*
===========
FS_FOpenFileRead

Finds the file in the search path.
Returns filesize and an open FILE pointer.
Used for streaming data out of either a
separate file or a ZIP file.
===========
*/
extern qboolean		com_fullyInitialized;

int FS_FOpenFileRead( const char *filename, fileHandle_t *file, qboolean uniqueFILE ) {

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !filename ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'filename' parameter passed\n" );
	}

	// qpaths are not supposed to have a leading slash
	if ( filename[0] == '/' || filename[0] == '\\' ) {
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo" 
	if ( strstr( filename, ".." ) || strstr( filename, "::" ) ) {
		*file = 0;
		return -1;
	}

	// make sure the stkey file is only readable by the quake3.exe at initialization
	// any other time the key should only be accessed in memory using the provided functions
	if( com_fullyInitialized && strstr( filename, "stkey" ) ) {
		*file = 0;
		return -1;
	}

	//
	// search through the path, one element at a time
	//
	sql_prepare	( &com_db, "SELECT paths.id[ path_id ]^path || '/' || fullname, length, offset, pak_id, id FROM files SEARCH fullname $1;" );
	sql_bindtext( &com_db, 1, filename );

	if ( sql_step( &com_db ) ) {
		const char *	file_path	= sql_columnastext	( &com_db, 0 );
		int				file_length	= sql_columnasint	( &com_db, 1 );
		int				file_offset = sql_columnasint	( &com_db, 2 );
		int				file_pakid	= sql_columnasint	( &com_db, 3 );
		int				file_id		= sql_columnasint	( &com_db, 4 );

		sql_done( &com_db );

		if ( !file ) {
			return file_length;
		}

		*file = FS_HandleForFile();

		fsh[*file].id = file_id;
		fsh[*file].handleFiles.unique = uniqueFILE;

		if ( file_pakid >= 1 ) {

			const char *	pakFilename;
			unzFile			pakHandle;
			unz_s			*zfi;
			FILE			*temp;

			sql_prepare( &com_db, "SELECT handle, name FROM pakfiles SEARCH id ?1;" );
			sql_bindint( &com_db, 1, file_pakid );
			
			if ( !sql_step( &com_db ) ) {
				Com_Error( ERR_FATAL, "can't find pak file" );
			}

			pakHandle	= (unzFile)sql_columnasint	( &com_db, 0 );
			pakFilename	= sql_columnastext	( &com_db, 1 );

			sql_done( &com_db );

			if ( uniqueFILE ) {
				// open a new file on the pakfile
				fsh[*file].handleFiles.file.z = unzReOpen (pakFilename, pakHandle);
				if (fsh[*file].handleFiles.file.z == NULL) {
					Com_Error (ERR_FATAL, "Couldn't reopen %s", pakFilename);
				}

			} else {
				fsh[*file].handleFiles.file.z = pakHandle;
			}
			fsh[*file].zipFile = qtrue;
			zfi = (unz_s *)fsh[*file].handleFiles.file.z;
			// in case the file was new
			temp = zfi->file;
			// set the file position in the zip file (also sets the current file info)
			unzSetCurrentFileInfoPosition(pakHandle, file_offset);
			// copy the file info into the unzip structure
			Com_Memcpy( zfi, pakHandle, sizeof(unz_s) );
			// we copy this back into the structure
			zfi->file = temp;
			// open the file in the zip
			unzOpenCurrentFile( fsh[*file].handleFiles.file.z );
			fsh[*file].zipFilePos = file_offset;

			if ( fs_debug->integer ) {
				Com_Printf( "FS_FOpenFileRead: %s (found in '%s')\n", 
					filename, pakFilename );
			}


		} else {
			fsh[*file].handleFiles.file.o = fopen (file_path, "rb");
			if ( !fsh[*file].handleFiles.file.o ) {
				*file = 0;
				return -1;
			}
			fsh[*file].zipFile = qfalse;
		}

#ifdef DEVELOPER
		sql_prepare	( &com_db, "INSERT INTO access(id,time,path) VALUES(?,SYS_TIME,$);" );
		sql_bindint	( &com_db, 1, file_id );
		sql_bindtext( &com_db, 2, filename ); 
		sql_step	( &com_db );
		sql_done	( &com_db );
#endif

		return file_length;
	}

	sql_done( &com_db );

	if ( !file )
		return 0;
	
	*file = 0;
	return -1;
}

int Q_EXTERNAL_CALL FS_Read( void *buffer, int len, fileHandle_t f ) {
	int		block, remaining;
	int		read;
	byte	*buf;
	int		tries;

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !f ) {
		return 0;
	}

	buf = (byte *)buffer;
	fs_readCount += len;

	if (fsh[f].zipFile == qfalse) {
		remaining = len;
		tries = 0;
		while (remaining) {
			block = remaining;
			read = fread (buf, 1, block, fsh[f].handleFiles.file.o);
			if (read == 0) {
				// we might have been trying to read from a CD, which
				// sometimes returns a 0 read on windows
				if (!tries) {
					tries = 1;
				} else {
					return len-remaining;	//Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
				}
			}

			if (read == -1) {
				Com_Error (ERR_FATAL, "FS_Read: -1 bytes read");
			}

			remaining -= read;
			buf += read;
		}
		return len;
	} else {
		return unzReadCurrentFile(fsh[f].handleFiles.file.z, buffer, len);
	}
}

/*
=================
FS_Write

Properly handles partial writes
=================
*/
int Q_EXTERNAL_CALL FS_Write( const void *buffer, int len, fileHandle_t h )
{
	int		block, remaining;
	int		written;
	byte	*buf;
	int		tries;
	FILE	*f;

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !h ) {
		return 0;
	}

	f = FS_FileForHandle(h);
	buf = (byte *)buffer;

	remaining = len;
	tries = 0;
	while (remaining) {
		block = remaining;
		written = fwrite (buf, 1, block, f);
		if (written == 0) {
			if (!tries) {
				tries = 1;
			} else {
				Com_Printf( "FS_Write: 0 bytes written\n" );
				return 0;
			}
		}

		if (written == -1) {
			Com_Printf( "FS_Write: -1 bytes written\n" );
			return 0;
		}

		remaining -= written;
		buf += written;
	}

	sql_prepare	( &com_db, "UPDATE openfiles SET xfer=xfer+?2 SEARCH f ?1;" );
	sql_bindint	( &com_db, 1, h );
	sql_bindint	( &com_db, 2, len );
	sql_step	( &com_db );
	sql_done	( &com_db );

	return len;
}

void QDECL FS_Printf( fileHandle_t h, const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf( msg, sizeof( msg ) - 1, fmt, argptr );
	msg[sizeof( msg ) - 1] = 0;
	va_end (argptr);

	FS_Write(msg, strlen(msg), h);
}

#define PK3_SEEK_BUFFER_SIZE 65536

/*
=================
FS_Seek

=================
*/
int Q_EXTERNAL_CALL FS_Seek( fileHandle_t f, long offset, int origin )
{
	int		_origin;

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
		return -1;
	}

	if (fsh[f].zipFile == qtrue) {
		//FIXME: this is incomplete and really, really
		//crappy (but better than what was here before)
		byte	buffer[PK3_SEEK_BUFFER_SIZE];
		int		remainder = offset;

		if( offset < 0 || origin == FS_SEEK_END ) {
			Com_Error( ERR_FATAL, "Negative offsets and FS_SEEK_END not implemented "
					"for FS_Seek on pk3 file contents\n" );
			return -1;
		}

		switch( origin ) {
			case FS_SEEK_SET:
				unzSetCurrentFileInfoPosition(fsh[f].handleFiles.file.z, fsh[f].zipFilePos);
				unzOpenCurrentFile(fsh[f].handleFiles.file.z);
				//fallthrough

			case FS_SEEK_CUR:
				while( remainder > PK3_SEEK_BUFFER_SIZE ) {
					FS_Read( buffer, PK3_SEEK_BUFFER_SIZE, f );
					remainder -= PK3_SEEK_BUFFER_SIZE;
				}
				FS_Read( buffer, remainder, f );
				return offset;
				break;

			default:
				Com_Error( ERR_FATAL, "Bad origin in FS_Seek\n" );
				return -1;
				break;
		}
	} else {
		FILE *file;
		file = FS_FileForHandle(f);
		switch( origin ) {
		case FS_SEEK_CUR:
			_origin = SEEK_CUR;
			break;
		case FS_SEEK_END:
			_origin = SEEK_END;
			break;
		case FS_SEEK_SET:
			_origin = SEEK_SET;
			break;
		default:
			_origin = SEEK_CUR;
			Com_Error( ERR_FATAL, "Bad origin in FS_Seek\n" );
			break;
		}

		return fseek( file, offset, _origin );
	}
}


/*
======================================================================================

CONVENIENCE FUNCTIONS FOR ENTIRE FILES

======================================================================================
*/

/*
============
FS_ReadFile

Filename are relative to the quake search path
a null buffer will just return the file length without loading
============
*/
int Q_EXTERNAL_CALL FS_ReadFile( const char *qpath, void **buffer ) {
	fileHandle_t	h;
	byte*			buf;
	qboolean		isConfig;
	int				len;

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "FS_ReadFile with empty name\n" );
	}

	buf = NULL;	// quiet compiler warning

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if ( strstr( qpath, ".cfg" ) ) {
		isConfig = qtrue;
		if ( com_journal && com_journal->integer == 2 ) {
			int		r;

			Com_DPrintf( "Loading %s from journal file.\n", qpath );
			r = FS_Read( &len, sizeof( len ), com_journalDataFile );
			if ( r != sizeof( len ) ) {
				if (buffer != NULL) *buffer = NULL;
				return -1;
			}
			// if the file didn't exist when the journal was created
			if (!len) {
				if (buffer == NULL) {
					return 1;			// hack for old journal files
				}
				*buffer = NULL;
				return -1;
			}
			if (buffer == NULL) {
				return len;
			}

			buf = Hunk_AllocateTempMemory(len+1);
			*buffer = buf;

			r = FS_Read( buf, len, com_journalDataFile );
			if ( r != len ) {
				Com_Error( ERR_FATAL, "Read from journalDataFile failed" );
			}

			fs_loadCount++;
			fs_loadStack++;

			// guarantee that it will have a trailing 0 for string operations
			buf[len] = 0;

			return len;
		}
	} else {
		isConfig = qfalse;
	}

	// look for it in the filesystem or pack files
	len = FS_FOpenFileRead( qpath, &h, qfalse );
	if ( h == 0 ) {
		if ( buffer ) {
			*buffer = NULL;
		}
		// if we are journalling and it is a config file, write a zero to the journal file
		if ( isConfig && com_journal && com_journal->integer == 1 ) {
			Com_DPrintf( "Writing zero for %s to journal file.\n", qpath );
			len = 0;
			FS_Write( &len, sizeof( len ), com_journalDataFile );
			FS_Flush( com_journalDataFile );
		}
		#ifdef DEVELOPER
			sql_prepare	( &com_db, "INSERT INTO missing(path) VALUES($);" );
			sql_bindtext( &com_db, 1, qpath ); 
			sql_step	( &com_db );
			sql_done	( &com_db );
		#endif
		return -1;
	}
	
	if ( !buffer ) {
		if ( isConfig && com_journal && com_journal->integer == 1 ) {
			Com_DPrintf( "Writing len for %s to journal file.\n", qpath );
			FS_Write( &len, sizeof( len ), com_journalDataFile );
			FS_Flush( com_journalDataFile );
		}
		FS_FCloseFile( h);
		return len;
	}

	fs_loadCount++;
	fs_loadStack++;

	buf = Hunk_AllocateTempMemory(len+1);
	*buffer = buf;

	FS_Read (buf, len, h);

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFile( h );

	// if we are journalling and it is a config file, write it to the journal file
	if ( isConfig && com_journal && com_journal->integer == 1 ) {
		Com_DPrintf( "Writing %s to journal file.\n", qpath );
		FS_Write( &len, sizeof( len ), com_journalDataFile );
		FS_Write( buf, len, com_journalDataFile );
		FS_Flush( com_journalDataFile );
	}
	return len;
}

/*
=============
FS_FreeFile
=============
*/
void Q_EXTERNAL_CALL FS_FreeFile( void *buffer ) {
	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}
	if ( !buffer ) {
		Com_Error( ERR_FATAL, "FS_FreeFile( NULL )" );
	}
	fs_loadStack--;

	Hunk_FreeTempMemory( buffer );

	// if all of our temp files are free, clear all of our space
	if ( fs_loadStack == 0 ) {
		Hunk_ClearTempMemory();
	}
}

/*
============
FS_WriteFile

Filename are reletive to the quake search path
============
*/
void Q_EXTERNAL_CALL FS_WriteFile( const char *qpath, const void *buffer, int size ) {
	fileHandle_t f;

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !qpath || !buffer ) {
		Com_Error( ERR_FATAL, "FS_WriteFile: NULL parameter" );
	}

	f = FS_FOpenFileWrite( qpath );
	if ( !f ) {
		Com_Printf( "Failed to open %s\n", qpath );
		return;
	}

	FS_Write( buffer, size, f );

	FS_FCloseFile( f );
}

#ifdef USE_WEBHOST
const char * FS_GetMD5AndLength( const char * name ) {

	const char * info = Cvar_VariableString( "sv_paks" );

	for ( ;; )
	{
		char * n = COM_Parse( &info );
		if ( *n == '\0' ) {
			//	pak wasn't found in server lists
			return 0;
		}

		if ( !Q_stricmp( name, n ) ) {
			break;
		}

		COM_Parse( &info );	// skip md5
		COM_Parse( &info );	// skip length
	}

	return info;
}
#endif


#ifdef USE_WEBHOST
/*
=================
FS_IsPakPure

checks to see if the local pak file is the same as the web host's version
=================
*/
int FS_PakIsPure( const char * filename ) {

	char			sv_md5[ 33 ];
	char			cl_md5[ 33 ];
	int				sv_length;
	int				cl_length;
	const char *	info;
	int				sv_pure;

	sv_pure = Cvar_VariableIntegerValue( "sv_pure" );

	info = FS_GetMD5AndLength( filename );

	//	pak wasn't found in server lists
	if ( !info && sv_pure ) {
		return 0;	// if pure server then don't allow this pak to be loaded
	}

	cl_length = FS_FOpenFileRead( filename, 0, qfalse );

	if ( cl_length <= 0 ) {
		return 0;
	}

	//	allow any pak found matching name load
	if ( sv_pure == 0 ) {
		return 1;
	}

	//	get the MD5 of the pak on the website
	Q_strncpyz( sv_md5, COM_Parse( &info ), sizeof(sv_md5) );
	sv_length = atoi( COM_Parse( &info ) );

	if ( cl_length == sv_length ) {

		//	get the MD5 of the local copy
		Com_MD5File( cl_md5, filename );

		//	return if they're the same
		if ( !Q_stricmp( sv_md5, cl_md5 ) ) {
			return 1;
		}
	}

	return 0;
}
#endif


/*
=================
FS_LoadZipFile

loads the contents of the zip file into the FAT.
=================
*/
void FS_LoadZipFile( const char * filename, const char * basename, const char * ext )
{
	unzFile			uf;
	int				err;
	unz_global_info gi;
	unz_file_info	file_info;
	int				i;
	int				pak_id = -1;
	int				checksum;
	int				fs_numHeaderLongs;
	int				*fs_headerLongs;

	fs_numHeaderLongs = 0;

	//	find the id of the pak file
	sql_prepare	( &com_db, "SELECT id FROM files SEARCH name $1 WHERE ext like $2;" );
	sql_bindtext( &com_db, 1, basename );
	sql_bindtext( &com_db, 2, ext );
	if ( sql_step( &com_db ) ) {
		pak_id = sql_columnasint( &com_db, 0 );
	}
	sql_done	( &com_db );

	sql_bindint	( &com_db, 7, pak_id );

	//	open pak
	uf	= unzOpen( filename );
	err = unzGetGlobalInfo(uf,&gi);

	if (err != UNZ_OK)
		return;

	fs_packFiles += gi.number_entry;

	fs_headerLongs = Z_Malloc( gi.number_entry * sizeof(int) );

	unzGoToFirstFile(uf);
	for (i = 0; i < gi.number_entry; i++)
	{
		char fullname	[ MAX_ZPATH ];
		char path		[ MAX_QPATH ];
		char name		[ MAX_QPATH ];
		char ext		[ 16 ];
		unsigned long	pos;

		err = unzGetCurrentFileInfo		( uf, &file_info, fullname, sizeof(fullname), NULL, 0, NULL, 0);
		if (err != UNZ_OK) {
			break;
		}
		if (file_info.uncompressed_size > 0) {
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong(file_info.crc);
		}
		unzGetCurrentFileInfoPosition	( uf, &pos );

		Sys_SplitPath( fullname, path, sizeof(path), name, sizeof(name), ext, sizeof(ext) );

		sql_bindtext( &com_db, 2, path );
		sql_bindtext( &com_db, 3, name );
		sql_bindtext( &com_db, 4, ext );
		sql_bindint	( &com_db, 5, file_info.uncompressed_size );
		sql_bindint	( &com_db, 6, (int)pos );

		sql_step	( &com_db );

		unzGoToNextFile(uf);
	}

	checksum = Com_BlockChecksum( fs_headerLongs, 4 * fs_numHeaderLongs );
	checksum = LittleLong( checksum );

	Z_Free(fs_headerLongs);

	//	create entry in pak table
	sql_prepare	( &com_db, "INSERT INTO pakfiles(id,handle,name,checksum) VALUES(?1,?2,$3,?4);" );
	sql_bindint	( &com_db, 1, pak_id );
	sql_bindint	( &com_db, 2, (int)uf );
	sql_bindtext( &com_db, 3, filename );
	sql_bindint	( &com_db, 4, checksum );
	sql_step	( &com_db );
	sql_done	( &com_db );
}

/*
===============
FS_AddZipFile

adds the contents of the zip file to FAT.  this function is used for zip files that are outside
of the search paths

===============
*/
void FS_AddZipFile( const char * filename, int length, qboolean mount ) {

	char ext[ 16 ];
	char name[ MAX_ZPATH ];
	char path[ MAX_ZPATH ];
	int	path_id;

	Sys_SplitPath( filename, path, MAX_ZPATH, name, MAX_ZPATH, ext, 16 );
	FS_ReplaceSeparators( path );

	sql_prepare	( &com_db, "SELECT id FROM paths SEARCH path $1;" );
	sql_bindtext( &com_db, 1, path );

	//	pak files can only exist in the root of one of the search paths
	if ( sql_step( &com_db ) ) {
		path_id = sql_columnasint( &com_db, 0 );
	} else {
		path_id = 1;
	}

	sql_done	( &com_db );

	sql_prepare	( &com_db, "UPDATE OR INSERT files SET id=#+1, path_id=?1, path=$2,name=$3,ext=$4, length=?5, offset=?6, pak_id=?7, fullname=path||name||ext SEARCH name $3 WHERE path like $2 AND ext like $4;" );
	sql_bindint	( &com_db, 1, path_id );
	sql_bindtext( &com_db, 2, "" );
	sql_bindtext( &com_db, 3, name );
	sql_bindtext( &com_db, 4, ext );
	sql_bindint	( &com_db, 5, length );
	sql_bindint	( &com_db, 6, 0 );
	sql_bindint	( &com_db, 7, -1 );

	sql_step	( &com_db );

	if ( mount == qtrue ) {
		FS_LoadZipFile( filename, name, ext );
	}

	sql_done	( &com_db );
}


/*
===============
FS_ListFiles

Returns a uniqued list of files that match the given criteria
from all search paths
===============
*/
#define MAX_LISTFILES_BUFFER_SIZE	8192
char **Q_EXTERNAL_CALL FS_ListFiles( const char *path, const char *extension, int *numfiles ) {

	char * buffer = Z_Malloc( MAX_LISTFILES_BUFFER_SIZE );

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !path ) {
		*numfiles = 0;
		return NULL;
	}

	if ( !extension ) {
		extension = "";
	}

	*numfiles = sql_select( &com_db, "SELECT fullname FROM files SEARCH path $1 WHERE (ext like $2) OR ($2 like '');", buffer, buffer, MAX_LISTFILES_BUFFER_SIZE, path, extension );

	if ( *numfiles == 0 ) {
		Z_Free( buffer );
		return NULL;
	}

	return (char**)buffer;
}

/*
=================
FS_FreeFileList
=================
*/
void Q_EXTERNAL_CALL FS_FreeFileList( char **list ) {

	if ( !fs_initialized ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !list ) {
		return;
	}

	Z_Free( list );
}


/*
================
FS_GetFileList
================
*/
int	FS_GetFileList(  const char *path, const char *extension, char * segment, char * buffer, int size ) {

	return sql_select( &com_db, "SELECT fullname FROM files SEARCH path $1 WHERE (ext like $2) OR ($2 like '');", segment, buffer, size, path, extension );
}


/*
================
FS_Shutdown

Frees all resources and closes all files
================
*/
void FS_Shutdown( qboolean closemfp ) {

	int	i;

	//	close all files that are in the file system.
	if ( closemfp ) {
		for(i = 0; i < MAX_FILE_HANDLES; i++) {
			if ( fsh[i].handleFiles.file.o != 0 ) {
				FS_FCloseFile(i);
			}
		}
	}

	// free everything
	sql_prepare	( &com_db, "SELECT handle FROM pakfiles;" );
	while ( sql_step( &com_db ) ) {

		unzFile handle = (unzFile)sql_columnasint( &com_db, 0 );
		unzClose(handle);
	}
	sql_done	( &com_db );

#ifdef DEVELOPER
	Cmd_RemoveCommand( "fsdump" );
	Cmd_RemoveCommand( "fsmissing" );
	Cmd_RemoveCommand( "sql_f" );
#endif

	// any FS_ calls will now be an error until reinitialized
	fs_initialized = 0;
}

static void add_directory( const char * base, const char * path ) {

	if ( base && base[0] ) {
		Com_Printf( "mounting '%s/%s'...\n", base, path );
		sql_bindtext	( &com_db, 1, base );
		sql_bindtext	( &com_db, 2, path );
		sql_step		( &com_db );
	}
}

static void Q_EXTERNAL_CALL FS_sql_f( void )
{
	sql_prompt( &com_db, Cmd_Argv(1) );
}

/*
================
FS_Dump_f

Save the console contents out to a file
================
*/
#ifdef DEVELOPER
void Q_EXTERNAL_CALL FS_Dump_f (void)
{
	fileHandle_t	f;
	char *			filename;

	if (Cmd_Argc() != 2)
	{
		char * boss = Cvar_VariableString( "boss" );

		if ( boss && boss[0] ) {
			filename = va( "%s_%s_%s.dump", Cvar_VariableString( "sv_missionname" ), Cvar_VariableString( "mapname" ), boss );
		} else {
			filename = va( "%s_%s.dump", Cvar_VariableString( "sv_missionname" ), Cvar_VariableString( "mapname" ) );
		}
	} else {
		filename = Cmd_Argv(1);
	}

	Com_Printf ("Dumped file access log to %s.\n", filename );

	f = FS_FOpenFileWrite( filename );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	sql_prepare	( &com_db, "SELECT id FROM access SEARCH path UNIQUE;" );

	while( sql_step( &com_db ) ) {

		int		file_id = sql_columnasint( &com_db, 0 );
		int		len;
		char	buffer[ 1024 ];

		sql_prepare( &com_db, "SELECT access.id[?1]^path;" );
		sql_bindint( &com_db, 1, file_id );

		if ( sql_step( &com_db ) ) {
			const char *	file	= sql_columnastext	( &com_db, 0 );
			len = Com_sprintf( buffer, sizeof(buffer), "%s\n", file );
			FS_Write( buffer, len, f );
		}
		sql_done( &com_db );
	}

	sql_done	( &com_db );

	FS_FCloseFile( f );
}
#endif

/*
================
FS_Missing_f

Dump files we tried to load but couldn't to a file
================
*/
#ifdef DEVELOPER
void Q_EXTERNAL_CALL FS_Missing_f (void)
{
	fileHandle_t	f;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: fsmissing <filename>\n");
		return;
	}

	Com_Printf ("Dumped file missing log to %s.\n", Cmd_Argv(1) );

	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	sql_prepare	( &com_db, "SELECT path FROM missing SEARCH path;" );

	while( sql_step( &com_db ) ) {

		const char *	file	= sql_columnastext	( &com_db, 0 );

		int				len;
		char			buffer[ 1024 ];

		len = Com_sprintf( buffer, sizeof(buffer), "%s\n", file );
		FS_Write( buffer, len, f );
	}

	sql_done	( &com_db );

	FS_FCloseFile( f );
}
#endif


/*
================
FS_Startup
================
*/
static void FS_Startup( const char *game ) {

	char tmp[ MAX_OSPATH ];

	Com_Printf( "----- FS_Startup -----\n" );

	sql_exec( &com_db,	"CREATE TABLE paths "
						"("
							"id INTEGER, "
							"path STRING "
						");"
						"CREATE TABLE files "
						"("
							"id INTEGER, "
							"path_id INTEGER, "
							"fullname STRING, "
							"path STRING, "
							"name STRING, "
							"ext STRING, "
							"offset INTEGER, "
							"length INTEGER, "
							"pak_id INTEGER "
						");"

						"CREATE TABLE pakfiles "
						"("
							"id INTEGER, "
							"handle INTEGER, "
							"name STRING, "
							"checksum INTEGER "
						");"

						"CREATE TABLE IF NOT EXISTS openfiles "
						"("
							"f			INTEGER, "	//	handle to file
							"time		INTEGER, "	//	time file was opened
							"xfer		INTEGER, "	//	number of bytes written
							"pak		INTEGER, "	//	this is a pak file being downloaded
							"length		INTEGER, "	//	the intended length of this file
							"path		INTEGER, "
							"name		STRING "
						");"

#ifdef DEVELOPER
						"CREATE TABLE access "
						"("
							"id INTEGER, "
							"time INTEGER, "
							"path STRING "
						");"
						"CREATE TABLE missing "
						"("
							"path STRING "
						");"
#endif
			);

	fs_initialized = 1;

	fs_debug		= Cvar_Get( "fs_debug",		"0",		0 );
	fs_cdpath		= Cvar_Get( "fs_cdpath",	FS_ReplaceSeparators( Sys_DefaultCDPath(tmp, sizeof( tmp )) ),	CVAR_INIT );
	fs_basepath		= Cvar_Get( "fs_basepath",	FS_ReplaceSeparators( Sys_DefaultInstallPath(tmp, sizeof(tmp)) ),	CVAR_INIT );
	fs_basegame		= Cvar_Get( "fs_basegame",	"",			CVAR_INIT );
	fs_homepath		= Cvar_Get( "fs_homepath",	FS_ReplaceSeparators( Sys_DefaultHomePath( tmp, sizeof(tmp) ) ),	CVAR_INIT );
	fs_gamedirvar	= Cvar_Get( "fs_game",		"",			CVAR_INIT|CVAR_SYSTEMINFO );
	fs_restrict		= Cvar_Get( "fs_restrict",	"",			CVAR_INIT );
	fs_restart		= Cvar_Get( "fs_restart",	"0",		0 );

	fs_packFiles = 0; //reset counter

	//
	//	add paths to search table
	//
	sql_prepare		( &com_db, "INSERT INTO paths(id,path) VALUES(#+1,$1||'/'||$2||'/');" );
	
	add_directory( fs_cdpath->string, game );
	add_directory( fs_basepath->string, game );
	add_directory( fs_homepath->string, game );

	// check for additional game folder for mods
	if ( fs_gamedirvar->string[0] && Q_stricmp( fs_gamedirvar->string, game ) ) {
		add_directory( fs_basepath->string, fs_gamedirvar->string );
		Q_strncpyz( fs_gamedir, fs_gamedirvar->string, sizeof(fs_gamedir) );
	} else {
		Q_strncpyz( fs_gamedir, game, sizeof(fs_gamedir) );
	}

	sql_done		( &com_db );

	fs_gamedirvar->modified = qfalse; // We just loaded, it's not modified

	//
	//	scan search paths and create file table
	//
	sql_prepare( &com_db, "SELECT id, path FROM paths;" );

	while( sql_step( &com_db ) ) {

		const char *	path_name;
		int				path_id;

		path_id		= sql_columnasint	( &com_db, 0 );
		path_name	= sql_columnastext	( &com_db, 1 );

		sql_prepare	( &com_db, "UPDATE OR INSERT files SET id=#+1, path_id=?1, path=$2,name=$3,ext=$4, length=?5, offset=?6, pak_id=?7, fullname=path||name||ext SEARCH name $3 WHERE path like $2 AND ext like $4;" );
		sql_bindint	( &com_db, 1, path_id );
		sql_bindint	( &com_db, 6, 0 );
		sql_bindint	( &com_db, 7, -1 );

		Sys_AddFiles( path_name, "", ".hwp" );

		//	add any pak files found in the root, that are not being downloaded
		{
			char	buffer[ 4096 ];
			char **	paks;
			int		i,n;

			n = sql_select( &com_db, "SELECT name, ext,path||name||ext FROM files SEARCH path_id ?1 SORT 1 WHERE ext like '.hwp' AND openfiles.name[ path||name||ext ].f = -1;", buffer, buffer, sizeof(buffer), path_id );
			paks = (char**)buffer;
			for ( i=0; i<n; i++ ) {
				const char * name	= paks[ i*3+0 ];
				const char * ext	= paks[ i*3+1 ];

#ifdef USE_WEBHOST
				if ( !FS_PakIsPure( paks[ i*3+2 ] ) ) {
					continue;
				}
#endif

				FS_LoadZipFile( va( "%s/%s%s", path_name, name, ext ), name, ext );
			}
		}

		Sys_AddFiles( path_name, "", NULL );

		sql_done	( &com_db );
	}

	sql_done( &com_db );

#ifdef DEVELOPER
	Cmd_AddCommand( "fsdump", FS_Dump_f );
	Cmd_AddCommand( "fsmissing", FS_Missing_f );
	Cmd_AddCommand( "sql_f", FS_sql_f );
#endif

	Com_Printf( "----------------------\n" );
	Com_Printf( "%d files in pk3 files\n", fs_packFiles );
}

/*
===================
FS_SetRestrictions

Looks for product keys and restricts media add on ability
if the full version is not found
===================
*/
static void FS_SetRestrictions( void ) {

#ifndef PRE_RELEASE_DEMO
	char	*productId;

	// if fs_restrict is set, don't even look for the id file,
	// which allows the demo release to be tested even if
	// the full game is present
	if ( !fs_restrict->integer ) {
		// look for the full game id
		FS_ReadFile( "productid.txt", (void **)&productId );
		if ( productId ) {
			// check against the hardcoded string
			int		seed, i;

			seed = 5000;
			for ( i = 0 ; i < sizeof( fs_scrambledProductId ) ; i++ ) {
				if ( ( fs_scrambledProductId[i] ^ (seed&255) ) != productId[i] ) {
					break;
				}
				seed = (69069 * seed + 1);
			}

			FS_FreeFile( productId );

			if ( i == sizeof( fs_scrambledProductId ) ) {
				return;	// no restrictions
			}
			Com_Error( ERR_FATAL, "Invalid product identification" );
		}
	}
#endif
	Cvar_Set( "fs_restrict", "1" );

	Com_Printf( "\nRunning in restricted demo mode.\n\n" );

	// restart the filesystem with just the demo directory
	FS_Shutdown(qfalse);
	FS_Startup( DEMOGAME );

	sql_prepare( &com_db, "SELECT checksum FROM pakfiles;" );
	while ( sql_step( &com_db ) ) {
		int checksum = sql_columnasint( &com_db, 0 );
		// a tiny attempt to keep the checksum from being scannable from the exe
		if ( (checksum ^ 0x02261994u) != (DEMO_PAK_CHECKSUM ^ 0x02261994u) ) {
			Com_Error( ERR_FATAL, "Corrupted pak0.pk3: %u", checksum );
		}
	}
	sql_done( &com_db );
}

/*
================
FS_InitFilesystem

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void FS_InitFilesystem( void ) {
	// allow command line parms to override our defaults
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	Com_StartupVariable( "fs_cdpath" );
	Com_StartupVariable( "fs_basepath" );
	Com_StartupVariable( "fs_homepath" );
	Com_StartupVariable( "fs_game" );
	Com_StartupVariable( "fs_restrict" );

	Cvar_Set( "fs_noconfig", "0" );

	// try to start up normally
	FS_Startup( BASEGAME );

	// see if we are going to allow add-ons
#ifndef DEVELOPER
	FS_SetRestrictions();
#endif
	
	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 ) {
#ifndef USE_BOOTWITHNOFILES
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
#endif
		// bk001208 - SafeMode see below, FIXME?
		Cvar_Set( "fs_noconfig", "1" );
	}

  // bk001208 - SafeMode see below, FIXME?
}


/*
================
FS_Restart
================
*/
void FS_Restart( int checksumFeed )
{

	// free anything we currently have loaded
	FS_Shutdown(qfalse);

	// set the checksum feed
	fs_checksumFeed = checksumFeed;

	// try to start up normally
	FS_Startup( BASEGAME );

	// see if we are going to allow add-ons
#ifndef DEVELOPER
	FS_SetRestrictions();
#endif
	
	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 ) {
#ifndef USE_BOOTWITHNOFILES
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
#endif
		Cvar_Set( "fs_noconfig", "1" );
	} else {
		Cvar_Set("fs_noconfig", "0");
	}

	Cbuf_AddText ( "exec default.cfg\n" );

	// skip the st.cfg if "safe" is on the command line
	if ( !Com_SafeMode() ) {
		Cbuf_AddText ("exec st.cfg\n");
	}
}

/*
=================
FS_ConditionalRestart
restart if necessary
=================
*/
qboolean FS_ConditionalRestart( int checksumFeed ) {
	if( fs_gamedirvar->modified || fs_restart->integer || checksumFeed != fs_checksumFeed ) {
		FS_Restart( checksumFeed );
		Cvar_Set( "fs_restart", "0" );
		return qtrue;
	}
	return qfalse;
}

/*
========================================================================================

Handle based file calls for virtual machines

========================================================================================
*/

int Q_EXTERNAL_CALL FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode )
{
	int		r;

	switch( mode ) {
	case FS_READ:
		r = FS_FOpenFileRead( qpath, f, qfalse );
		break;
	case FS_WRITE:
		*f = FS_FOpenFileWrite( qpath );
		r = 0;
		if (*f == 0) {
			r = -1;
		}
		break;
	case FS_READ_DIRECT:
		r = FS_FOpenFileDirect( qpath, f );
		break;

	case FS_APPEND:
		r = FS_FOpenFileAppend( qpath, f );
		if (*f == 0) {
			r = -1;
		}
		break;
#ifdef DEVELOPER
	case FS_UPDATE:
		*f = FS_FOpenFileUpdate( qpath, &r );
		r = 0;
		if (*f == 0) {
			r = -1;
		}
		break;
#endif
	default:
		Com_Error( ERR_FATAL, "FSH_FOpenFile: bad mode" );
		return -1;
	}

	if (!f) {
		return r;
	}

	return r;
}

int		FS_FTell( fileHandle_t f ) {
	int pos;
	if (fsh[f].zipFile == qtrue) {
		pos = unztell(fsh[f].handleFiles.file.z);
	} else {
		pos = ftell(fsh[f].handleFiles.file.o);
	}
	return pos;
}

void	FS_Flush( fileHandle_t f ) {
	fflush(fsh[f].handleFiles.file.o);
}

void	FS_FilenameCompletion( const char *dir, const char *ext, qboolean stripExt, void(*callback)(const char *s) ) {

	sql_prepare	( &com_db, "SELECT name,name||ext FROM files SEARCH path $1 WHERE (ext like $2) OR ($2 like '');" );
	sql_bindtext( &com_db, 1, dir );
	sql_bindtext( &com_db, 2, ext );

	while( sql_step( &com_db ) ) {
		callback( sql_columnastext( &com_db, (stripExt)?0:1 ) );
	}

	sql_done	( &com_db );
}
