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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#import "../client/client.h"
#import "macosx_local.h"

#import "dlfcn.h"
#import "Q3Controller.h"

#import <AppKit/AppKit.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOBSD.h>
#import <IOKit/storage/IOCDMedia.h>
#import <mach/mach_error.h>

#import <sys/types.h>
#import <unistd.h>
#import <sys/param.h>
#import <sys/mount.h>
#import <sys/sysctl.h>
#import <sys/stat.h>
#import <fts.h>

#ifdef OMNI_TIMER
#import "macosx_timers.h"
#endif

#import <CoreServices/CoreServices.h>

qboolean stdin_active = qfalse;

static jmp_buf sys_exitframe;
static int sys_retcode;

static char sys_cmdline[MAX_STRING_CHARS];

#ifndef DEDICATED
WinVars_t	g_wv;
#endif

void Sys_GetCpuInfo( cpuInfo_t *cpuInfo ) {
	Com_Memset( cpuInfo, 0, sizeof( cpuInfo_t ) );
}

qboolean Sys_Fork( const char *path, char *cmdLine ) {
	int pid;
	
	pid = fork();
	if (pid == 0) {
		struct stat filestat;
		const char *argv[3];
		
		//Try to set the executable bit
		if ( stat(path, &filestat) == 0 )
		{
			chmod(path, filestat.st_mode | S_IXUSR);
		}
		argv[0] = path;
		argv[1] = cmdLine;
		argv[2] = NULL;
		execv(path, argv);
		printf("Exec Failed for: %s\n", path);
		_exit(255);
	} else if (pid == -1) {
		return qfalse;
	}
	return qtrue;
}
/*
 ===============
 Sys_SwapVersion
 - check to see if we are running the highest
 numbered exe, and switch to it if we are not
 ===============
 */
extern cvar_t *fs_homepath;
extern char *fs_gamedir;

void Sys_SwapVersion( )
{
	int i;
	char ospath[MAX_OSPATH];
	char *homedir, *gamedir, *cdpath;
	FILE *f;
	
	homedir = Cvar_VariableString("fs_homepath");
	gamedir = Cvar_VariableString("fs_game");
	cdpath = Cvar_VariableString("fs_cdpath");
	for (i=99; i >= 0; i--) {
		FS_BuildOSPath( ospath, sizeof(ospath), homedir, gamedir, va(PLATFORM_EXE_NAME, i) );
		f = fopen( ospath, "rb" );
		if (f) {
			fclose( f );
			Q_strcat(sys_cmdline, sizeof(sys_cmdline), va("+set fs_cdpath %s", cdpath)
					 );
			if (Sys_Fork( ospath, sys_cmdline ) ) {
				Com_Quit_f ();
			}
		}
	}
}


//===========================================================================
void Sys_Sleep( int msec )
{
}
qboolean Sys_IsForeground( void )
{
	return qtrue;
}
qboolean Sys_OpenUrl( const char *url )
{
	NSString *stringURL;
	char full_url[2048];
	full_url[0] = NULL;
	if (Q_stricmpn("http://", url, 7) != 0) {
		Q_strcat( full_url, sizeof(full_url), "http://");
	} 
	Q_strcat(full_url, sizeof(full_url), url);
	stringURL = [NSString stringWithCString:full_url];
	[[NSWorkspace sharedWorkspace] openURL: [NSURL URLWithString: stringURL]];
	// open url somehow
	return qtrue;
}

