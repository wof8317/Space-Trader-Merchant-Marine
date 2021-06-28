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
#include "../qcommon/rand.h"

#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif

#define MAX_COLUMNS_PER_TABLE	32
#define MAX_TABLES_PER_DB		48
#define MAX_STMTS_PER_DB		1024
#define MAX_CALL_STACK			8
#define SQL_CACHE_SIZE			65535


#ifdef DEVELOPER
extern const char * CURRENT_STMT;
#endif

//
// sql_strings.h
//
struct sql_strings;
typedef struct sql_strings sql_strings;

extern sql_strings*	sql_str_init_strings	( unsigned int hash_size, int alloc_tag ); //initializes the string pool, set hash_size to an adequately sized prime for efficiency (should be at least a third of the number of strings you expect to intern at once)

extern const char*	sql_str_intern			( sql_strings *strs, const char *str ); //add a string to the sql interned strings table - if it's already there it gets addref'd
extern const char*	sql_str_intern_n		( sql_strings *strs, const char *str, unsigned int n ); //same as sql_str_intern, only stop reading str at n chars
extern const char*	sql_str_is_interned		( sql_strings *strs, const char *str ); //if a string has been interned, return the interned pointer else return null - string is NOT addref'd
extern const char*	sql_str_is_interned_n	( sql_strings *strs, const char *str, unsigned int n  ); //same as sql_str_is_interned, only stop reading str at n chars

//
//	sql_parse.c
//
typedef enum {
	OP_END,
	OP_NOP,
	OP_ASSIGN_INT,
	OP_LOGICAL_AND,
	OP_LOGICAL_OR,
	OP_BITWISE_AND,
	OP_BITWISE_OR,
	OP_SUBTRACT,
	OP_ADD,
	OP_DIVIDE,
	OP_MULTIPLY,
	OP_MODULUS,
	OP_REMOVE,

	OP_EQ,
	OP_LE,
	OP_NE,
	OP_LSHIFT,
	OP_LT,
	OP_GE,
	OP_GT,
	OP_RSHIFT,

	OP_NOT,
	OP_NEGATE,

	OP_UMINUS,

	OP_PUSH_INTEGER,
	OP_PUSH_INTEGER_PARAM,
	OP_PUSH_INTEGER_GLOBAL,
	OP_PUSH_STRING,
	OP_PUSH_STRING_PARAM,
	OP_PUSH_COLUMN_VAL,
	OP_PUSH_GS,
	OP_PUSH_GS_OFFSET,
	OP_PUSH_PS_CLIENT,
	OP_PUSH_PS_CLIENT_OFFSET,

	OP_PUSH_COLUMN,
	OP_ASSIGN_INT_TO_COLUMN,
	OP_ASSIGN_STRING_TO_COLUMN,

	OP_ASSIGN_CS_TO_ROW,

	OP_CONCAT,
	OP_FORMAT,
	OP_ATOI,
	OP_LIKE,
	OP_MATCH,
	OP_NOTLIKE,
	OP_COLLAPSE,

	OP_COUNT,
	OP_SUM,
	OP_MAX,
	OP_MIN,

	OP_ACCESS_TABLE,
	OP_LOOKUP_I,
	OP_LOOKUP_S,
	OP_ACCESS_ROW_I,
	OP_ACCESS_ROW_S,

	OP_ROWINDEX,
	OP_ROWNUMBER,
	OP_ROWTOTAL,
	OP_ROWCOUNT,

	OP_IFTHENELSE,

	OP_SHADER,
	OP_SOUND,
	OP_MODEL,
	OP_PORTRAIT,
	OP_RND,

	OP_INT_MIN,
	OP_INT_MAX,
	OP_ABS,

	OP_EVAL,
	OP_PRINT,
	OP_RUN,

	OP_SYS_TIME,

	OP_CVAR,
	

} op_t;

typedef union {
	int		i;
	const char *	s;
	void *	p;
} value_t;

typedef struct expr_s {
	
	op_t	op;
	value_t	v;
	int		n;

	struct expr_s * left;
	struct expr_s * right;

} expr_t;

typedef enum {
	INVALID,
	INTEGER,
	STRING,
	COLUMN,
	ROW,
	PARAMETER,
	AGGREGATE,
	
} dataFormat_t;


expr_t *	op		( expr_t * left, expr_t * right, op_t op );
expr_t *	expr_i	( int i );
expr_t *	expr_s	( const char * s, int n );
int			gs_i	( const char* s );
int			ps_i	( const char* s );
void		compile	( expr_t * A, dataFormat_t rt );
int			local	( const char* s, int n );

