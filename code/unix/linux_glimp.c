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
#ifndef USE_SDL_VIDEO


#include <termios.h>
#include <sys/ioctl.h>
#ifdef __linux__
 #include <sys/stat.h>
 #include <sys/vt.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/utsname.h>

// bk001206 - from my Heretic2 by way of Ryan's Fakk2
// Needed for the new X11_PendingInput() function.
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "../renderer/tr_local.h"
#include "../client/client.h"
#include "linux_local.h" // bk001130

#include "unix_glw.h"

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

// don't use DGA anymore
//#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>

extern sqlInfo_t com_db;

#ifdef _XF86DGA_H_
#define HAVE_XF86DGA
#endif

#ifdef _XF86VIDMODE_H_
#define HAVE_XF86VIDMODE
#endif

#define	WINDOW_CLASS_NAME	"Space Trader"

typedef enum
{
    RSERR_OK,

    RSERR_INVALID_FULLSCREEN,
    RSERR_INVALID_MODE,

    RSERR_UNKNOWN
} rserr_t;

glwstate_t glw_state;

static Display *dpy = NULL;
static int scrnum;
static Window win = 0;
static GLXContext ctx = NULL;

// bk001206 - not needed anymore
// static qboolean autorepeaton = qtrue;

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
		    PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask | FocusChangeMask )

static qboolean mouse_avail;
static qboolean mouse_active = qfalse;
static int mwx, mwy;
static int mx = 0, my = 0;

// Time mouse was reset, we ignore the first 50ms of the mouse to allow settling of events
static int mouseResetTime = 0;
#define MOUSE_RESET_DELAY 50

static cvar_t *in_mouse;
static cvar_t *in_dgamouse; // user pref for dga mouse
cvar_t *in_subframe;
cvar_t *in_nograb; // this is strictly for developers

// bk001130 - from cvs1.17 (mkv), but not static
cvar_t *in_joystick = NULL;
cvar_t *in_joystickDebug = NULL;
cvar_t *joy_threshold = NULL;

cvar_t *r_allowSoftwareGL;   // don't abort out if the pixelformat claims software
cvar_t *r_previousglDriver;

qboolean vidmode_ext = qfalse;
#ifdef HAVE_XF86VIDMODE
static int vidmode_MajorVersion = 0, vidmode_MinorVersion = 0; // major and minor of XF86VidExtensions

// gamma value of the X display before we start playing with it
static XF86VidModeGamma vidmode_InitialGamma;
#endif /* HAVE_XF86VIDMODE */

static int win_x, win_y;

#ifdef HAVE_XF86VIDMODE
static XF86VidModeModeInfo **vidmodes;
#endif /* HAVE_XF86VIDMODE */ 
//static int default_dotclock_vidmode; // bk001204 - unused
static int num_vidmodes;
static qboolean vidmode_active = qfalse;

static int mouse_accel_numerator;
static int mouse_accel_denominator;
static int mouse_threshold;

//not implemented this side yet.  Force it to false restores old behavior
bool glimp_suspendRender = false;

qboolean app_active;

/*****************************************************************************
** KEYBOARD
** NOTE TTimo the keyboard handling is done with KeySyms
**   that means relying on the keyboard mapping provided by X
**   in-game it would probably be better to use KeyCode (i.e. hardware key codes)
**   you would still need the KeySyms in some cases, such as for the console and all entry textboxes
**     (cause there's nothing worse than a qwerty mapping on a french keyboard)
**
** you can turn on some debugging and verbose of the keyboard code with #define KBD_DBG
******************************************************************************/

//#define KBD_DBG

