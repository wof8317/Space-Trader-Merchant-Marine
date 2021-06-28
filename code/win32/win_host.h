/*
===========================================================================
Copyright (C) 2006 Hermitworks Entertainment Corp

This file is part of Space Trader source code.

Space Trader source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Space Trader source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Space Trader source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef GAME_HOST_H_
#define GAME_HOST_H_

#ifdef _MSC_VER
#define GAME_CALL __fastcall
#else
#define GAME_CALL
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************

	These types and functions are exported from the game to the game host.

******************************************************************************/
int GAME_CALL Game_Main( HINSTANCE hInstance, HWND hWnd, LPSTR lpCmdLine, int nCmdShow );

void GAME_CALL Game_GetExitState( int *retCode, char *exit_msg,
	size_t msg_buff_len );

/******************************************************************************

	These functions will be called by the game. The game host needs to
	implement all of them (most can be stubbed out).

******************************************************************************/
void GAME_CALL Host_RecordError( const char *msg );

#ifdef __cplusplus
}
#endif

#endif