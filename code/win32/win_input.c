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
// win_input.c -- win32 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#include "../client/client.h"
#include "win_local.h"


//
//	dinput.h
//
#undef DEFINE_GUID

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID(GUID_SysMouse,   0x6F1D2B60,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_XAxis,   0xA36D02E0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_YAxis,   0xA36D02E1,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_ZAxis,   0xA36D02E2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_GUID(GUID_RxAxis,  0xA36D02F4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_RyAxis,  0xA36D02F5,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);


DEFINE_GUID(IID_IDirectInput8A,    0xBF798030,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
DEFINE_GUID(IID_IDirectInput8W,    0xBF798031,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);

#define DINPUT_BUFFERSIZE           32




typedef struct {
	int			oldButtonState;

	qboolean	mouseActive;
	qboolean	mouseInitialized;
	qboolean	mouseStartupDelayed; // delay mouse init to try DI again when we have a window
} WinMouseVars_t;

static WinMouseVars_t s_wmv;

static POINT wnd_center;

//
// MIDI definitions
//
static void IN_StartupMIDI( void );
static void IN_ShutdownMIDI( void );

#define MAX_MIDIIN_DEVICES	8

typedef struct {
	int			numDevices;
	MIDIINCAPS	caps[MAX_MIDIIN_DEVICES];

	HMIDIIN		hMidiIn;
} MidiInfo_t;

static MidiInfo_t s_midiInfo;

cvar_t	*in_midi;
cvar_t	*in_midiport;
cvar_t	*in_midichannel;
cvar_t	*in_mididevice;

cvar_t	*in_mouse;
cvar_t  *in_logitechbug;
cvar_t	*in_joystick;
cvar_t	*in_joyBallScale;
cvar_t	*in_debugJoystick;
cvar_t	*joy_threshold;

qboolean	in_appactive;

// forward-referenced functions
void IN_StartupJoystick (void);
void IN_ShutdownJoystick (void);
void IN_JoyMove(void);

static void Q_EXTERNAL_CALL MidiInfo_f( void );

/*
============================================================

WIN32 MOUSE CONTROL

============================================================
*/

qboolean Sys_IsForeground( void )
{
	return in_appactive;
}


/*
================
IN_InitWin32Mouse
================
*/
void IN_InitWin32Mouse( void ) 
{
}

/*
================
IN_ShutdownWin32Mouse
================
*/
void IN_ShutdownWin32Mouse( void ) {
}

static POINT saveMousePos;

/*
================
IN_ActivateWin32Mouse
================
*/
void IN_ActivateWin32Mouse( void )
{
	POINT pt;
	RECT rc;

	GetClientRect( g_wv.hWnd, &rc );
		 
	pt.x = 0;
	pt.y = 0;

	ClientToScreen( g_wv.hWnd, &pt );
	OffsetRect( &rc, pt.x, pt.y );

	wnd_center.x = (rc.right + rc.left) / 2;
	wnd_center.y = (rc.top + rc.bottom) / 2;

	if( !Sys_IsForeground() )
		//don't try to steal focus
		//let WM_ACTIVATE call this when appropriate
		return;

	GetCursorPos( &saveMousePos );
	SetCursorPos( wnd_center.x, wnd_center.y );

	ClipCursor( &rc );

	PostMessage( g_wv.hWnd, WM_SHOWCURSOR, FALSE, 0 );
}

void Sys_SetCursor( int c ) {

	switch (c)
	{
	case 0: g_wv.cursor = LoadCursor( NULL, IDC_ARROW ); break;
	case 1: g_wv.cursor = LoadCursor( NULL, IDC_HAND ); break;
	case 2: g_wv.cursor = LoadCursor( NULL, IDC_WAIT ); break;
	}
}

/*
================
IN_DeactivateWin32Mouse
================
*/
void IN_DeactivateWin32Mouse( void ) 
{
	ClipCursor( NULL );
	ReleaseCapture();

	SetCursorPos( saveMousePos.x, saveMousePos.y );

	PostMessage( g_wv.hWnd, WM_SHOWCURSOR, TRUE, 0 );
}

/*
================
IN_Win32Mouse
================
*/
void IN_Win32Mouse( int *mx, int *my )
{
	static POINT lastPT;
	POINT pt;

	if( !Sys_IsForeground() )
		//don't try to steal focus
		//let WM_ACTIVATE call this when appropriate
		return;

	// find mouse movement
	GetCursorPos( &pt );

	// force the mouse to the center, so there's room to move
	SetCursorPos( wnd_center.x, wnd_center.y );

	*mx = pt.x - wnd_center.x;
	*my = pt.y - wnd_center.y;

	lastPT.x = pt.x;
	lastPT.y = pt.y;
}


/*
============================================================

DIRECT INPUT MOUSE CONTROL

============================================================
*/
typedef struct MYDATA {
	LONG	lX;					// X axis goes here
	LONG	lY;					// Y axis goes here
	LONG	lZ;					// Z axis goes here
	BYTE	bButtonA;			// One button goes here
	BYTE	bButtonB;			// Another button goes here
	BYTE	bButtonC;			// Another button goes here
	BYTE	bButtonD;			// Another button goes here
} MYDATA;

static DIOBJECTDATAFORMAT rgodf[] = {
  { &GUID_XAxis,    FIELD_OFFSET(MYDATA, lX),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { &GUID_YAxis,    FIELD_OFFSET(MYDATA, lY),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { &GUID_ZAxis,    FIELD_OFFSET(MYDATA, lZ),       0x80000000 | DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonA), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonB), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonC), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonD), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
};

// NOTE TTimo: would be easier using c_dfDIMouse or c_dfDIMouse2
// NOTE TO NOTE pdjonov: true, but then we'd have to link in dinput.lib

static DIDATAFORMAT	df =
{
	sizeof(DIDATAFORMAT),		// this structure
	sizeof(DIOBJECTDATAFORMAT),	// size of object data format
	DIDF_RELAXIS,				// absolute axis coordinates
	sizeof(MYDATA),				// device data size
	lengthof( rgodf ),			// number of objects
	rgodf,						// and here they are
};

typedef struct {
	LONG	x;				// x-axis
	LONG	y;				// y-axis
	LONG	u;
	LONG	v;
	BYTE	buttons[16];		// buttons
} joyData_t;

static DIOBJECTDATAFORMAT joyobjectsformat[] = {
	{ &GUID_XAxis,	FIELD_OFFSET(joyData_t, x),				DIDFT_AXIS | DIDFT_ANYINSTANCE,			0 },
	{ &GUID_YAxis,	FIELD_OFFSET(joyData_t, y),				DIDFT_AXIS | DIDFT_ANYINSTANCE,			0 },
	{ &GUID_ZAxis,	FIELD_OFFSET(joyData_t, u),				DIDFT_AXIS | DIDFT_ANYINSTANCE,			0 },
	{ 0,			FIELD_OFFSET(joyData_t, v),				DIDFT_AXIS | DIDFT_ANYINSTANCE,			0 },

	{ 0,			FIELD_OFFSET(joyData_t, buttons[ 0]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[ 1]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[ 2]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[ 3]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[ 4]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[ 5]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[ 6]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[ 7]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[ 8]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[ 9]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[10]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[11]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[12]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[13]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[14]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
	{ 0,			FIELD_OFFSET(joyData_t, buttons[15]),	DIDFT_PSHBUTTON | DIDFT_ANYINSTANCE,	0 },
};

static DIDATAFORMAT joydf = {
	sizeof(DIDATAFORMAT),
	sizeof(DIOBJECTDATAFORMAT),
	DIDF_ABSAXIS,
	sizeof(joyData_t),
	lengthof(joyobjectsformat),
	joyobjectsformat,
};

static IDirectInput8 *g_pdi;
static IDirectInputDevice8 *g_pMouse;
static IDirectInputDevice8 *g_pJoystick;

void IN_DIMouse( int *mx, int *my );


//
// Joystick definitions
//
typedef struct {

	qboolean	avail;
	DIDEVCAPS	jc;
	joyData_t	ji;

	BYTE	buttons_old[16];	// button state last frame

} joystickInfo_t;

static	joystickInfo_t	joy;


/*
========================
IN_InitDIMouse
========================
*/
qboolean IN_InitDIMouse( void )
{
    HRESULT hr;
	DIPROPDWORD	dipdw =
	{
		{
			sizeof(DIPROPDWORD),        // diph.dwSize
			sizeof(DIPROPHEADER),       // diph.dwHeaderSize
			0,                          // diph.dwObj
			DIPH_DEVICE,                // diph.dwHow
		},
		DINPUT_BUFFERSIZE,              // dwData
	};

	static HINSTANCE hInstDI;
	static HRESULT (WINAPI *pDirectInput8Create)( HINSTANCE hinst, DWORD dwVersion, REFIID riidltf,
		LPVOID *ppvOut, LPUNKNOWN punkOuter );

	Com_Printf( "Initializing DirectInput...\n");

	if( !hInstDI )
	{
		hInstDI = LoadLibrary( "dinput8.dll" );
		
		if( hInstDI == NULL )
		{
			Com_Printf( "Couldn't load dinput8.dll\n" );
			return qfalse;
		}
	}

	if( !pDirectInput8Create )
	{
		pDirectInput8Create = (HRESULT (WINAPI *)( HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN ))
			GetProcAddress( hInstDI, "DirectInput8Create" );

		if( !pDirectInput8Create )
		{
			Com_Printf( "Couldn't get DI proc addr\n" );
			return qfalse;
		}
	}

	// register with DirectInput and get an IDirectInput to play with.
	hr = pDirectInput8Create( g_wv.hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8, &g_pdi, NULL );

	if( FAILED( hr ) )
	{
		Com_Printf( "DirectInput8Create failed\n" );
		return qfalse;
	}

	// obtain an interface to the system mouse device.
	hr = IDirectInput8_CreateDevice( g_pdi, &GUID_SysMouse, &g_pMouse, NULL );

	if( FAILED( hr ) )
	{
		Com_Printf( "Couldn't open DI mouse device\n" );
		return qfalse;
	}

	// set the data format to "mouse format".
	hr = IDirectInputDevice8_SetDataFormat( g_pMouse, &df );

	if( FAILED( hr ) )
	{
		Com_Printf( "Couldn't set DI mouse format\n" );
		return qfalse;
	}
	
	// set the cooperativity level.
	hr = IDirectInputDevice8_SetCooperativeLevel( g_pMouse,
		GetAncestor( g_wv.hWnd, GA_ROOTOWNER ),
		DISCL_EXCLUSIVE | DISCL_FOREGROUND );

	if( FAILED( hr ) )
	{
		Com_Printf( "Couldn't set DI coop level\n" );
		return qfalse;
	}


	// set the buffer size to DINPUT_BUFFERSIZE elements.
	// the buffer size is a DWORD property associated with the device
	hr = IDirectInputDevice8_SetProperty( g_pMouse, DIPROP_BUFFERSIZE, &dipdw.diph );

	if( FAILED( hr ) )
	{
		Com_Printf( "Couldn't set DI buffersize\n" );
		return qfalse;
	}

	Com_Printf( "DirectInput initialized.\n");
	return qtrue;
}

/*
==========================
IN_ShutdownDIMouse
==========================
*/
void IN_ShutdownDIMouse( void )
{
    if( g_pMouse )
	{
		IDirectInputDevice8_Release( g_pMouse );
		g_pMouse = NULL;
	}

    if( g_pdi )
	{
		IDirectInput8_Release( g_pdi );
		g_pdi = NULL;
	}
}

/*
==========================
IN_ActivateDIMouse
==========================
*/
void IN_ActivateDIMouse( void )
{
	HRESULT hr;

	if( !g_pMouse )
		return;

	hr = IDirectInputDevice8_Acquire( g_pMouse );
	if( FAILED( hr ) )
	{
		// we may fail to reacquire if the window has been recreated

		if( IN_InitDIMouse() )
			hr = IDirectInputDevice8_Acquire( g_pMouse );
	}

	if( FAILED( hr ) )
	{
		//failed and didn't recover...
		Com_Printf ("Falling back to Win32 mouse support...\n");
		Cvar_Set( "in_mouse", "-1" );
	}
}

/*
==========================
IN_DeactivateDIMouse
==========================
*/
void IN_DeactivateDIMouse( void )
{
	if( !g_pMouse )
		return;

	IDirectInputDevice8_Unacquire( g_pMouse );
}


/*
===================
IN_DIMouse
===================
*/
void IN_DIMouse( int *mx, int *my )
{
	HRESULT hr;

	*mx = 0;
	*my = 0;

	if( !g_pMouse )
		return;

	// fetch new events
	for( ; ; )
	{
		DWORD dwElements = 1;
		DIDEVICEOBJECTDATA od;

		hr = IDirectInputDevice8_GetDeviceData( g_pMouse,
			sizeof( DIDEVICEOBJECTDATA ), &od, &dwElements, 0 );
		
		if( hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED )
		{
			IDirectInputDevice_Acquire( g_pMouse );
			return;
		}

		if( FAILED( hr ) )
			//failed to read data or none available
			break;
		
		if( dwElements == 0 )
			break;

		switch( od.dwOfs )
		{
		case DIMOFS_BUTTON0:
			Sys_QueEvent( od.dwTimeStamp, SE_KEY, K_MOUSE1, od.dwData & 0x80 ? qtrue : qfalse, 0, NULL );
			break;

		case DIMOFS_BUTTON1:
			Sys_QueEvent( od.dwTimeStamp, SE_KEY, K_MOUSE2, od.dwData & 0x80 ? qtrue : qfalse, 0, NULL );
			break;
			
		case DIMOFS_BUTTON2:
			Sys_QueEvent( od.dwTimeStamp, SE_KEY, K_MOUSE3, od.dwData & 0x80 ? qtrue : qfalse, 0, NULL );
			break;

		case DIMOFS_BUTTON3:
			Sys_QueEvent( od.dwTimeStamp, SE_KEY, K_MOUSE4, od.dwData & 0x80 ? qtrue : qfalse, 0, NULL );
			break;

		case DIMOFS_X:
			*mx += (int)od.dwData;
			break;

		case DIMOFS_Y:
			*my += (int)od.dwData;
			break;
		
		case DIMOFS_Z:
			if( od.dwData )
			{
				if( (int)od.dwData < 0 )
				{
					Sys_QueEvent( od.dwTimeStamp, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
					Sys_QueEvent( od.dwTimeStamp, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
				}
				else
				{
					Sys_QueEvent( od.dwTimeStamp, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
					Sys_QueEvent( od.dwTimeStamp, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
				}
			}
			break;
		}
	}
}

/*
============================================================

  MOUSE CONTROL

============================================================
*/

/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse( void ) 
{
	if (!s_wmv.mouseInitialized ) {
		return;
	}
	if ( !in_mouse->integer ) 
	{
		s_wmv.mouseActive = qfalse;
		return;
	}
	if ( s_wmv.mouseActive ) 
	{
		return;
	}

	s_wmv.mouseActive = qtrue;

	IN_ActivateWin32Mouse();
	IN_ActivateDIMouse();
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse( void )
{
	if( !s_wmv.mouseInitialized )
		return;

	if( !s_wmv.mouseActive )
		return;

	s_wmv.mouseActive = qfalse;

	IN_DeactivateDIMouse();
	IN_DeactivateWin32Mouse();
}



/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse( void ) 
{
	s_wmv.mouseInitialized = qfalse;
	s_wmv.mouseStartupDelayed = qfalse;

	if ( in_mouse->integer == 0 ) {
		Com_Printf ("Mouse control not active.\n");
		return;
	}

	// nt4.0 direct input is screwed up
	if ( ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT ) &&
		( g_wv.osversion.dwMajorVersion == 4 ) )
	{
		Com_Printf ("Disallowing DirectInput on NT 4.0\n");
		Cvar_Set( "in_mouse", "-1" );
	}

	if ( in_mouse->integer == -1 ) {
		Com_Printf ("Skipping check for DirectInput\n");
	} else {
		if (!g_wv.hWnd)
		{
			Com_Printf ("No window for DirectInput mouse init, delaying\n");
			s_wmv.mouseStartupDelayed = qtrue;
			return;
		}
		if ( IN_InitDIMouse() ) {
			s_wmv.mouseInitialized = qtrue;
			return;
		}
		Com_Printf ("Falling back to Win32 mouse support...\n");
	}
	
	s_wmv.mouseInitialized = qtrue;
	IN_InitWin32Mouse();
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent( int mstate )
{
	int		i;

	if ( !s_wmv.mouseInitialized )
		return;

	// perform button actions
	for( i = 0; i < 3; i++ )
	{
		if ( (mstate & (1<<i)) &&
			!(s_wmv.oldButtonState & (1<<i)) )
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MOUSE1 + i, qtrue, 0, NULL );
		}

		if ( !(mstate & (1<<i)) &&
			(s_wmv.oldButtonState & (1<<i)) )
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MOUSE1 + i, qfalse, 0, NULL );
		}
	}	

	s_wmv.oldButtonState = mstate;
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove ( void ) {
	int		mx, my;

	if( g_pMouse )
		IN_DIMouse( &mx, &my );
	else
		IN_Win32Mouse( &mx, &my );

	if( !mx && !my )
		return;

	Sys_QueEvent( g_wv.sysMsgTime, SE_MOUSE_REL, mx, my, 0, NULL );
}


/*
=========================================================================

=========================================================================
*/

/*
===========
IN_Startup
===========
*/
void IN_Startup( void ) {
	Com_Printf ("\n------- Input Initialization -------\n");
	IN_StartupMouse ();
	IN_StartupJoystick();
	IN_StartupMIDI();
	Com_Printf ("------------------------------------\n");

	in_mouse->modified = qfalse;
	in_joystick->modified = qfalse;
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown( void ) {
	IN_DeactivateMouse();
	IN_ShutdownDIMouse();
	IN_ShutdownJoystick();
	IN_ShutdownMIDI();
	Cmd_RemoveCommand("midiinfo" );
}


/*
===========
IN_Init
===========
*/
void IN_Init( void ) {
	// MIDI input controler variables
	in_midi					= Cvar_Get ("in_midi",					"0",		CVAR_ARCHIVE);
	in_midiport				= Cvar_Get ("in_midiport",				"1",		CVAR_ARCHIVE);
	in_midichannel			= Cvar_Get ("in_midichannel",			"1",		CVAR_ARCHIVE);
	in_mididevice			= Cvar_Get ("in_mididevice",			"0",		CVAR_ARCHIVE);

	Cmd_AddCommand( "midiinfo", MidiInfo_f );

	// mouse variables
	in_mouse				= Cvar_Get ("in_mouse",					"1",		CVAR_ARCHIVE|CVAR_LATCH);
	in_logitechbug			= Cvar_Get ("in_logitechbug",			"0",		CVAR_ARCHIVE);

	// joystick variables
	in_joystick				= Cvar_Get ("in_joystick",				"0",		CVAR_ARCHIVE|CVAR_LATCH);
	in_joyBallScale			= Cvar_Get ("in_joyBallScale",			"0.02",		CVAR_ARCHIVE);
	in_debugJoystick		= Cvar_Get ("in_debugjoystick",			"0",		CVAR_TEMP);

	joy_threshold			= Cvar_Get ("joy_threshold",			"0.15",		CVAR_ARCHIVE);

	IN_Startup();
}


/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate (qboolean active) {
	in_appactive = active;

	if ( !active )
	{
		IN_DeactivateMouse();
	}
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void) {
	// post joystick events
	IN_JoyMove();

	if ( !s_wmv.mouseInitialized )
	{
		if (s_wmv.mouseStartupDelayed && g_wv.hWnd)
		{
			Com_Printf("Proceeding with delayed mouse init\n");
			IN_StartupMouse();
			IN_StartupJoystick ();
			s_wmv.mouseStartupDelayed = qfalse;
		}
		return;
	}

	if( cls.keyCatchers || (cls.state >= CA_CONNECTING && cls.state < CA_PRIMED) )
	{
		static POINT lastPoint = { 0x7FFFFFFF, 0x7FFFFFFF };
		POINT pt;

		IN_DeactivateMouse();

		// find mouse movement
		GetCursorPos( &pt );

		if( WindowFromPoint( pt ) == g_wv.hWnd ) {

			ScreenToClient( g_wv.hWnd, &pt );
		} else {

			pt.x = -1;
			pt.y = -1;
		}

		if( lastPoint.x != pt.x || lastPoint.y != pt.y ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_MOUSE_ABS, pt.x, pt.y, 0, NULL );
			lastPoint = pt;
		}

		return;
	}

	if ( !in_appactive || !cgvm ) {
		IN_DeactivateMouse ();
		return;
	}

	IN_ActivateMouse();

	// post events to the system que
	IN_MouseMove();
}

/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void) 
{
	s_wmv.oldButtonState = 0;
}


/*
=========================================================================

JOYSTICK

=========================================================================
*/

static BOOL __stdcall DevicesCallback( LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef ) {

	// Step 2: Creating the DirectInput Joystick Device
	HRESULT hr = IDirectInput8_CreateDevice( g_pdi, &lpddi->guidInstance, &g_pJoystick, NULL );
	if( FAILED(hr) ) 
		return DIENUM_CONTINUE;

	return DIENUM_STOP;
}
		
static BOOL __stdcall DeviceObjectsCallback( LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef ) {

	DIPROPDWORD deadzone = {
		{
			sizeof(DIPROPDWORD),	// diph.dwSize
			sizeof(DIPROPHEADER),	// diph.dwHeaderSize
			0,						// diph.dwObj
			DIPH_BYID,				// diph.dwHow
		},

		2000,						// dwData
	};
	DIPROPRANGE range = {
		{
			sizeof(DIPROPRANGE),	// diph.dwSize
			sizeof(DIPROPHEADER),	// diph.dwHeaderSize
			0,						// diph.dwObj
			DIPH_BYID,				// diph.dwHow
		},

		-128,						// lMin
		+128,						// lMax
	};

	HRESULT hr;

	deadzone.diph.dwObj		= lpddoi->dwType;
	range.diph.dwObj		= lpddoi->dwType;

	if ( FAILED(hr = IDirectInputDevice8_SetProperty( g_pJoystick, DIPROP_RANGE, &range.diph )) ) {
		return DIENUM_STOP;
	}

	if ( FAILED(hr = IDirectInputDevice8_SetProperty( g_pJoystick, DIPROP_DEADZONE, &deadzone.diph )) ) {
		return DIENUM_STOP;
	}

	return DIENUM_CONTINUE;
}

/* 
=============== 
IN_StartupJoystick 
=============== 
*/  
void IN_StartupJoystick (void) { 
	HRESULT		hr;

	// assume no joystick
	joy.avail = qfalse; 

	if (! in_joystick->integer ) {
		Com_Printf ("Joystick is not active.\n");
		return;
	}

	// Step 1: Enumerating the Joysticks
	if ( g_pdi ) {
		IDirectInput8_EnumDevices( g_pdi, DI8DEVCLASS_GAMECTRL, DevicesCallback, 0, DIEDFL_ATTACHEDONLY );
	}

	if ( !g_pJoystick ) {
		Com_Printf ("joystick not found -- driver not present\n");
		return;
	}

	// Step 3: Setting the Joystick Data Format
	if ( FAILED(hr = IDirectInputDevice8_SetDataFormat( g_pJoystick, &joydf )) ) {
		return;
	}

	// Step 4: Setting the Joystick Behaviour
	if ( FAILED(hr = IDirectInputDevice8_SetCooperativeLevel( g_pJoystick, g_wv.hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND )) ) {
		return;
	}

	joy.jc.dwSize = sizeof(DIDEVCAPS);
	if ( FAILED(hr = IDirectInputDevice8_GetCapabilities( g_pJoystick, &joy.jc )) ) {
		return;
	}

	if ( FAILED(hr = IDirectInputDevice8_EnumObjects( g_pJoystick, DeviceObjectsCallback, 0, DIDFT_AXIS )) ) {
		return;
	}

	Com_Printf( "Joystick found.\n" );
	//Com_Printf( "Pname: %s\n", joy.jc. dwFFDriverVersion szPname );
	//Com_Printf( "OemVxD: %s\n", joy.jc.szOEMVxD );
	//Com_Printf( "RegKey: %s\n", joy.jc.szRegKey );

	//Com_Printf( "Numbuttons: %i / %i\n", joy.jc.wNumButtons, joy.jc.wMaxButtons );
	//Com_Printf( "Axis: %i / %i\n", joy.jc.wNumAxes, joy.jc.wMaxAxes );
	//Com_Printf( "Caps: 0x%x\n", joy.jc.wCaps );
	//if ( joy.jc.wCaps & JOYCAPS_HASPOV ) {
	//	Com_Printf( "HASPOV\n" );
	//} else {
	//	Com_Printf( "no POV\n" );
	//}

	// mark the joystick as available
	joy.avail = qtrue; 
}

/* 
=============== 
IN_ShutdownJoystick 
=============== 
*/  
void IN_ShutdownJoystick (void) { 
	if ( g_pJoystick ) {
		IDirectInputDevice8_Unacquire( g_pJoystick );
	}
}



static const int	joyDirectionKeys[16] = {
	K_LEFTARROW, K_RIGHTARROW,
	K_JOY15, K_DOWNARROW,
	K_JOY16, K_JOY17,
	K_JOY18, K_JOY19,
	K_JOY20, K_JOY21,
	K_JOY22, K_JOY23,

	K_UPARROW, K_RIGHTARROW,
	K_DOWNARROW, K_LEFTARROW
};

/*
===========
IN_JoyMove
===========
*/
void IN_JoyMove( void ) {
	HRESULT hr;
	int i;

	// verify joystick is available and that the user wants to use it
	if ( !joy.avail ) {
		return; 
	}

	if ( FAILED(hr = IDirectInputDevice8_Poll( g_pJoystick )) ) {

		do {
			// Step 5: Gaining Access to the Joystick
			hr = IDirectInputDevice8_Acquire( g_pJoystick );
		} while ( hr == DIERR_INPUTLOST );
	}

	// Step 6: Retrieving Data from the Joystick
	if ( FAILED(hr = IDirectInputDevice8_GetDeviceState( g_pJoystick, sizeof(joyData_t), &joy.ji )) ) {
		return;
	}

	if ( in_debugJoystick->integer ) {
		int i;
		Com_Printf( "%5d\t%5d\t%5d\t%5d\n", joy.ji.x, joy.ji.y, joy.ji.u, joy.ji.v );
		for ( i=0; i<16; i++ ) {
			Com_Printf( "%5d", joy.ji.buttons[i] );
		}
		Com_Printf( "\n" );
	}

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	for ( i=0 ; i < lengthof(joy.ji.buttons) ; i++ ) {

		if ( joy.ji.buttons[i] != joy.buttons_old[i] ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, joyDirectionKeys[i], (joy.ji.buttons[i])?qtrue:qfalse, 0, NULL );
			joy.buttons_old[i] = joy.ji.buttons[i];
		}
	}

	// up-> Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_JOY1 + i, qtrue, 0, NULL );
	// down-> Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_JOY1 + i, qfalse, 0, NULL );

	// up->	Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, joyDirectionKeys[i], qtrue, 0, NULL );
	// down-> Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, joyDirectionKeys[i], qfalse, 0, NULL );

	Sys_QueEvent( g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_YAW,		-joy.ji.x, 0, NULL );
	Sys_QueEvent( g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_UP,		joy.ji.y, 0, NULL );
	Sys_QueEvent( g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_SIDE,		joy.ji.u, 0, NULL );
	Sys_QueEvent( g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_FORWARD,	-joy.ji.v, 0, NULL );
}

/*
=========================================================================

MIDI

=========================================================================
*/

static void MIDI_NoteOff( int note )
{
	int qkey;

	qkey = note - 60 + K_AUX1;

	if ( qkey > 255 || qkey < K_AUX1 )
		return;

	Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, qkey, qfalse, 0, NULL );
}

static void MIDI_NoteOn( int note, int velocity )
{
	int qkey;

	if ( velocity == 0 )
		MIDI_NoteOff( note );

	qkey = note - 60 + K_AUX1;

	if ( qkey > 255 || qkey < K_AUX1 )
		return;

	Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, qkey, qtrue, 0, NULL );
}

static void CALLBACK MidiInProc( HMIDIIN hMidiIn, UINT uMsg, DWORD dwInstance, 
								 DWORD dwParam1, DWORD dwParam2 )
{
	int message;

	switch ( uMsg )
	{
	case MIM_OPEN:
		break;
	case MIM_CLOSE:
		break;
	case MIM_DATA:
		message = dwParam1 & 0xff;

		// note on
		if ( ( message & 0xf0 ) == 0x90 )
		{
			if ( ( ( message & 0x0f ) + 1 ) == in_midichannel->integer )
				MIDI_NoteOn( ( dwParam1 & 0xff00 ) >> 8, ( dwParam1 & 0xff0000 ) >> 16 );
		}
		else if ( ( message & 0xf0 ) == 0x80 )
		{
			if ( ( ( message & 0x0f ) + 1 ) == in_midichannel->integer )
				MIDI_NoteOff( ( dwParam1 & 0xff00 ) >> 8 );
		}
		break;
	case MIM_LONGDATA:
		break;
	case MIM_ERROR:
		break;
	case MIM_LONGERROR:
		break;
	}

//	Sys_QueEvent( sys_msg_time, SE_KEY, wMsg, qtrue, 0, NULL );
}

static void Q_EXTERNAL_CALL MidiInfo_f( void )
{
	int i;

	const char *enableStrings[] = { "disabled", "enabled" };

	Com_Printf( "\nMIDI control:       %s\n", enableStrings[in_midi->integer != 0] );
	Com_Printf( "port:               %d\n", in_midiport->integer );
	Com_Printf( "channel:            %d\n", in_midichannel->integer );
	Com_Printf( "current device:     %d\n", in_mididevice->integer );
	Com_Printf( "number of devices:  %d\n", s_midiInfo.numDevices );
	for ( i = 0; i < s_midiInfo.numDevices; i++ )
	{
		if ( i == Cvar_VariableValue( "in_mididevice" ) )
			Com_Printf( "***" );
		else
			Com_Printf( "..." );
		Com_Printf(    "device %2d:       %s\n", i, s_midiInfo.caps[i].szPname );
		Com_Printf( "...manufacturer ID: 0x%hx\n", s_midiInfo.caps[i].wMid );
		Com_Printf( "...product ID:      0x%hx\n", s_midiInfo.caps[i].wPid );

		Com_Printf( "\n" );
	}
}

static void IN_StartupMIDI( void )
{
	int i;

	if ( !Cvar_VariableValue( "in_midi" ) )
		return;

	//
	// enumerate MIDI IN devices
	//
	s_midiInfo.numDevices = midiInGetNumDevs();

	for ( i = 0; i < s_midiInfo.numDevices; i++ )
	{
		midiInGetDevCaps( i, &s_midiInfo.caps[i], sizeof( s_midiInfo.caps[i] ) );
	}

	//
	// open the MIDI IN port
	//
	if ( midiInOpen( &s_midiInfo.hMidiIn, 
		             in_mididevice->integer,
					 ( unsigned long ) MidiInProc,
					 ( unsigned long ) NULL,
					 CALLBACK_FUNCTION ) != MMSYSERR_NOERROR )
	{
		Com_Printf( "WARNING: could not open MIDI device %d: '%s'\n", in_mididevice->integer , s_midiInfo.caps[( int ) in_mididevice->value] );
		return;
	}

	midiInStart( s_midiInfo.hMidiIn );
}

static void IN_ShutdownMIDI( void )
{
	if ( s_midiInfo.hMidiIn )
	{
		midiInClose( s_midiInfo.hMidiIn );
	}
	Com_Memset( &s_midiInfo, 0, sizeof( s_midiInfo ) );
}
