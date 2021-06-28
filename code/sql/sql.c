/*
===========================================================================
Copyright (C) 2007 HermitWorks Entertainment Corporation

This file is part of the Space Trader source code.

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


#ifdef DEVELOPER
const char * CURRENT_STMT;
#endif


/*
===============
sql_parse

parses an sql statement.  data_p is the text buffer containing the statement.  data_p is moved
to the where sql_parse stops.  if tmp is used it points to a buffer SQL_STMT_ALLOC bytes in length
which is used to parse the statement into.
===============
*/
stmtInfo_t * sql_parse( sqlInfo_t * db, const char ** data_p, char * tmp )
{
	const char * s = *data_p;
	char * t;
	stmtInfo_t * stmt;
	sqlStack_t * save;

#ifdef DEVELOPER
	CURRENT_STMT = s;
#endif

	save = db->stmt_buffer.c;
	if ( tmp ) {
		db->stmt_buffer.c		= (sqlStack_t*)tmp;
		db->stmt_buffer.c->top	= sizeof(sqlStack_t);
	} else {
		db->stmt_buffer.c		= db->stmt_buffer.p;
	}

	ASSERT( db->stmt_buffer.c || db->stmt_buffer.c == db->stmt_buffer.p );

	for ( stmt=0;!stmt; )
	{
		const char * stmt_src = s;
		t = parse_temp( &s );

		if ( t[ 0 ] == '\0' ) {
			s = 0;
			break;
		}

		switch( SWITCHSTRING(t) )
		{
		case CS('c','r','e','a'):	stmt = sql_create_parse( db, &s );	break;
		case CS('s','e','l','e'):	stmt = sql_select_parse( db, &s );	break;
		case CS('i','n','s','e'):	stmt = sql_insert_parse( db, &s );	break;
		case CS('d','e','l','e'):	stmt = sql_delete_parse( db, &s );	break;
		case CS('u','p','d','a'):	stmt = sql_update_parse( db, &s );	break;
		case CS('a','l','t','e'):	stmt = sql_alter_parse( db, &s );	break;

		case CS('b','e','g','i'):
		case CS('c','o','m','m'):
			break;

		default:
			Com_Error( ERR_FATAL, "invalid sql statement: '%s'\n%s\n", t, s );
		}

		if ( stmt ) {
			stmt->src = stmt_src;
		}

		if ( !parse_tonextstatement( &s ) )
			break;
	}

	*data_p = s;

	db->stmt_buffer.c = (save)?save:db->stmt_buffer.p;

	return stmt;
}

/*
===============
sql_work

executes a statement
===============
*/
static int sql_work( sqlInfo_t * db, stmtInfo_t * stmt )
{
	switch ( stmt->type )
	{
	case SQL_SELECT:	sql_select_work( db, (selectInfo_t*)stmt );	break;
	case SQL_INSERT:	sql_insert_work( db, (insertInfo_t*)stmt );	break;
	case SQL_UPDATE:	sql_update_work( db, (updateInfo_t*)stmt );	break;
	case SQL_DELETE:	sql_delete_work( db, (deleteInfo_t*)stmt ); break;
	case SQL_FORMAT:	sql_format_work( db, (formatInfo_t*)stmt ); break;
	}

	return 1;
}

/*
===============
sql_prompt

executes a statement and prints out the result
===============
*/
void sql_prompt( sqlInfo_t * db, char * src )
{
	int f=0;
	sql_prepare( db, src );

	while ( sql_step( db ) )
	{
		int i;
		int n = sql_columncount( db );
		if ( f==0 ) {
			f=1;
			for ( i=0; i<n; i++ ) {
				Com_Printf( "%16s", sql_columnname( db, i ) );
			}
			Com_Printf( "\n" );
			for ( i=0; i<n; i++ ) {
				Com_Printf( "----------------" );
			}
			Com_Printf( "\n" );
		}

		for ( i=0; i<n; i++ ) {
			Com_Printf( "%16s", sql_columnastext( db, i ) );
		}
		Com_Printf( "\n" );
	}
	sql_done( db );

	// sql_stmt_free( db, stmt );
}