void Sys_WriteDump( const char *fmt, ... )
{
  
}
void* Q_EXTERNAL_CALL Sys_PlatformGetVars( void )
{
	return &g_wv;
}
qboolean Sys_DetectAltivec( void )
{
  qboolean altivec = qfalse;
  
#if idppc_altivec
#ifdef MACOS_X
  long feat = 0;
  OSErr err = Gestalt(gestaltPowerPCProcessorFeatures, &feat);
  if ((err==noErr) && ((1 << gestaltPowerPCHasVectorInstructions) & feat)) {
    altivec = qtrue;
  }
#else
  void (*handler)(int sig);
  handler = signal(SIGILL, illegal_instruction);
  if ( setjmp(jmpbuf) == 0 ) {
    asm volatile ("mtspr 256, %0\n\t"
                  "vand %%v0, %%v0, %%v0"
                  :
                  : "r" (-1));
    altivec = qtrue;
  }
  signal(SIGILL, handler);
#endif
#endif
  
  return altivec;
}
/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/
void Sys_SplitPath( const char * fullname, char * path, int path_size, char * name, int name_size, char * ext, int ext_size )
{
	char _dir		[ MAX_OSPATH ];
	char _name		[ MAX_OSPATH ];
	char *extloc;
  
	Q_strncpyz( _dir, fullname, sizeof(_dir) );
	Q_strncpyz( _name, fullname, sizeof(_name) );
	
	if ( path ) {
		char *dirpath = dirname(_dir);
		if (dirpath && dirpath[0] != '.') {
			Q_strncpyz( path, dirpath, path_size );
			Q_strcat(path, path_size, "/");
		} else {
			Q_strncpyz( path, "", path_size);
		}
	}
	if ( name )
		COM_StripExtension( COM_SkipPath(_name), name, name_size );
	if ( ext ) {
		if ( (extloc = rindex(fullname, '.')) != NULL ) {
			if (rindex(extloc, '/') == NULL){
				Q_strncpyz( ext, extloc, ext_size);
			} else {
				ext[0] = NULL;
			}
		} else {
			ext[0] = NULL;
		}
	}
}
// Creates any directories needed to be able to create a file at the specified path.  Raises an exception on failure.
static void Sys_CreatePathToFile(NSString *path, NSDictionary *attributes)
{
    NSArray *pathComponents;
    unsigned int dirIndex, dirCount;
    unsigned int startingIndex;
    NSFileManager *manager;
    
    manager = [NSFileManager defaultManager];
    pathComponents = [path pathComponents];
    dirCount = [pathComponents count] - 1;

    startingIndex = 0;
    for (dirIndex = startingIndex; dirIndex < dirCount; dirIndex++) {
        NSString *partialPath;
        BOOL fileExists;

        partialPath = [NSString pathWithComponents:[pathComponents subarrayWithRange:NSMakeRange(0, dirIndex + 1)]];
        
        // Don't use the 'fileExistsAtPath:isDirectory:' version since it doesn't traverse symlinks
        fileExists = [manager fileExistsAtPath:partialPath];
        if (!fileExists) {
            if (![manager createDirectoryAtPath:partialPath attributes:attributes]) {
                [NSException raise:NSGenericException format:@"Unable to create a directory at path: %@", partialPath];
            }
        } else {
            NSDictionary *attributes;

            attributes = [manager fileAttributesAtPath:partialPath traverseLink:YES];
            if (![[attributes objectForKey:NSFileType] isEqualToString: NSFileTypeDirectory]) {
                [NSException raise:NSGenericException format:@"Unable to write to path \"%@\" because \"%@\" is not a directory",
                    path, partialPath];
            }
        }
    }
}