extern int		parse					( const char ** data_p, char * buffer, int size, int * string_literal );
extern int		parse_tofirstparam		( const char ** data_p );
extern int		parse_tonextparam		( const char ** data_p );
extern int		parse_tonextstatement	( const char ** data_p );
extern char *	parse_keep				( const char ** s );
extern char *	parse_temp				( const char ** s );

//
//	sql.c
//

#define SQL_STMT_ALLOC	12*1024

typedef struct {
	int		top;
} sqlStack_t;

typedef union {
	int			integer;
	const char	*string;
} cellInfo_t;

typedef struct {
	dataFormat_t		format;
	cellInfo_t			payload;
} sqlData_t;

typedef struct {
	int				num;
	dataFormat_t	format;
	short *			index;
	const char *	name;

#ifdef DEVELOPER
	struct {
		const char *	table;
		const char *	column;
	} link;
#endif

} columnInfo_t;

#define		SQL_TABLE_FROMSERVER		0x0001		// a table on the client db that is sync'd from the server
#define		SQL_TABLE_CLIENTONLY		0x0002		// table that is only on the client
#define		SQL_TABLE_SERVERONLY		0x0004		// table that is only on the server
#define		SQL_TABLE_TEMPORARY			0x0008		// temporary, not written to disk
#define		SQL_TABLE_BOTH				0x0010		// a table on both client and server but is not sync. they are expected to be the same!

typedef struct {

	const char *	name;

	columnInfo_t	columns[ MAX_COLUMNS_PER_TABLE ];
	int				column_count;

	cellInfo_t *	rows;
	int				row_count;

	int				row_total;

	struct stmtInfo_t *	sync;

	int				last_changed;
	int				last_indexed;
	int				flags;

	int				primary_column;

	unsigned int	modified;	// bit field of columns just modified

} tableInfo_t;


typedef char * Expr;

typedef enum {
	SQL_INSERT,
	SQL_UPDATE,
	SQL_CREATE,
	SQL_SELECT,
	SQL_DELETE,
	SQL_FORMAT,
} stmtType_t;

typedef struct stmtInfo_t {

	stmtType_t			type;				// type of statement
	const char *		src;
	tableInfo_t *		table;
	sqlData_t			params[ MAX_COLUMNS_PER_TABLE ];		// bound arguments '?'

	int					step;

} stmtInfo_t;

typedef struct {

	int			column;
	Expr		start;
	Expr		end;
	int			descending;
	int			unique;

} searchInfo_t;

typedef struct {
	stmtInfo_t		stmt;

	Expr			column_expr[ MAX_COLUMNS_PER_TABLE ];
	dataFormat_t	column_type[ MAX_COLUMNS_PER_TABLE ];
	const char *	column_name[ MAX_COLUMNS_PER_TABLE ];
	int				column_count;
	Expr			where_expr;
	Expr			limit;
	Expr			offset;
	Expr			random;
	Expr			sort;
	int				is_aggregate;
	int				entire_row;

	searchInfo_t	search;

	struct {
		cellInfo_t	*	result;
		int				row_count;
	} cache;


} selectInfo_t;

typedef struct {
	stmtType_t			type;				// type of statement
	Expr				print;

} formatInfo_t;

typedef struct {
	stmtInfo_t		stmt;

	Expr			values_expr[ MAX_COLUMNS_PER_TABLE ];
	int				values_count;

	selectInfo_t *	select;	
	char			columns[ MAX_COLUMNS_PER_TABLE ];

} insertInfo_t;

typedef struct {
	stmtInfo_t		stmt;

	Expr			assignments[ MAX_COLUMNS_PER_TABLE ];
	int				assignmentCount;
	Expr			where_expr;
	Expr			limit;
	Expr			offset;

	searchInfo_t	search;

	int				rows_updated;
	int				orinsert;

} updateInfo_t;

typedef struct {
	stmtInfo_t		stmt;

	Expr			where_expr;
} deleteInfo_t;

struct sqlInfo_s;

typedef void (*sqltrigger_t)( struct sqlInfo_s * db, tableInfo_t * table, int row );