/*
===============
sql_exec

executes a statement without caching anything.
===============
*/
int sql_exec( sqlInfo_t * db, const char * src ) {
		
	char tmp[ SQL_STMT_ALLOC ];
	const char * s;
	stmtInfo_t * stmt;

	//
	//	allow statement to be re-directed to another db
	//
	switch( SWITCHSTRING( src ) )
	{
	case CS(':','s','e','r'):	db = sql_getserverdb(); break;
	case CS(':','c','l','i'):	db = sql_getclientdb(); break;
	case CS(':','c','o','m'):	db = sql_getcommondb(); break;
	case CS(':','b','o','t'):
		{
			COM_Parse( &src );
			parse_tofirstparam( &src );
			sql_exec( sql_getserverdb(), src);
			sql_exec( sql_getclientdb(), src);
			return 1;
		}	break;
	default:
		goto start;
	}
	COM_Parse( &src );
	parse_tofirstparam( &src );


start:
	s = src;

	for ( ;s; )
	{
		stmt = sql_parse( db, &s, tmp );
		if ( stmt ) {
			sql_work( db, stmt );
		}
	}

	return 1;
}


/*
===============
sql_prepare

pushes the statement in 'src' on to the execution stack.  src may have been parsed
from a previous call to sql_prepare.
===============
*/
stmtInfo_t * sql_prepare( sqlInfo_t * db, const char * src ) {

	stmtInfo_t * stmt = find_stmt( db, src );

	if ( !stmt ) {
		const char * s = src;
		stmt = sql_parse( db, &s, 0 );
		if ( stmt ) {
			stmt->src	= sql_alloc_string( db, src );
			insert_stmt( db, stmt );
		}
	}

	ASSERT( stmt );

	if ( stmt && (db->pc ==0 || stmt != db->callstack[ db->pc-1 ])  ) {
		db->callstack[ db->pc++ ] = stmt; 
		stmt->step	= -1;
	}

	return stmt;
}

int sql_compile( sqlInfo_t * db, const char * src ) {
	int index = db->stmts_byindex_count++;
	//ASSERT( index >= 0 && index < MAX_STMTS_PER_DB );
	if (!db->stmts_byindex[ index ])
		db->stmts_byindex[ index ] = sql_format_parse( db, &src );
	return index;
}

const char * sql_run( sqlInfo_t * db, int index ) {
	return sql_format_work( db, (formatInfo_t*)db->stmts_byindex[ index ] );
}

int sql_expr( sqlInfo_t * db, const char * src ) {

	Expr	e;
	int		r;

	parseInfo_t	pi = { 0 };
	pi.db		= db;

	e = parse_expression( &src, &pi );

	ASSERT( pi.rt == INTEGER );
	ASSERT( pi.more == 0 );
	
	r = sql_eval( db, e, 0, 0, 0, 0, 0, 0 ).integer;

	sql_free( db, e );

	return r;
}

int sql_bind( sqlInfo_t * db, intptr_t *args ) {
	int i;
	stmtInfo_t * stmt;

	sql_prepare( db, VMA(1) );
	stmt = db->callstack[ db->pc-1 ];

	for ( i=0; i<MAX_COLUMNS_PER_TABLE && stmt->params[ i ].format != INVALID; i++ ) {

		if ( stmt->params[ i ].format == STRING ) {
			stmt->params[ i ].payload.string = VMA(2+i);
		} else {
			stmt->params[ i ].payload.integer = args[2+i];
		}
	}

	return 1;
}

int sql_bindtext( sqlInfo_t * db, int arg, const char * text ) {

	stmtInfo_t * stmt = db->callstack[ db->pc-1 ];

	ASSERT( db->pc>0 && stmt );

	stmt->params[ arg-1 ].format = STRING;
	stmt->params[ arg-1 ].payload.string = (char*)text;
	return 1;
}

int sql_bindint( sqlInfo_t * db, int arg, int integer ) {
	stmtInfo_t * stmt = db->callstack[ db->pc-1 ];

	ASSERT( db->pc>0 && stmt );

	stmt->params[ arg-1 ].format = INTEGER;
	stmt->params[ arg-1 ].payload.integer = integer;
	return 1;
}

