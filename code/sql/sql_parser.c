/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is include which follows the "include" declaration
** in the input file. */
#include <stdio.h>
#line 36 "c:\\st-gpl\\code\\sql\\sql_parser.y"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sql.h"
#line 14 "c:\\st-gpl\\code\\sql\\sql_parser.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    ParseTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 75
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE expr_t*
typedef union {
  ParseTOKENTYPE yy0;
  int yy149;
} YYMINORTYPE;
#define YYSTACKDEPTH 100
#define ParseARG_SDECL
#define ParseARG_PDECL
#define ParseARG_FETCH
#define ParseARG_STORE
#define YYNSTATE 182
#define YYNRULE 77
#define YYERRORSYMBOL 67
#define YYERRSYMDT yy149
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* Next are that tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */    52,  158,  115,   49,  156,   44,  116,  167,    4,   14,
 /*    10 */    28,   11,   50,   43,  260,   74,   97,  177,  118,  112,
 /*    20 */    48,   46,  163,   13,   17,   15,   51,    3,    1,   47,
 /*    30 */     8,   22,   20,   10,  153,  117,  169,  168,  149,  114,
 /*    40 */   122,   30,  135,  121,  125,  160,  159,  129,  113,  143,
 /*    50 */   124,  120,  132,    4,   14,   28,   11,  119,  126,  139,
 /*    60 */   131,  134,  130,  127,  123,  153,   52,   53,   55,   49,
 /*    70 */    56,   44,   42,    1,    4,   14,   28,   11,   32,   43,
 /*    80 */     9,    2,   13,   17,   15,   24,   48,   46,   75,  105,
 /*    90 */     6,  118,  112,    3,    1,   34,   38,   12,   45,   10,
 /*   100 */   153,  117,  169,  168,  149,  141,  133,   36,  128,  121,
 /*   110 */   125,  160,  159,  129,  113,  143,  124,  120,   60,   98,
 /*   120 */   154,  118,  112,  119,  126,  139,  131,  134,  130,  127,
 /*   130 */   123,  153,  147,   37,   35,  153,   16,   54,   23,   19,
 /*   140 */   153,   33,   21,   27,   29,  153,   39,   41,   25,   31,
 /*   150 */     9,    2,   13,   17,   15,  153,   73,   98,  153,  118,
 /*   160 */   112,  153,  104,   98,  171,  118,  112,  153,  147,   37,
 /*   170 */    35,   40,  153,  153,   23,   19,  153,   33,   21,   27,
 /*   180 */    29,  153,   39,   41,   25,   31,    9,    2,   13,   17,
 /*   190 */    15,   75,   88,  153,  118,  112,  153,  153,  153,  153,
 /*   200 */   175,  153,  153,  153,  147,   37,   35,   18,  153,  153,
 /*   210 */    23,   19,  153,   33,   21,   27,   29,  153,   39,   41,
 /*   220 */    25,   31,    9,    2,   13,   17,   15,  153,  147,   37,
 /*   230 */    35,  153,  153,  153,   23,   19,  145,   33,   21,   27,
 /*   240 */    29,  153,   39,   41,   25,   31,    9,    2,   13,   17,
 /*   250 */    15,  153,  147,   37,   35,  153,  153,  153,   23,   19,
 /*   260 */   151,   33,   21,   27,   29,  153,   39,   41,   25,   31,
 /*   270 */     9,    2,   13,   17,   15,  147,   37,   35,  153,  153,
 /*   280 */   153,   23,   19,  153,   33,   21,   27,   29,  153,   39,
 /*   290 */    41,   25,   31,    9,    2,   13,   17,   15,   61,   98,
 /*   300 */   153,  118,  112,  153,  153,  165,  153,  181,  153,  147,
 /*   310 */    37,   35,  153,  153,  153,   23,   19,  153,   33,   21,
 /*   320 */    27,   29,  153,   39,   41,   25,   31,    9,    2,   13,
 /*   330 */    17,   15,  153,   49,  153,  147,   37,   35,    4,   14,
 /*   340 */   153,   23,   19,  153,   33,   21,   27,   29,   18,   39,
 /*   350 */    41,   25,   31,    9,    2,   13,   17,   15,    1,  147,
 /*   360 */    37,   35,  153,  153,  153,   23,   19,  178,   33,   21,
 /*   370 */    27,   29,  153,   39,   41,   25,   31,    9,    2,   13,
 /*   380 */    17,   15,  153,  147,   37,   35,  153,  153,  153,   23,
 /*   390 */    19,  152,   33,   21,   27,   29,  153,   39,   41,   25,
 /*   400 */    31,    9,    2,   13,   17,   15,  147,   37,   35,    5,
 /*   410 */   153,  153,   23,   19,  153,   33,   21,   27,   29,  153,
 /*   420 */    39,   41,   25,   31,    9,    2,   13,   17,   15,  153,
 /*   430 */   153,  153,  147,   37,   35,  153,  153,  153,   23,   19,
 /*   440 */   153,   33,   21,   27,   29,   26,   39,   41,   25,   31,
 /*   450 */     9,    2,   13,   17,   15,  147,   37,   35,  153,    7,
 /*   460 */   153,   23,   19,  153,   33,   21,   27,   29,  153,   39,
 /*   470 */    41,   25,   31,    9,    2,   13,   17,   15,  153,  147,
 /*   480 */    37,   35,  153,  153,  153,   23,   19,  148,   33,   21,
 /*   490 */    27,   29,  153,   39,   41,   25,   31,    9,    2,   13,
 /*   500 */    17,   15,  153,  147,   37,   35,  153,  153,  153,   23,
 /*   510 */    19,  179,   33,   21,   27,   29,  153,   39,   41,   25,
 /*   520 */    31,    9,    2,   13,   17,   15,  153,  147,   37,   35,
 /*   530 */   153,  153,  153,   23,   19,  137,   33,   21,   27,   29,
 /*   540 */   153,   39,   41,   25,   31,    9,    2,   13,   17,   15,
 /*   550 */   153,  153,  153,  147,   37,   35,  153,  153,  153,   23,
 /*   560 */    19,  153,   33,   21,   27,   29,   40,   39,   41,   25,
 /*   570 */    31,    9,    2,   13,   17,   15,  153,  147,   37,   35,
 /*   580 */   153,  153,  153,   23,   19,  140,   33,   21,   27,   29,
 /*   590 */   153,   39,   41,   25,   31,    9,    2,   13,   17,   15,
 /*   600 */    35,  153,  153,  153,   23,   19,  153,   33,   21,   27,
 /*   610 */    29,  153,   39,   41,   25,   31,    9,    2,   13,   17,
 /*   620 */    15,  153,   23,   19,  153,   33,   21,   27,   29,  153,
 /*   630 */    39,   41,   25,   31,    9,    2,   13,   17,   15,   33,
 /*   640 */    21,   27,   29,  153,   39,   41,   25,   31,    9,    2,
 /*   650 */    13,   17,   15,   39,   41,   25,   31,    9,    2,   13,
 /*   660 */    17,   15,   49,  153,  153,  153,  153,    4,   14,   28,
 /*   670 */    11,   49,  153,  153,  153,  153,    4,   14,   28,   11,
 /*   680 */    75,   95,  153,  118,  112,  103,   98,    1,  118,  112,
 /*   690 */    75,  102,  153,  118,  112,  146,    1,   75,   93,  153,
 /*   700 */   118,  112,   75,  136,  164,  118,  112,   49,  153,  153,
 /*   710 */   153,  153,    4,   14,   28,   11,   49,  153,  153,  153,
 /*   720 */   153,    4,   14,   28,   11,   76,   98,  153,  118,  112,
 /*   730 */    77,   98,    1,  118,  112,  153,  153,  153,  153,  153,
 /*   740 */   142,    1,  153,  153,  153,  153,  153,   75,   91,  150,
 /*   750 */   118,  112,   49,  153,  153,  153,  153,    4,   14,   28,
 /*   760 */    11,   49,  153,  153,  153,  153,    4,   14,   28,   11,
 /*   770 */   111,   98,  153,  118,  112,   78,   98,    1,  118,  112,
 /*   780 */   153,  153,  153,  153,  153,  138,    1,  153,  153,  153,
 /*   790 */   153,  153,   81,   98,  157,  118,  112,   49,  153,  153,
 /*   800 */   153,  153,    4,   14,   28,   11,   49,  153,  153,  153,
 /*   810 */   153,    4,   14,   28,   11,  153,  153,   75,   89,  153,
 /*   820 */   118,  112,    1,  153,  153,  153,  153,  153,  153,  153,
 /*   830 */   172,    1,  153,  153,  153,  153,  153,   68,   98,  176,
 /*   840 */   118,  112,   49,  153,  153,  153,  153,    4,   14,   28,
 /*   850 */    11,   49,  144,  153,  153,  153,    4,   14,   28,   11,
 /*   860 */   153,  153,   83,   98,  153,  118,  112,    1,  153,  153,
 /*   870 */   153,  153,  153,  153,   59,   98,    1,  118,  112,  153,
 /*   880 */   153,   75,   87,  153,  118,  112,  153,   75,  108,  153,
 /*   890 */   118,  112,  100,   98,  153,  118,  112,  153,  153,  153,
 /*   900 */    66,   86,  153,  118,  112,  162,  153,   63,   98,  153,
 /*   910 */   118,  112,   72,   94,  153,  118,  112,   84,   98,  153,
 /*   920 */   118,  112,  155,   98,  153,  118,  112,   75,  109,  153,
 /*   930 */   118,  112,   75,   90,  153,  118,  112,   85,   98,  153,
 /*   940 */   118,  112,  153,  153,   75,   99,  153,  118,  112,  153,
 /*   950 */   153,   64,   98,  153,  118,  112,  153,  180,   98,  153,
 /*   960 */   118,  112,  101,   98,  153,  118,  112,  153,  153,  153,
 /*   970 */    62,   98,  153,  118,  112,   65,   98,  153,  118,  112,
 /*   980 */   166,   98,  153,  118,  112,   79,   98,  153,  118,  112,
 /*   990 */    71,   98,  153,  118,  112,   75,  161,  153,  118,  112,
 /*  1000 */    75,   96,  153,  118,  112,   82,   98,  153,  118,  112,
 /*  1010 */   110,   98,  153,  118,  112,   57,   98,  153,  118,  112,
 /*  1020 */   153,   80,   98,  153,  118,  112,  153,   69,   92,  153,
 /*  1030 */   118,  112,   67,   98,  153,  118,  112,  153,  153,  153,
 /*  1040 */   170,   98,  153,  118,  112,   75,  107,  153,  118,  112,
 /*  1050 */    58,   98,  153,  118,  112,  173,   98,  153,  118,  112,
 /*  1060 */    70,   98,  153,  118,  112,   75,  106,  153,  118,  112,
 /*  1070 */   174,   98,  153,  118,  112,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     1,   54,   34,    2,   57,    6,   23,   35,    7,    8,
 /*    10 */     9,   10,   55,   14,   68,   69,   70,   71,   72,   73,
 /*    20 */    21,   22,   23,   23,   24,   25,   11,   28,   27,   34,
 /*    30 */    34,   30,   34,   34,   74,   36,   37,   38,   39,   40,
 /*    40 */    41,   34,   43,   44,   45,   46,   47,   48,   49,   50,
 /*    50 */    51,   52,   53,    7,    8,    9,   10,   58,   59,   60,
 /*    60 */    61,   62,   63,   64,   65,   66,    1,    1,   34,    2,
 /*    70 */    34,    6,   34,   27,    7,    8,    9,   10,   34,   14,
 /*    80 */    21,   22,   23,   24,   25,   34,   21,   22,   69,   70,
 /*    90 */    34,   72,   73,   28,   27,   34,   34,   34,   34,   34,
 /*   100 */    74,   36,   37,   38,   39,   40,   41,   34,   43,   44,
 /*   110 */    45,   46,   47,   48,   49,   50,   51,   52,   69,   70,
 /*   120 */    54,   72,   73,   58,   59,   60,   61,   62,   63,   64,
 /*   130 */    65,   66,    3,    4,    5,   74,   34,   34,    9,   10,
 /*   140 */    74,   12,   13,   14,   15,   74,   17,   18,   19,   20,
 /*   150 */    21,   22,   23,   24,   25,   74,   69,   70,   74,   72,
 /*   160 */    73,   74,   69,   70,   35,   72,   73,   74,    3,    4,
 /*   170 */     5,   42,   74,   74,    9,   10,   74,   12,   13,   14,
 /*   180 */    15,   74,   17,   18,   19,   20,   21,   22,   23,   24,
 /*   190 */    25,   69,   70,   74,   72,   73,   74,   74,   74,   74,
 /*   200 */    35,   74,   74,   74,    3,    4,    5,   42,   74,   74,
 /*   210 */     9,   10,   74,   12,   13,   14,   15,   74,   17,   18,
 /*   220 */    19,   20,   21,   22,   23,   24,   25,   74,    3,    4,
 /*   230 */     5,   74,   74,   74,    9,   10,   35,   12,   13,   14,
 /*   240 */    15,   74,   17,   18,   19,   20,   21,   22,   23,   24,
 /*   250 */    25,   74,    3,    4,    5,   74,   74,   74,    9,   10,
 /*   260 */    35,   12,   13,   14,   15,   74,   17,   18,   19,   20,
 /*   270 */    21,   22,   23,   24,   25,    3,    4,    5,   74,   74,
 /*   280 */    74,    9,   10,   74,   12,   13,   14,   15,   74,   17,
 /*   290 */    18,   19,   20,   21,   22,   23,   24,   25,   69,   70,
 /*   300 */    74,   72,   73,   74,   74,   56,   74,   35,   74,    3,
 /*   310 */     4,    5,   74,   74,   74,    9,   10,   74,   12,   13,
 /*   320 */    14,   15,   74,   17,   18,   19,   20,   21,   22,   23,
 /*   330 */    24,   25,   74,    2,   74,    3,    4,    5,    7,    8,
 /*   340 */    74,    9,   10,   74,   12,   13,   14,   15,   42,   17,
 /*   350 */    18,   19,   20,   21,   22,   23,   24,   25,   27,    3,
 /*   360 */     4,    5,   74,   74,   74,    9,   10,   35,   12,   13,
 /*   370 */    14,   15,   74,   17,   18,   19,   20,   21,   22,   23,
 /*   380 */    24,   25,   74,    3,    4,    5,   74,   74,   74,    9,
 /*   390 */    10,   35,   12,   13,   14,   15,   74,   17,   18,   19,
 /*   400 */    20,   21,   22,   23,   24,   25,    3,    4,    5,   29,
 /*   410 */    74,   74,    9,   10,   74,   12,   13,   14,   15,   74,
 /*   420 */    17,   18,   19,   20,   21,   22,   23,   24,   25,   74,
 /*   430 */    74,   74,    3,    4,    5,   74,   74,   74,    9,   10,
 /*   440 */    74,   12,   13,   14,   15,   42,   17,   18,   19,   20,
 /*   450 */    21,   22,   23,   24,   25,    3,    4,    5,   74,   30,
 /*   460 */    74,    9,   10,   74,   12,   13,   14,   15,   74,   17,
 /*   470 */    18,   19,   20,   21,   22,   23,   24,   25,   74,    3,
 /*   480 */     4,    5,   74,   74,   74,    9,   10,   35,   12,   13,
 /*   490 */    14,   15,   74,   17,   18,   19,   20,   21,   22,   23,
 /*   500 */    24,   25,   74,    3,    4,    5,   74,   74,   74,    9,
 /*   510 */    10,   35,   12,   13,   14,   15,   74,   17,   18,   19,
 /*   520 */    20,   21,   22,   23,   24,   25,   74,    3,    4,    5,
 /*   530 */    74,   74,   74,    9,   10,   35,   12,   13,   14,   15,
 /*   540 */    74,   17,   18,   19,   20,   21,   22,   23,   24,   25,
 /*   550 */    74,   74,   74,    3,    4,    5,   74,   74,   74,    9,
 /*   560 */    10,   74,   12,   13,   14,   15,   42,   17,   18,   19,
 /*   570 */    20,   21,   22,   23,   24,   25,   74,    3,    4,    5,
 /*   580 */    74,   74,   74,    9,   10,   35,   12,   13,   14,   15,
 /*   590 */    74,   17,   18,   19,   20,   21,   22,   23,   24,   25,
 /*   600 */     5,   74,   74,   74,    9,   10,   74,   12,   13,   14,
 /*   610 */    15,   74,   17,   18,   19,   20,   21,   22,   23,   24,
 /*   620 */    25,   74,    9,   10,   74,   12,   13,   14,   15,   74,
 /*   630 */    17,   18,   19,   20,   21,   22,   23,   24,   25,   12,
 /*   640 */    13,   14,   15,   74,   17,   18,   19,   20,   21,   22,
 /*   650 */    23,   24,   25,   17,   18,   19,   20,   21,   22,   23,
 /*   660 */    24,   25,    2,   74,   74,   74,   74,    7,    8,    9,
 /*   670 */    10,    2,   74,   74,   74,   74,    7,    8,    9,   10,
 /*   680 */    69,   70,   74,   72,   73,   69,   70,   27,   72,   73,
 /*   690 */    69,   70,   74,   72,   73,   35,   27,   69,   70,   74,
 /*   700 */    72,   73,   69,   70,   35,   72,   73,    2,   74,   74,
 /*   710 */    74,   74,    7,    8,    9,   10,    2,   74,   74,   74,
 /*   720 */    74,    7,    8,    9,   10,   69,   70,   74,   72,   73,
 /*   730 */    69,   70,   27,   72,   73,   74,   74,   74,   74,   74,
 /*   740 */    35,   27,   74,   74,   74,   74,   74,   69,   70,   35,
 /*   750 */    72,   73,    2,   74,   74,   74,   74,    7,    8,    9,
 /*   760 */    10,    2,   74,   74,   74,   74,    7,    8,    9,   10,
 /*   770 */    69,   70,   74,   72,   73,   69,   70,   27,   72,   73,
 /*   780 */    74,   74,   74,   74,   74,   35,   27,   74,   74,   74,
 /*   790 */    74,   74,   69,   70,   35,   72,   73,    2,   74,   74,
 /*   800 */    74,   74,    7,    8,    9,   10,    2,   74,   74,   74,
 /*   810 */    74,    7,    8,    9,   10,   74,   74,   69,   70,   74,
 /*   820 */    72,   73,   27,   74,   74,   74,   74,   74,   74,   74,
 /*   830 */    35,   27,   74,   74,   74,   74,   74,   69,   70,   35,
 /*   840 */    72,   73,    2,   74,   74,   74,   74,    7,    8,    9,
 /*   850 */    10,    2,   12,   74,   74,   74,    7,    8,    9,   10,
 /*   860 */    74,   74,   69,   70,   74,   72,   73,   27,   74,   74,
 /*   870 */    74,   74,   74,   74,   69,   70,   27,   72,   73,   74,
 /*   880 */    74,   69,   70,   74,   72,   73,   74,   69,   70,   74,
 /*   890 */    72,   73,   69,   70,   74,   72,   73,   74,   74,   74,
 /*   900 */    69,   70,   74,   72,   73,   56,   74,   69,   70,   74,
 /*   910 */    72,   73,   69,   70,   74,   72,   73,   69,   70,   74,
 /*   920 */    72,   73,   69,   70,   74,   72,   73,   69,   70,   74,
 /*   930 */    72,   73,   69,   70,   74,   72,   73,   69,   70,   74,
 /*   940 */    72,   73,   74,   74,   69,   70,   74,   72,   73,   74,
 /*   950 */    74,   69,   70,   74,   72,   73,   74,   69,   70,   74,
 /*   960 */    72,   73,   69,   70,   74,   72,   73,   74,   74,   74,
 /*   970 */    69,   70,   74,   72,   73,   69,   70,   74,   72,   73,
 /*   980 */    69,   70,   74,   72,   73,   69,   70,   74,   72,   73,
 /*   990 */    69,   70,   74,   72,   73,   69,   70,   74,   72,   73,
 /*  1000 */    69,   70,   74,   72,   73,   69,   70,   74,   72,   73,
 /*  1010 */    69,   70,   74,   72,   73,   69,   70,   74,   72,   73,
 /*  1020 */    74,   69,   70,   74,   72,   73,   74,   69,   70,   74,
 /*  1030 */    72,   73,   69,   70,   74,   72,   73,   74,   74,   74,
 /*  1040 */    69,   70,   74,   72,   73,   69,   70,   74,   72,   73,
 /*  1050 */    69,   70,   74,   72,   73,   69,   70,   74,   72,   73,
 /*  1060 */    69,   70,   74,   72,   73,   69,   70,   74,   72,   73,
 /*  1070 */    69,   70,   74,   72,   73,
};
#define YY_SHIFT_USE_DFLT (-54)
#define YY_SHIFT_MAX 135
static const short yy_shift_ofst[] = {
 /*     0 */    -1,   65,   65,   65,   65,   65,   65,   65,   65,   65,
 /*    10 */    65,   65,   65,   65,   65,   65,   65,   65,   65,   65,
 /*    20 */    65,   65,   65,   65,   65,   65,   65,   65,   65,   65,
 /*    30 */    65,   65,   65,   65,   65,   65,   65,   65,   65,   65,
 /*    40 */    65,   65,   65,   65,   65,   65,   65,   65,   65,   65,
 /*    50 */    65,   65,   65,   65,   65,   65,   65,  129,  165,  524,
 /*    60 */   550,  500,  452,  403,  356,  306,  249,  201,  380,  476,
 /*    70 */   272,  225,  429,  332,  574,  574,  595,  613,  613,  627,
 /*    80 */   627,  627,  636,  636,  636,  636,  849,  660,  669,  705,
 /*    90 */   714,  750,  759,  795,    1,  804,  840,   67,   67,   46,
 /*   100 */    59,   59,   46,   59,   59,   46,  331,  331,  331,  331,
 /*   110 */     0,    0,  -53,   66,  -32,  -17,  -28,   15,  -43,   -5,
 /*   120 */   103,   -4,   -2,    7,   34,   36,   38,   44,   51,   56,
 /*   130 */    61,   62,   63,   64,   73,  102,
};
#define YY_REDUCE_USE_DFLT (-55)
#define YY_REDUCE_MAX 56
static const short yy_reduce_ofst[] = {
 /*     0 */   -54,  633,  701,  768,  818,  843,  863,  888,  921,  941,
 /*    10 */   958,  976,  991, 1001,  996,  986,  981,  971,  963,  952,
 /*    20 */   946,  936,  926,  916,  906,  893,  882,  868,  858,  848,
 /*    30 */   838,  823,  812,  793,  748,  706,  678,  656,  628,  616,
 /*    40 */   229,   93,   49,  931,  661,  805,  853,  901,  911,  875,
 /*    50 */   831,  723,  621,   19,   87,  122,  611,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   182,  259,  259,  259,  259,  259,  259,  259,  259,  259,
 /*    10 */   259,  259,  259,  259,  259,  259,  259,  259,  259,  259,
 /*    20 */   259,  259,  259,  259,  259,  259,  259,  259,  259,  259,
 /*    30 */   259,  259,  259,  259,  259,  259,  259,  259,  259,  259,
 /*    40 */   259,  259,  259,  259,  259,  259,  259,  259,  259,  234,
 /*    50 */   259,  259,  259,  259,  259,  259,  259,  259,  259,  259,
 /*    60 */   259,  259,  259,  259,  259,  259,  259,  259,  259,  259,
 /*    70 */   259,  259,  259,  259,  183,  259,  202,  203,  201,  195,
 /*    80 */   193,  208,  194,  200,  198,  197,  259,  259,  259,  259,
 /*    90 */   259,  259,  259,  259,  259,  259,  259,  220,  259,  236,
 /*   100 */   199,  196,  233,  204,  205,  235,  226,  225,  224,  227,
 /*   110 */   189,  188,  259,  229,  212,  259,  259,  218,  259,  247,
 /*   120 */   259,  259,  259,  259,  259,  259,  250,  259,  259,  259,
 /*   130 */   259,  259,  259,  259,  259,  259,  228,  213,  254,  252,
 /*   140 */   251,  212,  255,  230,  249,  214,  256,  222,  248,  211,
 /*   150 */   223,  215,  257,  258,  242,  206,  246,  221,  245,  219,
 /*   160 */   217,  186,  244,  184,  231,  243,  207,  238,  210,  209,
 /*   170 */   192,  239,  253,  191,  190,  240,  216,  237,  232,  187,
 /*   180 */   185,  241,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammer, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  int stateno;       /* The state-number */
  int major;         /* The major token value.  This is the code
                     ** number for the token at this stack level */
  YYMINORTYPE minor; /* The user-supplied minor token value.  This
                     ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
  int yyerrcnt;                 /* Shifts left before out of the error */
  ParseARG_SDECL                /* A place to hold %extra_argument */
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void ParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "LC",            "RC",            "FORMAT",      
  "OR",            "AND",           "NOT",           "LIKE",        
  "MATCH",         "NE",            "EQ",            "REMOVE",      
  "GT",            "LE",            "LT",            "GE",          
  "ESCAPE",        "BITAND",        "BITOR",         "LSHIFT",      
  "RSHIFT",        "PLUS",          "MINUS",         "STAR",        
  "SLASH",         "MODULUS",       "REM",           "CONCAT",      
  "IF",            "THEN",          "ELSE",          "UMINUS",      
  "UPLUS",         "BITNOT",        "LP",            "RP",          
  "INTEGER_PARAM",  "HASH",          "AT",            "TOTAL",       
  "COUNT",         "MIN",           "COMMA",         "MAX",         
  "ABS",           "EVAL",          "INTEGER",       "INTEGER_GLOBAL",
  "ATOI",          "STRING",        "STRING_PARAM",  "PRINT",       
  "RUN",           "SUM",           "COLUMN",        "LB",          
  "RB",            "COLUMN_S",      "GS",            "PS",          
  "T",             "SHADER",        "SOUND",         "PORTRAIT",    
  "MODEL",         "RND",           "SYS_TIME",      "error",       
  "stmt",          "expr_i",        "expr_s",        "expr_a",      
  "lookup",        "search",      
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "stmt ::=",
 /*   1 */ "stmt ::= expr_i",
 /*   2 */ "stmt ::= STAR",
 /*   3 */ "expr_i ::= IF expr_i THEN expr_i ELSE expr_i",
 /*   4 */ "expr_s ::= IF expr_i THEN expr_s ELSE expr_s",
 /*   5 */ "expr_i ::= LP expr_i RP",
 /*   6 */ "expr_i ::= expr_i MINUS expr_i",
 /*   7 */ "expr_i ::= expr_i PLUS expr_i",
 /*   8 */ "expr_i ::= expr_i STAR expr_i",
 /*   9 */ "expr_i ::= expr_i MODULUS expr_i",
 /*  10 */ "expr_i ::= expr_i SLASH expr_i",
 /*  11 */ "expr_i ::= expr_i EQ expr_i",
 /*  12 */ "expr_i ::= expr_i LE expr_i",
 /*  13 */ "expr_i ::= expr_i NE expr_i",
 /*  14 */ "expr_i ::= expr_i LSHIFT expr_i",
 /*  15 */ "expr_i ::= expr_i LT expr_i",
 /*  16 */ "expr_i ::= expr_i GE expr_i",
 /*  17 */ "expr_i ::= expr_i RSHIFT expr_i",
 /*  18 */ "expr_i ::= expr_i GT expr_i",
 /*  19 */ "expr_i ::= expr_i AND expr_i",
 /*  20 */ "expr_i ::= expr_i OR expr_i",
 /*  21 */ "expr_i ::= NOT expr_i",
 /*  22 */ "expr_i ::= expr_i BITAND expr_i",
 /*  23 */ "expr_i ::= expr_i BITOR expr_i",
 /*  24 */ "expr_i ::= MINUS expr_i",
 /*  25 */ "expr_i ::= PLUS expr_i",
 /*  26 */ "expr_i ::= INTEGER_PARAM REMOVE expr_i",
 /*  27 */ "expr_i ::= HASH",
 /*  28 */ "expr_i ::= AT",
 /*  29 */ "expr_i ::= TOTAL",
 /*  30 */ "expr_i ::= COUNT",
 /*  31 */ "expr_i ::= MIN LP expr_i COMMA expr_i RP",
 /*  32 */ "expr_i ::= MAX LP expr_i COMMA expr_i RP",
 /*  33 */ "expr_i ::= ABS LP expr_i RP",
 /*  34 */ "expr_i ::= EVAL LP expr_s RP",
 /*  35 */ "expr_i ::= INTEGER",
 /*  36 */ "expr_i ::= INTEGER_PARAM",
 /*  37 */ "expr_i ::= INTEGER_GLOBAL",
 /*  38 */ "stmt ::= expr_s",
 /*  39 */ "expr_s ::= LP expr_s RP",
 /*  40 */ "expr_s ::= expr_i FORMAT",
 /*  41 */ "expr_i ::= ATOI LP expr_s RP",
 /*  42 */ "expr_i ::= expr_s LIKE expr_s",
 /*  43 */ "expr_i ::= expr_s EQ expr_s",
 /*  44 */ "expr_i ::= expr_s MATCH expr_s",
 /*  45 */ "expr_i ::= expr_s NE expr_s",
 /*  46 */ "expr_s ::= expr_s CONCAT expr_s",
 /*  47 */ "expr_s ::= STRING",
 /*  48 */ "expr_s ::= STRING_PARAM",
 /*  49 */ "expr_s ::= PRINT LP expr_s RP",
 /*  50 */ "expr_s ::= RUN LP expr_i RP",
 /*  51 */ "expr_s ::= LC expr_s",
 /*  52 */ "expr_s ::= expr_s RC",
 /*  53 */ "expr_s ::= STRING LC expr_s",
 /*  54 */ "expr_s ::= expr_s RC expr_s",
 /*  55 */ "stmt ::= expr_a",
 /*  56 */ "expr_a ::= COUNT LP STAR RP",
 /*  57 */ "expr_a ::= MIN LP expr_i RP",
 /*  58 */ "expr_a ::= MAX LP expr_i RP",
 /*  59 */ "expr_a ::= SUM LP expr_i RP",
 /*  60 */ "lookup ::= STRING COLUMN",
 /*  61 */ "search ::= lookup LB expr_i RB",
 /*  62 */ "search ::= lookup LB expr_s RB",
 /*  63 */ "expr_i ::= search COLUMN",
 /*  64 */ "expr_s ::= search COLUMN_S",
 /*  65 */ "expr_i ::= GS",
 /*  66 */ "expr_i ::= GS LP expr_i RP",
 /*  67 */ "expr_s ::= LT expr_s GT",
 /*  68 */ "expr_i ::= PS",
 /*  69 */ "expr_i ::= PS LP expr_i RP",
 /*  70 */ "expr_s ::= T",
 /*  71 */ "expr_i ::= SHADER LP expr_s RP",
 /*  72 */ "expr_i ::= SOUND LP expr_s RP",
 /*  73 */ "expr_i ::= PORTRAIT LP expr_s RP",
 /*  74 */ "expr_i ::= MODEL LP expr_s RP",
 /*  75 */ "expr_i ::= RND LP expr_i COMMA expr_i RP",
 /*  76 */ "expr_i ::= SYS_TIME",
};
#endif /* NDEBUG */

