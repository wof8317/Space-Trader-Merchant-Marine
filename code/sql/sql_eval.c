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

static int concat2( char * buffer, int size, value_t * stack, int n ) {

	int i,j,t;
	const char * s;

	for ( i=0,j=0,t=0,s=0; i<(size-1); i++,j++ ) {

		if ( !s || !s[j] ) {
			if ( t >= n )
				break;
			s = stack[ t++ ].s;
			j=0;
		}

		buffer[ i ] = s[ j ];
	}

	buffer[ i++ ] = '\0';
	return i;
}

static char * concat( const char * A, const char * B, char * buffer, int size, int * used ) {
	int i;
	int n = *used;
	char * r = buffer + n;

	size--;	// room for null

	for ( i=0; n<size && A[i]; i++ ) {
		buffer[ n++ ] = A[i];
	}

	for ( i=0; n<size && B[i]; i++ ) {
		buffer[ n++ ] = B[i];
	}

	buffer[ n++ ] = '\0';
	*used = n;

	return r;
}

static void row_cs( sqlInfo_t * db, tableInfo_t * table, cellInfo_t * row, const char * cs ) {

	columnInfo_t * c;
	char	key		[ MAX_INFO_KEY ];
	char	value	[ MAX_INFO_VALUE ];

	for ( ; ; ) {
		Info_NextPair( &cs, key, value );

		if ( key[ 0 ] == '\0' )
			break;

		c = find_column( table, key );
		if ( c ) {
			if ( c->format == INTEGER ) {

				int v = atoi( value );
				if ( row[ c->num ].integer != v ) {
					table->modified |= (1<<c->num);
					row[ c->num ].integer = v;
				}
			} else {
				const char * s = sql_alloc_string( db, value );
				if ( Q_stricmp( row[ c->num ].string, s ) ) {
					table->modified |= (1<<c->num);
					row[ c->num ].string = s;
				}
			}
		}
	}
}

//	evaluate the expression string 'expr' down to 1 value in 'r'
#define READ_OP	expr[ i++ ]
#define READ_INT *((int*)&expr[i]); i+=4
#define READ_STRING (char*)&expr[i] ; while( expr[i] != '\0' ) i++ ; i++
#define LEFT_OPERAND stack[ sp-2 ]
#define RIGHT_OPERAND stack[ sp-1 ]
#define LVALUE LEFT_OPERAND

