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

static stmtInfo_t * sql_create_schema( sqlInfo_t * db, const char ** s );

/*
===============
sql_create_table

`CREATE TABLE npcs (isMerchant bool, id INTEGER PRIMARY KEY, planet_id integer, name varchar(255), spawn varchar(255), type varchar(255), bounty integer, map varchar(255), music varchar(255), good bool, dialog varchar(255), used integer default 0, model varchar(255), gang_count_medium integer default 0, gang_count_easy integer default 0, gang_count_hard integer default 0, moa_rank integer, contact text);`
===============
*/
static tableInfo_t * sql_create_table( sqlInfo_t * db, const char ** s )
{
	tableInfo_t	* table;
	char * n;
	int i;
	int flags = 0;

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
		flags |= SQL_TABLE_SERVERONLY;
		break;
	case CS('c','l','_',0):
		if ( db->memory_tag == TAG_SQL_SERVER && !db->create_filter  )
			return 0;
		n += 3;
		flags |= SQL_TABLE_CLIENTONLY;
		break;
	case CS('b','g','_',0):
		n += 3;
		flags |= SQL_TABLE_BOTH;
		break;
	}

	//	IF NOT EXISTS
	if ( SWITCHSTRING( n ) == CS('i','f',0,0 ) ) {
		parse_temp( s );	// NOT
		parse_temp( s );	// EXISTS

		n = parse_temp( s );
		table = find_table( db, n );

		if ( table ) {
			return 0;
		}
	} else {
		table = find_table( db, n );
	}

	if ( table ) {

		if ( db->delete_trigger )
			db->delete_trigger( db, table, -1 );

		for ( i=0; i<table->column_count; i++ ) {
			sql_free( db, table->columns[ i ].index );
		}
		table->column_count = 0;
		table->row_total =
		table->row_count = 0;
		table->last_changed++;
		sql_free( db, table->rows );
		table->rows = 0;

	} else {

		//	create new table
		table = sql_calloc( db, sizeof(tableInfo_t) );
		table->name = sql_alloc_string( db, n );
		insert_table( db, table );
	}

	table->flags |= flags;

	if ( !parse_tofirstparam( s ) ) {
		if ( SWITCHSTRING( parse_temp( s ) ) == CS('a','s',0,0) ) {
			selectInfo_t *	select = (selectInfo_t*)sql_parse( db, s, 0 );
			int i;

			ASSERT( select->stmt.type == SQL_SELECT );
			sql_select_work( db, select );

			table->column_count = select->column_count;

			table->row_total	=
			table->row_count	= select->cache.row_count;
			table->rows			= (table->row_total)?sql_alloc( db, sizeof(int) * table->column_count * table->row_total ):0;

			for ( i=0; i<select->column_count; i++ ) {

				int j;

				columnInfo_t * c = table->columns + i;

				c->name = sql_alloc_string( db, select->column_name[i] );
				c->format = select->column_type[i];
				c->index = 0;
				c->num = i;

				if ( c->format == STRING ) {

					for ( j=0; j<table->row_count; j++ ) {
						int index = table->column_count*j + i;
						table->rows[ index ].string = sql_alloc_string( db, select->cache.result[ index ].string );
					}

				} else {

					for ( j=0; j<table->row_count; j++ ) {
						int index = table->column_count*j + i;
						table->rows[ index ].integer = select->cache.result[ index ].integer;
					}
				}
			}

			sql_cache_pop( db, select->cache.result );

			return table;
		}
	}

	//	create columns
	sql_table_addcolumns( db, table, s );

	return table;
}


void sql_table_addcolumns( sqlInfo_t * db, tableInfo_t * table, const char ** s )
{
	for ( ;; )
	{
		char * name = parse_temp( s );

		columnInfo_t * c = find_column( table, name );

		if ( !c ) {
			
			c = table->columns + table->column_count;
			c->name = sql_alloc_string( db, name );

			switch( SWITCHSTRING(parse_temp( s )) )
			{
			case CS('i','n','t','e'):	//	integer
			case CS('b','o','o','l'):	//	bool
			case CS('n','u','m','e'):	//	numeric
				c->format = INTEGER;
				break;
				
			case CS('v','a','r','c'):	//	varchar
				parse_tonextparam( s );
			case CS('t','e','x','t'):	//	text
			case CS('s','t','r','i'):
				c->format = STRING;
				break;
			case CS('U','N','I','Q'):	//	UNIQUE
				c->format = INTEGER;
				table->primary_column = table->column_count+1;
				break;
			}

			c->index = 0;
			c->num = table->column_count;

			table->column_count++;
			ASSERT( table->column_count < MAX_COLUMNS_PER_TABLE );
		}

		if ( !parse_tonextparam( s ) )
			break;
	}
}

void sql_create_index( sqlInfo_t * db, tableInfo_t * table, columnInfo_t * column ) {

	short i;
	column->index = sql_alloc( db, sizeof(short) * table->row_total );
	for ( i=0; i<table->row_count; i++ ) {
		((column->format==STRING)?insert_index_s:insert_index_i)( column->index, i, table->rows, table->column_count, column->num, i );
	}
}