/*
** This function returns the symbolic name associated with a token
** value.
*/
const char *ParseTokenName(int tokenType){
#ifndef NDEBUG
  if( tokenType>0 && tokenType<(sizeof(yyTokenName)/sizeof(yyTokenName[0])) ){
    return yyTokenName[tokenType];
  }else{
    return "Unknown";
  }
#else
  return "";
#endif
}

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to Parse and ParseFree.
*/
void *ParseAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(YYCODETYPE yymajor, YYMINORTYPE *yypminor){
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor( yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from ParseAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void ParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
  (*freeProc)((void*)pParser);
}

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  if( iLookAhead==YYNOCODE ){
    return YY_NO_ACTION;
  }
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      int iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  /* int stateno = pParser->yystack[pParser->yyidx].stateno; */
 
  if( stateno>YY_REDUCE_MAX ||
      (i = yy_reduce_ofst[stateno])==YY_REDUCE_USE_DFLT ){
    return yy_default[stateno];
  }
  if( iLookAhead==YYNOCODE ){
    return YY_NO_ACTION;
  }
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer ot the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
  if( yypParser->yyidx>=YYSTACKDEPTH ){
     ParseARG_FETCH;
     yypParser->yyidx--;
#ifndef NDEBUG
     if( yyTraceFILE ){
       fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
     }
#endif
     while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
     /* Here code is inserted which will execute if the parser
     ** stack every overflows */
#line 30 "c:\\st-gpl\\code\\sql\\sql_parser.y"

	sql_error( "sql : stack overflow" );
#line 810 "c:\\st-gpl\\code\\sql\\sql_parser.c"
     ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
     return;
  }
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 68, 0 },
  { 68, 1 },
  { 68, 1 },
  { 69, 6 },
  { 70, 6 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 2 },
  { 69, 3 },
  { 69, 3 },
  { 69, 2 },
  { 69, 2 },
  { 69, 3 },
  { 69, 1 },
  { 69, 1 },
  { 69, 1 },
  { 69, 1 },
  { 69, 6 },
  { 69, 6 },
  { 69, 4 },
  { 69, 4 },
  { 69, 1 },
  { 69, 1 },
  { 69, 1 },
  { 68, 1 },
  { 70, 3 },
  { 70, 2 },
  { 69, 4 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 70, 3 },
  { 70, 1 },
  { 70, 1 },
  { 70, 4 },
  { 70, 4 },
  { 70, 2 },
  { 70, 2 },
  { 70, 3 },
  { 70, 3 },
  { 68, 1 },
  { 71, 4 },
  { 71, 4 },
  { 71, 4 },
  { 71, 4 },
  { 72, 2 },
  { 73, 4 },
  { 73, 4 },
  { 69, 2 },
  { 70, 2 },
  { 69, 1 },
  { 69, 4 },
  { 70, 3 },
  { 69, 1 },
  { 69, 4 },
  { 70, 1 },
  { 69, 4 },
  { 69, 4 },
  { 69, 4 },
  { 69, 4 },
  { 69, 6 },
  { 69, 1 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  ParseARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

#ifndef NDEBUG
  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  */
  memset(&yygotominor, 0, sizeof(yygotominor));
#endif

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 1:
#line 66 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ compile( yymsp[0].minor.yy0, INTEGER ); }
#line 965 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 2:
#line 68 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ compile( 0, ROW ); }
#line 970 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 3:
      case 4:
#line 74 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_NOP ), yymsp[-4].minor.yy0, OP_IFTHENELSE ); }
#line 976 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 5:
      case 39:
      case 52:
#line 77 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 983 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 6:
#line 79 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_SUBTRACT	); }
#line 988 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 7:
#line 80 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_ADD		); }
#line 993 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 8:
#line 81 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_MULTIPLY	); }
#line 998 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 9:
#line 82 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_MODULUS	); }
#line 1003 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 10:
#line 83 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_DIVIDE	); }
#line 1008 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 11:
#line 84 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_EQ		); }
#line 1013 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 12:
#line 85 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_LE		); }
#line 1018 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 13:
#line 86 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_NE		); }
#line 1023 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 14:
#line 87 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_LSHIFT	); }
#line 1028 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 15:
#line 88 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_LT		); }
#line 1033 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 16:
#line 89 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_GE		); }
#line 1038 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 17:
#line 90 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_RSHIFT	); }
#line 1043 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 18:
#line 91 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_GT		); }
#line 1048 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 19:
#line 92 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_LOGICAL_AND ); }
#line 1053 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 20:
#line 93 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_LOGICAL_OR ); }
#line 1058 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 21:
#line 94 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,yymsp[0].minor.yy0, OP_NOT		); }
#line 1063 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 22:
#line 95 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_BITWISE_AND ); }
#line 1068 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 23:
#line 96 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_BITWISE_OR ); }
#line 1073 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 24:
#line 97 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,yymsp[0].minor.yy0, OP_UMINUS	); }
#line 1078 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 25:
      case 35:
      case 36:
      case 37:
      case 47:
      case 48:
      case 51:
#line 98 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
#line 1089 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 26:
#line 99 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = yymsp[-2].minor.yy0; yymsp[-2].minor.yy0->right = yymsp[0].minor.yy0; yymsp[-2].minor.yy0->op = OP_REMOVE; }
#line 1094 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 27:
#line 101 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,0, OP_ROWINDEX ); }
#line 1099 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 28:
#line 102 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,0, OP_ROWNUMBER ); }
#line 1104 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 29:
#line 103 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,0, OP_ROWTOTAL ); }
#line 1109 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 30:
#line 104 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,0, OP_ROWCOUNT ); }
#line 1114 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 31:
#line 106 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-3].minor.yy0,yymsp[-1].minor.yy0, OP_INT_MIN ); }
#line 1119 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 32:
#line 107 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-3].minor.yy0,yymsp[-1].minor.yy0, OP_INT_MAX ); }
#line 1124 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 33:
#line 108 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,yymsp[-1].minor.yy0, OP_ABS ); }
#line 1129 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 34:
#line 110 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0, 0, OP_EVAL ); }
#line 1134 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 38:
#line 120 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ compile( yymsp[0].minor.yy0, STRING ); }
#line 1139 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 40:
#line 124 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0; yygotominor.yy0->right = yymsp[-1].minor.yy0; }
#line 1144 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 41:
#line 126 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0,0, OP_ATOI		); }
#line 1149 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 42:
      case 43:
#line 127 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_LIKE		); }
#line 1155 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 44:
#line 129 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_MATCH		); }
#line 1160 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 45:
#line 130 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_NOTLIKE	); }
#line 1165 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 46:
#line 131 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_CONCAT	); }
#line 1170 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 49:
#line 136 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0, 0, OP_PRINT ); }
#line 1175 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 50:
#line 137 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0, 0, OP_RUN ); }
#line 1180 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 53:
      case 54:
#line 145 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-2].minor.yy0,yymsp[0].minor.yy0, OP_NOP		); }
#line 1186 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 55:
#line 151 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ compile( yymsp[0].minor.yy0, AGGREGATE ); }
#line 1191 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 56:
#line 153 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,0, OP_COUNT ); }
#line 1196 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 57:
#line 154 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,yymsp[-1].minor.yy0, OP_MIN	); }
#line 1201 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 58:
#line 155 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,yymsp[-1].minor.yy0, OP_MAX	); }
#line 1206 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 59:
#line 156 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,yymsp[-1].minor.yy0, OP_SUM	); }
#line 1211 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 60:
#line 165 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0,yymsp[0].minor.yy0, OP_ACCESS_TABLE ); }
#line 1216 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 61:
#line 166 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-3].minor.yy0,yymsp[-1].minor.yy0, OP_LOOKUP_I ); }
#line 1221 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 62:
#line 167 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-3].minor.yy0,yymsp[-1].minor.yy0, OP_LOOKUP_S ); }
#line 1226 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 63:
#line 169 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0,yymsp[0].minor.yy0, OP_ACCESS_ROW_I ); }
#line 1231 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 64:
#line 170 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0,yymsp[0].minor.yy0, OP_ACCESS_ROW_S ); }
#line 1236 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 65:
#line 176 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0; yymsp[0].minor.yy0->op = OP_PUSH_GS; yymsp[0].minor.yy0->v.i = gs_i( yymsp[0].minor.yy0->v.s ); }
#line 1241 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 66:
#line 178 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{
		yymsp[-3].minor.yy0->op	= OP_PUSH_GS_OFFSET;
		yymsp[-3].minor.yy0->v.i	= gs_i( yymsp[-3].minor.yy0->v.s );
		yymsp[-3].minor.yy0->left	= yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0->right= 0;
		yygotominor.yy0 = yymsp[-3].minor.yy0;
	}
