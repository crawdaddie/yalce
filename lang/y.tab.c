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
     IDENTIFIER = 260,
     MACRO_IDENTIFIER = 261,
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
     MATCH = 273,
     WITH = 274,
     ARROW = 275,
     DOUBLE_COLON = 276,
     TOK_VOID = 277,
     IN = 278,
     AND = 279,
     ASYNC = 280,
     DOUBLE_AT = 281,
     THUNK = 282,
     IMPORT = 283,
     OPEN = 284,
     IMPLEMENTS = 285,
     AMPERSAND = 286,
     TYPE = 287,
     TEST_ID = 288,
     MUT = 289,
     THEN = 290,
     ELSE = 291,
     FSTRING_START = 292,
     FSTRING_END = 293,
     FSTRING_INTERP_START = 294,
     FSTRING_INTERP_END = 295,
     FSTRING_TEXT = 296,
     APPLICATION = 297,
     DOUBLE_PIPE = 298,
     DOUBLE_AMP = 299,
     NE = 300,
     EQ = 301,
     LE = 302,
     GE = 303,
     MODULO = 304,
     UMINUS = 305
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define DOUBLE 259
#define IDENTIFIER 260
#define MACRO_IDENTIFIER 261
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
#define MATCH 273
#define WITH 274
#define ARROW 275
#define DOUBLE_COLON 276
#define TOK_VOID 277
#define IN 278
#define AND 279
#define ASYNC 280
#define DOUBLE_AT 281
#define THUNK 282
#define IMPORT 283
#define OPEN 284
#define IMPLEMENTS 285
#define AMPERSAND 286
#define TYPE 287
#define TEST_ID 288
#define MUT 289
#define THEN 290
#define ELSE 291
#define FSTRING_START 292
#define FSTRING_END 293
#define FSTRING_INTERP_START 294
#define FSTRING_INTERP_END 295
#define FSTRING_TEXT 296
#define APPLICATION 297
#define DOUBLE_PIPE 298
#define DOUBLE_AMP 299
#define NE 300
#define EQ 301
#define LE 302
#define GE 303
#define MODULO 304
#define UMINUS 305




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
    char vchar;
}
/* Line 193 of yacc.c.  */
#line 234 "lang/y.tab.c"
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
#line 259 "lang/y.tab.c"

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
#define YYFINAL  95
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2243

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  74
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  151
/* YYNRULES -- Number of states.  */
#define YYNSTATES  327

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   305

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      65,    66,    54,    52,    70,    53,    58,    55,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    57,    60,
      47,    64,    46,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    68,     2,    69,     2,    72,     2,     2,     2,     2,
       2,     2,    63,     2,     2,    71,     2,     2,     2,    67,
       2,    73,     2,     2,     2,     2,    62,     2,     2,     2,
       2,    61,     2,     2,    42,     2,     2,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    43,    44,    45,
      48,    49,    50,    51,    56,    59
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    11,    14,    18,    21,
      25,    29,    33,    37,    41,    45,    49,    53,    57,    61,
      65,    69,    73,    77,    81,    85,    89,    91,    93,    95,
      98,   101,   103,   106,   113,   118,   120,   122,   124,   126,
     128,   130,   132,   134,   136,   138,   140,   142,   146,   153,
     160,   164,   168,   172,   176,   180,   184,   188,   192,   196,
     200,   204,   208,   212,   216,   220,   224,   228,   232,   234,
     236,   240,   245,   250,   257,   262,   268,   273,   279,   284,
     288,   290,   297,   304,   307,   310,   313,   316,   323,   329,
     335,   342,   349,   355,   359,   361,   365,   368,   373,   375,
     381,   384,   391,   393,   397,   399,   402,   406,   411,   416,
     422,   429,   433,   437,   441,   445,   450,   452,   456,   461,
     468,   473,   475,   479,   484,   490,   496,   500,   501,   504,
     506,   510,   515,   518,   523,   525,   528,   532,   536,   540,
     544,   546,   549,   553,   555,   557,   559,   563,   567,   571,
     575,   577
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      75,     0,    -1,    79,    60,    -1,    79,    -1,    -1,    77,
      -1,    61,    76,    -1,    76,    26,    76,    -1,    76,    77,
      -1,    76,    52,    76,    -1,    76,    53,    76,    -1,    76,
      54,    76,    -1,    76,    55,    76,    -1,    76,    56,    76,
      -1,    76,    47,    76,    -1,    76,    46,    76,    -1,    76,
      45,    76,    -1,    76,    44,    76,    -1,    76,    51,    76,
      -1,    76,    50,    76,    -1,    76,    48,    76,    -1,    76,
      49,    76,    -1,    76,    13,    76,    -1,    76,    57,    76,
      -1,    76,    62,    76,    -1,    76,    21,    76,    -1,    80,
      -1,    89,    -1,    95,    -1,    27,    76,    -1,    15,    76,
      -1,     8,    -1,     6,    76,    -1,    63,     5,    64,    76,
      23,    76,    -1,    76,    57,    64,    76,    -1,     3,    -1,
       4,    -1,     9,    -1,    11,    -1,    12,    -1,     5,    -1,
      22,    -1,    84,    -1,    85,    -1,    87,    -1,    92,    -1,
      10,    -1,    65,    79,    66,    -1,    65,    17,    82,    20,
      79,    66,    -1,    65,    17,    22,    20,    79,    66,    -1,
      65,    52,    66,    -1,    65,    53,    66,    -1,    65,    54,
      66,    -1,    65,    55,    66,    -1,    65,    56,    66,    -1,
      65,    47,    66,    -1,    65,    46,    66,    -1,    65,    45,
      66,    -1,    65,    44,    66,    -1,    65,    51,    66,    -1,
      65,    50,    66,    -1,    65,    48,    66,    -1,    65,    49,
      66,    -1,    65,    13,    66,    -1,    65,    57,    66,    -1,
      65,    21,    66,    -1,    65,    78,    66,    -1,    77,    58,
       5,    -1,     5,    -1,    76,    -1,    79,    60,    76,    -1,
      16,    33,    64,    76,    -1,    16,     5,    64,    76,    -1,
      16,     5,    64,    14,    17,    97,    -1,    16,    83,    64,
      76,    -1,    16,    34,    83,    64,    76,    -1,    16,    88,
      64,    76,    -1,    16,    34,    88,    64,    76,    -1,    16,
      22,    64,    76,    -1,    80,    23,    76,    -1,    81,    -1,
      16,    65,     5,    66,    64,    81,    -1,    16,    65,     5,
      66,    64,    76,    -1,    28,     7,    -1,    29,     7,    -1,
      28,     5,    -1,    29,     5,    -1,    16,     5,    57,     5,
      64,    81,    -1,    17,    82,    20,    79,    60,    -1,    17,
      22,    20,    79,    60,    -1,    65,    17,    82,    20,    79,
      66,    -1,    65,    17,    22,    20,    79,    66,    -1,    67,
      82,    20,    79,    60,    -1,    67,    79,    60,    -1,    83,
      -1,    83,    64,    76,    -1,    82,    83,    -1,    82,    83,
      64,    76,    -1,    83,    -1,    83,    57,    65,    99,    66,
      -1,    82,    83,    -1,    82,    83,    57,    65,    99,    66,
      -1,     5,    -1,    65,    88,    66,    -1,    86,    -1,    68,
      69,    -1,    68,    88,    69,    -1,    68,    88,    70,    69,
      -1,    68,    42,    42,    69,    -1,    68,    42,    88,    42,
      69,    -1,    68,    42,    88,    70,    42,    69,    -1,     5,
      21,     5,    -1,     5,    21,    76,    -1,    65,    76,    66,
      -1,    65,    88,    66,    -1,    65,    88,    70,    66,    -1,
      76,    -1,    88,    70,    76,    -1,    18,    76,    19,    91,
      -1,    71,    76,    35,    76,    36,    76,    -1,    71,    76,
      35,    76,    -1,    76,    -1,    76,    71,    76,    -1,    42,
      90,    20,    76,    -1,    91,    42,    90,    20,    76,    -1,
      91,    42,    72,    20,    76,    -1,    37,    93,    38,    -1,
      -1,    93,    94,    -1,    41,    -1,    39,    76,    40,    -1,
      32,     5,    64,    99,    -1,    32,     5,    -1,    32,    96,
      64,    99,    -1,     5,    -1,    96,     5,    -1,    99,    20,
      99,    -1,    97,    20,    99,    -1,   100,    70,   100,    -1,
      98,    70,   100,    -1,   100,    -1,    42,   100,    -1,    99,
      42,   100,    -1,    97,    -1,    98,    -1,     5,    -1,     5,
      64,     3,    -1,     5,    73,   100,    -1,     5,    57,   100,
      -1,    65,    99,    66,    -1,    22,    -1,     5,    58,     5,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   124,   124,   125,   126,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   163,   172,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,   207,   208,   212,   235,
     236,   240,   241,   242,   245,   247,   251,   253,   261,   262,
     267,   269,   276,   286,   287,   288,   289,   291,   303,   304,
     305,   306,   307,   308,   315,   316,   317,   318,   320,   321,
     322,   323,   327,   328,   329,   339,   340,   341,   345,   346,
     347,   351,   352,   356,   357,   358,   362,   363,   367,   368,
     369,   373,   374,   377,   378,   379,   382,   386,   387,   391,
     392,   396,   402,   408,   421,   422,   425,   426,   430,   431,
     435,   436,   437,   438,   439,   443,   444,   445,   446,   447,
     448,   449
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INTEGER", "DOUBLE", "IDENTIFIER",
  "MACRO_IDENTIFIER", "PATH_IDENTIFIER", "IDENTIFIER_LIST", "TOK_STRING",
  "TOK_CHAR", "TRUE", "FALSE", "PIPE", "EXTERN", "TRIPLE_DOT", "LET", "FN",
  "MATCH", "WITH", "ARROW", "DOUBLE_COLON", "TOK_VOID", "IN", "AND",
  "ASYNC", "DOUBLE_AT", "THUNK", "IMPORT", "OPEN", "IMPLEMENTS",
  "AMPERSAND", "TYPE", "TEST_ID", "MUT", "THEN", "ELSE", "FSTRING_START",
  "FSTRING_END", "FSTRING_INTERP_START", "FSTRING_INTERP_END",
  "FSTRING_TEXT", "'|'", "APPLICATION", "DOUBLE_PIPE", "DOUBLE_AMP", "'>'",
  "'<'", "NE", "EQ", "LE", "GE", "'+'", "'-'", "'*'", "'/'", "MODULO",
  "':'", "'.'", "UMINUS", "';'", "'yield'", "'to'", "'for'", "'='", "'('",
  "')'", "'module'", "'['", "']'", "','", "'if'", "'_'", "'of'", "$accept",
  "program", "expr", "simple_expr", "custom_binop", "expr_sequence",
  "let_binding", "lambda_expr", "lambda_args", "lambda_arg", "list",
  "array", "list_match_expr", "tuple", "expr_list", "match_expr",
  "match_test_clause", "match_branches", "fstring", "fstring_parts",
  "fstring_part", "type_decl", "type_args", "fn_signature", "tuple_type",
  "type_expr", "type_atom", 0
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
     295,   296,   124,   297,   298,   299,    62,    60,   300,   301,
     302,   303,    43,    45,    42,    47,   304,    58,    46,   305,
      59,   121,   116,   102,    61,    40,    41,   109,    91,    93,
      44,   105,    95,   111
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    74,    75,    75,    75,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    78,    79,
      79,    80,    80,    80,    80,    80,    80,    80,    80,    80,
      80,    80,    80,    80,    80,    80,    80,    80,    81,    81,
      81,    81,    81,    81,    82,    82,    82,    82,    82,    82,
      82,    82,    83,    83,    83,    84,    84,    84,    85,    85,
      85,    86,    86,    87,    87,    87,    88,    88,    89,    89,
      89,    90,    90,    91,    91,    91,    92,    93,    93,    94,
      94,    95,    95,    95,    96,    96,    97,    97,    98,    98,
      99,    99,    99,    99,    99,   100,   100,   100,   100,   100,
     100,   100
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     2,     3,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     1,     2,
       2,     1,     2,     6,     4,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     6,     6,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     1,
       3,     4,     4,     6,     4,     5,     4,     5,     4,     3,
       1,     6,     6,     2,     2,     2,     2,     6,     5,     5,
       6,     6,     5,     3,     1,     3,     2,     4,     1,     5,
       2,     6,     1,     3,     1,     2,     3,     4,     4,     5,
       6,     3,     3,     3,     3,     4,     1,     3,     4,     6,
       4,     1,     3,     4,     5,     5,     3,     0,     2,     1,
       3,     4,     2,     4,     1,     2,     3,     3,     3,     3,
       1,     2,     3,     1,     1,     1,     3,     3,     3,     3,
       1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    35,    36,    40,     0,    31,    37,    46,    38,    39,
       0,     0,     0,     0,    41,     0,     0,     0,     0,   127,
       0,     0,     0,     0,     0,     0,     0,    69,     5,     3,
      26,    80,    42,    43,    44,    27,    45,    28,    32,    30,
      40,    41,     0,     0,     0,   116,     0,   104,     0,   102,
       0,     0,     0,    94,     0,    29,    85,    83,    86,    84,
     132,     0,     0,     6,     0,    40,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    69,     0,     0,     0,    40,     0,     0,
       0,     0,   105,     0,     0,     1,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     0,     2,     0,     0,
       0,     0,     0,     0,     0,     0,    40,     0,     0,     0,
       0,     0,     0,     0,    96,     0,     0,     0,     0,   135,
       0,   126,     0,   129,   128,     0,    63,     0,     0,    65,
      58,    57,    56,    55,    61,    62,    60,    59,    50,    51,
      52,    53,    54,    64,   113,    66,     0,    47,   114,     0,
      93,     0,     0,     0,   106,     0,     0,    22,    25,     7,
      17,    16,    15,    14,    20,    21,    19,    18,     9,    10,
      11,    12,    13,     0,    23,    24,     0,    67,    70,    79,
      40,   112,     0,     0,    72,    78,    71,     0,     0,     0,
     114,    74,    76,   117,     0,   103,     0,     0,     0,     0,
      95,     0,   118,   145,   150,     0,     0,   143,   144,   131,
     140,   133,     0,     0,     0,     0,   115,     0,   108,     0,
       0,   107,   120,    34,     0,     0,     0,     0,    75,    77,
       0,    89,    88,     0,    97,     0,   121,     0,     0,     0,
       0,     0,     0,   141,     0,     0,     0,     0,     0,     0,
     130,     0,     0,     0,    92,   109,     0,     0,     0,     0,
       0,    87,    73,     0,    82,    80,     0,    99,     0,     0,
       0,     0,   148,   151,   146,   147,   149,   137,   139,   136,
     142,   138,    33,    49,    48,   110,   119,     0,     0,     0,
     101,   122,   123,     0,     0,    49,    48,     0,     0,   125,
     124,     0,     0,     0,     0,    91,    90
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    26,    27,    28,    84,    85,    30,    31,    52,    53,
      32,    33,    47,    34,    86,    35,   257,   222,    36,    62,
     144,    37,    61,   227,   228,   229,   230
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -71
static const yytype_int16 yypact[] =
{
    1304,   -71,   -71,   -71,  1304,   -71,   -71,   -71,   -71,   -71,
    1304,   752,     3,  1304,   -71,  1304,    39,   138,    51,   -71,
    1304,    59,   476,  1373,   821,  1304,    74,  1876,    22,    30,
      75,   -71,   -71,   -71,   -71,   -71,   -71,   -71,  1876,  1876,
     123,    66,    76,  1373,   545,  1876,    91,   -71,   -10,   144,
     173,  1304,     4,   -53,  1546,  1876,   -71,   -71,   -71,   -71,
     132,     1,   169,  1876,   134,   -71,   140,     7,   149,   150,
     155,   162,   165,   171,   172,   175,   179,   181,   186,   187,
     188,   189,   193,  1478,   196,    13,    29,   182,   476,   152,
      17,   890,   -71,   148,  1612,   -71,  1304,  1304,  1304,  1304,
    1304,  1304,  1304,  1304,  1304,  1304,  1304,  1304,  1304,  1304,
    1304,  1304,   959,  1304,   614,    22,   234,  1304,  1304,  1442,
     258,  1028,  1304,  1304,   201,    68,   200,    84,  1304,  1304,
    1304,  1304,   120,  1304,    14,   202,  1304,   226,    10,   -71,
      10,   -71,  1304,   -71,   -71,  1304,   -71,   249,    28,   -71,
     -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71,
     -71,   -71,   -71,   -71,   -71,   -71,  1304,   -71,   -71,  1097,
    1304,  1304,   203,   -28,   -71,  1166,  1304,  1942,  1876,  2008,
    2074,  2074,  2095,  2095,  2095,  2095,  2095,  2095,  2161,  2161,
    2175,  2175,   239,  1304,   157,  1876,    31,   -71,  1876,  1876,
     -71,  1876,   206,   254,  1876,  1876,  1876,  1304,  1304,   209,
      -2,  1876,  1876,  1876,   214,   -71,   215,   216,  1304,    10,
    1876,  1304,   240,   147,   -71,    36,    10,   264,   218,    47,
     219,    47,  1678,  1744,  1304,  1304,   -71,   225,   -71,   217,
    1235,   -71,  1810,  1876,   270,    56,    21,    10,  1876,  1876,
    1304,  1304,  1304,    10,  1876,   -15,   683,   271,   406,    36,
     287,   290,    36,   -71,    -7,    10,    36,    10,    36,    36,
     -71,  1304,    25,    81,  1304,   -71,   228,  1304,  1304,  1304,
     281,   -71,   264,    47,  1876,   -71,    80,   -71,  1304,  1304,
     279,   280,   -71,   -71,   -71,   -71,   -71,    47,   -71,    47,
     -71,   -71,  1876,   -71,   -71,   -71,  1876,    92,    93,    61,
     -71,  1876,  1876,  1304,  1304,   -71,   -71,   282,    72,  1876,
    1876,  1304,  1304,    97,   104,   -71,   -71
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -71,   -71,     6,   146,   -71,     0,   -71,   -58,   -22,    -9,
     -71,   -71,   -71,   -71,    -4,   -71,    45,   -71,   -71,   -71,
     -71,   -71,   -71,    58,   -71,   -70,   -36
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -117
static const yytype_int16 yytable[] =
{
      29,    90,    46,  -103,   135,   267,   139,    48,    49,    49,
      38,   136,    49,   267,   239,   223,    39,    45,  -103,    54,
      93,    55,    49,    89,   133,    50,    63,   268,    83,   147,
      45,    94,   224,    49,   124,   268,    49,   171,    12,   125,
     127,   223,   240,   134,    56,   148,    57,   132,   235,    45,
      83,   287,   225,   244,   129,  -103,    60,    45,   224,   296,
     130,    49,  -103,  -103,    64,   140,    49,   267,    51,    51,
     231,   217,    51,   166,    95,   226,   279,    49,   218,   167,
     116,   134,    51,   317,   127,   251,   280,   173,    23,   268,
     117,   303,   322,    51,    83,   168,    51,    45,   118,   169,
     267,   226,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   194,   195,
      83,    51,   268,   198,   199,   201,    51,   204,   205,   206,
     122,   214,   208,   216,   211,   212,   213,    51,   130,   134,
     123,   252,   220,    58,   119,    59,   310,   304,   232,   255,
     210,   233,   251,   252,   169,   128,   264,   166,   315,   316,
       1,     2,     3,   325,   166,   119,     6,     7,     8,     9,
     326,   237,   198,   115,   245,   213,   198,   283,    97,    14,
     120,   213,   242,   286,   115,   115,   215,   121,   281,   263,
     130,   115,   285,   131,    19,   297,   138,   299,   145,   243,
     115,   115,  -102,   119,   259,   260,   146,   141,   142,   115,
     143,   261,   170,   248,   249,   149,   150,   174,   175,   113,
     262,   151,   114,   292,   254,    24,   295,   256,   152,   115,
     298,   153,   300,   301,   272,   273,   134,   154,   155,   197,
     115,   156,     1,     2,     3,   157,   213,   158,     6,     7,
       8,     9,   159,   160,   161,   162,   284,   198,   198,   163,
      97,    14,   165,   202,   256,   207,   209,   219,   221,   234,
     246,   247,   238,   250,   251,   252,    19,   302,   307,   308,
     198,   253,   258,   306,   265,   274,   275,   318,   266,   269,
     278,   289,   293,   294,   311,   312,   112,   305,   309,   313,
     314,   113,   321,   291,   114,   282,     0,    24,     0,   134,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   319,
     320,   323,   324,   115,   115,   115,   115,   115,   115,   115,
     115,   115,   115,   115,   115,   115,   115,   115,   115,     0,
     115,   115,     0,     0,   115,   115,     0,   115,     0,     0,
     115,   115,   115,     0,     0,     0,     0,   115,   115,   115,
       0,     0,     0,     0,     0,     0,   115,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   115,   115,
       0,     0,     0,     0,     0,     0,     0,     0,   115,   115,
       0,     0,     0,     0,   115,   115,     0,     0,     0,     0,
     115,     0,   115,     0,     0,     0,     0,     0,     0,     1,
       2,     3,     4,     0,     5,     6,     7,     8,     9,     0,
       0,    10,    11,    12,    13,     0,     0,     0,    14,     0,
     115,     0,     0,    15,    16,    17,     0,     0,    18,     0,
       0,     0,     0,    19,     0,     0,     0,     0,   115,     0,
       0,     0,   115,     0,     0,     0,     0,   115,   115,     0,
       0,     0,     0,     0,     0,   115,   115,    20,     0,    21,
       0,    22,     0,    23,    24,     0,     0,    25,   290,     1,
       2,    65,     4,     0,     5,     6,     7,     8,     9,    66,
       0,    10,    11,    67,    13,     0,     0,    68,    14,     0,
       0,     0,     0,    15,    16,    17,     0,     0,    18,     0,
       0,     0,     0,    19,     0,     0,     0,     0,     0,     0,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,     0,     0,     0,    20,     0,    21,
       0,    22,     0,    23,    24,     0,     0,    25,     1,     2,
     126,     4,     0,     5,     6,     7,     8,     9,    66,     0,
      10,    11,    67,    13,     0,     0,    68,    14,     0,     0,
       0,     0,    15,    16,    17,     0,     0,    18,     0,     0,
       0,     0,    19,     0,     0,     0,     0,     0,     0,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,     0,     0,     0,    20,     0,    21,     0,
      22,     0,    23,    24,     0,     0,    25,     1,     2,    65,
       4,     0,     5,     6,     7,     8,     9,    66,     0,    10,
      11,   196,    13,     0,     0,    68,    14,     0,     0,     0,
       0,    15,    16,    17,     0,     0,    18,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,     0,     0,     0,    20,     0,    21,     0,    22,
       0,    23,    24,     0,     0,    25,     1,     2,     3,     0,
       0,     0,     6,     7,     8,     9,    96,     0,     0,     0,
       0,     0,     0,     0,    97,    14,     0,     0,     0,    98,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      19,     0,     0,     0,     0,     0,     0,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,     0,     0,     0,     0,   113,     0,     0,   114,     0,
       0,    24,     0,     0,   288,     1,     2,    40,     4,     0,
       5,     6,     7,     8,     9,     0,     0,    10,    11,    12,
      13,     0,     0,     0,    41,     0,     0,     0,     0,    15,
      16,    17,     0,     0,    18,    42,    43,     0,     0,    19,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    20,     0,    21,     0,    44,     0,    23,
      24,     0,     0,    25,     1,     2,     3,     4,     0,     5,
       6,     7,     8,     9,     0,     0,    10,    11,    12,    13,
       0,     0,     0,    14,     0,     0,     0,     0,    15,    16,
      17,     0,     0,    18,     0,     0,     0,     0,    19,     0,
       0,     0,     0,    91,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    20,     0,    21,     0,    22,     0,    23,    24,
      92,     0,    25,     1,     2,     3,     4,     0,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,     0,
       0,     0,    14,     0,     0,     0,     0,    15,    16,    17,
       0,     0,    18,     0,     0,     0,     0,    19,     0,     0,
       0,     0,   172,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    20,     0,    21,     0,    22,     0,    23,    24,     0,
       0,    25,     1,     2,     3,     4,     0,     5,     6,     7,
       8,     9,     0,     0,    10,    11,    12,    13,     0,     0,
       0,    14,     0,     0,     0,     0,    15,    16,    17,     0,
       0,    18,     0,     0,     0,     0,    19,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      20,     0,    21,   193,    22,     0,    23,    24,     0,     0,
      25,     1,     2,     3,     4,     0,     5,     6,     7,     8,
       9,     0,   203,    10,    11,    12,    13,     0,     0,     0,
      14,     0,     0,     0,     0,    15,    16,    17,     0,     0,
      18,     0,     0,     0,     0,    19,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    20,
       0,    21,     0,    22,     0,    23,    24,     0,     0,    25,
       1,     2,     3,     4,     0,     5,     6,     7,     8,     9,
       0,     0,    10,    11,    12,    13,     0,     0,     0,    14,
       0,     0,     0,     0,    15,    16,    17,     0,     0,    18,
       0,     0,     0,     0,    19,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    20,     0,
      21,     0,    22,   236,    23,    24,     0,     0,    25,     1,
       2,     3,     4,     0,     5,     6,     7,     8,     9,     0,
       0,    10,    11,    12,    13,     0,     0,     0,    14,     0,
       0,     0,     0,    15,    16,    17,     0,     0,    18,     0,
       0,     0,     0,    19,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    20,     0,    21,
       0,    22,     0,    23,    24,   241,     0,    25,     1,     2,
       3,     4,     0,     5,     6,     7,     8,     9,     0,     0,
      10,    11,    12,    13,     0,     0,     0,    14,     0,     0,
       0,     0,    15,    16,    17,     0,     0,    18,     0,     0,
       0,     0,    19,     0,     0,     0,     0,   276,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    20,     0,    21,     0,
      22,     0,    23,    24,     0,     0,    25,     1,     2,     3,
       4,     0,     5,     6,     7,     8,     9,     0,     0,    10,
      11,    12,    13,     0,     0,     0,    14,     0,     0,     0,
       0,    15,    16,    17,     0,     0,    18,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    20,     0,    21,     0,    22,
       0,    23,    24,     0,     0,    25,     1,     2,    87,     4,
       0,     5,     6,     7,     8,     9,     0,     0,    10,    11,
      12,    13,     0,     0,     0,    14,     0,     0,     0,     0,
      15,    16,    17,     0,     0,    18,     0,     0,     0,     0,
      19,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    20,     0,    21,     0,    88,     0,
      23,    24,     0,     0,    25,     1,     2,   200,     4,     0,
       5,     6,     7,     8,     9,     0,     0,    10,    11,    12,
      13,     0,     0,     0,    14,     0,     0,     0,     0,    15,
      16,    17,     0,     0,    18,     0,     0,     0,     0,    19,
       0,     1,     2,     3,     0,     0,     0,     6,     7,     8,
       9,    96,     0,     0,     0,     0,     0,     0,     0,    97,
      14,     0,     0,    20,    98,    21,     0,    22,     0,    23,
      24,     0,     0,    25,     0,    19,     0,     0,     0,     0,
       0,     0,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,     0,     0,     0,     0,
     113,     0,     0,   114,   164,     0,    24,     0,  -116,     1,
       2,     3,     0,     0,     0,     6,     7,     8,     9,    96,
       0,     0,     0,     0,     0,   137,     0,    97,    14,     0,
       0,     0,    98,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    19,     0,     0,     0,     0,     0,     0,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,     0,     0,     0,     0,   113,     0,
       0,   114,     0,     0,    24,     1,     2,     3,     0,     0,
       0,     6,     7,     8,     9,    96,     0,     0,     0,     0,
       0,     0,     0,    97,    14,     0,     0,     0,    98,     0,
       0,     0,     0,     0,     0,     0,     0,   176,     0,    19,
       0,     0,     0,     0,     0,     0,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
       0,     0,     0,     0,   113,     0,     0,   114,     0,     0,
      24,     1,     2,     3,     0,     0,     0,     6,     7,     8,
       9,    96,     0,     0,     0,     0,     0,     0,     0,    97,
      14,     0,     0,     0,    98,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    19,     0,     0,   270,     0,
       0,     0,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,     0,     0,     0,     0,
     113,     0,     0,   114,     0,     0,    24,     1,     2,     3,
       0,     0,     0,     6,     7,     8,     9,    96,     0,     0,
       0,     0,     0,     0,     0,    97,    14,   271,     0,     0,
      98,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,     0,     0,     0,     0,   113,     0,     0,   114,
       0,     0,    24,     1,     2,     3,     0,     0,     0,     6,
       7,     8,     9,    96,     0,     0,     0,     0,     0,     0,
       0,    97,    14,     0,     0,     0,    98,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   277,    19,     0,     0,
       0,     0,     0,     0,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,     0,     0,
       0,     0,   113,     0,     0,   114,     0,     0,    24,     1,
       2,     3,     0,     0,     0,     6,     7,     8,     9,    96,
       0,     0,     0,     0,     0,     0,     0,    97,    14,     0,
       0,     0,    98,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    19,     0,     0,     0,     0,     0,     0,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,     0,     0,     0,     0,   113,     0,
       0,   114,     0,     0,    24,     1,     2,     3,     0,     0,
       0,     6,     7,     8,     9,     0,     0,     0,     0,     0,
       0,     0,     0,    97,    14,     0,     0,     0,    98,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    19,
       0,     0,     0,     0,     0,     0,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
       0,     0,     0,     0,   113,     0,     0,   114,     0,     0,
      24,     1,     2,     3,     0,     0,     0,     6,     7,     8,
       9,     0,     0,     0,     0,     0,     0,     0,     0,    97,
      14,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    19,     0,     0,     0,     0,
       0,     0,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,     0,     0,     0,     0,
     113,     0,     0,   114,     0,     0,    24,     1,     2,     3,
       0,     0,     0,     6,     7,     8,     9,     0,     0,     0,
       0,     0,     0,     0,     0,    97,    14,     0,     1,     2,
       3,     0,     0,     0,     6,     7,     8,     9,     0,     0,
       0,    19,     0,     0,     0,     0,    97,    14,     0,     0,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,    19,     0,     0,     0,   113,     0,     0,   114,
       0,     0,    24,     0,     0,     0,     0,   107,   108,   109,
     110,   111,   112,     0,     0,     0,     0,   113,     0,     0,
     114,     0,     0,    24,     1,     2,     3,     0,     0,     0,
       6,     7,     8,     9,     0,     0,     0,     0,     1,     2,
       3,     0,    97,    14,     6,     7,     8,     9,     0,     0,
       0,     0,     0,     0,     0,     0,    97,    14,    19,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    19,     0,     0,   109,   110,   111,   112,     0,
       0,     0,     0,   113,     0,     0,   114,     0,     0,    24,
       0,   111,   112,     0,     0,     0,     0,   113,     0,     0,
     114,     0,     0,    24
};

static const yytype_int16 yycheck[] =
{
       0,    23,    11,     5,    57,    20,     5,    11,     5,     5,
       4,    64,     5,    20,    42,     5,    10,    11,    20,    13,
      24,    15,     5,    23,    20,    22,    20,    42,    22,    22,
      24,    25,    22,     5,    43,    42,     5,    20,    17,    43,
      44,     5,    70,    52,     5,    67,     7,    51,    20,    43,
      44,    66,    42,    22,    64,    57,     5,    51,    22,    66,
      70,     5,    64,    65,     5,    64,     5,    20,    65,    65,
     140,    57,    65,    60,     0,    65,    20,     5,    64,    66,
      58,    90,    65,    22,    88,    60,    65,    91,    67,    42,
      60,    66,    20,    65,    88,    66,    65,    91,    23,    70,
      20,    65,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,    65,    42,   117,   118,   119,    65,   121,   122,   123,
      64,   131,    64,   133,   128,   129,   130,    65,    70,   148,
      64,    60,   136,     5,    21,     7,    66,    66,   142,   219,
      66,   145,    60,    60,    70,    64,   226,    60,    66,    66,
       3,     4,     5,    66,    60,    21,     9,    10,    11,    12,
      66,   171,   166,    27,   196,   169,   170,   247,    21,    22,
      57,   175,   176,   253,    38,    39,    66,    64,   246,   225,
      70,    45,   250,    20,    37,   265,    64,   267,    64,   193,
      54,    55,    20,    21,    57,    58,    66,    38,    39,    63,
      41,    64,    60,   207,   208,    66,    66,    69,    70,    62,
      73,    66,    65,   259,   218,    68,   262,   221,    66,    83,
     266,    66,   268,   269,   234,   235,   245,    66,    66,     5,
      94,    66,     3,     4,     5,    66,   240,    66,     9,    10,
      11,    12,    66,    66,    66,    66,   250,   251,   252,    66,
      21,    22,    66,     5,   258,    64,    66,    65,    42,    20,
      64,    17,    69,    64,    60,    60,    37,   271,   278,   279,
     274,    65,    42,   277,    20,    60,    69,   309,    70,    70,
      20,    20,     5,     3,   288,   289,    57,    69,    17,    20,
      20,    62,    20,   258,    65,   247,    -1,    68,    -1,   318,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   313,
     314,   321,   322,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,    -1,
     194,   195,    -1,    -1,   198,   199,    -1,   201,    -1,    -1,
     204,   205,   206,    -1,    -1,    -1,    -1,   211,   212,   213,
      -1,    -1,    -1,    -1,    -1,    -1,   220,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   232,   233,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   242,   243,
      -1,    -1,    -1,    -1,   248,   249,    -1,    -1,    -1,    -1,
     254,    -1,   256,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      -1,    15,    16,    17,    18,    -1,    -1,    -1,    22,    -1,
     284,    -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,   302,    -1,
      -1,    -1,   306,    -1,    -1,    -1,    -1,   311,   312,    -1,
      -1,    -1,    -1,    -1,    -1,   319,   320,    61,    -1,    63,
      -1,    65,    -1,    67,    68,    -1,    -1,    71,    72,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    13,
      -1,    15,    16,    17,    18,    -1,    -1,    21,    22,    -1,
      -1,    -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    -1,    -1,    -1,    61,    -1,    63,
      -1,    65,    -1,    67,    68,    -1,    -1,    71,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    13,    -1,
      15,    16,    17,    18,    -1,    -1,    21,    22,    -1,    -1,
      -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,    -1,
      -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    -1,    -1,    -1,    61,    -1,    63,    -1,
      65,    -1,    67,    68,    -1,    -1,    71,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    13,    -1,    15,
      16,    17,    18,    -1,    -1,    21,    22,    -1,    -1,    -1,
      -1,    27,    28,    29,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    -1,    -1,    -1,    61,    -1,    63,    -1,    65,
      -1,    67,    68,    -1,    -1,    71,     3,     4,     5,    -1,
      -1,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    26,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    -1,    -1,    -1,    -1,    62,    -1,    -1,    65,    -1,
      -1,    68,    -1,    -1,    71,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    -1,    15,    16,    17,
      18,    -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,    27,
      28,    29,    -1,    -1,    32,    33,    34,    -1,    -1,    37,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    61,    -1,    63,    -1,    65,    -1,    67,
      68,    -1,    -1,    71,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    15,    16,    17,    18,
      -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    -1,
      -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    61,    -1,    63,    -1,    65,    -1,    67,    68,
      69,    -1,    71,     3,     4,     5,     6,    -1,     8,     9,
      10,    11,    12,    -1,    -1,    15,    16,    17,    18,    -1,
      -1,    -1,    22,    -1,    -1,    -1,    -1,    27,    28,    29,
      -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    -1,    -1,
      -1,    -1,    42,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    61,    -1,    63,    -1,    65,    -1,    67,    68,    -1,
      -1,    71,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    15,    16,    17,    18,    -1,    -1,
      -1,    22,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,
      -1,    32,    -1,    -1,    -1,    -1,    37,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      61,    -1,    63,    64,    65,    -1,    67,    68,    -1,    -1,
      71,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    -1,    14,    15,    16,    17,    18,    -1,    -1,    -1,
      22,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,
      -1,    63,    -1,    65,    -1,    67,    68,    -1,    -1,    71,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    -1,    15,    16,    17,    18,    -1,    -1,    -1,    22,
      -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,    32,
      -1,    -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,
      63,    -1,    65,    66,    67,    68,    -1,    -1,    71,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      -1,    15,    16,    17,    18,    -1,    -1,    -1,    22,    -1,
      -1,    -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,
      -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,    63,
      -1,    65,    -1,    67,    68,    69,    -1,    71,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    -1,
      15,    16,    17,    18,    -1,    -1,    -1,    22,    -1,    -1,
      -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,    -1,
      -1,    -1,    37,    -1,    -1,    -1,    -1,    42,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,    63,    -1,
      65,    -1,    67,    68,    -1,    -1,    71,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    -1,    15,
      16,    17,    18,    -1,    -1,    -1,    22,    -1,    -1,    -1,
      -1,    27,    28,    29,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    61,    -1,    63,    -1,    65,
      -1,    67,    68,    -1,    -1,    71,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    -1,    15,    16,
      17,    18,    -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,
      27,    28,    29,    -1,    -1,    32,    -1,    -1,    -1,    -1,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    61,    -1,    63,    -1,    65,    -1,
      67,    68,    -1,    -1,    71,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    -1,    15,    16,    17,
      18,    -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,    27,
      28,    29,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,
      -1,     3,     4,     5,    -1,    -1,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,    -1,    -1,    61,    26,    63,    -1,    65,    -1,    67,
      68,    -1,    -1,    71,    -1,    37,    -1,    -1,    -1,    -1,
      -1,    -1,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    -1,    -1,    -1,    -1,
      62,    -1,    -1,    65,    66,    -1,    68,    -1,    70,     3,
       4,     5,    -1,    -1,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    -1,    -1,    19,    -1,    21,    22,    -1,
      -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    -1,    -1,    -1,    -1,    62,    -1,
      -1,    65,    -1,    -1,    68,     3,     4,     5,    -1,    -1,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    26,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,    -1,    37,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    -1,    62,    -1,    -1,    65,    -1,    -1,
      68,     3,     4,     5,    -1,    -1,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,    -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    -1,    -1,    40,    -1,
      -1,    -1,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    -1,    -1,    -1,    -1,
      62,    -1,    -1,    65,    -1,    -1,    68,     3,     4,     5,
      -1,    -1,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    23,    -1,    -1,
      26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    -1,    -1,    -1,    -1,    62,    -1,    -1,    65,
      -1,    -1,    68,     3,     4,     5,    -1,    -1,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,    -1,    26,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    -1,    -1,
      -1,    -1,    -1,    -1,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      -1,    -1,    62,    -1,    -1,    65,    -1,    -1,    68,     3,
       4,     5,    -1,    -1,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,
      -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    -1,    -1,    -1,    -1,    62,    -1,
      -1,    65,    -1,    -1,    68,     3,     4,     5,    -1,    -1,
      -1,     9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    26,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    -1,    62,    -1,    -1,    65,    -1,    -1,
      68,     3,     4,     5,    -1,    -1,    -1,     9,    10,    11,
      12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,
      -1,    -1,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    -1,    -1,    -1,    -1,
      62,    -1,    -1,    65,    -1,    -1,    68,     3,     4,     5,
      -1,    -1,    -1,     9,    10,    11,    12,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    -1,     3,     4,
       5,    -1,    -1,    -1,     9,    10,    11,    12,    -1,    -1,
      -1,    37,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    37,    -1,    -1,    -1,    62,    -1,    -1,    65,
      -1,    -1,    68,    -1,    -1,    -1,    -1,    52,    53,    54,
      55,    56,    57,    -1,    -1,    -1,    -1,    62,    -1,    -1,
      65,    -1,    -1,    68,     3,     4,     5,    -1,    -1,    -1,
       9,    10,    11,    12,    -1,    -1,    -1,    -1,     3,     4,
       5,    -1,    21,    22,     9,    10,    11,    12,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    37,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    37,    -1,    -1,    54,    55,    56,    57,    -1,
      -1,    -1,    -1,    62,    -1,    -1,    65,    -1,    -1,    68,
      -1,    56,    57,    -1,    -1,    -1,    -1,    62,    -1,    -1,
      65,    -1,    -1,    68
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     8,     9,    10,    11,    12,
      15,    16,    17,    18,    22,    27,    28,    29,    32,    37,
      61,    63,    65,    67,    68,    71,    75,    76,    77,    79,
      80,    81,    84,    85,    87,    89,    92,    95,    76,    76,
       5,    22,    33,    34,    65,    76,    83,    86,    88,     5,
      22,    65,    82,    83,    76,    76,     5,     7,     5,     7,
       5,    96,    93,    76,     5,     5,    13,    17,    21,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    76,    78,    79,    88,     5,    65,    79,
      82,    42,    69,    88,    76,     0,    13,    21,    26,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    62,    65,    77,    58,    60,    23,    21,
      57,    64,    64,    64,    83,    88,     5,    88,    64,    64,
      70,    20,    88,    20,    83,    57,    64,    19,    64,     5,
      64,    38,    39,    41,    94,    64,    66,    22,    82,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    66,    66,    60,    66,    66,    70,
      60,    20,    42,    88,    69,    70,    35,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    64,    76,    76,    17,     5,    76,    76,
       5,    76,     5,    14,    76,    76,    76,    64,    64,    66,
      66,    76,    76,    76,    79,    66,    79,    57,    64,    65,
      76,    42,    91,     5,    22,    42,    65,    97,    98,    99,
     100,    99,    76,    76,    20,    20,    66,    79,    69,    42,
      70,    69,    76,    76,    22,    82,    64,    17,    76,    76,
      64,    60,    60,    65,    76,    99,    76,    90,    42,    57,
      58,    64,    73,   100,    99,    20,    70,    20,    42,    70,
      40,    23,    79,    79,    60,    69,    42,    36,    20,    20,
      65,    81,    97,    99,    76,    81,    99,    66,    71,    20,
      72,    90,   100,     5,     3,   100,    66,    99,   100,    99,
     100,   100,    76,    66,    66,    69,    76,    79,    79,    17,
      66,    76,    76,    20,    20,    66,    66,    22,    82,    76,
      76,    20,    20,    79,    79,    66,    66
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
                                        // TODO: not doing anything with macros yet - do we want to??
                                        printf("macro '%s'\n", (yyvsp[(1) - (2)].vident).chars);
                                        (yyval.ast_node_ptr) = (yyvsp[(2) - (2)].ast_node_ptr);
                                      }
    break;

  case 33:
#line 163 "lang/parser.y"
    {
                                          Ast *let = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), (yyvsp[(4) - (6)].ast_node_ptr), (yyvsp[(6) - (6)].ast_node_ptr));
                                          let->tag = AST_LOOP;
                                          (yyval.ast_node_ptr) = let;

                                      }
    break;

  case 34:
#line 172 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assignment((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 35:
#line 176 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[(1) - (1)].vint)); }
    break;

  case 36:
#line 177 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[(1) - (1)].vdouble)); }
    break;

  case 37:
#line 178 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 38:
#line 179 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); }
    break;

  case 39:
#line 180 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); }
    break;

  case 40:
#line 181 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 41:
#line 182 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;

  case 42:
#line 183 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 43:
#line 184 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 44:
#line 185 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 45:
#line 186 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 46:
#line 187 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_char((yyvsp[(1) - (1)].vchar)); }
    break;

  case 47:
#line 188 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 48:
#line 189 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(3) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 49:
#line 190 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 50:
#line 191 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"+", 1}); }
    break;

  case 51:
#line 192 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"-", 1}); }
    break;

  case 52:
#line 193 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"*", 1}); }
    break;

  case 53:
#line 194 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"/", 1}); }
    break;

  case 54:
#line 195 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"%", 1}); }
    break;

  case 55:
#line 196 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<", 1}); }
    break;

  case 56:
#line 197 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){">", 1}); }
    break;

  case 57:
#line 198 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"&&", 2}); }
    break;

  case 58:
#line 199 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"||", 2}); }
    break;

  case 59:
#line 200 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){">=", 2}); }
    break;

  case 60:
#line 201 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<=", 2}); }
    break;

  case 61:
#line 202 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"!=", 2}); }
    break;

  case 62:
#line 203 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"==", 2}); }
    break;

  case 63:
#line 204 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"|", 1}); }
    break;

  case 64:
#line 205 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){":", 1}); }
    break;

  case 65:
#line 206 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"::", 2}); }
    break;

  case 66:
#line 207 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 67:
#line 208 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access((yyvsp[(1) - (3)].ast_node_ptr), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 68:
#line 213 "lang/parser.y"
    {
      // Check if the identifier is in the custom_binops list
      bool found = false;
      custom_binops_t* current = __custom_binops;
      while (current != NULL) {
        if (strcmp(current->binop, (yyvsp[(1) - (1)].vident).chars) == 0) {
          found = true;
          break;
        }
        current = current->next;
      }
      
      if (!found) {
        yyerror("Invalid operator in section syntax");
        YYERROR;
      }
      
      (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident));
    }
    break;

  case 69:
#line 235 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 70:
#line 236 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 71:
#line 240 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_test_module((yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 72:
#line 241 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 73:
#line 243 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), ast_extern_fn((yyvsp[(2) - (6)].vident), (yyvsp[(6) - (6)].ast_node_ptr)), NULL); }
    break;

  case 74:
#line 245 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 75:
#line 247 "lang/parser.y"
    { Ast *let = ast_let((yyvsp[(3) - (5)].ast_node_ptr), (yyvsp[(5) - (5)].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 76:
#line 251 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);}
    break;

  case 77:
#line 253 "lang/parser.y"
    { Ast *let = ast_let(ast_tuple((yyvsp[(3) - (5)].ast_node_ptr)), (yyvsp[(5) - (5)].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 78:
#line 261 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(4) - (4)].ast_node_ptr); }
    break;

  case 79:
#line 262 "lang/parser.y"
    {
                                      Ast *let = (yyvsp[(1) - (3)].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[(3) - (3)].ast_node_ptr);
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 80:
#line 267 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 81:
#line 270 "lang/parser.y"
    {
                                      Ast *id = ast_identifier((yyvsp[(3) - (6)].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[(6) - (6)].ast_node_ptr), NULL);
                                    }
    break;

  case 82:
#line 277 "lang/parser.y"
    {
                                      Ast *id = ast_identifier((yyvsp[(3) - (6)].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[(6) - (6)].ast_node_ptr), NULL);
                                    }
    break;

  case 83:
#line 286 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), false); }
    break;

  case 84:
#line 287 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), true); }
    break;

  case 85:
#line 288 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), false); }
    break;

  case 86:
#line 289 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), true); }
    break;

  case 87:
#line 291 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_trait_impl((yyvsp[(2) - (6)].vident), (yyvsp[(4) - (6)].vident), (yyvsp[(6) - (6)].ast_node_ptr)); }
    break;

  case 88:
#line 303 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 89:
#line 304 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 90:
#line 305 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(3) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 91:
#line 306 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 92:
#line 307 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_module(ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr))); }
    break;

  case 93:
#line 308 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_module(ast_lambda(NULL, (yyvsp[(2) - (3)].ast_node_ptr))); }
    break;

  case 94:
#line 315 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 95:
#line 316 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 96:
#line 317 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 97:
#line 318 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 98:
#line 320 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 99:
#line 321 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 100:
#line 322 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 101:
#line 323 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (6)].ast_node_ptr), (yyvsp[(2) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 102:
#line 327 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 103:
#line 328 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 104:
#line 329 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 105:
#line 339 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 106:
#line 340 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 107:
#line 341 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (4)].ast_node_ptr); }
    break;

  case 108:
#line 345 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_array(); }
    break;

  case 109:
#line 346 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (5)].ast_node_ptr)); }
    break;

  case 110:
#line 347 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (6)].ast_node_ptr)); }
    break;

  case 111:
#line 351 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 112:
#line 352 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 113:
#line 356 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 114:
#line 357 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 115:
#line 358 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)); }
    break;

  case 116:
#line 362 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 117:
#line 363 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 118:
#line 367 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 119:
#line 368 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_if_else((yyvsp[(2) - (6)].ast_node_ptr), (yyvsp[(4) - (6)].ast_node_ptr) ,(yyvsp[(6) - (6)].ast_node_ptr));}
    break;

  case 120:
#line 369 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_if_else((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr), NULL);}
    break;

  case 121:
#line 373 "lang/parser.y"
    {(yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr);}
    break;

  case 122:
#line 374 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr));}
    break;

  case 123:
#line 377 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 124:
#line 378 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(3) - (5)].ast_node_ptr), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 125:
#line 379 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 126:
#line 382 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 127:
#line 386 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 128:
#line 387 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 129:
#line 391 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 130:
#line 392 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 131:
#line 396 "lang/parser.y"
    {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
    break;

  case 132:
#line 402 "lang/parser.y"
    {
                                      Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (2)].vident)), NULL, NULL);
                                      type_decl->tag = AST_TYPE_DECL;
                                      (yyval.ast_node_ptr) = type_decl;
                                   }
    break;

  case 133:
#line 408 "lang/parser.y"
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

  case 134:
#line 421 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list(ast_identifier((yyvsp[(1) - (1)].vident)), NULL); }
    break;

  case 135:
#line 422 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), ast_identifier((yyvsp[(2) - (2)].vident)), NULL); }
    break;

  case 136:
#line 425 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 137:
#line 426 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 138:
#line 430 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 139:
#line 431 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 140:
#line 435 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 141:
#line 436 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 142:
#line 437 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 143:
#line 438 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_signature_of_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 144:
#line 439 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 145:
#line 443 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 146:
#line 444 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(1) - (3)].vident)), AST_CONST(AST_INT, (yyvsp[(3) - (3)].vint)), NULL); }
    break;

  case 147:
#line 445 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 148:
#line 446 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assoc(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 149:
#line 447 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 150:
#line 448 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;

  case 151:
#line 449 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access(ast_identifier((yyvsp[(1) - (3)].vident)), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;


/* Line 1267 of yacc.c.  */
#line 2957 "lang/y.tab.c"
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


#line 451 "lang/parser.y"



void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, _cur_script);
}
#endif _LANG_TAB_Hparse

