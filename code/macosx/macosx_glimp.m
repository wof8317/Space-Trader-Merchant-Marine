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
#include "../renderer/tr_local.h"

#import "macosx_glimp.h"

#import "macosx_local.h"
#import "macosx_display.h"
#import "macosx_timers.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#import <mach-o/dyld.h>
#import <mach/mach.h>
#import <mach/mach_error.h>
#import <sys/utsname.h>

#include "../sql/sql.h"
extern sqlInfo_t com_db;

#define MAX_DISPLAYS 32

cvar_t	*r_allowSoftwareGL;		// don't abort out if the pixelformat claims software
cvar_t  *r_enablerender;                // Enable actual rendering
cvar_t  *r_appleTransformHint;          // Enable Apple transform hint

static void GLW_InitExtensions( void );
static qboolean CreateGameWindow( qboolean isSecondTry );
static unsigned long Sys_QueryVideoMemory();
static CGDisplayErr Sys_CaptureActiveDisplays(void);

typedef enum
{
  RSERR_OK,
  
  RSERR_INVALID_FULLSCREEN,
  RSERR_INVALID_MODE,
  
  RSERR_UNKNOWN
} rserr_t;

glwstate_t glw_state;
qboolean Sys_IsHidden = qfalse;

//not implemented this side yet.  Force it to false restores old behavior
bool glimp_suspendRender = false;

#ifdef OMNI_TIMER
OTStampList glThreadStampList;
#endif

@interface NSOpenGLContext (CGLContextAccess)
- (CGLContextObj) cglContext;
@end

@implementation NSOpenGLContext (CGLContextAccess)
- (CGLContextObj) cglContext;
{
    return _contextAuxiliary;
}

@end

EventTrap *evtTrap;
/*
============
CheckErrors
============
*/
void CheckErrors( void )
{		
    GLenum   err;

    err = glGetError();
    if ( err != GL_NO_ERROR ) {
        ri.Error( ERR_FATAL, "glGetError: %s\n", glGetString( err ) );
    }
}

#if !defined(NDEBUG) && defined(QGL_CHECK_GL_ERRORS)

unsigned int QGLBeginStarted = 0;

void QGLErrorBreak(void)
{
}

void QGLCheckError(const char *message)
{
    GLenum        error;
    static unsigned int errorCount = 0;
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        if (errorCount == 100) {
            Com_Printf("100 GL errors printed ... disabling further error reporting.\n");
        } else if (errorCount < 100) {
            if (errorCount == 0) {
                fprintf(stderr, "BREAK ON QGLErrorBreak to stop at the GL errors\n");
            }
            fprintf(stderr, "OpenGL Error(%s): 0x%04x -- %s\n", message, (int)error,  gluErrorString(error));
            QGLErrorBreak();
        }
        errorCount++;
    }
}
#endif


/*
** GLimp_SetMode
*/

qboolean GLimp_SetMode( qboolean isSecondTry )
{
    if ( !CreateGameWindow(isSecondTry) ) {
        ri.Printf( PRINT_ALL, "GLimp_Init: window could not be created!\n" );
        return qfalse;
    }

    // draw something to show that GL is alive	
    if (r_enablerender->integer) {
        glClearColor( 0.5, 0.5, 0.7, 0 );
        glClear( GL_COLOR_BUFFER_BIT );
        GLimp_EndFrame();
        
        glClearColor( 0.5, 0.5, 0.7, 0 );
        glClear( GL_COLOR_BUFFER_BIT );
        GLimp_EndFrame();
    }

    Sys_UnfadeScreen(glw_state.display, NULL);
    
    CheckErrors();

    return qtrue;
}

/*
 =================
 GetPixelAttributes
 =================
 */

