#ifdef _MSC_VER
#pragma warning(disable : 4206)
#endif
#ifdef USE_SDL_VIDEO

/*
 * SDL implementation for Quake 3: Arena's GPL source release.
 *
 * I wrote such a beast originally for Loki's port of Heavy Metal: FAKK2,
 *  and then wrote it again for the Linux client of Medal of Honor: Allied
 *  Assault. Third time's a charm, so I'm rewriting this once more for the
 *  GPL release of Quake 3.
 *
 * Written by Ryan C. Gordon (icculus@icculus.org). Please refer to
 *    http://icculus.org/quake3/ for the latest version of this code.
 *
 *  Patches and comments are welcome at the above address.
 *
 * I cut-and-pasted this from linux_glimp.c, and moved it to SDL line-by-line.
 *  There is probably some cruft that could be removed here.
 *
 * You should define USE_SDL=1 and then add this to the makefile.
 *  USE_SDL will disable the X11 target.
 */

/*
Original copyright on Q3A sources:
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
/*
** GLW_IMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
** GLimp_SetGamma
**
*/

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#ifdef SMP
#include "SDL_thread.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <dlfcn.h>
#endif

#include "../render-base/tr_local.h"

#ifndef _MSC_VER
#include "linux_local.h" // bk001130

#include "unix_glw.h"
#else
#include "../win32/glw_win.h"
#include "../win32/win_local.h"
#endif

/* Just hack it for now. */
#ifdef MACOS_X
#include <OpenGL/OpenGL.h>
typedef CGLContextObj QGLContext;
#define GLimp_GetCurrentContext() CGLGetCurrentContext()
#define GLimp_SetCurrentContext(ctx) CGLSetCurrentContext(ctx)
#else
typedef void *QGLContext;
#define GLimp_GetCurrentContext() (NULL)
#define GLimp_SetCurrentContext(ctx)
#endif

static QGLContext opengl_context;

#define	WINDOW_CLASS_NAME	"SpaceTrader"
#define	WINDOW_CLASS_NAME_BRIEF	"st"


//not implemented this side yet.  Force it to false restores old behavior
bool glimp_suspendRender = false;

//#define KBD_DBG

typedef enum
{
    RSERR_OK,

    RSERR_INVALID_FULLSCREEN,
    RSERR_INVALID_MODE,

    RSERR_UNKNOWN
} rserr_t;

glwstate_t glw_state;

cvar_t *in_nograb; // this is strictly for developers

cvar_t  *r_allowSoftwareGL;   // don't abort out if the pixelformat claims software
cvar_t  *r_previousglDriver;

void IN_StartupJoystick( void );
void IN_JoyMove( void );

qboolean GLimp_sdl_init_video(void)
{
	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{

		ri.Printf( PRINT_ALL, "Calling SDL_Init(SDL_INIT_VIDEO)...\n");
		if (SDL_Init(SDL_INIT_VIDEO) == -1)
		{
			ri.Printf( PRINT_ALL, "SDL_Init(SDL_INIT_VIDEO) failed: %s\n", SDL_GetError());
			return qfalse;
		}
		ri.Printf( PRINT_ALL, "SDL_Init(SDL_INIT_VIDEO) passed.\n");
	}

	return qtrue;
}


// NOTE TTimo for the tty console input, we didn't rely on those ..
//   it's not very surprising actually cause they are not used otherwise
void KBD_Init(void)
{}

void KBD_Close(void)
{}

/*****************************************************************************/

/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	// NOTE TTimo we get the gamma value from cvar, because we can't work with the s_gammatable
	//   the API wasn't changed to avoid breaking other OSes
	float g;

	if ( r_ignorehwgamma->integer )
		return;

	g  = ri.Cvar_Get("r_gamma", "1.0", 0)->value;
	SDL_SetGamma(g, g, g);
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
void GLimp_Shutdown( void )
{
//	R_BloomKill();
//	R_CgKill();
	R_StateKill();
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	((WinVars_t *)ri.PlatformGetVars())->screen = NULL;

	memset(&glConfig, 0, sizeof(glConfig));

}