int sql_step( sqlInfo_t * db ) {

	stmtInfo_t * stmt = db->callstack[ db->pc-1 ];
	ASSERT( db->pc>0 && stmt );

#ifdef DEVELOPER
	CURRENT_STMT = stmt->src;
#endif

	switch( stmt->type ) {

		case SQL_SELECT:
			{
				selectInfo_t * select = ((selectInfo_t*)stmt);

				if ( stmt->step == -1 ) {

					sql_select_work( db, select );
				}

				stmt->step++;

				if ( stmt->step < select->cache.row_count )
					return 1;

			} break;

		default:
			sql_work( db, stmt );
			break;
	}

	return 0;
}

int sql_columncount( sqlInfo_t * db ) {

	stmtInfo_t * stmt = db->callstack[ db->pc-1 ];

	ASSERT( db->pc>0 );
	ASSERT( stmt );
	ASSERT( stmt->type == SQL_SELECT );

	return ((selectInfo_t*)stmt)->column_count;
}

int sql_rowcount( sqlInfo_t * db ) {
	stmtInfo_t * stmt = db->callstack[ db->pc-1 ];

	ASSERT( db->pc>0 );
	ASSERT( stmt );
	ASSERT( stmt->type == SQL_SELECT );

	return ((selectInfo_t*)stmt)->cache.row_count;
}

const char * sql_columnastext( sqlInfo_t * db, int column ) {
	
	stmtInfo_t *	stmt	= db->callstack[ db->pc-1 ];
	selectInfo_t *	select	= (selectInfo_t*)stmt;
	cellInfo_t		cell;

	ASSERT( db->pc>0 );
	ASSERT( stmt );
	ASSERT( stmt->type == SQL_SELECT );
	ASSERT( select->cache.result );

	cell = select->cache.result[ stmt->step*select->column_count + column ];

	if ( select->column_type[ column ] == STRING ) {
		return (cell.string)?cell.string:"(NULL)";
	}

	return fn( cell.integer, FN_PLAIN );
}

int sql_columnasint( sqlInfo_t * db, int column ) {

	stmtInfo_t *	stmt	= db->callstack[ db->pc-1 ];
	selectInfo_t *	select	= (selectInfo_t*)stmt;
	cellInfo_t		cell;

	ASSERT( db->pc>0 );
	ASSERT( stmt );
	ASSERT( stmt->type == SQL_SELECT );
	ASSERT( select->cache.result );

	cell = select->cache.result[ stmt->step*select->column_count + column ];
	return ( select->column_type[ column ] == STRING )?atoi( cell.string ):cell.integer;
}

const char * sql_columnname( sqlInfo_t * db, int column ) {

	stmtInfo_t *	stmt	= db->callstack[ db->pc-1 ];
	selectInfo_t *	select	= (selectInfo_t*)stmt;

	ASSERT( db->pc>0 );
	ASSERT( stmt );
	ASSERT( stmt->type == SQL_SELECT );

	return select->column_name[ column ];
}

int sql_done( sqlInfo_t * db ) {
	stmtInfo_t *	stmt	= db->callstack[ db->pc-1 ];
	ASSERT( db->pc>0 );
	--db->pc;
	db->callstack[ db->pc ]->step = -1;
	db->callstack[ db->pc ] = NULL;

	switch ( stmt->type )
	{
	case SQL_UPDATE:
		return ((updateInfo_t*)stmt)->rows_updated;

	case SQL_SELECT:
		{
			selectInfo_t * select = (selectInfo_t*)stmt;
			if ( select->cache.row_count > 0 )
				sql_cache_pop( db, select->cache.result );
		} break;
	}

	return 0;
}

int sql_select( sqlInfo_t * db, const char * src, char * segment, char * buffer, int size, ... ) {

	char *	tmp_top		= db->cache.top;
	char *	tmp_buffer	= db->cache.buffer;
	int		tmp_size	= db->cache.size;
	char *	tmp_segment	= db->cache.segment;
	int		i;
	va_list	argptr;

	

	selectInfo_t * select = (selectInfo_t*)sql_prepare( db, src );

	ASSERT( select->stmt.type == SQL_SELECT );


	va_start(argptr, size);
	for ( i=0; i<MAX_COLUMNS_PER_TABLE && select->stmt.params[ i ].format != INVALID; i++ ) {

		if ( select->stmt.params[ i ].format == STRING ) {
			select->stmt.params[ i ].payload.string	= va_arg( argptr, char* );
		} else {
			select->stmt.params[ i ].payload.integer= va_arg( argptr, int );
		}
	}
	va_end(argptr);


	db->cache.buffer =
	db->cache.top = buffer;
	db->cache.size = size;
	db->cache.segment = segment;

	sql_select_work( db, select );

	sql_done( db );

	db->cache.top		= tmp_top;
	db->cache.buffer	= tmp_buffer;
	db->cache.size		= tmp_size;
	db->cache.segment	= tmp_segment;

	return select->cache.row_count;
}

