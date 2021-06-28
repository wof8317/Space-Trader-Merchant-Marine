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


stmtInfo_t * sql_update_parse( sqlInfo_t * db, const char ** s )
{
	updateInfo_t * update = sql_calloc_stmt( db, sizeof(updateInfo_t) );
	columnInfo_t * c;
	parseInfo_t	pi = { 0 };
	char * tableName;

	update->stmt.type	= SQL_UPDATE;

	tableName = parse_temp( s );

	if ( SWITCHSTRING( tableName ) == CS('o','r',0,0 ) ) {
		parse_temp( s );	// INSERT
		update->orinsert = 1;
		tableName = parse_temp( s );
	}

	update->stmt.table	= find_table( db, tableName );

#ifdef DEVELOPER
	if ( !update->stmt.table ) {
		Com_Error( ERR_FATAL, "table '%s' does not exist.\n\n%s\n", tableName, CURRENT_STMT );
	}
#endif

	// 'SET'
	switch( SWITCHSTRING( parse_temp( s ) ) ) {
		case CS('s','e','t',0):	pi.flags = PARSE_ASSIGN_TO_COLUMN;	break;
		case CS('c','s',0,0):	pi.flags = PARSE_ASSIGN_CS;			break;
		default:
			ASSERT(0);
	}

	update->search.column = -1;
	update->assignmentCount = 0;

	pi.db		= db;
	pi.params	= update->stmt.params;
	pi.table	= update->stmt.table;

	do {
		if ( pi.flags & PARSE_ASSIGN_TO_COLUMN ) {
			char * n = parse_temp( s );
			c = find_column( update->stmt.table, n );
#ifdef DEVELOPER
			if ( !c ) {
				Com_Error( ERR_FATAL, "cannot find column '%s' on table '%s'.\n", n, update->stmt.table->name );
			}
#endif
			parse_temp( s );	// '='
			pi.column	= c->num;
		}

		update->assignments[ update->assignmentCount++ ] = parse_expression( s, &pi );

	} while ( pi.more );
	
	pi.column	= 0;
	pi.flags	= 0;

	for ( ;; )
	{
		char * t = parse_temp(s);
		if ( t[0] == '\0' )
			break;

		switch( SWITCHSTRING(t) )
		{
		case CS('w','h','e','r'):	update->where_expr	= parse_expression( s, &pi );	break;
		case CS('l','i','m','i'):	update->limit		= parse_expression( s, &pi );	break;
		case CS('o','f','f','s'):	update->offset		= parse_expression( s, &pi );	break;
		case CS('s','e','a','r'):
			{
				columnInfo_t * c = find_column( update->stmt.table, parse_temp( s ) );
				ASSERT( c );
				update->search.column = c->num;

				update->search.start = parse_expression( s, &pi );
				if ( pi.more )
					update->search.end = parse_expression( s, &pi );

			} break;
		case CS('d','e','s','c'):
			{
				update->search.descending = 1;
			} break;

		}
	}

	return &update->stmt;
}

int sql_update_work( sqlInfo_t * db, updateInfo_t * update )
{
	int i;
	cellInfo_t *	rows[ 512 ];
	int				modified[ 512 ];
	int limit, offset;
	stmtInfo_t * stmt = &update->stmt;
	stmtType_t op;

	//	setup bounds
	if ( update->limit ) {
		limit = sql_eval( db, update->limit, stmt->table, 0,0,0, stmt->params, 0 ).integer;
		limit = min( lengthof( rows ), limit );
	} else
		limit = lengthof( rows );

	if ( update->offset ) {
		offset = sql_eval( db, update->offset, stmt->table, 0,0,0, stmt->params, 0 ).integer;
	} else
		offset = 0;

	update->rows_updated = sql_eval_search_and_where( db, &update->search, update->where_expr, stmt->table, stmt->params, rows, limit, offset );

	//	check to see if this update is actually an insert
	if ( update->rows_updated == 0 && update->orinsert ) {
		rows[ 0 ] = sql_insert_begin( db, stmt->table );
		update->rows_updated = 1;
		op = SQL_INSERT;
	} else {
		op = SQL_UPDATE;
	}

	//	modify the rows
	for ( i=0; i<update->rows_updated; i++ )
	{
		int j;
		stmt->table->modified = 0;

		for ( j=0; j<update->assignmentCount; j++ ) {
			sql_eval( db, update->assignments[ j ], stmt->table, rows[ i ],i,update->rows_updated, stmt->params, 0 );
		}

		if ( op == SQL_UPDATE ) {
			modified[ i ] = stmt->table->modified;
		}
	}

	//	make callbacks
	if ( op == SQL_UPDATE ) {

		if ( update->rows_updated ) {
			stmt->table->last_changed++;

			if ( db->update_trigger ) {
				for ( i=0; i<update->rows_updated; i++ ) {

					// don't send update to clients if the row actually hasn't been modified.


					// !! this is turned off right now because this command is not gaurenteed to make it
					// across to the client.  the game only works because the commands that fail to make
					// it across must some how be sent again later.


					if( modified[ i ] )
					{
						db->update_trigger( db, stmt->table, (rows[ i ] - stmt->table->rows) / stmt->table->column_count );
					}
				}
			}
		}

	} else {
		sql_insert_done( db, stmt->table );
	}

	return 1;
}