/*
** GLimp_LogComment
*/
#ifdef _DEBUG
void GLimp_LogComment( int level, char *comment, ... )
{
	if ( glw_state.log_fp )
	{
		fprintf( glw_state.log_fp, "%s", comment );
	}
}
#endif
/*
** GLW_StartDriverAndSetMode
*/
// bk001204 - prototype needed
static rserr_t GLW_SetMode( const char *drivername, int mode, qboolean fullscreen );
static qboolean GLW_StartDriverAndSetMode( const char *drivername,
        int mode,
        qboolean fullscreen )
{
	rserr_t err;

	if (GLimp_sdl_init_video() == qfalse)
		return qfalse;

	in_nograb = ri.Cvar_Get("in_nograb", "0", 0);

	if (fullscreen && in_nograb->integer)
	{
		ri.Printf( PRINT_ALL, "Fullscreen should not be used with in_nograb 1\n");
		/*ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;*/
	}

	err = GLW_SetMode( drivername, mode, fullscreen );

	switch ( err )
	{
	case RSERR_INVALID_FULLSCREEN:
		ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
		return qfalse;
	case RSERR_INVALID_MODE:
		ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

/*
** GLW_SetMode
*/
static rserr_t GLW_SetMode(const char *drivername, int mode, qboolean fullscreen)
{
	const char *glstring;	// bk001130 - from cvs1.17 (mkv)
	int sdlcolorbits = 4;
	int colorbits, depthbits, stencilbits;
	int tcolorbits, tdepthbits, tstencilbits;
	int fsaabits;
	int fullNoBorder=qfalse;
	int i = 0;
	SDL_Surface *vidscreen = NULL;
	Uint32 flags;

	ri.Printf(PRINT_ALL, "Initializing OpenGL display\n");

	ri.Printf(PRINT_ALL, "...setting mode %d:", mode);

	if (mode < -1 )
	{
		mode *=-1;
		fullNoBorder = qtrue;
	}

	if(!R_GetModeInfo(&glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode))
	{
		ri.Printf(PRINT_ALL, " invalid mode\n");
		return RSERR_INVALID_MODE;
	}
	if (g_wv.hWnd) {
		int width, height, x_return, y_return, border_width_return, depth_return;
		Window w;
		
		g_wv.display = XOpenDisplay(NULL);
		
		
		XGetGeometry( g_wv.display, g_wv.hWnd, &w, &x_return, &y_return, &width, &height, &border_width_return, &depth_return);
		glConfig.vidWidth = width;
		glConfig.vidHeight = height;
	}

	glConfig.xscale = glConfig.vidWidth / 640.0f;
	glConfig.yscale = glConfig.vidHeight / 480.0f;

	if ( glConfig.vidWidth * 480 > glConfig.vidHeight * 640 ) {
		// wide screen
		glConfig.xscale	= glConfig.yscale;
		glConfig.xbias	= ((float)glConfig.vidWidth - (640.0f * glConfig.xscale)) * 0.5f;
	}
	else {
		// no wide screen
		glConfig.xbias	= 0.0f;
	}

	ri.Printf(PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

	flags = SDL_OPENGL;
	if(fullscreen)
	{
		if (fullNoBorder)
		{
			flags |= SDL_NOFRAME;
		}
		else
		{
			flags |= SDL_FULLSCREEN;
		}
	}

	if(!r_colorbits->value)
		colorbits = 24;
	else
		colorbits = r_colorbits->value;

	if(!r_depthbits->value)
		depthbits = 24;
	else
		depthbits = r_depthbits->value;
	stencilbits = r_stencilbits->value;

	for (i = 0; i < 16; i++)
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
			case 2:
				if(colorbits == 24)
					colorbits = 16;
				break;
			case 1:
				if(depthbits == 24)
					depthbits = 16;
				else if(depthbits == 16)
					depthbits = 8;
			case 3:
				if(stencilbits == 24)
					stencilbits = 16;
				else if(stencilbits == 16)
					stencilbits = 8;
			}
		}

		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;

		if((i % 4) == 3)
		{	// reduce colorbits
			if(tcolorbits == 24)
				tcolorbits = 16;
		}

		if((i % 4) == 2)
		{	// reduce depthbits
			if(tdepthbits == 24)
				tdepthbits = 16;
			else if(tdepthbits == 16)
				tdepthbits = 8;
		}

		if((i % 4) == 1)
		{	// reduce stencilbits
			if(tstencilbits == 24)
				tstencilbits = 16;
			else if(tstencilbits == 16)
				tstencilbits = 8;
			else
				tstencilbits = 0;
		}

		sdlcolorbits = 4;
		if(tcolorbits == 24)
			sdlcolorbits = 8;

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, sdlcolorbits);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, sdlcolorbits);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, sdlcolorbits);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, tdepthbits);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, tstencilbits);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		if( r_fsaa->integer )
		{
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, r_fsaa->integer);
		}

		SDL_WM_SetCaption(WINDOW_CLASS_NAME, WINDOW_CLASS_NAME_BRIEF);
		SDL_ShowCursor(0);
		SDL_EnableUNICODE(1);
		//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		((WinVars_t *)ri.PlatformGetVars())->sdlrepeatenabled = qtrue;

		if((vidscreen = SDL_SetVideoMode(glConfig.vidWidth, glConfig.vidHeight, colorbits, flags)) == NULL)
		{
			fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
			continue;
		}
		//Safe to get this info now.
		SDL_GetWMInfo(&g_wv.wmInfo);

		if( glewInit() == GLEW_OK )
		{
			R_StateKill();
			R_StateInit();
			//Setup CG
//			R_CgKill(); //sanity check
//			R_CgInit();
//			R_BloomInit();
			ri.Printf(PRINT_ALL, "GLEW Loaded\n");
		}
		if (g_wv.hWnd) {
			int result;
			/*for ( ; ; ) {
				result = XGrabPointer(g_wv.wmInfo.info.x11.display, g_wv.wmInfo.info.x11.window, True, 0, GrabModeAsync, GrabModeAsync, g_wv.wmInfo.info.x11.window, None, CurrentTime);
				if ( result == GrabSuccess ) {
					break;
				}
				SDL_Delay(100);
			}*/
			
			//asm("int $3");
			//result = XGrabKeyboard(g_wv.wmInfo.info.x11.display, g_wv.wmInfo.info.x11.window, True, GrabModeAsync, GrabModeAsync, CurrentTime);
			
			//XRaiseWindow(g_wv.wmInfo.info.x11.display, g_wv.wmInfo.info.x11.window);
			SDL_WM_GrabInput(SDL_GRAB_ON);
			//XReparentWindow(g_wv.wmInfo.info.x11.display, g_wv.wmInfo.info.x11.window, (Window)g_wv.hWnd, 0, 0);
			//XReparentWindow(g_wv.wmInfo
		}
		
		
		opengl_context = GLimp_GetCurrentContext();
