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
#define YYLSP_NEEDED 0



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
     IN = 276,
     AND = 277,
     ASYNC = 278,
     FSTRING_START = 279,
     FSTRING_END = 280,
     FSTRING_INTERP_START = 281,
     FSTRING_INTERP_END = 282,
     FSTRING_TEXT = 283,
     APPLICATION = 284,
     MODULO = 285,
     NE = 286,
     EQ = 287,
     LE = 288,
     GE = 289,
     UMINUS = 290
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
#define IN 276
#define AND 277
#define ASYNC 278
#define FSTRING_START 279
#define FSTRING_END 280
#define FSTRING_INTERP_START 281
#define FSTRING_INTERP_END 282
#define FSTRING_TEXT 283
#define APPLICATION 284
#define MODULO 285
#define NE 286
#define EQ 287
#define LE 288
#define GE 289
#define UMINUS 290




/* Copy the first part of user declarations.  */
#line 1 "../lang/parser.y"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parse.h"
#include "serde.h"
#include "common.h"

/* prototypes */
extern void yyerror(const char *s);

extern int yylineno;
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
#line 27 "../lang/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    char vchar;
}
/* Line 193 of yacc.c.  */
#line 200 "../lang/y.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 213 "../lang/y.tab.c"

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
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

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
#define YYLAST   1124

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  57
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  26
/* YYNRULES -- Number of rules.  */
#define YYNRULES  106
/* YYNRULES -- Number of states.  */
#define YYNSTATES  209

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    46,     2,
      48,    49,    41,    39,    51,    40,    29,    42,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    43,    45,
      34,    50,    33,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    52,     2,    53,     2,    54,     2,    47,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    56,     2,     2,     2,     2,    55,     2,     2,     2,
       2,     2,     2,     2,    30,     2,     2,     2,     2,     2,
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
      25,    26,    27,    28,    31,    32,    35,    36,    37,    38,
      44
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    11,    15,    18,    21,
      24,    28,    32,    36,    40,    44,    48,    52,    56,    60,
      64,    68,    72,    76,    80,    82,    84,    86,    89,    91,
      93,    95,    97,    99,   101,   103,   105,   107,   109,   111,
     115,   117,   121,   126,   131,   136,   143,   148,   152,   154,
     157,   161,   165,   169,   175,   178,   184,   190,   197,   204,
     206,   210,   213,   218,   220,   224,   227,   232,   234,   238,
     240,   243,   247,   252,   256,   260,   264,   268,   273,   275,
     279,   284,   289,   295,   301,   305,   306,   309,   311,   315,
     320,   325,   327,   330,   334,   338,   342,   346,   348,   351,
     355,   357,   359,   361,   365,   369,   373
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      58,     0,    -1,    61,    45,    -1,    61,    -1,    -1,    60,
      -1,    59,    29,     5,    -1,    59,    60,    -1,    46,    60,
      -1,    41,    60,    -1,    59,    39,    59,    -1,    59,    40,
      59,    -1,    59,    41,    59,    -1,    59,    42,    59,    -1,
      59,    32,    59,    -1,    59,    34,    59,    -1,    59,    33,
      59,    -1,    59,    38,    59,    -1,    59,    37,    59,    -1,
      59,    35,    59,    -1,    59,    36,    59,    -1,    59,    11,
      59,    -1,    59,    43,    59,    -1,    59,    19,    59,    -1,
      62,    -1,    72,    -1,    77,    -1,    47,    59,    -1,     3,
      -1,     4,    -1,     7,    -1,     9,    -1,    10,    -1,     5,
      -1,    20,    -1,    68,    -1,    70,    -1,    74,    -1,     8,
      -1,    48,    61,    49,    -1,    59,    -1,    61,    45,    59,
      -1,    14,     5,    50,    59,    -1,    14,    67,    50,    59,
      -1,    14,     5,    50,    63,    -1,    14,     5,    50,    48,
      64,    49,    -1,    14,    20,    50,    59,    -1,    62,    21,
      59,    -1,    65,    -1,    23,    65,    -1,    12,    15,    59,
      -1,    63,    18,    59,    -1,    63,    43,     7,    -1,    64,
      51,    63,    43,     7,    -1,    64,    51,    -1,    15,    66,
      18,    61,    45,    -1,    15,    20,    18,    61,    45,    -1,
      48,    15,    66,    18,    61,    49,    -1,    48,    15,    20,
      18,    61,    49,    -1,    67,    -1,    67,    50,    59,    -1,
      66,    67,    -1,    66,    67,    50,    59,    -1,    67,    -1,
      67,    43,    59,    -1,    66,    67,    -1,    66,    67,    43,
      59,    -1,     5,    -1,    48,    71,    49,    -1,    69,    -1,
      52,    53,    -1,    52,    71,    53,    -1,    52,    71,    51,
      53,    -1,     5,    19,     5,    -1,     5,    19,    59,    -1,
      48,    59,    49,    -1,    48,    71,    49,    -1,    48,    71,
      51,    49,    -1,    59,    -1,    71,    51,    59,    -1,    16,
      59,    17,    73,    -1,    30,    59,    18,    59,    -1,    73,
      30,    59,    18,    59,    -1,    73,    30,    54,    18,    59,
      -1,    24,    75,    25,    -1,    -1,    75,    76,    -1,    28,
      -1,    26,    59,    27,    -1,    55,     5,    50,    81,    -1,
      55,    78,    50,    81,    -1,     5,    -1,    78,     5,    -1,
      81,    18,    81,    -1,    79,    18,    81,    -1,    82,    41,
      82,    -1,    80,    41,    82,    -1,    82,    -1,    30,    82,
      -1,    81,    30,    82,    -1,    79,    -1,    80,    -1,     5,
      -1,     5,    50,     3,    -1,     5,    56,    82,    -1,    48,
      81,    49,    -1,    20,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   104,   104,   105,   106,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     153,   154,   158,   159,   160,   164,   171,   172,   177,   179,
     186,   187,   192,   195,   198,   203,   204,   205,   206,   213,
     214,   215,   216,   218,   219,   220,   221,   225,   226,   227,
     237,   238,   239,   243,   244,   248,   249,   250,   254,   255,
     259,   263,   264,   265,   268,   272,   273,   277,   278,   282,
     288,   301,   302,   305,   306,   310,   311,   315,   316,   317,
     318,   319,   323,   324,   325,   326,   327
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
  "DOUBLE_COLON", "TOK_VOID", "IN", "AND", "ASYNC", "FSTRING_START",
  "FSTRING_END", "FSTRING_INTERP_START", "FSTRING_INTERP_END",
  "FSTRING_TEXT", "'.'", "'|'", "APPLICATION", "MODULO", "'>'", "'<'",
  "NE", "EQ", "LE", "GE", "'+'", "'-'", "'*'", "'/'", "':'", "UMINUS",
  "';'", "'&'", "'await'", "'('", "')'", "'='", "','", "'['", "']'", "'_'",
  "'type'", "'of'", "$accept", "program", "expr", "simple_expr",
  "expr_sequence", "let_binding", "extern_typed_signature",
  "extern_variants", "lambda_expr", "lambda_args", "lambda_arg", "list",
  "list_match_expr", "tuple", "expr_list", "match_expr", "match_branches",
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
     275,   276,   277,   278,   279,   280,   281,   282,   283,    46,
     124,   284,   285,    62,    60,   286,   287,   288,   289,    43,
      45,    42,    47,    58,   290,    59,    38,    97,    40,    41,
      61,    44,    91,    93,    95,   116,   111
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    57,    58,    58,    58,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      61,    61,    62,    62,    62,    62,    62,    62,    62,    62,
      63,    63,    64,    64,    64,    65,    65,    65,    65,    66,
      66,    66,    66,    66,    66,    66,    66,    67,    67,    67,
      68,    68,    68,    69,    69,    70,    70,    70,    71,    71,
      72,    73,    73,    73,    74,    75,    75,    76,    76,    77,
      77,    78,    78,    79,    79,    80,    80,    81,    81,    81,
      81,    81,    82,    82,    82,    82,    82
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     3,     2,     2,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     1,     1,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       1,     3,     4,     4,     4,     6,     4,     3,     1,     2,
       3,     3,     3,     5,     2,     5,     5,     6,     6,     1,
       3,     2,     4,     1,     3,     2,     4,     1,     3,     1,
       2,     3,     4,     3,     3,     3,     3,     4,     1,     3,
       4,     4,     5,     5,     3,     0,     2,     1,     3,     4,
       4,     1,     2,     3,     3,     3,     3,     1,     2,     3,
       1,     1,     1,     3,     3,     3,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    28,    29,    33,    30,    38,    31,    32,     0,     0,
       0,    34,     0,    85,     0,     0,     0,     0,     0,     0,
       0,    40,     5,     3,    24,    48,    35,    36,    25,    37,
      26,     0,     0,     0,     0,    69,    67,     0,     0,    59,
       0,     0,    49,     0,     0,     9,     8,    27,     0,    40,
       0,     0,    70,    78,     0,    91,     0,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     7,     2,     0,     0,     0,     0,     0,
       0,     0,     0,    61,     0,     0,     0,     0,    84,     0,
      87,    86,     0,     0,    75,     0,    39,    76,     0,     0,
      71,     0,    92,     0,    21,    23,     6,    14,    16,    15,
      19,    20,    18,    17,    10,    11,    12,    13,    22,    41,
      47,    33,    74,     0,     0,    42,    44,    46,    68,     0,
      43,     0,     0,     0,     0,    64,    60,     0,    80,     0,
       0,     0,     0,     0,    77,    79,    72,   102,   106,     0,
       0,   100,   101,    89,    97,    90,     0,     0,     0,     0,
      56,    55,    66,    62,     0,     0,     0,     0,    88,     0,
       0,     0,     0,    98,     0,     0,     0,     0,     0,     0,
      50,     0,    45,    54,    51,     0,     0,     0,     0,     0,
      58,    57,   103,   104,   105,    94,    96,    93,    99,    95,
      52,     0,    81,     0,     0,     0,    83,    82,    53
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    20,    21,    22,    50,    24,   126,   158,    25,    38,
      39,    26,    35,    27,    51,    28,   138,    29,    43,    91,
      30,    56,   151,   152,   153,   154
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -121
static const yytype_int16 yypact[] =
{
     561,  -121,  -121,  -121,  -121,  -121,  -121,  -121,    21,    24,
     561,  -121,    -1,  -121,   379,   379,   561,   614,   236,    16,
      33,   926,  -121,    39,    88,  -121,  -121,  -121,  -121,  -121,
    -121,     0,    72,   561,    87,  -121,   117,   124,    25,   -28,
     742,   147,  -121,    82,   561,  -121,  -121,   926,    95,   692,
     -33,   -11,  -121,   926,    67,   116,    -4,  -121,   561,   561,
     160,   561,   561,   561,   561,   561,   561,   561,   561,   561,
     561,   561,   561,  -121,   561,   561,   667,   296,   561,    79,
     561,   561,   561,   -25,   561,   561,   137,    97,  -121,   561,
    -121,  -121,   150,    85,  -121,   561,  -121,  -121,   349,   402,
    -121,    -3,  -121,    -3,   972,   926,  -121,  1018,  1064,  1064,
    1064,  1064,  1064,  1064,  1072,  1072,    73,    73,   326,   926,
     926,  -121,   926,   154,   455,   926,   152,   926,  -121,   561,
     926,   128,   129,   561,   561,   926,   926,   561,   146,   159,
      86,   788,   561,   561,  -121,   926,  -121,   -19,  -121,   106,
      -3,   161,   139,     2,   140,     2,   561,    -8,   112,   561,
     561,   561,   926,   926,   834,   508,   561,   561,  -121,    50,
      90,   175,   106,  -121,    83,    -3,   106,    -3,   106,   106,
     926,   176,  -121,   170,   326,   561,   166,   880,   108,   115,
    -121,  -121,  -121,  -121,  -121,     2,  -121,     2,  -121,  -121,
    -121,    -7,   926,   561,   561,   178,   926,   926,  -121
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -121,  -121,   -10,    91,     5,  -121,  -120,  -121,   174,   -45,
       1,  -121,  -121,  -121,    -5,  -121,  -121,  -121,  -121,  -121,
    -121,  -121,  -121,  -121,   -79,   -20
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -79
static const yytype_int16 yytable[] =
{
      40,   102,   147,    93,   157,    23,    47,    49,    53,    34,
     159,   159,    95,    54,     9,    84,    96,   148,   133,    76,
     177,    55,    85,    53,   155,   134,    31,   149,    79,    36,
      36,   171,   178,    57,    49,   181,   205,   172,    97,    83,
      98,    32,   140,    82,    37,   150,   103,    41,   104,   105,
      77,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   201,   119,   120,   122,   125,   127,    33,
     130,   174,    33,    33,   135,   136,     1,     2,     3,   141,
       4,     5,     6,     7,    74,   119,   131,   132,   145,   145,
      36,    36,    59,    11,    83,   160,   195,    13,   197,   190,
      36,   177,    36,   143,   167,    45,    46,    88,    89,    75,
      90,   147,    73,   178,    49,    92,    72,   139,    99,   145,
     100,    44,    78,   162,   163,    18,   148,   164,   128,   173,
     129,    73,   194,    33,    33,   161,    76,    80,    73,   191,
      73,    83,    81,    33,    73,    33,   180,   169,   170,   184,
     119,   119,   193,    95,   150,   187,   196,   190,   198,   199,
      95,   182,    87,   183,   191,   106,   101,   137,   142,   156,
     159,   188,   189,   160,   161,   202,   165,   166,   192,   175,
     176,   179,   123,   200,   203,   208,    42,     0,     0,     0,
       0,     0,     0,   206,   207,    73,    73,     0,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    73,
      73,    73,     0,    73,     0,     0,    73,     0,    73,     0,
       0,    73,     0,     0,     0,     0,    73,    73,     0,     0,
       0,     0,    73,     0,     0,     0,    73,     0,     0,     1,
       2,     3,     0,     4,     5,     6,     7,     0,     0,     0,
       8,     9,    10,    73,    73,    73,    11,     0,     0,    12,
      13,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    73,     0,     0,     0,    73,     0,    14,    73,     0,
       0,     0,    15,    16,    17,     0,     0,     0,    18,    52,
       0,    19,     0,    73,     0,     0,     0,    73,    73,     1,
       2,     3,     0,     4,     5,     6,     7,     0,   123,     0,
       8,     9,    10,     0,     0,     0,    11,     0,     0,    12,
      13,     0,     0,     0,     0,     0,     0,     0,     0,     1,
       2,     3,     0,     4,     5,     6,     7,    14,     0,     0,
       0,     0,    15,    16,   124,    59,    11,     0,    18,     0,
      13,    19,     1,     2,     3,     0,     4,     5,     6,     7,
       0,     0,     0,     8,     9,    10,     0,     0,     0,    11,
       0,     0,    12,    13,    44,     0,     0,     0,    18,     0,
       0,     0,     1,     2,     3,     0,     4,     5,     6,     7,
      14,     0,     0,     0,     0,    15,    16,    17,   144,    11,
       0,    18,     0,    13,    19,     1,     2,     3,     0,     4,
       5,     6,     7,     0,     0,     0,     8,     9,    10,     0,
       0,     0,    11,     0,     0,    12,    13,    44,     0,     0,
       0,    18,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    14,     0,     0,     0,     0,    15,    16,
      17,     0,     0,     0,    18,   146,     0,    19,     1,     2,
       3,     0,     4,     5,     6,     7,     0,   123,     0,     8,
      48,    10,     0,     0,     0,    11,     0,     0,    12,    13,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    14,     0,     0,     0,
       0,    15,    16,    17,     0,     0,     0,    18,     0,     0,
      19,     1,     2,     3,     0,     4,     5,     6,     7,     0,
       0,     0,     8,     9,    10,     0,     0,     0,    11,     0,
       0,    12,    13,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    14,
       0,     0,     0,     0,    15,    16,    17,     0,     0,     0,
      18,     0,   186,    19,     1,     2,     3,     0,     4,     5,
       6,     7,     0,     0,     0,     8,     9,    10,     0,     0,
       0,    11,     0,     0,    12,    13,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    14,     0,     0,     0,     0,    15,    16,    17,
       0,     0,     0,    18,     0,     0,    19,     1,     2,     3,
       0,     4,     5,     6,     7,     0,     0,     0,     8,    48,
      10,     0,     0,     0,    11,     0,     0,    12,    13,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    14,     0,     0,     0,     0,
      15,    16,    17,     0,     0,     0,    18,     0,     0,    19,
       1,     2,   121,     0,     4,     5,     6,     7,     0,     0,
       0,     8,     9,    10,     0,     0,     0,    11,     0,     0,
      12,    13,     0,     0,     0,     1,     2,     3,     0,     4,
       5,     6,     7,    58,     0,     0,     0,     0,    14,     0,
       0,    59,    11,    15,    16,    17,    13,     0,     0,    18,
       0,    60,    19,     0,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,     0,     0,     0,     0,
      44,    94,     0,   -78,    18,     1,     2,     3,     0,     4,
       5,     6,     7,    58,     0,     0,     0,     0,     0,    86,
       0,    59,    11,     0,     0,     0,    13,     0,     0,     0,
       0,    60,     0,     0,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,     0,     0,     0,     0,
      44,     1,     2,     3,    18,     4,     5,     6,     7,    58,
       0,     0,     0,     0,     0,     0,     0,    59,    11,     0,
       0,     0,    13,     0,     0,   168,     0,    60,     0,     0,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,     0,     0,     0,     0,    44,     1,     2,     3,
      18,     4,     5,     6,     7,    58,     0,     0,     0,     0,
       0,     0,   185,    59,    11,     0,     0,     0,    13,     0,
       0,     0,     0,    60,     0,     0,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,     0,     0,
       0,     0,    44,     1,     2,     3,    18,     4,     5,     6,
       7,    58,     0,     0,     0,     0,     0,     0,   204,    59,
      11,     0,     0,     0,    13,     0,     0,     0,     0,    60,
       0,     0,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,     0,     0,     0,     0,    44,     1,
       2,     3,    18,     4,     5,     6,     7,    58,     0,     0,
       0,     0,     0,     0,     0,    59,    11,     0,     0,     0,
      13,     0,     0,     0,     0,    60,     0,     0,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
       0,     0,     0,     0,    44,     1,     2,     3,    18,     4,
       5,     6,     7,     0,     0,     0,     0,     0,     0,     0,
       0,    59,    11,     0,     0,     0,    13,     0,     0,     0,
       0,     0,     0,     0,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,     0,     0,     0,     0,
      44,     1,     2,     3,    18,     4,     5,     6,     7,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    11,     0,
       0,     0,    13,     0,     0,     0,     0,     0,     0,     0,
       0,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,     0,     0,     0,     0,    44,     1,     2,     3,
      18,     4,     5,     6,     7,     1,     2,     3,     0,     4,
       5,     6,     7,    59,    11,     0,     0,     0,    13,     0,
       0,    59,    11,     0,     0,     0,    13,     0,     0,     0,
       0,     0,     0,    68,    69,    70,    71,    72,     0,     0,
       0,     0,    44,    70,    71,    72,    18,     0,     0,     0,
      44,     0,     0,     0,    18
};

static const yytype_int16 yycheck[] =
{
      10,     5,     5,    48,   124,     0,    16,    17,    18,     8,
      18,    18,    45,    18,    15,    43,    49,    20,    43,    19,
      18,     5,    50,    33,   103,    50,     5,    30,    33,     5,
       5,    50,    30,     0,    44,    43,    43,    56,    49,    38,
      51,    20,    87,    18,    20,    48,    50,    48,    58,    59,
      50,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,   183,    74,    75,    76,    77,    78,    48,
      80,   150,    48,    48,    84,    85,     3,     4,     5,    89,
       7,     8,     9,    10,    45,    95,    81,    82,    98,    99,
       5,     5,    19,    20,    93,    45,   175,    24,   177,    49,
       5,    18,     5,    18,    18,    14,    15,    25,    26,    21,
      28,     5,    21,    30,   124,    20,    43,    20,    51,   129,
      53,    48,    50,   133,   134,    52,    20,   137,    49,   149,
      51,    40,    49,    48,    48,    45,    19,    50,    47,    49,
      49,   140,    18,    48,    53,    48,   156,   142,   143,   159,
     160,   161,   172,    45,    48,   165,   176,    49,   178,   179,
      45,    49,    15,    51,    49,     5,    50,    30,    18,    15,
      18,   166,   167,    45,    45,   185,    30,    18,     3,    18,
      41,    41,    12,     7,    18,     7,    12,    -1,    -1,    -1,
      -1,    -1,    -1,   203,   204,   104,   105,    -1,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,    -1,   122,    -1,    -1,   125,    -1,   127,    -1,
      -1,   130,    -1,    -1,    -1,    -1,   135,   136,    -1,    -1,
      -1,    -1,   141,    -1,    -1,    -1,   145,    -1,    -1,     3,
       4,     5,    -1,     7,     8,     9,    10,    -1,    -1,    -1,
      14,    15,    16,   162,   163,   164,    20,    -1,    -1,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   180,    -1,    -1,    -1,   184,    -1,    41,   187,    -1,
      -1,    -1,    46,    47,    48,    -1,    -1,    -1,    52,    53,
      -1,    55,    -1,   202,    -1,    -1,    -1,   206,   207,     3,
       4,     5,    -1,     7,     8,     9,    10,    -1,    12,    -1,
      14,    15,    16,    -1,    -1,    -1,    20,    -1,    -1,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,    -1,     7,     8,     9,    10,    41,    -1,    -1,
      -1,    -1,    46,    47,    48,    19,    20,    -1,    52,    -1,
      24,    55,     3,     4,     5,    -1,     7,     8,     9,    10,
      -1,    -1,    -1,    14,    15,    16,    -1,    -1,    -1,    20,
      -1,    -1,    23,    24,    48,    -1,    -1,    -1,    52,    -1,
      -1,    -1,     3,     4,     5,    -1,     7,     8,     9,    10,
      41,    -1,    -1,    -1,    -1,    46,    47,    48,    49,    20,
      -1,    52,    -1,    24,    55,     3,     4,     5,    -1,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    15,    16,    -1,
      -1,    -1,    20,    -1,    -1,    23,    24,    48,    -1,    -1,
      -1,    52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,    46,    47,
      48,    -1,    -1,    -1,    52,    53,    -1,    55,     3,     4,
       5,    -1,     7,     8,     9,    10,    -1,    12,    -1,    14,
      15,    16,    -1,    -1,    -1,    20,    -1,    -1,    23,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,    -1,
      -1,    46,    47,    48,    -1,    -1,    -1,    52,    -1,    -1,
      55,     3,     4,     5,    -1,     7,     8,     9,    10,    -1,
      -1,    -1,    14,    15,    16,    -1,    -1,    -1,    20,    -1,
      -1,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,
      -1,    -1,    -1,    -1,    46,    47,    48,    -1,    -1,    -1,
      52,    -1,    54,    55,     3,     4,     5,    -1,     7,     8,
       9,    10,    -1,    -1,    -1,    14,    15,    16,    -1,    -1,
      -1,    20,    -1,    -1,    23,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    -1,    -1,    -1,    -1,    46,    47,    48,
      -1,    -1,    -1,    52,    -1,    -1,    55,     3,     4,     5,
      -1,     7,     8,     9,    10,    -1,    -1,    -1,    14,    15,
      16,    -1,    -1,    -1,    20,    -1,    -1,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,
      46,    47,    48,    -1,    -1,    -1,    52,    -1,    -1,    55,
       3,     4,     5,    -1,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    15,    16,    -1,    -1,    -1,    20,    -1,    -1,
      23,    24,    -1,    -1,    -1,     3,     4,     5,    -1,     7,
       8,     9,    10,    11,    -1,    -1,    -1,    -1,    41,    -1,
      -1,    19,    20,    46,    47,    48,    24,    -1,    -1,    52,
      -1,    29,    55,    -1,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    -1,    -1,    -1,    -1,
      48,    49,    -1,    51,    52,     3,     4,     5,    -1,     7,
       8,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,    17,
      -1,    19,    20,    -1,    -1,    -1,    24,    -1,    -1,    -1,
      -1,    29,    -1,    -1,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    -1,    -1,    -1,    -1,
      48,     3,     4,     5,    52,     7,     8,     9,    10,    11,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    -1,
      -1,    -1,    24,    -1,    -1,    27,    -1,    29,    -1,    -1,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    -1,    -1,    -1,    -1,    48,     3,     4,     5,
      52,     7,     8,     9,    10,    11,    -1,    -1,    -1,    -1,
      -1,    -1,    18,    19,    20,    -1,    -1,    -1,    24,    -1,
      -1,    -1,    -1,    29,    -1,    -1,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    -1,    -1,
      -1,    -1,    48,     3,     4,     5,    52,     7,     8,     9,
      10,    11,    -1,    -1,    -1,    -1,    -1,    -1,    18,    19,
      20,    -1,    -1,    -1,    24,    -1,    -1,    -1,    -1,    29,
      -1,    -1,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    -1,    -1,    -1,    -1,    48,     3,
       4,     5,    52,     7,     8,     9,    10,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    20,    -1,    -1,    -1,
      24,    -1,    -1,    -1,    -1,    29,    -1,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      -1,    -1,    -1,    -1,    48,     3,     4,     5,    52,     7,
       8,     9,    10,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,    20,    -1,    -1,    -1,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    -1,    -1,    -1,    -1,
      48,     3,     4,     5,    52,     7,     8,     9,    10,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    -1,
      -1,    -1,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    -1,    -1,    -1,    -1,    48,     3,     4,     5,
      52,     7,     8,     9,    10,     3,     4,     5,    -1,     7,
       8,     9,    10,    19,    20,    -1,    -1,    -1,    24,    -1,
      -1,    19,    20,    -1,    -1,    -1,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    40,    41,    42,    43,    -1,    -1,
      -1,    -1,    48,    41,    42,    43,    52,    -1,    -1,    -1,
      48,    -1,    -1,    -1,    52
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     7,     8,     9,    10,    14,    15,
      16,    20,    23,    24,    41,    46,    47,    48,    52,    55,
      58,    59,    60,    61,    62,    65,    68,    70,    72,    74,
      77,     5,    20,    48,    67,    69,     5,    20,    66,    67,
      59,    48,    65,    75,    48,    60,    60,    59,    15,    59,
      61,    71,    53,    59,    71,     5,    78,     0,    11,    19,
      29,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    60,    45,    21,    19,    50,    50,    71,
      50,    18,    18,    67,    43,    50,    17,    15,    25,    26,
      28,    76,    20,    66,    49,    45,    49,    49,    51,    51,
      53,    50,     5,    50,    59,    59,     5,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,     5,    59,    12,    48,    59,    63,    59,    49,    51,
      59,    61,    61,    43,    50,    59,    59,    30,    73,    20,
      66,    59,    18,    18,    49,    59,    53,     5,    20,    30,
      48,    79,    80,    81,    82,    81,    15,    63,    64,    18,
      45,    45,    59,    59,    59,    30,    18,    18,    27,    61,
      61,    50,    56,    82,    81,    18,    41,    18,    30,    41,
      59,    43,    49,    51,    59,    18,    54,    59,    61,    61,
      49,    49,     3,    82,    49,    81,    82,    81,    82,    82,
       7,    63,    59,    18,    18,    43,    59,    59,     7
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
		  Type, Value); \
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
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
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
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
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

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



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


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


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

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

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 104 "../lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (2)].ast_node_ptr)); }
    break;

  case 3:
#line 105 "../lang/parser.y"
    { parse_stmt_list(ast_root, (yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 6:
#line 113 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_record_access((yyvsp[(1) - (3)].ast_node_ptr), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 7:
#line 114 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 8:
#line 115 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_unop(TOKEN_AMPERSAND, (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 9:
#line 116 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_unop(TOKEN_STAR, (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 10:
#line 117 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 11:
#line 118 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 12:
#line 119 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 13:
#line 120 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 14:
#line 121 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 15:
#line 122 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 16:
#line 123 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 17:
#line 124 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 18:
#line 125 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 19:
#line 126 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 20:
#line 127 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 21:
#line 128 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_application((yyvsp[(3) - (3)].ast_node_ptr), (yyvsp[(1) - (3)].ast_node_ptr)); }
    break;

  case 22:
#line 129 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_assoc((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 23:
#line 130 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 24:
#line 131 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 25:
#line 132 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 26:
#line 133 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 27:
#line 134 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_await((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 28:
#line 138 "../lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[(1) - (1)].vint)); }
    break;

  case 29:
#line 139 "../lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[(1) - (1)].vdouble)); }
    break;

  case 30:
#line 140 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 31:
#line 141 "../lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); }
    break;

  case 32:
#line 142 "../lang/parser.y"
    { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); }
    break;

  case 33:
#line 143 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 34:
#line 144 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;

  case 35:
#line 145 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 36:
#line 146 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 37:
#line 147 "../lang/parser.y"
    { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 38:
#line 148 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_char((yyvsp[(1) - (1)].vchar)); }
    break;

  case 39:
#line 149 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 40:
#line 153 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 41:
#line 154 "../lang/parser.y"
    { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 42:
#line 158 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 43:
#line 159 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr), NULL); }
    break;

  case 44:
#line 161 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), ast_extern_fn((yyvsp[(2) - (4)].vident), (yyvsp[(4) - (4)].ast_node_ptr)), NULL); }
    break;

  case 45:
#line 165 "../lang/parser.y"
    {
                                      Ast *variants = (yyvsp[(5) - (6)].ast_node_ptr);
                                      variants->tag = AST_EXTERN_VARIANTS;
                                      (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(2) - (6)].vident)), variants, NULL);
                                    }
    break;

  case 46:
#line 171 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(4) - (4)].ast_node_ptr); }
    break;

  case 47:
#line 172 "../lang/parser.y"
    {
                                      Ast *let = (yyvsp[(1) - (3)].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[(3) - (3)].ast_node_ptr);
                                      (yyval.ast_node_ptr) = let;
                                    }
    break;

  case 48:
#line 177 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 49:
#line 179 "../lang/parser.y"
    { Ast *lambda = (yyvsp[(2) - (2)].ast_node_ptr);
                                      lambda->data.AST_LAMBDA.is_async = true;
                                      (yyval.ast_node_ptr) = lambda;
                                    }
    break;

  case 50:
#line 186 "../lang/parser.y"
    { (yyval.ast_node_ptr) = extern_typed_signature((yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 51:
#line 188 "../lang/parser.y"
    { (yyval.ast_node_ptr) = extern_typed_signature_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 52:
#line 193 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list(ast_extern_fn((yyvsp[(3) - (3)].vstr), (yyvsp[(1) - (3)].ast_node_ptr))); }
    break;

  case 53:
#line 196 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (5)].ast_node_ptr), ast_extern_fn((yyvsp[(5) - (5)].vstr), (yyvsp[(3) - (5)].ast_node_ptr))); }
    break;

  case 54:
#line 198 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (2)].ast_node_ptr); }
    break;

  case 55:
#line 203 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(2) - (5)].ast_node_ptr), (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 56:
#line 204 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda(NULL, (yyvsp[(4) - (5)].ast_node_ptr)); }
    break;

  case 57:
#line 205 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda((yyvsp[(3) - (6)].ast_node_ptr), (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 58:
#line 206 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_lambda(NULL, (yyvsp[(5) - (6)].ast_node_ptr)); }
    break;

  case 59:
#line 213 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 60:
#line 214 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 61:
#line 215 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 62:
#line 216 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 63:
#line 218 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (1)].ast_node_ptr), NULL); }
    break;

  case 64:
#line 219 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 65:
#line 220 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr), NULL); }
    break;

  case 66:
#line 221 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (4)].ast_node_ptr), (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 67:
#line 225 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 68:
#line 226 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 69:
#line 227 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 70:
#line 237 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 71:
#line 238 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 72:
#line 239 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (4)].ast_node_ptr); }
    break;

  case 73:
#line 243 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), ast_identifier((yyvsp[(3) - (3)].vident))); }
    break;

  case 74:
#line 244 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 75:
#line 248 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 76:
#line 249 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (3)].ast_node_ptr)); }
    break;

  case 77:
#line 250 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple((yyvsp[(2) - (4)].ast_node_ptr)); }
    break;

  case 78:
#line 254 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(1) - (1)].ast_node_ptr)); }
    break;

  case 79:
#line 255 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 80:
#line 259 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_match((yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr)); }
    break;

  case 81:
#line 263 "../lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[(2) - (4)].ast_node_ptr), (yyvsp[(4) - (4)].ast_node_ptr));}
    break;

  case 82:
#line 264 "../lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), (yyvsp[(3) - (5)].ast_node_ptr), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 83:
#line 265 "../lang/parser.y"
    {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[(1) - (5)].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[(5) - (5)].ast_node_ptr));}
    break;

  case 84:
#line 268 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 85:
#line 272 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_empty_list(); }
    break;

  case 86:
#line 273 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (2)].ast_node_ptr), (yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 87:
#line 277 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_string((yyvsp[(1) - (1)].vstr)); }
    break;

  case 88:
#line 278 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 89:
#line 282 "../lang/parser.y"
    {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[(2) - (4)].vident)), (yyvsp[(4) - (4)].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
    break;

  case 90:
#line 288 "../lang/parser.y"
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

  case 91:
#line 301 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list(ast_identifier((yyvsp[(1) - (1)].vident)), NULL); }
    break;

  case 92:
#line 302 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[(1) - (2)].ast_node_ptr), ast_identifier((yyvsp[(2) - (2)].vident)), NULL); }
    break;

  case 93:
#line 305 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 94:
#line 306 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 95:
#line 310 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 96:
#line 311 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 97:
#line 315 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 98:
#line 316 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list((yyvsp[(2) - (2)].ast_node_ptr)); }
    break;

  case 99:
#line 317 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_list_push((yyvsp[(1) - (3)].ast_node_ptr), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 100:
#line 318 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 101:
#line 319 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(1) - (1)].ast_node_ptr); }
    break;

  case 102:
#line 323 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_identifier((yyvsp[(1) - (1)].vident)); }
    break;

  case 103:
#line 324 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[(1) - (3)].vident)), AST_CONST(AST_INT, (yyvsp[(3) - (3)].vint)), NULL); }
    break;

  case 104:
#line 325 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_binop(TOKEN_OF, ast_identifier((yyvsp[(1) - (3)].vident)), (yyvsp[(3) - (3)].ast_node_ptr)); }
    break;

  case 105:
#line 326 "../lang/parser.y"
    { (yyval.ast_node_ptr) = (yyvsp[(2) - (3)].ast_node_ptr); }
    break;

  case 106:
#line 327 "../lang/parser.y"
    { (yyval.ast_node_ptr) = ast_void(); }
    break;


/* Line 1267 of yacc.c.  */
#line 2317 "../lang/y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


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
		      yytoken, &yylval);
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


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
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
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


#line 329 "../lang/parser.y"



void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at line %d near '%s'\n", s, yylineno, yytext);
}