int main(int argc, const char *argv[]) {
#ifdef DEDICATED
    Q3Controller *controller;
    
    stdin_active = qtrue;
    controller = [[Q3Controller alloc] init];
    [controller quakeMain];
    return 0;
#else
    return NSApplicationMain(argc, argv);
#endif
}
#if 0
int TheRealMain(char *cmdline, void * hWnd) {
    NSString *installationPathKey, *installationPath;
	NSString *appName;
	NSAutoreleasePool *pool;
	int count=0;  
	struct timeval tp;
#if !defined( DEDICATED )
	g_wv.plugin = hWnd;
#endif

	pool = [[NSAutoreleasePool alloc] init];
	
	//[NSApp setServicesProvider:self];
	
    installationPathKey = @"RetailInstallationPath";

    installationPath = [[NSUserDefaults standardUserDefaults] objectForKey:installationPathKey];
    if (!installationPath) {
        // Default to the directory containing the executable (which is where most users will want to put it
        installationPath = [[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent];
    }
	
	appName = @"SpaceTrader";
    // Create the application support directory if it doesn't exist already
    do {
        NSArray *results;
        NSString *libraryPath, *homePath, *filePath;
        NSDictionary *attributes;
        
        results = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
        if (![results count])
            break;
        
        libraryPath = [results objectAtIndex: 0];
        homePath = [libraryPath stringByAppendingPathComponent: @"Application Support"];
        homePath = [homePath stringByAppendingPathComponent: appName];
        filePath = [homePath stringByAppendingPathComponent: @"foo"];
        
        attributes = [NSDictionary dictionaryWithObjectsAndKeys: [NSNumber numberWithUnsignedInt: 0750], NSFilePosixPermissions, nil];
        NS_DURING {
            Sys_CreatePathToFile(filePath, attributes);
            Sys_SetDefaultHomePath([homePath fileSystemRepresentation]);
			// Let the filesystem know where our local install is
			Sys_SetDefaultInstallPath([homePath fileSystemRepresentation]);
        } NS_HANDLER {
            NSLog(@"Exception: %@", localException);
#ifndef DEDICATED
            NSRunAlertPanel(nil, @"Unable to create '%@'.  Please make sure that you have permission to write to this folder and re-run the game.", @"OK", nil, nil, homePath);
#endif
            Sys_Quit();
        } NS_ENDHANDLER;
    } while(0);

	//Seed our random number generator
	gettimeofday(&tp, NULL);
	Sys_QueEvent( 0, SE_RANDSEED, ((tp.tv_sec*1000) + (tp.tv_usec/1000)), 0, 0, 0 );

	
	Com_Init(cmdline);
	NET_Init();
  
	//Sys_ConsoleInputInit();
  
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
  
	InitSig();
  
	[NSApp activateIgnoringOtherApps:YES];
  
	while (1)
	{
		if (setjmp( sys_exitframe ) ) {
	#if !defined( DEDICATED )
		  S_Shutdown();
		  CL_ShutdownRef();
	#endif
		  return sys_retcode;
		}
		//Check for input
		Com_Frame ();
		
		if ((count & 15)==0) {
            // We should think about doing this less frequently than every frame
            [pool release];
            pool = [[NSAutoreleasePool alloc] init];
        }
	}
    [pool release];
	  
	return 0;
}
#endif
//===========================================================================

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	dlclose( dllHandle );
}

/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine
=================
*/
void	FS_BuildOSPath( char * ospath, int size, const char *base, const char *game, const char *qpath );

void	* QDECL Sys_LoadDll( const char *name, char *fqpath , intptr_t (QDECL **entryPoint)(int, ...),
                           intptr_t (QDECL *systemcalls)(intptr_t, ...) ) {
    void *libHandle;
    void	(*dllEntry)( int (*syscallptr)(int, ...) );
    NSString *libraryPath;
    const char *path;
    
	// TTimo
	// I don't understand the search strategy here. How can the Quake3 bundle know about the location
	// of the other bundles? is that configured somewhere in XCode?
	/*
    bundlePath = [[NSBundle mainBundle] pathForResource: [NSString stringWithCString: name] ofType: @"bundle"];
    libraryPath = [NSString stringWithFormat: @"%@/Contents/MacOS/%s", bundlePath, name];
	*/	
	libraryPath = [NSString stringWithFormat: @"%s.bundle/Contents/MacOS/%s", name, name];
    if (!libraryPath)
        return NULL;
    
    path = [libraryPath cString];
    Com_Printf("Loading '%s'.\n", path);
    libHandle = dlopen( [libraryPath cString], RTLD_LAZY );
    if (!libHandle) {
        libHandle = dlopen( name, RTLD_LAZY );
        if (!libHandle) {
            Com_Printf("Error loading dll: %s\n", dlerror());
            return NULL;
        }
    }

    dllEntry = dlsym( libHandle, "_dllEntry" );
    if (!dllEntry) {
        Com_Printf("Error loading dll:  No dllEntry symbol.\n");
        dlclose(libHandle);
        return NULL;
    }
    
    *entryPoint = dlsym( libHandle, "_vmMain" );
    if (!*entryPoint) {
        Com_Printf("Error loading dll:  No vmMain symbol.\n");
        dlclose(libHandle);
        return NULL;
    }
    
    dllEntry(systemcalls);
    return libHandle;
}

//===========================================================================

char *Sys_GetClipboardData(void) // FIXME
{
    NSPasteboard *pasteboard;
    NSArray *pasteboardTypes;

    pasteboard = [NSPasteboard generalPasteboard];
    pasteboardTypes = [pasteboard types];
    if ([pasteboardTypes containsObject:NSStringPboardType]) {
        NSString *clipboardString;

        clipboardString = [pasteboard stringForType:NSStringPboardType];
        if (clipboardString && [clipboardString length] > 0) {
            return strdup([clipboardString cString]);
        }
    }
    return NULL;
}

