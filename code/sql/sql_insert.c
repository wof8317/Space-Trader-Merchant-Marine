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



//	INSERT INTO "commodities_events" VALUES(7, 5, 3, 400, 600);
//	INSERT INTO contacts(player,npc) VALUES(?,?);
stmtInfo_t * sql_insert_parse( sqlInfo_t * db, const char ** s )
{
	columnInfo_t *	c[ MAX_COLUMNS_PER_TABLE ];
	int	i,count=0;
	tableInfo_t * table;
	insertInfo_t * insert;
	char * n;

	//	'INTO'
	parse_temp( s );


	n = parse_temp( s );

	if ( db->create_filter ) {

		if ( Q_stricmp( n, db->create_filter ) ) {
			return 0;
		}
	}

	switch ( CS(n[0],n[1],n[2],0) )
	{
	case CS('s','v','_',0):
		if ( db->memory_tag == TAG_SQL_CLIENT && !db->create_filter  )
			return 0;
		n += 3;
		break;
	case CS('c','l','_',0):
		if ( db->memory_tag == TAG_SQL_SERVER && !db->create_filter  )
			return 0;
		n += 3;
		break;
	case CS('b','g','_',0):
		n += 3;
		break;
	}


	table = find_table( db, n );
	if ( !table )
		return 0;

	ASSERT( table );

	insert = sql_calloc_stmt( db, sizeof(insertInfo_t) );
	insert->stmt.type	= SQL_INSERT;
	insert->stmt.table	= table;

	//	parse column names
	if ( parse_tofirstparam( s ) )
	{
		do
		{
			char * name = parse_temp( s );
			c[ count ] = find_column( table, name );

#ifdef DEVELOPER
			if ( c[ count ] == 0 ) {
				Com_Error( ERR_FATAL, "column '%s' does not exist on table '%s'.\n", name, table->name );
			}
#endif
			count++;

		} while( parse_tonextparam( s ) );
	} else
	{
		for ( i=0; i<table->column_count; i++ )
			c[ count++ ] = &table->columns[ i ];
	}

	//	skip VALUES keyword
	switch ( SWITCHSTRING( parse_temp( s ) ) )
	{
		case CS('s','e','l','e'):
			{
				insert->select = (selectInfo_t*)sql_select_parse( db, s );
				for ( i=0; i<count; i++ ) {
					insert->columns[ i ] = (char)c[ i ]->num;
				}
				ASSERT( count == insert->select->column_count );

			} break;
		
		case CS('v','a','l','u'):
			{
				parseInfo_t	pi = { 0 };

				pi.db		= db;
				pi.params	= insert->stmt.params;
				pi.table	= insert->stmt.table;
				pi.flags	= PARSE_ASSIGN_TO_COLUMN;
				pi.more		= 1;

				//	parse column assignments
				if ( parse_tofirstparam( s ) )
				{
					for ( i=0; pi.more; i++ ) {
						pi.column = c[ i ]->num;
						insert->values_expr[ i ] = parse_expression( s, &pi );
					}
					insert->values_count = i;
				}

				ASSERT( insert->values_count == count );

			} break;

		case CS('r','o','w','s'):
			{
				cellInfo_t * row;
				parseInfo_t	pi = { 0 };
				Expr	e;

				pi.db		= db;
				pi.params	= insert->stmt.params;
				pi.table	= insert->stmt.table;
				pi.more		= 1;

				if ( parse_tofirstparam( s ) )
				{

					while ( pi.more ) {
						row = sql_insert_begin( db, table );

						for ( i=0; i<table->column_count; i++ ) {
							
							int top = db->stmt_buffer.c->top;

							e = parse_expression( s, &pi );
							row[ i ] = sql_eval( db, e, table, 0,0,1, 0, 0 );

							if ( table->columns[ i ].format == STRING ) {
								row[ i ].string = sql_alloc_string( db, row[ i ].string );
							}

							db->stmt_buffer.c->top = top;
						}

						sql_insert_done( db, table );
					}
				}

				return 0;

			} break;
	}

	return &insert->stmt;
}