typedef struct sqlInfo_s {

	tableInfo_t *		tables	[ MAX_TABLES_PER_DB ];
	int					table_count;

	stmtInfo_t *		stmts	[ MAX_STMTS_PER_DB ];
	int					stmt_count;

	stmtInfo_t *		stmts_byindex[ MAX_STMTS_PER_DB ];
	int					stmts_byindex_count;

	stmtInfo_t *		callstack[ MAX_CALL_STACK ];
	int					pc;

	sqltrigger_t		insert_trigger;
	sqltrigger_t		update_trigger;
	sqltrigger_t		delete_trigger;

	int *				gs;
	int *				ps;
	
	randState_t			rand;

	int					memory_tag;

	struct {
		sqlStack_t *	c;	//	current
		sqlStack_t *	p;	//	permanent
	} stmt_buffer;

	sql_strings			*strings;

	struct {
		char *	segment;
		char *	top;
		char *	buffer;
		int		size;
	} cache;

	//	functions
	qhandle_t		(*shader)		( const char *name );
	qhandle_t		(*sound)		( const char *name );
	qhandle_t		(*portrait)		( const char * );
	qhandle_t		(*model)		( const char * );
	int				(*global_int)	( const char * );

#ifdef DEVELOPER
	int		ops;
#endif

	char *			create_filter;

} sqlInfo_t;

extern void *	sql_cache_push	( sqlInfo_t * db, int n );
extern void		sql_cache_pop	( sqlInfo_t * db, void * p );

//	makes a copy of a string which can never be deleted
extern const char *	sql_alloc_string	( sqlInfo_t * db, const char * s );
extern const char *	sql_alloc_stringn	( sqlInfo_t * db, const char * s, int n );

//	memory
extern void *	sql_calloc	( sqlInfo_t * db, size_t size );
extern void *	sql_alloc	( sqlInfo_t * db, size_t size );
extern void *	sql_realloc	( sqlInfo_t * db, void * p, size_t size1, size_t size2 );
extern void		sql_free	( sqlInfo_t * db, void * p );

extern void *	sql_alloc_stmt	( sqlInfo_t * db, int n );
extern void *	sql_calloc_stmt	( sqlInfo_t * db, int n );




extern tableInfo_t *	find_table			( sqlInfo_t * db, const char * name );
extern columnInfo_t *	find_column			( tableInfo_t * table, const char * name );
extern stmtInfo_t *		find_stmt			( sqlInfo_t * db, const char * src );
extern void				insert_stmt			( sqlInfo_t * db, stmtInfo_t * stmt );
extern void				insert_table		( sqlInfo_t * db, tableInfo_t * table );


extern int sql_insert_work( sqlInfo_t * db, insertInfo_t * insert );
extern int sql_select_work( sqlInfo_t * db, selectInfo_t * select );
extern int sql_update_work( sqlInfo_t * db, updateInfo_t * update );
extern int sql_delete_work( sqlInfo_t * db, deleteInfo_t * delete );
extern const char * sql_format_work( sqlInfo_t * db, formatInfo_t * format );

extern stmtInfo_t * sql_parse		( sqlInfo_t * db, const char ** data_p, char * tmp );
extern stmtInfo_t * sql_create_parse( sqlInfo_t * db, const char ** data_p );
extern stmtInfo_t * sql_select_parse( sqlInfo_t * db, const char ** data_p );
extern stmtInfo_t * sql_insert_parse( sqlInfo_t * db, const char ** data_p );
extern stmtInfo_t * sql_delete_parse( sqlInfo_t * db, const char ** data_p );
extern stmtInfo_t * sql_update_parse( sqlInfo_t * db, const char ** data_p );
extern stmtInfo_t * sql_format_parse( sqlInfo_t * db, const char ** data_p );
extern stmtInfo_t * sql_alter_parse	( sqlInfo_t * db, const char ** data_p );

extern void			sql_create_index( sqlInfo_t * db, tableInfo_t * table, columnInfo_t * column );

extern void			sql_insert_done	( sqlInfo_t * db, tableInfo_t * table );
extern void			sql_update_done	( sqlInfo_t * db, tableInfo_t * table );

extern void			sql_table_reindex( tableInfo_t * table );


extern cellInfo_t *	sql_insert_begin( sqlInfo_t * db, tableInfo_t * table );
extern void			sql_insert_done	( sqlInfo_t * db, tableInfo_t * table );

extern void			sql_table_addcolumns( sqlInfo_t * db, tableInfo_t * table, const char ** s );

//
//	sql_eval.c
//

