/*
===========================================================================
Copyright (C) 2006 HermitWorks Entertainment Corporation

This file is part of Space Trader source code.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
===========================================================================
*/

#define ST_START_TIME -50	// the day that the stock market starts record


extern void			BG_ST_CreateTables		( const char * schema );

extern int			BG_ST_GetMissionInt		( const char *key );
extern const char *	BG_ST_GetMissionString	( const char *key );
extern void			BG_ST_GetMissionStringBuffer( const char * key, char * buffer, int size );


extern void			BG_ST_ReadState			( const char * info, globalState_t * gs );
extern void			BG_ST_WriteState		( char * info, int size, globalState_t * gs );
