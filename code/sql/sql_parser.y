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



%token_prefix TK_

%syntax_error {
	sql_error( "sql : syntax error" );
}
%stack_overflow {
	sql_error( "sql : stack overflow" );
}

%token_type {expr_t*}  
   
%include {
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sql.h"
}



%left LC RC.
%left FORMAT.
%left OR.
%left AND.
%right NOT.
%left LIKE MATCH NE EQ REMOVE.
%left GT LE LT GE.
%right ESCAPE.
%left BITAND BITOR LSHIFT RSHIFT.
%left PLUS MINUS.
%left STAR SLASH MODULUS REM.
%left CONCAT IF THEN ELSE.
%right UMINUS UPLUS BITNOT.


// this allows for empty expressions without triggering a syntax error
stmt ::= .

//
//	INTEGER expressions
//

stmt ::= expr_i(A). { compile( A, INTEGER ); }

stmt ::= STAR. { compile( 0, ROW ); }

//
//	if condition
//
//if (A) THEN (B) ELSE (c)
expr_i(A)	::= IF expr_i(B) THEN expr_i(C) ELSE expr_i(D).		{ A = op( op( C,D, OP_NOP ), B, OP_IFTHENELSE ); }
expr_s(A)	::= IF expr_i(B) THEN expr_s(C) ELSE expr_s(D).		{ A = op( op( C,D, OP_NOP ), B, OP_IFTHENELSE ); }

expr_i(A) ::= LP expr_i(B) RP.		{ A = B; }
   
expr_i(A)	::= expr_i(B)	MINUS	expr_i(C).			{ A = op( B,C, OP_SUBTRACT	); }  
expr_i(A)	::= expr_i(B)	PLUS	expr_i(C).			{ A = op( B,C, OP_ADD		); }
expr_i(A)	::= expr_i(B)	STAR	expr_i(C).			{ A = op( B,C, OP_MULTIPLY	); }
expr_i(A)	::= expr_i(B)	MODULUS	expr_i(C).			{ A = op( B,C, OP_MODULUS	); }
expr_i(A)	::= expr_i(B)	SLASH	expr_i(C).			{ A = op( B,C, OP_DIVIDE	); }
expr_i(A)	::= expr_i(B)	EQ		expr_i(C).			{ A = op( B,C, OP_EQ		); }
expr_i(A)	::= expr_i(B)	LE		expr_i(C).			{ A = op( B,C, OP_LE		); }
expr_i(A)	::= expr_i(B)	NE		expr_i(C).			{ A = op( B,C, OP_NE		); }
expr_i(A)	::= expr_i(B)	LSHIFT	expr_i(C).			{ A = op( B,C, OP_LSHIFT	); }
expr_i(A)	::= expr_i(B)	LT		expr_i(C).			{ A = op( B,C, OP_LT		); }
expr_i(A)	::= expr_i(B)	GE		expr_i(C).			{ A = op( B,C, OP_GE		); }
expr_i(A)	::= expr_i(B)	RSHIFT	expr_i(C).			{ A = op( B,C, OP_RSHIFT	); }
expr_i(A)	::= expr_i(B)	GT		expr_i(C).			{ A = op( B,C, OP_GT		); }
expr_i(A)	::= expr_i(B)	AND		expr_i(C).			{ A = op( B,C, OP_LOGICAL_AND ); }
expr_i(A)	::= expr_i(B)	OR		expr_i(C).			{ A = op( B,C, OP_LOGICAL_OR ); }
expr_i(A)	::=				NOT		expr_i(C).			{ A = op( 0,C, OP_NOT		); }
expr_i(A)	::= expr_i(B)	BITAND	expr_i(C).			{ A = op( B,C, OP_BITWISE_AND ); }
expr_i(A)	::= expr_i(B)	BITOR	expr_i(C).			{ A = op( B,C, OP_BITWISE_OR ); }
expr_i(A)	::=				MINUS	expr_i(C). [UMINUS] { A = op( 0,C, OP_UMINUS	); }
expr_i(A)	::=				PLUS	expr_i(C). [UPLUS]	{ A = C; }
expr_i(A)	::= INTEGER_PARAM(B) REMOVE expr_i(C).		{ A = B; B->right = C; B->op = OP_REMOVE; }

expr_i(A)	::= HASH.				{ A = op( 0,0, OP_ROWINDEX ); }		// index to the table
expr_i(A)	::= AT.					{ A = op( 0,0, OP_ROWNUMBER ); }	// index within the current statement
expr_i(A)	::= TOTAL.				{ A = op( 0,0, OP_ROWTOTAL ); }		// number of rows within the select
expr_i(A)	::= COUNT.				{ A = op( 0,0, OP_ROWCOUNT ); }		// number of rows in the table

expr_i(A)	::= MIN LP expr_i(B) COMMA expr_i(C) RP.		{ A = op( B,C, OP_INT_MIN ); }
expr_i(A)	::= MAX LP expr_i(B) COMMA expr_i(C) RP.		{ A = op( B,C, OP_INT_MAX ); }
expr_i(A)	::= ABS LP expr_i(C) RP.						{ A = op( 0,C, OP_ABS ); }

expr_i(A)	::= EVAL LP expr_s(B) RP.	{ A = op( B, 0, OP_EVAL ); }

expr_i(A)	::= INTEGER(B).			{ A = B; }
expr_i(A)	::= INTEGER_PARAM(B).	{ A = B; }