static char *XLateKey( XKeyEvent *ev, int *key )
{
	static char buf[ 64 ];
	KeySym keysym;
	int XLookupRet;

	*key = 0;

	XLookupRet = XLookupString( ev, buf, sizeof buf, &keysym, 0 );
#ifdef KBD_DBG
	ri.Printf( PRINT_ALL, "XLookupString ret: %d buf: %s keysym: %x\n", XLookupRet, buf, keysym );
#endif

	switch ( keysym )
	{
			case XK_KP_Page_Up:
			case XK_KP_9: *key = K_KP_PGUP; break;
			case XK_Page_Up: *key = K_PGUP; break;

			case XK_KP_Page_Down:
			case XK_KP_3: *key = K_KP_PGDN; break;
			case XK_Page_Down: *key = K_PGDN; break;

			case XK_KP_Home: *key = K_KP_HOME; break;
			case XK_KP_7: *key = K_KP_HOME; break;
			case XK_Home: *key = K_HOME; break;

			case XK_KP_End:
			case XK_KP_1: *key = K_KP_END; break;
			case XK_End: *key = K_END; break;

			case XK_KP_Left: *key = K_KP_LEFTARROW; break;
			case XK_KP_4: *key = K_KP_LEFTARROW; break;
			case XK_Left: *key = K_LEFTARROW; break;

			case XK_KP_Right: *key = K_KP_RIGHTARROW; break;
			case XK_KP_6: *key = K_KP_RIGHTARROW; break;
			case XK_Right: *key = K_RIGHTARROW; break;

			case XK_KP_Down:
			case XK_KP_2: *key = K_KP_DOWNARROW; break;
			case XK_Down: *key = K_DOWNARROW; break;

			case XK_KP_Up:
			case XK_KP_8: *key = K_KP_UPARROW; break;
			case XK_Up: *key = K_UPARROW; break;

			case XK_Escape: *key = K_ESCAPE; break;

			case XK_KP_Enter: *key = K_KP_ENTER; break;
			case XK_Return: *key = K_ENTER; break;

			case XK_Tab: *key = K_TAB; break;

			case XK_F1: *key = K_F1; break;

			case XK_F2: *key = K_F2; break;

			case XK_F3: *key = K_F3; break;

			case XK_F4: *key = K_F4; break;

			case XK_F5: *key = K_F5; break;

			case XK_F6: *key = K_F6; break;

			case XK_F7: *key = K_F7; break;

			case XK_F8: *key = K_F8; break;

			case XK_F9: *key = K_F9; break;

			case XK_F10: *key = K_F10; break;

			case XK_F11: *key = K_F11; break;

			case XK_F12: *key = K_F12; break;

			// bk001206 - from Ryan's Fakk2
			//case XK_BackSpace: *key = 8; break; // ctrl-h
			case XK_BackSpace: *key = K_BACKSPACE; break; // ctrl-h

			case XK_KP_Delete:
			case XK_KP_Decimal: *key = K_KP_DEL; break;
			case XK_Delete: *key = K_DEL; break;

			case XK_Pause: *key = K_PAUSE; break;

			case XK_Shift_L:
			case XK_Shift_R: *key = K_SHIFT; break;

			case XK_Execute:
			case XK_Control_L:
			case XK_Control_R: *key = K_CTRL; break;

			case XK_Alt_L:
			case XK_Meta_L:
			case XK_Alt_R:
			case XK_Meta_R: *key = K_ALT; break;

			case XK_KP_Begin: *key = K_KP_5; break;

			case XK_Insert: *key = K_INS; break;
			case XK_KP_Insert:
			case XK_KP_0: *key = K_KP_INS; break;

			case XK_KP_Multiply: *key = '*'; break;
			case XK_KP_Add: *key = K_KP_PLUS; break;
			case XK_KP_Subtract: *key = K_KP_MINUS; break;
			case XK_KP_Divide: *key = K_KP_SLASH; break;

			// bk001130 - from cvs1.17 (mkv)
			case XK_exclam: *key = '1'; break;
			case XK_at: *key = '2'; break;
			case XK_numbersign: *key = '3'; break;
			case XK_dollar: *key = '4'; break;
			case XK_percent: *key = '5'; break;
			case XK_asciicircum: *key = '6'; break;
			case XK_ampersand: *key = '7'; break;
			case XK_asterisk: *key = '8'; break;
			case XK_parenleft: *key = '9'; break;
			case XK_parenright: *key = '0'; break;

			// weird french keyboards ..
			// NOTE: console toggle is hardcoded in cl_keys.c, can't be unbound
			//   cleaner would be .. using hardware key codes instead of the key syms
			//   could also add a new K_KP_CONSOLE
			case XK_twosuperior: *key = '~'; break;

			// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=472
			case XK_space:
			case XK_KP_Space: *key = K_SPACE; break;

			default:
			if ( XLookupRet == 0 )
			{
				if ( com_developer->value )
				{
					ri.Printf( PRINT_ALL, "Warning: XLookupString failed on KeySym %d\n", keysym );
				}
				return NULL;
			}
			else
			{
				// XK_* tests failed, but XLookupString got a buffer, so let's try it
				*key = *( unsigned char * ) buf;
				if ( *key >= 'A' && *key <= 'Z' )
					* key = *key - 'A' + 'a';
				// if ctrl is pressed, the keys are not between 'A' and 'Z', for instance ctrl-z == 26 ^Z ^C etc.
				// see https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=19
				else if ( *key >= 1 && *key <= 26 )
					* key = *key + 'a' - 1;
			}
			break;
	}

	return buf;
}

// ========================================================================
// makes a null cursor
// ========================================================================

static Cursor CreateNullCursor( Display *display, Window root )
{
	Pixmap cursormask;
	XGCValues xgc;
	GC gc;
	XColor dummycolour;
	Cursor cursor;

	cursormask = XCreatePixmap( display, root, 1, 1, 1 /*depth*/ );
	xgc.function = GXclear;
	gc = XCreateGC( display, cursormask, GCFunction, &xgc );
	XFillRectangle( display, cursormask, gc, 0, 0, 1, 1 );
	dummycolour.pixel = 0;
	dummycolour.red = 0;
	dummycolour.flags = 04;
	cursor = XCreatePixmapCursor( display, cursormask, cursormask,
	                              &dummycolour, &dummycolour, 0, 0 );
	XFreePixmap( display, cursormask );
	XFreeGC( display, gc );
	return cursor;
}

static void install_grabs( void )
{
	// inviso cursor
	XWarpPointer( dpy, None, win,
	              0, 0, 0, 0,
	              glConfig.vidWidth / 2, glConfig.vidHeight / 2 );
	XSync( dpy, False );

	XDefineCursor( dpy, win, CreateNullCursor( dpy, win ) );

	XGrabPointer( dpy, win,  // bk010108 - do this earlier?
	              False,
	              MOUSE_MASK,
	              GrabModeAsync, GrabModeAsync,
	              win,
	              None,
	              CurrentTime );

	XGetPointerControl( dpy, &mouse_accel_numerator, &mouse_accel_denominator,
	                    &mouse_threshold );

	XChangePointerControl( dpy, True, True, 1, 1, 0 );

	XSync( dpy, False );

	mouseResetTime = Sys_Milliseconds ();

#ifdef HAVE_XF86DGA
	if ( in_dgamouse->value )
	{
		int MajorVersion, MinorVersion;

		if ( !XF86DGAQueryVersion( dpy, &MajorVersion, &MinorVersion ) )
		{
			// unable to query, probalby not supported, force the setting to 0
			ri.Printf( PRINT_ALL, "Failed to detect XF86DGA Mouse\n" );
			ri.Cvar_Set( "in_dgamouse", "0" );
		}
		else
		{
			XF86DGADirectVideo( dpy, DefaultScreen( dpy ), XF86DGADirectMouse );
			XWarpPointer( dpy, None, win, 0, 0, 0, 0, 0, 0 );
		}
	}
	else
#endif /* HAVE_XF86DGA */

	{
		mwx = glConfig.vidWidth / 2;
		mwy = glConfig.vidHeight / 2;
		mx = my = 0;
	}

	XGrabKeyboard( dpy, win,
	               False,
	               GrabModeAsync, GrabModeAsync,
	               CurrentTime );

	XSync( dpy, False );
}

