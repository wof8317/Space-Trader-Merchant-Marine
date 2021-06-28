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

#ifndef __ui_anim_h_
#define __ui_anim_h_

typedef enum
{
	EF_DELETEME			= 0x0000001,
	EF_INFOCUS			= 0x0000002,
	EF_NOFOCUS			= 0x0000004,
	EF_NOANIM			= 0x0000008,
	EF_NORECT			= 0x0000010,
	EF_FOREVER			= 0x0000020,

	/*
		Various types of "one frame" flags.

		These flags are set to queue up behaviors that must be cancelled
		each frame or something will happen - this is set up clients that
		don't have a definite concept of "stop drawing this now" and to
		allow for exit animation control in the effectDef rather than in
		the client.

		These flags may *not* be combined with each other (though they may
		freely combine with other, non ONEFRAME, flags).

		EF_ONEFRAME_NORECT will set EF_NORECT each frame after drawing.
		EF_ONEFRAME_NOFOCUS will set EF_NOFOCUS each frame after drawing.

		I'm reserving a nibble for all the ONEFRAME types. Heh. "Nibble".
	*/
	EF_ONEFRAME_BITS	= 0x0F00000,
	EF_ONEFRAME_RECT	= 0x0100000,
	EF_ONEFRAME_NOFOCUS	= 0x0200000,
} effectFlag_t;

void			UI_Effect_Init			( void );
effectDef_t*	UI_Effect_Find			( const char * name );
qhandle_t	UI_Effect_SpawnText		( rectDef_t * r, effectDef_t * effect, const char * text, qhandle_t shader );
void			UI_Effect_SetFlag		( qhandle_t h, int flags );
void			UI_Effect_ClearFlag		( qhandle_t h, int flags );
int			UI_Effect_GetFlags		( qhandle_t h );
int			UI_Effect_GetTouchCount	( qhandle_t h );
int			UI_Effect_SetTouchCount	( qhandle_t h, int newTouch );
void			UI_Effects_UpdateToolTip( itemDef_t * item );
void			UI_Effect_SetRect		( qhandle_t h, rectDef_t * r, itemDef_t * item );
void			UI_Effect_SetJustRect	( qhandle_t h, rectDef_t * r );
void			UI_Effects_Update		();
void			UI_Effects_Draw		( menuDef_t * menu );
qboolean		UI_Effect_IsAlive		( qhandle_t h );
effectDef_t*	UI_Effect_GetEffect		( qhandle_t h );

#endif