#define ADD_ATTR(x) \
do { \
    if (attributeIndex >= attributeSize) { \
        attributeSize *= 2; \
        pixelAttributes = NSZoneRealloc(NULL, pixelAttributes, sizeof(*pixelAttributes) * attributeSize); \
    } \
    pixelAttributes[attributeIndex] = x; \
    attributeIndex++; \
    if (verbose) { \
        ri.Printf(PRINT_ALL, "Adding pixel attribute: %d (%s)\n", x, #x); \
    } \
} while(0)

static NSOpenGLPixelFormatAttribute *GetPixelAttributes()
{
    NSOpenGLPixelFormatAttribute *pixelAttributes;
    unsigned int attributeIndex = 0;
    unsigned int attributeSize = 128;
    int verbose = 0;
    unsigned int colorDepth;
    
    verbose = r_verbose->integer;
    
    pixelAttributes = NSZoneMalloc(NULL, sizeof(*pixelAttributes) * attributeSize);

    if (r_fullscreen->integer) {
        ADD_ATTR(NSOpenGLPFAFullScreen);

        // Since we are fullscreen, specify the screen that we need.
        ADD_ATTR(NSOpenGLPFAScreenMask);
        ADD_ATTR(CGDisplayIDToOpenGLDisplayMask(glw_state.display));
    }
    
    // Require hardware acceleration unless otherwise directed
    if (!r_allowSoftwareGL->integer) {
        ADD_ATTR(NSOpenGLPFAAccelerated);
    }

    // Require double-buffer
    ADD_ATTR(NSOpenGLPFADoubleBuffer);

    // Specify the number of color bits.  If we don't have a valid specified value or we are not full screen, use the current display mode's value.
    ADD_ATTR(NSOpenGLPFAColorSize);
    colorDepth = r_colorbits->integer;
    if (colorDepth < 16)
        colorDepth = 16;
    else if (colorDepth > 16)
        colorDepth = 32;
    if (!r_fullscreen->integer)
        colorDepth = [[glw_state.desktopMode objectForKey: (id)kCGDisplayBitsPerPixel] intValue];
    ADD_ATTR(colorDepth);

    // Specify the number of depth bits
    ADD_ATTR(NSOpenGLPFADepthSize);
    ADD_ATTR(r_depthbits->integer ? r_depthbits->integer : 32);

    // Specify the number of stencil bits
    if (r_stencilbits->integer) {
        ADD_ATTR(NSOpenGLPFAStencilSize);
        ADD_ATTR(r_stencilbits->integer);
    }

    // Terminate the list
    ADD_ATTR(0);
    
    return pixelAttributes;
}

// Needs to be visible to Q3Controller.m.
void Sys_UpdateWindowMouseInputRect(void)
{		
    NSRect           windowRect, screenRect;
    NSScreen        *screen;

    // It appears we need to flip the coordinate system here.  This means we need
    // to know the size of the screen.
	if ( g_wv.plugin ) {
		screen = [[g_wv.plugin window] screen];
		screenRect = [screen frame];
		windowRect = [[g_wv.plugin window] frame];
	} else {
		screen = [glw_state.window screen];
		screenRect = [screen frame];
		windowRect = [glw_state.window frame];
	}
    windowRect.origin.y = screenRect.size.height - (windowRect.origin.y + windowRect.size.height);
    
    Sys_SetMouseInputRect(CGRectMake(windowRect.origin.x, windowRect.origin.y,
                                    windowRect.size.width, windowRect.size.height));
}									

// This is needed since CGReleaseAllDisplays() restores the gamma on the displays and we want to fade it up rather than just flickering all the displays
static void ReleaseAllDisplays()
{
    CGDisplayCount displayIndex;

    Com_Printf("Releasing displays\n");
    for (displayIndex = 0; displayIndex < glw_state.displayCount; displayIndex++) {
        CGDisplayRelease(glw_state.originalDisplayGammaTables[displayIndex].display);
    }
}

/*
=================
CreateGameWindow
=================
*/
static qboolean CreateGameWindow( qboolean isSecondTry )
{
    const char *windowed[] = { "Windowed", "Fullscreen" };
    int			current_mode;
    NSOpenGLPixelFormatAttribute *pixelAttributes;
    NSOpenGLPixelFormat *pixelFormat;
    CGDisplayErr err;
	int x, y, w, h;
	int refresh = 0;
	int colorDepth = 0;
	float aspect;
	
	x = 0;
	y = 0;
    // get mode info
    current_mode = r_mode->integer;
    glConfig.isFullscreen = (r_fullscreen->integer != 0);

	if ( glConfig.isFullscreen ) {
		const char *res = r_fsmode->string;
		int mode_id;
		
		w = atoi( COM_Parse( &res ) );
		h = atoi( COM_Parse( &res ) );
		aspect = (float)w / (float)h;
		
		sql_prepare( &com_db, "SELECT x,y FROM monitors SEARCH dev_name $");
		sql_bindtext( &com_db, 1, r_fsmonitor->string );
		if ( sql_step( &com_db ) ){
			x = sql_columnasint( &com_db, 0);
			y = sql_columnasint( &com_db, 1);
			glw_state.x = x;
			glw_state.y = y;
		}
		sql_done( &com_db );
		
		mode_id = -1;
		sql_prepare( &com_db, "SELECT id FROM fsmodes SEARCH dev_name $1 WHERE w=?2 AND h=?3" );
		sql_bindtext( &com_db, 1, r_fsmonitor->string );
		sql_bindint( &com_db, 2, w );
		sql_bindint( &com_db, 3, h );
		if ( sql_step( &com_db ) ) {
			mode_id = sql_columnasint( &com_db, 0 );
		}
		sql_done ( &com_db );
		
		sql_prepare( &com_db, "select bpp FROM fsmodes_ext SEARCH id ?1" );
		sql_bindint( &com_db, 1, mode_id );
		
		while ( sql_step( &com_db ) )
		{
			int bpp = sql_columnasint( &com_db, 0 );
			if ( r_colorbits->integer )
			{
				if ( bpp == r_colorbits->integer )
				{
					colorDepth = bpp;
					break;
				}
				if ( bpp > r_colorbits->integer ) 
				{
					if (colorDepth < r_colorbits->integer || bpp < colorDepth )
					{
						colorDepth = bpp;
					}
				} else if ( bpp > colorDepth ) {
					colorDepth = bpp;
				}
			} else if ( bpp > colorDepth )
				colorDepth = bpp;
		}
		sql_done( &com_db );
		
		sql_prepare( &com_db, "SELECT hz FROM fsmodes_ext SEARCH id ?1 WHERE bpp=?2" );
		sql_bindint( &com_db, 1, mode_id );
		sql_bindint( &com_db, 2, colorDepth );
		
		while( sql_step( &com_db ) )
		{
			int hz = sql_columnasint( &com_db, 0 );
			if( r_displayRefresh->integer )
			{
				if( hz == r_displayRefresh->integer )
				{
					//take an exact match
					refresh = hz;
					break;
				}
				
				if( hz > r_displayRefresh->integer )
				{
					if( refresh < r_displayRefresh->integer || hz < refresh )
						//if we must go over, take the smallest value that goes over
						refresh = hz;
				}
				else if( hz > refresh )
					refresh = hz;
			}
			else if( hz > refresh )
				//take the highest refresh rate
				refresh = hz;
		}
		sql_done( &com_db );
		
	} else {
		if( !R_GetModeInfo( &w, &h, &aspect, r_mode->integer ) )
		{
			w = 0;
			h = 0;
		}
	}
	
    glw_state.desktopMode = (NSDictionary *)CGDisplayCurrentMode(glw_state.display);
    if (!glw_state.desktopMode) {
		//reset fullscreen to 0 in case of error going full screen.
		Cvar_Set("r_fullscreen", "0");
        ri.Error(ERR_FATAL, "Could not get current graphics mode for display 0x%08x\n", glw_state.display);
    }
	
	if ( r_fullscreen->integer )
	{
		CGDirectDisplayID displayArray[MAX_DISPLAYS];
		CGDisplayCount numDisplays;
		
		CGGetActiveDisplayList( MAX_DISPLAYS, displayArray, &numDisplays);
		if ( r_fsmonitor->integer >= numDisplays ) {
			//r_fsmonitor is invalid.  Possibly set to a display that is no longer connected.
			// set back to 0
			Cvar_Set("r_fsmonitor", "0");
		}
		glw_state.gameMode = CGDisplayBestModeForParametersAndRefreshRate( displayArray[r_fsmonitor->integer], colorDepth, w, h, refresh, false);
		glw_state.display = displayArray[r_fsmonitor->integer];
		if (!glw_state.gameMode) {
            ri.Printf( PRINT_ALL, "Unable to find requested display mode.\n");
            return qfalse;
        }
		
        // Fade all screens to black
        //Sys_FadeScreens();
        /*
        err = Sys_CaptureActiveDisplays();
        if ( err != CGDisplayNoErr ) {
            CGDisplayRestoreColorSyncSettings();
            ri.Printf( PRINT_ALL, " Unable to capture displays err = %d\n", err );
            return qfalse;
        }*/ 
		
        err = CGDisplaySwitchToMode(glw_state.display, (CFDictionaryRef)glw_state.gameMode);
        if ( err != CGDisplayNoErr ) {
            CGDisplayRestoreColorSyncSettings();
            ReleaseAllDisplays();
            ri.Printf( PRINT_ALL, " Unable to set display mode, err = %d\n", err );
            return qfalse;
        }
		CGDisplayCapture( glw_state.display );
        
		glConfig.vidWidth = w;
		glConfig.vidHeight = h;
		glw_state.w = CGDisplayPixelsWide(glw_state.display);
		glw_state.h = CGDisplayPixelsHigh(glw_state.display);
		glConfig.displayFrequency = refresh;
	} else {
		ri.Printf( PRINT_ALL, "...setting mode %d:\n", current_mode );
		if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, current_mode ) )  {
			ri.Printf( PRINT_ALL, " invalid mode\n" );
			return qfalse;
		}
		glw_state.gameMode = glw_state.desktopMode;

		
	}
	

