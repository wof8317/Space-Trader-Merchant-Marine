/*
===========================================================================
Copyright (C) 2007 HermitWorks Entertainment Corporation

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
#include "../qcommon/qcommon.h"

#include "sql.h"



void MSG_WriteTables( int client, msg_t * msg, struct sqlInfo_s * db ) {

	int i,j,k;
	for ( i=0; i<db->table_count; i++ ) {

		tableInfo_t *	t		= db->tables[ i ];
		selectInfo_t *	select	= (selectInfo_t*)t->sync;
		cellInfo_t *	cells;

		if ( select ) {

			int cn,rn;

			MSG_WriteByte	( msg, svc_table );
			MSG_WriteString	( msg, t->name );

			//	bind client
			select->stmt.params[ 0 ].format = INTEGER;
			select->stmt.params[ 0 ].payload.integer = client;

			//	get client's copy of table
			sql_select_work( db, select );

			cn = select->column_count;
			rn = select->cache.row_count;

			cells = select->cache.result;

			MSG_WriteByte	( msg, cn );
			MSG_WriteShort	( msg, rn );

			for ( j=0; j<cn; j++ ) {

				dataFormat_t df = select->column_type[ j ];

				MSG_WriteBits	( msg, (df == STRING)?1:0, 1 );
				MSG_WriteString	( msg, select->column_name[ j ] );

				if ( df == STRING ) {
					for ( k=0; k<rn; k++ ) {
						MSG_WriteString( msg, cells[ k*cn + j ].string );
					}
				} else {
					for ( k=0; k<rn; k++ ) {
						MSG_WriteLong( msg, cells[ k*cn + j ].integer );
					}
				}
			}

			sql_cache_pop( db, select->cache.result );
		}
	}
}

void MSG_WriteAllTables( msg_t * msg, struct sqlInfo_s * db ) {

	int i,j,k,cn,rn;
	for ( i=0; i<db->table_count; i++ ) {

		tableInfo_t *	t		= db->tables[ i ];
		cellInfo_t *	cells;

		if ( !(t->flags&SQL_TABLE_FROMSERVER) )
			continue;

		MSG_WriteByte	( msg, svc_table );
		MSG_WriteString	( msg, t->name );

		cn = t->column_count;
		rn = t->row_count;

		cells = t->rows;

		MSG_WriteByte	( msg, cn );
		MSG_WriteShort	( msg, rn );

		for ( j=0; j<cn; j++ ) {

			dataFormat_t df = t->columns[ j ].format;

			MSG_WriteBits	( msg, (df == STRING)?1:0, 1 );
			MSG_WriteString	( msg, t->columns[ j ].name );

			if ( df == STRING ) {
				for ( k=0; k<rn; k++ ) {
					MSG_WriteString( msg, cells[ k*cn + j ].string );
				}
			} else {
				for ( k=0; k<rn; k++ ) {
					MSG_WriteLong( msg, cells[ k*cn + j ].integer );
				}
			}
		}
	}
}

char * MSG_ReadTable( msg_t * msg, struct sqlInfo_s * db ) {

	int i, j;

	char * tableName = MSG_ReadString( msg );
	tableInfo_t * table = find_table( db, tableName );

	if ( !table ) {

		//	create new table
		table = sql_calloc( db, sizeof(tableInfo_t) );
		table->name = sql_alloc_string( db, tableName );

		insert_table( db, table );

	} else {

		sql_free( db, table->rows );
	}
	
	table->flags |= SQL_TABLE_FROMSERVER;

	table->column_count	= MSG_ReadByte	( msg );
	table->row_total	=
	table->row_count	= MSG_ReadShort	( msg );
	table->rows			= (table->row_total)?sql_alloc( db, sizeof(int) * table->column_count * table->row_total ):0;

	for ( i=0; i<table->column_count; i++ ) {
		columnInfo_t * c = table->columns + i;

		if ( c->index ) {
			sql_free( db, c->index );
		}

		c->format	= (MSG_ReadBits( msg, 1 ) == 1)?STRING:INTEGER;
		c->index	= 0;
		c->name		= sql_alloc_string( db, MSG_ReadString( msg ) );
		c->num = i;

		if ( c->format == STRING ) {

			for ( j=0; j<table->row_count; j++ ) {
				table->rows[ table->column_count*j + i ].string = sql_alloc_string( db, MSG_ReadString( msg ) );
			}

		} else {

			for ( j=0; j<table->row_count; j++ ) {
				table->rows[ table->column_count*j + i ].integer = MSG_ReadLong( msg );
			}
		}
	}

	table->last_changed++;

	return tableName;
}
