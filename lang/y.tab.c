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
     META_IDENTIFIER = 261,
     TOK_STRING = 262,
     TOK_CHAR = 263,
     TRUE = 264,
     FALSE = 265,
     PIPE = 266,
     EXTERN = 267,
     TRIPLE_DOT = 268,
     LET = 269,
     FN = 270,
     MATCH = 271,
     WITH = 272,
     ARROW = 273,
     DOUBLE_COLON = 274,
     TOK_VOID = 275,
     TOK_DOUBLE_AMP = 276,
     TOK_DOUBLE_PIPE = 277,
     IN = 278,
     AND = 279,
     ASYNC = 280,
     DOUBLE_AT = 281,
     DOUBLE_AMP = 282,
     DOUBLE_PIPE = 283,
     FSTRING_START = 284,
     FSTRING_END = 285,
     FSTRING_INTERP_START = 286,
     FSTRING_INTERP_END = 287,
     FSTRING_TEXT = 288,
     APPLICATION = 289,
     MODULO = 290,
     NE = 291,
     EQ = 292,
     LE = 293,
     GE = 294,
     UMINUS = 295
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define DOUBLE 259
#define IDENTIFIER 260
#define META_IDENTIFIER 261
#define TOK_STRING 262
#define TOK_CHAR 263
#define TRUE 264
#define FALSE 265
#define PIPE 266
#define EXTERN 267
#define TRIPLE_DOT 268
#define LET 269
#define FN 270
#define MATCH 271
#define WITH 272
#define ARROW 273
#define DOUBLE_COLON 274
#define TOK_VOID 275
#define TOK_DOUBLE_AMP 276
#define TOK_DOUBLE_PIPE 277
#define IN 278
#define AND 279
#define ASYNC 280
#define DOUBLE_AT 281
#define DOUBLE_AMP 282
#define DOUBLE_PIPE 283
#define FSTRING_START 284
#define FSTRING_END 285
#define FSTRING_INTERP_START 286
#define FSTRING_INTERP_END 287
#define FSTRING_TEXT 288
#define APPLICATION 289
#define MODULO 290
#define NE 291
#define EQ 292
#define LE 293
#define GE 294
#define UMINUS 295




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
#line 30 "lang/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    char vchar;
}
/* Line 193 of yacc.c.  */
#line 213 "lang/y.tab.c"
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
#line 238 "lang/y.tab.c"

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
#define YYFINAL  57
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1322

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  63
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  28
/* YYNRULES -- Number of rules.  */
#define YYNRULES  115
/* YYNRULES -- Number of states.  */
#define YYNSTATES  236

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   295

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    52,     2,
      53,    54,    46,    44,    56,    45,    34,    47,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    48,    50,
      39,    55,    38,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    57,     2,    58,     2,    60,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    59,     2,     2,     2,     2,
       2,    62,     2,     2,     2,     2,    61,     2,     2,     2,
       2,    51,     2,     2,    35,     2,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    36,
      37,    40,    41,    42,    43,    49
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    11,    14,    18,    22,
      25,    28,    31,    35,    39,    43,    47,    51,    55,    59,
      63,    67,    71,    75,    79,    83,    87,    91,    95,    97,
      99,   101,   103,   105,   107,   109,   111,   113,   115,   117,
     119,   121,   123,   125,   129,   131,   135,   140,   145,   152,
     159,   164,   168,   170,   177,   181,   185,   189,   195,   198,
     204,   210,   217,   224,   226,   230,   233,   238,   240,   246,
     249,   256,   258,   262,   264,   267,   271,   276,   281,   287,
     294,   298,   302,   306,   310,   315,   317,   321,   326,   328,
     332,   337,   343,   349,   353,   354,   357,   359,   363,   368,
     373,   375,   378,   381,   384,   388,   392,   394,   397,   401,
     403,   405,   407,   411,   415,   419
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      64,     0,    -1,    67,    50,    -1,    67,    -1,    -1,    66,
      -1,    51,    65,    -1,    65,    34,     5,    -1,    65,    26,
      65,    -1,    65,    66,    -1,    52,    66,    -1,    46,    66,
      -1,    65,    44,    65,    -1,    65,    45,    65,    -1,    65,
      46,    65,    -1,    65,    47,    65,    -1,    65,    37,    65,
      -1,    65,    39,    65,    -1,    65,    38,    65,    -1,    65,
      21,    65,    -1,    65,    22,    65,    -1,    65,    43,    65,
      -1,    65,    42,    65,    -1,    65,    40,    65,    -1,    65,
      41,    65,    -1,    65,    11,    65,    -1,    65,    48,    65,
      -1,    65,    19,    65,    -1,    68,    -1,    79,    -1,    85,
      -1,     3,    -1,     4,    -1,     7,    -1,     9,    -1,    10,
      -1,     5,    -1,    20,    -1,    74,    -1,    75,    -1,    77,
      -1,    82,    -1,     8,    -1,    53,    67,    54,    -1,    65,
      -1,    67,    50,    65,    -1,    14,     5,    55,    65,    -1,
      14,    73,    55,    65,    -1,    14,     5,    55,    12,    15,
      87,    -1,    14,     5,    55,    53,    70,    54,    -1,    14,
      20,    55,    65,    -1,    68,    23,    65,    -1,    71,    -1,
      14,    53,     5,    54,    55,    71,    -1,    12,    15,    65,
      -1,    69,    18,    65,    -1,    69,    48,     7,    -1,    70,
      56,    69,    48,     7,    -1,    70,    56,    -1,    15,    72,
      18,    67,    50,    -1,    15,    20,    18,    67,    50,    -1,
      53,    15,    72,    18,    67,    54,    -1,    53,    15,    20,
      18,    67,    54,    -1,    73,    -1,    73,    55,    65,    -1,
      72,    73,    -1,    72,    73,    55,    65,    -1,    73,    -1,
      73,    48,    53,    89,    54,    -1,    72,    73,    -1,    72,
      73,    48,    53,    89,    54,    -1,     5,    -1,    53,    78,
      54,    -1,    76,    -1,    57,    58,    -1,    57,    78,    58,
      -1,    57,    78,    56,    58,    -1,    57,    35,    35,    58,
      -1,    57,    35,    78,    35,    58,    -1,    57,    35,    78,
      56,    35,    58,    -1,     5,    19,     5,    -1,     5,    19,
      65,    -1,    53,    65,    54,    -1,    53,    78,    54,    -1,
      53,    78,    56,    54,    -1,    65,    -1,    78,    56,    65,
      -1,    16,    65,    17,    81,    -1,    65,    -1,    65,    59,
      65,    -1,    35,    80,    18,    65,    -1,    81,    35,    80,
      18,    65,    -1,    81,    35,    60,    18,    65,    -1,    29,
      83,    30,    -1,    -1,    83,    84,    -1,    33,    -1,    31,
      65,    32,    -1,    61,     5,    55,    89,    -1,    61,    86,
      55,    89,    -1,     5,    -1,    86,     5,    -1,    89,    18,
      -1,    87,    89,    -1,    90,    46,    90,    -1,    88,    46,
      90,    -1,    90,    -1,    35,    90,    -1,    89,    35,    90,
      -1,    87,    -1,    88,    -1,     5,    -1,     5,    55,     3,
      -1,     5,    62,    90,    -1,    53,    89,    54,    -1,    20,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   116,   116,   117,   118,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   169,   170,   174,   175,   176,   180,
     187,   188,   193,   195,   204,   205,   210,   213,   216,   221,
     222,   223,   224,   231,   232,   233,   234,   236,   237,   238,
     239,   243,   244,   245,   255,   256,   257,   261,   262,   263,
     267,   268,   272,   273,   274,   278,   279,   283,   287,   288,
     291,   292,   293,   296,   300,   301,   305,   306,   310,   316,
     329,   330,   333,   334,   338,   339,   343,   344,   345,   346,
     347,   351,   352,   353,   354,   355
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INTEGER", "DOUBLE", "IDENTIFIER",
  "META_IDENTIFIER", "TOK_STRING", "TOK_CHAR", "TRUE", "FALSE", "PIPE",
  "EXTERN", "TRIPLE_DOT", "LET", "FN", "MATCH", "WITH", "ARROW",
  "DOUBLE_COLON", "TOK_VOID", "TOK_DOUBLE_AMP", "TOK_DOUBLE_PIPE", "IN",
  "AND", "ASYNC", "DOUBLE_AT", "DOUBLE_AMP", "DOUBLE_PIPE",
  "FSTRING_START", "FSTRING_END", "FSTRING_INTERP_START",
  "FSTRING_INTERP_END", "FSTRING_TEXT", "'.'", "'|'", "APPLICATION",
  "MODULO", "'>'", "'<'", "NE", "EQ", "LE", "GE", "'+'", "'-'", "'*'",
  "'/'", "':'", "UMINUS", "';'", "'yield'", "'&'", "'('", "')'", "'='",
  "','", "'['", "']'", "'if'", "'_'", "'type'", "'of'", "$accept",
  "program", "expr", "simple_expr", "expr_sequence", "let_binding",
  "extern_typed_signature", "extern_variants", "lambda_expr",
  "lambda_args", "lambda_arg", "list", "array", "list_match_expr", "tuple",
  "expr_list", "match_expr", "match_test_clause", "match_branches",
  "fstring", "fstring_parts", "fstring_part", "type_decl", "type_args",
  "fn_signature", "tuple_type", "type_expr", "type_atom", 0
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
     285,   286,   287,   288,    46,   124,   289,   290,    62,    60,
     291,   292,   293,   294,    43,    45,    42,    47,    58,   295,
      59,   121,    38,    40,    41,    61,    44,    91,    93,   105,
      95,   116,   111
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    63,    64,    64,    64,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    66,    67,    67,    68,    68,    68,    68,
      68,    68,    68,    68,    69,    69,    70,    70,    70,    71,
      71,    71,    71,    72,    72,    72,    72,    72,    72,    72,
      72,    73,    73,    73,    74,    74,    74,    75,    75,    75,
      76,    76,    77,    77,    77,    78,    78,    79,    80,    80,
      81,    81,    81,    82,    83,    83,    84,    84,    85,    85,
      86,    86,    87,    87,    88,    88,    89,    89,    89,    89,
      89,    90,    90,    90,    90,    90
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     2,     3,     3,     2,
       2,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     1,     3,     4,     4,     6,     6,
       4,     3,     1,     6,     3,     3,     3,     5,     2,     5,
       5,     6,     6,     1,     3,     2,     4,     1,     5,     2,
       6,     1,     3,     1,     2,     3,     4,     4,     5,     6,
       3,     3,     3,     3,     4,     1,     3,     4,     1,     3,
       4,     5,     5,     3,     0,     2,     1,     3,     4,     4,
       1,     2,     2,     2,     3,     3,     1,     2,     3,     1,
       1,     1,     3,     3,     3,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    31,    32,    36,    33,    42,    34,    35,     0,     0,
       0,    37,    94,     0,     0,     0,     0,     0,     0,     0,
      44,     5,     3,    28,    52,    38,    39,    40,    29,    41,
      30,     0,     0,     0,     0,    73,    71,     0,     0,     0,
      63,     0,     0,     0,    11,     6,    10,     0,    44,     0,
       0,     0,    74,    85,     0,   100,     0,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     9,     2,     0,     0,
       0,     0,    36,     0,     0,     0,     0,    65,     0,     0,
       0,    93,     0,    96,    95,     0,     0,    82,     0,    43,
      83,     0,     0,     0,     0,    75,     0,   101,     0,    25,
      27,    19,    20,     8,     7,    16,    18,    17,    23,    24,
      22,    21,    12,    13,    14,    15,    26,    45,    51,    36,
      81,     0,     0,    46,    50,     0,    72,     0,    47,     0,
       0,     0,     0,     0,    64,     0,    87,     0,     0,     0,
      84,    86,    77,     0,     0,    76,   111,   115,     0,     0,
     109,   110,    98,   106,    99,     0,     0,     0,     0,     0,
      60,    59,     0,    66,     0,    88,     0,     0,    97,     0,
       0,    78,     0,     0,     0,   107,     0,   103,     0,   102,
       0,     0,    48,     0,     0,     0,     0,    49,    58,     0,
      53,     0,    68,     0,     0,     0,     0,    62,    61,    79,
     112,   113,   114,   105,   108,   104,    54,    55,    56,     0,
       0,    70,    89,    90,     0,     0,     0,     0,     0,    92,
      91,    57,     0,     0,     0,     0
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    19,    20,    21,    49,    23,   167,   168,    24,    39,
      40,    25,    26,    35,    27,    50,    28,   176,   146,    29,
      42,    94,    30,    56,   160,   161,   187,   163
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -101
static const yytype_int16 yypact[] =
{
     585,  -101,  -101,  -101,  -101,  -101,  -101,  -101,    16,    21,
     585,  -101,  -101,   121,   585,   121,   603,     9,     5,    77,
     945,  -101,    32,    67,  -101,  -101,  -101,  -101,  -101,  -101,
    -101,    33,    39,   658,    63,  -101,   124,    78,   585,    10,
      23,   843,   136,   585,  -101,   945,  -101,    26,   788,    30,
      -3,   281,  -101,   945,    76,    96,     2,  -101,   585,   585,
     585,   585,   585,    90,   585,   585,   585,   585,   585,   585,
     585,   585,   585,   585,   585,   585,  -101,   585,   585,   676,
     353,   585,    98,   146,   585,   585,   585,    89,   104,   585,
     140,  -101,   585,  -101,  -101,   164,    22,  -101,   585,  -101,
    -101,   380,   119,    41,   435,  -101,    15,  -101,    15,  1047,
     945,   945,   945,  1098,  -101,  1149,   151,   151,   151,   151,
     151,   151,  1200,  1200,  1223,  1223,  1265,   945,   945,  -101,
     945,   174,   453,   945,   945,   135,  -101,   585,   945,   142,
     167,   148,   585,    15,   945,   585,   187,   894,   585,   585,
    -101,   945,  -101,   165,   508,  -101,    83,  -101,    28,    15,
      15,   178,   -13,   179,   -13,    15,   211,    73,   160,    -4,
     585,   585,    15,   945,    29,   731,   209,   526,  -101,    92,
     134,  -101,   171,   227,    28,  -101,    38,   -13,    28,  -101,
      28,    28,    15,   -13,   585,   585,   224,  -101,   222,   225,
    -101,    85,  -101,   585,   585,   235,   241,  -101,  -101,  -101,
    -101,  -101,  -101,  -101,  -101,  -101,   945,   996,  -101,   117,
      34,  -101,   945,   945,   585,   585,   253,   244,    40,   945,
     945,  -101,   585,   585,   153,   159
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -101,  -101,   177,   -11,     0,  -101,    65,  -101,    95,   -44,
      -7,  -101,  -101,  -101,  -101,   130,  -101,    88,  -101,  -101,
    -101,  -101,  -101,  -101,   102,  -101,  -100,    -5
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -86
static const yytype_int16 yytable[] =
{
      22,    34,    44,    96,    46,   189,   162,   107,   164,    76,
      55,     9,     1,     2,     3,    36,     4,     5,     6,     7,
     156,    31,   190,     8,     9,    10,    36,    36,    86,    11,
      76,    36,    87,   156,    76,   157,    32,    76,    12,    36,
     149,    37,    76,   174,    51,    36,    95,   189,   157,   199,
     158,   100,    79,   101,   227,    13,   189,   108,   233,   186,
      14,    15,    16,    38,   190,   193,    17,    52,   159,    33,
      18,    88,   201,   190,    38,    38,   153,    57,    89,    38,
      98,   159,    77,   202,    99,   139,   140,    38,    80,    87,
      78,   195,   212,    38,    81,   114,    85,   154,    76,    76,
      76,    76,    76,   189,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    84,    76,
     190,   196,    76,    76,     1,     2,     3,    76,     4,     5,
       6,     7,   104,    76,   105,   195,    76,   141,   183,   221,
      76,    11,   170,    79,   142,   184,   207,    54,   179,   180,
      12,   106,   135,   185,     1,     2,     3,   143,     4,     5,
       6,     7,    76,    83,    76,   226,    91,    92,    83,    93,
      59,    11,    60,    61,    43,   145,   228,   152,    17,   211,
      12,   103,   148,   213,   171,   214,   215,    41,   208,   165,
     169,    45,   170,    48,    53,    71,    72,    73,    74,    75,
     136,   172,   137,    98,    43,    76,    76,   207,    17,    98,
      53,    76,    76,   208,   197,    53,   198,   171,    76,    76,
      48,    87,   177,   181,   188,   191,   194,   204,    53,   209,
     210,   218,   234,   235,   166,   109,   110,   111,   112,   113,
     220,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   224,   127,   128,   130,   133,   134,   225,
     231,   138,   232,   219,   200,   206,   144,   192,     0,   147,
       0,     0,     0,     0,     0,   127,     0,     0,   151,     0,
       0,   151,     0,     0,     1,     2,     3,     0,     4,     5,
       6,     7,     0,     0,     0,     8,     9,    10,     0,     0,
       0,    11,     0,     0,     0,     0,     0,     0,     0,    48,
      12,     0,     0,     0,   151,     0,   102,     0,     0,   173,
       0,     0,   175,     0,     0,     0,     0,    13,     0,     0,
       0,   151,    14,    15,    16,     0,     0,     0,    17,     0,
       0,     0,    18,     0,     0,     0,     0,   127,   127,     0,
       0,     0,     0,     0,   175,     0,     1,     2,     3,     0,
       4,     5,     6,     7,     0,   131,     0,     8,     9,    10,
       0,   216,   217,    11,     0,     0,     0,     0,     0,     0,
     222,   223,    12,     1,     2,     3,     0,     4,     5,     6,
       7,     0,     0,     0,     8,     9,    10,     0,     0,    13,
      11,   229,   230,     0,    14,    15,   132,     0,     0,    12,
      17,     0,     0,     0,    18,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    13,     0,     0,     0,
       0,    14,    15,    16,   150,     0,     0,    17,     1,     2,
       3,    18,     4,     5,     6,     7,     0,     0,     0,     8,
       9,    10,     0,     0,     0,    11,     1,     2,     3,     0,
       4,     5,     6,     7,    12,   166,     0,     8,    47,    10,
       0,     0,     0,    11,     0,     0,     0,     0,     0,     0,
       0,    13,    12,     0,     0,     0,    14,    15,    16,     0,
       0,     0,    17,   155,     0,     0,    18,     0,     0,    13,
       0,     0,     0,     0,    14,    15,    16,     0,     0,     0,
      17,     1,     2,     3,    18,     4,     5,     6,     7,     0,
       0,     0,     8,     9,    10,     0,     0,     0,    11,     1,
       2,     3,     0,     4,     5,     6,     7,    12,     0,     0,
       8,     9,    10,   182,     0,     0,    11,     0,     0,     0,
       0,     0,     0,     0,    13,    12,     0,     0,     0,    14,
      15,    16,     0,     0,     0,    17,     0,     0,     0,    18,
       0,     0,    13,     0,     0,     0,     0,    14,    15,    16,
       0,     0,     0,    17,     0,     0,   205,    18,     1,     2,
       3,     0,     4,     5,     6,     7,     0,     0,     0,     8,
       9,    10,     0,     0,     0,    11,     1,     2,     3,     0,
       4,     5,     6,     7,    12,     0,     0,     8,    47,    10,
       0,     0,     0,    11,     0,     0,     0,     0,     0,     0,
       0,    13,    12,     0,     0,     0,    14,    15,    16,     0,
       0,     0,    17,     0,     0,     0,    18,     0,     0,    13,
       0,     0,     0,     0,    14,    15,    16,     0,     0,     0,
      17,     1,     2,    82,    18,     4,     5,     6,     7,     0,
       0,     0,     8,     9,    10,     0,     0,     0,    11,     1,
       2,   129,     0,     4,     5,     6,     7,    12,     0,     0,
       8,     9,    10,     0,     0,     0,    11,     0,     0,     0,
       0,     0,     0,     0,    13,    12,     0,     0,     0,    14,
      15,    16,     0,     0,     0,    17,     0,     0,     0,    18,
       0,     0,    13,     0,     0,     0,     0,    14,    15,    16,
       0,     0,     0,    17,     1,     2,     3,    18,     4,     5,
       6,     7,    58,     0,     0,     0,     0,     0,     0,     0,
      59,    11,    60,    61,     0,     0,     0,    62,     0,     0,
      12,     0,     0,     0,     0,    63,     0,     0,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
       0,     0,     0,     0,    43,     0,     0,     0,    17,     0,
     203,     1,     2,     3,     0,     4,     5,     6,     7,    58,
       0,     0,     0,     0,     0,     0,     0,    59,    11,    60,
      61,     0,     0,     0,    62,     0,     0,    12,     0,     0,
       0,     0,    63,     0,     0,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,     0,     0,     0,
       0,    43,    97,     0,   -85,    17,     1,     2,     3,     0,
       4,     5,     6,     7,    58,     0,     0,     0,     0,     0,
      90,     0,    59,    11,    60,    61,     0,     0,     0,    62,
       0,     0,    12,     0,     0,     0,     0,    63,     0,     0,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,     0,     0,     0,     0,    43,     1,     2,     3,
      17,     4,     5,     6,     7,    58,     0,     0,     0,     0,
       0,     0,     0,    59,    11,    60,    61,     0,     0,     0,
      62,     0,     0,    12,     0,     0,   178,     0,    63,     0,
       0,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,     0,     0,     0,     0,    43,     1,     2,
       3,    17,     4,     5,     6,     7,    58,     0,     0,     0,
       0,     0,     0,     0,    59,    11,    60,    61,     0,     0,
       0,    62,     0,     0,    12,     0,     0,     0,     0,    63,
       0,     0,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,     0,     0,     0,     0,    43,     1,
       2,     3,    17,     4,     5,     6,     7,    58,     0,     0,
       0,     0,     0,     0,     0,    59,    11,    60,    61,     0,
       0,     0,    62,     0,     0,    12,     0,     0,     0,     0,
      63,     0,     0,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,     0,     0,     0,     0,     0,    43,
       1,     2,     3,    17,     4,     5,     6,     7,     0,     0,
       0,     0,     0,     0,     0,     0,    59,    11,    60,    61,
       0,     0,     0,    62,     0,     0,    12,     0,     0,     0,
       0,     0,     0,     0,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,     0,     0,     0,     0,
      43,     1,     2,     3,    17,     4,     5,     6,     7,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    11,    60,
      61,     0,     0,     0,     0,     0,     0,    12,     0,     0,
       0,     0,     0,     0,     0,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,     0,     0,     0,
       0,    43,     1,     2,     3,    17,     4,     5,     6,     7,
       0,     0,     0,     0,     0,     0,     0,     0,    59,    11,
      60,    61,     0,     0,     0,     0,     0,     0,    12,     0,
       0,     0,     0,     0,     0,     0,     0,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,     0,     0,
       0,     0,    43,     1,     2,     3,    17,     4,     5,     6,
       7,     0,     0,     0,     0,     0,     0,     0,     0,    59,
      11,    60,    61,     0,     0,     0,     1,     2,     3,    12,
       4,     5,     6,     7,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    11,    60,    61,    73,    74,    75,     0,
       0,     0,    12,    43,     0,     0,     0,    17,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     1,     2,
       3,    75,     4,     5,     6,     7,    43,     0,     0,     0,
      17,     0,     0,     0,    59,    11,    60,    61,     0,     0,
       0,     0,     0,     0,    12,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    43,     0,
       0,     0,    17
};

static const yytype_int16 yycheck[] =
{
       0,     8,    13,    47,    15,    18,   106,     5,   108,    20,
       5,    15,     3,     4,     5,     5,     7,     8,     9,    10,
       5,     5,    35,    14,    15,    16,     5,     5,    18,    20,
      41,     5,    39,     5,    45,    20,    20,    48,    29,     5,
      18,    20,    53,   143,    35,     5,    20,    18,    20,    53,
      35,    54,    19,    56,    20,    46,    18,    55,    18,   159,
      51,    52,    53,    53,    35,   165,    57,    58,    53,    53,
      61,    48,   172,    35,    53,    53,    35,     0,    55,    53,
      50,    53,    50,    54,    54,    85,    86,    53,    55,    96,
      23,    18,    54,    53,    55,     5,    18,    56,   109,   110,
     111,   112,   113,    18,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,    55,   130,
      35,    48,   133,   134,     3,     4,     5,   138,     7,     8,
       9,    10,    56,   144,    58,    18,   147,    48,    55,    54,
     151,    20,    50,    19,    55,    62,    54,    17,   148,   149,
      29,    55,    54,   158,     3,     4,     5,    53,     7,     8,
       9,    10,   173,    33,   175,    48,    30,    31,    38,    33,
      19,    20,    21,    22,    53,    35,   220,    58,    57,   184,
      29,    51,    18,   188,    50,   190,   191,    10,    54,    15,
      55,    14,    50,    16,    17,    44,    45,    46,    47,    48,
      54,    53,    56,    50,    53,   216,   217,    54,    57,    50,
      33,   222,   223,    54,    54,    38,    56,    50,   229,   230,
      43,   228,    35,    58,    46,    46,    15,    18,    51,    58,
       3,     7,   232,   233,    12,    58,    59,    60,    61,    62,
      15,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    18,    77,    78,    79,    80,    81,    18,
       7,    84,    18,   198,   169,   177,    89,   165,    -1,    92,
      -1,    -1,    -1,    -1,    -1,    98,    -1,    -1,   101,    -1,
      -1,   104,    -1,    -1,     3,     4,     5,    -1,     7,     8,
       9,    10,    -1,    -1,    -1,    14,    15,    16,    -1,    -1,
      -1,    20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   132,
      29,    -1,    -1,    -1,   137,    -1,    35,    -1,    -1,   142,
      -1,    -1,   145,    -1,    -1,    -1,    -1,    46,    -1,    -1,
      -1,   154,    51,    52,    53,    -1,    -1,    -1,    57,    -1,
      -1,    -1,    61,    -1,    -1,    -1,    -1,   170,   171,    -1,
      -1,    -1,    -1,    -1,   177,    -1,     3,     4,     5,    -1,
       7,     8,     9,    10,    -1,    12,    -1,    14,    15,    16,
      -1,   194,   195,    20,    -1,    -1,    -1,    -1,    -1,    -1,
     203,   204,    29,     3,     4,     5,    -1,     7,     8,     9,
      10,    -1,    -1,    -1,    14,    15,    16,    -1,    -1,    46,
      20,   224,   225,    -1,    51,    52,    53,    -1,    -1,    29,
      57,    -1,    -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,
      -1,    51,    52,    53,    54,    -1,    -1,    57,     3,     4,
       5,    61,     7,     8,     9,    10,    -1,    -1,    -1,    14,
      15,    16,    -1,    -1,    -1,    20,     3,     4,     5,    -1,
       7,     8,     9,    10,    29,    12,    -1,    14,    15,    16,
      -1,    -1,    -1,    20,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    46,    29,    -1,    -1,    -1,    51,    52,    53,    -1,
      -1,    -1,    57,    58,    -1,    -1,    61,    -1,    -1,    46,
      -1,    -1,    -1,    -1,    51,    52,    53,    -1,    -1,    -1,
      57,     3,     4,     5,    61,     7,     8,     9,    10,    -1,
      -1,    -1,    14,    15,    16,    -1,    -1,    -1,    20,     3,
       4,     5,    -1,     7,     8,     9,    10,    29,    -1,    -1,
      14,    15,    16,    35,    -1,    -1,    20,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    29,    -1,    -1,    -1,    51,
      52,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    61,
      -1,    -1,    46,    -1,    -1,    -1,    -1,    51,    52,    53,
      -1,    -1,    -1,    57,    -1,    -1,    60,    61,     3,     4,
       5,    -1,     7,     8,     9,    10,    -1,    -1,    -1,    14,
      15,    16,    -1,    -1,    -1,    20,     3,     4,     5,    -1,
       7,     8,     9,    10,    29,    -1,    -1,    14,    15,    16,
      -1,    -1,    -1,    20,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    46,    29,    -1,    -1,    -1,    51,    52,    53,    -1,
      -1,    -1,    57,    -1,    -1,    -1,    61,    -1,    -1,    46,
      -1,    -1,    -1,    -1,    51,    52,    53,    -1,    -1,    -1,
      57,     3,     4,     5,    61,     7,     8,     9,    10,    -1,
      -1,    -1,    14,    15,    16,    -1,    -1,    -1,    20,     3,
       4,     5,    -1,     7,     8,     9,    10,    29,    -1,    -1,
      14,    15,    16,    -1,    -1,    -1,    20,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    29,    -1,    -1,    -1,    51,
      52,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    61,
      -1,    -1,    46,    -1,    -1,    -1,    -1,    51,    52,    53,
      -1,    -1,    -1,    57,     3,     4,     5,    61,     7,     8,
       9,    10,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    21,    22,    -1,    -1,    -1,    26,    -1,    -1,
      29,    -1,    -1,    -1,    -1,    34,    -1,    -1,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,    -1,
      59,     3,     4,     5,    -1,     7,     8,     9,    10,    11,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,
      22,    -1,    -1,    -1,    26,    -1,    -1,    29,    -1,    -1,
      -1,    -1,    34,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    -1,    -1,
      -1,    53,    54,    -1,    56,    57,     3,     4,     5,    -1,
       7,     8,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,
      17,    -1,    19,    20,    21,    22,    -1,    -1,    -1,    26,
      -1,    -1,    29,    -1,    -1,    -1,    -1,    34,    -1,    -1,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    -1,    -1,    -1,    -1,    53,     3,     4,     5,
      57,     7,     8,     9,    10,    11,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    19,    20,    21,    22,    -1,    -1,    -1,
      26,    -1,    -1,    29,    -1,    -1,    32,    -1,    34,    -1,
      -1,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    -1,    -1,    -1,    -1,    53,     3,     4,
       5,    57,     7,     8,     9,    10,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    21,    22,    -1,    -1,
      -1,    26,    -1,    -1,    29,    -1,    -1,    -1,    -1,    34,
      -1,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    -1,    -1,    -1,    -1,    53,     3,
       4,     5,    57,     7,     8,     9,    10,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    20,    21,    22,    -1,
      -1,    -1,    26,    -1,    -1,    29,    -1,    -1,    -1,    -1,
      34,    -1,    -1,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    -1,    -1,    -1,    -1,    -1,    53,
       3,     4,     5,    57,     7,     8,     9,    10,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,    22,
      -1,    -1,    -1,    26,    -1,    -1,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    -1,    -1,    -1,    -1,
      53,     3,     4,     5,    57,     7,     8,     9,    10,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    -1,    -1,    -1,
      -1,    53,     3,     4,     5,    57,     7,     8,     9,    10,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    29,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    -1,    -1,
      -1,    -1,    53,     3,     4,     5,    57,     7,     8,     9,
      10,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,
      20,    21,    22,    -1,    -1,    -1,     3,     4,     5,    29,
       7,     8,     9,    10,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    20,    21,    22,    46,    47,    48,    -1,
      -1,    -1,    29,    53,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,    48,     7,     8,     9,    10,    53,    -1,    -1,    -1,
      57,    -1,    -1,    -1,    19,    20,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    53,    -1,
      -1,    -1,    57
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     7,     8,     9,    10,    14,    15,
      16,    20,    29,    46,    51,    52,    53,    57,    61,    64,
      65,    66,    67,    68,    71,    74,    75,    77,    79,    82,
      85,     5,    20,    53,    73,    76,     5,    20,    53,    72,
      73,    65,    83,    53,    66,    65,    66,    15,    65,    67,
      78,    35,    58,    65,    78,     5,    86,     0,    11,    19,
      21,    22,    26,    34,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    66,    50,    23,    19,
      55,    55,     5,    78,    55,    18,    18,    73,    48,    55,
      17,    30,    31,    33,    84,    20,    72,    54,    50,    54,
      54,    56,    35,    78,    56,    58,    55,     5,    55,    65,
      65,    65,    65,    65,     5,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,     5,
      65,    12,    53,    65,    65,    54,    54,    56,    65,    67,
      67,    48,    55,    53,    65,    35,    81,    65,    18,    18,
      54,    65,    58,    35,    56,    58,     5,    20,    35,    53,
      87,    88,    89,    90,    89,    15,    12,    69,    70,    55,
      50,    50,    53,    65,    89,    65,    80,    35,    32,    67,
      67,    58,    35,    55,    62,    90,    89,    89,    46,    18,
      35,    46,    87,    89,    15,    18,    48,    54,    56,    53,
      71,    89,    54,    59,    18,    60,    80,    54,    54,    58,
       3,    90,    54,    90,    90,    90,    65,    65,     7,    69,
      15,    54,    65,    65,    18,    18,    48,    20,    72,    65,
      65,     7,    18,    18,    67,    67
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
#line 116 "lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (2)].ast_node_ptr)); }
    break;

  case 3:
#line 117 "lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 6:
#line 125 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_yield((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 7:
#line 126 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access((yyvsp[(1) - (3)].ast_node_ptr), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 8:
#line 127 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 9:
#line 128 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 10:
#line 129 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_unop(TOKEN_AMPERSAND, (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 11:
#line 130 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_unop(TOKEN_STAR, (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 12:
#line 131 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 13:
#line 132 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 14:
#line 133 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 15:
#line 134 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 16:
#line 135 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 17:
#line 136 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 18:
#line 137 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 19:
#line 138 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_AMP, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 20:
#line 139 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_PIPE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 21:
#line 140 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 22:
#line 141 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 23:
#line 142 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 24:
#line 143 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 25:
#line 144 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(3) - (3)].ast_node_ptr), (yyvsp[(1) - (3)].ast_node_ptr)); }
    break;

  case 26:
#line 145 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assoc((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 27:
#line 146 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 28:
#line 147 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 29:
#line 148 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 30:
#line 149 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 31:
#line 153 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[(1) - (1)].vint)); }
    break;

  case 32:
#line 154 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[(1) - (1)].vdouble)); }
    break;

  case 33:
#line 155 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 34:
#line 156 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); }
    break;

  case 35:
#line 157 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); }
    break;

  case 36:
#line 158 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 37:
#line 159 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;

  case 38:
#line 160 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 39:
#line 161 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 40:
#line 162 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 41:
#line 163 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 42:
#line 164 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_char((yyvsp[(1) - (1)].vchar)); }
    break;

  case 43:
#line 165 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 44:
#line 169 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 45:
#line 170 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 46:
#line 174 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 47:
#line 175 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 48:
#line 177 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), ast_extern_fn((yyvsp[(2) - (6)].vident), (yyvsp[(6) - (6)].ast_node_ptr)), NULL); }
    break;

  case 49:
#line 181 "lang/parser.y"
    {
                                      Ast *variants = (yyvsp[(5) - (6)].ast_node_ptr);
                                      variants->tag = AST_EXTERN_VARIANTS;
                                      (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), variants, NULL);
                                    }
    break;

  case 50:
#line 187 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(4) - (4)].ast_node_ptr); }
    break;

  case 51:
#line 188 "lang/parser.y"
    {
                                      Ast *let = (yyvsp[(1) - (3)].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[(3) - (3)].ast_node_ptr);
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 52:
#line 193 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 53:
#line 196 "lang/parser.y"
    {
                                      Ast *id = ast_identifier((yyvsp[(3) - (6)].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[(6) - (6)].ast_node_ptr), NULL);
                                    }
    break;

  case 54:
#line 204 "lang/parser.y"
    { (yyval.ast_node_ptr) = extern_typed_signature((yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 55:
#line 206 "lang/parser.y"
    { (yyval.ast_node_ptr) = extern_typed_signature_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 56:
#line 211 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list(ast_extern_fn((yyvsp[(3) - (3)].vstr), (yyvsp[(1) - (3)].ast_node_ptr))); }
    break;

  case 57:
#line 214 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (5)].ast_node_ptr), ast_extern_fn((yyvsp[(5) - (5)].vstr), (yyvsp[(3) - (5)].ast_node_ptr))); }
    break;

  case 58:
#line 216 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (2)].ast_node_ptr); }
    break;

  case 59:
#line 221 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 60:
#line 222 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 61:
#line 223 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(3) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 62:
#line 224 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 63:
#line 231 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 64:
#line 232 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 65:
#line 233 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 66:
#line 234 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 67:
#line 236 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 68:
#line 237 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 69:
#line 238 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 70:
#line 239 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (6)].ast_node_ptr), (yyvsp[(2) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 71:
#line 243 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 72:
#line 244 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 73:
#line 245 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 74:
#line 255 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 75:
#line 256 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 76:
#line 257 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (4)].ast_node_ptr); }
    break;

  case 77:
#line 261 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_array(); }
    break;

  case 78:
#line 262 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (5)].ast_node_ptr)); }
    break;

  case 79:
#line 263 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (6)].ast_node_ptr)); }
    break;

  case 80:
#line 267 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 81:
#line 268 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 82:
#line 272 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 83:
#line 273 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 84:
#line 274 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)); }
    break;

  case 85:
#line 278 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 86:
#line 279 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 87:
#line 283 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 88:
#line 287 "lang/parser.y"
    {(yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr);}
    break;

  case 89:
#line 288 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr));}
    break;

  case 90:
#line 291 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 91:
#line 292 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(3) - (5)].ast_node_ptr), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 92:
#line 293 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 93:
#line 296 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 94:
#line 300 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 95:
#line 301 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 96:
#line 305 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 97:
#line 306 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 98:
#line 310 "lang/parser.y"
    {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
    break;

  case 99:
#line 316 "lang/parser.y"
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

  case 100:
#line 329 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list(ast_identifier((yyvsp[(1) - (1)].vident)), NULL); }
    break;

  case 101:
#line 330 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), ast_identifier((yyvsp[(2) - (2)].vident)), NULL); }
    break;

  case 102:
#line 333 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[(1) - (2)].ast_node_ptr), NULL); }
    break;

  case 103:
#line 334 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 104:
#line 338 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 105:
#line 339 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 106:
#line 343 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 107:
#line 344 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 108:
#line 345 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 109:
#line 346 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 110:
#line 347 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 111:
#line 351 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 112:
#line 352 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(1) - (3)].vident)), AST_CONST(AST_INT, (yyvsp[(3) - (3)].vint)), NULL); }
    break;

  case 113:
#line 353 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 114:
#line 354 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 115:
#line 355 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;


/* Line 1267 of yacc.c.  */
#line 2473 "lang/y.tab.c"
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


#line 357 "lang/parser.y"



void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, _cur_script);
}
#endif _LANG_TAB_H