#if 0
    ri.Printf( PRINT_ALL, "... desktop mode %d = %dx%d %s\n", glw_state.desktopMode,
               glw_state.desktopDesc.width, glw_state.desktopDesc.height,
               depthStrings[glw_state.desktopDesc.depth]);
#endif

    if (g_wv.plugin) {

		ri.Cvar_Set( "vid_xpos", va("%f", [g_wv.plugin frame].origin.x ) );
		ri.Cvar_Set( "vid_ypos", va("%f", [g_wv.plugin frame].origin.y ) );

		glConfig.vidWidth = [g_wv.plugin frame].size.width;
		glConfig.vidHeight = [g_wv.plugin frame].size.height;
    }
    glConfig.deviceSupportsGamma = qfalse;
    glConfig.isFullscreen = r_fullscreen->integer;
    
    glConfig.xscale = glConfig.vidWidth / 640.0f;
    glConfig.yscale = glConfig.vidHeight / 480.0f;
    
    if( glConfig.vidWidth * 480 > glConfig.vidHeight * 640 )
    {
      // wide screen
      glConfig.xscale	= glConfig.yscale;
      glConfig.xbias	= ((float)glConfig.vidWidth - (640.0F * glConfig.xscale)) * 0.5F;
    }
    else
    {
      // no wide screen
      glConfig.xbias	= 0.0f;
    }	    
    ri.Printf( PRINT_ALL, " %d %d %s\n", glConfig.vidWidth, glConfig.vidHeight, windowed[glConfig.isFullscreen] );

    
    // Get the GL pixel format
    pixelAttributes = GetPixelAttributes();
    pixelFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes: pixelAttributes] autorelease];
    NSZoneFree(NULL, pixelAttributes);
    
    if (!pixelFormat) {
        CGDisplayRestoreColorSyncSettings();
        CGDisplaySwitchToMode(glw_state.display, (CFDictionaryRef)glw_state.desktopMode);
        ReleaseAllDisplays();
        ri.Printf( PRINT_ALL, " No pixel format found\n");
        return qfalse;
    }

    // Create a context with the desired pixel attributes
    OSX_SetGLContext([[NSOpenGLContext alloc] initWithFormat: pixelFormat shareContext: nil]);
    if (!OSX_GetNSGLContext()) {
        CGDisplayRestoreColorSyncSettings();
        CGDisplaySwitchToMode(glw_state.display, (CFDictionaryRef)glw_state.desktopMode);
        ReleaseAllDisplays();
        ri.Printf(PRINT_ALL, "... +[NSOpenGLContext createWithFormat:share:] failed.\n" );
        return qfalse;
    }

    if (!glConfig.isFullscreen) {
        cvar_t		*vid_xpos;
        cvar_t		*vid_ypos;
        NSRect           windowRect;
        
        vid_xpos = ri.Cvar_Get( "vid_xpos", "100", CVAR_ARCHIVE );
        vid_ypos = ri.Cvar_Get( "vid_ypos", "100", CVAR_ARCHIVE );

        // Create a window of the desired size
        windowRect.origin.x = abs(vid_xpos->integer);
        windowRect.origin.y = abs(vid_ypos->integer);
        windowRect.size.width = glConfig.vidWidth;
        windowRect.size.height = glConfig.vidHeight;
        if (g_wv.plugin == NULL) {
			glw_state.window = [[NSWindow alloc] initWithContentRect:windowRect
                                                      styleMask:(NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask)
                                                        backing:NSBackingStoreBuffered
                                                          defer:NO];
			[glw_state.window setTitle: @"SpaceTrader"];
			[glw_state.window setAcceptsMouseMovedEvents: YES];
			[glw_state.window orderFront: nil];
			[OSX_GetNSGLContext() setView: [glw_state.window contentView]];
		} else {
			[[g_wv.plugin window] setAcceptsMouseMovedEvents: YES];
			[OSX_GetNSGLContext() setView: g_wv.plugin];
		}
        
        // Sync input rect with where the window actually is...
        Sys_UpdateWindowMouseInputRect();
    } else {
        CGLError err;
        CGLSetCurrentContext(OSX_GetCGLContext());
        err = CGLSetFullScreen(OSX_GetCGLContext());
        if (err) {
            CGDisplayRestoreColorSyncSettings();
            CGDisplaySwitchToMode(glw_state.display, (CFDictionaryRef)glw_state.desktopMode);
            ReleaseAllDisplays();
            Com_Printf("CGLSetFullScreen -> %d (%s)\n", err, CGLErrorString(err));
            return qfalse;
        }
        Sys_SetMouseInputRect(CGDisplayBounds(glw_state.display));
    }


