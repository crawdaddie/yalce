/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INTEGER = 258,
     DOUBLE = 259,
     FLOAT = 260,
     IDENTIFIER = 261,
     PATH_IDENTIFIER = 262,
     IDENTIFIER_LIST = 263,
     TOK_STRING = 264,
     TOK_CHAR = 265,
     TRUE = 266,
     FALSE = 267,
     PIPE = 268,
     EXTERN = 269,
     TRIPLE_DOT = 270,
     LET = 271,
     FN = 272,
     MODULE = 273,
     MATCH = 274,
     WITH = 275,
     ARROW = 276,
     DOUBLE_COLON = 277,
     TOK_VOID = 278,
     IN = 279,
     AND = 280,
     ASYNC = 281,
     DOUBLE_AT = 282,
     THUNK = 283,
     IMPORT = 284,
     OPEN = 285,
     IMPLEMENTS = 286,
     AMPERSAND = 287,
     TYPE = 288,
     TEST_ID = 289,
     MUT = 290,
     THEN = 291,
     ELSE = 292,
     FSTRING_START = 293,
     FSTRING_END = 294,
     FSTRING_INTERP_START = 295,
     FSTRING_INTERP_END = 296,
     FSTRING_TEXT = 297,
     DOUBLE_PIPE = 298,
     DOUBLE_AMP = 299,
     NE = 300,
     EQ = 301,
     LE = 302,
     GE = 303,
     MODULO = 304,
     APPLICATION = 305,
     UMINUS = 306
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define DOUBLE 259
#define FLOAT 260
#define IDENTIFIER 261
#define PATH_IDENTIFIER 262
#define IDENTIFIER_LIST 263
#define TOK_STRING 264
#define TOK_CHAR 265
#define TRUE 266
#define FALSE 267
#define PIPE 268
#define EXTERN 269
#define TRIPLE_DOT 270
#define LET 271
#define FN 272
#define MODULE 273
#define MATCH 274
#define WITH 275
#define ARROW 276
#define DOUBLE_COLON 277
#define TOK_VOID 278
#define IN 279
#define AND 280
#define ASYNC 281
#define DOUBLE_AT 282
#define THUNK 283
#define IMPORT 284
#define OPEN 285
#define IMPLEMENTS 286
#define AMPERSAND 287
#define TYPE 288
#define TEST_ID 289
#define MUT 290
#define THEN 291
#define ELSE 292
#define FSTRING_START 293
#define FSTRING_END 294
#define FSTRING_INTERP_START 295
#define FSTRING_INTERP_END 296
#define FSTRING_TEXT 297
#define DOUBLE_PIPE 298
#define DOUBLE_AMP 299
#define NE 300
#define EQ 301
#define LE 302
#define GE 303
#define MODULO 304
#define APPLICATION 305
#define UMINUS 306




/* Copy the first part of user declarations.  */
#line 1 "lang/parser.y"

#ifndef _LANG_TAB_H
#define _LANG_TAB_H
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parse.h"
#include "serde.h"
#include "common.h"
#include <string.h>

/* prototypes */
extern void yyerror(const char *s);

extern int yylineno;
extern int yycolumn;
extern char *yytext;

