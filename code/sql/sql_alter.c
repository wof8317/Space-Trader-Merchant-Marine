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



/*
===============
sql_alter_parse

parses an alter statement.

`ALTER TABLE <table> ADD <columns>;`
===============
*/
stmtInfo_t * sql_alter_parse( sqlInfo_t * db, const char ** data_p )
{
	const char * s = (char*)*data_p;
	tableInfo_t * table;
	char * n;

	//	skip 'TABLE'
	parse_temp( &s );

	n = parse_temp( &s );
	table = find_table( db, n );

	if ( !table ) {
		Com_Error( ERR_FATAL, "table '%s' does not exist.", n );
	}

	n = parse_temp( &s );
	switch( SWITCHSTRING(n) )
	{
	case CS('a','d','d',0):
		{
			int count = table->column_count;
			sql_table_addcolumns( db, table, &s );

			//	resize rows
			if ( table->row_count > 0 ) {
				int i;
				cellInfo_t * rows = sql_alloc( db, table->row_total * table->column_count * sizeof(cellInfo_t) );

				//	copy rows into new table
				for ( i=0; i<table->row_count; i++ ) {
					memcpy( &rows[ i*table->column_count ], &table->rows[ i*count ], sizeof(cellInfo_t) * count );
					memset( &rows[ (i*table->column_count) + count ], 0, sizeof(cellInfo_t) * ( table->column_count-count ) );
				}

				sql_free( db, table->rows );
				table->rows = rows;
			}

		} break;
	}

	if ( table->sync ) {
		selectInfo_t *	select = (selectInfo_t*)table->sync;
		if ( select->entire_row ) {
			int i;
			select->column_count = table->column_count;
			for ( i=0; i<table->column_count; i++ ) {
				select->column_type[ i ] = table->columns[ i ].format;
				select->column_name[ i ] = table->columns[ i ].name;
			}
		}
	}

	return 0;
}
