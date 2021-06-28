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
#include "../qcommon/q_shared.h"
#include "bg_spacetrader.h"

#ifdef _MSC_VER
	#ifdef _DEBUG 
		#define STASSERT(e) if (!(e)) { Com_Error( ERR_FATAL, "assert (%s) failed at %s:%d\n", #e, __FILE__,__LINE__ ); }
	#else
		#define STASSERT(e) __assume(e)
	#endif
#else
	#define STASSERT(e) if (!(e)) { Com_Error( ERR_FATAL, "assert (%s) failed at %s:%d\n", #e, __FILE__,__LINE__ ); }
#endif

extern void			trap_SQL_Exec( const char* statement );
extern qhandle_t	trap_SQL_Prepare( const char* statement );

extern void			trap_SQL_BindText( int i, const char* text );
extern void			trap_SQL_BindInt( int i, int v );
extern qboolean		trap_SQL_Step();

extern int			trap_SQL_ColumnCount();
extern void			trap_SQL_ColumnAsText( char * buffer, int size, int i );
extern int			trap_SQL_ColumnAsInt( int i );
extern void			trap_SQL_ColumnName( char * buffer, int size, int i );

extern int			trap_SQL_Done();

int sqldone( void )
{
	return trap_SQL_Done();
}

const char * sqltext( int column )
{
	//	todo : try to eliminate some of these copies!1
	char buffer[ MAX_INFO_STRING ];
	trap_SQL_ColumnAsText( buffer, sizeof(buffer), column );
	return va( "%s", buffer );
}

int sqlint( int column )
{
	return trap_SQL_ColumnAsInt( column );
}

int sqlstep()
{
	return trap_SQL_Step();
}


int sql( const char * stmt, const char * argdef, ... )
{
	trap_SQL_Prepare( stmt );

	if ( argdef )
	{
		int column = -1;
		int i;
		va_list	argptr;
		va_start( argptr, argdef );

		for ( i=0; argdef[ i ] != '\0'; i++ )
		{
			int c = argdef[ i ];

			switch ( c )
			{
			case 'i':
				trap_SQL_BindInt( i+1, va_arg( argptr, int ) );
				break;

			case 't':
				trap_SQL_BindText( i+1, va_arg( argptr, const char * ) );
				break;

			case 'e':
				trap_SQL_Step();
				return trap_SQL_Done();

			case 's':
				if ( !trap_SQL_Step() )
				{
					trap_SQL_Done();
					return 0;
				}
				column = 0;
				break;

			case 'd':
				trap_SQL_Done();
				return 1;

			case 'I':
				{
					int * r = va_arg( argptr, int * );
					*r = trap_SQL_ColumnAsInt( column++ );
				} break;

			case 'T':
				{
					const char** r = va_arg( argptr, const char** );
					*r = sqltext( column++ );
				}
			}
		}

		va_end( argptr );
	}

	if ( trap_SQL_Step() == 1 )
		return 1;

	return trap_SQL_Done();
}

static int find_range( int low, int high, int price, int rangeCount )
{
	int range;

	STASSERT( low > 0 && low < high && high > low	);
	STASSERT( price >= low && price <= high			);
	STASSERT( rangeCount >= 3 && rangeCount <= 16	);

	range = (price * rangeCount) / (high-low);

	STASSERT( range >= 0 && range <= rangeCount		);
	return range;
}

int BG_ST_GetMissionInt(const char *key)
{
	int value;
	if (sql( "SELECT missions.key[ $ ].value;", "tsId", key, &value) ) {
		return value;
	} else {
		Com_Error(ERR_FATAL, "Key: %s not found in missions table", key );
		return 0;
	}
}

const char * BG_ST_GetMissionString(const char *key)
{
	if (sql( "SELECT missions.key[ $ ]^value;", "t", key))
	{
		char *s;
		s = va("%s", sqltext( 0 ));
		sqldone();
		return s;
	} else {
		Com_Error(ERR_FATAL, "Key: %s not found in missions table", key );
	}
	return NULL;
}

void BG_ST_GetMissionStringBuffer( const char * key, char * buffer, int size )
{
	if ( sql( "SELECT missions.key[ $ ]^value;", "t", key ) )
	{
		Q_strncpyz( buffer, sqltext(0), size );
		sqldone();
	} else {
		Com_Error(ERR_FATAL, "Key: %s not found in missions table", key );
	}
}



void BG_ST_ReadState( const char * info, globalState_t * gs)
{
	int i;
	const char * s = info;
	for ( i=0; i<sizeof(globalState_t) / sizeof(int); i++ )
		((int*)gs)[ i ] = atoi( COM_ParseExt( &s, qfalse ) );
}

void BG_ST_WriteState( char * info, int size, globalState_t * gs )
{
	int i;
	info[ 0 ] = '\0';

	for ( i=0; i<sizeof(globalState_t) / sizeof(int); i++ )
	{
		Q_strcat( info, size, va("%d ", ((int*)gs)[ i ] ) );
	}
}