#ifndef USE_CGLMACROS
    // Make this the current context
    OSX_GLContextSetCurrent();
#endif

    // Store off the pixel format attributes that we actually got
    [pixelFormat getValues: (long *) &glConfig.colorBits forAttribute: NSOpenGLPFAColorSize forVirtualScreen: 0];
    [pixelFormat getValues: (long *) &glConfig.depthBits forAttribute: NSOpenGLPFADepthSize forVirtualScreen: 0];
    [pixelFormat getValues: (long *) &glConfig.stencilBits forAttribute: NSOpenGLPFAStencilSize forVirtualScreen: 0];

    glConfig.displayFrequency = [[glw_state.gameMode objectForKey: (id)kCGDisplayRefreshRate] intValue];
	[[NSOpenGLContext currentContext] setValues: (long *)&r_swapInterval->integer forParameter: NSOpenGLCPSwapInterval];
    if ( glewInit() != GLEW_OK )
    {
      ri.Error( ERR_DROP, "Failed loading GL extensions." );
      return RSERR_INVALID_MODE;
    }
    
    ri.Printf(PRINT_ALL, "ok\n" );

    return qtrue;
}

// This can be used to temporarily disassociate the GL context from the screen so that CoreGraphics can be used to draw to the screen.
void Sys_PauseGL ()
{
    if (!glw_state.glPauseCount) {
        glFinish (); // must do this to ensure the queue is complete
        
        // Have to call both to actually deallocate kernel resources and free the NSSurface
        CGLClearDrawable(OSX_GetCGLContext());
        [OSX_GetNSGLContext() clearDrawable];
    }
    glw_state.glPauseCount++;
}

// This can be used to reverse the pausing caused by Sys_PauseGL()
void Sys_ResumeGL ()
{
    if (glw_state.glPauseCount) {
        glw_state.glPauseCount--;
        if (!glw_state.glPauseCount) {
            if (!glConfig.isFullscreen) {
                [OSX_GetNSGLContext() setView: [glw_state.window contentView]];
            } else {
                CGLError err;
                
                err = CGLSetFullScreen(OSX_GetCGLContext());
                if (err)
                    Com_Printf("CGLSetFullScreen -> %d (%s)\n", err, CGLErrorString(err));
            }
        }
    }
}

void Glimp_PrintSystemInfo(){
}

/*
===================
GLimp_Init

Don't return unless OpenGL has been properly initialized
===================
*/

static void GLImp_Toggle_Renderer_f(void)
{
    ri.Cvar_Set("r_enablerender", r_enablerender->integer ? "0" : "1");
}

#ifdef OMNI_TIMER
static void GLImp_Dump_Stamp_List_f(void)
{
    OTStampListDumpToFile(glThreadStampList, "/tmp/gl_stamps");
}
#endif