static const unsigned int primes[] =
{
	3, 7, 11, 0x11, 0x17, 0x1D, 0x25, 0x2F, 0x3B, 0x47, 0x59, 0x6B, 0x83, 0xA3, 0xC5,
	0xEF, 0x125, 0x161, 0x1AF, 0x209, 0x277, 0x2F9, 0x397, 0x44F, 0x52F, 0x63D, 0x78B,
	0x91D, 0xAF1, 0xD2B, 0xFD1, 0x12FD, 0x16CF, 0x1B65, 0x20E3, 0x2777, 0x2F6F, 0x38FF,
	0x446F, 0x521F, 0x628D, 0x7655, 0x8E01, 0xAA6B, 0xCC89, 0xF583, 0x126A7, 0x1619B, 
	0x1A857, 0x1FD3B, 0x26315, 0x2DD67, 0x3701B, 0x42023, 0x4F361, 0x5F0ED, 0x72125,
	0x88E31, 0xA443B, 0xC51EB, 0xEC8C1, 0x11BDBF, 0x154A3F, 0x198C4F,  0x1EA867, 0x24CA19,
	0x2C25C1, 0x34FA1B, 0x3F928F, 0x4C4987, 0x5B8B6F, 0x6DDA89, //that last one's a bit over 7 million...god help us if we blow it
};

static unsigned int try_to_get_a_prime( unsigned int min )
{
	int i;

	for( i = 0; i < lengthof( primes ); i++ )
		if( primes[i] >= min )
			return primes[i];

	return min;
}

//	comes from common.c set by the 'outside' via SE_RANDSEED event
extern int com_randseed;

void sql_reset( sqlInfo_t * db, int memory_tag ) {

	Z_FreeTags( memory_tag );

	memset( db, 0, sizeof(sqlInfo_t) );
	db->memory_tag = memory_tag;

	db->cache.segment	=
	db->cache.top		=
	db->cache.buffer	= sql_alloc( db, SQL_CACHE_SIZE );
	db->cache.size		= SQL_CACHE_SIZE;

	db->strings			= sql_str_init_strings( try_to_get_a_prime( 512 ), memory_tag );

	Rand_Initialize( &db->rand, com_randseed );
}

void sql_assign_gs( sqlInfo_t * db, int * gs ) {
	db->gs = gs;
}

void sql_assign_triggers( sqlInfo_t * db, sqltrigger_t insert, sqltrigger_t update, sqltrigger_t delete ) {
		
	db->delete_trigger = delete;
	db->insert_trigger = insert;
	db->update_trigger = update;
}

//
//	memory
//
void * sql_alloc( sqlInfo_t * db, size_t size ) {
	return Z_TagMalloc(size,db->memory_tag);
}

void * sql_calloc( sqlInfo_t * db, size_t size ) {
	void * p = Z_TagMalloc(size,db->memory_tag);
	memset( p, 0, size );
	return p;
}

void * sql_realloc( sqlInfo_t * db, void * p, size_t size1, size_t size2 ) {
	void * pp = sql_alloc( db, size2 );
	if ( p ) {
		Com_Memcpy( pp,p,size1 );
		sql_free( db, p );
	}
	return pp;
}

void sql_free( sqlInfo_t * db, void * p ) {
	if ( p ) {
		Z_Free( p );
	}
}

void *	sql_cache_push	( sqlInfo_t * db, int n ) {
	void * p = db->cache.top;
	db->cache.top += n;
	ASSERT( db->cache.top - db->cache.buffer < db->cache.size );
	memset(p,0,n);
	return p;
}

void		sql_cache_pop	( sqlInfo_t * db, void * p ) {
	if ( p ) {
		db->cache.top = p;
	}
}

const char* sql_alloc_string( sqlInfo_t * db, const char * s ) {
	if ( s == NULL || (int)s == -1 ) {
		return "NULL";
	}
	else
	{
		return sql_str_intern( db->strings, s );
	}
}

