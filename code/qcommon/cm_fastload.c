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

#include "cm_local.h"

static void* g_data[2];

void CM_PreloadFastMapData( const char *basePath )
{
	fileHandle_t fh;
	int len;

	len = FS_FOpenFileRead( va( "%s.bspf", basePath ), &fh, qfalse );

	if( len > 0 )
	{
		void *buf = Hunk_Alloc( len, h_high );
		FS_Read( buf, len, fh );
		FS_FCloseFile( fh );

		g_data[CM_FDB_BSPF] = buf;
	}

	len = FS_FOpenFileRead( va( "%s.aas", basePath ), &fh, qfalse );
	
	if( len > 0 )
	{
		void *buf = Hunk_Alloc( len, h_high );
		FS_Read( buf, len, fh );
		FS_FCloseFile( fh );

		g_data[CM_FDB_AAS] = buf;
	}
}

void CM_ClearPreloaderData( void )
{
	memset( g_data, 0, sizeof( g_data ) );
}

void *CM_GetPreloadedData( cmFastDataBlock_t data )
{
	return g_data[data];
}