char *Sys_GetWholeClipboard ( void )
{
    return NULL;
}

void Sys_SetClipboard (const char *contents)
{
}


/*
==================
Sys_FunctionCheckSum
==================
*/
int Sys_FunctionCheckSum(void *f1) {
	return 0;
}

/*
==================
Sys_MonkeyShouldBeSpanked
==================
*/
int Sys_MonkeyShouldBeSpanked( void ) {
	return 0;
}

//===========================================================================

void Sys_BeginProfiling(void)
{
}

void Sys_EndProfiling(void)
{
}

//===========================================================================

/*
================
Sys_Init

The cvar and file system has been setup, so configurations are loaded
================
*/
void Sys_Init(void)
{
#ifdef OMNI_TIMER
    InitializeTimers();
    OTStackPushRoot(rootNode);
#endif
	{
		long arch = 0;
		long speed = 0;
		OSErr err = Gestalt(gestaltSysArchitecture, &arch);
		if (err==noErr) {
			Gestalt(gestaltProcClkSpeed, &speed);
			switch (arch)
			{
				case gestalt68k:
					Cvar_Set("sys_cpustring", va("68k %.2f MHz", speed/1000000.0f));
					break;
				case gestaltPowerPC:
					Cvar_Set("sys_cpustring", va("PPC %.2f GHz", speed/1000000000.0f));
					break;
				case gestaltIntel:
					Cvar_Set("sys_cpustring", va("Intel %.2f GHz", speed/1000000000.0f));
					break;
			}
		}
	}
    NET_Init();
    Sys_InitInput();	
}
/*
========================
Sys_SecretSauce
========================
*/
char *Sys_SecretSauce(char *buffer, int size) {
	NSString         *result = @"";
	mach_port_t       masterPort;
	kern_return_t      kr = noErr;
	io_registry_entry_t  entry;    
	CFDataRef         propData;
	CFTypeRef         prop;
	CFTypeID         propID=NULL;
	UInt8           *data;
	unsigned int    bufSize;
	char            serial[128];
	memset( serial, 0, sizeof(serial) );
	kr = IOMasterPort(MACH_PORT_NULL, &masterPort);        
	if (kr == noErr) {
		entry = IORegistryGetRootEntry(masterPort);
		if (entry != MACH_PORT_NULL) {
			prop = IORegistryEntrySearchCFProperty(entry,
												   kIODeviceTreePlane,
												   CFSTR("serial-number"),
												   nil, kIORegistryIterateRecursively);
			if (prop == nil) {
				result = @"null";
			} else {
				propID = CFGetTypeID(prop);
			}
			if (propID == CFDataGetTypeID()) {
				propData = (CFDataRef)prop;
				bufSize = CFDataGetLength(propData);
				if (bufSize > 0) {
					data = CFDataGetBytePtr(propData);
					memcpy( serial, data, bufSize-1 );
					Com_MD5Buffer( buffer, serial, size );
					return buffer;
				}
			}
		}
		mach_port_deallocate(mach_task_self(), masterPort);
	}
	return NULL;
}




/*
=================
Sys_Shutdown
=================
*/
void Sys_Shutdown(void)
{
    Com_Printf( "----- Sys_Shutdown -----\n" );
    Sys_EndProfiling();
    Sys_ShutdownInput();	
    Com_Printf( "------------------------\n" );
}

void Sys_Error(const char *error, ...)
{
    va_list argptr;
    NSString *formattedString;

    Sys_Shutdown();

    va_start(argptr,error);
    formattedString = [[NSString alloc] initWithFormat:[NSString stringWithCString:error] arguments:argptr];
    va_end(argptr);

    NSLog(@"Sys_Error: %@", formattedString);
    NSRunAlertPanel(@"SpaceTrader Error", formattedString, nil, nil, nil);

    Sys_Quit();
}

void Sys_Exit( int ex ) {
  //Sys_ConsoleInputShutdown();
  sys_retcode = ex;
  longjmp( sys_exitframe, -1);
}

void Sys_Quit(void)
{
    Sys_Shutdown();
    [NSApp terminate:nil];
}

/*
================
Sys_Print

This is called for all console output, even if the game is running
full screen and the dedicated console window is hidden.
================
*/