const char* sql_alloc_stringn( sqlInfo_t * db, const char * s, int n ) {
	if ( s == NULL || (int)s == -1 ) {
		return "NULL";
	} else {
		return sql_str_intern_n( db->strings, s, n );
	}
}

void * sql_alloc_stmt( sqlInfo_t * db, int n ) {

	void * m;

	if ( db->stmt_buffer.c == db->stmt_buffer.p ) {

		if ( !db->stmt_buffer.p || db->stmt_buffer.p->top + n > SQL_STMT_ALLOC ) {

			db->stmt_buffer.c		=
			db->stmt_buffer.p		= sql_alloc( db, SQL_STMT_ALLOC );
			db->stmt_buffer.p->top	= sizeof( sqlStack_t );
		}
	} else {

		ASSERT( db->stmt_buffer.c && db->stmt_buffer.c->top + n <= SQL_STMT_ALLOC );
	}

	m = ((char*)db->stmt_buffer.c) + db->stmt_buffer.c->top;
	db->stmt_buffer.c->top += n;

	return m;
}

void * sql_calloc_stmt( sqlInfo_t * db, int n ) {
	return Com_Memset( sql_alloc_stmt( db, n ), 0, n );
}


int parse( const char ** data_p, char * buffer, int size, int * string_literal )
{
	const char *data;
	int len = 0;
	char c;

	data = *data_p;

	//	skip white space
	for ( ;; data++ )
	{
		c = *data;

		if ( c == '\0' || c == ';' )
		{
			buffer[ 0 ] = '\0';
			return 0;
		}

		if ( c <= ' ' || c == '\n' )
			continue;

		break;
	}

	// handle quoted strings
	if ( c == '\"' || c == '\'' )
	{
		int quote = c;
		data++;
		for( ; ; )
		{
			c = *data++;
			if( !c || c == quote )
			{
				if ( *data != quote )
					break;

				c = *data++;
			}

			if( len < size - 1 )
			{
				buffer[len++] = (char)c;
			}
		}
		if ( string_literal )
			*string_literal = 1;
		buffer[ len++ ] = '\0';
		*data_p = ( char * ) data;
		return len;
	}

	// parse a regular word
	len = 0;
	for ( ;; data++ )
	{
		c = *data;
		if ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			(c == '?') ||
			(c == '_') ||
			(c == '$') )
		{
			buffer[ len++ ] = c;
		} else
			break;
	}

	// parse an operator
	if ( len == 0 )
	{
		switch( CS(data[0],data[1],0,0) )
		{
		case CS('<','=',0,0):
		case CS('>','=',0,0):
		case CS('=','=',0,0):
		case CS('&','&',0,0):
		case CS('|','|',0,0):
			buffer[ len++ ] = *data++;
			buffer[ len++ ] = *data++;
			break;
		default:
			buffer[ len++ ] = *data++;
			break;
		}
	}

	buffer[ len++ ] = '\0';
	*data_p = data;
	return len;
}

int parse_tofirstparam( const char ** data_p )
{
	const char *data = *data_p;
	int c;

	for ( ;; data++ )
	{
		c = *data;
		if ( c == ' ' || c == '\t' || c == '\n' )
			continue;

		break;
	}

	if ( c == '(' ) {
		*data_p = data + 1;
		return 1;
	}

	*data_p = data;
	return 0;
}


int parse_tonextparam( const char ** data_p )
{
	const char *data = *data_p;
	int c;

	for ( ;; data++ )
	{
		c = *data;
		if ( c == ',' || c == ')' || c == '\0' )
			break;
	}

	*data_p = data+1;
	return c == ',';
}

int parse_tonextstatement( const char ** data_p )
{
	const char *data = *data_p;
	int c;

	for ( ;; data++ )
	{
		c = *data;

		// handle quoted strings
		if ( c == '\"' || c == '\'' )
		{
			int quote = c;
			data++;
			for( ; ; )
			{
				c = *data++;
				if( !c || c == quote )
				{
					if ( *data != quote )
						break;

					c = *data++;
				}
			}
		}


		if ( c == ';' || c == '\0' )
			break;
	}

	*data_p = data+1;
	return c == ';';
}

