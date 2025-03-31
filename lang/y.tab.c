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
     PATH_IDENTIFIER = 261,
     IDENTIFIER_LIST = 262,
     TOK_STRING = 263,
     TOK_CHAR = 264,
     TRUE = 265,
     FALSE = 266,
     PIPE = 267,
     EXTERN = 268,
     TRIPLE_DOT = 269,
     LET = 270,
     FN = 271,
     MATCH = 272,
     WITH = 273,
     ARROW = 274,
     DOUBLE_COLON = 275,
     TOK_VOID = 276,
     IN = 277,
     AND = 278,
     ASYNC = 279,
     DOUBLE_AT = 280,
     THUNK = 281,
     IMPORT = 282,
     OPEN = 283,
     IMPLEMENTS = 284,
     AMPERSAND = 285,
     TYPE = 286,
     TEST_ID = 287,
     FSTRING_START = 288,
     FSTRING_END = 289,
     FSTRING_INTERP_START = 290,
     FSTRING_INTERP_END = 291,
     FSTRING_TEXT = 292,
     APPLICATION = 293,
     DOUBLE_PIPE = 294,
     DOUBLE_AMP = 295,
     NE = 296,
     EQ = 297,
     LE = 298,
     GE = 299,
     MODULO = 300,
     UMINUS = 301
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define DOUBLE 259
#define IDENTIFIER 260
#define PATH_IDENTIFIER 261
#define IDENTIFIER_LIST 262
#define TOK_STRING 263
#define TOK_CHAR 264
#define TRUE 265
#define FALSE 266
#define PIPE 267
#define EXTERN 268
#define TRIPLE_DOT 269
#define LET 270
#define FN 271
#define MATCH 272
#define WITH 273
#define ARROW 274
#define DOUBLE_COLON 275
#define TOK_VOID 276
#define IN 277
#define AND 278
#define ASYNC 279
#define DOUBLE_AT 280
#define THUNK 281
#define IMPORT 282
#define OPEN 283
#define IMPLEMENTS 284
#define AMPERSAND 285
#define TYPE 286
#define TEST_ID 287
#define FSTRING_START 288
#define FSTRING_END 289
#define FSTRING_INTERP_START 290
#define FSTRING_INTERP_END 291
#define FSTRING_TEXT 292
#define APPLICATION 293
#define DOUBLE_PIPE 294
#define DOUBLE_AMP 295
#define NE 296
#define EQ 297
#define LE 298
#define GE 299
#define MODULO 300
#define UMINUS 301




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
#line 225 "lang/y.tab.c"
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
#line 250 "lang/y.tab.c"

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
#define YYFINAL  79
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1524

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  28
/* YYNRULES -- Number of rules.  */
#define YYNRULES  134
/* YYNRULES -- Number of states.  */
#define YYNSTATES  274

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   301

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      58,    59,    51,    49,    61,    50,    38,    52,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    54,    56,
      44,    60,    43,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    63,     2,    64,     2,    66,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    65,     2,     2,     2,    62,
       2,    67,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    57,     2,     2,    39,     2,     2,     2,     2,     2,
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
      35,    36,    37,    40,    41,    42,    45,    46,    47,    48,
      53,    55
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    11,    14,    18,    22,
      25,    28,    33,    38,    43,    48,    52,    56,    60,    64,
      68,    72,    76,    80,    84,    88,    92,    96,   100,   104,
     108,   112,   116,   118,   120,   122,   125,   128,   132,   134,
     136,   138,   140,   142,   144,   146,   148,   150,   152,   154,
     156,   158,   162,   164,   168,   173,   178,   183,   190,   195,
     202,   207,   211,   213,   220,   227,   230,   233,   236,   239,
     243,   247,   251,   257,   260,   266,   272,   279,   286,   292,
     296,   298,   302,   305,   310,   312,   318,   321,   328,   330,
     334,   336,   339,   343,   348,   353,   359,   366,   370,   374,
     378,   382,   387,   389,   393,   398,   400,   404,   409,   415,
     421,   425,   426,   429,   431,   435,   440,   443,   448,   450,
     453,   457,   461,   465,   469,   471,   474,   478,   480,   482,
     484,   488,   492,   496,   500
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      69,     0,    -1,    72,    56,    -1,    72,    -1,    -1,    71,
      -1,    57,    70,    -1,    70,    38,     5,    -1,    70,    25,
      70,    -1,    70,    71,    -1,    30,    71,    -1,    58,    51,
      59,    71,    -1,    58,    52,    59,    71,    -1,    58,    49,
      59,    71,    -1,    58,    50,    59,    71,    -1,    70,    49,
      70,    -1,    70,    50,    70,    -1,    70,    51,    70,    -1,
      70,    52,    70,    -1,    70,    53,    70,    -1,    70,    44,
      70,    -1,    70,    43,    70,    -1,    70,    42,    70,    -1,
      70,    41,    70,    -1,    70,    48,    70,    -1,    70,    47,
      70,    -1,    70,    45,    70,    -1,    70,    46,    70,    -1,
      70,    12,    70,    -1,    70,    54,    70,    -1,    70,    38,
       5,    -1,    70,    20,    70,    -1,    73,    -1,    84,    -1,
      90,    -1,    26,    70,    -1,    14,    70,    -1,     5,    29,
       5,    -1,     7,    -1,     3,    -1,     4,    -1,     8,    -1,
      10,    -1,    11,    -1,     5,    -1,    21,    -1,    79,    -1,
      80,    -1,    82,    -1,    87,    -1,     9,    -1,    58,    72,
      59,    -1,    70,    -1,    72,    56,    70,    -1,    15,    32,
      60,    70,    -1,    15,     5,    60,    70,    -1,    15,    78,
      60,    70,    -1,    15,     5,    60,    13,    16,    92,    -1,
      15,    83,    60,    70,    -1,    15,     5,    60,    58,    75,
      59,    -1,    15,    21,    60,    70,    -1,    73,    22,    70,
      -1,    76,    -1,    15,    58,     5,    59,    60,    76,    -1,
      15,    58,     5,    59,    60,    70,    -1,    27,     6,    -1,
      28,     6,    -1,    27,     5,    -1,    28,     5,    -1,    13,
      16,    70,    -1,    74,    19,    70,    -1,    74,    54,     8,
      -1,    75,    61,    74,    54,     8,    -1,    75,    61,    -1,
      16,    77,    19,    72,    56,    -1,    16,    21,    19,    72,
      56,    -1,    58,    16,    77,    19,    72,    59,    -1,    58,
      16,    21,    19,    72,    59,    -1,    62,    77,    19,    72,
      56,    -1,    62,    72,    56,    -1,    78,    -1,    78,    60,
      70,    -1,    77,    78,    -1,    77,    78,    60,    70,    -1,
      78,    -1,    78,    54,    58,    94,    59,    -1,    77,    78,
      -1,    77,    78,    54,    58,    94,    59,    -1,     5,    -1,
      58,    83,    59,    -1,    81,    -1,    63,    64,    -1,    63,
      83,    64,    -1,    63,    83,    61,    64,    -1,    63,    39,
      39,    64,    -1,    63,    39,    83,    39,    64,    -1,    63,
      39,    83,    61,    39,    64,    -1,     5,    20,     5,    -1,
       5,    20,    70,    -1,    58,    70,    59,    -1,    58,    83,
      59,    -1,    58,    83,    61,    59,    -1,    70,    -1,    83,
      61,    70,    -1,    17,    70,    18,    86,    -1,    70,    -1,
      70,    65,    70,    -1,    39,    85,    19,    70,    -1,    86,
      39,    85,    19,    70,    -1,    86,    39,    66,    19,    70,
      -1,    33,    88,    34,    -1,    -1,    88,    89,    -1,    37,
      -1,    35,    70,    36,    -1,    31,     5,    60,    94,    -1,
      31,     5,    -1,    31,    91,    60,    94,    -1,     5,    -1,
      91,     5,    -1,    94,    19,    94,    -1,    92,    19,    94,
      -1,    95,    61,    95,    -1,    93,    61,    95,    -1,    95,
      -1,    39,    95,    -1,    94,    39,    95,    -1,    92,    -1,
      93,    -1,     5,    -1,     5,    60,     3,    -1,     5,    67,
      95,    -1,     5,    54,    95,    -1,    58,    94,    59,    -1,
      21,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   120,   120,   121,   122,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,   158,   159,   160,   161,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   181,   182,   186,   187,   188,   189,   192,   195,
     202,   203,   208,   210,   217,   227,   228,   229,   230,   234,
     235,   240,   243,   246,   251,   252,   253,   254,   255,   256,
     263,   264,   265,   266,   268,   269,   270,   271,   275,   276,
     277,   287,   288,   289,   293,   294,   295,   299,   300,   304,
     305,   306,   310,   311,   315,   319,   320,   323,   324,   325,
     328,   332,   333,   337,   338,   342,   348,   354,   367,   368,
     371,   372,   376,   377,   381,   382,   383,   384,   385,   389,
     390,   391,   392,   393,   394
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INTEGER", "DOUBLE", "IDENTIFIER",
  "PATH_IDENTIFIER", "IDENTIFIER_LIST", "TOK_STRING", "TOK_CHAR", "TRUE",
  "FALSE", "PIPE", "EXTERN", "TRIPLE_DOT", "LET", "FN", "MATCH", "WITH",
  "ARROW", "DOUBLE_COLON", "TOK_VOID", "IN", "AND", "ASYNC", "DOUBLE_AT",
  "THUNK", "IMPORT", "OPEN", "IMPLEMENTS", "AMPERSAND", "TYPE", "TEST_ID",
  "FSTRING_START", "FSTRING_END", "FSTRING_INTERP_START",
  "FSTRING_INTERP_END", "FSTRING_TEXT", "'.'", "'|'", "APPLICATION",
  "DOUBLE_PIPE", "DOUBLE_AMP", "'>'", "'<'", "NE", "EQ", "LE", "GE", "'+'",
  "'-'", "'*'", "'/'", "MODULO", "':'", "UMINUS", "';'", "'yield'", "'('",
  "')'", "'='", "','", "'module'", "'['", "']'", "'if'", "'_'", "'of'",
  "$accept", "program", "expr", "simple_expr", "expr_sequence",
  "let_binding", "extern_typed_signature", "extern_variants",
  "lambda_expr", "lambda_args", "lambda_arg", "list", "array",
  "list_match_expr", "tuple", "expr_list", "match_expr",
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
     285,   286,   287,   288,   289,   290,   291,   292,    46,   124,
     293,   294,   295,    62,    60,   296,   297,   298,   299,    43,
      45,    42,    47,   300,    58,   301,    59,   121,    40,    41,
      61,    44,   109,    91,    93,   105,    95,   111
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    68,    69,    69,    69,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    72,    72,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    74,
      74,    75,    75,    75,    76,    76,    76,    76,    76,    76,
      77,    77,    77,    77,    77,    77,    77,    77,    78,    78,
      78,    79,    79,    79,    80,    80,    80,    81,    81,    82,
      82,    82,    83,    83,    84,    85,    85,    86,    86,    86,
      87,    88,    88,    89,    89,    90,    90,    90,    91,    91,
      92,    92,    93,    93,    94,    94,    94,    94,    94,    95,
      95,    95,    95,    95,    95
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     2,     3,     3,     2,
       2,     4,     4,     4,     4,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     1,     1,     1,     2,     2,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     1,     3,     4,     4,     4,     6,     4,     6,
       4,     3,     1,     6,     6,     2,     2,     2,     2,     3,
       3,     3,     5,     2,     5,     5,     6,     6,     5,     3,
       1,     3,     2,     4,     1,     5,     2,     6,     1,     3,
       1,     2,     3,     4,     4,     5,     6,     3,     3,     3,
       3,     4,     1,     3,     4,     1,     3,     4,     5,     5,
       3,     0,     2,     1,     3,     4,     2,     4,     1,     2,
       3,     3,     3,     3,     1,     2,     3,     1,     1,     1,
       3,     3,     3,     3,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    39,    40,    44,    38,    41,    50,    42,    43,     0,
       0,     0,     0,    45,     0,     0,     0,     0,     0,   111,
       0,     0,     0,     0,     0,    52,     5,     3,    32,    62,
      46,    47,    48,    33,    49,    34,     0,    36,    44,    45,
       0,     0,   102,     0,    90,     0,    88,     0,     0,     0,
      80,     0,    35,    67,    65,    68,    66,    44,     0,    10,
     116,     0,     0,     6,     0,     0,     0,     0,     0,    52,
       0,     0,    44,     0,     0,     0,     0,    91,     0,     1,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     9,     2,
       0,    37,     0,     0,     0,     0,    44,     0,     0,     0,
       0,     0,     0,     0,    82,     0,     0,     0,     0,   119,
       0,   110,     0,   113,   112,     0,     0,     0,     0,     0,
       0,    99,     0,    51,   100,     0,    79,     0,     0,     0,
       0,    92,    28,    31,     8,     7,    23,    22,    21,    20,
      26,    27,    25,    24,    15,    16,    17,    18,    19,    29,
      53,    61,    44,    98,     0,     0,    55,    60,    54,     0,
     100,    56,    58,   103,     0,    89,     0,     0,     0,     0,
      81,     0,   104,   129,   134,     0,     0,   127,   128,   115,
     124,   117,     0,     0,     0,    13,    14,    11,    12,   101,
       0,    94,     0,     0,    93,     0,     0,     0,     0,     0,
      75,    74,     0,    83,     0,   105,     0,     0,     0,     0,
       0,   125,     0,     0,     0,     0,     0,     0,   114,     0,
       0,    78,    95,     0,    57,     0,     0,     0,     0,    59,
      73,    64,    62,     0,    85,     0,     0,     0,     0,   132,
     130,   131,   133,   121,   123,   120,   126,   122,    77,    76,
      96,    69,    70,    71,     0,    87,   106,   107,     0,     0,
       0,   109,   108,    72
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    24,    25,    26,    70,    28,   207,   208,    29,    49,
      50,    30,    31,    44,    32,    71,    33,   216,   182,    34,
      62,   124,    35,    61,   187,   188,   189,   190
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -78
static const yytype_int16 yypact[] =
{
    1270,   -78,   -78,    13,   -78,   -78,   -78,   -78,   -78,  1270,
    1030,    31,  1270,   -78,  1270,    32,   137,   157,    16,   -78,
    1270,   822,  1308,   428,    28,   674,   -78,   -10,    29,   -78,
     -78,   -78,   -78,   -78,   -78,   -78,   100,   674,    -2,   103,
     110,   883,   674,   111,   -78,    93,   159,   169,  1270,     5,
     -19,   562,   674,   -78,   -78,   -78,   -78,   -78,  1270,   -78,
     135,     2,    99,   674,    39,   140,   148,   152,   154,   501,
      47,   125,    92,   822,   158,    11,  1068,   -78,    58,   -78,
    1270,  1270,  1270,   191,  1270,  1270,  1270,  1270,  1270,  1270,
    1270,  1270,  1270,  1270,  1270,  1270,  1270,  1270,   -78,  1270,
    1270,   -78,  1346,  1106,  1270,  1270,    -6,   128,  1270,  1270,
    1270,  1270,   145,  1270,    60,   160,  1270,   178,     8,   -78,
       8,   -78,  1270,   -78,   -78,   200,    12,   157,   157,   157,
     157,   -78,  1270,   -78,   -78,  1144,  1270,  1270,   165,    -5,
     467,   -78,   917,   674,   973,   -78,  1175,  1175,  1377,  1377,
    1377,  1377,  1377,  1377,   172,   172,  1391,  1391,  1447,  1461,
     674,   674,    13,   674,   205,   731,   674,   674,   674,   171,
       1,   674,   674,   674,   176,   -78,   177,   180,  1270,     8,
     674,  1270,   195,   149,   -78,    87,     8,   220,   179,     6,
     181,     6,   618,  1270,  1270,   -78,   -78,   -78,   -78,   -78,
     185,   -78,   182,  1232,   -78,     8,   227,     0,   151,  1270,
    1270,  1270,     8,   674,    79,   365,   225,   296,    87,   242,
      87,   -78,    85,     8,    87,     8,    87,    87,   -78,    73,
      81,  1270,   -78,   183,   220,     6,  1270,  1270,   240,   -78,
     241,   674,   -78,    91,   -78,  1270,  1270,   234,   236,   -78,
     -78,   -78,   -78,     6,   -78,     6,   -78,   -78,   -78,   -78,
     -78,   674,   765,   -78,     3,   -78,   674,   674,  1270,  1270,
     248,   674,   674,   -78
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -78,   -78,    -9,   122,     4,   -78,    17,   -78,    49,   -14,
      -1,   -78,   -78,   -78,   -78,    -8,   -78,    44,   -78,   -78,
     -78,   -78,   -78,   -78,    57,   -78,   -77,   -69
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -103
static const yytype_int16 yytable[] =
{
      37,    42,    45,    51,    27,    52,   -89,   119,    75,    43,
      46,    63,    69,   183,    42,    78,    46,    46,   102,   237,
     -89,    60,   237,    36,   113,   225,    74,    36,    79,   184,
     137,   194,    69,   107,   202,   115,    46,    53,    54,    42,
     112,   116,    36,   191,    46,   226,    99,   185,   114,    69,
     126,   100,    47,   169,   238,   -89,   203,   270,   103,   -89,
     125,   -89,   120,    48,    69,   107,   186,    42,   139,    48,
      48,   142,   143,   144,   114,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   158,   159,    48,
     160,   161,   183,   163,   166,   167,   168,    48,   225,   171,
     172,   173,   214,   132,   225,   101,   133,   180,   184,   222,
     225,   -88,   102,   192,   177,   174,   221,   176,   226,   140,
     178,    36,   141,   160,   226,   114,   173,   160,   235,   210,
     226,   173,   258,   121,   122,   243,   123,   211,   244,    59,
     259,   200,    55,    56,   252,   186,   253,    98,   255,   249,
     265,   251,   -88,   109,   110,   254,    69,   256,   257,    98,
       1,     2,    57,   104,    98,     5,     6,     7,     8,   213,
     105,   108,   215,    98,    98,     1,     2,    57,    13,   102,
       5,     6,     7,     8,   134,    98,   135,   170,   111,   135,
      19,    98,    81,    13,   173,   118,   145,   229,   230,   127,
     241,   160,   160,   218,   175,    19,   110,   128,   215,   219,
     239,   129,   240,   130,   136,    58,   220,   181,   179,   193,
      23,   205,   160,    94,    95,    96,    97,   261,   262,   201,
      58,   209,   210,   211,   217,    23,   266,   267,   212,   223,
     224,   231,   227,   236,   246,   250,   232,   260,   263,   195,
     196,   197,   198,   268,   206,   269,   273,   264,   242,   271,
     272,   248,   234,     0,    98,    98,    98,     0,    98,    98,
      98,    98,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    98,    98,    98,     0,    98,     0,     0,    98,    98,
      98,     0,     0,    98,    98,    98,     0,     0,     0,     1,
       2,     3,    98,     4,     5,     6,     7,     8,     0,     0,
       9,    10,    11,    12,    98,     0,     0,    13,     0,     0,
       0,     0,    14,    15,    16,     0,    17,    18,     0,    19,
       0,     0,     0,     0,     0,    98,     0,    98,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    20,    21,     0,     0,     0,    22,    23,
       0,     0,   247,    98,     0,     0,     0,     0,     1,     2,
      57,     0,     0,     5,     6,     7,     8,    80,     0,     0,
       0,     0,     0,    98,    98,    81,    13,     0,    98,    98,
      82,     0,     0,    98,    98,     0,     0,     0,    19,     0,
       0,     0,     0,    83,     0,     0,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       0,     0,     0,    58,     0,     0,     0,     0,    23,     0,
     245,     1,     2,     3,     0,     4,     5,     6,     7,     8,
       0,     0,     9,    10,    11,    12,     0,     0,     0,    13,
       0,     0,     0,     0,    14,    15,    16,     0,    17,    18,
       0,    19,     0,     0,     0,     0,     0,    76,     0,     0,
       1,     2,     3,     0,     4,     5,     6,     7,     8,     0,
       0,     9,    10,    11,    12,    20,    21,     0,    13,     0,
      22,    23,    77,    14,    15,    16,     0,    17,    18,     0,
      19,     0,     0,     0,     1,     2,    57,     0,     0,     5,
       6,     7,     8,    80,     0,     0,     0,     0,     0,     0,
       0,    81,    13,     0,    20,    21,    82,     0,     0,    22,
      23,   204,     0,     0,    19,     0,     0,     0,     0,    83,
       0,     0,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,     0,     0,     0,    58,
     131,     0,  -102,     0,    23,     1,     2,    57,     0,     0,
       5,     6,     7,     8,    80,     0,     0,     0,     0,     0,
     117,     0,    81,    13,     0,     0,     0,    82,     0,     0,
       0,     0,     0,     0,     0,    19,     0,     0,     0,     0,
      83,     0,     0,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
      58,     1,     2,    57,     0,    23,     5,     6,     7,     8,
      80,     0,     0,     0,     0,     0,     0,     0,    81,    13,
       0,     0,     0,    82,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,   228,     0,    83,     0,     0,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,     0,     0,     0,    58,     1,     2,    57,
       0,    23,     5,     6,     7,     8,    80,     0,     0,     0,
       0,     0,     0,     0,    81,    13,     0,     0,     0,    82,
       0,     0,     0,     0,     0,     0,     0,    19,     0,     0,
       0,     0,    83,     0,     0,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,     0,
       0,     0,    58,     0,     1,     2,     3,    23,     4,     5,
       6,     7,     8,     0,   206,     9,    10,    64,    12,     0,
       0,     0,    13,     0,     0,     0,     0,    14,    15,    16,
       0,    17,    18,     0,    19,     0,     0,     0,     1,     2,
      57,     0,     0,     5,     6,     7,     8,    80,     0,     0,
      65,    66,    67,    68,     0,    81,    13,     0,    20,    21,
      82,     0,     0,    22,    23,     0,     0,     0,    19,     0,
       0,     0,     0,    83,     0,     0,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,     0,
       0,     0,     0,    58,     0,     1,     2,     3,    23,     4,
       5,     6,     7,     8,     0,     0,     9,    10,    64,    12,
       0,     0,     0,    13,     0,     0,     0,     0,    14,    15,
      16,     0,    17,    18,     0,    19,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    65,    66,    67,    68,     0,     0,     0,     0,    20,
      21,     0,     0,     0,    22,    23,     1,     2,   106,     0,
       4,     5,     6,     7,     8,     0,     0,     9,    10,    64,
      12,     0,     0,     0,    13,     0,     0,     0,     0,    14,
      15,    16,     0,    17,    18,     0,    19,     0,     0,     0,
       1,     2,    57,     0,     0,     5,     6,     7,     8,     0,
       0,     0,    65,    66,    67,    68,     0,    81,    13,     0,
      20,    21,    82,     0,     0,    22,    23,     0,     0,     0,
      19,     0,     0,     0,     0,     0,     0,     0,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,     0,     0,     0,    58,     1,     2,    57,     0,
      23,     5,     6,     7,     8,     0,     0,     0,     0,     0,
       0,     0,     0,    81,    13,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    19,     0,     0,     0,
       0,     0,     0,     0,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,     0,     0,
       0,    58,     0,     1,     2,    38,    23,     4,     5,     6,
       7,     8,     0,     0,     9,    10,    11,    12,     0,     0,
       0,    39,     0,     0,     0,     0,    14,    15,    16,     0,
      17,    18,    40,    19,     0,     0,     0,     0,     0,     0,
       0,     1,     2,     3,     0,     4,     5,     6,     7,     8,
       0,     0,     9,    10,    11,    12,     0,    20,    41,    13,
       0,     0,    22,    23,    14,    15,    16,     0,    17,    18,
       0,    19,     0,     0,     0,     0,     0,   138,     0,     1,
       2,     3,     0,     4,     5,     6,     7,     8,     0,   164,
       9,    10,    11,    12,     0,    20,    21,    13,     0,     0,
      22,    23,    14,    15,    16,     0,    17,    18,     0,    19,
       0,     0,     0,     0,     0,     0,     0,     1,     2,     3,
       0,     4,     5,     6,     7,     8,     0,     0,     9,    10,
      11,    12,     0,    20,   165,    13,     0,     0,    22,    23,
      14,    15,    16,     0,    17,    18,     0,    19,     1,     2,
      57,     0,     0,     5,     6,     7,     8,     0,     0,     0,
       0,     0,     0,     0,     0,    81,    13,     0,     0,     0,
       0,    20,    21,   199,     0,     0,    22,    23,    19,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       0,     0,     0,    58,     0,     1,     2,     3,    23,     4,
       5,     6,     7,     8,     0,     0,     9,    10,    11,    12,
       0,     0,     0,    13,     0,     0,     0,     0,    14,    15,
      16,     0,    17,    18,     0,    19,     0,     0,     0,     0,
       0,   233,     0,     1,     2,     3,     0,     4,     5,     6,
       7,     8,     0,     0,     9,    10,    11,    12,     0,    20,
      21,    13,     0,     0,    22,    23,    14,    15,    16,     0,
      17,    18,     0,    19,     0,     0,     0,     0,     0,     0,
       0,     1,     2,    72,     0,     4,     5,     6,     7,     8,
       0,     0,     9,    10,    11,    12,     0,    20,    21,    13,
       0,     0,    22,    23,    14,    15,    16,     0,    17,    18,
       0,    19,     0,     0,     0,     0,     0,     0,     0,     1,
       2,   162,     0,     4,     5,     6,     7,     8,     0,     0,
       9,    10,    11,    12,     0,    20,    73,    13,     0,     0,
      22,    23,    14,    15,    16,     0,    17,    18,     0,    19,
       1,     2,    57,     0,     0,     5,     6,     7,     8,     0,
       0,     0,     0,     0,     1,     2,    57,    81,    13,     5,
       6,     7,     8,    20,    21,     0,     0,     0,    22,    23,
      19,    81,    13,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    19,     0,    92,    93,    94,    95,
      96,    97,     0,     0,     0,    58,     0,     0,     0,     0,
      23,     0,     0,     0,    96,    97,     0,     0,     0,    58,
       1,     2,    57,     0,    23,     5,     6,     7,     8,     0,
       0,     0,     0,     0,     1,     2,    57,    81,    13,     5,
       6,     7,     8,     0,     0,     0,     0,     0,     0,     0,
      19,    81,    13,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    19,     0,     0,     0,     0,     0,
       0,    97,     0,     0,     0,    58,     0,     0,     0,     0,
      23,     0,     0,     0,     0,     0,     0,     0,     0,    58,
       0,     0,     0,     0,    23
};

static const yytype_int16 yycheck[] =
{
       9,    10,    10,    12,     0,    14,     5,     5,    22,    10,
       5,    20,    21,     5,    23,    23,     5,     5,    20,    19,
      19,     5,    19,    29,    19,    19,    22,    29,     0,    21,
      19,    19,    41,    41,    39,    54,     5,     5,     6,    48,
      48,    60,    29,   120,     5,    39,    56,    39,    49,    58,
      64,    22,    21,    59,    54,    54,    61,    54,    60,    58,
      21,    60,    60,    58,    73,    73,    58,    76,    76,    58,
      58,    80,    81,    82,    75,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    58,
      99,   100,     5,   102,   103,   104,   105,    58,    19,   108,
     109,   110,   179,    56,    19,     5,    59,   116,    21,   186,
      19,    19,    20,   122,    54,   111,   185,   113,    39,    61,
      60,    29,    64,   132,    39,   126,   135,   136,   205,    56,
      39,   140,    59,    34,    35,   212,    37,    56,    59,    17,
      59,   137,     5,     6,    59,    58,   223,    25,   225,   218,
      59,   220,    60,    60,    61,   224,   165,   226,   227,    37,
       3,     4,     5,    60,    42,     8,     9,    10,    11,   178,
      60,    60,   181,    51,    52,     3,     4,     5,    21,    20,
       8,     9,    10,    11,    59,    63,    61,    59,    19,    61,
      33,    69,    20,    21,   203,    60,     5,   193,   194,    59,
     209,   210,   211,    54,    59,    33,    61,    59,   217,    60,
      59,    59,    61,    59,    56,    58,    67,    39,    58,    19,
      63,    16,   231,    51,    52,    53,    54,   236,   237,    64,
      58,    60,    56,    56,    39,    63,   245,   246,    58,    19,
      61,    56,    61,    16,    19,     3,    64,    64,     8,   127,
     128,   129,   130,    19,    13,    19,     8,   240,   209,   268,
     269,   217,   205,    -1,   142,   143,   144,    -1,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,    -1,   163,    -1,    -1,   166,   167,
     168,    -1,    -1,   171,   172,   173,    -1,    -1,    -1,     3,
       4,     5,   180,     7,     8,     9,    10,    11,    -1,    -1,
      14,    15,    16,    17,   192,    -1,    -1,    21,    -1,    -1,
      -1,    -1,    26,    27,    28,    -1,    30,    31,    -1,    33,
      -1,    -1,    -1,    -1,    -1,   213,    -1,   215,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    57,    58,    -1,    -1,    -1,    62,    63,
      -1,    -1,    66,   241,    -1,    -1,    -1,    -1,     3,     4,
       5,    -1,    -1,     8,     9,    10,    11,    12,    -1,    -1,
      -1,    -1,    -1,   261,   262,    20,    21,    -1,   266,   267,
      25,    -1,    -1,   271,   272,    -1,    -1,    -1,    33,    -1,
      -1,    -1,    -1,    38,    -1,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    -1,    -1,    58,    -1,    -1,    -1,    -1,    63,    -1,
      65,     3,     4,     5,    -1,     7,     8,     9,    10,    11,
      -1,    -1,    14,    15,    16,    17,    -1,    -1,    -1,    21,
      -1,    -1,    -1,    -1,    26,    27,    28,    -1,    30,    31,
      -1,    33,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
       3,     4,     5,    -1,     7,     8,     9,    10,    11,    -1,
      -1,    14,    15,    16,    17,    57,    58,    -1,    21,    -1,
      62,    63,    64,    26,    27,    28,    -1,    30,    31,    -1,
      33,    -1,    -1,    -1,     3,     4,     5,    -1,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    20,    21,    -1,    57,    58,    25,    -1,    -1,    62,
      63,    64,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    -1,    -1,    -1,    58,
      59,    -1,    61,    -1,    63,     3,     4,     5,    -1,    -1,
       8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,
      18,    -1,    20,    21,    -1,    -1,    -1,    25,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    33,    -1,    -1,    -1,    -1,
      38,    -1,    -1,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    -1,    -1,    -1,
      58,     3,     4,     5,    -1,    63,     8,     9,    10,    11,
      12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    21,
      -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    33,    -1,    -1,    36,    -1,    38,    -1,    -1,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    -1,    -1,    -1,    58,     3,     4,     5,
      -1,    63,     8,     9,    10,    11,    12,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    20,    21,    -1,    -1,    -1,    25,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    33,    -1,    -1,
      -1,    -1,    38,    -1,    -1,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    -1,
      -1,    -1,    58,    -1,     3,     4,     5,    63,     7,     8,
       9,    10,    11,    -1,    13,    14,    15,    16,    17,    -1,
      -1,    -1,    21,    -1,    -1,    -1,    -1,    26,    27,    28,
      -1,    30,    31,    -1,    33,    -1,    -1,    -1,     3,     4,
       5,    -1,    -1,     8,     9,    10,    11,    12,    -1,    -1,
      49,    50,    51,    52,    -1,    20,    21,    -1,    57,    58,
      25,    -1,    -1,    62,    63,    -1,    -1,    -1,    33,    -1,
      -1,    -1,    -1,    38,    -1,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    -1,
      -1,    -1,    -1,    58,    -1,     3,     4,     5,    63,     7,
       8,     9,    10,    11,    -1,    -1,    14,    15,    16,    17,
      -1,    -1,    -1,    21,    -1,    -1,    -1,    -1,    26,    27,
      28,    -1,    30,    31,    -1,    33,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    -1,    -1,    -1,    -1,    57,
      58,    -1,    -1,    -1,    62,    63,     3,     4,     5,    -1,
       7,     8,     9,    10,    11,    -1,    -1,    14,    15,    16,
      17,    -1,    -1,    -1,    21,    -1,    -1,    -1,    -1,    26,
      27,    28,    -1,    30,    31,    -1,    33,    -1,    -1,    -1,
       3,     4,     5,    -1,    -1,     8,     9,    10,    11,    -1,
      -1,    -1,    49,    50,    51,    52,    -1,    20,    21,    -1,
      57,    58,    25,    -1,    -1,    62,    63,    -1,    -1,    -1,
      33,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    -1,    -1,    58,     3,     4,     5,    -1,
      63,     8,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    20,    21,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    33,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    -1,
      -1,    58,    -1,     3,     4,     5,    63,     7,     8,     9,
      10,    11,    -1,    -1,    14,    15,    16,    17,    -1,    -1,
      -1,    21,    -1,    -1,    -1,    -1,    26,    27,    28,    -1,
      30,    31,    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,    -1,     7,     8,     9,    10,    11,
      -1,    -1,    14,    15,    16,    17,    -1,    57,    58,    21,
      -1,    -1,    62,    63,    26,    27,    28,    -1,    30,    31,
      -1,    33,    -1,    -1,    -1,    -1,    -1,    39,    -1,     3,
       4,     5,    -1,     7,     8,     9,    10,    11,    -1,    13,
      14,    15,    16,    17,    -1,    57,    58,    21,    -1,    -1,
      62,    63,    26,    27,    28,    -1,    30,    31,    -1,    33,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
      -1,     7,     8,     9,    10,    11,    -1,    -1,    14,    15,
      16,    17,    -1,    57,    58,    21,    -1,    -1,    62,    63,
      26,    27,    28,    -1,    30,    31,    -1,    33,     3,     4,
       5,    -1,    -1,     8,     9,    10,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    20,    21,    -1,    -1,    -1,
      -1,    57,    58,    59,    -1,    -1,    62,    63,    33,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    -1,    -1,    58,    -1,     3,     4,     5,    63,     7,
       8,     9,    10,    11,    -1,    -1,    14,    15,    16,    17,
      -1,    -1,    -1,    21,    -1,    -1,    -1,    -1,    26,    27,
      28,    -1,    30,    31,    -1,    33,    -1,    -1,    -1,    -1,
      -1,    39,    -1,     3,     4,     5,    -1,     7,     8,     9,
      10,    11,    -1,    -1,    14,    15,    16,    17,    -1,    57,
      58,    21,    -1,    -1,    62,    63,    26,    27,    28,    -1,
      30,    31,    -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,    -1,     7,     8,     9,    10,    11,
      -1,    -1,    14,    15,    16,    17,    -1,    57,    58,    21,
      -1,    -1,    62,    63,    26,    27,    28,    -1,    30,    31,
      -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,    -1,     7,     8,     9,    10,    11,    -1,    -1,
      14,    15,    16,    17,    -1,    57,    58,    21,    -1,    -1,
      62,    63,    26,    27,    28,    -1,    30,    31,    -1,    33,
       3,     4,     5,    -1,    -1,     8,     9,    10,    11,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,    20,    21,     8,
       9,    10,    11,    57,    58,    -1,    -1,    -1,    62,    63,
      33,    20,    21,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    33,    -1,    49,    50,    51,    52,
      53,    54,    -1,    -1,    -1,    58,    -1,    -1,    -1,    -1,
      63,    -1,    -1,    -1,    53,    54,    -1,    -1,    -1,    58,
       3,     4,     5,    -1,    63,     8,     9,    10,    11,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,    20,    21,     8,
       9,    10,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      33,    20,    21,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    33,    -1,    -1,    -1,    -1,    -1,
      -1,    54,    -1,    -1,    -1,    58,    -1,    -1,    -1,    -1,
      63,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      -1,    -1,    -1,    -1,    63
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     7,     8,     9,    10,    11,    14,
      15,    16,    17,    21,    26,    27,    28,    30,    31,    33,
      57,    58,    62,    63,    69,    70,    71,    72,    73,    76,
      79,    80,    82,    84,    87,    90,    29,    70,     5,    21,
      32,    58,    70,    78,    81,    83,     5,    21,    58,    77,
      78,    70,    70,     5,     6,     5,     6,     5,    58,    71,
       5,    91,    88,    70,    16,    49,    50,    51,    52,    70,
      72,    83,     5,    58,    72,    77,    39,    64,    83,     0,
      12,    20,    25,    38,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    71,    56,
      22,     5,    20,    60,    60,    60,     5,    83,    60,    60,
      61,    19,    83,    19,    78,    54,    60,    18,    60,     5,
      60,    34,    35,    37,    89,    21,    77,    59,    59,    59,
      59,    59,    56,    59,    59,    61,    56,    19,    39,    83,
      61,    64,    70,    70,    70,     5,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,     5,    70,    13,    58,    70,    70,    70,    59,
      59,    70,    70,    70,    72,    59,    72,    54,    60,    58,
      70,    39,    86,     5,    21,    39,    58,    92,    93,    94,
      95,    94,    70,    19,    19,    71,    71,    71,    71,    59,
      72,    64,    39,    61,    64,    16,    13,    74,    75,    60,
      56,    56,    58,    70,    94,    70,    85,    39,    54,    60,
      67,    95,    94,    19,    61,    19,    39,    61,    36,    72,
      72,    56,    64,    39,    92,    94,    16,    19,    54,    59,
      61,    70,    76,    94,    59,    65,    19,    66,    85,    95,
       3,    95,    59,    94,    95,    94,    95,    95,    59,    59,
      64,    70,    70,     8,    74,    59,    70,    70,    19,    19,
      54,    70,    70,     8
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
#line 120 "lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (2)].ast_node_ptr)); }
    break;

  case 3:
#line 121 "lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 6:
#line 129 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_yield((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 7:
#line 130 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access((yyvsp[(1) - (3)].ast_node_ptr), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 8:
#line 131 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 9:
#line 132 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 10:
#line 133 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_unop(TOKEN_AMPERSAND, (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 11:
#line 134 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application(ast_identifier((ObjString){"*", 1}), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 12:
#line 135 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application(ast_identifier((ObjString){"/", 1}), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 13:
#line 136 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application(ast_identifier((ObjString){"+", 1}), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 14:
#line 137 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application(ast_identifier((ObjString){"-", 1}), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 15:
#line 138 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 16:
#line 139 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 17:
#line 140 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 18:
#line 141 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 19:
#line 142 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 20:
#line 143 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 21:
#line 144 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 22:
#line 145 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_AMP, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 23:
#line 146 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_PIPE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 24:
#line 147 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 25:
#line 148 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 26:
#line 149 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 27:
#line 150 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 28:
#line 151 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(3) - (3)].ast_node_ptr), (yyvsp[(1) - (3)].ast_node_ptr)); }
    break;

  case 29:
#line 152 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assoc((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 30:
#line 153 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access((yyvsp[(1) - (3)].ast_node_ptr), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 31:
#line 154 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 32:
#line 155 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 33:
#line 156 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 34:
#line 157 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 35:
#line 158 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_thunk_expr((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 36:
#line 159 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_spread_operator((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 37:
#line 160 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_implements((yyvsp[(1) - (3)].vident), (yyvsp[(3) - (3)].vident)); }
    break;

  case 38:
#line 161 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_typed_empty_list((yyvsp[(1) - (1)].vident)); }
    break;

  case 39:
#line 165 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[(1) - (1)].vint)); }
    break;

  case 40:
#line 166 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[(1) - (1)].vdouble)); }
    break;

  case 41:
#line 167 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 42:
#line 168 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); }
    break;

  case 43:
#line 169 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); }
    break;

  case 44:
#line 170 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 45:
#line 171 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;

  case 46:
#line 172 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 47:
#line 173 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 48:
#line 174 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 49:
#line 175 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 50:
#line 176 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_char((yyvsp[(1) - (1)].vchar)); }
    break;

  case 51:
#line 177 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 52:
#line 181 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 53:
#line 182 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 54:
#line 186 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_test_module((yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 55:
#line 187 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 56:
#line 188 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 57:
#line 190 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), ast_extern_fn((yyvsp[(2) - (6)].vident), (yyvsp[(6) - (6)].ast_node_ptr)), NULL); }
    break;

  case 58:
#line 192 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);}
    break;

  case 59:
#line 196 "lang/parser.y"
    {
                                      Ast *variants = (yyvsp[(5) - (6)].ast_node_ptr);
                                      variants->tag = AST_EXTERN_VARIANTS;
                                      (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), variants, NULL);
                                    }
    break;

  case 60:
#line 202 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(4) - (4)].ast_node_ptr); }
    break;

  case 61:
#line 203 "lang/parser.y"
    {
                                      Ast *let = (yyvsp[(1) - (3)].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[(3) - (3)].ast_node_ptr);
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 62:
#line 208 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 63:
#line 211 "lang/parser.y"
    {
                                      Ast *id = ast_identifier((yyvsp[(3) - (6)].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[(6) - (6)].ast_node_ptr), NULL);
                                    }
    break;

  case 64:
#line 218 "lang/parser.y"
    {
                                      Ast *id = ast_identifier((yyvsp[(3) - (6)].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[(6) - (6)].ast_node_ptr), NULL);
                                    }
    break;

  case 65:
#line 227 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), false); }
    break;

  case 66:
#line 228 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), true); }
    break;

  case 67:
#line 229 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), false); }
    break;

  case 68:
#line 230 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), true); }
    break;

  case 69:
#line 234 "lang/parser.y"
    { (yyval.ast_node_ptr) = extern_typed_signature((yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 70:
#line 236 "lang/parser.y"
    { (yyval.ast_node_ptr) = extern_typed_signature_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 71:
#line 241 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list(ast_extern_fn((yyvsp[(3) - (3)].vstr), (yyvsp[(1) - (3)].ast_node_ptr))); }
    break;

  case 72:
#line 244 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (5)].ast_node_ptr), ast_extern_fn((yyvsp[(5) - (5)].vstr), (yyvsp[(3) - (5)].ast_node_ptr))); }
    break;

  case 73:
#line 246 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (2)].ast_node_ptr); }
    break;

  case 74:
#line 251 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 75:
#line 252 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 76:
#line 253 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(3) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 77:
#line 254 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 78:
#line 255 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_module(ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr))); }
    break;

  case 79:
#line 256 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_module(ast_lambda(NULL, (yyvsp[(2) - (3)].ast_node_ptr))); }
    break;

  case 80:
#line 263 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 81:
#line 264 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 82:
#line 265 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 83:
#line 266 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 84:
#line 268 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 85:
#line 269 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 86:
#line 270 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 87:
#line 271 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (6)].ast_node_ptr), (yyvsp[(2) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 88:
#line 275 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 89:
#line 276 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 90:
#line 277 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 91:
#line 287 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 92:
#line 288 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 93:
#line 289 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (4)].ast_node_ptr); }
    break;

  case 94:
#line 293 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_array(); }
    break;

  case 95:
#line 294 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (5)].ast_node_ptr)); }
    break;

  case 96:
#line 295 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (6)].ast_node_ptr)); }
    break;

  case 97:
#line 299 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 98:
#line 300 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 99:
#line 304 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 100:
#line 305 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 101:
#line 306 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)); }
    break;

  case 102:
#line 310 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 103:
#line 311 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 104:
#line 315 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 105:
#line 319 "lang/parser.y"
    {(yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr);}
    break;

  case 106:
#line 320 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr));}
    break;

  case 107:
#line 323 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 108:
#line 324 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(3) - (5)].ast_node_ptr), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 109:
#line 325 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 110:
#line 328 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 111:
#line 332 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 112:
#line 333 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 113:
#line 337 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 114:
#line 338 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 115:
#line 342 "lang/parser.y"
    {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
    break;

  case 116:
#line 348 "lang/parser.y"
    {
                                      Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (2)].vident)), NULL, NULL);
                                      type_decl->tag = AST_TYPE_DECL;
                                      (yyval.ast_node_ptr) = type_decl;
                                   }
    break;

  case 117:
#line 354 "lang/parser.y"
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

  case 118:
#line 367 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list(ast_identifier((yyvsp[(1) - (1)].vident)), NULL); }
    break;

  case 119:
#line 368 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), ast_identifier((yyvsp[(2) - (2)].vident)), NULL); }
    break;

  case 120:
#line 371 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 121:
#line 372 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 122:
#line 376 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 123:
#line 377 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 124:
#line 381 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 125:
#line 382 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 126:
#line 383 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 127:
#line 384 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_signature_of_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 128:
#line 385 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 129:
#line 389 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 130:
#line 390 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(1) - (3)].vident)), AST_CONST(AST_INT, (yyvsp[(3) - (3)].vint)), NULL); }
    break;

  case 131:
#line 391 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 132:
#line 392 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assoc(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 133:
#line 393 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 134:
#line 394 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;


/* Line 1267 of yacc.c.  */
#line 2658 "lang/y.tab.c"
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


#line 396 "lang/parser.y"



void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, _cur_script);
}
#endif _LANG_TAB_H