stmtInfo_t * sql_create_parse( sqlInfo_t * db, const char ** data_p )
{
	const char * s = *data_p;
	char * t;
	int		flags = 0;

	t = parse_temp( &s );
	switch( SWITCHSTRING(t) )
	{
	case CS('t','e','m','p'):
		parse_temp( &s );
		flags |= SQL_TABLE_TEMPORARY;
		//	create table
	case CS('t','a','b','l'):
		{
			tableInfo_t * t = sql_create_table( db, &s );
			if ( t && flags ) {
				t->flags |= flags;
			}

		} break;
		//	create index
	case CS('i','n','d','e'):
		{
			tableInfo_t * table;
			char * n;

			//	skip name
			parse_temp( &s );

			//	skip 'ON'
			parse_temp( &s );

			n = parse_temp( &s );
			table = find_table( db, n );

#ifdef DEVELOPER
			if ( !table ) {
				Com_Error( ERR_FATAL, "table '%s' does not exist.", n );
			}
#endif


			if ( parse_tofirstparam( &s ) )
			{
				columnInfo_t * column = find_column( table, parse_temp( &s ) );

				if ( column && !column->index ) {
					sql_create_index( db, table, column );
				}
			}

		} break;

		//	create sync
	case CS('s','y','n','c'):
		{
			tableInfo_t * table;

			//	skip 'ON'
			parse_temp( &s );

			table = find_table( db, parse_temp( &s ) );
			ASSERT( table );

			table->sync = sql_parse( db, &s, 0 );
			table->flags |= SQL_TABLE_SERVERONLY;
		} break;

		//	create schema
	case CS('s','c','h','e'):
		return sql_create_schema( db, &s );

#ifdef DEVELOPER
		//	create link
	case CS('l','i','n','k'):
		{
			tableInfo_t *	t1;
			columnInfo_t *	c1;
			char * n;

			//	CREATE LINK clients.id TO npcs.id, clients.client TO players.client;
			for ( ;; ) {

				t1 = find_table( db, parse_temp( &s ) );
				parse_temp( &s );
				c1 = find_column( t1, parse_temp( &s ) );

				//	TO
				parse_temp( &s );

				c1->link.table = sql_alloc_string( db, parse_temp( &s ) );
				parse_temp( &s );
				c1->link.column = sql_alloc_string( db, parse_temp( &s ) );

				n = parse_temp( &s );

				if ( n[ 0 ] != ',' )
					break;
			}

			
		} break;
#endif

	}

	return 0;
}




/*
===============
sql_create_schema

`CREATE SCHEMA mydb (
	<table row> {
		<column name>	<cell data>
		<column name>	<cell data>

		<table row> {
			<column name>	<cell data>
			<column name>	<cell data>
		}
	}
);`
===============
*/
static int parse_rows( sqlInfo_t * db, const char ** s, const char * schema, const char * name ) {

	tableInfo_t *	table = NULL;
	int				id = 0;

	if ( name ) {

		char * table_name = va("%ss",name);
		table	= find_table( db, table_name );

		if ( !table ) {
			sql_exec( db, va(	"CREATE TABLE %s ("
									"id		INTEGER "
								");"
								"INSERT INTO %s VALUES('%s');", table_name, schema, table_name ) );

			table = find_table( db, table_name );
		}

		
		id		= table->row_count;
	}

	for ( ; ; ) {

		columnInfo_t * c;
		char	column[ MAX_NAME_LENGTH ];
		char	cell[ MAX_INFO_STRING ];
		int		lit;

		//	parse line
		parse( s, column, sizeof(column), 0 );
		if ( column[0] == '}' || column[0] == ')' ) {
			break;
		}

		parse( s, cell, sizeof(cell), &lit );

		//	recurses into another table
		if ( cell[0] == '{' ) {

			int	child_id = parse_rows( db, s, schema, column );

			if ( name ) {
				sql_exec( db, va(	"CREATE TABLE IF NOT EXISTS %ss_%ss ("
										"p INTEGER, "
										"c INTEGER "
									");"

									"INSERT INTO %ss_%ss VALUES( %d, %d );", name, column, name, column, id, child_id ) );
			}
			continue;
		}

		//	assign cell
		c = find_column( table, column );

		if ( !c ) {
			const char * type = "STRING";
			if ( !Q_stricmp( cell, fn( atoi( cell ), FN_PLAIN ) ) ) {
				type = "INTEGER";
			}
			sql_exec( db, va("ALTER TABLE %s ADD %s %s;", table->name, column, type) );
			c = find_column( table, column );
		}

		if ( c && c->format == STRING ) {
			sql_exec( db, va("UPDATE OR INSERT %s SET id=%d,%s='%s' SEARCH id %d;", table->name, id, column, cell, id) );
		} else {
			sql_exec( db, va("UPDATE OR INSERT %s SET id=%d,%s=%s SEARCH id %d;", table->name, id, column, cell, id) );
		}
	}

	return id;
}


static stmtInfo_t * sql_create_schema( sqlInfo_t * db, const char ** s ) {

	char schema[ MAX_NAME_LENGTH ];

	parse( s, schema, sizeof(schema), 0 );

	//	create table to store the schema
	sql_exec( db, va(	"CREATE TABLE %s ("
							"table	STRING "
						");", schema ) );

	//	'('
	parse_temp( s );

	parse_rows( db, s, schema, NULL );

	return NULL;
}