cellInfo_t sql_eval( sqlInfo_t *db, Expr expr, tableInfo_t * table, cellInfo_t * row, int index, int total, sqlData_t * params, cellInfo_t * aggregate )
{
	value_t		stack[ 64 ];
	int sp;
	int i;

	static char	buffer[ 16384 ];		// FIX ME: this is the source of bugs
	static int size = 0;

	int top = size;

	for ( i=0,sp=0; expr[ i ] != OP_END; ) {

		op_t op = READ_OP;

		switch( op ) {

			case OP_PUSH_INTEGER:		stack[ sp++ ].i = READ_INT;								break;
			case OP_PUSH_STRING:		stack[ sp++ ].s = READ_STRING;							break;
			case OP_PUSH_COLUMN:		stack[ sp++ ].p = table->columns + READ_OP;				break;
			case OP_PUSH_COLUMN_VAL:	stack[ sp++ ].i	= row[ READ_OP ].integer;				break;
			case OP_PUSH_STRING_PARAM:	stack[ sp++ ].s = params[ READ_OP ].payload.string;		break;
			case OP_PUSH_INTEGER_PARAM:	stack[ sp++ ].i = params[ READ_OP ].payload.integer;	break;
			case OP_ROWINDEX:			stack[ sp++ ].i = (row - table->rows) / table->column_count;	break;
			case OP_ROWNUMBER:			stack[ sp++ ].i = index; break;
			case OP_ROWTOTAL:			stack[ sp++ ].i = total; break;
			case OP_ROWCOUNT:			stack[ sp++ ].i = table->row_count; break;
			case OP_SYS_TIME:			stack[ sp++ ].i = Sys_Milliseconds(); break;
			

			case OP_SUBTRACT:		LVALUE.i = LEFT_OPERAND.i	-	RIGHT_OPERAND.i; sp--;	break;
			case OP_ADD:			LVALUE.i = LEFT_OPERAND.i	+	RIGHT_OPERAND.i; sp--;	break;
			case OP_DIVIDE:			LVALUE.i = LEFT_OPERAND.i	/	RIGHT_OPERAND.i; sp--;	break;
			case OP_MULTIPLY:		LVALUE.i = LEFT_OPERAND.i	*	RIGHT_OPERAND.i; sp--;	break;
			case OP_MODULUS:		LVALUE.i = LEFT_OPERAND.i	%	RIGHT_OPERAND.i; sp--;	break;

			case OP_LOGICAL_AND:	LVALUE.i = LEFT_OPERAND.i	&&	RIGHT_OPERAND.i; sp--;	break;
			case OP_LOGICAL_OR:		LVALUE.i = LEFT_OPERAND.i	||	RIGHT_OPERAND.i; sp--;	break;
			case OP_BITWISE_AND:	LVALUE.i = LEFT_OPERAND.i	&	RIGHT_OPERAND.i; sp--;	break;
			case OP_BITWISE_OR:		LVALUE.i = LEFT_OPERAND.i	|	RIGHT_OPERAND.i; sp--;	break;

			case OP_GT:				LVALUE.i = LEFT_OPERAND.i	>	RIGHT_OPERAND.i; sp--;	break;
			case OP_LT:				LVALUE.i = LEFT_OPERAND.i	<	RIGHT_OPERAND.i; sp--;	break;
			case OP_GE:				LVALUE.i = LEFT_OPERAND.i	>=	RIGHT_OPERAND.i; sp--;	break;
			case OP_LE:				LVALUE.i = LEFT_OPERAND.i	<=	RIGHT_OPERAND.i; sp--;	break;
			case OP_EQ:				LVALUE.i = LEFT_OPERAND.i	==	RIGHT_OPERAND.i; sp--;	break;
			case OP_NE:				LVALUE.i = LEFT_OPERAND.i	!=	RIGHT_OPERAND.i; sp--;	break;

			case OP_ATOI:
				if (stack[ sp-1 ].s) {
					stack[ sp-1 ].i = atoi( stack[ sp-1 ].s );
				} else {
					stack[ sp-1 ].i = -1;
				}
				break;
			case OP_LIKE:			LVALUE.i = Q_stricmp( LEFT_OPERAND.s, RIGHT_OPERAND.s ) == 0; sp--;	break;
			case OP_MATCH:			LVALUE.i = Com_Filter( RIGHT_OPERAND.s, LEFT_OPERAND.s, 0 ); sp--; break;
			case OP_NOTLIKE:		LVALUE.i = Q_stricmp( LEFT_OPERAND.s, RIGHT_OPERAND.s ) != 0; sp--; break;
			case OP_INT_MIN:		LVALUE.i = (LEFT_OPERAND.i<RIGHT_OPERAND.i)?LEFT_OPERAND.i:RIGHT_OPERAND.i; sp--; break;
			case OP_INT_MAX:		LVALUE.i = (LEFT_OPERAND.i>RIGHT_OPERAND.i)?LEFT_OPERAND.i:RIGHT_OPERAND.i; sp--; break;
			case OP_ABS:			stack[ sp-1 ].i = abs( stack[ sp-1 ].i ); break;

			case OP_UMINUS:			
				stack[ sp-1 ].i = -stack[ sp-1 ].i;
				break;

			case OP_NOT:
				stack[ sp-1 ].i = !(stack[ sp-1 ].i);
				break;

			case OP_REMOVE:
				{
					int p = READ_OP;
					int n = min( params[ p ].payload.integer, stack[ sp-1 ].i );
					params[ p ].payload.integer -= n;
					stack[ sp-1 ].i = n;
				} break;


			case OP_ASSIGN_INT_TO_COLUMN:
				{
					columnInfo_t *	c		= (columnInfo_t*)LEFT_OPERAND.p;
					ASSERT( c->num >= 0 && c->num < table->column_count );

					if ( row[ c->num ].integer != RIGHT_OPERAND.i ) {
						table->modified |= (1<<c->num);
					}

					LVALUE.i = row[ c->num ].integer	= RIGHT_OPERAND.i; sp--;
				} break;

			case OP_ASSIGN_STRING_TO_COLUMN:
				{
					//	a string is being inserted into a table.  this string is expected to remain
					//	constant throughout the life of the table.  strings stored in tables do not
					//	change.  string cells can not be modified with an 'UPDATE' command
					columnInfo_t *	c		= (columnInfo_t*)LEFT_OPERAND.p;
					cellInfo_t *	cell	= row + c->num;
					const char *	o		= cell->string;

					ASSERT( c->format == STRING );
					//ASSERT( cell->string == 0 );

					//	help!!
					if ( abs(RIGHT_OPERAND.i) < 0x10000 ) {
						//	can't figure out data type for cases like: INSERT INTO missions VALUES(7,'wealth',500); 3rd column in text
						//	so trying to guess
						LVALUE.s = cell->string = sql_alloc_string( db, fn( RIGHT_OPERAND.i, FN_PLAIN ) );  sp--;
					} else { 
						LVALUE.s = cell->string = sql_alloc_string( db, RIGHT_OPERAND.s ); sp--;
					}

					if ( Q_stricmp( o, cell->string ) ) {
						table->modified |= (1<<c->num);
					}

				} break;

			case OP_ASSIGN_CS_TO_ROW:
				{
					row_cs( db, table, row, stack[ sp-1 ].s );
				} break;

			case OP_COUNT:	(*aggregate).integer++;			break;
			case OP_MAX:
				{
					int v = stack[ sp-1 ].i;
					(*aggregate).integer = (index==0)?v:max( (*aggregate).integer, v );
				} break;
			case OP_MIN:
				{
					int v = stack[ sp-1 ].i;
					(*aggregate).integer = (index==0)?v:min( (*aggregate).integer, v );
				} break;


			case OP_SUM:
				(*aggregate).integer += stack[ sp-1 ].i;
				break;

			case OP_FORMAT:
				{
					char *	s		= buffer + size;
					int		flags	= READ_INT;

					size += fn_buffer( s, stack[ sp-1 ].i, flags );
					stack[ sp-1 ].s = s;

				} break;

			case OP_CONCAT:
				{
					LVALUE.s = concat( LEFT_OPERAND.s, RIGHT_OPERAND.s, buffer, sizeof(buffer), &size ); sp--;

				} break;

			case OP_COLLAPSE:
				{
					char * s = buffer + size;
					size += concat2( s, sizeof(buffer)-size, stack, sp );
					stack[ 0 ].s = s;
					sp = 1;
				} break;

			case OP_CVAR:
				{
					stack[ sp-1 ].s = Cvar_VariableString( stack[ sp-1 ].s );
				} break;

			case OP_ACCESS_TABLE:
				{
					tableInfo_t *	table;
					columnInfo_t *	c;

					table = find_table( db, LEFT_OPERAND.s );

					//	allow table access outside current db
					if ( !table ) {
						table = find_table( sql_getclientdb(), LEFT_OPERAND.s );
						if ( !table ) {
							table = find_table( sql_getserverdb(), LEFT_OPERAND.s );
							if ( !table ) {
								table = find_table( sql_getcommondb(), LEFT_OPERAND.s );
							}
						}
					}

#ifdef DEVELOPER
					if ( !table ) {
						Com_Error( ERR_FATAL, "table '%s' does not exist.\n\n%s", LEFT_OPERAND.s, CURRENT_STMT );
					}
#endif

					c = find_column( table, RIGHT_OPERAND.s );

#ifdef DEVELOPER
					if ( !c ) {
						Com_Error( ERR_FATAL, "column '%s' expected on table '%s'.\n\n%s\n", RIGHT_OPERAND.s, LEFT_OPERAND.s, CURRENT_STMT );
					}
#endif

					LVALUE.p		= table; sp--;
					stack[ sp++ ].p	= c;

				} break;

			case OP_LOOKUP_I:
				{
					tableInfo_t	*	t = stack[ sp-3 ].p;
					columnInfo_t *	c = stack[ sp-2 ].p;
					cellInfo_t		k;
					int				index;

					k.integer = stack[ sp-1 ].i;

					if ( !c->index ) {
						sql_create_index( db, t, c );
					}

#ifdef DEVELOPER
					if ( !c->index ) {
						Com_Error( ERR_FATAL, "index needed for column '%s' on table '%s'.\n\n%s", c->name, t->name, CURRENT_STMT );
					}
					if ( c->format != INTEGER ) {
						Com_Error( ERR_FATAL, "expecting column '%s' on table '%s' to be integer, not string.\n\n%s", c->name, t->name, CURRENT_STMT );
					}
#endif

					if ( t->last_changed != t->last_indexed )
						sql_table_reindex( t );

					index = search_index_i( c->index, t->row_count, t->rows, t->column_count, c->num, k );

					LVALUE.i = (index>=0)?c->index[ index ]:index; sp--;

				} break;

			case OP_ACCESS_ROW_I:
				{
					tableInfo_t *	t = stack[ sp-3 ].p;
					int				r = stack[ sp-2 ].i;
					columnInfo_t *	c = find_column( t, stack[ sp-1 ].s );

#ifdef DEVELOPER
					if ( !t ) {
						Com_Error( ERR_FATAL, "table '%s' does not exist.\n\n%s", stack[sp-3].s, CURRENT_STMT );
					}
					if ( !c ) {
						Com_Error( ERR_FATAL, "could not find column '%s' on table '%s' in statement:\n\n%s", stack[ sp-1 ].s, stack[sp-3].s, CURRENT_STMT );
					}
#endif

					sp -= 3;
					if ( r < 0 ) {
						stack[ sp++ ].i = -1;

					} else {

						int cell = (t->column_count*r) + c->num;

						if ( c->format == STRING ) {
							stack[ sp++ ].i = atoi( t->rows[ cell ].string );
						} else {
							stack[ sp++ ].i = t->rows[ cell ].integer;
						}
					}

				} break;

			case OP_LOOKUP_S:
				{
					tableInfo_t	*	t = stack[ sp-3 ].p;
					columnInfo_t *	c = stack[ sp-2 ].p;
					cellInfo_t		k;
					int				index;

					k.string = stack[ sp-1 ].s;

					if ( !c->index ) {
						sql_create_index( db, t, c );
					}

#ifdef DEVELOPER
					if ( !c->index ) {
						Com_Error( ERR_FATAL, "index needed for column '%s' on table '%s'.\n\n%s", c->name, t->name, CURRENT_STMT );
					}
					if ( c->format != STRING ) {
						Com_Error( ERR_FATAL, "expecting column '%s' on table '%s' to be string, not integer.\n\n%s", c->name, t->name, CURRENT_STMT );
					}
#endif

					if ( t->last_changed != t->last_indexed )
						sql_table_reindex( t );

					index = search_index_s( c->index, t->row_count, t->rows, t->column_count, c->num, k );

					LVALUE.i = (index>=0)?c->index[ index ]:index; sp--;

				} break;

			case OP_ACCESS_ROW_S:
				{
					tableInfo_t *	t = stack[ sp-3 ].p;
					int				r = stack[ sp-2 ].i;
					columnInfo_t *	c = find_column( t, stack[ sp-1 ].s );

#ifdef DEVELOPER
					if ( !t ) {
						Com_Error( ERR_FATAL, "table does not exist.\n\n%s", CURRENT_STMT );
					}
					if ( !c ) {
						Com_Error( ERR_FATAL, "invalid column for table '%s' in statement:\n\n%s", t->name, CURRENT_STMT );
					}
#endif

					sp -= 3;
					stack[ sp++ ].s = (r>=0)?t->rows[ (t->column_count*r) + c->num ].string:"???";
		
				} break;

			case OP_PUSH_GS:
				{
					int		offset	= READ_INT;

					stack[ sp++ ].i = db->gs[ offset ];
			
				} break;

			case OP_PUSH_GS_OFFSET:
				{
					int		offset	= READ_INT;

					stack[ sp-1 ].i = db->gs[ offset + stack[ sp-1 ].i ];
				} break;

			case OP_PUSH_PS_CLIENT:
				{
					int		offset	= READ_INT;
					stack[ sp++ ].i = db->ps[ offset ];
				} break;

			case OP_PUSH_PS_CLIENT_OFFSET:
				{
					int		offset	= READ_INT;

					stack[ sp-1 ].i = db->ps[ offset + stack[ sp-1 ].i ];
				} break;


			case OP_IFTHENELSE:
				{
					int		c	= stack[ sp-1 ].i;
					value_t	a	= stack[ sp-2 ];
					value_t	b	= stack[ sp-3 ];
					sp -= 3;

					stack[ sp++ ] = (c)?b:a;

				} break;

			case OP_SHADER:
				{
					ASSERT( db->shader );
					stack[ sp-1 ].i = db->shader( stack[ sp-1 ].s );
				} break;

			case OP_SOUND:
				{
					ASSERT( db->sound );
					stack[ sp-1 ].i = db->sound( stack[ sp-1 ].s );
				} break;

			case OP_MODEL:
				{
					ASSERT( db->model );
					stack[ sp-1 ].i = db->model( stack[ sp-1 ].s );
				} break;

			case OP_PORTRAIT:
				{
					ASSERT( db->portrait );
					stack[ sp-1 ].i = db->portrait( stack[ sp-1 ].s );
				} break;

			case OP_PUSH_INTEGER_GLOBAL:
				{
					const char * global_id = READ_STRING;
					ASSERT( db->global_int );
					stack[ sp++ ].i = db->global_int( global_id );
				} break;

				//	recursive integer call
			case OP_EVAL:
				{
					const char * s = stack[ sp-1 ].s;
					int r;

					switch ( SWITCHSTRING(s) )
					{
					case 0:
					case CS('0',0,0,0):
						r = 0;
						break;
					case CS('1',0,0,0):
						r = 1;
						break;
					default:
						{
							Expr		e;
							parseInfo_t	pi = { 0 };
							char		tmp[ SQL_STMT_ALLOC ];
							sqlStack_t*	save = db->stmt_buffer.c;

							db->stmt_buffer.c = (sqlStack_t*)tmp;
							db->stmt_buffer.c->top = sizeof(sqlStack_t);

								pi.db = db;
								e = parse_expression( &s, &pi );

								ASSERT( pi.rt == INTEGER );
								ASSERT( pi.more == 0 );
		
								r = sql_eval( db, e, table, row, index, total, params, aggregate ).integer;

							db->stmt_buffer.c = save;
						} break;
					}

					stack[ sp-1 ].i = r;
				} break;

				//	recursive string call
			case OP_PRINT:
				{
					const char * s = stack[ sp-1 ].s;
					Expr		e;
					parseInfo_t	pi = { 0 };
					char		tmp[ SQL_STMT_ALLOC ];
					sqlStack_t*	save = db->stmt_buffer.c;

					db->stmt_buffer.c = (sqlStack_t*)tmp;
					db->stmt_buffer.c->top = sizeof(sqlStack_t);

					pi.db		= db;
					pi.flags	= PARSE_STRINGLITERAL;

						e = parse_expression( &s, &pi );

						ASSERT( pi.rt == STRING );
						ASSERT( pi.more == 0 );
						
						stack[ sp-1 ].s = sql_eval( db, e, table, row, index, total, params, aggregate ).string;

					db->stmt_buffer.c = save;

				} break;

				//	execute a precompiled expression, returns string
			case OP_RUN:
				{
					int		index = stack[ sp-1 ].i;

					if ( index < 0 || index >= db->stmts_byindex_count || !db->stmts_byindex[ index ] ) {
						stack[ sp-1 ].s = "???";
						break;
					}

					stack[ sp-1 ].s = sql_eval( db, ((formatInfo_t*)db->stmts_byindex[ index ])->print, 0, 0, 0, 0, 0, 0 ).string;
					size += strlen(stack[ sp-1 ].s) + 1;

				} break;

			case OP_RND:
				{
					LVALUE.i = Rand_NextInt32InRange( &db->rand, LEFT_OPERAND.i, RIGHT_OPERAND.i ); sp--;
				} break;
#if DEVELOPER
			default:
				{
					Com_Error(ERR_FATAL, "invalid sql op code: '%d'.\n", op );

				} break;
#endif
		}

#ifdef DEVELOPER
		db->ops++;
#endif
	}

	ASSERT( size <= sizeof(buffer) );	// stack overflow

	size = top;

	if ( sp == 0 )
	{
		cellInfo_t c;
		c.integer = 0;
		return c;
	}

	ASSERT( sp == 1 );

	return *(cellInfo_t*)stack;
}