expr_i(A)	::= INTEGER_GLOBAL(B).	{ A = B; }
//
//	STRING expressions
//

stmt ::= expr_s(A). { compile( A, STRING ); }

expr_s(A)	::= LP		expr_s(B)		RP.					{ A = B; }

expr_s(A)	::= expr_i(B) FORMAT(C).	{ A = C; A->right = B; }

expr_i(A)	::= ATOI LP expr_s(B) RP.						{ A = op( B,0, OP_ATOI		); }
expr_i(A)	::= expr_s(B)	LIKE	expr_s(C).				{ A = op( B,C, OP_LIKE		); }
expr_i(A)	::= expr_s(B)	EQ		expr_s(C).				{ A = op( B,C, OP_LIKE		); }
expr_i(A)	::= expr_s(B)	MATCH	expr_s(C).				{ A = op( B,C, OP_MATCH		); }
expr_i(A)	::= expr_s(B)	NE		expr_s(C).				{ A = op( B,C, OP_NOTLIKE	); }
expr_s(A)	::= expr_s(B)	CONCAT	expr_s(C).				{ A = op( B,C, OP_CONCAT	); }

expr_s(A)	::= STRING(B).									{ A = B; }
expr_s(A)	::= STRING_PARAM(B).							{ A = B; }

expr_s(A)	::= PRINT LP expr_s(B) RP.						{ A = op( B, 0, OP_PRINT ); }
expr_s(A)	::= RUN LP expr_i(B) RP.						{ A = op( B, 0, OP_RUN ); }


//
//	inline string expressions '{ }'
//
expr_s(A)	::= LC expr_s(B).								{ A = B; }
expr_s(A)	::= expr_s(B)	RC.								{ A = B; }
expr_s(A)	::= STRING(B)	LC		expr_s(C).				{ A = op( B,C, OP_NOP		); }
expr_s(A)	::= expr_s(B)	RC		expr_s(C).				{ A = op( B,C, OP_NOP		); }

//
//	AGGREGATES
//
stmt ::= expr_a(A). { compile( A, AGGREGATE ); }

expr_a(A)	::= COUNT LP STAR RP.		{ A = op( 0,0, OP_COUNT ); }
expr_a(A)	::= MIN LP expr_i(C) RP.	{ A = op( 0,C, OP_MIN	); }
expr_a(A)	::= MAX LP expr_i(C) RP.	{ A = op( 0,C, OP_MAX	); }
expr_a(A)	::= SUM LP expr_i(C) RP.	{ A = op( 0,C, OP_SUM	); }


//
//	TABLE LOOK UPS
//
//	integer	=	<table>.<column>[ <search> ].<column>
//	string = ( <table>.<column> : <value> )^<column>
//
lookup(A)	::= STRING(B) COLUMN(C).					{ A = op( B,C, OP_ACCESS_TABLE ); }
search(A)	::= lookup(B) LB expr_i(C) RB.				{ A = op( B,C, OP_LOOKUP_I ); }
search(A)	::= lookup(B) LB expr_s(C) RB.				{ A = op( B,C, OP_LOOKUP_S ); }

expr_i(A)	::= search(B) COLUMN(C).						{ A = op( B,C, OP_ACCESS_ROW_I ); }
expr_s(A)	::= search(B) COLUMN_S(C).						{ A = op( B,C, OP_ACCESS_ROW_S ); }


//
//	GS_ ACCESS
//
expr_i(A)	::= GS(B).							{ A = B; B->op = OP_PUSH_GS; B->v.i = gs_i( B->v.s ); }
expr_i(A)	::= GS(B) LP expr_i(C) RP.			
	{
		B->op	= OP_PUSH_GS_OFFSET;
		B->v.i	= gs_i( B->v.s );
		B->left	= C;
		B->right= 0;
		A = B;
	}
	
//
//	cvar access
//

expr_s(A)	::= LT expr_s(B) GT.				{ A = op( B,0, OP_CVAR ); }
	
//
//	playerState_t	Read Only Access	PS_
//
expr_i(A)	::= PS(B).							{ A = B; B->op = OP_PUSH_PS_CLIENT; B->v.i = ps_i( B->v.s ); }
expr_i(A)	::= PS(B) LP expr_i(C) RP.
	{
		B->op	= OP_PUSH_PS_CLIENT_OFFSET;
		B->v.i	= ps_i( B->v.s );
		B->left	= C;
		B->right= 0;
		A = B;
	}

//
//	localization funtion	T_<string>
//
expr_s(A)	::= T(B).							{ A = op( B,0, OP_RUN ); B->op = OP_PUSH_INTEGER; B->v.i = local( B->v.s, B->n ); }

//
//	functions
//
expr_i(A)	::= SHADER LP expr_s(B) RP.					{ A = op( B,0, OP_SHADER ); }
expr_i(A)	::= SOUND LP expr_s(B) RP.					{ A = op( B,0, OP_SOUND ); }
expr_i(A)	::= PORTRAIT LP expr_s(B) RP.				{ A = op( B,0, OP_PORTRAIT ); }
expr_i(A)	::= MODEL LP expr_s(B) RP.					{ A = op( B,0, OP_MODEL ); }
expr_i(A)	::= RND LP expr_i(B) COMMA expr_i(C) RP.	{ A = op( B,C, OP_RND ); }
expr_i(A)	::= SYS_TIME.								{ A = op( 0,0, OP_SYS_TIME ); }
