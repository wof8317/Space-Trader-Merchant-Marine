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
#include "sql_parser.h"


extern void *	ParseAlloc	( void *(*mallocProc)(size_t) );
extern void		Parse		( void *yyp, int yymajor, expr_t * yyminor );
extern void		ParseFree	( void *p, void (*freeProc)(void*) );


#define TK_SPACE	-1
#define TK_COMMENT	-1
#define TK_ILLEGAL	-2
#define TK_REGISTER	-8
#define TK_KEYWORD	-9
#define TK_SEMI		-10
#define TK_QUOTE	-11


int IdChar( char c ) {
	if ((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		(c == '_') )
		return 1;
	return 0;
} 

int token_fromliteral( const char *z, int delim, int *tokenType, int tokenize ) {

	int i;

	if ( tokenize && *z == '{' ) {
		*tokenType = TK_LC;
		return 1;
	}

	*tokenType = TK_STRING;

	for( i=0; z[i]; i++ ){

		if ( z[i] == delim ) {
			
			if  ( z[i+1] == delim )
				i++;
			else
				return i+1;
		}

		if ( tokenize && z[i] == '{' )
			return i;
	}

	
	return i;
}

/*
** Return the length of the token that begins at z[0]. 
** Store the token type in *tokenType before returning.
*/
int token( const char *z, int *tokenType ){

	int i, c;

	switch( *z ){
	case ' ': case '\t': case '\n': case '\f': case '\r': {
		for(i=1; isspace(z[i]); i++){}
		*tokenType = TK_SPACE;
		return i;
			  }
	case '-': {
		if( z[1]=='-' ){
			for(i=2; (c=z[i])!=0 && c!='\n'; i++){}
			*tokenType = TK_COMMENT;
			return i;
		}
		*tokenType = TK_MINUS;
		return 1;
			  }
	case '{':	*tokenType = TK_LC;		return 1;
	case '}':	*tokenType = TK_RC;		return 1;
	case '(':	*tokenType = TK_LP;		return 1;
	case ')':	*tokenType = TK_RP;		return 1;
	case '[':	*tokenType = TK_LB;		return 1;
	case ']':	*tokenType = TK_RB;		return 1;
	case ';':	*tokenType = TK_SEMI;	return 1;
	case '+':	*tokenType = TK_PLUS;	return 1;
	case '*':	*tokenType = TK_STAR;	return 1;
	case '/': {
		if( z[1]!='*' || z[2]==0 ){
			*tokenType = TK_SLASH;
			return 1;
		}
		for(i=3, c=z[2]; (c!='*' || z[i]!='/') && (c=z[i])!=0; i++){}
		if( c ) i++;
		*tokenType = TK_COMMENT;
		return i;
			  }
	//case '%':	*tokenType = TK_REM;	return 1;
	case '=': {
		*tokenType = TK_EQ;
		return 1 + (z[1]=='=');
			  }
	case '<': {
		if( (c=z[1])=='=' ){
			*tokenType = TK_LE;
			return 2;
		}else if( c=='>' ){
			*tokenType = TK_NE;
			return 2;
		}else if( c=='<' ){
			*tokenType = TK_LSHIFT;
			return 2;
		}else{
			*tokenType = TK_LT;
			return 1;
		}
			  }
	case '>': {
		if( (c=z[1])=='=' ){
			*tokenType = TK_GE;
			return 2;
		}else if( c=='>' ){
			*tokenType = TK_RSHIFT;
			return 2;
		}else{
			*tokenType = TK_GT;
			return 1;
		}
			  }
	case '!': {
		if( z[1]=='=' ){
			*tokenType = TK_NE;
			return 2;
		}
		*tokenType = TK_NOT;
		return 1;
			  }
	case '|': {
		if( z[1]!='|' ){
			*tokenType = TK_BITOR;
			return 1;
		}else{
			*tokenType = TK_CONCAT;
			return 2;
		}
			  }
	case ',':	*tokenType = TK_COMMA;		return 1;
	case '&':
		if ( z[1]=='&' ) {
			*tokenType = TK_AND;
			return 2;
		} 
		*tokenType = TK_BITAND;
		return 1;
	case '~':	*tokenType = TK_BITNOT;		return 1;

	case '`':
	case '\'':
	case '"':
		*tokenType = TK_QUOTE;
		return 1;

	case '.':
		*tokenType = TK_COLUMN;
		for(i=1; IdChar(z[i]); i++){}
		return i;
	case '^':
		*tokenType = TK_COLUMN_S;
		for(i=1; IdChar(z[i]); i++){}
		return i;
	case '#':	*tokenType = TK_HASH;		return 1;
	case '@':	*tokenType = TK_AT;			return 1;

	case 'G':
		if ( z[1] == 'S' && z[2] == '_' ) {
			for(i=3; IdChar(z[i]); i++){}
			*tokenType = TK_GS;
		} else {
			for(i=1; IdChar(z[i]); i++){}
			*tokenType = TK_KEYWORD;
		}
		return i;

	case 'P':
		if ( z[1] == 'S' && z[2] == '_' ) {
			for(i=3; IdChar(z[i]); i++){}
			*tokenType = TK_PS;
		} else {
			for(i=1; IdChar(z[i]); i++){}
			*tokenType = TK_KEYWORD;
		}
		return i;

	case 'T':
		if ( z[1] == '_' && z[2] ) {
			for(i=2; IdChar(z[i]); i++){}
			*tokenType = TK_T;
		} else {
			for(i=1; IdChar(z[i]); i++){}
			*tokenType = TK_KEYWORD;
		}
		return i;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		*tokenType = TK_INTEGER;
		for(i=0; isdigit(z[i]); i++){}
		return i;

	case '?':
		*tokenType = TK_INTEGER_PARAM;
		for(i=1; isdigit(z[i]); i++){}
		return i;

	case '$':
		*tokenType = TK_STRING_PARAM;
		for(i=1; isdigit(z[i]); i++){}
		return i;

	case ':':
		for(i=1; isspace(z[i]); i++){}
		for( ; IdChar(z[i]); i++){}
		*tokenType = TK_FORMAT;
		return i;

	case '%':
		{
			for(i=1; IdChar(z[i]); i++){}
			if ( i==1 ) {
				*tokenType = TK_MODULUS;
			} else {
				*tokenType = TK_INTEGER_GLOBAL;
			}
			return i;
		} break;

	default: {
		if( !IdChar(*z) ){
			break;
		}
		for(i=1; IdChar(z[i]); i++){}
		//*tokenType = keywordCode((char*)z, i);
		*tokenType = TK_KEYWORD;
		return i;
			 }
	}
	*tokenType = TK_ILLEGAL;
	return 1;
}


static int emit_int( char * expr, int n, int v ) {

	*((int*)&expr[ n ]) = v;
	return n + 4;
}


static parseInfo_t * g_expr;

static void * parse_malloc( size_t size ) {
	return sql_cache_push( g_expr->db, size );
}

Expr parse_expression( const char ** expression, parseInfo_t * pi )
{
	void * p;
	const char * z = *expression;
	int			brackets = 0;
	int i,n;
	int	done;
	int abort;
	char	buffer[ 2048 ];
	Expr	ret;
	int		in_literal		= pi->flags & PARSE_STRINGLITERAL;
	int		literal_delim	= 0;

	g_expr		= pi;
	g_expr->cache = sql_cache_push( g_expr->db, 0 );

	p = ParseAlloc( parse_malloc );

	pi->n		= 0;
	pi->z		= buffer;
	pi->more	= 0;

	if ( pi->flags&(PARSE_ASSIGN_TO_COLUMN) ) {

		pi->z[pi->n++ ] = OP_PUSH_COLUMN;
		pi->z[pi->n++ ] = pi->column;
	}

	for ( i=0, done=0,abort=0; z[i] && !done; i+=n ) {

 		int tt;

		if ( in_literal ) {
			n = token_fromliteral( z+i, literal_delim, &tt, pi->flags & PARSE_STRINGLITERAL );
			in_literal = 0;

			literal_delim = 0;

			if ( tt == TK_STRING ) {
				Parse( p, tt, expr_s( z+i, n ) );
				continue;
			}

		} else {
			n = token( z+i, &tt );
			if ( tt == TK_QUOTE ) {
				in_literal		= 1;
				literal_delim	= z[i];
				continue;
			}
		}

		switch ( tt ) {

			case TK_COMMA:
				if ( brackets == 0 ) {
					pi->more = 1;
					done = 1;
				} else {
					Parse( p, TK_COMMA, 0 );
				}
				break;

			case TK_SPACE:
				break;

			case TK_RC:
				in_literal = 1;
				Parse( p, tt, 0 );
				break;

			case TK_LP:
				brackets++;
				Parse( p, tt, 0 );
				break;

			case TK_RP:
				if ( brackets == 0 ) {
					done = 1;
				} else {
					brackets--;
					Parse( p, tt, 0 );
				}
				break;

			case TK_INTEGER:
				Parse( p, tt, expr_i( atoi( z+i ) ) );
				break;

			case TK_INTEGER_PARAM:
				{
					expr_t * node = op( 0,0, OP_PUSH_INTEGER_PARAM );
					node->v.i = (n==1) ? (pi->arg)++ : atoi( z+i+1 ) - 1;
					Parse( p, tt, node );
				} break;

			case TK_INTEGER_GLOBAL:
				{
					expr_t * node = op( 0,0, OP_PUSH_INTEGER_GLOBAL );
					node->v.s = z+i+1;
					node->n = n-1;
					Parse( p, tt, node );
				} break;

			case TK_COLUMN:
			case TK_COLUMN_S:
				{
					expr_t * node = op( 0,0, OP_PUSH_STRING );
					node->v.s = z+i+1;
					node->n = n-1;
					Parse( p, tt, node );
				} break;

			case TK_T:
			case TK_GS:
			case TK_PS:
				Parse( p, tt, expr_s( z+i, n ) );
				break;

			case TK_SEMI:
				abort = 1;
				break;

			case TK_STRING_PARAM:
				{
					expr_t * node = op( 0,0, OP_PUSH_STRING_PARAM );
					node->v.i = (n==1) ? (pi->arg)++ : atoi( z+i+1 ) - 1;
					Parse( p, tt, node );
				} break;

			case TK_FORMAT:
				{
					expr_t * node = op( 0,0, OP_FORMAT );

					switch( SWITCHSTRING( z+i+1 ) ) {
						case CS('n','o','r','m'):	node->v.i = FN_NORMAL;		break;
						case CS('d','a','t','e'):	node->v.i = FN_DATE;		break;
						case CS('c','u','r','r'):	node->v.i = FN_CURRENCY;	break;
						case CS('d','i','s','a'):	node->v.i = FN_DISABLED;	break;
						case CS('g','o','o','d'):	node->v.i = FN_GOOD;		break;
						case CS('b','a','d',0):		node->v.i = FN_BAD;			break;
						case CS('t','i','m','e'):	node->v.i = FN_TIME;		break;
						case CS('s','h','o','r'):	node->v.i = FN_SHORT;		break;
						case CS('r','a','n','k'):	node->v.i = FN_RANK;		break;
						case CS('p','l','a','i'):	node->v.i = FN_PLAIN;		break;
						case CS('m','o','n','e'):	node->v.i = FN_CURRENCY|FN_SHORT;	break;
						case CS('c','h','a','n'):	node->v.i = FN_CURRENCY|FN_GOOD; break;
					}

					Parse( p, tt, node );

				} break;

			case TK_KEYWORD:
				{
					const char * temp = z+i;
					char * keyword = parse_temp( &temp );

					//	check for column name
					if ( pi->table ) {
						columnInfo_t * c = find_column( pi->table, keyword );

						if ( c ) {
							expr_t * node = op( 0,0, OP_PUSH_COLUMN_VAL );
							node->v.i = c->num;

							Parse( p, (c->format == INTEGER) ? TK_INTEGER:TK_STRING, node );
							break;
						}
					}

					switch( SWITCHSTRING( keyword ) )
					{
						case CS('c','o','u','n'):	Parse( p, TK_COUNT, 0 );break;
						case CS('m','i','n',0):		Parse( p, TK_MIN, 0 ); break;
						case CS('m','a','x',0):		Parse( p, TK_MAX, 0 );	break;
						case CS('s','u','m',0):		Parse( p, TK_SUM, 0 );	break;
						case CS('a','b','s',0):		Parse( p, TK_ABS, 0 ); break;

						case CS('l','i','k','e'):	Parse( p, TK_LIKE, 0 );	break;
						case CS('m','a','t','c'):	Parse( p, TK_MATCH, 0 ); break;
						case CS('a','n','d',0):		Parse( p, TK_AND, 0 );	break;
						case CS('o','r',0,0):		Parse( p, TK_OR, 0 );	break;
						case CS('r','e','m','o'):	Parse( p, TK_REMOVE, 0 );	break;
						case CS('n','u','l','l'):	Parse( p, TK_INTEGER, expr_i( -1 ) );	break;
						case CS('i','f',0,0):		Parse( p, TK_IF, 0 ); break;
						case CS('t','h','e','n'):	Parse( p, TK_THEN, 0 ); break;
						case CS('e','l','s','e'):	Parse( p, TK_ELSE, 0 ); break;
						case CS('s','h','a','d'):	Parse( p, TK_SHADER, 0 ); break;
						case CS('s','o','u','n'):	Parse( p, TK_SOUND, 0 ); break;
						case CS('m','o','d','e'):	Parse( p, TK_MODEL, 0 ); break;
						case CS('t','o','t','a'):	Parse( p, TK_TOTAL, 0 ); break;
						case CS('e','v','a','l'):	Parse( p, TK_EVAL, 0 ); break;
						case CS('p','r','i','n'):	Parse( p, TK_PRINT, 0 ); break;
						case CS('r','u','n',0):		Parse( p, TK_RUN, 0 ); break;
						case CS('p','o','r','t'):	Parse( p, TK_PORTRAIT, 0 ); break;
						case CS('a','t','o','i'):	Parse( p, TK_ATOI, 0 );	break;
						case CS('r','n','d',0):		Parse( p, TK_RND, 0 ); break;
						case CS('s','y','s','_'):	Parse( p, TK_SYS_TIME, 0 ); break;



						case CS('a','s',0,0):
							i += n;						// skip 'AS'
							i += token( z+i, &tt );		// skip space
							ASSERT( tt == TK_SPACE );
							n = token( z+i, &tt );		// get name

							if ( pi->as ) {

								if ( tt == TK_STRING ) {
									*pi->as = sql_alloc_stringn( g_expr->db, z+i+1, n-2 );
								} else if ( tt == TK_KEYWORD ) {
									*pi->as = sql_alloc_stringn( g_expr->db, z+i, n );
								}
							}
							break;


						case CS('f','r','o','m'):
						case CS('l','i','m','i'):
						case CS('o','f','f','s'):
						case CS('w','h','e','r'):
						case CS('s','e','a','r'):
						case CS('d','e','s','c'):
						case CS('s','o','r','t'):
						case CS('r','a','n','d'):
						case CS('u','n','i','q'):
							abort = 1;
							break;

						default:
							Parse( p, TK_STRING, expr_s( z+i, n ) );
					}
				} break;

			default:
				Parse( p, tt, 0 );
				break;
		}

		if ( abort )
			break;
	}

	Parse( p, 0, 0 );

	*expression = z + i;

	if ( pi->flags&(PARSE_ASSIGN_TO_COLUMN) ) {
		pi->z[ pi->n++ ] = (pi->table->columns[ pi->column ].format==INTEGER)?OP_ASSIGN_INT_TO_COLUMN:OP_ASSIGN_STRING_TO_COLUMN;
	}

	if  (pi->flags&PARSE_ASSIGN_CS) {
		pi->z[ pi->n++ ] = OP_ASSIGN_CS_TO_ROW;
	}

	if ( pi->n <= 0 )
		return 0;

	if ( pi->flags&PARSE_STRINGLITERAL ) {
		pi->z[ pi->n++ ] = OP_COLLAPSE;
	}
	pi->z[ pi->n++ ] = OP_END;

	ret = sql_alloc_stmt( g_expr->db, pi->n );
	Com_Memcpy( ret, pi->z, pi->n );

	sql_cache_pop( g_expr->db, g_expr->cache );
		
	return ret;
}

expr_t * expr_i( int i ) {

	expr_t * A = sql_cache_push( g_expr->db, sizeof( expr_t ) );
	A->op	= OP_PUSH_INTEGER;
	A->v.i	= i;
	return A;
}

expr_t * expr_s( const char * z, int n ) {

	expr_t * A = sql_cache_push( g_expr->db,  sizeof( expr_t ) );
	A->op	= OP_PUSH_STRING;
	A->v.s	= z;
	A->n	= n;
	return A;
}

expr_t * op( expr_t * left, expr_t * right, op_t op ) {

	expr_t * A = sql_cache_push( g_expr->db,  sizeof( expr_t ) );
	A->op		= op;
	A->left		= left;
	A->right	= right;
	return A;
}

#define GE(n) (int)&((globalState_t*)0)->n / sizeof(int)
int gs_i( const char* s ) {

	char tmp[ 64 ];
	parse( &s, tmp, sizeof(tmp), 0 );

	switch( SWITCHSTRING( tmp+3 ) ) {
		case CS('p','l','a','n'):	return GE(PLANET);
		case CS('t','u','r','n'):	return GE(TURN);
		case CS('t','i','m','e'):
			{
				switch( SWITCHSTRING(tmp+7) ) {
					case 0:						return GE(TIME);
					case CS('l','e','f','t'):	return GE(TIMELEFT);
				}
			} break;
		case CS('b','o','s','s'):	return GE(BOSS);
		case CS('m','e','n','u'):	return GE(MENU);
		case CS('c','o','u','n'):	return GE(COUNTDOWN);
	}
#ifdef DEVELOPER
	Com_Error( ERR_FATAL, "Unknown keyword '%s'.\n", tmp );
#endif
	return 0;
}

#define PE(n) (int)&((playerState_t*)0)->n / sizeof(int)
int ps_i( const char * s ) {

	char tmp[ 64 ];
	parse( &s, tmp, sizeof(tmp), 0 );

	switch ( SWITCHSTRING( tmp+3 ) )
	{
	case CS('p','e','r','s'):		return PE(persistant);
	case CS('s','t','a','t'):		return PE(stats);
	case CS('a','m','m','o'):		return PE(ammo);
	case CS('w','e','a','p'):		return PE(weapon);
	case CS('s','h','i','e'):		return PE(shields);
	case CS('c','l','i','e'):		return PE(clientNum);
	}

#ifdef DEVELOPER
	Com_Error( ERR_FATAL, "Unknown keyword '%s'.\n", tmp );
#endif
	return 0;
}

//	omg, look up strings.name[$].index by hand and store it
int local( const char * s, int n ) {

	char tmp[ 64 ];
	sqlInfo_t *		db	= sql_getclientdb();
	tableInfo_t *	t	= find_table( db, "strings" );
	columnInfo_t *	c	= find_column( t, "name" );
	cellInfo_t		k;
	int				index;

	if ( !c->index ) sql_create_index( db, t, c );
	if ( t->last_changed != t->last_indexed ) sql_table_reindex( t );

	Q_strncpyz( tmp, s, n+1 );
	k.string = tmp;

	index = search_index_s( c->index, t->row_count, t->rows, t->column_count, c->num, k );
	if ( index >= 0 ) {

		int row = c->index[ index ];
		columnInfo_t *	col	= find_column( t, "index" );
		return t->rows[ (t->column_count*row) + col->num ].integer;
	}

	return 0;
}


int emit( expr_t * A, char * z, int n ) {

	if ( A->left ) {
		n = emit( A->left, z, n );
	}

	if ( A->right ) {
		n = emit( A->right, z, n );
	}

	if ( A->op != OP_NOP )
		z[ n++ ] = A->op;

	switch( A->op ) {
		case OP_PUSH_GS:
		case OP_PUSH_GS_OFFSET:
		case OP_PUSH_PS_CLIENT:
		case OP_PUSH_PS_CLIENT_OFFSET:
		case OP_PUSH_INTEGER:
		case OP_FORMAT:
			n = emit_int( z, n, A->v.i );
			break;

		case OP_PUSH_INTEGER_GLOBAL:
		case OP_PUSH_STRING:
			{
				const char *	s = A->v.s;
				int				c = A->n;
				int				i = 0;


				if ( (s[ 0 ] == '\"' || s[ 0 ] == '\'') && s[ 0 ] != s[ 1 ] ) {
					i++;
				}

				if ( (s[c-1] == '\"' || s[c-1] == '\'') && s[c-1] != s[c-2] ) {
					c--;
				}

				while ( i<c ) {

					//	strip quotes
					if ( (s[ i ] == '\"' || s[ i ] == '\'') && s[ i ] == s[ i+1 ] ) {
						i++;
					}

					z[ n++ ] = s[ i++ ];
				}
				z[ n++ ] = '\0';

			} break;

		case OP_PUSH_STRING_PARAM:
			if ( g_expr->params ) {
				g_expr->params[ A->v.i ].format = STRING;
			}
			z[ n++ ] = (char)A->v.i;
			break;

		case OP_PUSH_INTEGER_PARAM:
			if ( g_expr->params ) {
				g_expr->params[ A->v.i ].format = INTEGER;
			}
			z[ n++ ] = (char)A->v.i;
			break;

		case OP_PUSH_COLUMN_VAL:
		case OP_REMOVE:
			z[ n++ ] = (char)A->v.i;
			break;
	}

	return n;
}


void compile( expr_t * A, dataFormat_t rt ) {
	g_expr->n	= (A)?emit( A, g_expr->z, g_expr->n ):0;
	g_expr->rt	= rt;
}

void sql_error( const char * msg ) {
#ifdef DEVELOPER
	Com_Printf( "SQL SYNTAX ERROR:\n%s\n", CURRENT_STMT );
#endif
}