char *ansiColors[8] =
	{ "\033[30m" ,	/* ANSI Black */
	  "\033[31m" ,	/* ANSI Red */
	  "\033[32m" ,	/* ANSI Green */
	  "\033[33m" ,  /* ANSI Yellow */
	  "\033[34m" ,	/* ANSI Blue */
	  "\033[36m" ,  /* ANSI Cyan */
	  "\033[35m" ,	/* ANSI Magenta */
	  "\033[37m" }; /* ANSI White */
	  
void Sys_Print(const char *text)
{
#if 0
	/* Okay, this is a stupid hack, but what the hell, I was bored. ;) */
	const char *scan = text;
	int index;
	
	/* Make sure terminal mode is reset at the start of the line... */
	fputs("\033[0m", stdout);
	
	while(*scan) {
		/* See if we have a color control code.  If so, snarf the character, 
		print what we have so far, print the ANSI Terminal color code,
		skip over the color control code and continue */
		if(Q_IsColorString(scan)) {
			index = ColorIndex(scan[1]);
			
			/* Flush current message */
			if(scan != text) {
				fwrite(text, scan - text, 1, stdout);
			}
			
			/* Write ANSI color code */
			fputs(ansiColors[index], stdout);
			
			/* Reset search */
			text = scan+2;
			scan = text;
			continue;			
		}
		scan++;
	}

	/* Flush whatever's left */
	fputs(text, stdout);

	/* Make sure terminal mode is reset at the end of the line too... */
	fputs("\033[0m", stdout);

#else
    fputs(text, stdout);
#endif	
}



/*
================
Sys_CheckCD

Return true if the proper CD is in the drive
================
*/

qboolean Sys_ObjectIsCDRomDevice(io_object_t object)
{
    CFStringRef value;
    kern_return_t krc;
    CFDictionaryRef properties;
    qboolean isCDROM = qfalse;
    io_iterator_t parentIterator;
    io_object_t parent;
    
    krc = IORegistryEntryCreateCFProperties(object, &properties, kCFAllocatorDefault, (IOOptionBits)0);
    if (krc != KERN_SUCCESS) {
        fprintf(stderr, "IORegistryEntryCreateCFProperties returned 0x%08x -- %s\n", krc, mach_error_string(krc));
        return qfalse;
    }

    //NSLog(@"properties = %@", properties);
    
    // See if this is a CD-ROM
    value = CFDictionaryGetValue(properties, CFSTR(kIOCDMediaTypeKey));
    if (value && CFStringCompare(value, CFSTR("CD-ROM"), 0) == kCFCompareEqualTo)
        isCDROM = qtrue;
    CFRelease(properties);

    // If it isn't check each of its parents.  It seems that the parent enumerator only returns the immediate parent.  Maybe the plural indicates that an object can have multiple direct parents.  So, we'll call ourselves recursively for each parent.
    if (!isCDROM) {
        krc = IORegistryEntryGetParentIterator(object, kIOServicePlane, &parentIterator);
        if (krc != KERN_SUCCESS) {
            fprintf(stderr, "IOServiceGetMatchingServices returned 0x%08x -- %s\n",
                    krc, mach_error_string(krc));
        } else {
            while (!isCDROM && (parent = IOIteratorNext(parentIterator))) {
                if (Sys_ObjectIsCDRomDevice(parent))
                    isCDROM = qtrue;
                IOObjectRelease(parent);
            }
    
            IOObjectRelease(parentIterator);
        }
    }
    
    //NSLog(@"Sys_ObjectIsCDRomDevice -> %d", isCDROM);
    return isCDROM;
}

qboolean Sys_IsCDROMDevice(const char *deviceName)
{
    kern_return_t krc;
    io_iterator_t deviceIterator;
    mach_port_t masterPort;
    io_object_t object;
    qboolean isCDROM = qfalse;
    
    krc = IOMasterPort(bootstrap_port, &masterPort);
    if (krc != KERN_SUCCESS) {
        fprintf(stderr, "IOMasterPort returned 0x%08x -- %s\n", krc, mach_error_string(krc));
        return qfalse;
    }

    // Get an iterator for this BSD device.  If it is a CD, it will likely only be one partition of the larger CD-ROM device.
    krc = IOServiceGetMatchingServices(masterPort,
                                       IOBSDNameMatching(masterPort, 0, deviceName),
                                       &deviceIterator);
    if (krc != KERN_SUCCESS) {
        fprintf(stderr, "IOServiceGetMatchingServices returned 0x%08x -- %s\n",
                krc, mach_error_string(krc));
        return qfalse;
    }

    while (!isCDROM && (object = IOIteratorNext(deviceIterator))) {
        if (Sys_ObjectIsCDRomDevice(object)) {
            isCDROM = qtrue;
        }
        IOObjectRelease(object);
    }
    
    IOObjectRelease(deviceIterator);

    //NSLog(@"Sys_IsCDROMDevice -> %d", isCDROM);
    return isCDROM;
}