static void GLImp_GetDisplayModes(){
	CGDirectDisplayID displayArray[MAX_DISPLAYS] ; 
	CGDisplayCount numDisplays; 
	CFArrayRef displayModeArray; 
	CFDictionaryRef displayMode; 
	CFNumberRef number; 
	int height, width, hz, bpp;
	int i,j, modes;

	sql_exec( &com_db, 
			  "CREATE TABLE monitors"
			  "("
				"dev_name STRING, "		// name of the monitor
				"x INTEGER, "
				"y INTEGER, "
				"w INTEGER, "
				"h INTEGER "
			  ");"
			  
			  "CREATE TABLE fsmodes"
			  "("
				"id INTEGER, "			// global unique for all modes
				"w INTEGER, "
				"h INTEGER, "
				"dev_name STRING"		// name of the monitor
			  ");"
			  
			  "CREATE TABLE fsmodes_ext"
			  "("
				"id INTEGER, "
				"hz INTEGER, "
				"bpp INTEGER "
			  ");"
			  );
	
	CGGetActiveDisplayList (MAX_DISPLAYS, displayArray, &numDisplays);
	
    // This is documented (in CGDirectDisplay.h) to be the main display.  We want to
    // return this instead of kCGDirectMainDisplay since this will allow us to compare
    // display IDs.
    glw_state.display = displayArray[ 0 ];
    	
	for ( i=0; i < numDisplays; i++ ) {
		CGRect dimensions;
		//CGDisplayCapture (lastDisplay); 
		displayModeArray = CGDisplayAvailableModes (displayArray[i]); 
		modes = CFArrayGetCount(displayModeArray);
		
		dimensions = CGDisplayBounds(displayArray[i]);
		
		sql_prepare ( &com_db, "INSERT INTO monitors(dev_name, x, y, w, h ) VALUES($,?,?,?,?);" );
		sql_bindtext( &com_db, 1, va("%d", i) );
		sql_bindint( &com_db, 2, dimensions.origin.x );
		sql_bindint( &com_db, 3, dimensions.origin.y );
		sql_bindint( &com_db, 4, dimensions.size.width );
		sql_bindint( &com_db, 5, dimensions.size.height );
		sql_step( &com_db );
		sql_done( &com_db );
		
		for (j=0; j < modes ; j++) {
			int monitor_id;
			displayMode = (CFDictionaryRef) CFArrayGetValueAtIndex (displayModeArray, j); 
			number = CFDictionaryGetValue(displayMode, kCGDisplayWidth);
			CFNumberGetValue( number, kCFNumberLongType, &width);
			number = CFDictionaryGetValue(displayMode, kCGDisplayHeight);
			CFNumberGetValue( number, kCFNumberLongType, &height);
			number = CFDictionaryGetValue(displayMode, kCGDisplayRefreshRate);
			CFNumberGetValue( number, kCFNumberLongType, &hz);
			number = CFDictionaryGetValue(displayMode, kCGDisplayBitsPerPixel);
			CFNumberGetValue( number, kCFNumberLongType, &bpp);

			sql_prepare( &com_db, "UPDATE OR INSERT fsmodes SET id=#,w=?1,h=?2,dev_name=?3 SEARCH dev_name ?3 WHERE w=?1 and h=?2");
			sql_bindint( &com_db, 1, width);
			sql_bindint( &com_db, 2, height);
			sql_bindtext( &com_db, 3, va("%d", i));
			sql_step( &com_db );
			sql_done( &com_db );
			
			sql_prepare( &com_db, "SELECT id FROM fsmodes SEARCH dev_name ?3 WHERE w=?1 AND h=?2");
			sql_bindint( &com_db, 1, width);
			sql_bindint( &com_db, 2, height);
			sql_bindtext( &com_db, 3, va("%d", i) );
			sql_step( &com_db );
			monitor_id = sql_columnasint( &com_db, 0 );
			sql_done( &com_db );

			sql_prepare( &com_db, "UPDATE OR INSERT fsmodes_ext SET id=?1,hz=?2,bpp=?3 SEARCH id ?1 WHERE hz=?2 and bpp=?3");
			sql_bindint( &com_db, 1, monitor_id);
			sql_bindint( &com_db, 2, hz);
			sql_bindint( &com_db, 3, bpp);
			sql_step( &com_db );
			sql_done( &com_db );
		}
	}
}

void GLimp_Init( void )
{
    cvar_t *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );
    char *buf;

#ifdef OMNI_TIMER
    glThreadStampList = OTStampListCreate(64);
	Cmd_AddCommand ("dump_stamp_list", GLImp_Dump_Stamp_List_f);