char * parse_temp( const char ** s )
{
	static char buffer[ 64 ];
	parse( s, buffer, sizeof(buffer), 0 );
	return buffer;
}

static void sql_export_text( fileHandle_t f, const char * fmt, ... ) {
	int len;
	char dest[ 8192 ];
	va_list argptr;
	va_start (argptr,fmt);
	len = vsnprintf( dest, sizeof(dest), fmt, argptr );
	va_end (argptr);
	
	FS_Write( dest, len, f );
}

static int sql_export_escape( char * dst, int size, const char * src, char quote ) {

	int i,j=0;

	dst[ j++ ] = quote;
	for ( i=0; src[i] && j<size; i++ ) {
		
		dst[ j++ ] = src[ i ];

		if ( src[ i ] == quote ) {
			dst[ j++ ] = quote;
		}
	}

	dst[ j++ ] = quote;
	dst[ j ] = '\0';

	return j;
}

static void sql_export_table( fileHandle_t f, tableInfo_t * table ) {
/*
CREATE TABLE commodities_npcs (
	"commodity_id" integer,
	"npc_id" integer
);
	INSERT INTO "commodities_npcs" VALUES(13, 17);
*/
	char * prefix;
	int i;

	if ( table->flags&SQL_TABLE_CLIENTONLY ) {
		prefix = "cl_";
	} else if ( table->flags&SQL_TABLE_SERVERONLY ) {
		prefix = "sv_";
#ifdef DEVELOPER
	} else if ( table->flags&SQL_TABLE_BOTH ) {
		prefix="bg_";
#endif
	} else {
		prefix = "";
	}

	//
	//	CREATE TABLE
	//
	sql_export_text( f, "CREATE TABLE %s%s (\n", prefix, table->name );

	for ( i=0; i<table->column_count; i++ ) {
		const char * type = (table->columns[ i ].format==STRING)?"string":"integer";
		if ( table->primary_column == (i+1) ) {
			type = "unique";
		}
		sql_export_text( f, "\t%s %s%s\n", table->columns[ i ].name, type, (i==table->column_count-1)?"":"," );
	}

	sql_export_text( f, ");\n" );

	//
	//	INSERT INTO
	//
	if ( table->row_count>0 ) {
		sql_export_text( f, "INSERT INTO %s%s ROWS (\n", prefix, table->name );

		for ( i=0; i<table->row_count; i++ ) {
			int j;
			char buff[8192];

			sql_export_text( f, "\t" );
			for ( j=0; j<table->column_count; j++ ) {

				char * fmt;
				if ( j == table->column_count-1 ) {

					if ( i == table->row_count-1 ) {
						fmt = (table->columns[ j ].format == STRING )?"%s\n":"%d\n";
					} else {
						fmt = (table->columns[ j ].format == STRING )?"%s,\n":"%d,\n";
					}
				} else {
					fmt = (table->columns[ j ].format == STRING )?"%s,\t":"%d,\t";
				}
				if ( table->columns[ j ].format == STRING ) {
					if ( table->rows[ i*table->column_count+j ].string ) {
						sql_export_escape( buff, sizeof(buff), table->rows[ i*table->column_count+j ].string, '\''); 
						sql_export_text( f, fmt, buff );
					} else {
						sql_export_text( f, fmt, "''" );
					}
					
				} else {
					sql_export_text( f, fmt, table->rows[ i*table->column_count+j ].integer );
				}
			}
		}
		sql_export_text( f, ");\n" );
	}
}

static void sql_export_db( fileHandle_t f, sqlInfo_t * db, int server ) {
	int i;
	for ( i=0; i<db->table_count; i++ ) {

		tableInfo_t * table = db->tables[ i ];

		if ( table->flags&SQL_TABLE_TEMPORARY ) {
			continue;
		}

		if ( !server && !(table->flags&SQL_TABLE_CLIENTONLY) ) {
			continue;
		}

		sql_export_table( f, table );
	}
}

