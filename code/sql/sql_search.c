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


int		CompareTableByName( const void * a, const void * b ) {
	return Q_stricmp( (*(tableInfo_t**)a)->name, (char*)b );
}
int		CompareTables( const void * a, const void * b ) {
	return Q_stricmp( (*(tableInfo_t**)a)->name, (*(tableInfo_t**)b)->name );
}
int		CompareColumnByName( const void * a, const void * b ) {
	return Q_stricmp( ((columnInfo_t*)a)->name, (char*)b );
}
int		CompareStmtByName( const void * a, const void * b ) {
	return Q_stricmp( (*(stmtInfo_t**)a)->src, (char*)b );
}
int		CompareStmts( const void * a, const void * b ) {
	return Q_stricmp( (*(stmtInfo_t**)a)->src, (*(stmtInfo_t**)b)->src );
}


void * search( const void * base, int count, int sizeOfElement, const void * key, int( *compare)(const void*, const void*) )
{
	int i;
	int lower = 0;
	int upper = count-1;

	if ( count==0 )
		return 0;

	for ( i=(lower+upper)>>1; ( lower<=upper ); )
	{
		int c = compare( ((char*)base) + sizeOfElement * i, key );

		if ( c == 0 )
			break;

		if ( c > 0 )
			upper = i-1;
		else
			lower = i+1;

		i = (lower+upper)>>1;
	}

	if ( lower <= upper )
		return ((char*)base) + sizeOfElement * i;

	return 0;
}

int insert( void * base, int * count, int sizeOfElement, const void * element, int( *compare)(const void*, const void*) ) {

	int lower	= 0;
	int upper	= *count-1;
	int i;

	if ( *count > 0 ) {
		for ( i=(lower+upper)>>1; ( lower<=upper ); )
		{
			int c = compare( ((char*)base) + sizeOfElement * i, element );
			if ( c == 0 )
				break;

			if ( c > 0 )
				upper = i-1;
			else
				lower = i+1;

			i = (lower+upper)>>1;
		}
		if ( lower > upper )
			i++;

		memmove		( ((char*)base) + (i+1) * sizeOfElement, ((char*)base) + i * sizeOfElement, (*count-i) * sizeOfElement );

	} else
		i = 0;

	
	Com_Memcpy	( ((char*)base) + i * sizeOfElement, element, sizeOfElement );

	(*count)++;

	return i;
}


tableInfo_t * find_table( sqlInfo_t * db, const char * name ) {
	tableInfo_t** t = (tableInfo_t**)search( db->tables, db->table_count, sizeof(tableInfo_t*), name, CompareTableByName );
	return ( t )?*t:0;
}

columnInfo_t * find_column( tableInfo_t * table, const char * name ) {
	int i;
	for ( i=0; i<table->column_count; i++ ) {
		if ( Q_stricmp( table->columns[ i ].name, name ) == 0 ) {
			return table->columns + i;
		}
	}
	return 0;
}

stmtInfo_t * find_stmt( sqlInfo_t * db, const char * src ) {
	stmtInfo_t** s = (stmtInfo_t**)search( db->stmts, db->stmt_count, sizeof(stmtInfo_t*), src, CompareStmtByName );
	return ( s )?*s:0;
}

void insert_stmt( sqlInfo_t * db, stmtInfo_t * stmt ) {
	ASSERT( db->stmt_count < MAX_STMTS_PER_DB );
	insert( db->stmts, &db->stmt_count, sizeof(stmtInfo_t*), &stmt, CompareStmts );
}

void insert_table( sqlInfo_t * db, tableInfo_t * table ) {
	ASSERT( db->table_count < MAX_TABLES_PER_DB );
	insert( db->tables, &db->table_count, sizeof(tableInfo_t*), &table, CompareTables );
}

void insert_index_i( short * index, int count, cellInfo_t * rows, int column_count, int column, short row_index ) {

	int lower	= 0;
	int upper	= count-1;
	cellInfo_t key = rows[ row_index * column_count + column ];
	int i;
	if ( count > 0 ) {
		for ( i=(lower+upper)>>1; ( lower<=upper ); )
		{
			int c = rows[ index[ i ] * column_count + column ].integer - key.integer;
			if ( c <= 0 )
				lower = i+1;
			else
				upper = i-1;

			i = (lower+upper)>>1;
		}
		if ( lower > upper )
			i++;

		memmove( &index[ i + 1 ], &index[ i ], sizeof(index[ 0 ]) * (count-i) );
	} else
		i = 0;
	index[ i ] = row_index;
}

