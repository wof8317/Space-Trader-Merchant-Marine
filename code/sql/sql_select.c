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


//	"SELECT id FROM npcs WHERE name like ? LIMIT 1;", "t", v
stmtInfo_t * sql_select_parse( sqlInfo_t * db, const char ** s )
{
	char tmp[ 128 ];
	int lit;

	//	parse select statements
	parseInfo_t	pi = { 0 };
	const char * scan;
	selectInfo_t * select = sql_calloc_stmt( db, sizeof(selectInfo_t) );

	select->stmt.type = SQL_SELECT;

	//	find which table this statement is selecting from, need to find it first...
	for ( scan=*s;scan; ) {
		lit = 0;
		parse( &scan, tmp, sizeof(tmp), &lit );
		if ( lit )
			continue;
		switch( SWITCHSTRING( tmp ) ) {
			case CS('f','r','o','m'):
				{
					char * n = parse_temp( &scan );
					if ( scan[ 0 ] == '.' ) {
						sqlInfo_t * edb;
						switch( SWITCHSTRING( n ) )
						{
#ifndef DEDICATED			
							case CS('c','l','i','e'):	edb = sql_getclientdb();	break;			
#endif
							case CS('s','e','r','v'):	edb = sql_getserverdb();	break;
							case CS('c','o','m','m'):	edb = sql_getcommondb();	break;
							default:
								edb = 0;
						}

						if ( edb ) {
							scan++;
							n = parse_temp( &scan );
							select->stmt.table = find_table( edb, n );
						}

					} else {
						select->stmt.table = find_table( db, n );
					}
					if ( !select->stmt.table ) {
#ifdef DEVELOPER
						Com_Error( ERR_FATAL, "table '%s' does not exist.\n\n%s", n, CURRENT_STMT );
#else
						return NULL;
#endif
					}
					scan = 0;
				} break;
			case CS(';',0,0,0):
			case 0:
				scan = 0;
				break;
		}
	}

	select->search.column = -1;
	select->column_count = 0;

	pi.db		= db;
	pi.params	= select->stmt.params;
	pi.table	= select->stmt.table;

	do {
		pi.as = &select->column_name[ select->column_count ];

		select->column_expr[ select->column_count ] = parse_expression( s, &pi );
		select->is_aggregate = ( pi.rt == AGGREGATE );
		if ( pi.rt == ROW ) {
			int i;
#ifdef DEVELOPER
			if ( !select->stmt.table ) {
				Com_Error( ERR_FATAL, "select statement expected table.\n\n%s\n", CURRENT_STMT );
			}
#endif
			select->entire_row = 1;
			select->column_count = select->stmt.table->column_count;
			for ( i=0; i<select->stmt.table->column_count; i++ ) {
				select->column_type[ i ] = select->stmt.table->columns[ i ].format;
				select->column_name[ i ] = select->stmt.table->columns[ i ].name;
			}
		} else {
#ifdef DEVELOPER
			if ( select->column_expr[ select->column_count ] == 0 ) {
				Com_Error( ERR_FATAL, "invalid column (%d) in select statement.\n\n%s\n", select->column_count, CURRENT_STMT );
			}
#endif
			select->column_type[ select->column_count ] = pi.rt;
			select->column_count++;
		}

	} while ( pi.more );

	pi.as		= 0;

	for ( ;; )
	{
		lit = 0;
		parse( s, tmp, sizeof(tmp), &lit );
		if ( lit )
			continue;

		if ( tmp[0] == '\0' )
			break;

		switch( SWITCHSTRING(tmp) )
		{
		case CS('f','r','o','m'):	parse_temp(s);	break;
		case CS('w','h','e','r'):	select->where_expr	= parse_expression( s, &pi );	break;
		case CS('l','i','m','i'):	select->limit		= parse_expression( s, &pi );	break;
		case CS('o','f','f','s'):	select->offset		= parse_expression( s, &pi );	break;
		case CS('r','a','n','d'):	select->random		= parse_expression( s, &pi );	break;
		case CS('s','o','r','t'):	select->sort		= parse_expression( s, &pi );	break;
		case CS('s','e','a','r'):
			{
				char * n = parse_temp( s );
				columnInfo_t * c = find_column( select->stmt.table, n );

				if ( !c ) {
#ifdef DEVELOPER
					Com_Error( ERR_FATAL, "expected column '%s' for SEARCH on table '%s'.\n\n%s\n", n, select->stmt.table->name, CURRENT_STMT );
#else
					return NULL;
#endif
				}
				select->search.column	= c->num;
				select->search.start = parse_expression( s, &pi );

				if ( pi.more )
					select->search.end = parse_expression( s, &pi );

			} break;
		case CS('d','e','s','c'):
			{
				select->search.descending = 1;
			} break;
		case CS('u','n','i','q'):
			{
				select->search.unique = 1;
			} break;
		}
	}

	return &select->stmt;
}