void sql_export( const char * filename ) {
	char clientdb[MAX_OSPATH];
	fileHandle_t f;

	if ( FS_FOpenFileByMode( filename, &f, FS_WRITE ) == -1 ) {
		Com_Printf( "cannot open %s for sql export\n", filename );
		return;
	}

	sql_export_db( f, sql_getserverdb(), 1 );
	
	FS_FCloseFile( f );
	
	if ( Cvar_VariableIntegerValue( "sql_export_text" ) == 1 ) {
		COM_StripExtension(filename, clientdb, sizeof(clientdb));
		Q_strcat( clientdb, sizeof(clientdb), "_en-US.txt");
		if ( FS_FOpenFileByMode( clientdb, &f, FS_WRITE ) == -1 ) {
			Com_Printf( "cannot open %s for sql export\n", filename );
			return;
		}
		sql_export_db( f, sql_getclientdb(), 0 );
	
		FS_FCloseFile( f );
	}
}
#ifdef DEVELOPER
static void sql_save_db( fileHandle_t f, sqlInfo_t * db, int server ) {
	int i;
	for ( i=0; i<db->table_count; i++ ) {

		tableInfo_t * table = db->tables[ i ];

		if ( table->flags&SQL_TABLE_TEMPORARY ) { // no temp tables
			continue;
		}

		if ( !server && !(table->flags&SQL_TABLE_CLIENTONLY) ) { // if it's not the server, and not client only, skip
			continue;
		}
		if ( server && (table->flags & SQL_TABLE_CLIENTONLY) ) { // if it is server, and not SERVERONLY or BOTH skip
			continue;
		}

		sql_export_table( f, table );
	}
}

void sql_save( const char * filename ) {
	char clientdb[MAX_OSPATH];
	fileHandle_t f;

	if ( FS_FOpenFileByMode( filename, &f, FS_WRITE ) == -1 ) {
		Com_Printf( "cannot open %s for sql export\n", filename );
		return;
	}
	
	// prepare db for export
	sql_exec( sql_getserverdb(), "UPDATE missions SET value = 0 SEARCH key 'hidden';");
	
	// these are re-generated on db load   DELETE FROM contacts; DELETE FROM todaysprices; DELETE FROM todaysnews;
	sql_exec( sql_getserverdb(), "DELETE FROM prices;");
	sql_exec( sql_getserverdb(), "DELETE FROM contacts;");
	sql_exec( sql_getserverdb(), "DELETE FROM todaysprices;");
	sql_exec( sql_getserverdb(), "DELETE FROM todaysnews;");
	sql_exec( sql_getserverdb(), "DELETE FROM purchases;");
	sql_exec( sql_getserverdb(), "DELETE FROM inventory;");
	sql_exec( sql_getserverdb(), "DELETE FROM flags;");
	sql_exec( sql_getserverdb(), "DELETE FROM travels;");
	sql_exec( sql_getserverdb(), "DELETE FROM history;");
	sql_exec( sql_getserverdb(), "DELETE FROM players;");
	sql_exec( sql_getserverdb(), "DELETE FROM stash;");
	
	// these are cached on the client side, so zero them out on export
	sql_exec( sql_getclientdb(), "UPDATE commodities_text SET icon=0, chart=0;" );
	
	
	sql_save_db( f, sql_getserverdb(), 1 );
	
	FS_FCloseFile( f );
	
	COM_StripExtension(filename, clientdb, sizeof(clientdb));
	Q_strcat( clientdb, sizeof(clientdb), "_en-US.txt");
	if ( FS_FOpenFileByMode( clientdb, &f, FS_WRITE ) == -1 ) {
		Com_Printf( "cannot open %s for sql export\n", filename );
		return;
	}
	sql_save_db( f, sql_getclientdb(), 0 );

	FS_FCloseFile( f );
}
#endif
static void FS_WriteString( const char * s, fileHandle_t f ) {
	FS_Write( s, strlen(s), f );
}
void sql_graph( sqlInfo_t * db, fileHandle_t f ) {

	int i,j;

	FS_WriteString( "digraph tables {\n", f );

	for ( i=0; i<db->table_count; i++ ) {

		tableInfo_t * table = db->tables[ i ];

		FS_WriteString ( va( "%s [shape=record,label=\"%s|{", table->name, table->name ), f );

		for ( j=0; j<table->column_count; j++ ) {
			if ( j>0 )
				FS_WriteString( "|", f );

			FS_WriteString( va( "<%s> %s", table->columns[ j ].name, table->columns[ j ].name ), f );
		}

		FS_WriteString ( "}\"];\n", f );
	
	}

	FS_WriteString( "}\n", f );

	

	FS_FCloseFile( f );
}