#define		PARSE_ALLOW_ASSIGNMENT		0x00001		// interprets '=' as assignment not equality
#define		PARSE_ASSIGN_TO_COLUMN		0x00002		// adds ops to assign the final result to a column
#define		PARSE_STRINGLITERAL			0x00004		// parse assuming input is a string literal
#define		PARSE_ASSIGN_CS				0x00008		// parse as to assign a configstring to a row

typedef struct {

	//	in
	sqlInfo_t *		db;
	tableInfo_t *	table;		// the table that is in scope
	int 			arg;		// the last argument that's been assigned to a '?'
	int				flags;		// PARSE_* flags
	int				column;

	//	out
	const char **	as;			// optional name that the result is being called.
	sqlData_t *		params;		// stores the parameter types i.e ?1 or $1
	char *			z;			// the resulting expression
	int				n;			// expression length
	dataFormat_t	rt;			// expression return type
	int				more;		// there's more expressions to parse

	//	temp
	void *			cache;

} parseInfo_t;

extern Expr			parse_expression		(	const char ** expression,		// the regular expression to be parsed
												parseInfo_t * pi
											);

extern cellInfo_t	sql_eval				(	sqlInfo_t * db,
												Expr expr,				// the expression to be evaluated
												tableInfo_t * table,	// the table in scope
												cellInfo_t * row,		// the row in scope
												int	index,				// the index of this row
												int total,				// the total rows in this statement
												sqlData_t * params,		// currently bound parameters
												cellInfo_t * aggregate	// aggregate storage
											);
extern int			sql_eval_where				( sqlInfo_t * db, Expr where_expr, tableInfo_t * table, sqlData_t * params, cellInfo_t ** rows, int limit, int offset );
extern int			sql_eval_search_and_where	( sqlInfo_t * db, searchInfo_t * search, Expr where_expr, tableInfo_t * table, sqlData_t * params, cellInfo_t ** rows, int limit, int offset );

//
//	sql_search.c
//
extern void *		search					( const void * base, int count, int sizeOfElement, const void * key, int( *compare)(const void*, const void*) );
extern void			insert_index_i			( short * index, int count, cellInfo_t * rows, int column_count, int column, short row_index );
extern void			insert_index_s			( short * index, int count, cellInfo_t * rows, int column_count, int column, short row_index );
extern int			search_index_i_first	( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key );
extern int			search_index_i_last		( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key );
extern int			search_index_s_first	( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key );
extern int			search_index_s_last		( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key );

extern int			search_index_i			( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key );
extern int			search_index_s			( short * index, int count, cellInfo_t * rows, int column_count, int column, cellInfo_t key );

//
//	public
//
extern stmtInfo_t *	sql_prepare				( sqlInfo_t * db, const char * src );
extern int			sql_bind				( sqlInfo_t	* db, intptr_t *args );
extern int			sql_bindtext			( sqlInfo_t * db, int arg, const char * text );
extern int			sql_bindint				( sqlInfo_t * db, int arg, int integer );
extern int			sql_step				( sqlInfo_t * db );
extern int			sql_exec				( sqlInfo_t * db, const char * src );
extern void			sql_prompt				( sqlInfo_t * db, char * src );
extern int			sql_columncount			( sqlInfo_t * db );
extern const char *	sql_columnastext		( sqlInfo_t * db, int column );
extern int			sql_columnasint			( sqlInfo_t * db, int column );
extern const char * sql_columnname			( sqlInfo_t * db, int column );
extern int			sql_rowcount			( sqlInfo_t * db );
extern int			sql_done				( sqlInfo_t * db );

extern int			sql_expr				( sqlInfo_t * db, const char * src );

extern int			sql_select				( sqlInfo_t * db, const char * src, char * segment, char * buffer, int size, ... );

//	same as 'prepare' but in two steps
extern int			sql_compile				( sqlInfo_t * db, const char * src );
extern const char *	sql_run					( sqlInfo_t * db, int id );



extern void			sql_error				( const char * msg );


extern void			sql_reset				( sqlInfo_t * db, int memory_tag );
extern void			sql_assign_triggers		( sqlInfo_t * db, sqltrigger_t insert, sqltrigger_t update, sqltrigger_t delete );
extern void			sql_assign_gs			( sqlInfo_t * db, int * gs );

extern void			sql_export				( const char * filename );
#ifdef DEVELOPER
extern void			sql_save				( const char * filename );
#endif

extern sqlInfo_t *	sql_getclientdb			( void );
extern sqlInfo_t *	sql_getserverdb			( void );
extern sqlInfo_t *	sql_getcommondb			( void );