static int sort_column;
static int qvm_offset;
static int QDECL sorttable_integer_asc( const void *a, const void *b ) {
	cellInfo_t * r1 = (cellInfo_t*)a;
	cellInfo_t * r2 = (cellInfo_t*)b;

	return r1[ sort_column ].integer - r2[ sort_column ].integer;
}
static int QDECL sorttable_integer_dec( const void *a, const void *b ) {
	cellInfo_t * r1 = (cellInfo_t*)a;
	cellInfo_t * r2 = (cellInfo_t*)b;

	return r2[ sort_column ].integer - r1[ sort_column ].integer;
}
static int QDECL sorttable_string_asc( const void *a, const void *b ) {
	cellInfo_t * r1 = (cellInfo_t*)a;
	cellInfo_t * r2 = (cellInfo_t*)b;

	//subtract segment to get into QVM address space
	return Q_stricmp( r1[ sort_column ].string-qvm_offset, r2[ sort_column ].string-qvm_offset );
}
static int QDECL sorttable_string_dec( const void *a, const void *b ) {
	cellInfo_t * r1 = (cellInfo_t*)a;
	cellInfo_t * r2 = (cellInfo_t*)b;
	
	//subtract segment to get into QVM address space
	return Q_stricmp( r2[ sort_column ].string-qvm_offset, r1[ sort_column ].string-qvm_offset );
}

int sql_select_work( sqlInfo_t * db, selectInfo_t * select )
{
	int				row_count = 0;
	int				limit, offset;
	stmtInfo_t *	stmt = &select->stmt;
	tableInfo_t *	table = stmt->table;
	cellInfo_t*		rows[ 4096 ];

	select->cache.row_count = 0;

	if ( table )
	{
		//	setup bounds
		if ( table->row_count > 0 ) {

			if ( select->limit ) {
				limit = sql_eval( db, select->limit, stmt->table, 0,0,0, stmt->params, 0 ).integer;
				limit = min( lengthof( rows ), limit );
			} else
				limit = lengthof( rows );

			if ( select->offset ) {
				offset = sql_eval( db, select->offset, stmt->table, 0,0,0, stmt->params, 0 ).integer;
			} else
				offset = 0;

			//	search and filter
			row_count = sql_eval_search_and_where( db, &select->search, select->where_expr, stmt->table, stmt->params, rows, limit, offset );

			//	select some rows at random
			if ( row_count && select->random ) {
				cellInfo_t * row;
				int	n = min( row_count, sql_eval( db, select->random, stmt->table, 0,0,row_count, stmt->params, 0 ).integer );

				if ( row_count > 1 ) {
					int i,r;
					for( i = 0; i < n; i++ )
					{
						r = Rand_NextInt32InRange( &db->rand, i, row_count );
						row = rows[ r ];
						rows[ r ] = rows[ i ];
						rows[ i ] = row;
					}
				}

				row_count = n;
			}
		}

		if ( row_count == 0 && !select->is_aggregate )
			return 1;

	} else {
		row_count = 1;
		rows[ 0 ] = 0;
	}


	//	do work
	if ( select->is_aggregate ) {

		int i,j;

		select->cache.result	= sql_cache_push	( db, sizeof(cellInfo_t) * select->column_count );
		select->cache.row_count	= 1;
		
		for ( i=0; i<row_count; i++ ) {
			for ( j=0; j<select->column_count; j++ ) {
				sql_eval( db, select->column_expr[ j ], stmt->table, rows[ i ],i,row_count, stmt->params, select->cache.result+j );
			}
		}

	} else {

		int i,j;
		cellInfo_t * cell;

		select->cache.result	= sql_cache_push( db, sizeof(cellInfo_t) * select->column_count * row_count );
		select->cache.row_count	= row_count;

		for ( i=0,cell=select->cache.result; i<row_count; i++ ) {
			for ( j=0; j<select->column_count; j++,cell++ ) {

				*cell = ( select->entire_row )?rows[ i ][ j ]:sql_eval( db, select->column_expr[ j ], stmt->table, rows[ i ],i,row_count, stmt->params, 0 );

				if ( select->column_type[ j ] == STRING && cell->string ) {

					int		n = strlen( cell->string ) + 1;
					char *	s = sql_cache_push( db, n );

					Q_strncpyz( s, cell->string, n );

					cell->string = db->cache.segment + (s-db->cache.buffer);
				}
			}
		}

		if ( row_count && select->sort ) {
			int				column = sql_eval( db, select->sort, stmt->table, 0,0,0, stmt->params, 0 ).integer;
			dataFormat_t	format = select->column_type[ abs( column )-1 ];
			int (QDECL *f)( const void *, const void * );

			if ( column < 0 ) {
				f = ( format == STRING )?sorttable_string_dec:sorttable_integer_dec;
			} else {
				f = ( format == STRING )?sorttable_string_asc:sorttable_integer_asc;
			}

			sort_column = abs(column) - 1;
			qvm_offset = db->cache.segment - db->cache.top;
			qsort( select->cache.result, row_count, sizeof(cellInfo_t) * select->column_count, f ); 
		}
	}

	return 1;
}