qboolean        Sys_CheckCD( void )
{
    // DO NOT just return success here if we have a library directory.
    // Actually look for the CD.

    // We'll look through the actual mount points rather than just looking
    // for a particular directory since (a) the mount point may change
    // between OS version (/foo in Public Beta, /Volumes/foo after Public Beta)
    // and (b) this way someone can't just create a directory and warez the files.
    
    unsigned int mountCount;
    struct statfs  *mounts;
    
    mountCount = getmntinfo(&mounts, MNT_NOWAIT);
    if (mountCount <= 0) {
        perror("getmntinfo");
#if 1 // Q3:TA doesn't need a CD, but we still need to locate it to allow for partial installs
        return qtrue;
#else
        return qfalse;
#endif
    }
    
    while (mountCount--) {
        const char *lastComponent;
        
        if ((mounts[mountCount].f_flags & MNT_RDONLY) != MNT_RDONLY) {
            // Should have been a read only CD... this isn't it
            continue;
        }
        
        if ((mounts[mountCount].f_flags & MNT_LOCAL) != MNT_LOCAL) {
            // Should have been a local filesystem
            continue;
        }
        
        lastComponent = strrchr(mounts[mountCount].f_mntonname, '/');
        if (!lastComponent) {
            // No slash in the mount point!  How is that possible?
            continue;
        }
        
        // Skip the slash and look for the game name
        lastComponent++;
        if ((strcasecmp(lastComponent, "Quake3") != 0)) {
            continue;
        }

            
#if 0
        fprintf(stderr, "f_bsize: %d\n", mounts[mountCount].f_bsize);
        fprintf(stderr, "f_blocks: %d\n", mounts[mountCount].f_blocks);
        fprintf(stderr, "type: %d\n", mounts[mountCount].f_type);
        fprintf(stderr, "flags: %d\n", mounts[mountCount].f_flags);
        fprintf(stderr, "fstype: %s\n", mounts[mountCount].f_fstypename);
        fprintf(stderr, "f_mntonname: %s\n", mounts[mountCount].f_mntonname);
        fprintf(stderr, "f_mntfromname: %s\n", mounts[mountCount].f_mntfromname);
        fprintf(stderr, "\n\n");
#endif

        lastComponent = strrchr(mounts[mountCount].f_mntfromname, '/');
        if (!lastComponent) {
            // No slash in the device name!  How is that possible?
            continue;
        }
        lastComponent++;
        if (!Sys_IsCDROMDevice(lastComponent))
            continue;

        // This looks good
        Sys_SetDefaultCDPath(mounts[mountCount].f_mntonname);
        return qtrue;
    }
    
#if 1 // Q3:TA doesn't need a CD, but we still need to locate it to allow for partial installs
    return qtrue;
#else
    return qfalse;
#endif
}


//===================================================================

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


void OutputDebugString(char * s)
{
#ifdef DEBUG
    fprintf(stderr, "%s", s);
#endif
}

/*
==================
Sys_LowPhysicalMemory()
==================
*/
#define MEM_THRESHOLD 96*1024*1024

qboolean Sys_LowPhysicalMemory()
{
    return NSRealMemoryAvailable() <= MEM_THRESHOLD;
}

static unsigned int _Sys_ProcessorCount = 0;

unsigned int Sys_ProcessorCount()
{
    if (!_Sys_ProcessorCount) {
        int name[] = {CTL_HW, HW_NCPU};
        size_t size;
    
        size = sizeof(_Sys_ProcessorCount);
        if (sysctl(name, 2, &_Sys_ProcessorCount, &size, NULL, 0) < 0) {
            perror("sysctl");
            _Sys_ProcessorCount = 1;
        } else {
            Com_Printf("System processor count is %d\n", _Sys_ProcessorCount);
        }
    }
    
    return _Sys_ProcessorCount;
}