static void uninstall_grabs( void )
{
#ifdef HAVE_XF86DGA
	if ( in_dgamouse->value )
	{
		if ( com_developer->value )
			ri.Printf( PRINT_ALL, "DGA Mouse - Disabling DGA DirectVideo\n" );
		XF86DGADirectVideo( dpy, DefaultScreen( dpy ), 0 );
	}
#endif /* HAVE_XF86DGA */
	
	XChangePointerControl( dpy, qtrue, qtrue, mouse_accel_numerator,
	                       mouse_accel_denominator, mouse_threshold );

	XUngrabPointer( dpy, CurrentTime );
	// we never want to ungrab the keyboard when in fullscreen
	if ( r_fullscreen->integer == 0 ) {
		XUngrabKeyboard( dpy, CurrentTime );
	}

	XWarpPointer( dpy, None, win,
	              0, 0, 0, 0,
	              glConfig.vidWidth / 2, glConfig.vidHeight / 2 );

	// inviso cursor
	XUndefineCursor( dpy, win );
}

// bk001206 - from Ryan's Fakk2
/**
 * XPending() actually performs a blocking read 
 *  if no events available. From Fakk2, by way of
 *  Heretic2, by way of SDL, original idea GGI project.
 * The benefit of this approach over the quite
 *  badly behaved XAutoRepeatOn/Off is that you get
 *  focus handling for free, which is a major win
 *  with debug and windowed mode. It rests on the
 *  assumption that the X server will use the
 *  same timestamp on press/release event pairs 
 *  for key repeats. 
 */
static qboolean X11_PendingInput( void )
{

	assert( dpy != NULL );

	// Flush the display connection
	//  and look to see if events are queued
	XFlush( dpy );
	if ( XEventsQueued( dpy, QueuedAlready ) )
	{
		return qtrue;
	}

	// More drastic measures are required -- see if X is ready to talk
	{
		static struct timeval zero_time;
		int x11_fd;
		fd_set fdset;

		x11_fd = ConnectionNumber( dpy );
		FD_ZERO( &fdset );
		FD_SET( x11_fd, &fdset );
		if ( select( x11_fd + 1, &fdset, NULL, NULL, &zero_time ) == 1 )
		{
			return ( XPending( dpy ) );
		}
	}

	// Oh well, nothing is ready ..
	return qfalse;
}

// bk001206 - from Ryan's Fakk2. See above.
static qboolean repeated_press( XEvent *event )
{
	XEvent peekevent;
	qboolean repeated = qfalse;

	assert( dpy != NULL );

	if ( X11_PendingInput() )
	{
		XPeekEvent( dpy, &peekevent );

		if ( ( peekevent.type == KeyPress ) &&
		        ( peekevent.xkey.keycode == event->xkey.keycode ) &&
		        ( peekevent.xkey.time == event->xkey.time ) )
		{
			repeated = qtrue;
			XNextEvent( dpy, &peekevent );  // skip event.
		} // if
	} // if

	return ( repeated );
} // repeated_press