#endif

    ri.Printf( PRINT_ALL, "Initializing OpenGL subsystem\n" );
    ri.Printf( PRINT_ALL, "  Last renderer was '%s'\n", lastValidRenderer->string);
    memset( &glConfig, 0, sizeof( glConfig ) );

    
    r_allowSoftwareGL	= ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
    r_enablerender		= ri.Cvar_Get("r_enablerender", "1", 0 );
	r_fullscreen		= ri.Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_fsmode			= ri.Cvar_Get("r_fsmode", "640 480", CVAR_ARCHIVE | CVAR_LATCH );
	r_mode				= Cvar_Get( "r_mode", "4", CVAR_ARCHIVE | CVAR_LATCH );
	r_fsmonitor			= Cvar_Get( "r_fsmonitor", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorbits			= ri.Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );

	GLImp_GetDisplayModes();
	
	// We only allow changing the gamma if we are full screen
    glConfig.deviceSupportsGamma = (r_fullscreen->integer != 0);
    if (glConfig.deviceSupportsGamma) {
        Sys_StoreGammaTables();
    }


    if (Sys_QueryVideoMemory() == 0 && !r_allowSoftwareGL->integer) {
        ri.Error( ERR_FATAL, "Could not initialize OpenGL.  There does not appear to be an OpenGL-supported video card in your system.\n" );
    }
    
    if ( ! GLimp_SetMode(qfalse) ) {
        // fall back to the known-good mode
        ri.Cvar_Set( "r_fullscreen", "0" );
        ri.Cvar_Set( "r_mode", "3" );
        ri.Cvar_Set( "r_stereo", "0" );
        ri.Cvar_Set( "r_depthBits", "16" );
        ri.Cvar_Set( "r_colorBits", "16" );
        ri.Cvar_Set( "r_stencilBits", "0" );
        if ( GLimp_SetMode(qtrue) ) {
            ri.Printf( PRINT_ALL, "------------------\n" );
            return;
        }

        ri.Error( ERR_FATAL, "Could not initialize OpenGL\n" );
        return;
    }

    ri.Printf( PRINT_ALL, "------------------\n" );

    // get our config strings
    Q_strncpyz( glConfig.vendor_string, (const char *)glGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
    Q_strncpyz( glConfig.renderer_string, (const char *)glGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
    Q_strncpyz( glConfig.version_string, (const char *)glGetString (GL_VERSION), sizeof( glConfig.version_string ) );
    Q_strncpyz( glConfig.extensions_string, (const char *)glGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );

    //
    // chipset specific configuration
    //
    buf = malloc(strlen(glConfig.renderer_string) + 1);
    strcpy( buf, glConfig.renderer_string );
    Q_strlwr( buf );

    ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );
    free(buf);

    GLW_InitExtensions();
    
#ifndef USE_CGLMACROS
    if (!r_enablerender->integer)
        OSX_GLContextClearCurrent();
#endif
}


