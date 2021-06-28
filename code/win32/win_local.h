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
// win_local.h: Win32-specific Quake3 header file

#ifndef __win_local_h_
#define __win_local_h_

#include "platform/pop_segs.h"

#if defined( _MSC_VER ) && (_MSC_VER >= 1200)
#pragma warning( disable : 4201 ) //nameless struct/union
#pragma warning( push )
#endif

#define _WIN32_WINNT 0x400

#ifdef APIENTRY
#undef APIENTRY
#endif

#include <windows.h>
#include <windowsx.h>

#if defined( _MSC_VER ) && (_MSC_VER >= 1200)
#pragma warning( pop )
#endif

#ifndef BUILD_RENDER

#define	DIRECTSOUND_VERSION	0x0300
#include <dsound.h>

#define	DIRECTINPUT_VERSION	0x0800
#include <dinput.h>

#include <Iphlpapi.h>
#include <winsock.h>
#include <wsipx.h>

#endif //BUILD_RENDER

#include "platform/push_segs.h"

void	IN_MouseEvent (int mstate);

void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );

void	Sys_CreateConsole( void );
void	Sys_DestroyConsole( void );

char	*Sys_ConsoleInput (void);

qboolean	Sys_GetPacket ( netadr_t *net_from, msg_t *net_message );

// Input subsystem

void	IN_Init (void);
void	IN_Shutdown (void);
void	IN_JoystickCommands (void);

void	IN_Move (usercmd_t *cmd);
// add additional non keyboard / non mouse movement on top of the keyboard move cmd

void	IN_DeactivateWin32Mouse( void);

void	IN_Activate (qboolean active);
void	IN_Frame (void);

// window procedure
LONG WINAPI MainWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam);

void Conbuf_AppendText( const char *msg );

void SNDDMA_Activate( void );
int  SNDDMA_InitDS (void);

typedef struct
{
	
	HINSTANCE		reflib_library;		// Handle to refresh DLL 
	qboolean		reflib_active;

	HWND			hWnd, hWndHost;
	HINSTANCE		hInstance;
	qboolean		activeApp;
	qboolean		isMinimized;
	OSVERSIONINFO	osversion;

	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event
	unsigned		sysMsgTime;

	HCURSOR			cursor;

} WinVars_t;

#ifndef BUILD_RENDER //must not reference inside render!
extern WinVars_t g_wv;
#endif

#define WM_SHOWCURSOR WM_USER+1

#include "win_host.h"

#endif