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
     TOK_STRING = 261,
     TOK_CHAR = 262,
     TRUE = 263,
     FALSE = 264,
     PIPE = 265,
     EXTERN = 266,
     TRIPLE_DOT = 267,
     LET = 268,
     FN = 269,
     MATCH = 270,
     WITH = 271,
     ARROW = 272,
     DOUBLE_COLON = 273,
     TOK_VOID = 274,
     IN = 275,
     AND = 276,
     ASYNC = 277,
     DOUBLE_AT = 278,
     DOUBLE_AMP = 279,
     DOUBLE_PIPE = 280,
     THUNK = 281,
     IMPORT = 282,
     FSTRING_START = 283,
     FSTRING_END = 284,
     FSTRING_INTERP_START = 285,
     FSTRING_INTERP_END = 286,
     FSTRING_TEXT = 287,
     APPLICATION = 288,
     MODULO = 289,
     NE = 290,
     EQ = 291,
     LE = 292,
     GE = 293,
     UMINUS = 294
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define DOUBLE 259
#define IDENTIFIER 260
#define TOK_STRING 261
#define TOK_CHAR 262
#define TRUE 263
#define FALSE 264
#define PIPE 265
#define EXTERN 266
#define TRIPLE_DOT 267
#define LET 268
#define FN 269
#define MATCH 270
#define WITH 271
#define ARROW 272
#define DOUBLE_COLON 273
#define TOK_VOID 274
#define IN 275
#define AND 276
#define ASYNC 277
#define DOUBLE_AT 278
#define DOUBLE_AMP 279
#define DOUBLE_PIPE 280
#define THUNK 281
#define IMPORT 282
#define FSTRING_START 283
#define FSTRING_END 284
#define FSTRING_INTERP_START 285
#define FSTRING_INTERP_END 286
#define FSTRING_TEXT 287
#define APPLICATION 288
#define MODULO 289
#define NE 290
#define EQ 291
#define LE 292
#define GE 293
#define UMINUS 294




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
#line 211 "lang/y.tab.c"
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
#line 236 "lang/y.tab.c"

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
#define YYFINAL  61
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1503

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  62
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  28
/* YYNRULES -- Number of rules.  */
#define YYNRULES  117
/* YYNRULES -- Number of states.  */
#define YYNSTATES  240

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   294

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    51,     2,
      52,    53,    45,    43,    55,    44,    33,    46,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    47,    49,
      38,    54,    37,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    56,     2,    57,     2,    59,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    58,     2,     2,     2,     2,
       2,    61,     2,     2,     2,     2,    60,     2,     2,     2,
       2,    50,     2,     2,    34,     2,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    35,    36,
      39,    40,    41,    42,    48
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    11,    14,    18,    22,
      25,    28,    31,    35,    39,    43,    47,    51,    55,    59,
      63,    67,    71,    75,    79,    83,    87,    91,    95,    97,
      99,   101,   104,   106,   108,   110,   112,   114,   116,   118,
     120,   122,   124,   126,   128,   132,   134,   138,   143,   148,
     155,   162,   167,   171,   173,   180,   183,   187,   191,   195,
     201,   204,   210,   216,   223,   230,   232,   236,   239,   244,
     246,   252,   255,   262,   264,   268,   270,   273,   277,   282,
     287,   293,   300,   304,   308,   312,   316,   321,   323,   327,
     332,   334,   338,   343,   349,   355,   359,   360,   363,   365,
     369,   374,   379,   381,   384,   387,   390,   394,   398,   400,
     403,   407,   409,   411,   413,   417,   421,   425
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      63,     0,    -1,    66,    49,    -1,    66,    -1,    -1,    65,
      -1,    50,    64,    -1,    64,    33,     5,    -1,    64,    23,
      64,    -1,    64,    65,    -1,    51,    65,    -1,    45,    65,
      -1,    64,    43,    64,    -1,    64,    44,    64,    -1,    64,
      45,    64,    -1,    64,    46,    64,    -1,    64,    36,    64,
      -1,    64,    38,    64,    -1,    64,    37,    64,    -1,    64,
      24,    64,    -1,    64,    25,    64,    -1,    64,    42,    64,
      -1,    64,    41,    64,    -1,    64,    39,    64,    -1,    64,
      40,    64,    -1,    64,    10,    64,    -1,    64,    47,    64,
      -1,    64,    18,    64,    -1,    67,    -1,    78,    -1,    84,
      -1,    26,    64,    -1,     3,    -1,     4,    -1,     6,    -1,
       8,    -1,     9,    -1,     5,    -1,    19,    -1,    73,    -1,
      74,    -1,    76,    -1,    81,    -1,     7,    -1,    52,    66,
      53,    -1,    64,    -1,    66,    49,    64,    -1,    13,     5,
      54,    64,    -1,    13,    72,    54,    64,    -1,    13,     5,
      54,    11,    14,    86,    -1,    13,     5,    54,    52,    69,
      53,    -1,    13,    19,    54,    64,    -1,    67,    20,    64,
      -1,    70,    -1,    13,    52,     5,    53,    54,    70,    -1,
      27,     6,    -1,    11,    14,    64,    -1,    68,    17,    64,
      -1,    68,    47,     6,    -1,    69,    55,    68,    47,     6,
      -1,    69,    55,    -1,    14,    71,    17,    66,    49,    -1,
      14,    19,    17,    66,    49,    -1,    52,    14,    71,    17,
      66,    53,    -1,    52,    14,    19,    17,    66,    53,    -1,
      72,    -1,    72,    54,    64,    -1,    71,    72,    -1,    71,
      72,    54,    64,    -1,    72,    -1,    72,    47,    52,    88,
      53,    -1,    71,    72,    -1,    71,    72,    47,    52,    88,
      53,    -1,     5,    -1,    52,    77,    53,    -1,    75,    -1,
      56,    57,    -1,    56,    77,    57,    -1,    56,    77,    55,
      57,    -1,    56,    34,    34,    57,    -1,    56,    34,    77,
      34,    57,    -1,    56,    34,    77,    55,    34,    57,    -1,
       5,    18,     5,    -1,     5,    18,    64,    -1,    52,    64,
      53,    -1,    52,    77,    53,    -1,    52,    77,    55,    53,
      -1,    64,    -1,    77,    55,    64,    -1,    15,    64,    16,
      80,    -1,    64,    -1,    64,    58,    64,    -1,    34,    79,
      17,    64,    -1,    80,    34,    79,    17,    64,    -1,    80,
      34,    59,    17,    64,    -1,    28,    82,    29,    -1,    -1,
      82,    83,    -1,    32,    -1,    30,    64,    31,    -1,    60,
       5,    54,    88,    -1,    60,    85,    54,    88,    -1,     5,
      -1,    85,     5,    -1,    88,    17,    -1,    86,    88,    -1,
      89,    45,    89,    -1,    87,    45,    89,    -1,    89,    -1,
      34,    89,    -1,    88,    34,    89,    -1,    86,    -1,    87,
      -1,     5,    -1,     5,    54,     3,    -1,     5,    61,    89,
      -1,    52,    88,    53,    -1,    19,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   115,   115,   116,   117,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   169,   170,   174,   175,   176,
     180,   187,   188,   193,   195,   201,   205,   206,   211,   214,
     217,   222,   223,   224,   225,   232,   233,   234,   235,   237,
     238,   239,   240,   244,   245,   246,   256,   257,   258,   262,
     263,   264,   268,   269,   273,   274,   275,   279,   280,   284,
     288,   289,   292,   293,   294,   297,   301,   302,   306,   307,
     311,   317,   330,   331,   334,   335,   339,   340,   344,   345,
     346,   347,   348,   352,   353,   354,   355,   356
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INTEGER", "DOUBLE", "IDENTIFIER",
  "TOK_STRING", "TOK_CHAR", "TRUE", "FALSE", "PIPE", "EXTERN",
  "TRIPLE_DOT", "LET", "FN", "MATCH", "WITH", "ARROW", "DOUBLE_COLON",
  "TOK_VOID", "IN", "AND", "ASYNC", "DOUBLE_AT", "DOUBLE_AMP",
  "DOUBLE_PIPE", "THUNK", "IMPORT", "FSTRING_START", "FSTRING_END",
  "FSTRING_INTERP_START", "FSTRING_INTERP_END", "FSTRING_TEXT", "'.'",
  "'|'", "APPLICATION", "MODULO", "'>'", "'<'", "NE", "EQ", "LE", "GE",
  "'+'", "'-'", "'*'", "'/'", "':'", "UMINUS", "';'", "'yield'", "'&'",
  "'('", "')'", "'='", "','", "'['", "']'", "'if'", "'_'", "'type'",
  "'of'", "$accept", "program", "expr", "simple_expr", "expr_sequence",
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
     285,   286,   287,    46,   124,   288,   289,    62,    60,   290,
     291,   292,   293,    43,    45,    42,    47,    58,   294,    59,
     121,    38,    40,    41,    61,    44,    91,    93,   105,    95,
     116,   111
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    62,    63,    63,    63,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    66,    66,    67,    67,    67,
      67,    67,    67,    67,    67,    67,    68,    68,    69,    69,
      69,    70,    70,    70,    70,    71,    71,    71,    71,    71,
      71,    71,    71,    72,    72,    72,    73,    73,    73,    74,
      74,    74,    75,    75,    76,    76,    76,    77,    77,    78,
      79,    79,    80,    80,    80,    81,    82,    82,    83,    83,
      84,    84,    85,    85,    86,    86,    87,    87,    88,    88,
      88,    88,    88,    89,    89,    89,    89,    89
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     2,     3,     3,     2,
       2,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     1,
       1,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     3,     4,     4,     6,
       6,     4,     3,     1,     6,     2,     3,     3,     3,     5,
       2,     5,     5,     6,     6,     1,     3,     2,     4,     1,
       5,     2,     6,     1,     3,     1,     2,     3,     4,     4,
       5,     6,     3,     3,     3,     3,     4,     1,     3,     4,
       1,     3,     4,     5,     5,     3,     0,     2,     1,     3,
       4,     4,     1,     2,     2,     2,     3,     3,     1,     2,
       3,     1,     1,     1,     3,     3,     3,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    32,    33,    37,    34,    43,    35,    36,     0,     0,
       0,    38,     0,     0,    96,     0,     0,     0,     0,     0,
       0,     0,    45,     5,     3,    28,    53,    39,    40,    41,
      29,    42,    30,     0,     0,     0,     0,    75,    73,     0,
       0,     0,    65,     0,    31,    55,     0,     0,    11,     6,
      10,     0,    45,     0,     0,     0,    76,    87,     0,   102,
       0,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       9,     2,     0,     0,     0,     0,    37,     0,     0,     0,
       0,    67,     0,     0,     0,    95,     0,    98,    97,     0,
       0,    84,     0,    44,    85,     0,     0,     0,     0,    77,
       0,   103,     0,    25,    27,     8,    19,    20,     7,    16,
      18,    17,    23,    24,    22,    21,    12,    13,    14,    15,
      26,    46,    52,    37,    83,     0,     0,    47,    51,     0,
      74,     0,    48,     0,     0,     0,     0,     0,    66,     0,
      89,     0,     0,     0,    86,    88,    79,     0,     0,    78,
     113,   117,     0,     0,   111,   112,   100,   108,   101,     0,
       0,     0,     0,     0,    62,    61,     0,    68,     0,    90,
       0,     0,    99,     0,     0,    80,     0,     0,     0,   109,
       0,   105,     0,   104,     0,     0,    49,     0,     0,     0,
       0,    50,    60,     0,    54,     0,    70,     0,     0,     0,
       0,    64,    63,    81,   114,   115,   116,   107,   110,   106,
      56,    57,    58,     0,     0,    72,    91,    92,     0,     0,
       0,     0,     0,    94,    93,    59,     0,     0,     0,     0
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    21,    22,    23,    53,    25,   171,   172,    26,    41,
      42,    27,    28,    37,    29,    54,    30,   180,   150,    31,
      46,    98,    32,    60,   164,   165,   191,   167
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -69
static const yytype_int16 yypact[] =
{
     670,   -69,   -69,   -69,   -69,   -69,   -69,   -69,     7,    26,
     670,   -69,   670,     3,   -69,  1447,   670,  1447,   728,   279,
       9,    19,  1061,   -69,    55,    21,   -69,   -69,   -69,   -69,
     -69,   -69,   -69,    73,    51,   755,    69,   -69,   113,    33,
     670,    31,   -32,   953,  1061,   -69,   114,   670,   -69,  1061,
     -69,    32,   899,   -20,   117,   357,   -69,  1061,   123,    83,
      18,   -69,   670,   670,   670,   670,   670,   146,   670,   670,
     670,   670,   670,   670,   670,   670,   670,   670,   670,   670,
     -69,   670,   670,   813,   415,   670,   108,   126,   670,   670,
     670,   -29,   105,   670,   137,   -69,   670,   -69,   -69,   143,
      35,   -69,   670,   -69,   -69,   442,   118,   -27,   500,   -69,
       1,   -69,     1,  1169,  1061,  1223,  1361,  1361,   -69,  1277,
    1331,  1331,  1331,  1331,  1331,  1331,  1391,  1391,   107,   107,
    1421,  1061,  1061,   -69,  1061,   170,   527,  1061,  1061,   132,
     -69,   670,  1061,   138,   139,   140,   670,     1,  1061,   670,
     155,  1007,   670,   670,   -69,  1061,   -69,   136,   585,   -69,
     -37,   -69,    77,     1,     1,   149,     4,   150,     4,     1,
     182,    -1,   130,    -9,   670,   670,     1,  1061,    -4,   843,
     180,   643,   -69,    50,    71,   -69,   141,   198,    77,   -69,
      64,     4,    77,   -69,    77,    77,     1,     4,   670,   670,
     196,   -69,   192,   190,   -69,   102,   -69,   670,   670,   188,
     189,   -69,   -69,   -69,   -69,   -69,   -69,   -69,   -69,   -69,
    1061,  1115,   -69,   101,    87,   -69,  1061,  1061,   670,   670,
     201,   191,   104,  1061,  1061,   -69,   670,   670,    81,    96
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -69,   -69,    -8,   125,     0,   -69,     8,   -69,    36,   -48,
      -7,   -69,   -69,   -69,   -69,    67,   -69,    30,   -69,   -69,
     -69,   -69,   -69,   -69,    43,   -69,   -68,   -30
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -88
static const yytype_int16 yytable[] =
{
      24,    36,    43,   100,    44,     9,   160,   157,    49,    45,
      52,    57,    33,   193,    59,    92,   199,   187,   145,    61,
     161,   193,    93,   111,   188,   146,    34,    57,   158,   102,
     194,    38,    57,   103,    91,   162,    38,    38,   194,    52,
      38,    82,   166,   203,   168,    39,   200,    57,    90,   206,
      89,    99,   153,   163,   113,   114,   115,   116,   117,    35,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   112,   131,   132,   134,   137,   138,    40,   178,
     142,   193,   160,    40,    40,   148,    58,    40,   151,   143,
     144,    83,    38,    91,   131,   190,   161,   155,   194,   174,
     155,   197,    87,   211,    81,    85,   231,    87,   205,    38,
       1,     2,     3,     4,     5,     6,     7,   216,   199,   193,
     175,   237,   107,    88,   212,    63,    11,    84,    52,   163,
     102,    83,   189,   155,   211,    14,   194,   110,   177,    40,
      48,   179,    50,    95,    96,   102,    97,    80,   230,   212,
     155,   118,   183,   184,    79,   225,    40,   147,   215,    47,
     152,   139,   217,    19,   218,   219,   131,   131,    80,    80,
     104,   149,   105,   179,    80,   156,   232,    80,   108,   140,
     109,   141,    80,   201,   169,   202,   173,   174,   175,   181,
     220,   221,   176,   185,   192,   195,   198,   208,   213,   226,
     227,   214,   222,   170,   224,   228,   229,   235,   236,   204,
     223,   210,   196,     0,     0,     0,     0,     0,     0,     0,
     233,   234,     0,     0,     0,    91,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   238,   239,    80,    80,
      80,    80,    80,     0,    80,    80,    80,    80,    80,    80,
      80,    80,    80,    80,    80,    80,    80,    80,     0,    80,
       0,     0,    80,    80,     0,     0,     0,    80,     0,     0,
       0,     0,     0,    80,     0,     0,    80,     0,     0,     0,
      80,     0,     1,     2,     3,     4,     5,     6,     7,     0,
       0,     0,     8,     9,    10,     0,     0,     0,    11,     0,
       0,     0,    80,     0,    80,    12,    13,    14,     0,     0,
       0,     0,     0,    55,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    15,     0,     0,     0,     0,    16,
      17,    18,     0,     0,     0,    19,    56,     0,     0,    20,
       0,     0,     0,     0,     0,    80,    80,     0,     0,     0,
       0,    80,    80,     0,     0,     0,     0,     0,    80,    80,
       1,     2,     3,     4,     5,     6,     7,     0,     0,     0,
       8,     9,    10,     0,     0,     0,    11,     0,     0,     0,
       0,     0,     0,    12,    13,    14,     0,     0,     0,     0,
       0,   106,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    15,     0,     0,     0,     0,    16,    17,    18,
       0,     0,     0,    19,     0,     0,     0,    20,     1,     2,
       3,     4,     5,     6,     7,     0,   135,     0,     8,     9,
      10,     0,     0,     0,    11,     0,     0,     0,     0,     0,
       0,    12,    13,    14,     0,     1,     2,     3,     4,     5,
       6,     7,     0,     0,     0,     8,     9,    10,     0,     0,
      15,    11,     0,     0,     0,    16,    17,   136,    12,    13,
      14,    19,     0,     0,     0,    20,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    15,     0,     0,
       0,     0,    16,    17,    18,   154,     0,     0,    19,     0,
       0,     0,    20,     1,     2,     3,     4,     5,     6,     7,
       0,     0,     0,     8,     9,    10,     0,     0,     0,    11,
       0,     0,     0,     0,     0,     0,    12,    13,    14,     0,
       1,     2,     3,     4,     5,     6,     7,     0,   170,     0,
       8,    51,    10,     0,     0,    15,    11,     0,     0,     0,
      16,    17,    18,    12,    13,    14,    19,   159,     0,     0,
      20,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    15,     0,     0,     0,     0,    16,    17,    18,
       0,     0,     0,    19,     0,     0,     0,    20,     1,     2,
       3,     4,     5,     6,     7,     0,     0,     0,     8,     9,
      10,     0,     0,     0,    11,     0,     0,     0,     0,     0,
       0,    12,    13,    14,     0,     0,     0,     0,     0,   186,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      15,     0,     0,     0,     0,    16,    17,    18,     0,     0,
       0,    19,     0,     0,     0,    20,     1,     2,     3,     4,
       5,     6,     7,     0,     0,     0,     8,     9,    10,     0,
       0,     0,    11,     0,     0,     0,     0,     0,     0,    12,
      13,    14,     0,     1,     2,     3,     4,     5,     6,     7,
       0,     0,     0,     8,     9,    10,     0,     0,    15,    11,
       0,     0,     0,    16,    17,    18,    12,    13,    14,    19,
       0,     0,   209,    20,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    15,     0,     0,     0,     0,
      16,    17,    18,     0,     0,     0,    19,     0,     0,     0,
      20,     1,     2,     3,     4,     5,     6,     7,     0,     0,
       0,     8,    51,    10,     0,     0,     0,    11,     0,     0,
       0,     0,     0,     0,    12,    13,    14,     0,     1,     2,
      86,     4,     5,     6,     7,     0,     0,     0,     8,     9,
      10,     0,     0,    15,    11,     0,     0,     0,    16,    17,
      18,    12,    13,    14,    19,     0,     0,     0,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      15,     0,     0,     0,     0,    16,    17,    18,     0,     0,
       0,    19,     0,     0,     0,    20,     1,     2,   133,     4,
       5,     6,     7,     0,     0,     0,     8,     9,    10,     0,
       0,     0,    11,     0,     0,     0,     0,     0,     0,    12,
      13,    14,     0,     0,     0,     0,     1,     2,     3,     4,
       5,     6,     7,    62,     0,     0,     0,     0,    15,     0,
       0,    63,    11,    16,    17,    18,    64,    65,    66,    19,
       0,    14,     0,    20,     0,     0,    67,     0,     0,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,     0,     0,     0,     0,    47,     0,     0,     0,    19,
       0,   207,     1,     2,     3,     4,     5,     6,     7,    62,
       0,     0,     0,     0,     0,     0,     0,    63,    11,     0,
       0,     0,    64,    65,    66,     0,     0,    14,     0,     0,
       0,     0,    67,     0,     0,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,     0,     0,     0,
       0,    47,   101,     0,   -87,    19,     1,     2,     3,     4,
       5,     6,     7,    62,     0,     0,     0,     0,     0,    94,
       0,    63,    11,     0,     0,     0,    64,    65,    66,     0,
       0,    14,     0,     0,     0,     0,    67,     0,     0,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,     0,     0,     0,     0,    47,     0,     0,     0,    19,
       1,     2,     3,     4,     5,     6,     7,    62,     0,     0,
       0,     0,     0,     0,     0,    63,    11,     0,     0,     0,
      64,    65,    66,     0,     0,    14,     0,     0,   182,     0,
      67,     0,     0,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,     0,     0,     0,     0,    47,
       0,     0,     0,    19,     1,     2,     3,     4,     5,     6,
       7,    62,     0,     0,     0,     0,     0,     0,     0,    63,
      11,     0,     0,     0,    64,    65,    66,     0,     0,    14,
       0,     0,     0,     0,    67,     0,     0,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,     0,
       0,     0,     0,    47,     0,     0,     0,    19,     1,     2,
       3,     4,     5,     6,     7,    62,     0,     0,     0,     0,
       0,     0,     0,    63,    11,     0,     0,     0,    64,    65,
      66,     0,     0,    14,     0,     0,     0,     0,    67,     0,
       0,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,     0,     0,     0,     0,     0,    47,     0,     0,
       0,    19,     1,     2,     3,     4,     5,     6,     7,     0,
       0,     0,     0,     0,     0,     0,     0,    63,    11,     0,
       0,     0,    64,    65,    66,     0,     0,    14,     0,     0,
       0,     0,     0,     0,     0,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,     0,     0,     0,
       0,    47,     0,     0,     0,    19,     1,     2,     3,     4,
       5,     6,     7,     0,     0,     0,     0,     0,     0,     0,
       0,    63,    11,     0,     0,     0,     0,    65,    66,     0,
       0,    14,     0,     0,     0,     0,     0,     0,     0,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,     0,     0,     0,     0,    47,     0,     0,     0,    19,
       1,     2,     3,     4,     5,     6,     7,     0,     0,     0,
       0,     0,     0,     0,     0,    63,    11,     0,     0,     0,
       0,    65,    66,     0,     0,    14,     0,     0,     0,     0,
       0,     0,     0,     0,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,     0,     0,     0,     0,    47,
       0,     0,     0,    19,     1,     2,     3,     4,     5,     6,
       7,     0,     0,     0,     0,     0,     0,     0,     0,    63,
      11,     0,     0,     0,     0,    65,    66,     0,     0,    14,
       0,     0,     0,     0,     1,     2,     3,     4,     5,     6,
       7,     0,     0,     0,    75,    76,    77,    78,    79,    63,
      11,     0,     0,    47,     0,     0,     0,    19,     0,    14,
       0,     0,     0,     0,     1,     2,     3,     4,     5,     6,
       7,     0,     0,     0,    75,    76,    77,    78,    79,    63,
      11,     0,     0,    47,     0,     0,     0,    19,     0,    14,
       0,     0,     0,     0,     1,     2,     3,     4,     5,     6,
       7,     0,     0,     0,     0,     0,    77,    78,    79,    63,
      11,     0,     0,    47,     0,     0,     0,    19,     0,    14,
       1,     2,     3,     4,     5,     6,     7,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    11,     0,     0,     0,
       0,     0,     0,    47,     0,    14,     0,    19,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    47,
       0,     0,     0,    19
};

static const yytype_int16 yycheck[] =
{
       0,     8,    10,    51,    12,    14,     5,    34,    16,     6,
      18,    19,     5,    17,     5,    47,    17,    54,    47,     0,
      19,    17,    54,     5,    61,    54,    19,    35,    55,    49,
      34,     5,    40,    53,    41,    34,     5,     5,    34,    47,
       5,    20,   110,    52,   112,    19,    47,    55,    17,    53,
      17,    19,    17,    52,    62,    63,    64,    65,    66,    52,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    54,    81,    82,    83,    84,    85,    52,   147,
      88,    17,     5,    52,    52,    93,    19,    52,    96,    89,
      90,    18,     5,   100,   102,   163,    19,   105,    34,    49,
     108,   169,    35,    53,    49,    54,    19,    40,   176,     5,
       3,     4,     5,     6,     7,     8,     9,    53,    17,    17,
      49,    17,    55,    54,    53,    18,    19,    54,   136,    52,
      49,    18,   162,   141,    53,    28,    34,    54,   146,    52,
      15,   149,    17,    29,    30,    49,    32,    22,    47,    53,
     158,     5,   152,   153,    47,    53,    52,    52,   188,    52,
      17,    53,   192,    56,   194,   195,   174,   175,    43,    44,
      53,    34,    55,   181,    49,    57,   224,    52,    55,    53,
      57,    55,    57,    53,    14,    55,    54,    49,    49,    34,
     198,   199,    52,    57,    45,    45,    14,    17,    57,   207,
     208,     3,     6,    11,    14,    17,    17,     6,    17,   173,
     202,   181,   169,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     228,   229,    -1,    -1,    -1,   232,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   236,   237,   113,   114,
     115,   116,   117,    -1,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,    -1,   134,
      -1,    -1,   137,   138,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,   148,    -1,    -1,   151,    -1,    -1,    -1,
     155,    -1,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    13,    14,    15,    -1,    -1,    -1,    19,    -1,
      -1,    -1,   177,    -1,   179,    26,    27,    28,    -1,    -1,
      -1,    -1,    -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    50,
      51,    52,    -1,    -1,    -1,    56,    57,    -1,    -1,    60,
      -1,    -1,    -1,    -1,    -1,   220,   221,    -1,    -1,    -1,
      -1,   226,   227,    -1,    -1,    -1,    -1,    -1,   233,   234,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      13,    14,    15,    -1,    -1,    -1,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    26,    27,    28,    -1,    -1,    -1,    -1,
      -1,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    -1,    -1,    50,    51,    52,
      -1,    -1,    -1,    56,    -1,    -1,    -1,    60,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    -1,    13,    14,
      15,    -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,
      -1,    26,    27,    28,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    -1,    -1,    13,    14,    15,    -1,    -1,
      45,    19,    -1,    -1,    -1,    50,    51,    52,    26,    27,
      28,    56,    -1,    -1,    -1,    60,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      -1,    -1,    50,    51,    52,    53,    -1,    -1,    56,    -1,
      -1,    -1,    60,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    13,    14,    15,    -1,    -1,    -1,    19,
      -1,    -1,    -1,    -1,    -1,    -1,    26,    27,    28,    -1,
       3,     4,     5,     6,     7,     8,     9,    -1,    11,    -1,
      13,    14,    15,    -1,    -1,    45,    19,    -1,    -1,    -1,
      50,    51,    52,    26,    27,    28,    56,    57,    -1,    -1,
      60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    45,    -1,    -1,    -1,    -1,    50,    51,    52,
      -1,    -1,    -1,    56,    -1,    -1,    -1,    60,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    13,    14,
      15,    -1,    -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,
      -1,    26,    27,    28,    -1,    -1,    -1,    -1,    -1,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    -1,    -1,    50,    51,    52,    -1,    -1,
      -1,    56,    -1,    -1,    -1,    60,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    13,    14,    15,    -1,
      -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,    -1,    26,
      27,    28,    -1,     3,     4,     5,     6,     7,     8,     9,
      -1,    -1,    -1,    13,    14,    15,    -1,    -1,    45,    19,
      -1,    -1,    -1,    50,    51,    52,    26,    27,    28,    56,
      -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,
      50,    51,    52,    -1,    -1,    -1,    56,    -1,    -1,    -1,
      60,     3,     4,     5,     6,     7,     8,     9,    -1,    -1,
      -1,    13,    14,    15,    -1,    -1,    -1,    19,    -1,    -1,
      -1,    -1,    -1,    -1,    26,    27,    28,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    -1,    -1,    13,    14,
      15,    -1,    -1,    45,    19,    -1,    -1,    -1,    50,    51,
      52,    26,    27,    28,    56,    -1,    -1,    -1,    60,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    -1,    -1,    50,    51,    52,    -1,    -1,
      -1,    56,    -1,    -1,    -1,    60,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    13,    14,    15,    -1,
      -1,    -1,    19,    -1,    -1,    -1,    -1,    -1,    -1,    26,
      27,    28,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    10,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    18,    19,    50,    51,    52,    23,    24,    25,    56,
      -1,    28,    -1,    60,    -1,    -1,    33,    -1,    -1,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    -1,    -1,    -1,    52,    -1,    -1,    -1,    56,
      -1,    58,     3,     4,     5,     6,     7,     8,     9,    10,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    18,    19,    -1,
      -1,    -1,    23,    24,    25,    -1,    -1,    28,    -1,    -1,
      -1,    -1,    33,    -1,    -1,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    -1,    -1,    -1,
      -1,    52,    53,    -1,    55,    56,     3,     4,     5,     6,
       7,     8,     9,    10,    -1,    -1,    -1,    -1,    -1,    16,
      -1,    18,    19,    -1,    -1,    -1,    23,    24,    25,    -1,
      -1,    28,    -1,    -1,    -1,    -1,    33,    -1,    -1,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    -1,    -1,    -1,    52,    -1,    -1,    -1,    56,
       3,     4,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    18,    19,    -1,    -1,    -1,
      23,    24,    25,    -1,    -1,    28,    -1,    -1,    31,    -1,
      33,    -1,    -1,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,    52,
      -1,    -1,    -1,    56,     3,     4,     5,     6,     7,     8,
       9,    10,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    18,
      19,    -1,    -1,    -1,    23,    24,    25,    -1,    -1,    28,
      -1,    -1,    -1,    -1,    33,    -1,    -1,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
      -1,    -1,    -1,    52,    -1,    -1,    -1,    56,     3,     4,
       5,     6,     7,     8,     9,    10,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    18,    19,    -1,    -1,    -1,    23,    24,
      25,    -1,    -1,    28,    -1,    -1,    -1,    -1,    33,    -1,
      -1,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    -1,    -1,    -1,    -1,    -1,    52,    -1,    -1,
      -1,    56,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    18,    19,    -1,
      -1,    -1,    23,    24,    25,    -1,    -1,    28,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    -1,    -1,    -1,
      -1,    52,    -1,    -1,    -1,    56,     3,     4,     5,     6,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    18,    19,    -1,    -1,    -1,    -1,    24,    25,    -1,
      -1,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    -1,    -1,    -1,    52,    -1,    -1,    -1,    56,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    18,    19,    -1,    -1,    -1,
      -1,    24,    25,    -1,    -1,    28,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,    52,
      -1,    -1,    -1,    56,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    18,
      19,    -1,    -1,    -1,    -1,    24,    25,    -1,    -1,    28,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    43,    44,    45,    46,    47,    18,
      19,    -1,    -1,    52,    -1,    -1,    -1,    56,    -1,    28,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    43,    44,    45,    46,    47,    18,
      19,    -1,    -1,    52,    -1,    -1,    -1,    56,    -1,    28,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,    18,
      19,    -1,    -1,    52,    -1,    -1,    -1,    56,    -1,    28,
       3,     4,     5,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    52,    -1,    28,    -1,    56,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      -1,    -1,    -1,    56
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    13,    14,
      15,    19,    26,    27,    28,    45,    50,    51,    52,    56,
      60,    63,    64,    65,    66,    67,    70,    73,    74,    76,
      78,    81,    84,     5,    19,    52,    72,    75,     5,    19,
      52,    71,    72,    64,    64,     6,    82,    52,    65,    64,
      65,    14,    64,    66,    77,    34,    57,    64,    77,     5,
      85,     0,    10,    18,    23,    24,    25,    33,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      65,    49,    20,    18,    54,    54,     5,    77,    54,    17,
      17,    72,    47,    54,    16,    29,    30,    32,    83,    19,
      71,    53,    49,    53,    53,    55,    34,    77,    55,    57,
      54,     5,    54,    64,    64,    64,    64,    64,     5,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,     5,    64,    11,    52,    64,    64,    53,
      53,    55,    64,    66,    66,    47,    54,    52,    64,    34,
      80,    64,    17,    17,    53,    64,    57,    34,    55,    57,
       5,    19,    34,    52,    86,    87,    88,    89,    88,    14,
      11,    68,    69,    54,    49,    49,    52,    64,    88,    64,
      79,    34,    31,    66,    66,    57,    34,    54,    61,    89,
      88,    88,    45,    17,    34,    45,    86,    88,    14,    17,
      47,    53,    55,    52,    70,    88,    53,    58,    17,    59,
      79,    53,    53,    57,     3,    89,    53,    89,    89,    89,
      64,    64,     6,    68,    14,    53,    64,    64,    17,    17,
      47,    19,    71,    64,    64,     6,    17,    17,    66,    66
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
#line 115 "lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (2)].ast_node_ptr)); }
    break;

  case 3:
#line 116 "lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 6:
#line 124 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_yield((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 7:
#line 125 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access((yyvsp[(1) - (3)].ast_node_ptr), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 8:
#line 126 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 9:
#line 127 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 10:
#line 128 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_unop(TOKEN_AMPERSAND, (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 11:
#line 129 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_unop(TOKEN_STAR, (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 12:
#line 130 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 13:
#line 131 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 14:
#line 132 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 15:
#line 133 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 16:
#line 134 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 17:
#line 135 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 18:
#line 136 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 19:
#line 137 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_AMP, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 20:
#line 138 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_PIPE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 21:
#line 139 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 22:
#line 140 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 23:
#line 141 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 24:
#line 142 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 25:
#line 143 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(3) - (3)].ast_node_ptr), (yyvsp[(1) - (3)].ast_node_ptr)); }
    break;

  case 26:
#line 144 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assoc((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 27:
#line 145 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 28:
#line 146 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 29:
#line 147 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 30:
#line 148 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 31:
#line 149 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_thunk_expr((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 32:
#line 153 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[(1) - (1)].vint)); }
    break;

  case 33:
#line 154 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[(1) - (1)].vdouble)); }
    break;

  case 34:
#line 155 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 35:
#line 156 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); }
    break;

  case 36:
#line 157 "lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); }
    break;

  case 37:
#line 158 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 38:
#line 159 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;

  case 39:
#line 160 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 40:
#line 161 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 41:
#line 162 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 42:
#line 163 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 43:
#line 164 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_char((yyvsp[(1) - (1)].vchar)); }
    break;

  case 44:
#line 165 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 45:
#line 169 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 46:
#line 170 "lang/parser.y"
    { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 47:
#line 174 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 48:
#line 175 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 49:
#line 177 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), ast_extern_fn((yyvsp[(2) - (6)].vident), (yyvsp[(6) - (6)].ast_node_ptr)), NULL); }
    break;

  case 50:
#line 181 "lang/parser.y"
    {
                                      Ast *variants = (yyvsp[(5) - (6)].ast_node_ptr);
                                      variants->tag = AST_EXTERN_VARIANTS;
                                      (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), variants, NULL);
                                    }
    break;

  case 51:
#line 187 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(4) - (4)].ast_node_ptr); }
    break;

  case 52:
#line 188 "lang/parser.y"
    {
                                      Ast *let = (yyvsp[(1) - (3)].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[(3) - (3)].ast_node_ptr);
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 53:
#line 193 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 54:
#line 196 "lang/parser.y"
    {
                                      Ast *id = ast_identifier((yyvsp[(3) - (6)].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[(6) - (6)].ast_node_ptr), NULL);
                                    }
    break;

  case 55:
#line 201 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_bare_import((yyvsp[(2) - (2)].vstr)); }
    break;

  case 56:
#line 205 "lang/parser.y"
    { (yyval.ast_node_ptr) = extern_typed_signature((yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 57:
#line 207 "lang/parser.y"
    { (yyval.ast_node_ptr) = extern_typed_signature_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 58:
#line 212 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list(ast_extern_fn((yyvsp[(3) - (3)].vstr), (yyvsp[(1) - (3)].ast_node_ptr))); }
    break;

  case 59:
#line 215 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (5)].ast_node_ptr), ast_extern_fn((yyvsp[(5) - (5)].vstr), (yyvsp[(3) - (5)].ast_node_ptr))); }
    break;

  case 60:
#line 217 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (2)].ast_node_ptr); }
    break;

  case 61:
#line 222 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 62:
#line 223 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 63:
#line 224 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(3) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 64:
#line 225 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 65:
#line 232 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 66:
#line 233 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 67:
#line 234 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 68:
#line 235 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 69:
#line 237 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 70:
#line 238 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 71:
#line 239 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 72:
#line 240 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (6)].ast_node_ptr), (yyvsp[(2) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 73:
#line 244 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 74:
#line 245 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 75:
#line 246 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 76:
#line 256 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 77:
#line 257 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 78:
#line 258 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (4)].ast_node_ptr); }
    break;

  case 79:
#line 262 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_array(); }
    break;

  case 80:
#line 263 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (5)].ast_node_ptr)); }
    break;

  case 81:
#line 264 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[(3) - (6)].ast_node_ptr)); }
    break;

  case 82:
#line 268 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 83:
#line 269 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 84:
#line 273 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 85:
#line 274 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 86:
#line 275 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)); }
    break;

  case 87:
#line 279 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 88:
#line 280 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 89:
#line 284 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 90:
#line 288 "lang/parser.y"
    {(yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr);}
    break;

  case 91:
#line 289 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr));}
    break;

  case 92:
#line 292 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 93:
#line 293 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(3) - (5)].ast_node_ptr), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 94:
#line 294 "lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 95:
#line 297 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 96:
#line 301 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 97:
#line 302 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 98:
#line 306 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 99:
#line 307 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 100:
#line 311 "lang/parser.y"
    {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
    break;

  case 101:
#line 317 "lang/parser.y"
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

  case 102:
#line 330 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list(ast_identifier((yyvsp[(1) - (1)].vident)), NULL); }
    break;

  case 103:
#line 331 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), ast_identifier((yyvsp[(2) - (2)].vident)), NULL); }
    break;

  case 104:
#line 334 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[(1) - (2)].ast_node_ptr), NULL); }
    break;

  case 105:
#line 335 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 106:
#line 339 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 107:
#line 340 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 108:
#line 344 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 109:
#line 345 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 110:
#line 346 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 111:
#line 347 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 112:
#line 348 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 113:
#line 352 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 114:
#line 353 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(1) - (3)].vident)), AST_CONST(AST_INT, (yyvsp[(3) - (3)].vint)), NULL); }
    break;

  case 115:
#line 354 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 116:
#line 355 "lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 117:
#line 356 "lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;


/* Line 1267 of yacc.c.  */
#line 2517 "lang/y.tab.c"
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


#line 358 "lang/parser.y"



void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, _cur_script);
}
#endif _LANG_TAB_H