#line 1252 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 67:
#line 190 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0,0, OP_CVAR ); }
#line 1257 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 68:
#line 195 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0; yymsp[0].minor.yy0->op = OP_PUSH_PS_CLIENT; yymsp[0].minor.yy0->v.i = ps_i( yymsp[0].minor.yy0->v.s ); }
#line 1262 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 69:
#line 197 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{
		yymsp[-3].minor.yy0->op	= OP_PUSH_PS_CLIENT_OFFSET;
		yymsp[-3].minor.yy0->v.i	= ps_i( yymsp[-3].minor.yy0->v.s );
		yymsp[-3].minor.yy0->left	= yymsp[-1].minor.yy0;
		yymsp[-3].minor.yy0->right= 0;
		yygotominor.yy0 = yymsp[-3].minor.yy0;
	}
#line 1273 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 70:
#line 208 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[0].minor.yy0,0, OP_RUN ); yymsp[0].minor.yy0->op = OP_PUSH_INTEGER; yymsp[0].minor.yy0->v.i = local( yymsp[0].minor.yy0->v.s, yymsp[0].minor.yy0->n ); }
#line 1278 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 71:
#line 213 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0,0, OP_SHADER ); }
#line 1283 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 72:
#line 214 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0,0, OP_SOUND ); }
#line 1288 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 73:
#line 215 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0,0, OP_PORTRAIT ); }
#line 1293 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 74:
#line 216 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-1].minor.yy0,0, OP_MODEL ); }
#line 1298 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 75:
#line 217 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( yymsp[-3].minor.yy0,yymsp[-1].minor.yy0, OP_RND ); }
#line 1303 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
      case 76:
#line 218 "c:\\st-gpl\\code\\sql\\sql_parser.y"
{ yygotominor.yy0 = op( 0,0, OP_SYS_TIME ); }
#line 1308 "c:\\st-gpl\\code\\sql\\sql_parser.c"
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = yyact;
      yymsp->major = yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else if( yyact == YYNSTATE + YYNRULE + 1 ){
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 27 "c:\\st-gpl\\code\\sql\\sql_parser.y"

	sql_error( "sql : syntax error" );
#line 1369 "c:\\st-gpl\\code\\sql\\sql_parser.c"
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
    /* if( yymajor==0 ) return; // not sure why this was here... */
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  ParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,yymajor);
    if( yyact<YYNSTATE ){
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      if( yyendofinput && yypParser->yyidx>=0 ){
        yymajor = 0;
      }else{
        yymajor = YYNOCODE;
      }
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else if( yyact == YY_ERROR_ACTION ){
      int yymx;
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }else{
      yy_accept(yypParser);
      yymajor = YYNOCODE;
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
