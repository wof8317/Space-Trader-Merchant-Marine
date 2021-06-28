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


stmtInfo_t * sql_delete_parse( sqlInfo_t * db, const char ** s )
{
	deleteInfo_t * del = sql_calloc_stmt( db, sizeof(deleteInfo_t) );
	del->stmt.type	= SQL_DELETE;

	//	'FROM'
	if ( SWITCHSTRING( parse_temp( s ) ) == CS('f','r','o','m') ) {

		del->stmt.table	= find_table( db, parse_temp( s ) );
	} else
		return 0;

	
	switch ( SWITCHSTRING( parse_temp( s ) ) ) {
		//	'SEARCH'
		case CS('s','e','a','r'):
			Com_Error( ERR_FATAL, "DELETE statement does not support SEARCH.\n" );
			break;
		//	'WHERE'
		case CS('w','h','e','r'):
			{
				parseInfo_t pi = { 0 };
				pi.db		= db;
				pi.table	= del->stmt.table;
				pi.params	= del->stmt.params;

				del->where_expr = parse_expression( s, &pi );
			} break;
	}

	return &del->stmt;
}


int sql_delete_work( sqlInfo_t * db, deleteInfo_t * del )
{
	tableInfo_t * table = del->stmt.table;
	int i;

	if ( !table || table->row_count == 0 )
		return 1;

	if ( del->where_expr )
	{
		const int lastrow = table->row_count-1;
		for ( i=lastrow; i>=0; i-- ) {

			cellInfo_t * row = table->rows + (i*table->column_count);

			if ( sql_eval( db, del->where_expr, table, row,0,0, del->stmt.params, 0 ).integer != 0 ) {
				if ( db->delete_trigger )
					db->delete_trigger( db, table, i );

				memmove( row, row + table->column_count, (lastrow-i) * table->column_count * sizeof(cellInfo_t) );
				table->row_count--;
				table->last_changed++;
			}
		}

	} else {
		table->row_count = 0;

		if ( db->delete_trigger )
			db->delete_trigger( db, table, -1 );
	}


	sql_table_reindex( table );


	return 1;
}