//		SDL_VERSION(&g_wv.wmInfo.version);
//		SDL_GetWMInfo(&g_wv.wmInfo);
		ri.Printf(PRINT_ALL, "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n",
		          sdlcolorbits, sdlcolorbits, sdlcolorbits, tdepthbits, tstencilbits);

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}

	if(r_fsaa->integer)
	{
		SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &fsaabits);
		ri.Printf(PRINT_ALL, "SDL_GL_MULTISAMPLEBUFFERS: requested 1, got %d\n", fsaabits);
		SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &fsaabits);
		ri.Printf(PRINT_ALL, "SDL_GL_MULTISAMPLESAMPLES: requested %d, got %d\n", r_fsaa->integer, fsaabits);
	}
	if(!vidscreen)
	{
		ri.Printf(PRINT_ALL, "Couldn't get a visual\n");
		return RSERR_INVALID_MODE;
	}

	((WinVars_t *)ri.PlatformGetVars())->screen = vidscreen;

	// bk001130 - from cvs1.17 (mkv)
	glstring = (char *) glGetString(GL_RENDERER);
	ri.Printf(PRINT_ALL, "GL_RENDERER: %s\n", glstring);

	// bk010122 - new software token (Indirect)
	if(!Q_stricmp(glstring, "Mesa X11")
	        || !Q_stricmp(glstring, "Mesa GLX Indirect"))
	{
		if(!r_allowSoftwareGL->integer)
		{
			ri.Printf(PRINT_ALL, "\n\n***********************************************************\n");
			ri.Printf(PRINT_ALL, " You are using software Mesa (no hardware acceleration)!   \n");
			ri.Printf(PRINT_ALL, " Driver DLL used: %s\n", drivername);
			ri.Printf(PRINT_ALL, " If this is intentional, add\n");
			ri.Printf(PRINT_ALL, "       \"+set r_allowSoftwareGL 1\"\n");
			ri.Printf(PRINT_ALL, " to the command line when starting the game.\n");
			ri.Printf(PRINT_ALL, "***********************************************************\n");
			GLimp_Shutdown();
			return RSERR_INVALID_MODE;
		}
		else
		{
			ri.Printf(PRINT_ALL, "...using software Mesa (r_allowSoftwareGL==1).\n");
		}
	}

	return RSERR_OK;
}

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void )
{
	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( GLEW_EXT_texture_env_add )
	{
		glConfig.textureEnvAddAvailable = qtrue;
		ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}
}