/* Define global variable for the root of AST */
Ast* ast_root = NULL;
#define AST_CONST(type, val)                                            \
    ({                                                                  \
      Ast *prefix = Ast_new(type);                                      \
      prefix->data.type.value = val;                                    \
      prefix;                                                           \
    })



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 31 "lang/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    float vfloat;
    char vchar;
}
/* Line 193 of yacc.c.  */
#line 237 "lang/y.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 262 "lang/y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  92
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2161

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  74
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  26
/* YYNRULES -- Number of rules.  */
#define YYNRULES  147
/* YYNRULES -- Number of states.  */
#define YYNSTATES  311

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   306

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      68,    69,    54,    52,    71,    53,    59,    55,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    57,    61,
      47,    65,    46,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    66,     2,    67,     2,    70,     2,     2,     2,     2,
       2,     2,    64,     2,     2,    72,     2,     2,     2,     2,
       2,    73,     2,     2,     2,     2,    63,     2,     2,     2,
       2,    62,     2,     2,    43,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    44,    45,
      48,    49,    50,    51,    56,    58,    60
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    11,    14,    18,    21,
      25,    29,    33,    37,    41,    45,    49,    53,    57,    61,
      65,    69,    73,    77,    81,    85,    89,    91,    93,    95,
      98,   101,   103,   110,   115,   120,   122,   126,   128,   130,
     132,   134,   136,   138,   140,   142,   144,   146,   148,   150,
     152,   156,   163,   170,   174,   178,   182,   186,   190,   194,
     198,   202,   206,   210,   214,   218,   222,   226,   230,   234,
     238,   240,   244,   249,   254,   261,   266,   272,   277,   283,
     288,   292,   294,   301,   308,   311,   314,   317,   320,   327,
     333,   339,   345,   351,   353,   357,   363,   366,   371,   378,
     380,   384,   388,   390,   393,   397,   402,   407,   413,   420,
     424,   428,   433,   435,   439,   444,   451,   456,   458,   462,
     467,   473,   479,   483,   484,   487,   489,   493,   498,   501,
     506,   509,   512,   516,   520,   524,   528,   530,   533,   537,
     539,   541,   543,   547,   551,   555,   559,   561
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      75,     0,    -1,    79,    61,    -1,    79,    -1,    -1,    77,
      -1,    62,    76,    -1,    76,    27,    76,    -1,    76,    77,
      -1,    76,    52,    76,    -1,    76,    53,    76,    -1,    76,
      54,    76,    -1,    76,    55,    76,    -1,    76,    56,    76,
      -1,    76,    47,    76,    -1,    76,    46,    76,    -1,    76,
      45,    76,    -1,    76,    44,    76,    -1,    76,    51,    76,
      -1,    76,    50,    76,    -1,    76,    48,    76,    -1,    76,
      49,    76,    -1,    76,    13,    76,    -1,    76,    57,    76,
      -1,    76,    63,    76,    -1,    76,    22,    76,    -1,    80,
      -1,    88,    -1,    94,    -1,    28,    76,    -1,    15,    76,
      -1,     8,    -1,    64,     6,    65,    76,    24,    76,    -1,
      76,    66,    76,    67,    -1,    76,    57,    65,    76,    -1,
      78,    -1,    77,    59,     6,    -1,     3,    -1,     4,    -1,
       5,    -1,     9,    -1,    11,    -1,    12,    -1,     6,    -1,
      23,    -1,    84,    -1,    85,    -1,    86,    -1,    91,    -1,
      10,    -1,    68,    79,    69,    -1,    68,    17,    82,    21,
      79,    69,    -1,    68,    17,    23,    21,    79,    69,    -1,
      68,    52,    69,    -1,    68,    53,    69,    -1,    68,    54,
      69,    -1,    68,    55,    69,    -1,    68,    56,    69,    -1,
      68,    47,    69,    -1,    68,    46,    69,    -1,    68,    45,
      69,    -1,    68,    44,    69,    -1,    68,    51,    69,    -1,
      68,    50,    69,    -1,    68,    48,    69,    -1,    68,    49,
      69,    -1,    68,    13,    69,    -1,    68,    57,    69,    -1,
      68,    22,    69,    -1,    68,     6,    69,    -1,    76,    -1,
      79,    61,    76,    -1,    16,    34,    65,    76,    -1,    16,
       6,    65,    76,    -1,    16,     6,    65,    14,    17,    96,
      -1,    16,    83,    65,    76,    -1,    16,    35,    83,    65,
      76,    -1,    16,    87,    65,    76,    -1,    16,    35,    87,
      65,    76,    -1,    16,    23,    65,    76,    -1,    80,    24,
      76,    -1,    81,    -1,    16,    68,     6,    69,    65,    81,
      -1,    16,    68,     6,    69,    65,    76,    -1,    29,     7,
      -1,    30,     7,    -1,    29,     6,    -1,    30,     6,    -1,
      16,     6,    57,     6,    65,    81,    -1,    17,    82,    21,
      79,    61,    -1,    17,    23,    21,    79,    61,    -1,    18,
      82,    21,    79,    61,    -1,    18,    23,    21,    79,    61,
      -1,    83,    -1,    83,    65,    76,    -1,    83,    57,    68,
      98,    69,    -1,    82,    83,    -1,    82,    83,    65,    76,
      -1,    82,    83,    57,    68,    98,    69,    -1,     6,    -1,
      68,    87,    69,    -1,     6,    22,    83,    -1,    70,    -1,
      66,    67,    -1,    66,    87,    67,    -1,    66,    87,    71,
      67,    -1,    66,    43,    43,    67,    -1,    66,    43,    87,
      43,    67,    -1,    66,    43,    87,    71,    43,    67,    -1,
      68,    76,    69,    -1,    68,    87,    69,    -1,    68,    87,
      71,    69,    -1,    76,    -1,    87,    71,    76,    -1,    19,
      76,    20,    90,    -1,    72,    76,    36,    76,    37,    76,
      -1,    72,    76,    36,    76,    -1,    76,    -1,    76,    72,
      76,    -1,    43,    89,    21,    76,    -1,    90,    43,    89,
      21,    76,    -1,    90,    43,    70,    21,    76,    -1,    38,
      92,    39,    -1,    -1,    92,    93,    -1,    42,    -1,    40,
      76,    41,    -1,    33,     6,    65,    98,    -1,    33,     6,
      -1,    33,    95,    65,    98,    -1,     6,     6,    -1,    95,
       6,    -1,    98,    21,    98,    -1,    96,    21,    98,    -1,
      99,    71,    99,    -1,    97,    71,    99,    -1,    99,    -1,
      43,    99,    -1,    98,    43,    99,    -1,    96,    -1,    97,
      -1,     6,    -1,     6,    65,     3,    -1,     6,    73,    99,
      -1,     6,    57,    99,    -1,    68,    98,    69,    -1,    23,
      -1,     6,    59,     6,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   124,   124,   125,   126,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   164,   165,   169,   170,   174,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     211,   212,   216,   217,   218,   221,   223,   227,   229,   237,
     238,   243,   245,   252,   262,   263,   264,   265,   267,   273,
     274,   275,   276,   283,   284,   285,   286,   287,   288,   292,
     293,   294,   295,   300,   301,   302,   306,   307,   308,   313,
     314,   315,   319,   320,   324,   325,   326,   330,   331,   334,
     335,   336,   339,   343,   344,   348,   349,   353,   359,   365,
     378,   379,   382,   383,   387,   388,   392,   393,   394,   395,
     396,   400,   401,   402,   403,   404,   405,   406
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INTEGER", "DOUBLE", "FLOAT",
  "IDENTIFIER", "PATH_IDENTIFIER", "IDENTIFIER_LIST", "TOK_STRING",
  "TOK_CHAR", "TRUE", "FALSE", "PIPE", "EXTERN", "TRIPLE_DOT", "LET", "FN",
  "MODULE", "MATCH", "WITH", "ARROW", "DOUBLE_COLON", "TOK_VOID", "IN",
  "AND", "ASYNC", "DOUBLE_AT", "THUNK", "IMPORT", "OPEN", "IMPLEMENTS",
  "AMPERSAND", "TYPE", "TEST_ID", "MUT", "THEN", "ELSE", "FSTRING_START",
  "FSTRING_END", "FSTRING_INTERP_START", "FSTRING_INTERP_END",
  "FSTRING_TEXT", "'|'", "DOUBLE_PIPE", "DOUBLE_AMP", "'>'", "'<'", "NE",
  "EQ", "LE", "GE", "'+'", "'-'", "'*'", "'/'", "MODULO", "':'",
  "APPLICATION", "'.'", "UMINUS", "';'", "'yield'", "'to'", "'for'", "'='",
  "'['", "']'", "'('", "')'", "'_'", "','", "'if'", "'of'", "$accept",
  "program", "expr", "atom_expr", "simple_expr", "expr_sequence",
  "let_binding", "lambda_expr", "lambda_args", "lambda_arg", "list",
  "array", "tuple", "expr_list", "match_expr", "match_test_clause",
  "match_branches", "fstring", "fstring_parts", "fstring_part",
  "type_decl", "type_args", "fn_signature", "tuple_type", "type_expr",
  "type_atom", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   124,   298,   299,    62,    60,   300,   301,
     302,   303,    43,    45,    42,    47,   304,    58,   305,    46,
     306,    59,   121,   116,   102,    61,    91,    93,    40,    41,
      95,    44,   105,   111
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    74,    75,    75,    75,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    77,    77,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      79,    79,    80,    80,    80,    80,    80,    80,    80,    80,
      80,    80,    80,    80,    80,    80,    80,    80,    80,    81,
      81,    81,    81,    82,    82,    82,    82,    82,    82,    83,
      83,    83,    83,    84,    84,    84,    85,    85,    85,    86,
      86,    86,    87,    87,    88,    88,    88,    89,    89,    90,
      90,    90,    91,    92,    92,    93,    93,    94,    94,    94,
      95,    95,    96,    96,    97,    97,    98,    98,    98,    98,
      98,    99,    99,    99,    99,    99,    99,    99
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     2,     3,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     1,     2,
       2,     1,     6,     4,     4,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     6,     6,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       1,     3,     4,     4,     6,     4,     5,     4,     5,     4,
       3,     1,     6,     6,     2,     2,     2,     2,     6,     5,
       5,     5,     5,     1,     3,     5,     2,     4,     6,     1,
       3,     3,     1,     2,     3,     4,     4,     5,     6,     3,
       3,     4,     1,     3,     4,     6,     4,     1,     3,     4,
       5,     5,     3,     0,     2,     1,     3,     4,     2,     4,
       2,     2,     3,     3,     3,     3,     1,     2,     3,     1,
       1,     1,     3,     3,     3,     3,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    37,    38,    39,    43,    31,    40,    49,    41,    42,
       0,     0,     0,     0,     0,    44,     0,     0,     0,     0,
     123,     0,     0,     0,     0,     0,     0,    70,     5,    35,
       3,    26,    81,    45,    46,    47,    27,    48,    28,    30,
      43,    44,     0,     0,     0,   102,   112,     0,     0,    99,
       0,     0,     0,    93,     0,     0,     0,    29,    86,    84,
      87,    85,   128,     0,     0,     6,     0,     0,   103,     0,
      43,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    70,     0,
       0,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     0,     2,     0,     0,     0,     0,     0,
       0,    43,     0,     0,     0,    43,     0,     0,     0,     0,
       0,     0,     0,    96,     0,     0,     0,     0,     0,   130,
       0,   131,     0,   122,     0,   125,   124,     0,     0,     0,
     104,     0,    69,    66,     0,     0,    68,    61,    60,    59,
      58,    64,    65,    63,    62,    53,    54,    55,    56,    57,
      67,   109,     0,    50,   110,     0,     0,    22,    25,     7,
      17,    16,    15,    14,    20,    21,    19,    18,     9,    10,
      11,    12,    13,     0,    23,    24,   112,    36,    71,    80,
     101,     0,     0,    73,    79,    72,     0,     0,    69,   110,
      75,    77,   113,     0,   100,     0,     0,     0,     0,    94,
       0,     0,     0,   114,   141,   146,     0,     0,   139,   140,
     127,   136,   129,     0,     0,   106,     0,     0,   105,     0,
       0,   111,   116,    34,    33,     0,     0,    76,    78,     0,
      90,    89,     0,    97,     0,    92,    91,   117,     0,     0,
       0,     0,     0,     0,   137,     0,     0,     0,     0,     0,
       0,   126,     0,   107,     0,     0,     0,     0,    88,    74,
       0,    83,    81,     0,    95,     0,     0,     0,     0,   144,
     147,   142,   143,   145,   133,   135,   132,   138,   134,    32,
     108,    52,    51,   115,    98,   118,   119,     0,     0,   121,
     120
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    26,    27,    28,    29,    89,    31,    32,    52,    53,
      33,    34,    35,    69,    36,   258,   223,    37,    64,   146,
      38,    63,   228,   229,   230,   231
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -108
static const yytype_int16 yypact[] =
{
    1270,  -108,  -108,  -108,  -108,  -108,  -108,  -108,  -108,  -108,
    1270,   570,    -4,     1,  1270,  -108,  1270,   157,   168,    17,
    -108,  1270,    33,   640,   430,  1270,     9,  1766,   -47,  -108,
      12,   119,  -108,  -108,  -108,  -108,  -108,  -108,  -108,  1766,
      89,    67,    83,   710,   500,  -108,  1766,    90,   -11,   137,
     155,  1270,     4,   -30,   159,    10,  1370,  1766,  -108,  -108,
    -108,  -108,    38,    41,   118,  1766,   117,   780,  -108,   -41,
     116,   120,    53,   125,   126,   129,   130,   131,   132,   133,
     134,   135,   136,   139,   140,   142,   145,   146,  1301,   -33,
     -13,  1436,  -108,  1270,  1270,  1270,  1270,  1270,  1270,  1270,
    1270,  1270,  1270,  1270,  1270,  1270,  1270,  1270,  1270,   850,
    1270,   640,   -47,   180,  1270,  1270,    56,   200,   920,  1270,
    1270,   137,   430,   151,    49,   148,    98,  1270,  1270,  1270,
    1270,   102,  1270,    -2,   150,  1270,  1270,  1270,   176,  -108,
      59,  -108,    59,  -108,  1270,  -108,  -108,  1270,   153,   -21,
    -108,   990,  -108,  -108,   201,    11,  -108,  -108,  -108,  -108,
    -108,  -108,  -108,  -108,  -108,  -108,  -108,  -108,  -108,  -108,
    -108,  -108,  1270,  -108,  -108,  1060,  1270,  1832,  2093,  1898,
    1964,  1964,   225,   225,   225,   225,   225,   225,  1985,  1985,
    2051,  2051,  2072,  1270,  2093,  1766,  1502,  -108,  1766,  1766,
    -108,   156,   206,  1766,  1766,  1766,  1270,  1270,   160,   161,
    1766,  1766,  1766,   163,  -108,   171,   165,  1270,    59,  1766,
     177,   183,  1270,   207,    71,  -108,    47,    59,   230,   181,
       0,   182,     0,  1568,  1634,  -108,   187,  1130,  -108,  1270,
    1270,  -108,  1700,  1766,  -108,   175,    59,  1766,  1766,  1270,
    1270,  1270,    59,  1766,    -1,  1270,  1270,   339,   234,  1200,
      47,   250,   254,    47,  -108,     8,    59,    47,    59,    47,
      47,  -108,  1270,  -108,   191,     6,    84,  1270,  -108,   230,
       0,  1766,  -108,   108,  -108,  1270,  1270,   238,   239,  -108,
    -108,  -108,  -108,  -108,     0,  -108,     0,  -108,  -108,  1766,
    -108,  -108,  -108,  1766,  -108,  1766,  1766,  1270,  1270,  1766,
    1766
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -108,  -108,   -10,   122,  -108,     3,  -108,  -107,    35,    -3,
    -108,  -108,  -108,    -6,  -108,     2,  -108,  -108,  -108,  -108,
    -108,  -108,    18,  -108,   -96,   -79
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -113
static const yytype_int16 yytable[] =
{
      39,    46,    49,    30,    56,    48,    57,    49,    47,    92,
      49,    65,   113,    46,    88,    91,    49,    49,    90,    50,
     268,   268,   236,    62,    54,   132,   150,   134,   172,   268,
     151,   137,   240,    46,    88,   135,   173,   124,   126,    66,
     123,    46,   269,   269,   139,   131,   232,   141,    55,   133,
     237,   269,   133,   224,   128,   216,   174,    46,   175,    49,
     129,   149,    49,   217,    51,   224,    45,   250,   284,    51,
     225,    45,    51,   114,    45,   301,   154,   293,    51,    51,
      45,    45,   225,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   194,
     195,   196,   226,   140,   198,   199,   142,   155,   203,   204,
     205,   116,    88,   200,   207,   227,   126,   210,   211,   212,
     129,    51,   254,    45,    51,   219,    45,   227,   260,   268,
     261,   265,   119,   213,   233,   215,   262,   234,   278,   220,
     221,   212,   282,   115,   263,   251,   117,   264,   120,   112,
     280,   269,   133,   302,   118,   127,   283,   143,   144,   116,
     145,   112,   198,    58,    59,   212,   242,   209,   112,   175,
     294,   214,   296,   129,    60,    61,   130,   304,   112,   112,
     136,   289,   147,   243,   292,   152,   197,   112,   295,   153,
     297,   298,    12,    13,   156,   157,   247,   248,   158,   159,
     160,   161,   162,   163,   164,   165,   201,   253,   166,   167,
     112,   168,   257,   112,   169,   170,   206,   208,   218,   222,
     235,   245,   239,   246,   250,   249,  -100,   212,     1,     2,
       3,     4,   251,   252,     6,     7,     8,     9,   255,   281,
     198,   198,   275,   276,   256,   198,   198,    94,    15,   257,
     259,   266,   267,   270,   273,   286,   290,   291,   300,   307,
     308,   288,   299,    20,   279,     0,     0,   303,     0,     0,
       0,     0,     0,     0,     0,   305,   306,   104,   105,   106,
     107,   108,   109,     0,     0,     0,     0,     0,   110,     0,
       0,   111,     0,    24,     0,     0,     0,   309,   310,   112,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   112,
     112,   112,   112,   112,   112,     0,   112,   112,   112,     0,
     112,   112,     0,     0,     0,   112,   112,   112,     0,     0,
       0,     0,   112,   112,   112,     0,     0,     0,     0,     0,
       0,   112,     1,     2,     3,     4,     0,     0,     6,     7,
       8,     9,    93,     0,     0,   112,   112,     0,     0,     0,
       0,    94,    15,     0,   112,   112,    95,     0,     0,   112,
     112,     0,     0,     0,     0,   112,     0,    20,     0,   112,
       0,     0,     0,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,     0,     0,     0,
       0,     0,   110,   112,     0,   111,     0,    24,     0,     0,
       0,   285,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   112,     0,     0,     0,   112,     0,   112,   112,     0,
       0,   112,   112,     1,     2,     3,    70,     0,     5,     6,
       7,     8,     9,    71,     0,    10,    11,    72,    13,    14,
       0,     0,    73,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,     0,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
       0,     0,    21,     0,    22,     0,    23,     0,    24,     0,
       0,     0,    25,     1,     2,     3,   125,     0,     5,     6,
       7,     8,     9,    71,     0,    10,    11,    72,    13,    14,
       0,     0,    73,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,     0,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
       0,     0,    21,     0,    22,     0,    23,     0,    24,     0,
       0,     0,    25,     1,     2,     3,    40,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
       0,     0,     0,    41,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,    42,    43,     0,     0,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    21,     0,    22,     0,    23,     0,    44,     0,
      45,     0,    25,     1,     2,     3,     4,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,    67,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    21,     0,    22,     0,    23,    68,    24,     0,
       0,     0,    25,     1,     2,     3,   121,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    21,     0,    22,     0,    23,     0,   122,     0,
      45,     0,    25,     1,     2,     3,     4,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,   148,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    21,     0,    22,     0,    23,     0,    24,     0,
       0,     0,    25,     1,     2,     3,     4,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    21,     0,    22,   193,    23,     0,    24,     0,
       0,     0,    25,     1,     2,     3,     4,     0,     5,     6,
       7,     8,     9,     0,   202,    10,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    21,     0,    22,     0,    23,     0,    24,     0,
       0,     0,    25,     1,     2,     3,     4,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    21,     0,    22,     0,    23,   238,    24,     0,
       0,     0,    25,     1,     2,     3,     4,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    21,     0,    22,     0,    23,     0,    24,   241,
       0,     0,    25,     1,     2,     3,     4,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,   274,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    21,     0,    22,     0,    23,     0,    24,     0,
       0,     0,    25,     1,     2,     3,     4,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    21,     0,    22,     0,    23,     0,    24,     0,
     287,     0,    25,     1,     2,     3,     4,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     1,     2,     3,     4,    20,     0,
       6,     7,     8,     9,    93,     0,     0,     0,     0,     0,
       0,     0,     0,    94,    15,     0,     0,     0,    95,     0,
       0,     0,    21,     0,    22,     0,    23,     0,    24,    20,
       0,     0,    25,     0,     0,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,     0,
       0,     0,     0,     0,   110,     0,     0,   111,     0,    24,
     171,     0,  -112,     1,     2,     3,     4,     0,     0,     6,
       7,     8,     9,    93,     0,     0,     0,     0,     0,     0,
     138,     0,    94,    15,     0,     0,     0,    95,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    20,     0,
       0,     0,     0,     0,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,     0,     0,
       0,     0,     0,   110,     0,     0,   111,     0,    24,     1,
       2,     3,     4,     0,     0,     6,     7,     8,     9,    93,
       0,     0,     0,     0,     0,     0,     0,     0,    94,    15,
       0,     0,     0,    95,     0,     0,     0,     0,     0,     0,
       0,     0,   176,     0,    20,     0,     0,     0,     0,     0,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,     0,     0,     0,     0,     0,   110,
       0,     0,   111,     0,    24,     1,     2,     3,     4,     0,
       0,     6,     7,     8,     9,    93,     0,     0,     0,     0,
       0,     0,     0,     0,    94,    15,     0,     0,     0,    95,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      20,     0,     0,     0,     0,     0,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
       0,     0,     0,     0,     0,   110,     0,     0,   111,   244,
      24,     1,     2,     3,     4,     0,     0,     6,     7,     8,
       9,    93,     0,     0,     0,     0,     0,     0,     0,     0,
      94,    15,     0,     0,     0,    95,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    20,     0,     0,   271,
       0,     0,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,     0,     0,     0,     0,
       0,   110,     0,     0,   111,     0,    24,     1,     2,     3,
       4,     0,     0,     6,     7,     8,     9,    93,     0,     0,
       0,     0,     0,     0,     0,     0,    94,    15,   272,     0,
       0,    95,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    20,     0,     0,     0,     0,     0,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,     0,     0,     0,     0,     0,   110,     0,     0,
     111,     0,    24,     1,     2,     3,     4,     0,     0,     6,
       7,     8,     9,    93,     0,     0,     0,     0,     0,     0,
       0,     0,    94,    15,     0,     0,     0,    95,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   277,    20,     0,
       0,     0,     0,     0,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,     0,     0,
       0,     0,     0,   110,     0,     0,   111,     0,    24,     1,
       2,     3,     4,     0,     0,     6,     7,     8,     9,    93,
       0,     0,     0,     0,     0,     0,     0,     0,    94,    15,
       0,     0,     0,    95,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    20,     0,     0,     0,     0,     0,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,     0,     0,     0,     0,     0,   110,
       0,     0,   111,     0,    24,     1,     2,     3,     4,     0,
       0,     6,     7,     8,     9,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    94,    15,     0,     0,     0,    95,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      20,     0,     0,     0,     0,     0,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
       0,     0,     0,     0,     0,   110,     0,     0,   111,     0,
      24,     1,     2,     3,     4,     0,     0,     6,     7,     8,
       9,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      94,    15,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    20,     0,     0,     0,
       0,     0,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,     0,     0,     0,     0,
       0,   110,     0,     0,   111,     0,    24,     1,     2,     3,
       4,     0,     0,     6,     7,     8,     9,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    94,    15,     1,     2,
       3,     4,     0,     0,     6,     7,     8,     9,     0,     0,
       0,     0,    20,     0,     0,     0,     0,    94,    15,     0,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,     0,    20,     0,     0,     0,   110,     0,     0,
     111,     0,    24,     0,     0,     0,     0,     0,     0,   106,
     107,   108,   109,     0,     0,     0,     0,     0,   110,     0,
       0,   111,     0,    24,     1,     2,     3,     4,     0,     0,
       6,     7,     8,     9,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    94,    15,     1,     2,     3,     4,     0,
       0,     6,     7,     8,     9,     0,     0,     0,     0,    20,
       0,     0,     0,     0,    94,    15,     1,     2,     3,     4,
       0,     0,     6,     7,     8,     9,     0,   108,   109,     0,
      20,     0,     0,     0,   110,    94,    15,   111,     0,    24,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   109,
       0,    20,     0,     0,     0,   110,     0,     0,   111,     0,
      24,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   110,     0,     0,   111,
       0,    24
};

static const yytype_int16 yycheck[] =
{
      10,    11,     6,     0,    14,    11,    16,     6,    11,     0,
       6,    21,    59,    23,    24,    25,     6,     6,    24,    23,
      21,    21,    43,     6,    23,    21,    67,    57,    61,    21,
      71,    21,    21,    43,    44,    65,    69,    43,    44,     6,
      43,    51,    43,    43,     6,    51,   142,     6,    13,    52,
      71,    43,    55,     6,    65,    57,    69,    67,    71,     6,
      71,    67,     6,    65,    68,     6,    70,    61,    69,    68,
      23,    70,    68,    61,    70,    69,    23,    69,    68,    68,
      70,    70,    23,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    43,    65,   114,   115,    65,    72,   118,   119,
     120,    22,   122,   116,    65,    68,   122,   127,   128,   129,
      71,    68,   218,    70,    68,   135,    70,    68,    57,    21,
      59,   227,    65,   130,   144,   132,    65,   147,   245,   136,
     137,   151,   249,    24,    73,    61,    57,   226,    65,    27,
     246,    43,   155,    69,    65,    65,   252,    39,    40,    22,
      42,    39,   172,     6,     7,   175,   176,    69,    46,    71,
     266,    69,   268,    71,     6,     7,    21,    69,    56,    57,
      21,   260,    65,   193,   263,    69,     6,    65,   267,    69,
     269,   270,    17,    18,    69,    69,   206,   207,    69,    69,
      69,    69,    69,    69,    69,    69,     6,   217,    69,    69,
      88,    69,   222,    91,    69,    69,    65,    69,    68,    43,
      67,    65,    21,    17,    61,    65,    65,   237,     3,     4,
       5,     6,    61,    68,     9,    10,    11,    12,    61,   249,
     250,   251,   239,   240,    61,   255,   256,    22,    23,   259,
      43,    21,    71,    71,    67,    21,     6,     3,    67,    21,
      21,   259,   272,    38,   246,    -1,    -1,   277,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   285,   286,    52,    53,    54,
      55,    56,    57,    -1,    -1,    -1,    -1,    -1,    63,    -1,
      -1,    66,    -1,    68,    -1,    -1,    -1,   307,   308,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,    -1,   194,   195,   196,    -1,
     198,   199,    -1,    -1,    -1,   203,   204,   205,    -1,    -1,
      -1,    -1,   210,   211,   212,    -1,    -1,    -1,    -1,    -1,
      -1,   219,     3,     4,     5,     6,    -1,    -1,     9,    10,
      11,    12,    13,    -1,    -1,   233,   234,    -1,    -1,    -1,
      -1,    22,    23,    -1,   242,   243,    27,    -1,    -1,   247,
     248,    -1,    -1,    -1,    -1,   253,    -1,    38,    -1,   257,
      -1,    -1,    -1,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    -1,    -1,    -1,
      -1,    -1,    63,   281,    -1,    66,    -1,    68,    -1,    -1,
      -1,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   299,    -1,    -1,    -1,   303,    -1,   305,   306,    -1,
      -1,   309,   310,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    13,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    -1,    68,    -1,
      -1,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    13,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    -1,    68,    -1,
      -1,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    34,    35,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    -1,    68,    -1,
      70,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    67,    68,    -1,
      -1,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    -1,    68,    -1,
      70,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    -1,    68,    -1,
      -1,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    64,    65,    66,    -1,    68,    -1,
      -1,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    14,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    -1,    68,    -1,
      -1,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    67,    68,    -1,
      -1,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    -1,    68,    69,
      -1,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    -1,    68,    -1,
      -1,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    -1,    68,    -1,
      70,    -1,    72,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    23,    -1,    -1,    -1,    -1,    28,    29,
      30,    -1,    -1,    33,     3,     4,     5,     6,    38,    -1,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    22,    23,    -1,    -1,    -1,    27,    -1,
      -1,    -1,    62,    -1,    64,    -1,    66,    -1,    68,    38,
      -1,    -1,    72,    -1,    -1,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    -1,
      -1,    -1,    -1,    -1,    63,    -1,    -1,    66,    -1,    68,
      69,    -1,    71,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      20,    -1,    22,    23,    -1,    -1,    -1,    27,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      -1,    -1,    -1,    63,    -1,    -1,    66,    -1,    68,     3,
       4,     5,     6,    -1,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,
      -1,    -1,    -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    36,    -1,    38,    -1,    -1,    -1,    -1,    -1,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    -1,    -1,    -1,    -1,    -1,    63,
      -1,    -1,    66,    -1,    68,     3,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    22,    23,    -1,    -1,    -1,    27,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      38,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    -1,    -1,    63,    -1,    -1,    66,    67,
      68,     3,     4,     5,     6,    -1,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      22,    23,    -1,    -1,    -1,    27,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    41,
      -1,    -1,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    -1,    -1,    -1,    -1,
      -1,    63,    -1,    -1,    66,    -1,    68,     3,     4,     5,
       6,    -1,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,
      -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    38,    -1,    -1,    -1,    -1,    -1,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    -1,    -1,    -1,    -1,    -1,    63,    -1,    -1,
      66,    -1,    68,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    22,    23,    -1,    -1,    -1,    27,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    -1,
      -1,    -1,    -1,    -1,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      -1,    -1,    -1,    63,    -1,    -1,    66,    -1,    68,     3,
       4,     5,     6,    -1,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,
      -1,    -1,    -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    38,    -1,    -1,    -1,    -1,    -1,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    -1,    -1,    -1,    -1,    -1,    63,
      -1,    -1,    66,    -1,    68,     3,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    22,    23,    -1,    -1,    -1,    27,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      38,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    -1,    -1,    63,    -1,    -1,    66,    -1,
      68,     3,     4,     5,     6,    -1,    -1,     9,    10,    11,
      12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      22,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    -1,
      -1,    -1,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    -1,    -1,    -1,    -1,
      -1,    63,    -1,    -1,    66,    -1,    68,     3,     4,     5,
       6,    -1,    -1,     9,    10,    11,    12,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    22,    23,     3,     4,
       5,     6,    -1,    -1,     9,    10,    11,    12,    -1,    -1,
      -1,    -1,    38,    -1,    -1,    -1,    -1,    22,    23,    -1,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    -1,    38,    -1,    -1,    -1,    63,    -1,    -1,
      66,    -1,    68,    -1,    -1,    -1,    -1,    -1,    -1,    54,
      55,    56,    57,    -1,    -1,    -1,    -1,    -1,    63,    -1,
      -1,    66,    -1,    68,     3,     4,     5,     6,    -1,    -1,
       9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    22,    23,     3,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    -1,    -1,    -1,    -1,    38,
      -1,    -1,    -1,    -1,    22,    23,     3,     4,     5,     6,
      -1,    -1,     9,    10,    11,    12,    -1,    56,    57,    -1,
      38,    -1,    -1,    -1,    63,    22,    23,    66,    -1,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,
      -1,    38,    -1,    -1,    -1,    63,    -1,    -1,    66,    -1,
      68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    63,    -1,    -1,    66,
      -1,    68
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     8,     9,    10,    11,    12,
      15,    16,    17,    18,    19,    23,    28,    29,    30,    33,
      38,    62,    64,    66,    68,    72,    75,    76,    77,    78,
      79,    80,    81,    84,    85,    86,    88,    91,    94,    76,
       6,    23,    34,    35,    68,    70,    76,    83,    87,     6,
      23,    68,    82,    83,    23,    82,    76,    76,     6,     7,
       6,     7,     6,    95,    92,    76,     6,    43,    67,    87,
       6,    13,    17,    22,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    76,    79,
      87,    76,     0,    13,    22,    27,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      63,    66,    77,    59,    61,    24,    22,    57,    65,    65,
      65,     6,    68,    83,    87,     6,    87,    65,    65,    71,
      21,    87,    21,    83,    57,    65,    21,    21,    20,     6,
      65,     6,    65,    39,    40,    42,    93,    65,    43,    87,
      67,    71,    69,    69,    23,    82,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    61,    69,    69,    71,    36,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    65,    76,    76,    76,     6,    76,    76,
      83,     6,    14,    76,    76,    76,    65,    65,    69,    69,
      76,    76,    76,    79,    69,    79,    57,    65,    68,    76,
      79,    79,    43,    90,     6,    23,    43,    68,    96,    97,
      98,    99,    98,    76,    76,    67,    43,    71,    67,    21,
      21,    69,    76,    76,    67,    65,    17,    76,    76,    65,
      61,    61,    68,    76,    98,    61,    61,    76,    89,    43,
      57,    59,    65,    73,    99,    98,    21,    71,    21,    43,
      71,    41,    24,    67,    43,    79,    79,    37,    81,    96,
      98,    76,    81,    98,    69,    72,    21,    70,    89,    99,
       6,     3,    99,    69,    98,    99,    98,    99,    99,    76,
      67,    69,    69,    76,    69,    76,    76,    21,    21,    76,
      76
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 124 "lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (2)].ast_node_ptr)); }
    break;

  case 3:
#line 125 "lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 6:
#line 132 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_yield((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 7:
#line 133 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 8:
#line 134 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 9:
#line 135 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 10:
#line 136 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 11:
#line 137 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 12:
#line 138 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 13:
#line 139 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 14:
#line 140 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 15:
#line 141 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 16:
#line 142 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_AMP, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 17:
#line 143 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_PIPE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 18:
#line 144 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 19:
#line 145 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 20:
#line 146 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 21:
#line 147 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 22:
#line 148 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(3) - (3)].ast_node_ptr), (yyvsp[(1) - (3)].ast_node_ptr)); }
    break;

  case 23:
#line 149 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assoc((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 24:
#line 150 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_range_expression((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 25:
#line 151 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 26:
#line 152 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 27:
#line 153 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 28:
#line 154 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 29:
#line 155 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_thunk_expr((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 30:
#line 156 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_spread_operator((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 31:
#line 157 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_typed_empty_list((yyvsp[(1) - (1)].vident)); }
    break;

  case 32:
#line 158 "lang/parser.y"
    {
                                          Ast *let = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), (yyvsp[(4) - (6)].ast_node_ptr), (yyvsp[(6) - (6)].ast_node_ptr));
                                          let->tag = AST_LOOP;
                                          (yyval.ast_node_ptr) = let;

                                      }
    break;

  case 33:
#line 164 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application(ast_application(ast_identifier((ObjString){.chars = "array_at", 8}), (yyvsp[(1) - (4)].ast_node_ptr)), (yyvsp[(3) - (4)].ast_node_ptr)); }
    break;

  case 34:
#line 165 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assignment((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 36:
#line 170 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access((yyvsp[(1) - (3)].ast_node_ptr), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 37:
#line 174 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[(1) - (1)].vint)); }
    break;

  case 38:
#line 175 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[(1) - (1)].vdouble)); }
    break;

  case 39:
#line 176 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_FLOAT, (yyvsp[(1) - (1)].vfloat)); }
    break;

  case 40:
#line 177 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 41:
#line 178 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); }
    break;

  case 42:
#line 179 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); }
    break;

  case 43:
#line 180 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 44:
#line 181 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;

  case 45:
#line 182 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 46:
#line 183 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 47:
#line 184 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 48:
#line 185 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 49:
#line 186 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_char((yyvsp[(1) - (1)].vchar)); }
    break;

  case 50:
#line 187 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 51:
#line 188 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(3) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 52:
#line 189 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 53:
#line 190 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"+", 1}); }
    break;

  case 54:
#line 191 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"-", 1}); }
    break;

  case 55:
#line 192 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"*", 1}); }
    break;

  case 56:
#line 193 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"/", 1}); }
    break;

  case 57:
#line 194 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"%", 1}); }
    break;

  case 58:
#line 195 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<", 1}); }
    break;

  case 59:
#line 196 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){">", 1}); }
    break;

  case 60:
#line 197 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"&&", 2}); }
    break;

  case 61:
#line 198 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"||", 2}); }
    break;

  case 62:
#line 199 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){">=", 2}); }
    break;

  case 63:
#line 200 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<=", 2}); }
    break;

  case 64:
#line 201 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"!=", 2}); }
    break;

  case 65:
#line 202 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"==", 2}); }
    break;

  case 66:
#line 203 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"|", 1}); }
    break;

  case 67:
#line 204 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){":", 1}); }
    break;

  case 68:
#line 205 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"::", 2}); }
    break;

  case 69:
#line 206 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(2) - (3)].vident)); }
    break;

  case 70:
#line 211 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 71:
#line 212 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 72:
#line 216 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_test_module((yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 73:
#line 217 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 74:
#line 219 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), ast_extern_fn((yyvsp[(2) - (6)].vident), (yyvsp[(6) - (6)].ast_node_ptr)), NULL); }
    break;

  case 75:
#line 221 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 76:
#line 223 "lang/parser.y"
    { Ast *let = ast_let((yyvsp[(3) - (5)].ast_node_ptr), (yyvsp[(5) - (5)].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 77:
#line 227 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);}
    break;

  case 78:
#line 229 "lang/parser.y"
    { Ast *let = ast_let(ast_tuple((yyvsp[(3) - (5)].ast_node_ptr)), (yyvsp[(5) - (5)].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 79:
#line 237 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(4) - (4)].ast_node_ptr); }
    break;

  case 80:
#line 238 "lang/parser.y"
    {
                                      Ast *let = (yyvsp[(1) - (3)].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[(3) - (3)].ast_node_ptr);
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 81:
#line 243 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 82:
#line 246 "lang/parser.y"
    {
                                      Ast *id = ast_identifier((yyvsp[(3) - (6)].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[(6) - (6)].ast_node_ptr), NULL);
                                    }
    break;

  case 83:
#line 253 "lang/parser.y"
    {
                                      Ast *id = ast_identifier((yyvsp[(3) - (6)].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[(6) - (6)].ast_node_ptr), NULL);
                                    }
    break;

  case 84:
#line 262 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), false); }
    break;

  case 85:
#line 263 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), true); }
    break;

  case 86:
#line 264 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), false); }
    break;

  case 87:
#line 265 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), true); }
    break;

  case 88:
#line 267 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_trait_impl((yyvsp[(2) - (6)].vident), (yyvsp[(4) - (6)].vident), (yyvsp[(6) - (6)].ast_node_ptr)); }
    break;

  case 89:
#line 273 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 90:
#line 274 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 91:
#line 275 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_module(ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr))); }
    break;

  case 92:
#line 276 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_module(ast_lambda(NULL, (yyvsp[(4) - (5)].ast_node_ptr))); }
    break;

  case 93:
#line 283 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 94:
#line 284 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 95:
#line 285 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 96:
#line 286 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 97:
#line 287 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 98:
#line 288 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (6)].ast_node_ptr), (yyvsp[(2) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 99:
#line 292 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 100:
#line 293 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 101:
#line 294 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 102:
#line 295 "lang/parser.y"
    { (yyval.ast_node_ptr) = Ast_new(AST_PLACEHOLDER_ID); }
    break;

  case 103:
#line 300 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 104:
#line 301 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 105:
#line 302 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (4)].ast_node_ptr); }
    break;

  case 106:
#line 306 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_array(); }
    break;

  case 107:
#line 307 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (5)].ast_node_ptr)); }
    break;

  case 108:
#line 308 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (6)].ast_node_ptr)); }
    break;

  case 109:
#line 313 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 110:
#line 314 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 111:
#line 315 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)); }
    break;

  case 112:
#line 319 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 113:
#line 320 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 114:
#line 324 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 115:
#line 325 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_if_else((yyvsp[(2) - (6)].ast_node_ptr), (yyvsp[(4) - (6)].ast_node_ptr) ,(yyvsp[(6) - (6)].ast_node_ptr));}
    break;

  case 116:
#line 326 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_if_else((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr), NULL);}
    break;

  case 117:
#line 330 "lang/parser.y"
    {(yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr);}
    break;

  case 118:
#line 331 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr));}
    break;

  case 119:
#line 334 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 120:
#line 335 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(3) - (5)].ast_node_ptr), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 121:
#line 336 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 122:
#line 339 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 123:
#line 343 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 124:
#line 344 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 125:
#line 348 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 126:
#line 349 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 127:
#line 353 "lang/parser.y"
    {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
    break;

  case 128:
#line 359 "lang/parser.y"
    {
                                      Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (2)].vident)), NULL, NULL);
                                      type_decl->tag = AST_TYPE_DECL;
                                      (yyval.ast_node_ptr) = type_decl;
                                   }
    break;

  case 129:
#line 365 "lang/parser.y"
    {
                                    Ast *args = (yyvsp[(2) - (4)].ast_node_ptr);
                                    Ast *name = args->data.AST_LAMBDA.params;
                                    args->data.AST_LAMBDA.params = args->data.AST_LAMBDA.params + 1;
                                    args->data.AST_LAMBDA.len--;
                                    args->data.AST_LAMBDA.body = (yyvsp[(4) - (4)].ast_node_ptr);
                                    Ast *type_decl = ast_let(name, args, NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
    break;

  case 130:
#line 378 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push(ast_arg_list(ast_identifier((yyvsp[(1) - (2)].vident)), NULL), ast_identifier((yyvsp[(2) - (2)].vident)), NULL); }
    break;

  case 131:
#line 379 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), ast_identifier((yyvsp[(2) - (2)].vident)), NULL); }
    break;

  case 132:
#line 382 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 133:
#line 383 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 134:
#line 387 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 135:
#line 388 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 136:
#line 392 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 137:
#line 393 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 138:
#line 394 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 139:
#line 395 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_signature_of_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 140:
#line 396 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 141:
#line 400 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 142:
#line 401 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(1) - (3)].vident)), AST_CONST(AST_INT, (yyvsp[(3) - (3)].vint)), NULL); }
    break;

  case 143:
#line 402 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 144:
#line 403 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assoc(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 145:
#line 404 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 146:
#line 405 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;

  case 147:
#line 406 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access(ast_identifier((yyvsp[(1) - (3)].vident)), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;


/* Line 1267 of yacc.c.  */
#line 2888 "lang/y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 408 "lang/parser.y"



void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, parsing_context.cur_script);
}
#endif _LANG_TAB_H

