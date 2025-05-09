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
     FSTRING_START = 289,
     FSTRING_END = 290,
     FSTRING_INTERP_START = 291,
     FSTRING_INTERP_END = 292,
     FSTRING_TEXT = 293,
     APPLICATION = 294,
     DOUBLE_PIPE = 295,
     DOUBLE_AMP = 296,
     NE = 297,
     EQ = 298,
     LE = 299,
     GE = 300,
     MODULO = 301,
     UMINUS = 302
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
#define FSTRING_START 289
#define FSTRING_END 290
#define FSTRING_INTERP_START 291
#define FSTRING_INTERP_END 292
#define FSTRING_TEXT 293
#define APPLICATION 294
#define DOUBLE_PIPE 295
#define DOUBLE_AMP 296
#define NE 297
#define EQ 298
#define LE 299
#define GE 300
#define MODULO 301
#define UMINUS 302




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
#line 228 "lang/y.tab.c"
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
#line 253 "lang/y.tab.c"

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
#define YYFINAL  93
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2139

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  71
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  29
/* YYNRULES -- Number of rules.  */
#define YYNRULES  153
/* YYNRULES -- Number of states.  */
#define YYNSTATES  318

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   302

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      62,    63,    51,    49,    64,    50,    55,    52,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    54,    57,
      44,    61,    43,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    66,     2,    67,     2,    69,     2,     2,     2,     2,
       2,     2,    60,     2,     2,    68,     2,     2,     2,    65,
       2,    70,     2,     2,     2,     2,    59,     2,     2,     2,
       2,    58,     2,     2,    39,     2,     2,     2,     2,     2,
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
      35,    36,    37,    38,    40,    41,    42,    45,    46,    47,
      48,    53,    56
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    11,    14,    18,    21,
      25,    29,    33,    37,    41,    45,    49,    53,    57,    61,
      65,    69,    73,    77,    81,    85,    89,    91,    93,    95,
      98,   101,   105,   107,   110,   117,   122,   124,   126,   128,
     130,   132,   134,   136,   138,   140,   142,   144,   146,   150,
     157,   164,   168,   172,   176,   180,   184,   188,   192,   196,
     200,   204,   208,   212,   216,   220,   224,   228,   232,   236,
     238,   240,   244,   249,   254,   259,   266,   271,   278,   283,
     287,   289,   296,   303,   306,   309,   312,   315,   319,   323,
     327,   333,   336,   342,   348,   355,   362,   368,   372,   374,
     378,   381,   386,   388,   394,   397,   404,   406,   410,   412,
     415,   419,   424,   429,   435,   442,   446,   450,   454,   458,
     463,   465,   469,   474,   476,   480,   485,   491,   497,   501,
     502,   505,   507,   511,   516,   519,   524,   526,   529,   533,
     537,   541,   545,   547,   550,   554,   556,   558,   560,   564,
     568,   572,   576,   578
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      72,     0,    -1,    76,    57,    -1,    76,    -1,    -1,    74,
      -1,    58,    73,    -1,    73,    26,    73,    -1,    73,    74,
      -1,    73,    49,    73,    -1,    73,    50,    73,    -1,    73,
      51,    73,    -1,    73,    52,    73,    -1,    73,    53,    73,
      -1,    73,    44,    73,    -1,    73,    43,    73,    -1,    73,
      42,    73,    -1,    73,    41,    73,    -1,    73,    48,    73,
      -1,    73,    47,    73,    -1,    73,    45,    73,    -1,    73,
      46,    73,    -1,    73,    13,    73,    -1,    73,    54,    73,
      -1,    73,    59,    73,    -1,    73,    21,    73,    -1,    77,
      -1,    88,    -1,    94,    -1,    27,    73,    -1,    15,    73,
      -1,     5,    30,     5,    -1,     8,    -1,     6,    73,    -1,
      60,     5,    61,    73,    23,    73,    -1,    73,    54,    61,
      73,    -1,     3,    -1,     4,    -1,     9,    -1,    11,    -1,
      12,    -1,     5,    -1,    22,    -1,    83,    -1,    84,    -1,
      86,    -1,    91,    -1,    10,    -1,    62,    76,    63,    -1,
      62,    17,    81,    20,    76,    63,    -1,    62,    17,    22,
      20,    76,    63,    -1,    62,    49,    63,    -1,    62,    50,
      63,    -1,    62,    51,    63,    -1,    62,    52,    63,    -1,
      62,    53,    63,    -1,    62,    44,    63,    -1,    62,    43,
      63,    -1,    62,    42,    63,    -1,    62,    41,    63,    -1,
      62,    48,    63,    -1,    62,    47,    63,    -1,    62,    45,
      63,    -1,    62,    46,    63,    -1,    62,    13,    63,    -1,
      62,    54,    63,    -1,    62,    21,    63,    -1,    62,    75,
      63,    -1,    74,    55,     5,    -1,     5,    -1,    73,    -1,
      76,    57,    73,    -1,    16,    33,    61,    73,    -1,    16,
       5,    61,    73,    -1,    16,    82,    61,    73,    -1,    16,
       5,    61,    14,    17,    96,    -1,    16,    87,    61,    73,
      -1,    16,     5,    61,    62,    79,    63,    -1,    16,    22,
      61,    73,    -1,    77,    23,    73,    -1,    80,    -1,    16,
      62,     5,    63,    61,    80,    -1,    16,    62,     5,    63,
      61,    73,    -1,    28,     7,    -1,    29,     7,    -1,    28,
       5,    -1,    29,     5,    -1,    14,    17,    73,    -1,    78,
      20,    73,    -1,    78,    54,     9,    -1,    79,    64,    78,
      54,     9,    -1,    79,    64,    -1,    17,    81,    20,    76,
      57,    -1,    17,    22,    20,    76,    57,    -1,    62,    17,
      81,    20,    76,    63,    -1,    62,    17,    22,    20,    76,
      63,    -1,    65,    81,    20,    76,    57,    -1,    65,    76,
      57,    -1,    82,    -1,    82,    61,    73,    -1,    81,    82,
      -1,    81,    82,    61,    73,    -1,    82,    -1,    82,    54,
      62,    98,    63,    -1,    81,    82,    -1,    81,    82,    54,
      62,    98,    63,    -1,     5,    -1,    62,    87,    63,    -1,
      85,    -1,    66,    67,    -1,    66,    87,    67,    -1,    66,
      87,    64,    67,    -1,    66,    39,    39,    67,    -1,    66,
      39,    87,    39,    67,    -1,    66,    39,    87,    64,    39,
      67,    -1,     5,    21,     5,    -1,     5,    21,    73,    -1,
      62,    73,    63,    -1,    62,    87,    63,    -1,    62,    87,
      64,    63,    -1,    73,    -1,    87,    64,    73,    -1,    18,
      73,    19,    90,    -1,    73,    -1,    73,    68,    73,    -1,
      39,    89,    20,    73,    -1,    90,    39,    89,    20,    73,
      -1,    90,    39,    69,    20,    73,    -1,    34,    92,    35,
      -1,    -1,    92,    93,    -1,    38,    -1,    36,    73,    37,
      -1,    32,     5,    61,    98,    -1,    32,     5,    -1,    32,
      95,    61,    98,    -1,     5,    -1,    95,     5,    -1,    98,
      20,    98,    -1,    96,    20,    98,    -1,    99,    64,    99,
      -1,    97,    64,    99,    -1,    99,    -1,    39,    99,    -1,
      98,    39,    99,    -1,    96,    -1,    97,    -1,     5,    -1,
       5,    61,     3,    -1,     5,    70,    99,    -1,     5,    54,
      99,    -1,    62,    98,    63,    -1,    22,    -1,     5,    55,
       5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   123,   123,   124,   125,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   163,   172,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   207,   208,   212,
     235,   236,   240,   241,   242,   243,   246,   249,   256,   257,
     262,   264,   271,   281,   282,   283,   284,   288,   289,   294,
     297,   300,   305,   306,   307,   308,   309,   310,   317,   318,
     319,   320,   322,   323,   324,   325,   329,   330,   331,   341,
     342,   343,   347,   348,   349,   353,   354,   358,   359,   360,
     364,   365,   369,   373,   374,   377,   378,   379,   382,   386,
     387,   391,   392,   396,   402,   408,   421,   422,   425,   426,
     430,   431,   435,   436,   437,   438,   439,   443,   444,   445,
     446,   447,   448,   449
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
  "AMPERSAND", "TYPE", "TEST_ID", "FSTRING_START", "FSTRING_END",
  "FSTRING_INTERP_START", "FSTRING_INTERP_END", "FSTRING_TEXT", "'|'",
  "APPLICATION", "DOUBLE_PIPE", "DOUBLE_AMP", "'>'", "'<'", "NE", "EQ",
  "LE", "GE", "'+'", "'-'", "'*'", "'/'", "MODULO", "':'", "'.'", "UMINUS",
  "';'", "'yield'", "'to'", "'for'", "'='", "'('", "')'", "','",
  "'module'", "'['", "']'", "'if'", "'_'", "'of'", "$accept", "program",
  "expr", "simple_expr", "custom_binop", "expr_sequence", "let_binding",
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   124,
     294,   295,   296,    62,    60,   297,   298,   299,   300,    43,
      45,    42,    47,   301,    58,    46,   302,    59,   121,   116,
     102,    61,    40,    41,    44,   109,    91,    93,   105,    95,
     111
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    71,    72,    72,    72,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    75,
      76,    76,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    78,    78,    79,
      79,    79,    80,    80,    80,    80,    80,    80,    81,    81,
      81,    81,    81,    81,    81,    81,    82,    82,    82,    83,
      83,    83,    84,    84,    84,    85,    85,    86,    86,    86,
      87,    87,    88,    89,    89,    90,    90,    90,    91,    92,
      92,    93,    93,    94,    94,    94,    95,    95,    96,    96,
      97,    97,    98,    98,    98,    98,    98,    99,    99,    99,
      99,    99,    99,    99
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     2,     3,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     1,     2,
       2,     3,     1,     2,     6,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     6,
       6,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     1,
       1,     3,     4,     4,     4,     6,     4,     6,     4,     3,
       1,     6,     6,     2,     2,     2,     2,     3,     3,     3,
       5,     2,     5,     5,     6,     6,     5,     3,     1,     3,
       2,     4,     1,     5,     2,     6,     1,     3,     1,     2,
       3,     4,     4,     5,     6,     3,     3,     3,     3,     4,
       1,     3,     4,     1,     3,     4,     5,     5,     3,     0,
       2,     1,     3,     4,     2,     4,     1,     2,     3,     3,
       3,     3,     1,     2,     3,     1,     1,     1,     3,     3,
       3,     3,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    36,    37,    41,     0,    32,    38,    47,    39,    40,
       0,     0,     0,     0,    42,     0,     0,     0,     0,   129,
       0,     0,     0,     0,     0,     0,    70,     5,     3,    26,
      80,    43,    44,    45,    27,    46,    28,     0,    33,    30,
      41,    42,     0,     0,   120,     0,   108,     0,   106,     0,
       0,     0,    98,     0,    29,    85,    83,    86,    84,   134,
       0,     0,     6,     0,    41,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    70,     0,     0,     0,    41,     0,     0,     0,
       0,   109,     0,     1,    41,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     8,     0,     2,     0,    31,     0,
       0,     0,     0,    41,     0,     0,     0,     0,     0,     0,
       0,   100,     0,     0,     0,     0,   137,     0,   128,     0,
     131,   130,     0,    64,     0,     0,    66,    59,    58,    57,
      56,    62,    63,    61,    60,    51,    52,    53,    54,    55,
      65,   117,    67,     0,    48,   118,     0,    97,     0,     0,
       0,     0,   110,    22,    25,     7,    17,    16,    15,    14,
      20,    21,    19,    18,     9,    10,    11,    12,    13,     0,
      23,    24,     0,    68,    71,    79,    41,   116,     0,     0,
      73,    78,    72,     0,   118,    74,    76,   121,     0,   107,
       0,     0,     0,     0,    99,     0,   122,   147,   152,     0,
       0,   145,   146,   133,   142,   135,     0,     0,     0,     0,
     119,     0,   112,     0,     0,   111,    35,     0,     0,     0,
       0,     0,     0,     0,    93,    92,     0,   101,     0,   123,
       0,     0,     0,     0,     0,     0,   143,     0,     0,     0,
       0,     0,     0,   132,     0,     0,     0,    96,   113,     0,
       0,     0,    75,     0,     0,     0,     0,    77,    91,    82,
      80,     0,   103,     0,     0,     0,     0,   150,   153,   148,
     149,   151,   139,   141,   138,   144,   140,    34,    50,    49,
     114,     0,     0,    87,    88,    89,     0,   105,   124,   125,
       0,     0,    50,    49,     0,   127,   126,    90
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    25,    26,    27,    83,    84,    29,   241,   242,    30,
      51,    52,    31,    32,    46,    33,    85,    34,   250,   216,
      35,    61,   141,    36,    60,   221,   222,   223,   224
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -109
static const yytype_int16 yypact[] =
{
    1818,  -109,  -109,    27,  1818,  -109,  -109,  -109,  -109,  -109,
    1818,  1401,     0,  1818,  -109,  1818,    51,   153,    22,  -109,
    1818,    33,   721,  1882,   527,    66,  1157,    55,    16,    54,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,    77,  1157,  1157,
     104,    71,   107,   785,  1157,   132,  -109,   122,   173,   176,
    1818,     5,    14,   977,  1157,  -109,  -109,  -109,  -109,   137,
      18,   146,  1157,   138,    27,   139,     7,   140,   141,   142,
     143,   144,   147,   149,   150,   151,   152,   155,   156,   157,
     158,   159,   913,   160,    67,   -27,    29,   721,   167,     8,
    1465,  -109,   123,  -109,  -109,  1818,  1818,  1818,  1818,  1818,
    1818,  1818,  1818,  1818,  1818,  1818,  1818,  1818,  1818,  1818,
    1818,  1529,  1818,   849,    55,   195,  1818,  1818,  -109,  1946,
    1593,  1818,  1818,     2,    64,  1818,  1818,  1818,  1818,   113,
    1818,    89,   154,  1818,   162,     9,  -109,     9,  -109,  1818,
    -109,  -109,  1818,  -109,   189,    10,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,  -109,
    -109,  -109,  -109,  1818,  -109,  -109,  1657,  1818,  1818,   161,
      75,   592,  -109,  1217,  1157,  1337,  1690,  1690,   233,   233,
     233,   233,   233,   233,  1979,  1979,  2013,  2013,  2039,  1818,
    2073,  1157,    12,  -109,  1157,  1157,    27,  1157,   208,   657,
    1157,  1157,  1157,   165,    -1,  1157,  1157,  1157,   170,  -109,
     177,   171,  1818,     9,  1157,  1818,   196,    87,  -109,    19,
       9,   226,   184,    15,   185,    15,  1037,  1097,  1818,  1818,
    -109,   193,  -109,   186,  1754,  -109,  1157,   231,    58,     9,
     235,    31,   128,  1818,  1818,  1818,     9,  1157,    13,   461,
     236,   394,    19,   252,   255,    19,  -109,    25,     9,    19,
       9,    19,    19,  -109,  1818,    83,    92,  1818,  -109,   192,
    1818,  1818,   226,    15,  1818,  1818,   253,  -109,   247,  1157,
    -109,   117,  -109,  1818,  1818,   244,   245,  -109,  -109,  -109,
    -109,  -109,    15,  -109,    15,  -109,  -109,  1157,  -109,  -109,
    -109,   112,   115,  1157,  1277,  -109,    35,  -109,  1157,  1157,
    1818,  1818,  -109,  -109,   257,  1157,  1157,  -109
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -109,  -109,    -4,   135,  -109,     3,  -109,   -10,  -109,    26,
     -22,    -9,  -109,  -109,  -109,  -109,    -3,  -109,    21,  -109,
    -109,  -109,  -109,  -109,  -109,    36,  -109,   -94,  -108
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -121
static const yytype_int16 yytable[] =
{
      38,    89,    45,    28,  -107,    48,    39,    44,    47,    53,
      48,    54,    48,    48,   217,    48,    62,    48,    82,  -107,
      44,    92,    49,   136,   217,   130,    88,    59,   168,   144,
     229,   218,    37,   260,   237,   260,   165,   166,    63,    82,
     124,   218,   131,   225,   145,   260,    44,   129,   219,  -106,
     119,   275,   261,  -107,   261,   275,    55,    37,    56,    37,
    -107,  -107,    50,    48,   261,   203,    93,    50,   132,    50,
      50,   220,    50,   116,    50,   133,   282,   117,   271,   137,
     131,   220,   118,    82,   124,   276,    44,   170,   291,   314,
    -106,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   190,   191,    82,
     115,   256,   194,   195,   233,   197,   200,   201,   202,   248,
      50,   205,   206,   207,   163,   119,   257,   204,   166,   214,
     164,   208,   121,   210,    37,   226,   131,   260,   227,   234,
     244,   252,   253,   211,   287,   273,   298,   290,   254,   245,
     212,   293,   281,   295,   296,   299,   261,   255,    57,   194,
      58,   114,   207,   194,   292,   120,   294,   207,   122,   244,
     238,   231,   245,   114,   114,   312,   209,   127,   313,   114,
     307,   138,   139,   126,   140,   236,   127,   171,   114,   114,
     172,   277,   278,   125,   119,    82,   128,   114,   135,   142,
     193,   215,   143,   146,   147,   148,   149,   150,   247,   228,
     151,   249,   152,   153,   154,   155,   213,   114,   156,   157,
     158,   159,   160,   162,   167,   239,   243,   244,   232,   131,
     207,   265,   266,   246,   245,   251,     1,     2,    94,   279,
     194,   194,     6,     7,     8,     9,   258,   249,   259,   262,
     267,   270,   274,   268,    96,    14,   284,   288,   289,   300,
     297,   240,   305,   194,   310,   311,   317,    19,   306,   280,
     303,   304,   286,   301,   302,   272,     0,     0,     0,   308,
     309,     0,   106,   107,   108,   109,   110,   111,     0,     0,
       0,     0,   112,     0,     0,   113,     0,     0,     0,    24,
       0,     0,     0,     0,     0,     0,   315,   316,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,     0,   114,   114,     0,     0,   114,
     114,     0,   114,     0,     0,   114,   114,   114,     0,     0,
     114,   114,   114,     0,     0,     0,     0,     0,     0,   114,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   114,   114,     0,     0,     0,     0,     0,     0,     0,
       0,   114,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   114,     0,   114,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     1,     2,     3,
       4,     0,     5,     6,     7,     8,     9,     0,     0,    10,
      11,    12,    13,     0,   114,     0,    14,     0,     0,     0,
       0,    15,    16,    17,     0,     0,    18,     0,    19,     0,
       0,     0,   114,     0,     0,     0,     0,     0,   114,   114,
       0,     0,     0,   114,   114,     0,     0,     0,     0,     0,
     114,   114,    20,     0,    21,     0,    22,     0,     0,    23,
      24,     0,     0,   285,     1,     2,    94,     0,     0,     0,
       6,     7,     8,     9,    95,     0,     0,     0,     0,     0,
       0,     0,    96,    14,     0,     0,     0,    97,     0,     0,
       0,     0,     0,     0,     0,    19,     0,     0,     0,     0,
       0,     0,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,     0,     0,     0,     0,
     112,     0,     0,   113,     0,     0,     0,    24,     0,   283,
       1,     2,     3,     4,     0,     5,     6,     7,     8,     9,
       0,     0,    10,    11,    12,    13,     0,     0,     0,    14,
       0,     0,     0,     0,    15,    16,    17,     0,     0,    18,
       0,    19,     0,     0,     0,     0,    90,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    20,     0,    21,     0,    22,
       0,     0,    23,    24,    91,     1,     2,     3,     4,     0,
       5,     6,     7,     8,     9,     0,     0,    10,    11,    12,
      13,     0,     0,     0,    14,     0,     0,     0,     0,    15,
      16,    17,     0,     0,    18,     0,    19,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      20,     0,    21,     0,    22,     0,     0,    23,    24,   235,
       1,     2,    64,     4,     0,     5,     6,     7,     8,     9,
      65,   240,    10,    11,    66,    13,     0,     0,    67,    14,
       0,     0,     0,     0,    15,    16,    17,     0,     0,    18,
       0,    19,     0,     0,     0,     0,     0,     0,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,     0,     0,     0,    20,     0,    21,     0,    22,
       0,     0,    23,    24,     1,     2,    64,     4,     0,     5,
       6,     7,     8,     9,    65,     0,    10,    11,    66,    13,
       0,     0,    67,    14,     0,     0,     0,     0,    15,    16,
      17,     0,     0,    18,     0,    19,     0,     0,     0,     0,
       0,     0,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,     0,     0,     0,    20,
       0,    21,     0,    22,     0,     0,    23,    24,     1,     2,
     123,     4,     0,     5,     6,     7,     8,     9,    65,     0,
      10,    11,    66,    13,     0,     0,    67,    14,     0,     0,
       0,     0,    15,    16,    17,     0,     0,    18,     0,    19,
       0,     0,     0,     0,     0,     0,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
       0,     0,     0,    20,     0,    21,     0,    22,     0,     0,
      23,    24,     1,     2,    64,     4,     0,     5,     6,     7,
       8,     9,    65,     0,    10,    11,   192,    13,     0,     0,
      67,    14,     0,     0,     0,     0,    15,    16,    17,     0,
       0,    18,     0,    19,     0,     0,     0,     0,     0,     0,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,     0,     0,     0,    20,     0,    21,
       0,    22,     0,     0,    23,    24,     1,     2,    94,     0,
       0,     0,     6,     7,     8,     9,    95,     0,     0,     0,
       0,     0,     0,     0,    96,    14,     0,     0,     0,    97,
       0,     0,     0,     0,     0,     0,     0,    19,     0,     0,
       0,     0,     0,     0,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,     0,     0,
       0,     0,   112,     0,     0,   113,   161,  -120,     0,    24,
       1,     2,    94,     0,     0,     0,     6,     7,     8,     9,
      95,     0,     0,     0,     0,     0,   134,     0,    96,    14,
       0,     0,     0,    97,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,     0,     0,     0,     0,   112,     0,     0,   113,
       1,     2,    94,    24,     0,     0,     6,     7,     8,     9,
      95,     0,     0,     0,     0,     0,     0,     0,    96,    14,
       0,     0,     0,    97,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,   263,     0,     0,     0,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,     0,     0,     0,     0,   112,     0,     0,   113,
       1,     2,    94,    24,     0,     0,     6,     7,     8,     9,
      95,     0,     0,     0,     0,     0,     0,     0,    96,    14,
     264,     0,     0,    97,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,     0,     0,     0,     0,   112,     0,     0,   113,
       1,     2,    94,    24,     0,     0,     6,     7,     8,     9,
      95,     0,     0,     0,     0,     0,     0,     0,    96,    14,
       0,     0,     0,    97,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,     0,     0,     0,     0,   112,     0,     0,   113,
       1,     2,    94,    24,     0,     0,     6,     7,     8,     9,
       0,     0,     0,     0,     0,     0,     0,     0,    96,    14,
       0,     0,     0,    97,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,     0,     0,     0,     0,   112,     0,     0,   113,
       1,     2,    94,    24,     0,     0,     6,     7,     8,     9,
      95,     0,     0,     0,     0,     0,     0,     0,    96,    14,
       0,     0,     0,    97,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,     0,     0,     0,     0,     0,   112,     0,     0,   113,
       1,     2,    94,    24,     0,     0,     6,     7,     8,     9,
       0,     0,     0,     0,     0,     0,     0,     0,    96,    14,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,     0,     0,     0,     0,   112,     0,     0,   113,
       0,     0,     0,    24,     1,     2,    40,     4,     0,     5,
       6,     7,     8,     9,     0,     0,    10,    11,    12,    13,
       0,     0,     0,    41,     0,     0,     0,     0,    15,    16,
      17,     0,     0,    18,    42,    19,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    20,
       0,    21,     0,    43,     0,     0,    23,    24,     1,     2,
       3,     4,     0,     5,     6,     7,     8,     9,     0,     0,
      10,    11,    12,    13,     0,     0,     0,    14,     0,     0,
       0,     0,    15,    16,    17,     0,     0,    18,     0,    19,
       0,     0,     0,     0,   169,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    20,     0,    21,     0,    22,     0,     0,
      23,    24,     1,     2,     3,     4,     0,     5,     6,     7,
       8,     9,     0,     0,    10,    11,    12,    13,     0,     0,
       0,    14,     0,     0,     0,     0,    15,    16,    17,     0,
       0,    18,     0,    19,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    20,     0,    21,
     189,    22,     0,     0,    23,    24,     1,     2,     3,     4,
       0,     5,     6,     7,     8,     9,     0,   198,    10,    11,
      12,    13,     0,     0,     0,    14,     0,     0,     0,     0,
      15,    16,    17,     0,     0,    18,     0,    19,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    20,     0,    21,     0,   199,     0,     0,    23,    24,
       1,     2,     3,     4,     0,     5,     6,     7,     8,     9,
       0,     0,    10,    11,    12,    13,     0,     0,     0,    14,
       0,     0,     0,     0,    15,    16,    17,     0,     0,    18,
       0,    19,     0,     1,     2,    94,     0,     0,     0,     6,
       7,     8,     9,     0,     0,     0,     0,     0,     0,     0,
       0,    96,    14,     0,     0,    20,     0,    21,     0,    22,
     230,     0,    23,    24,    19,     0,     0,     0,     0,     0,
       0,     0,     0,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,     0,     0,     0,     0,   112,
       0,     0,   113,     0,     0,     0,    24,     1,     2,     3,
       4,     0,     5,     6,     7,     8,     9,     0,     0,    10,
      11,    12,    13,     0,     0,     0,    14,     0,     0,     0,
       0,    15,    16,    17,     0,     0,    18,     0,    19,     0,
       0,     0,     0,   269,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    20,     0,    21,     0,    22,     0,     0,    23,
      24,     1,     2,     3,     4,     0,     5,     6,     7,     8,
       9,     0,     0,    10,    11,    12,    13,     0,     0,     0,
      14,     0,     0,     0,     0,    15,    16,    17,     0,     0,
      18,     0,    19,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    20,     0,    21,     0,
      22,     0,     0,    23,    24,     1,     2,    86,     4,     0,
       5,     6,     7,     8,     9,     0,     0,    10,    11,    12,
      13,     0,     0,     0,    14,     0,     0,     0,     0,    15,
      16,    17,     0,     0,    18,     0,    19,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      20,     0,    21,     0,    87,     0,     0,    23,    24,     1,
       2,   196,     4,     0,     5,     6,     7,     8,     9,     0,
       0,    10,    11,    12,    13,     0,     0,     0,    14,     0,
       0,     0,     0,    15,    16,    17,     0,     0,    18,     0,
      19,     0,     1,     2,    94,     0,     0,     0,     6,     7,
       8,     9,     0,     0,     0,     0,     0,     0,     0,     0,
      96,    14,     0,     0,    20,     0,    21,     0,    22,     0,
       0,    23,    24,    19,     0,     0,     1,     2,    94,     0,
       0,     0,     6,     7,     8,     9,     0,     0,     0,     0,
     108,   109,   110,   111,    96,    14,     0,     0,   112,     0,
       0,   113,     1,     2,    94,    24,     0,    19,     6,     7,
       8,     9,     0,     0,     0,     0,     0,     0,     0,     0,
      96,    14,     0,     0,     0,     0,   110,   111,     0,     0,
       0,     0,   112,    19,     0,   113,     1,     2,    94,    24,
       0,     0,     6,     7,     8,     9,     0,     0,     0,     0,
       0,     0,     0,   111,    96,    14,     0,     0,   112,     0,
       0,   113,     0,     0,     0,    24,     0,    19,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   112,     0,     0,   113,     0,     0,     0,    24
};

static const yytype_int16 yycheck[] =
{
       4,    23,    11,     0,     5,     5,    10,    11,    11,    13,
       5,    15,     5,     5,     5,     5,    20,     5,    22,    20,
      24,    24,    22,     5,     5,    20,    23,     5,    20,    22,
      20,    22,    30,    20,    22,    20,    63,    64,     5,    43,
      43,    22,    51,   137,    66,    20,    50,    50,    39,    20,
      21,    20,    39,    54,    39,    20,     5,    30,     7,    30,
      61,    62,    62,     5,    39,    63,     0,    62,    54,    62,
      62,    62,    62,    57,    62,    61,    63,    23,    20,    61,
      89,    62,     5,    87,    87,    54,    90,    90,    63,    54,
      61,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
      55,   219,   116,   117,    39,   119,   120,   121,   122,   213,
      62,   125,   126,   127,    57,    21,   220,    63,    64,   133,
      63,   128,    61,   130,    30,   139,   145,    20,   142,    64,
      57,    54,    55,    54,   252,   239,    63,   255,    61,    57,
      61,   259,   246,   261,   262,    63,    39,    70,     5,   163,
       7,    26,   166,   167,   258,    61,   260,   171,    61,    57,
     192,   168,    57,    38,    39,    63,    63,    64,    63,    44,
      63,    35,    36,    61,    38,   189,    64,    64,    53,    54,
      67,    63,    64,    61,    21,   199,    20,    62,    61,    61,
       5,    39,    63,    63,    63,    63,    63,    63,   212,    20,
      63,   215,    63,    63,    63,    63,    62,    82,    63,    63,
      63,    63,    63,    63,    57,    17,    61,    57,    67,   238,
     234,   228,   229,    62,    57,    39,     3,     4,     5,   243,
     244,   245,     9,    10,    11,    12,    20,   251,    64,    64,
      57,    20,    17,    67,    21,    22,    20,     5,     3,    67,
     264,    14,     9,   267,    20,    20,     9,    34,   278,   243,
     274,   275,   251,   270,   271,   239,    -1,    -1,    -1,   283,
     284,    -1,    49,    50,    51,    52,    53,    54,    -1,    -1,
      -1,    -1,    59,    -1,    -1,    62,    -1,    -1,    -1,    66,
      -1,    -1,    -1,    -1,    -1,    -1,   310,   311,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,    -1,   190,   191,    -1,    -1,   194,
     195,    -1,   197,    -1,    -1,   200,   201,   202,    -1,    -1,
     205,   206,   207,    -1,    -1,    -1,    -1,    -1,    -1,   214,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   226,   227,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   236,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   247,    -1,   249,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    -1,    15,
      16,    17,    18,    -1,   279,    -1,    22,    -1,    -1,    -1,
      -1,    27,    28,    29,    -1,    -1,    32,    -1,    34,    -1,
      -1,    -1,   297,    -1,    -1,    -1,    -1,    -1,   303,   304,
      -1,    -1,    -1,   308,   309,    -1,    -1,    -1,    -1,    -1,
     315,   316,    58,    -1,    60,    -1,    62,    -1,    -1,    65,
      66,    -1,    -1,    69,     3,     4,     5,    -1,    -1,    -1,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    -1,    -1,    26,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    -1,    -1,    -1,    -1,
      59,    -1,    -1,    62,    -1,    -1,    -1,    66,    -1,    68,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    -1,    15,    16,    17,    18,    -1,    -1,    -1,    22,
      -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,    32,
      -1,    34,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    -1,    60,    -1,    62,
      -1,    -1,    65,    66,    67,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    -1,    15,    16,    17,
      18,    -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,    27,
      28,    29,    -1,    -1,    32,    -1,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    -1,    60,    -1,    62,    -1,    -1,    65,    66,    67,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    -1,    -1,    21,    22,
      -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,    32,
      -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    -1,    -1,    58,    -1,    60,    -1,    62,
      -1,    -1,    65,    66,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    13,    -1,    15,    16,    17,    18,
      -1,    -1,    21,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    -1,    -1,    32,    -1,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    -1,    -1,    -1,    58,
      -1,    60,    -1,    62,    -1,    -1,    65,    66,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    13,    -1,
      15,    16,    17,    18,    -1,    -1,    21,    22,    -1,    -1,
      -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    -1,    -1,    58,    -1,    60,    -1,    62,    -1,    -1,
      65,    66,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    13,    -1,    15,    16,    17,    18,    -1,    -1,
      21,    22,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,
      -1,    32,    -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    -1,    -1,    58,    -1,    60,
      -1,    62,    -1,    -1,    65,    66,     3,     4,     5,    -1,
      -1,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    26,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    -1,
      -1,    -1,    59,    -1,    -1,    62,    63,    64,    -1,    66,
       3,     4,     5,    -1,    -1,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    -1,    -1,    19,    -1,    21,    22,
      -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62,
       3,     4,     5,    66,    -1,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    -1,    -1,    37,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62,
       3,     4,     5,    66,    -1,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      23,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62,
       3,     4,     5,    66,    -1,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62,
       3,     4,     5,    66,    -1,    -1,     9,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62,
       3,     4,     5,    66,    -1,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    -1,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62,
       3,     4,     5,    66,    -1,    -1,     9,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62,
      -1,    -1,    -1,    66,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    15,    16,    17,    18,
      -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    -1,    -1,    32,    33,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      -1,    60,    -1,    62,    -1,    -1,    65,    66,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    -1,
      15,    16,    17,    18,    -1,    -1,    -1,    22,    -1,    -1,
      -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,    34,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    58,    -1,    60,    -1,    62,    -1,    -1,
      65,    66,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    15,    16,    17,    18,    -1,    -1,
      -1,    22,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,
      -1,    32,    -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    -1,    60,
      61,    62,    -1,    -1,    65,    66,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    14,    15,    16,
      17,    18,    -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,
      27,    28,    29,    -1,    -1,    32,    -1,    34,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    58,    -1,    60,    -1,    62,    -1,    -1,    65,    66,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    -1,    15,    16,    17,    18,    -1,    -1,    -1,    22,
      -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,    32,
      -1,    34,    -1,     3,     4,     5,    -1,    -1,    -1,     9,
      10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    -1,    -1,    58,    -1,    60,    -1,    62,
      63,    -1,    65,    66,    34,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    -1,    -1,    -1,    -1,    59,
      -1,    -1,    62,    -1,    -1,    -1,    66,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    -1,    15,
      16,    17,    18,    -1,    -1,    -1,    22,    -1,    -1,    -1,
      -1,    27,    28,    29,    -1,    -1,    32,    -1,    34,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    58,    -1,    60,    -1,    62,    -1,    -1,    65,
      66,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    -1,    -1,    15,    16,    17,    18,    -1,    -1,    -1,
      22,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,
      32,    -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    58,    -1,    60,    -1,
      62,    -1,    -1,    65,    66,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    -1,    15,    16,    17,
      18,    -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,    27,
      28,    29,    -1,    -1,    32,    -1,    34,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    -1,    60,    -1,    62,    -1,    -1,    65,    66,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      -1,    15,    16,    17,    18,    -1,    -1,    -1,    22,    -1,
      -1,    -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,
      34,    -1,     3,     4,     5,    -1,    -1,    -1,     9,    10,
      11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,    58,    -1,    60,    -1,    62,    -1,
      -1,    65,    66,    34,    -1,    -1,     3,     4,     5,    -1,
      -1,    -1,     9,    10,    11,    12,    -1,    -1,    -1,    -1,
      51,    52,    53,    54,    21,    22,    -1,    -1,    59,    -1,
      -1,    62,     3,     4,     5,    66,    -1,    34,     9,    10,
      11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    -1,    -1,    -1,    -1,    53,    54,    -1,    -1,
      -1,    -1,    59,    34,    -1,    62,     3,     4,     5,    66,
      -1,    -1,     9,    10,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    54,    21,    22,    -1,    -1,    59,    -1,
      -1,    62,    -1,    -1,    -1,    66,    -1,    34,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    59,    -1,    -1,    62,    -1,    -1,    -1,    66
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     8,     9,    10,    11,    12,
      15,    16,    17,    18,    22,    27,    28,    29,    32,    34,
      58,    60,    62,    65,    66,    72,    73,    74,    76,    77,
      80,    83,    84,    86,    88,    91,    94,    30,    73,    73,
       5,    22,    33,    62,    73,    82,    85,    87,     5,    22,
      62,    81,    82,    73,    73,     5,     7,     5,     7,     5,
      95,    92,    73,     5,     5,    13,    17,    21,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    73,    75,    76,    87,     5,    62,    76,    81,
      39,    67,    87,     0,     5,    13,    21,    26,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    59,    62,    74,    55,    57,    23,     5,    21,
      61,    61,    61,     5,    87,    61,    61,    64,    20,    87,
      20,    82,    54,    61,    19,    61,     5,    61,    35,    36,
      38,    93,    61,    63,    22,    81,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    57,    63,    63,    64,    57,    20,    39,
      87,    64,    67,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    61,
      73,    73,    17,     5,    73,    73,     5,    73,    14,    62,
      73,    73,    73,    63,    63,    73,    73,    73,    76,    63,
      76,    54,    61,    62,    73,    39,    90,     5,    22,    39,
      62,    96,    97,    98,    99,    98,    73,    73,    20,    20,
      63,    76,    67,    39,    64,    67,    73,    22,    81,    17,
      14,    78,    79,    61,    57,    57,    62,    73,    98,    73,
      89,    39,    54,    55,    61,    70,    99,    98,    20,    64,
      20,    39,    64,    37,    23,    76,    76,    57,    67,    39,
      20,    20,    96,    98,    17,    20,    54,    63,    64,    73,
      80,    98,    63,    68,    20,    69,    89,    99,     5,     3,
      99,    63,    98,    99,    98,    99,    99,    73,    63,    63,
      67,    76,    76,    73,    73,     9,    78,    63,    73,    73,
      20,    20,    63,    63,    54,    73,    73,     9
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
#line 123 "lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (2)].ast_node_ptr)); }
    break;

  case 3:
#line 124 "lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 6:
#line 131 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_yield((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 7:
#line 132 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 8:
#line 133 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 9:
#line 134 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 10:
#line 135 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 11:
#line 136 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 12:
#line 137 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 13:
#line 138 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 14:
#line 139 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 15:
#line 140 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 16:
#line 141 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_AMP, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 17:
#line 142 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_PIPE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 18:
#line 143 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 19:
#line 144 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 20:
#line 145 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 21:
#line 146 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 22:
#line 147 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(3) - (3)].ast_node_ptr), (yyvsp[(1) - (3)].ast_node_ptr)); }
    break;

  case 23:
#line 148 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assoc((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 24:
#line 149 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_range_expression((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 25:
#line 150 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 26:
#line 151 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 27:
#line 152 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 28:
#line 153 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 29:
#line 154 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_thunk_expr((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 30:
#line 155 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_spread_operator((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 31:
#line 156 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_implements((yyvsp[(1) - (3)].vident), (yyvsp[(3) - (3)].vident)); }
    break;

  case 32:
#line 157 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_typed_empty_list((yyvsp[(1) - (1)].vident)); }
    break;

  case 33:
#line 158 "lang/parser.y"
    {
                                        // TODO: not doing anything with macros yet - do we want to??
                                        printf("macro '%s'\n", (yyvsp[(1) - (2)].vident).chars);
                                        (yyval.ast_node_ptr) = (yyvsp[(2) - (2)].ast_node_ptr);
                                      }
    break;

  case 34:
#line 163 "lang/parser.y"
    {
                                          Ast *let = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), (yyvsp[(4) - (6)].ast_node_ptr), (yyvsp[(6) - (6)].ast_node_ptr));
                                          let->tag = AST_LOOP;
                                          (yyval.ast_node_ptr) = let;

                                      }
    break;

  case 35:
#line 172 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assignment((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 36:
#line 176 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[(1) - (1)].vint)); }
    break;

  case 37:
#line 177 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[(1) - (1)].vdouble)); }
    break;

  case 38:
#line 178 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 39:
#line 179 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); }
    break;

  case 40:
#line 180 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); }
    break;

  case 41:
#line 181 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 42:
#line 182 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;

  case 43:
#line 183 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 44:
#line 184 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 45:
#line 185 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 46:
#line 186 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 47:
#line 187 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_char((yyvsp[(1) - (1)].vchar)); }
    break;

  case 48:
#line 188 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 49:
#line 189 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(3) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 50:
#line 190 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 51:
#line 191 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"+", 1}); }
    break;

  case 52:
#line 192 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"-", 1}); }
    break;

  case 53:
#line 193 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"*", 1}); }
    break;

  case 54:
#line 194 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"/", 1}); }
    break;

  case 55:
#line 195 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"%", 1}); }
    break;

  case 56:
#line 196 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<", 1}); }
    break;

  case 57:
#line 197 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){">", 1}); }
    break;

  case 58:
#line 198 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"&&", 2}); }
    break;

  case 59:
#line 199 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"||", 2}); }
    break;

  case 60:
#line 200 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){">=", 2}); }
    break;

  case 61:
#line 201 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<=", 2}); }
    break;

  case 62:
#line 202 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"!=", 2}); }
    break;

  case 63:
#line 203 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"==", 2}); }
    break;

  case 64:
#line 204 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"|", 1}); }
    break;

  case 65:
#line 205 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){":", 1}); }
    break;

  case 66:
#line 206 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((ObjString){"::", 2}); }
    break;

  case 67:
#line 207 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 68:
#line 208 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access((yyvsp[(1) - (3)].ast_node_ptr), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 69:
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

  case 70:
#line 235 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 71:
#line 236 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 72:
#line 240 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_test_module((yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 73:
#line 241 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 74:
#line 242 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 75:
#line 244 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), ast_extern_fn((yyvsp[(2) - (6)].vident), (yyvsp[(6) - (6)].ast_node_ptr)), NULL); }
    break;

  case 76:
#line 246 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);}
    break;

  case 77:
#line 250 "lang/parser.y"
    {
                                      Ast *variants = (yyvsp[(5) - (6)].ast_node_ptr);
                                      variants->tag = AST_EXTERN_VARIANTS;
                                      (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), variants, NULL);
                                    }
    break;

  case 78:
#line 256 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(4) - (4)].ast_node_ptr); }
    break;

  case 79:
#line 257 "lang/parser.y"
    {
                                      Ast *let = (yyvsp[(1) - (3)].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[(3) - (3)].ast_node_ptr);
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 80:
#line 262 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 81:
#line 265 "lang/parser.y"
    {
                                      Ast *id = ast_identifier((yyvsp[(3) - (6)].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[(6) - (6)].ast_node_ptr), NULL);
                                    }
    break;

  case 82:
#line 272 "lang/parser.y"
    {
                                      Ast *id = ast_identifier((yyvsp[(3) - (6)].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[(6) - (6)].ast_node_ptr), NULL);
                                    }
    break;

  case 83:
#line 281 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), false); }
    break;

  case 84:
#line 282 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), true); }
    break;

  case 85:
#line 283 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), false); }
    break;

  case 86:
#line 284 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[(2) - (2)].vident), true); }
    break;

  case 87:
#line 288 "lang/parser.y"
    { (yyval.ast_node_ptr) = extern_typed_signature((yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 88:
#line 290 "lang/parser.y"
    { (yyval.ast_node_ptr) = extern_typed_signature_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 89:
#line 295 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list(ast_extern_fn((yyvsp[(3) - (3)].vstr), (yyvsp[(1) - (3)].ast_node_ptr))); }
    break;

  case 90:
#line 298 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (5)].ast_node_ptr), ast_extern_fn((yyvsp[(5) - (5)].vstr), (yyvsp[(3) - (5)].ast_node_ptr))); }
    break;

  case 91:
#line 300 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (2)].ast_node_ptr); }
    break;

  case 92:
#line 305 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 93:
#line 306 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 94:
#line 307 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(3) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 95:
#line 308 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 96:
#line 309 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_module(ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr))); }
    break;

  case 97:
#line 310 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_module(ast_lambda(NULL, (yyvsp[(2) - (3)].ast_node_ptr))); }
    break;

  case 98:
#line 317 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 99:
#line 318 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 100:
#line 319 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 101:
#line 320 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 102:
#line 322 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 103:
#line 323 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 104:
#line 324 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 105:
#line 325 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (6)].ast_node_ptr), (yyvsp[(2) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 106:
#line 329 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 107:
#line 330 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 108:
#line 331 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 109:
#line 341 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 110:
#line 342 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 111:
#line 343 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (4)].ast_node_ptr); }
    break;

  case 112:
#line 347 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_array(); }
    break;

  case 113:
#line 348 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (5)].ast_node_ptr)); }
    break;

  case 114:
#line 349 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (6)].ast_node_ptr)); }
    break;

  case 115:
#line 353 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 116:
#line 354 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 117:
#line 358 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 118:
#line 359 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 119:
#line 360 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)); }
    break;

  case 120:
#line 364 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 121:
#line 365 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 122:
#line 369 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 123:
#line 373 "lang/parser.y"
    {(yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr);}
    break;

  case 124:
#line 374 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr));}
    break;

  case 125:
#line 377 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 126:
#line 378 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(3) - (5)].ast_node_ptr), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 127:
#line 379 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 128:
#line 382 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 129:
#line 386 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 130:
#line 387 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 131:
#line 391 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 132:
#line 392 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 133:
#line 396 "lang/parser.y"
    {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
    break;

  case 134:
#line 402 "lang/parser.y"
    {
                                      Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (2)].vident)), NULL, NULL);
                                      type_decl->tag = AST_TYPE_DECL;
                                      (yyval.ast_node_ptr) = type_decl;
                                   }
    break;

  case 135:
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

  case 136:
#line 421 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list(ast_identifier((yyvsp[(1) - (1)].vident)), NULL); }
    break;

  case 137:
#line 422 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), ast_identifier((yyvsp[(2) - (2)].vident)), NULL); }
    break;

  case 138:
#line 425 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 139:
#line 426 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 140:
#line 430 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 141:
#line 431 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 142:
#line 435 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 143:
#line 436 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 144:
#line 437 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 145:
#line 438 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_signature_of_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 146:
#line 439 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 147:
#line 443 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 148:
#line 444 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(1) - (3)].vident)), AST_CONST(AST_INT, (yyvsp[(3) - (3)].vint)), NULL); }
    break;

  case 149:
#line 445 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 150:
#line 446 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assoc(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 151:
#line 447 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 152:
#line 448 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;

  case 153:
#line 449 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access(ast_identifier((yyvsp[(1) - (3)].vident)), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;


/* Line 1267 of yacc.c.  */
#line 2934 "lang/y.tab.c"
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