static void GLW_InitGamma( void )
{
	glConfig.deviceSupportsGamma = qtrue;
}

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that that attempts to load and use 
** a specific OpenGL DLL.
*/
static qboolean GLW_LoadOpenGL( const char *name )
{
	qboolean fullscreen;

	ri.Printf( PRINT_ALL, "...loading %s:\n", name );

	// disable the 3Dfx splash screen and set gamma
	// we do this all the time, but it shouldn't hurt anything
	// on non-3Dfx stuff
	putenv("FX_GLIDE_NO_SPLASH=0");

	// Mesa VooDoo hacks
	putenv("MESA_GLX_FX=fullscreen\n");

	// load the QGL layer
	fullscreen = r_fullscreen->integer?qtrue:qfalse;

	// create the window and set up the context
	if ( !GLW_StartDriverAndSetMode( name, r_fullscreen->integer ? r_fsmode->integer : r_mode->integer, fullscreen ) )
	{
		if (r_mode->integer != 3)
		{
			if ( !GLW_StartDriverAndSetMode( name, 3, fullscreen ) )
			{
				goto fail;
			}
		}
		else
			goto fail;
	}

	return qtrue;
fail:

	return qfalse;
}


/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  
*/
void GLimp_Init( void )
{
	cvar_t *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );

	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );

	r_previousglDriver = ri.Cvar_Get( "r_previousglDriver", "", CVAR_ROM );

	// Hack here so that if the UI
	if ( *r_previousglDriver->string )
	{
		// The UI changed it on us, hack it back
		// This means the renderer can't be changed on the fly
		ri.Cvar_Set( "r_glDriver", r_previousglDriver->string );
	}

	//
	// load and initialize the specific OpenGL driver
	//
	GLW_LoadOpenGL( OPENGL_DRIVER_NAME );

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, (const char*)glGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (const char*)glGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (const char*)glGetString (GL_VERSION), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (const char*)glGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );

	//
	// NOTE: if changing cvars, do it within this block.  This allows them
	// to be overridden when testing driver fixes, etc. but only sets
	// them to their default state when the hardware is first installed/run.
	//
	if ( Q_stricmp( lastValidRenderer->string, glConfig.renderer_string ) )
	{
		ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR" );
		ri.Cvar_Set( "r_picmip", "1" );
	}

	ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

	// initialize extensions
	GLW_InitExtensions();
	GLW_InitGamma();

	return;
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
	SDL_GL_SwapBuffers();

	if( r_fullscreen->modified )
	{
		qboolean    fullscreen;
		qboolean    sdlToggled = qfalse;
		SDL_Surface *s = SDL_GetVideoSurface( );

		if( s )
		{
			// Find out the current state
			if( s->flags & SDL_FULLSCREEN )
				fullscreen = qtrue;
			else
				fullscreen = qfalse;

			// Is the state we want different from the current state?
			if( !!r_fullscreen->integer != fullscreen )
				sdlToggled = SDL_WM_ToggleFullScreen( s ) ? qtrue : qfalse;
			else
				sdlToggled = qtrue;
		}

		r_fullscreen->modified = qfalse;
	}
}

#ifdef SMP
/*
===========================================================
 
SMP acceleration
 
===========================================================
*/

/*
 * I have no idea if this will even work...most platforms don't offer
 *  thread-safe OpenGL libraries, and it looks like the original Linux
 *  code counted on each thread claiming the GL context with glXMakeCurrent(),
 *  which you can't currently do in SDL. We'll just have to hope for the best.
 */

static SDL_mutex *smpMutex = NULL;
static SDL_cond *renderCommandsEvent = NULL;
static SDL_cond *renderCompletedEvent = NULL;
static void (*glimpRenderThread)( void ) = NULL;
static SDL_Thread *renderThread = NULL;

static void GLimp_ShutdownRenderThread(void)
{
	if (smpMutex != NULL)
	{
		SDL_DestroyMutex(smpMutex);
		smpMutex = NULL;
	}

	if (renderCommandsEvent != NULL)
	{
		SDL_DestroyCond(renderCommandsEvent);
		renderCommandsEvent = NULL;
	}

	if (renderCompletedEvent != NULL)
	{
		SDL_DestroyCond(renderCompletedEvent);
		renderCompletedEvent = NULL;
	}

	glimpRenderThread = NULL;
}