int sql_eval_where( sqlInfo_t * db, Expr where_expr, tableInfo_t * table, sqlData_t * params, cellInfo_t ** rows, int limit, int offset )
{
	int	lower = 0;
	int upper = table->row_count;
	int i;
	int row_count = 0;

	for ( i=lower; i<upper && limit; i++ ) {

		cellInfo_t * row = table->rows + table->column_count * i;

		if ( !where_expr || sql_eval( db, where_expr, table, row,row_count,row_count+1, params, 0 ).integer ) {

			if ( offset > 0 ) {
				offset--;
			} else {
				rows[ row_count++ ] = row;
				limit--;
			}
		}
	}

	return row_count;
}

int	sql_eval_search_and_where( sqlInfo_t * db, searchInfo_t * search, Expr where_expr, tableInfo_t * table, sqlData_t * params, cellInfo_t ** rows, int limit, int offset )
{
	int start;
	int end;
	cellInfo_t key1;
	cellInfo_t key2;
	int i;
	int row_count;
	Expr			start_expr	= search->start;
	Expr			end_expr	= search->end;

	if ( table->row_count <= 0 ) {
		return 0;
	}

	if ( search->column >= 0 )
	{
		columnInfo_t *	column	= table->columns + search->column;

		if ( !column->index ) {
			sql_create_index( db, table, column );
		}

#ifdef DEVELOPER
		if ( !column->index ) {
			Com_Error( ERR_FATAL, "column '%s' on table '%s' is expected to have an index.", column->name, table->name );
		}
#endif

		if ( table->last_changed != table->last_indexed )
			sql_table_reindex( table );

		if ( start_expr ) {
			key1	= sql_eval( db, start_expr, table, 0,0,0, params, 0 );
			key2	= (end_expr)?sql_eval( db, end_expr, table, 0,0,0, params, 0 ):key1;

			start	= ((column->format==STRING)?search_index_s_first:search_index_i_first)( column->index, table->row_count, table->rows, table->column_count, column->num, key1 );
			end		= ((column->format==STRING)?search_index_s_last:search_index_i_last)	( column->index, table->row_count, table->rows, table->column_count, column->num, key2 );

		} else {
			start	= 0;
			end		= table->row_count-1;
		}

		if ( !search->descending ) {

			for ( i=start,row_count=0; i<=end && limit; i++ ) {

				cellInfo_t * row = table->rows + table->column_count * column->index[ i ];

				if ( !where_expr || sql_eval( db, where_expr, table, row,0,0, params, 0 ).integer ) {

					if ( offset > 0 ) {
						offset--;
					} else {

						if ( !(search->unique && row_count>0 && rows[ row_count-1 ]->integer == row->integer) ) {
							rows[ row_count++ ] = row;
							limit--;
						}
					}
				}
			}
		} else {

			for ( i=end,row_count=0; i>=start && limit; i-- ) {

				cellInfo_t * row = table->rows + table->column_count * column->index[ i ];

				if ( !where_expr || sql_eval( db, where_expr, table, row,0,0, params, 0 ).integer ) {

					if ( offset > 0 ) {
						offset--;
					} else {

						if ( !(search->unique && row_count>0 && rows[ row_count-1 ]->integer == row->integer) ) {
							rows[ row_count++ ] = row;
							limit--;
						}
					}
				}
			}
		}

		return row_count;
	}

	return sql_eval_where( db, where_expr, table, params, rows, limit, offset );

}