/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame (void)
{
    GLSTAMP("GLimp_EndFrame start", 0);
    
    //
    // swapinterval stuff
    //
    if ( r_swapInterval->modified ) {
        r_swapInterval->modified = qfalse;

        if ( !glConfig.stereoEnabled ) {	// why?
            [[NSOpenGLContext currentContext] setValues: (long *)&r_swapInterval->integer
            forParameter: NSOpenGLCPSwapInterval];
        }
    }

#if !defined(NDEBUG) && defined(QGL_CHECK_GL_ERRORS)
    QGLCheckError("GLimp_EndFrame");
#endif

    if (!glw_state.glPauseCount && !Sys_IsHidden) {
        glw_state.bufferSwapCount++;
        [OSX_GetNSGLContext() flushBuffer];
    }
    
    // Enable turning off GL at any point for performance testing
    if (OSX_GLContextIsCurrent() != r_enablerender->integer) {
        if (r_enablerender->integer) {
            Com_Printf("--- Enabling Renderer ---\n");
            OSX_GLContextSetCurrent();
        } else {
            Com_Printf("--- Disabling Renderer ---\n");
            OSX_GLContextClearCurrent();
        }
    }

    GLSTAMP("GLimp_EndFrame end", 0);
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/

static void _GLimp_RestoreOriginalVideoSettings()
{
    CGDisplayErr err;
    
    // CGDisplayCurrentMode lies because we've captured the display and thus we won't
    // get any notifications about what the current display mode really is.  For now,
    // we just always force it back to what mode we remember the desktop being in.
    if (glConfig.isFullscreen) {
        err = CGDisplaySwitchToMode(glw_state.display, (CFDictionaryRef)glw_state.desktopMode);
        if ( err != CGDisplayNoErr )
            ri.Printf( PRINT_ALL, " Unable to restore display mode!\n" );

        ReleaseAllDisplays();
    }
}

void GLimp_Shutdown( void )
{
    CGDisplayCount displayIndex;

    Com_Printf("----- Shutting down GL -----\n");

    Sys_FadeScreen(glw_state.display);
    
    if (OSX_GetNSGLContext()) {
#ifndef USE_CGLMACROS
        OSX_GLContextClearCurrent();
#endif
        // Have to call both to actually deallocate kernel resources and free the NSSurface
        CGLClearDrawable(OSX_GetCGLContext());
        [OSX_GetNSGLContext() clearDrawable];
        
        [OSX_GetNSGLContext() release];
        OSX_SetGLContext((id)nil);
    }

    _GLimp_RestoreOriginalVideoSettings();
    
    Sys_UnfadeScreens();

    // Restore the original gamma if needed.
    if (glConfig.deviceSupportsGamma) {
        Com_Printf("Restoring ColorSync settings\n");
        CGDisplayRestoreColorSyncSettings();
    }

    if (glw_state.window) {
        [glw_state.window release];
        glw_state.window = nil;
    }

    if (glw_state.log_fp) {
        fclose(glw_state.log_fp);
        glw_state.log_fp = 0;
    }

    for (displayIndex = 0; displayIndex < glw_state.displayCount; displayIndex++) {
        free(glw_state.originalDisplayGammaTables[displayIndex].red);
        free(glw_state.originalDisplayGammaTables[displayIndex].blue);
        free(glw_state.originalDisplayGammaTables[displayIndex].green);
    }
    free(glw_state.originalDisplayGammaTables);
    if (glw_state.tempTable.red) {
        free(glw_state.tempTable.red);
        free(glw_state.tempTable.blue);
        free(glw_state.tempTable.green);
    }
    if (glw_state.inGameTable.red) {
        free(glw_state.inGameTable.red);
        free(glw_state.inGameTable.blue);
        free(glw_state.inGameTable.green);
    }
    
    memset(&glConfig, 0, sizeof(glConfig));
    memset(&glState, 0, sizeof(glState));
    memset(&glw_state, 0, sizeof(glw_state));
    
#ifdef OMNI_TIMER    
    Cmd_RemoveCommand ("dump_stamp_list");
#endif    

    Com_Printf("----- Done shutting down GL -----\n");
}

/*
===============
GLimp_LogComment

===============
*/
#ifdef _DEBUG
void	GLimp_LogComment( int level, char *comment, ... ) {
}
#endif
/*
===============
GLimp_SetGamma

===============
*/
void GLimp_SetGamma(unsigned char red[256],
                    unsigned char green[256],
                    unsigned char blue[256])
{
    CGGammaValue redGamma[256], greenGamma[256], blueGamma[256];
    CGTableCount i;
    CGDisplayErr err;
    
    if (!glConfig.deviceSupportsGamma)
        return;
        
    for (i = 0; i < 256; i++) {
        redGamma[i]   = red[i]   / 255.0;
        greenGamma[i] = green[i] / 255.0;
        blueGamma[i]  = blue[i]  / 255.0;
    }
    
    err = CGSetDisplayTransferByTable(glw_state.display, 256, redGamma, greenGamma, blueGamma);
    if (err != CGDisplayNoErr) {
        Com_Printf("GLimp_SetGamma: CGSetDisplayTransferByByteTable returned %d.\n", err);
    }
    
    // Store the gamma table that we ended up using so we can reapply it later when unhiding or to work around the bug where if you leave the game sitting and the monitor sleeps, when it wakes, the gamma isn't reset.
    glw_state.inGameTable.display = glw_state.display;
    Sys_GetGammaTable(&glw_state.inGameTable);
}

qboolean GLimp_ChangeMode( int mode )
{
    qboolean result;
    int oldvalue = r_mode->integer;

    Com_Printf("*** GLimp_ChangeMode\n");
    r_mode->integer = mode;
    if (!(result = GLimp_SetMode(qfalse)))
        r_mode->integer = oldvalue;
    
   return result;
}

void Glimp_PrintOSVersion(void)
{
	struct utsname osinfo;
	if ( uname(&osinfo) == 0 ) {
		ri.Printf( PRINT_ALL, "%s", osinfo.version);
	} else {
		ri.Printf( PRINT_ALL, "Failed to get OS version info.\n");
	}
	
}

/*****************************************************************************/

void *qwglGetProcAddress(const char *name)
{
    NSSymbol symbol;
    char *symbolName;

    // Prepend a '_' for the Unix C symbol mangling convention
    symbolName = malloc(strlen(name) + 2);
    strcpy(symbolName + 1, name);
    symbolName[0] = '_';

    if (NSIsSymbolNameDefined(symbolName))
        symbol = NSLookupAndBindSymbol(symbolName);
    else
        symbol = NULL;
    
    free(symbolName);
    
    if (!symbol)
        // shouldn't happen ...
        return NULL;

    return NSAddressOfSymbol(symbol);
}

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void )
{
	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( GLEW_EXT_texture_env_add ) {
		glConfig.textureEnvAddAvailable = qtrue;
		ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
	} else {
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}        
}


#define MAX_RENDERER_INFO_COUNT 128