static void sql_grow_table( sqlInfo_t * db, tableInfo_t * table ) {

	int i;
	int n1 = table->row_total;
	int n2 = (n1<32)?64:(n1 + (n1>>1));

	//	resize rows
	table->rows = sql_realloc	(	db,
									table->rows,
									n1 * sizeof(cellInfo_t) * table->column_count,
									n2 * sizeof(cellInfo_t) * table->column_count
								);

	//	resize column index
	for ( i=0; i<table->column_count; i++ ) {
		columnInfo_t * c = table->columns + i;
		if ( c->index ) {
			c->index = sql_realloc	(	db,
										c->index,
										n1 * sizeof(short),
										n2 * sizeof(short)
									);
		}
	}

	table->row_total = n2;
}

int search_first_gap( short * index, int count, cellInfo_t * rows, int column_count, int column ) {

	int lower	= 0;
	int upper	= count-1;
	int i;
	if ( count > 0 ) {
		for ( i=(lower+upper)>>1; ( lower<=upper ); )
		{
			if ( i < rows[ index[ i ] * column_count + column ].integer )
				upper = i-1;
			else
				lower = i+1;

			i = (lower+upper)>>1;
		}

		return lower;
	}

	return count;
}

cellInfo_t * sql_insert_begin( sqlInfo_t * db, tableInfo_t * table ) {

	cellInfo_t *	row;
	int				unique = 0;

	if ( table->primary_column > 0 ) {

		columnInfo_t * c = table->columns + (table->primary_column-1);

		if ( !c->index ) {
			sql_create_index( db, table, c );
		}

		if ( table->last_changed != table->last_indexed ) {
			sql_table_reindex( table );				   
		}

		unique = search_first_gap( c->index, table->row_count, table->rows, table->column_count, table->primary_column-1 );
		ASSERT( unique >= 0 );
	}

	table->row_count++;

	//	grow table
	if ( table->row_count >= table->row_total ) {
		sql_grow_table( db, table );
	}

	row = table->rows + table->column_count*(table->row_count-1);
	Com_Memset( row, 0, sizeof(int) * table->column_count );

	if ( table->primary_column > 0 ) {
		row[ table->primary_column-1 ].integer = unique;
	}

	return row;
}

void sql_insert_done( sqlInfo_t * db, tableInfo_t * table ) {

	int i;
	short row_index = table->row_count - 1;

	//	update index
	if ( table->last_changed == table->last_indexed ) {

		//	if the index is up to date, keep it that way
		for ( i=0; i<table->column_count; i++ ) {
			columnInfo_t * c = table->columns + i;
			if ( c->index ) {
				(( c->format == INTEGER )?insert_index_i:insert_index_s)( c->index, row_index, table->rows, table->column_count, c->num, row_index );
			}
		}

		//	mark the table as changed and the index as valid
		table->last_indexed = ++table->last_changed;
	} else {

		//	mark the table as changed and leave the index as it is.
		//	it will be re-indexed when it's needed
		table->last_changed++;
	}

	//	call trigger
	if ( db->insert_trigger )
		db->insert_trigger( db, table, row_index );
}


int sql_insert_work( sqlInfo_t * db, insertInfo_t * insert )
{
	tableInfo_t *	table	= insert->stmt.table;
	stmtInfo_t *	stmt	= &insert->stmt;
	cellInfo_t *	row;
	int i;

	if ( insert->select ) {
		selectInfo_t * select = insert->select;

		//	pass parameters to select statement
		memcpy( &select->stmt.params, &insert->stmt.params, sizeof(select->stmt.params) );

		sql_select_work( db, select );

		for ( i=0; i<select->cache.row_count; i++ ) {
			cellInfo_t *	row;
			int j;

			row = sql_insert_begin( db, table );

			for ( j=0; j<select->column_count; j++ ) {

				int c = insert->columns[ j ];
				ASSERT( select->column_type[j] == table->columns[c].format );
				if ( select->column_type[j] == STRING ) {
					row[ c ].string = sql_alloc_string( db, select->cache.result[ i*select->column_count+j ].string );
				} else {
					row[ c ].integer = select->cache.result[ i*select->column_count+j ].integer;
				}
			}

			sql_insert_done( db, table );
		}

		sql_cache_pop( db, select->cache.result );

		return 0;

	} else
	{
		row = sql_insert_begin( db, table );
		
		//	initialize new row
		for ( i=0; i<insert->values_count; i++ ) {
			sql_eval( db, insert->values_expr[ i ], stmt->table, row,0,1, stmt->params, 0 );
		}

		sql_insert_done( db, table );
	}

	ASSERT( table->row_count <= table->row_total );

	return 1;
}