void insert_index_s( short * index, int count, cellInfo_t * rows, int column_count, int column, short row_index ) {

	int lower	= 0;
	int upper	= count-1;
	cellInfo_t key	= rows[ row_index * column_count + column ];
	int i;
	if ( count > 0 ) {
		for ( i=(lower+upper)>>1; ( lower<=upper ); )
		{
			int c = Q_stricmp( rows[ index[ i ] * column_count + column ].string, key.string );
			if ( c == 0 )
				break;
			
			if ( c > 0 )
				upper = i-1;
			else
				lower = i+1;

			i = (lower+upper)>>1;
		}
		if ( lower > upper )
			i++;

		memmove( &index[ i + 1 ], &index[ i ], sizeof(index[ 0 ]) * (count-i) );
	} else
		i = 0;
	index[ i ] = row_index;
}

int search_index_i( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key ) {

	int lower	= 0;
	int upper	= count-1;
	int i;
	if ( count > 0 ) {
		for ( i=(lower+upper)>>1; ( lower<=upper ); )
		{
			int c = rows[ index[ i ] * column_count + column ].integer - key.integer;
			if ( c == 0 )
				break;
			
			if ( c > 0 )
				upper = i-1;
			else
				lower = i+1;

			i = (lower+upper)>>1;
		}

		if ( lower <= upper )
			return i;
	}

	return -1;
}

int search_index_i_first( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key ) {

	int lower	= 0;
	int upper	= count-1;
	int i;
	if ( count > 0 ) {
		for ( i=(lower+upper)>>1; ( lower<=upper ); )
		{
			if ( key.integer <= rows[ index[ i ] * column_count + column ].integer )
				upper = i-1;
			else
				lower = i+1;

			i = (lower+upper)>>1;
		}

		return lower;
	}

	return -1;
}

int search_index_i_last( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key ) {

	int lower	= 0;
	int upper	= count-1;
	int i;
	if ( count > 0 ) {
		for ( i=(lower+upper)>>1; ( lower<=upper ); )
		{
			if ( key.integer >= rows[ index[ i ] * column_count + column ].integer )
				lower = i+1;
			else
				upper = i-1;

			i = (lower+upper)>>1;
		}

		return upper;
	}

	return -1;
}

int search_index_s( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key ) {

	int lower	= 0;
	int upper	= count-1;
	int i;
	if ( count > 0 ) {
		for ( i=(lower+upper)>>1; ( lower<=upper ); )
		{
			int c = Q_stricmp( rows[ index[ i ] * column_count + column ].string, key.string );
			if ( c == 0 )
				break;
			
			if ( c > 0 )
				upper = i-1;
			else
				lower = i+1;

			i = (lower+upper)>>1;
		}

		if ( lower <= upper )
			return i;
	}

	return -1;
}


int search_index_s_first( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key ) {

	int lower	= 0;
	int upper	= count-1;
	int i;
	if ( count > 0 ) {
		for ( i=(lower+upper)>>1; ( lower<=upper ); )
		{
			if ( Q_stricmp( key.string, rows[ index[ i ] * column_count + column ].string ) <= 0 )
				upper = i-1;
			else
				lower = i+1;

			i = (lower+upper)>>1;
		}

		return lower;
	}

	return -1;
}

int search_index_s_last( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key ) {

	int lower	= 0;
	int upper	= count-1;
	int i;
	if ( count > 0 ) {
		for ( i=(lower+upper)>>1; ( lower<=upper ); )
		{
			if ( Q_stricmp( key.string, rows[ index[ i ] * column_count + column ].string ) >= 0 )
				lower = i+1;
			else
				upper = i-1;

			i = (lower+upper)>>1;
		}

		return upper;
	}

	return -1;
}


void sql_table_reindex( tableInfo_t * table )
{
	int i;

	// update index
	for ( i=0; i<table->column_count; i++ ) {
		columnInfo_t * column = table->columns + i;
		if ( column->index ) {
			short j;
			for ( j=0; j<table->row_count; j++ ) {
				((column->format==STRING)?insert_index_s:insert_index_i)( column->index, j, table->rows, table->column_count, column->num, j );
			}
		}
	}

	table->last_indexed = table->last_changed;
}