// Returns zero if there are no hardware renderers.  Otherwise, returns the max memory across all renderers (on the presumption that the screen that we'll use has the most memory).
static unsigned long Sys_QueryVideoMemory()
{
    CGLError err;
    CGLRendererInfoObj rendererInfo, rendererInfos[MAX_RENDERER_INFO_COUNT];
    long rendererInfoIndex, rendererInfoCount = MAX_RENDERER_INFO_COUNT;
    long rendererIndex, rendererCount;
    long maxVRAM = 0, vram;
    long accelerated;
    long rendererID;
    long totalRenderers = 0;
    
    err = CGLQueryRendererInfo(CGDisplayIDToOpenGLDisplayMask(glw_state.display), rendererInfos, &rendererInfoCount);
    if (err) {
        Com_Printf("CGLQueryRendererInfo -> %d\n", err);
        return vram;
    }
    
    //Com_Printf("rendererInfoCount = %d\n", rendererInfoCount);
    for (rendererInfoIndex = 0; rendererInfoIndex < rendererInfoCount && totalRenderers < rendererInfoCount; rendererInfoIndex++) {
        rendererInfo = rendererInfos[rendererInfoIndex];
        //Com_Printf("rendererInfo: 0x%08x\n", rendererInfo);
        

        err = CGLDescribeRenderer(rendererInfo, 0, kCGLRPRendererCount, &rendererCount);
        if (err) {
            Com_Printf("CGLDescribeRenderer(kCGLRPRendererID) -> %d\n", err);
            continue;
        }
        //Com_Printf("  rendererCount: %d\n", rendererCount);

        for (rendererIndex = 0; rendererIndex < rendererCount; rendererIndex++) {
            totalRenderers++;
            //Com_Printf("  rendererIndex: %d\n", rendererIndex);
            
            rendererID = 0xffffffff;
            err = CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPRendererID, &rendererID);
            if (err) {
                Com_Printf("CGLDescribeRenderer(kCGLRPRendererID) -> %d\n", err);
                continue;
            }
            //Com_Printf("    rendererID: 0x%08x\n", rendererID);
            
            accelerated = 0;
            err = CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPAccelerated, &accelerated);
            if (err) {
                Com_Printf("CGLDescribeRenderer(kCGLRPAccelerated) -> %d\n", err);
                continue;
            }
            //Com_Printf("    accelerated: %d\n", accelerated);
            if (!accelerated)
                continue;
            
            vram = 0;
            err = CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPVideoMemory, &vram);
            if (err) {
                Com_Printf("CGLDescribeRenderer -> %d\n", err);
                continue;
            }
            //Com_Printf("    vram: 0x%08x\n", vram);
            
            // presumably we'll be running on the best card, so we'll take the max of the vrams
            if (vram > maxVRAM)
                maxVRAM = vram;
        }
        
#if 0
        err = CGLDestroyRendererInfo(rendererInfo);
        if (err) {
            Com_Printf("CGLDestroyRendererInfo -> %d\n", err);
        }
#endif
    }

    return maxVRAM;
}


// We will set the Sys_IsHidden global to cause input to be handle differently (we'll just let NSApp handle events in this case).  We also will unbind the GL context and restore the video mode.
qboolean Sys_Hide()
{
    if (Sys_IsHidden)
        // Eh?
        return qfalse;
    
    if (!r_fullscreen->integer)
        // We only support hiding in fullscreen mode right now
        return qfalse;
    
    Sys_IsHidden = qtrue;

    // Don't need to store the current gamma since we always keep it around in glw_state.inGameTable.

    Sys_FadeScreen(glw_state.display);

    // Disassociate the GL context from the screen
    // Have to call both to actually deallocate kernel resources and free the NSSurface
    CGLClearDrawable(OSX_GetCGLContext());
    [OSX_GetNSGLContext() clearDrawable];
    
    // Restore the original video mode
    _GLimp_RestoreOriginalVideoSettings();

    // Restore the original gamma if needed.
    if (glConfig.deviceSupportsGamma) {
        CGDisplayRestoreColorSyncSettings();
    }

    // Release the screen(s)
    ReleaseAllDisplays();
    
    Sys_UnfadeScreens();
    
    // Shut down the input system so the mouse and keyboard settings are restore to normal
    Sys_ShutdownInput();
    
    // Hide the application so that when the user clicks on our app icon, we'll get an unhide notification
    [NSApp hide: nil];
    
    return qtrue;
}

static CGDisplayErr Sys_CaptureActiveDisplays(void)
{
    CGDisplayErr err;
    CGDisplayCount displayIndex;
    for (displayIndex = 0; displayIndex < glw_state.displayCount; displayIndex++) {
	const glwgamma_t *table;
	table = &glw_state.originalDisplayGammaTables[displayIndex];
	err = CGDisplayCapture(table->display);
	if (err != CGDisplayNoErr)
	    return err;
    }
    return CGDisplayNoErr;
}

qboolean Sys_Unhide()
{
    CGDisplayErr err;
    CGLError glErr;
    
    if (!Sys_IsHidden)
        // Eh?
        return qfalse;
        
    //Sys_FadeScreens();

    // Capture the screen(s)
    //err = Sys_CaptureActiveDisplays();
	err = CGDisplayCapture(glw_state.display);
    if (err != CGDisplayNoErr) {
        Sys_UnfadeScreens();
        ri.Printf( PRINT_ALL, "Unhide failed -- cannot capture the display again.\n" );
        return qfalse;
    }
    
    // Restore the game mode
    err = CGDisplaySwitchToMode(glw_state.display, (CFDictionaryRef)glw_state.gameMode);
    if ( err != CGDisplayNoErr ) {
        ReleaseAllDisplays();
        Sys_UnfadeScreens();
        ri.Printf( PRINT_ALL, "Unhide failed -- Unable to set display mode\n" );
        return qfalse;
    }

    // Reassociate the GL context and the screen
    glErr = CGLSetFullScreen(OSX_GetCGLContext());
    if (err) {
        ReleaseAllDisplays();
        Sys_UnfadeScreens();
        ri.Printf( PRINT_ALL, "Unhide failed: CGLSetFullScreen -> %d (%s)\n", err, CGLErrorString(err));
        return qfalse;
    }

    // Restore the current context
    [OSX_GetNSGLContext() makeCurrentContext];
    
    // Restore the gamma that the game had set
    Sys_UnfadeScreen(glw_state.display, &glw_state.inGameTable);
    
    // Restore the input system (last so if something goes wrong we don't eat the mouse)
    Sys_InitInput();
    
    Sys_IsHidden = qfalse;
    return qtrue;
}