int Sys_XTimeToSysTime ( Time xtime );
static void HandleEvents( void )
{
	int b;
	int key;
	XEvent event;
	qboolean dowarp = qfalse;
	char *p;
	int dx, dy;
	int t = 0; // default to 0 in case we don't set

	if ( !dpy )
		return ;

	while ( XPending( dpy ) )
	{
		XNextEvent( dpy, &event );
		switch ( event.type )
		{
				case KeyPress:
				t = Sys_XTimeToSysTime( event.xkey.time );
				p = XLateKey( &event.xkey, &key );
				if ( key )
				{
					Sys_QueEvent( t, SE_KEY, key, qtrue, 0, NULL );
				}
				if ( p )
				{
					while ( *p )
					{
						Sys_QueEvent( t, SE_CHAR, *p++, 0, 0, NULL );
					}
				}
				break;

				case KeyRelease:
				t = Sys_XTimeToSysTime( event.xkey.time );
				// bk001206 - handle key repeat w/o XAutRepatOn/Off
				//            also: not done if console/menu is active.
				// From Ryan's Fakk2.
				// see game/q_shared.h, KEYCATCH_* . 0 == in 3d game.
				if ( cls.keyCatchers == 0 )
				{   // FIXME: KEYCATCH_NONE
					if ( repeated_press( &event ) == qtrue )
						continue;
				} // if
				XLateKey( &event.xkey, &key );

				Sys_QueEvent( t, SE_KEY, key, qfalse, 0, NULL );
				break;

				case MotionNotify:
				t = Sys_XTimeToSysTime( event.xkey.time );
				if ( mouse_active )
				{
#ifdef HAVE_XF86DGA
					if ( in_dgamouse->value )
					{
						mx += event.xmotion.x_root;
						my += event.xmotion.y_root;
						if ( t - mouseResetTime > MOUSE_RESET_DELAY )
						{
							Sys_QueEvent( t, SE_MOUSE_REL, mx, my, 0, NULL );
						}
						mx = my = 0;
					}
					else
#endif /* HAVE_XF86DGA */

					{
						// If it's a center motion, we've just returned from our warp
						if ( event.xmotion.x == glConfig.vidWidth / 2 &&
						        event.xmotion.y == glConfig.vidHeight / 2 )
						{
							mwx = glConfig.vidWidth / 2;
							mwy = glConfig.vidHeight / 2;
							if ( t - mouseResetTime > MOUSE_RESET_DELAY )
							{
								Sys_QueEvent( t, SE_MOUSE_REL, mx, my, 0, NULL );
							}
							mx = my = 0;
							break;
						}

						dx = ( ( int ) event.xmotion.x - mwx );
						dy = ( ( int ) event.xmotion.y - mwy );
						mx += dx;
						my += dy;

						mwx = event.xmotion.x;
						mwy = event.xmotion.y;
						dowarp = qtrue;
					}
				}
				break;

				case ButtonPress:
				t = Sys_XTimeToSysTime( event.xkey.time );
				if ( event.xbutton.button == 4 )
				{
					Sys_QueEvent( t, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
				}
				else if ( event.xbutton.button == 5 )
				{
					Sys_QueEvent( t, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
				}
				else
				{
					// NOTE TTimo there seems to be a weird mapping for K_MOUSE1 K_MOUSE2 K_MOUSE3 ..
					b = -1;
					if ( event.xbutton.button == 1 )
					{
						b = 0; // K_MOUSE1
					}
					else if ( event.xbutton.button == 2 )
					{
						b = 2; // K_MOUSE3
					}
					else if ( event.xbutton.button == 3 )
					{
						b = 1; // K_MOUSE2
					}
					else if ( event.xbutton.button == 6 )
					{
						b = 3; // K_MOUSE4
					}
					else if ( event.xbutton.button == 7 )
					{
						b = 4; // K_MOUSE5
					};

					Sys_QueEvent( t, SE_KEY, K_MOUSE1 + b, qtrue, 0, NULL );
				}
				break;

				case ButtonRelease:
				t = Sys_XTimeToSysTime( event.xkey.time );
				if ( event.xbutton.button == 4 )
				{
					Sys_QueEvent( t, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
				}
				else if ( event.xbutton.button == 5 )
				{
					Sys_QueEvent( t, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
				}
				else
				{
					b = -1;
					if ( event.xbutton.button == 1 )
					{
						b = 0;
					}
					else if ( event.xbutton.button == 2 )
					{
						b = 2;
					}
					else if ( event.xbutton.button == 3 )
					{
						b = 1;
					}
					else if ( event.xbutton.button == 6 )
					{
						b = 3; // K_MOUSE4
					}
					else if ( event.xbutton.button == 7 )
					{
						b = 4; // K_MOUSE5
					};
					Sys_QueEvent( t, SE_KEY, K_MOUSE1 + b, qfalse, 0, NULL );
				}
				break;

				case CreateNotify :
				win_x = event.xcreatewindow.x;
				win_y = event.xcreatewindow.y;
				break;

				case ConfigureNotify :
				win_x = event.xconfigure.x;
				win_y = event.xconfigure.y;
				break;
				
				case FocusIn:
					app_active = qtrue;
					break;
				case FocusOut:
					app_active = qfalse;
					break;
		}
	}

	if ( dowarp )
	{
		XWarpPointer( dpy, None, win, 0, 0, 0, 0,
		              ( glConfig.vidWidth / 2 ), ( glConfig.vidHeight / 2 ) );
	}
}

// NOTE TTimo for the tty console input, we didn't rely on those ..
//   it's not very surprising actually cause they are not used otherwise
void KBD_Init( void )
{}

void KBD_Close( void )
{}

void IN_ActivateMouse( void )
{
	if ( !mouse_avail || !dpy || !win )
		return ;

	if ( !mouse_active )
	{
		if ( !in_nograb->value )
			install_grabs();
		else if ( in_dgamouse->value )  // force dga mouse to 0 if using nograb
			ri.Cvar_Set( "in_dgamouse", "0" );
		mouse_active = qtrue;
	}
}

void IN_DeactivateMouse( void )
{
	if ( !mouse_avail || !dpy || !win )
		return ;

	if ( mouse_active )
	{
		if ( !in_nograb->value )
			uninstall_grabs();
		else if ( in_dgamouse->value )  // force dga mouse to 0 if using nograb
			ri.Cvar_Set( "in_dgamouse", "0" );
		mouse_active = qfalse;
	}
}

void Sys_SetCursor( int c ) {
}
/*****************************************************************************/
static void GLimp_GetDisplayModes() {
	int i, j;
	
	sql_exec( &com_db,
			"CREATE TABLE monitors"
			"("
				"dev_name STRING, "
				"x INTEGER, "
				"y INTEGER, "
				"w INTEGER, "
				"h INTEGER, "
				"gl_level INTEGER"
			");"

			"CREATE TABLE fsmodes"
			"("
				"id INTEGER, "
				"w INTEGER, "
				"h INTEGER, "
				"dev_name STRING"
			");"
			);

	
	for (i=0; i < ScreenCount(dpy); i++) {
		XF86VidModeGetAllModeLines( dpy, i, &num_vidmodes, &vidmodes );
		sql_prepare ( &com_db, "INSERT INTO monitors(dev_name, x, y, w, h, gl_level) VALUES(?,?,?,?,?,?);" );
		sql_bindtext( &com_db, 1, va("%d", i) );
		sql_bindint( &com_db, 2, 0 );
		sql_bindint( &com_db, 3, 0 );
		sql_bindint( &com_db, 4, vidmodes[i]->vdisplay );
		sql_bindint( &com_db, 5, vidmodes[i]->hdisplay );
		sql_bindint( &com_db, 6, 2 );
		sql_step( &com_db );
		sql_done( &com_db );
		
		for (j=0; j < num_vidmodes; j++) {
			int width, height, hz, bpp, monitor_id;

			width = vidmodes[j]->hdisplay;
			height = vidmodes[j]->vdisplay;

			sql_prepare( &com_db, "UPDATE OR INSERT fsmodes SET id=#,w=?1,h=?2,dev_name=?3 SEARCH dev_name ?3 WHERE w=?1 and h=?2");
			sql_bindint( &com_db, 1, width);
			sql_bindint( &com_db, 2, height);
			sql_bindtext( &com_db, 3, va("%d", i));
			sql_step( &com_db );
			sql_done( &com_db );

		}
	}
}

/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( unsigned char red[ 256 ], unsigned char green[ 256 ], unsigned char blue[ 256 ] )
{
	// NOTE TTimo we get the gamma value from cvar, because we can't work with the s_gammatable
	//   the API wasn't changed to avoid breaking other OSes
#ifdef HAVE_XF86VIDMODE
	float g = Cvar_Get( "r_gamma", "1.0", 0 ) ->value;
	XF86VidModeGamma gamma;
	assert( glConfig.deviceSupportsGamma );
	gamma.red = g;
	gamma.green = g;
	gamma.blue = g;
	XF86VidModeSetGamma( dpy, scrnum, &gamma );
#endif /* HAVE_XF86VIDMODE */
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
	if ( !ctx || !dpy )
		return ;
	IN_DeactivateMouse();
	// bk001206 - replaced with H2/Fakk2 solution
	// XAutoRepeatOn(dpy);
	// autorepeaton = qfalse; // bk001130 - from cvs1.17 (mkv)
	if ( dpy )
	{
		if ( ctx )
			glXDestroyContext( dpy, ctx );
		if ( win )
			XDestroyWindow( dpy, win );
#ifdef HAVE_XF86VIDMODE
		if ( vidmode_active )
			XF86VidModeSwitchToMode( dpy, scrnum, vidmodes[ 0 ] );
		if ( glConfig.deviceSupportsGamma )
		{
			XF86VidModeSetGamma( dpy, scrnum, &vidmode_InitialGamma );
		}
#endif /* HAVE_XF86VIDMODE */
		// NOTE TTimo opening/closing the display should be necessary only once per run
		//   but it seems QGL_Shutdown gets called in a lot of occasion
		//   in some cases, this XCloseDisplay is known to raise some X errors
		//   ( https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=33 )
		XCloseDisplay( dpy );
	}
	vidmode_active = qfalse;
	dpy = NULL;
	win = 0;
	ctx = NULL;

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );

}

/*
** GLimp_LogComment
*/
#ifdef _DEBUG
void GLimp_LogComment( int level, char *comment, ... )
{
	if ( level && level > r_logLevel->integer )
		return ;

	if ( GLEW_GREMEDY_string_marker )
	{
		size_t len;
		char msg[ 2048 ];
		va_list vl;

		va_start( vl, comment );
		len = vsnprintf( msg, sizeof( msg ) - 1, comment, vl );
		va_end( vl );

		if ( len == -1 )
			len = sizeof( msg ) - 1;

		glStringMarkerGREMEDY( len, msg );
	}
}
#endif

/*
** GLW_StartDriverAndSetMode
*/ 
// bk001204 - prototype needed
int GLW_SetMode( const char *drivername, int mode, qboolean fullscreen );
static qboolean GLW_StartDriverAndSetMode( const char *drivername,
        int mode,
        qboolean fullscreen )
{
	rserr_t err;
	cvar_t *window_id;

	if ( fullscreen && in_nograb->value )
	{
		ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n" );
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}

	window_id = ri.Cvar_Get("window_id", "0", CVAR_ROM);
	if (window_id->integer != 0) {
		g_wv.hWnd = window_id->integer;
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
int GLW_SetMode( const char *drivername, int mode, qboolean fullscreen )
{
	int attrib[] = {
	                   GLX_RGBA,          // 0
	                   GLX_RED_SIZE, 4,       // 1, 2
	                   GLX_GREEN_SIZE, 4,       // 3, 4
	                   GLX_BLUE_SIZE, 4,      // 5, 6
	                   GLX_DOUBLEBUFFER,      // 7
	                   GLX_DEPTH_SIZE, 1,       // 8, 9
	                   GLX_STENCIL_SIZE, 1,     // 10, 11
	                   None
	               };
	// these match in the array
#define ATTR_RED_IDX 2
#define ATTR_GREEN_IDX 4
#define ATTR_BLUE_IDX 6
#define ATTR_DEPTH_IDX 9
#define ATTR_STENCIL_IDX 11
	Window root;
	XVisualInfo *visinfo;
	XSetWindowAttributes attr;
	XSizeHints sizehints;
	unsigned long mask;
	int colorbits, depthbits, stencilbits;
	int tcolorbits, tdepthbits, tstencilbits;
	int dga_MajorVersion, dga_MinorVersion;
	int actualWidth, actualHeight;
	int i;
	int eventBase, errorBase;
	const char* glstring; // bk001130 - from cvs1.17 (mkv)

	ri.Printf( PRINT_ALL, "Initializing OpenGL display\n" );

//	ri.Printf ( PRINT_ALL, "...setting mode %d:", mode );

	if (g_wv.hWnd) {
		int width, height, x_return, y_return, border_width_return, depth_return;
		Window w;
		
		g_wv.display = XOpenDisplay(NULL);
		
		XGetGeometry( g_wv.display, g_wv.hWnd, &w, &x_return, &y_return, &width, &height, &border_width_return, &depth_return);
		glConfig.vidWidth = width;
		glConfig.vidHeight = height;
	}
	if ( !( dpy = XOpenDisplay( NULL ) ) )
	{
		fprintf( stderr, "Error couldn't open the X display\n" );
		return RSERR_INVALID_MODE;
	}

	scrnum = DefaultScreen( dpy );
	if (g_wv.hWnd) {
		root = (Window)g_wv.hWnd;
	} else {
		root = RootWindow( dpy, scrnum );
	}

#ifdef HAVE_XF86VIDMODE
	if ( !XF86VidModeQueryExtension( dpy, &eventBase, &errorBase) ){
		ri.Printf( PRINT_ALL, "XFree86-VidModeExtension is not loaded, and is required\n");
		ri.Printf( PRINT_ALL, "\tDo you have 3d accelerated drivers installed?\n");
	}
#endif
	GLimp_GetDisplayModes();

#ifdef HAVE_XF86DGA
	// Check for DGA
	dga_MajorVersion = 0, dga_MinorVersion = 0;
	if ( in_dgamouse->value )
	{
		if ( !XF86DGAQueryVersion( dpy, &dga_MajorVersion, &dga_MinorVersion ) )
		{
			// unable to query, probably not supported
			ri.Printf( PRINT_ALL, "Failed to detect XF86DGA Mouse\n" );
			ri.Cvar_Set( "in_dgamouse", "0" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "XF86DGA Mouse (Version %d.%d) initialized\n",
			           dga_MajorVersion, dga_MinorVersion );
		}
	}
#endif

	// Are we going fullscreen?  If so, let's change video mode
	if ( fullscreen )
	{
		char *res = r_fsmode->string;
		int mode_id, w, h;
		
		w = atoi( COM_Parse( &res ) );
		h = atoi( COM_Parse( &res ) );
		
		sql_prepare( &com_db, "SELECT id FROM monitors SEARCH dev_name ?");
		sql_bindtext( &com_db, 1, r_fsmonitor->string );
		if ( sql_step( &com_db ) ){
			mode_id = sql_columnasint( &com_db, 0);
		}
		sql_done( &com_db );

		mode_id = -1;
		sql_prepare( &com_db, "SELECT id FROM fsmodes SEARCH dev_name ?1 WHERE w=?2 AND h=?3" );
		sql_bindtext( &com_db, 1, r_fsmonitor->string );
		sql_bindint( &com_db, 2, w );
		sql_bindint( &com_db, 3, h );
		if ( sql_step( &com_db ) ) {
			mode_id = sql_columnasint( &com_db, 0 );
		}
		sql_done ( &com_db );


		// change to the mode
		XF86VidModeSwitchToMode( dpy, scrnum, vidmodes[ mode_id ] );
		vidmode_active = qtrue;

		// Move the viewport to top left
		XF86VidModeSetViewPort( dpy, scrnum, 0, 0 );

		glConfig.vidWidth = w;
		glConfig.vidHeight = h;

	}
	else
	{
		if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
		{
			ri.Printf( PRINT_ALL, " invalid mode\n" );
			return RSERR_INVALID_MODE;
		}
		ri.Printf( PRINT_ALL, "XFree86-VidModeExtension:  Ignored on non-fullscreen/Voodoo\n" );
	}


	ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight );

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

	actualWidth = glConfig.vidWidth;
	actualHeight = glConfig.vidHeight;
	ri.Printf( PRINT_ALL, "Chose resolution of: %dx%d\n",
				actualWidth, actualHeight );

	if ( !r_colorbits->value )
		colorbits = 24;
	else
		colorbits = r_colorbits->value;

	if ( !r_depthbits->value )
		depthbits = 24;
	else
		depthbits = r_depthbits->value;
	stencilbits = r_stencilbits->value;

	for ( i = 0; i < 16; i++ )
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ( ( i % 4 ) == 0 && i )
		{
			// one pass, reduce
			switch ( i / 4 )
			{
					case 2 :
					if ( colorbits == 24 )
						colorbits = 16;
					break;
					case 1 :
					if ( depthbits == 24 )
						depthbits = 16;
					else if ( depthbits == 16 )
						depthbits = 8;
					case 3 :
					if ( stencilbits == 24 )
						stencilbits = 16;
					else if ( stencilbits == 16 )
						stencilbits = 8;
			}
		}

		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;

		if ( ( i % 4 ) == 3 )
		{ // reduce colorbits
			if ( tcolorbits == 24 )
				tcolorbits = 16;
		}

		if ( ( i % 4 ) == 2 )
		{ // reduce depthbits
			if ( tdepthbits == 24 )
				tdepthbits = 16;
			else if ( tdepthbits == 16 )
				tdepthbits = 8;
		}

		if ( ( i % 4 ) == 1 )
		{ // reduce stencilbits
			if ( tstencilbits == 24 )
				tstencilbits = 16;
			else if ( tstencilbits == 16 )
				tstencilbits = 8;
			else
				tstencilbits = 0;
		}

		if ( tcolorbits == 24 )
		{
			attrib[ ATTR_RED_IDX ] = 8;
			attrib[ ATTR_GREEN_IDX ] = 8;
			attrib[ ATTR_BLUE_IDX ] = 8;
		}
		else
		{
			// must be 16 bit
			attrib[ ATTR_RED_IDX ] = 4;
			attrib[ ATTR_GREEN_IDX ] = 4;
			attrib[ ATTR_BLUE_IDX ] = 4;
		}

		attrib[ ATTR_DEPTH_IDX ] = tdepthbits; // default to 24 depth
		attrib[ ATTR_STENCIL_IDX ] = tstencilbits;

		visinfo = glXChooseVisual( dpy, scrnum, attrib );
		if ( !visinfo )
		{
			continue;
		}

		ri.Printf( PRINT_ALL, "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n",
		           attrib[ ATTR_RED_IDX ], attrib[ ATTR_GREEN_IDX ], attrib[ ATTR_BLUE_IDX ],
		           attrib[ ATTR_DEPTH_IDX ], attrib[ ATTR_STENCIL_IDX ] );

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}

	if ( !visinfo )
	{
		ri.Printf( PRINT_ALL, "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	/* window attributes */
	attr.background_pixel = BlackPixel( dpy, scrnum );
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone );
	attr.event_mask = X_MASK;
	if ( vidmode_active )
	{
		mask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore |
		       CWEventMask | CWOverrideRedirect;
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	}
	else
		mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow( dpy, root, 0, 0,
	                     actualWidth, actualHeight,
	                     0, visinfo->depth, InputOutput,
	                     visinfo->visual, mask, &attr );

	XStoreName( dpy, win, WINDOW_CLASS_NAME );

	/* GH: Don't let the window be resized */
	sizehints.flags = PMinSize | PMaxSize;
	sizehints.min_width = sizehints.max_width = actualWidth;
	sizehints.min_height = sizehints.max_height = actualHeight;

	XSetWMNormalHints( dpy, win, &sizehints );

	XMapWindow( dpy, win );

	if ( vidmode_active )
		XMoveWindow( dpy, win, 0, 0 );

	XFlush( dpy );
	XSync( dpy, False ); // bk001130 - from cvs1.17 (mkv)
	ctx = glXCreateContext( dpy, visinfo, NULL, True );
	XSync( dpy, False ); // bk001130 - from cvs1.17 (mkv)

	/* GH: Free the visinfo after we're done with it */
	XFree( visinfo );

	glXMakeCurrent( dpy, win, ctx );

	if ( glewInit() != GLEW_OK )
	{
		ri.Error( ERR_DROP, "Failed loading GL extensions." );
		return RSERR_INVALID_MODE;
	}


	// bk001130 - from cvs1.17 (mkv)
	glstring = glGetString ( GL_RENDERER );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring );

	// bk010122 - new software token (Indirect)
	if ( !Q_stricmp( glstring, "Mesa X11" )
	        || !Q_stricmp( glstring, "Mesa GLX Indirect" ) )
	{
		if ( !r_allowSoftwareGL->integer )
		{
			ri.Printf( PRINT_ALL, "\n\n***********************************************************\n" );
			ri.Printf( PRINT_ALL, " You are using software Mesa (no hardware acceleration)!   \n" );
			ri.Printf( PRINT_ALL, " Driver DLL used: %s\n", drivername );
			ri.Printf( PRINT_ALL, " If this is intentional, add\n" );
			ri.Printf( PRINT_ALL, "       \"+set r_allowSoftwareGL 1\"\n" );
			ri.Printf( PRINT_ALL, " to the command line when starting the game.\n" );
			ri.Printf( PRINT_ALL, "***********************************************************\n" );
			GLimp_Shutdown( );
			return RSERR_INVALID_MODE;
		}
		else
		{
			ri.Printf( PRINT_ALL, "...using software Mesa (r_allowSoftwareGL==1).\n" );
		}
	}
	if ( fullscreen ) {
		XGrabPointer( dpy, win,  // bk010108 - do this earlier?
	              False,
	              MOUSE_MASK,
	              GrabModeAsync, GrabModeAsync,
	              win,
	              None,
	              CurrentTime );
		XGrabKeyboard( dpy, win,
	               False,
	               GrabModeAsync, GrabModeAsync,
	               CurrentTime );
	}
	return RSERR_OK;
}

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void )
{
	GLint glint;
	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if( GLEW_EXT_texture_env_add )
	{
		glConfig.textureEnvAddAvailable = qtrue;
		ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}
	if( GLEW_ARB_multitexture )
	{
		glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glint );
		glConfig.maxActiveTextures = glint;
	}

	glGetIntegerv( GL_MAX_TEXTURE_SIZE, &glint );
	glConfig.maxTextureSize = glint;
}


static void GLW_InitGamma( void )
{
	/* Minimum extension version required */
#define GAMMA_MINMAJOR 2
 #define GAMMA_MINMINOR 0

	glConfig.deviceSupportsGamma = qfalse;

#ifdef HAVE_XF86VIDMODE
	if ( vidmode_ext )
	{
		if ( vidmode_MajorVersion < GAMMA_MINMAJOR ||
		        ( vidmode_MajorVersion == GAMMA_MINMAJOR && vidmode_MinorVersion < GAMMA_MINMINOR ) )
		{
			ri.Printf( PRINT_ALL, "XF86 Gamma extension not supported in this version\n" );
			return ;
		}
		XF86VidModeGetGamma( dpy, scrnum, &vidmode_InitialGamma );
		ri.Printf( PRINT_ALL, "XF86 Gamma extension initialized\n" );
		glConfig.deviceSupportsGamma = qtrue;
	}
#endif /* HAVE_XF86VIDMODE */
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

	ri.Printf( PRINT_ALL, "...loading %s: ", name );

	// load the QGL layer
	fullscreen = r_fullscreen->integer;

	// create the window and set up the context
	if ( !GLW_StartDriverAndSetMode( name, r_mode->integer, fullscreen ) )
	{
		if ( r_mode->integer != 3 )
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
** XErrorHandler
**   the default X error handler exits the application
**   I found out that on some hosts some operations would raise X errors (GLXUnsupportedPrivateRequest)
**   but those don't seem to be fatal .. so the default would be to just ignore them
**   our implementation mimics the default handler behaviour (not completely cause I'm lazy)
*/
int qXErrorHandler( Display *dpy, XErrorEvent *ev )
{
	static char buf[ 1024 ];
	XGetErrorText( dpy, ev->error_code, buf, 1024 );
	ri.Printf( PRINT_ALL, "X Error of failed request: %s\n", buf );
	ri.Printf( PRINT_ALL, "  Major opcode of failed request: %d\n", ev->request_code, buf );
	ri.Printf( PRINT_ALL, "  Minor opcode of failed request: %d\n", ev->minor_code );
	ri.Printf( PRINT_ALL, "  Serial number of failed request: %d\n", ev->serial );
	return 0;
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

	r_mode = ri.Cvar_Get( "r_mode", "4", CVAR_ARCHIVE | CVAR_LATCH );
	r_fullscreen = ri.Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_fsmode = ri.Cvar_Get( "r_fsmode", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_fsmonitor = ri.Cvar_Get( "r_fsmonitor", "0", CVAR_ARCHIVE | CVAR_LATCH );

	
	// Hack here so that if the UI
	if ( *r_previousglDriver->string )
	{
		// The UI changed it on us, hack it back
		// This means the renderer can't be changed on the fly
		ri.Cvar_Set( "r_glDriver", r_previousglDriver->string );
	}
	// guarded, as this is only relevant to SMP renderer thread
#ifdef SMP
	if ( !XInitThreads() )
	{
		Com_Printf( "GLimp_Init() - XInitThreads() failed, disabling r_smp\n" );
		ri.Cvar_Set( "r_smp", "0" );
	}
#endif

	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );

	r_previousglDriver = ri.Cvar_Get( "r_previousglDriver", "", CVAR_ROM );

	InitSig();

	IN_Init();   // rcg08312005 moved into glimp.

	// set up our custom error handler for X failures
	XSetErrorHandler( &qXErrorHandler );

	//
	// load and initialize the specific OpenGL driver
	//
	if ( !GLW_LoadOpenGL( glConfig.renderer_string ) )
	{
		ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n" );

	}

	ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );
	
	// This values force the UI to disable driver selection

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, (const char*)glGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (const char*)glGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (const char*)glGetString (GL_VERSION), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (const char*)glGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );

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

	InitSig(); // not clear why this is at begin & end of function

	return ;
}


void Glimp_PrintSystemInfo()
{

}


/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame ( void )
{
	glXSwapBuffers( dpy, win );
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

#ifdef SMP 
/*
===========================================================
 
SMP acceleration
 
===========================================================
*/

static pthread_mutex_t	smpMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t	renderCommandsEvent = PTHREAD_COND_INITIALIZER;
static pthread_cond_t	renderCompletedEvent = PTHREAD_COND_INITIALIZER;

static void ( *glimpRenderThread ) ( void );

static void *GLimp_RenderThreadWrapper( void *arg )
{
	Com_Printf( "Render thread starting\n" );

	glimpRenderThread();

	qglXMakeCurrent( dpy, None, NULL );

	Com_Printf( "Render thread terminating\n" );

	return arg;
}

qboolean GLimp_SpawnRenderThread( void ( *function ) ( void ) )
{
	pthread_t renderThread;
	int ret;

	pthread_mutex_init( &smpMutex, NULL );

	pthread_cond_init( &renderCommandsEvent, NULL );
	pthread_cond_init( &renderCompletedEvent, NULL );

	glimpRenderThread = function;

	ret = pthread_create( &renderThread,
	                      NULL, 			// attributes
	                      GLimp_RenderThreadWrapper,
	                      NULL );		// argument
	if ( ret )
	{
		ri.Printf( PRINT_ALL, "pthread_create returned %d: %s", ret, strerror( ret ) );
		return qfalse;
	}
	else
	{
		ret = pthread_detach( renderThread );
		if ( ret )
		{
			ri.Printf( PRINT_ALL, "pthread_detach returned %d: %s", ret, strerror( ret ) );
		}
	}

	return qtrue;
}

static volatile void *smpData = NULL;
static volatile qboolean smpDataReady;

void *GLimp_RendererSleep( void )
{
	void * data;

	qglXMakeCurrent( dpy, None, NULL );

	pthread_mutex_lock( &smpMutex );
	{
		smpData = NULL;
		smpDataReady = qfalse;

		// after this, the front end can exit GLimp_FrontEndSleep
		pthread_cond_signal( &renderCompletedEvent );

		while ( !smpDataReady )
		{
			pthread_cond_wait( &renderCommandsEvent, &smpMutex );
		}

		data = ( void * ) smpData;
	}
	pthread_mutex_unlock( &smpMutex );

	qglXMakeCurrent( dpy, win, ctx );

	return data;
}

void GLimp_FrontEndSleep( void )
{
	pthread_mutex_lock( &smpMutex );
	{
		while ( smpData )
		{
			pthread_cond_wait( &renderCompletedEvent, &smpMutex );
		}
	}
	pthread_mutex_unlock( &smpMutex );

	qglXMakeCurrent( dpy, win, ctx );
}

void GLimp_WakeRenderer( void *data )
{
	qglXMakeCurrent( dpy, None, NULL );

	pthread_mutex_lock( &smpMutex );
	{
		assert( smpData == NULL );
		smpData = data;
		smpDataReady = qtrue;

		// after this, the renderer can continue through GLimp_RendererSleep
		pthread_cond_signal( &renderCommandsEvent );
	}
	pthread_mutex_unlock( &smpMutex );
}

#else

void GLimp_RenderThreadWrapper( void *stub ) {}
qboolean GLimp_SpawnRenderThread( void ( *function ) ( void ) )
{
	ri.Printf( PRINT_WARNING, "ERROR: SMP support was disabled at compile time\n" );
	return qfalse;
}
void *GLimp_RendererSleep( void )
{
	return NULL;
}
void GLimp_FrontEndSleep( void ) {}
void GLimp_WakeRenderer( void *data ) {}

#endif

/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/

void IN_Init( void )
{
	Com_Printf ( "\n------- Input Initialization -------\n" );
	// mouse variables
	in_mouse = Cvar_Get ( "in_mouse", "1", CVAR_ARCHIVE );
	in_dgamouse = Cvar_Get ( "in_dgamouse", "1", CVAR_ARCHIVE );

	// turn on-off sub-frame timing of X events
	in_subframe = Cvar_Get ( "in_subframe", "1", CVAR_ARCHIVE );

	// developer feature, allows to break without loosing mouse pointer
	in_nograb = Cvar_Get ( "in_nograb", "0", 0 );

	// bk001130 - from cvs.17 (mkv), joystick variables
	in_joystick = Cvar_Get ( "in_joystick", "0", CVAR_ARCHIVE | CVAR_LATCH );
	// bk001130 - changed this to match win32
	in_joystickDebug = Cvar_Get ( "in_debugjoystick", "0", CVAR_TEMP );
	joy_threshold = Cvar_Get ( "joy_threshold", "0.15", CVAR_ARCHIVE ); // FIXME: in_joythreshold

	Cvar_Set( "cl_platformSensitivity", "2.0" );

	if ( in_mouse->value )
		mouse_avail = qtrue;
	else
		mouse_avail = qfalse;

	//IN_StartupJoystick( ); // bk001130 - from cvs1.17 (mkv)
	Com_Printf ( "------------------------------------\n" );
}

void IN_Shutdown( void )
{
	IN_DeactivateMouse();

	mouse_avail = qfalse;
}

void IN_Frame ( void )
{

	// bk001130 - from cvs 1.17 (mkv)
	//IN_JoyMove(); // FIXME: disable if on desktop?
	if( cls.keyCatchers || ( cls.state < CA_ACTIVE) )
	{
		static int last_x, last_y;
		Window root, child;
		int root_x, root_y, win_x, win_y, mask;

		IN_DeactivateMouse();

		// find mouse movement
		if (XQueryPointer(dpy, win, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask) ) {
//			if (last_x != win_x && last_y != win_y) {
				Sys_QueEvent( 0, SE_MOUSE_ABS, win_x, win_y, 0, NULL );
//			}
			
//			last_x = win_x; last_y = win_y;
		}
		return;
	} else {
		IN_ActivateMouse();
	}
}

void IN_Activate( void )
{}

// bk001130 - cvs1.17 joystick code (mkv) was here, no linux_joystick.c

void Sys_SendKeyEvents ( void )
{
	// XEvent event; // bk001204 - unused

	if ( !dpy )
		return ;
	HandleEvents();
}


// bk010216 - added stubs for non-Linux UNIXes here
// FIXME - use NO_JOYSTICK or something else generic

#if defined( __FreeBSD__ ) // rb010123
void IN_StartupJoystick( void ) {}
void IN_JoyMove( void ) {}
#endif

#endif  // !USE_SDL_VIDEO