static int GLimp_RenderThreadWrapper( void *arg )
{
	Com_Printf( "Render thread starting\n" );

	glimpRenderThread();

	GLimp_SetCurrentContext(NULL);

	Com_Printf( "Render thread terminating\n" );

	return 0;
}

qboolean GLimp_SpawnRenderThread( void (*function)( void ) )
{
	static qboolean warned = qfalse;
	if (!warned)
	{
		Com_Printf("WARNING: You enable r_smp at your own risk!\n");
		warned = qtrue;
	}

#ifndef MACOS_X
	return qfalse;  /* better safe than sorry for now. */
#endif

	if (renderThread != NULL)  /* hopefully just a zombie at this point... */
	{
		Com_Printf("Already a render thread? Trying to clean it up...\n");
		SDL_WaitThread(renderThread, NULL);
		renderThread = NULL;
		GLimp_ShutdownRenderThread();
	}

	smpMutex = SDL_CreateMutex();
	if (smpMutex == NULL)
	{
		Com_Printf( "smpMutex creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}

	renderCommandsEvent = SDL_CreateCond();
	if (renderCommandsEvent == NULL)
	{
		Com_Printf( "renderCommandsEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}

	renderCompletedEvent = SDL_CreateCond();
	if (renderCompletedEvent == NULL)
	{
		Com_Printf( "renderCompletedEvent creation failed: %s\n", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}

	glimpRenderThread = function;
	renderThread = SDL_CreateThread(GLimp_RenderThreadWrapper, NULL);
	if ( renderThread == NULL )
	{
		ri.Printf( PRINT_ALL, "SDL_CreateThread() returned %s", SDL_GetError() );
		GLimp_ShutdownRenderThread();
		return qfalse;
	}
	else
	{
		// !!! FIXME: No detach API available in SDL!
		//ret = pthread_detach( renderThread );
		//if ( ret ) {
		//ri.Printf( PRINT_ALL, "pthread_detach returned %d: %s", ret, strerror( ret ) );
		//}
	}

	return qtrue;
}

static volatile void    *smpData = NULL;
static volatile qboolean smpDataReady;

void *GLimp_RendererSleep( void )
{
	void  *data = NULL;

	GLimp_SetCurrentContext(NULL);

	SDL_LockMutex(smpMutex);
	{
		smpData = NULL;
		smpDataReady = qfalse;

		// after this, the front end can exit GLimp_FrontEndSleep
		SDL_CondSignal(renderCompletedEvent);

		while ( !smpDataReady )
		{
			SDL_CondWait(renderCommandsEvent, smpMutex);
		}

		data = (void *)smpData;
	}
	SDL_UnlockMutex(smpMutex);

	GLimp_SetCurrentContext(opengl_context);

	return data;
}

void GLimp_FrontEndSleep( void )
{
	SDL_LockMutex(smpMutex);
	{
		while ( smpData )
		{
			SDL_CondWait(renderCompletedEvent, smpMutex);
		}
	}
	SDL_UnlockMutex(smpMutex);

	GLimp_SetCurrentContext(opengl_context);
}

void GLimp_WakeRenderer( void *data )
{
	GLimp_SetCurrentContext(NULL);

	SDL_LockMutex(smpMutex);
	{
		assert( smpData == NULL );
		smpData = data;
		smpDataReady = qtrue;

		// after this, the renderer can continue through GLimp_RendererSleep
		SDL_CondSignal(renderCommandsEvent);
	}
	SDL_UnlockMutex(smpMutex);
}

#else

void GLimp_RenderThreadWrapper( void *stub ) {}
qboolean GLimp_SpawnRenderThread( void (*function)( void ) )
{
	ri.Printf( PRINT_WARNING, "ERROR: SMP support was disabled at compile time\n");
	return qfalse;
}
void *GLimp_RendererSleep( void )
{
	return NULL;
}
void GLimp_FrontEndSleep( void ) {}
void GLimp_WakeRenderer( void *data ) {}

#endif


// bk001130 - cvs1.17 joystick code (mkv) was here, no linux_joystick.c



// (moved this back in here from linux_joystick.c, so it's all in one place...
//   --ryan.


#endif  // USE_SDL_VIDEO

// end sdl_glimp.c ...
