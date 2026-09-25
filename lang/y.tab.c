/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
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

#define SET_AST_LOC(ast, loc)                                                \
  ast_set_loc((ast), (loc).first_line, (loc).first_column, (loc).last_line,  \
              (loc).last_column)

#define AST_CONST(type, val)                                            \
    ({                                                                  \
      Ast *prefix = Ast_new(type);                                      \
      prefix->data.type.value = val;                                    \
      prefix;                                                           \
    })


#line 102 "lang/y.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "y.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INTEGER = 3,                    /* INTEGER  */
  YYSYMBOL_UINT64 = 4,                     /* UINT64  */
  YYSYMBOL_DOUBLE = 5,                     /* DOUBLE  */
  YYSYMBOL_FLOAT = 6,                      /* FLOAT  */
  YYSYMBOL_IDENTIFIER = 7,                 /* IDENTIFIER  */
  YYSYMBOL_PATH_IDENTIFIER = 8,            /* PATH_IDENTIFIER  */
  YYSYMBOL_IDENTIFIER_LIST = 9,            /* IDENTIFIER_LIST  */
  YYSYMBOL_TOK_STRING = 10,                /* TOK_STRING  */
  YYSYMBOL_TOK_CHAR = 11,                  /* TOK_CHAR  */
  YYSYMBOL_TRUE = 12,                      /* TRUE  */
  YYSYMBOL_FALSE = 13,                     /* FALSE  */
  YYSYMBOL_PIPE = 14,                      /* PIPE  */
  YYSYMBOL_EXTERN = 15,                    /* EXTERN  */
  YYSYMBOL_TRIPLE_DOT = 16,                /* TRIPLE_DOT  */
  YYSYMBOL_DOUBLE_DOT = 17,                /* DOUBLE_DOT  */
  YYSYMBOL_LET = 18,                       /* LET  */
  YYSYMBOL_FN = 19,                        /* FN  */
  YYSYMBOL_MODULE = 20,                    /* MODULE  */
  YYSYMBOL_MATCH = 21,                     /* MATCH  */
  YYSYMBOL_WITH = 22,                      /* WITH  */
  YYSYMBOL_ARROW = 23,                     /* ARROW  */
  YYSYMBOL_DOUBLE_COLON = 24,              /* DOUBLE_COLON  */
  YYSYMBOL_TOK_VOID = 25,                  /* TOK_VOID  */
  YYSYMBOL_IN = 26,                        /* IN  */
  YYSYMBOL_AND = 27,                       /* AND  */
  YYSYMBOL_ASYNC = 28,                     /* ASYNC  */
  YYSYMBOL_DOUBLE_AT = 29,                 /* DOUBLE_AT  */
  YYSYMBOL_AT = 30,                        /* AT  */
  YYSYMBOL_THUNK = 31,                     /* THUNK  */
  YYSYMBOL_IMPORT = 32,                    /* IMPORT  */
  YYSYMBOL_OPEN = 33,                      /* OPEN  */
  YYSYMBOL_IMPLEMENTS = 34,                /* IMPLEMENTS  */
  YYSYMBOL_AMPERSAND = 35,                 /* AMPERSAND  */
  YYSYMBOL_TYPE = 36,                      /* TYPE  */
  YYSYMBOL_TEST_ID = 37,                   /* TEST_ID  */
  YYSYMBOL_MUT = 38,                       /* MUT  */
  YYSYMBOL_THEN = 39,                      /* THEN  */
  YYSYMBOL_ELSE = 40,                      /* ELSE  */
  YYSYMBOL_YIELD = 41,                     /* YIELD  */
  YYSYMBOL_AWAIT = 42,                     /* AWAIT  */
  YYSYMBOL_FOR = 43,                       /* FOR  */
  YYSYMBOL_IF = 44,                        /* IF  */
  YYSYMBOL_OF = 45,                        /* OF  */
  YYSYMBOL_FSTRING_START = 46,             /* FSTRING_START  */
  YYSYMBOL_FSTRING_END = 47,               /* FSTRING_END  */
  YYSYMBOL_FSTRING_INTERP_START = 48,      /* FSTRING_INTERP_START  */
  YYSYMBOL_FSTRING_INTERP_END = 49,        /* FSTRING_INTERP_END  */
  YYSYMBOL_FSTRING_TEXT = 50,              /* FSTRING_TEXT  */
  YYSYMBOL_51_ = 51,                       /* '|'  */
  YYSYMBOL_MATCH_BODY_PREC = 52,           /* MATCH_BODY_PREC  */
  YYSYMBOL_DOUBLE_AMP = 53,                /* DOUBLE_AMP  */
  YYSYMBOL_DOUBLE_PIPE = 54,               /* DOUBLE_PIPE  */
  YYSYMBOL_GE = 55,                        /* GE  */
  YYSYMBOL_LE = 56,                        /* LE  */
  YYSYMBOL_EQ = 57,                        /* EQ  */
  YYSYMBOL_NE = 58,                        /* NE  */
  YYSYMBOL_59_ = 59,                       /* '>'  */
  YYSYMBOL_60_ = 60,                       /* '<'  */
  YYSYMBOL_61_ = 61,                       /* '+'  */
  YYSYMBOL_62_ = 62,                       /* '-'  */
  YYSYMBOL_63_ = 63,                       /* '*'  */
  YYSYMBOL_64_ = 64,                       /* '/'  */
  YYSYMBOL_MODULO = 65,                    /* MODULO  */
  YYSYMBOL_66_ = 66,                       /* ','  */
  YYSYMBOL_67_ = 67,                       /* ':'  */
  YYSYMBOL_APPLICATION = 68,               /* APPLICATION  */
  YYSYMBOL_69_ = 69,                       /* '.'  */
  YYSYMBOL_UMINUS = 70,                    /* UMINUS  */
  YYSYMBOL_BANG = 71,                      /* BANG  */
  YYSYMBOL_72_ = 72,                       /* ';'  */
  YYSYMBOL_73_ = 73,                       /* '='  */
  YYSYMBOL_74_ = 74,                       /* '['  */
  YYSYMBOL_75_ = 75,                       /* ']'  */
  YYSYMBOL_76_ = 76,                       /* '('  */
  YYSYMBOL_77_ = 77,                       /* ')'  */
  YYSYMBOL_78___ = 78,                     /* '_'  */
  YYSYMBOL_YYACCEPT = 79,                  /* $accept  */
  YYSYMBOL_program = 80,                   /* program  */
  YYSYMBOL_expr = 81,                      /* expr  */
  YYSYMBOL_atom_expr = 82,                 /* atom_expr  */
  YYSYMBOL_simple_expr = 83,               /* simple_expr  */
  YYSYMBOL_expr_sequence = 84,             /* expr_sequence  */
  YYSYMBOL_let_binding = 85,               /* let_binding  */
  YYSYMBOL_lambda_expr = 86,               /* lambda_expr  */
  YYSYMBOL_lambda_args = 87,               /* lambda_args  */
  YYSYMBOL_lambda_arg = 88,                /* lambda_arg  */
  YYSYMBOL_list = 89,                      /* list  */
  YYSYMBOL_array = 90,                     /* array  */
  YYSYMBOL_tuple = 91,                     /* tuple  */
  YYSYMBOL_expr_list = 92,                 /* expr_list  */
  YYSYMBOL_match_expr = 93,                /* match_expr  */
  YYSYMBOL_match_test_clause = 94,         /* match_test_clause  */
  YYSYMBOL_match_branches = 95,            /* match_branches  */
  YYSYMBOL_fstring = 96,                   /* fstring  */
  YYSYMBOL_fstring_parts = 97,             /* fstring_parts  */
  YYSYMBOL_fstring_part = 98,              /* fstring_part  */
  YYSYMBOL_type_decl = 99,                 /* type_decl  */
  YYSYMBOL_type_args = 100,                /* type_args  */
  YYSYMBOL_fn_signature = 101,             /* fn_signature  */
  YYSYMBOL_tuple_type = 102,               /* tuple_type  */
  YYSYMBOL_type_expr = 103,                /* type_expr  */
  YYSYMBOL_type_expr_no_tuple = 104,       /* type_expr_no_tuple  */
  YYSYMBOL_type_atom = 105                 /* type_atom  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  98
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2323

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  79
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  161
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  343

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   316


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      76,    77,    63,    61,    66,    62,    69,    64,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    67,    72,
      60,    73,    59,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    74,     2,    75,     2,    78,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    51,     2,     2,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    52,    53,    54,    55,
      56,    57,    58,    65,    68,    70,    71
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   137,   137,   138,   139,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   172,   173,   179,   180,   181,   182,   186,   187,
     194,   204,   205,   206,   207,   208,   209,   210,   211,   212,
     213,   214,   215,   216,   217,   218,   219,   220,   221,   223,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   246,   247,   251,
     252,   253,   256,   258,   260,   269,   270,   276,   278,   286,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   311,
     312,   313,   314,   321,   322,   323,   324,   325,   326,   330,
     331,   332,   333,   338,   339,   340,   344,   345,   346,   351,
     352,   353,   357,   358,   362,   363,   364,   368,   369,   372,
     373,   374,   377,   381,   382,   386,   387,   391,   398,   405,
     424,   425,   428,   429,   433,   434,   438,   439,   443,   444,
     445,   446,   447,   451,   452,   453,   454,   455,   456,   457,
     458,   459
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INTEGER", "UINT64",
  "DOUBLE", "FLOAT", "IDENTIFIER", "PATH_IDENTIFIER", "IDENTIFIER_LIST",
  "TOK_STRING", "TOK_CHAR", "TRUE", "FALSE", "PIPE", "EXTERN",
  "TRIPLE_DOT", "DOUBLE_DOT", "LET", "FN", "MODULE", "MATCH", "WITH",
  "ARROW", "DOUBLE_COLON", "TOK_VOID", "IN", "AND", "ASYNC", "DOUBLE_AT",
  "AT", "THUNK", "IMPORT", "OPEN", "IMPLEMENTS", "AMPERSAND", "TYPE",
  "TEST_ID", "MUT", "THEN", "ELSE", "YIELD", "AWAIT", "FOR", "IF", "OF",
  "FSTRING_START", "FSTRING_END", "FSTRING_INTERP_START",
  "FSTRING_INTERP_END", "FSTRING_TEXT", "'|'", "MATCH_BODY_PREC",
  "DOUBLE_AMP", "DOUBLE_PIPE", "GE", "LE", "EQ", "NE", "'>'", "'<'", "'+'",
  "'-'", "'*'", "'/'", "MODULO", "','", "':'", "APPLICATION", "'.'",
  "UMINUS", "BANG", "';'", "'='", "'['", "']'", "'('", "')'", "'_'",
  "$accept", "program", "expr", "atom_expr", "simple_expr",
  "expr_sequence", "let_binding", "lambda_expr", "lambda_args",
  "lambda_arg", "list", "array", "tuple", "expr_list", "match_expr",
  "match_test_clause", "match_branches", "fstring", "fstring_parts",
  "fstring_part", "type_decl", "type_args", "fn_signature", "tuple_type",
  "type_expr", "type_expr_no_tuple", "type_atom", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-239)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-147)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1984,  -239,  -239,  -239,  -239,  -239,  -239,  -239,  -239,  -239,
    -239,   425,     1,     4,  1984,  -239,  1984,   156,   277,    23,
    1984,  1984,    27,  1984,  -239,  1984,  1456,   716,    10,  1308,
       6,  -239,   -59,    32,  -239,  -239,  -239,  -239,  -239,  -239,
    -239,    98,   -13,    19,  1984,   790,  -239,  1308,    22,    86,
     104,   119,  1984,     8,    65,   127,     9,   177,  1308,  -239,
    -239,  -239,  -239,  -239,  -239,    20,    21,  1308,  1308,    88,
     864,   242,  2247,  1695,  -239,    -9,   102,   108,   501,    71,
     128,   130,   142,   183,   184,   190,   193,   194,   198,   201,
     203,   209,   211,   216,   217,   593,   123,   -44,  -239,  1984,
    1984,  1984,  1984,  1984,  1984,  1984,  1984,  1984,  1984,  1984,
    1984,  1984,  1984,  1984,  1984,  1984,  1769,  1456,     6,   271,
     288,  1984,  1984,    -4,   289,  1530,  1984,  1984,    89,   222,
     -31,  1984,  1984,  1984,  1984,    67,  1984,   101,   226,  1984,
    1984,  1984,   252,  -239,   240,  -239,   240,  1984,  1984,  -239,
    1984,  -239,  -239,   229,   -11,  1816,  -239,  -239,  -239,   136,
     282,   120,  -239,  -239,  -239,  -239,  -239,  -239,  -239,  -239,
    -239,  -239,  -239,  -239,  -239,  -239,  -239,  -239,  1984,  -239,
     668,  -239,  1382,  2247,  1409,  1574,  2028,  2028,  2102,  2102,
    2102,  2102,  2102,  2102,  2125,  2125,  2199,  2199,  2215,  1984,
    1409,   938,  -239,  -239,  1308,  1308,  -239,   233,   290,   300,
    1308,  1308,  1308,  1984,   235,   237,  1308,  1308,  1308,   239,
    -239,   241,   236,  1984,   240,  1308,   243,   246,  1984,   263,
     210,  -239,    40,   240,   296,   254,   298,     2,   306,   298,
    1012,  1086,  1160,  -239,   248,  1863,  -239,  1648,  1984,  1984,
    -239,  1308,  1937,  -239,   173,   240,   197,  1308,  1984,  1984,
    1984,   240,  1308,   -21,  1984,  1984,  1234,   301,   549,    40,
     240,   318,   323,  -239,   261,   -18,     3,   240,   240,   240,
      40,   240,  -239,  1984,  1984,  -239,  -239,   255,   132,   141,
     182,  -239,  2231,  -239,   296,   298,   312,  -239,  1308,  -239,
     -16,  -239,  1984,  1984,   309,   310,  -239,   313,  -239,  -239,
      13,  -239,   221,   298,  -239,   298,  -239,  -239,  1308,  1308,
    -239,   314,   144,  -239,  -239,  -239,   240,  -239,  1308,  1308,
    1984,  1984,  -239,  -239,  1984,  1984,   296,  1308,  1308,   192,
     196,  -239,  -239
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    41,    42,    43,    44,    48,    32,    45,    54,    46,
      47,     0,     0,     0,     0,    49,     0,     0,     0,     0,
       0,     0,     0,     0,   133,     0,     0,     0,     0,    77,
       5,    38,     3,    27,    87,    50,    51,    52,    28,    53,
      29,    48,    49,     0,     0,     0,   112,   122,     0,     0,
     109,     0,     0,     0,   103,     0,     0,     0,    30,    92,
      90,    94,    93,    91,    95,   138,     0,     6,     7,     0,
       0,     0,    31,     0,   113,     0,    48,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    77,     0,     0,     1,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     9,     0,
       0,     2,     0,     0,     0,     0,     0,     0,     0,    48,
       0,     0,     0,     0,     0,     0,     0,   106,     0,     0,
       0,     0,     0,   140,     0,   141,     0,     0,     0,   132,
       0,   135,   134,     0,     0,     0,   114,    76,    73,    48,
       0,     0,    75,    67,    68,    69,    70,    72,    71,    66,
      65,    60,    61,    62,    63,    64,    74,   119,     0,    55,
       0,   120,    23,    25,    26,     8,    17,    18,    19,    20,
      22,    21,    16,    15,    10,    11,    12,    13,    14,     0,
      24,   122,    40,    39,    78,    86,   111,     0,     0,     0,
      80,    85,    79,     0,    76,   120,    82,   123,    83,     0,
     110,     0,     0,     0,     0,   104,     0,     0,     0,   124,
     153,   160,     0,     0,   151,   147,   137,   146,   148,   139,
       0,   126,     0,   116,     0,     0,   115,     0,     0,     0,
     121,    37,     0,    34,     0,     0,     0,    84,     0,   100,
      99,     0,   107,     0,   102,   101,   127,     0,     0,     0,
       0,     0,     0,   149,   147,     0,   146,     0,     0,     0,
       0,     0,   152,     0,     0,   136,   117,     0,     0,     0,
       0,    35,    25,    96,    81,     0,     0,    97,    89,    87,
       0,   105,     0,     0,     0,     0,   155,   156,   161,   154,
       0,   157,     0,   143,   145,   142,   150,   144,    33,   125,
     118,     0,     0,    57,    56,    36,     0,   108,   128,   129,
       0,     0,   159,   158,     0,     0,    98,   131,   130,     0,
       0,    59,    58
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -239,  -239,    -2,   157,  -239,     0,  -239,   -81,   -12,    -5,
    -239,  -239,  -239,    18,  -239,    70,  -239,  -239,  -239,  -239,
    -239,  -239,  -238,   126,  -107,  -229,  -111
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    28,    29,    30,    31,    96,    33,    34,    53,    54,
      35,    36,    37,    49,    38,   267,   229,    39,    71,   152,
      40,    66,   234,   235,   295,   237,   238
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      32,    56,   279,    50,   276,   279,    48,   279,    50,    47,
      98,    50,    57,   121,    58,    50,    50,   294,    67,    68,
     230,    70,   180,    72,    47,    95,    51,   143,   145,    55,
      65,   136,   141,   181,    69,   180,   119,   236,   231,   239,
     244,   307,    47,    95,    75,    97,   215,   230,   137,   314,
      47,   137,   317,   280,   280,   245,   301,   155,   122,   311,
     126,   327,   128,   130,   232,   231,   156,   161,   281,   312,
     135,    47,    52,    48,    46,   120,    47,    52,    50,    46,
      52,   314,    46,   317,    52,    52,    46,    46,   336,   233,
     332,   154,   127,   144,   146,   131,   160,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   200,   201,   233,   263,   206,   204,
     205,   273,   123,   210,   211,   212,   275,    50,   123,   216,
     217,   218,   138,   132,   219,    75,   221,   225,   139,    50,
     226,   227,   134,   249,   220,   240,   241,    52,   242,    46,
     140,    50,   132,   217,   300,   132,   137,   321,   306,   133,
     123,   147,   213,    59,    60,   124,    61,   335,   222,   316,
     313,   125,   315,   293,   223,   297,   204,   299,   217,   157,
       1,     2,     3,     4,     5,   158,   118,     7,     8,     9,
      10,    99,    12,    13,   100,   178,    52,   251,    46,   142,
     179,   101,    15,   124,   118,   162,   102,   163,    52,   247,
      46,   257,   296,   259,   118,   118,    12,    13,   323,   164,
      52,   262,    46,    24,   118,   118,   266,   118,   230,   118,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   217,   116,   210,   231,   230,   289,   290,
     292,   117,   118,    27,   260,   269,   298,   204,   204,   324,
     165,   166,   204,   204,   259,   231,   266,   167,   260,   341,
     168,   169,   232,   342,   202,   170,   322,   270,   171,   271,
     172,   318,   319,   272,    62,    63,   173,    64,   174,   149,
     150,   232,   151,   175,   176,   203,   207,   233,   333,   214,
     328,   329,   224,   228,   243,   248,   254,   256,   258,   255,
    -110,   259,   261,   260,   268,   264,   233,   137,   265,   277,
     278,   279,   282,   286,   303,   308,   309,   310,   337,   338,
     320,   326,   330,   331,   339,   340,  -146,   334,   305,   118,
     118,   118,   118,   118,   118,   118,   118,   118,   118,   118,
     118,   118,   118,   118,   118,   118,     0,   118,   118,   274,
       0,   118,   118,     0,     0,     0,     0,   118,   118,   118,
       0,     0,     0,   118,   118,   118,     0,     0,     0,     0,
       0,     0,   118,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   118,   118,   118,
       0,     0,     0,     0,     0,     0,     0,     0,   118,     0,
       0,     0,     0,     0,   118,     0,     0,     0,     0,   118,
       0,     0,     0,   118,     0,     0,     0,     0,     1,     2,
       3,     4,    41,     0,     6,     7,     8,     9,    10,     0,
       0,     0,     0,    11,    12,    13,    14,     0,     0,   118,
      42,     0,     0,     0,     0,   118,    16,    17,    18,     0,
       0,    19,    43,    44,     0,     0,    20,    21,    22,    23,
       0,    24,     0,     0,     0,   118,   118,     0,     0,     0,
       0,     0,     0,     0,     0,   118,   118,     0,     0,     0,
       0,     0,     0,     0,   118,   118,    25,     0,     0,    26,
       0,    45,     0,    46,     1,     2,     3,     4,   159,     0,
       6,     7,     8,     9,    10,     0,     0,     0,     0,    11,
      12,    13,    14,     0,     0,     0,    42,     0,     0,     0,
       0,     0,    16,    17,    18,     0,     0,    19,    43,    44,
       0,     0,    20,    21,    22,    23,     0,    24,     0,     0,
       0,     0,     1,     2,     3,     4,     5,     0,     6,     7,
       8,     9,    10,     0,     0,     0,     0,    11,    12,    13,
      14,     0,    25,     0,    15,    26,     0,    45,     0,    46,
      16,    17,    18,     0,     0,    19,     0,     0,     0,     0,
      20,    21,    22,    23,     0,    24,     1,     2,     3,     4,
       5,     0,     0,     7,     8,     9,    10,    99,     0,     0,
     100,     0,     0,     0,     0,     0,     0,   101,    15,     0,
      25,     0,   102,    26,     0,    27,     0,   304,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    24,
       0,     0,     0,     0,     0,     0,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,  -122,
     116,     0,     0,     0,     0,     0,     0,   117,     0,    27,
     177,     1,     2,     3,     4,     5,     0,     6,     7,     8,
       9,    10,     0,     0,     0,     0,    11,    12,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,     0,    16,
      17,    18,     0,     0,    19,     0,     0,     0,     0,    20,
      21,    22,    23,     0,    24,     0,     0,     0,     0,     1,
       2,     3,     4,    76,     0,     6,     7,     8,     9,    10,
      77,     0,     0,     0,    78,    79,    13,    14,     0,    25,
      80,    15,    26,     0,    27,   250,     0,    16,    17,    18,
       0,     0,    19,     0,     0,     0,     0,    20,    21,    22,
      23,     0,    24,     0,     0,     0,     0,     0,     0,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,     0,    94,     0,     0,     0,    25,     0,     0,
      26,     0,    27,     1,     2,     3,     4,   129,     0,     6,
       7,     8,     9,    10,    77,     0,     0,     0,    78,    79,
      13,    14,     0,     0,    80,    15,     0,     0,     0,     0,
       0,    16,    17,    18,     0,     0,    19,     0,     0,     0,
       0,    20,    21,    22,    23,     0,    24,     0,     0,     0,
       0,     0,     0,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,     0,    94,     0,     0,
       0,    25,     0,     0,    26,     0,    27,     1,     2,     3,
       4,     5,     0,     0,     7,     8,     9,    10,    99,     0,
       0,   100,     0,     0,     0,     0,     0,     0,   101,    15,
       0,     0,     0,   102,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   148,     0,     0,     0,     0,     0,     0,
      24,     0,     0,     0,     0,     0,     0,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
       0,   116,     0,     0,     0,     0,     0,     0,   117,     0,
      27,     1,     2,     3,     4,     5,     0,     0,     7,     8,
       9,    10,    99,     0,     0,   252,     0,     0,     0,     0,
       0,     0,   101,    15,     0,     0,     0,   102,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    24,     0,     0,     0,     0,     0,
       0,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,     0,   116,     0,     0,     0,     0,
       0,     0,   117,   253,    27,     1,     2,     3,     4,     5,
       0,     0,     7,     8,     9,    10,    99,     0,     0,   100,
       0,     0,     0,     0,     0,     0,   101,    15,   283,     0,
       0,   102,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    24,     0,
       0,     0,     0,     0,     0,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,     0,   116,
       0,     0,     0,     0,     0,     0,   117,     0,    27,     1,
       2,     3,     4,     5,     0,     0,     7,     8,     9,    10,
      99,     0,     0,   100,     0,     0,     0,     0,     0,     0,
     101,    15,     0,     0,     0,   102,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   284,     0,     0,     0,
       0,     0,    24,     0,     0,     0,     0,     0,     0,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,     0,   116,     0,     0,     0,     0,     0,     0,
     117,     0,    27,     1,     2,     3,     4,     5,     0,     0,
       7,     8,     9,    10,    99,     0,     0,   100,     0,     0,
       0,     0,     0,     0,   101,    15,     0,     0,     0,   102,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    24,     0,     0,   285,
       0,     0,     0,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,     0,   116,     0,     0,
       0,     0,     0,     0,   117,     0,    27,     1,     2,     3,
       4,     5,     0,     0,     7,     8,     9,    10,    99,     0,
       0,   100,     0,     0,     0,     0,     0,     0,   101,    15,
       0,     0,     0,   102,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   302,     0,
      24,     0,     0,     0,     0,     0,     0,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
       0,   116,     0,     0,     0,     0,     0,     0,   117,     0,
      27,     1,     2,     3,     4,     5,     0,     0,     7,     8,
       9,    10,    99,     0,     0,   100,     0,     0,     0,     0,
       0,     0,   101,    15,     0,     0,     0,   102,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    24,     0,     0,     0,     0,     0,
       0,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,     0,   116,     0,     0,     0,     0,
       0,     0,   117,     0,    27,     1,     2,     3,     4,     5,
       0,     0,     7,     8,     9,    10,     0,     0,     0,   100,
       0,     0,     0,     0,     0,     0,   101,    15,     0,     0,
       0,   102,     1,     2,     3,     4,     5,     0,     0,     7,
       8,     9,    10,     0,     0,     0,   100,     0,    24,     0,
       0,     0,     0,   101,    15,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,     0,   116,
       0,     0,     0,     0,     0,    24,   117,     0,    27,     1,
       2,     3,     4,     5,     0,     6,     7,     8,     9,    10,
       0,     0,     0,     0,    11,    12,    13,    14,     0,     0,
       0,    15,     0,   117,     0,    27,     0,    16,    17,    18,
       0,     0,    19,     0,     0,     0,     0,    20,    21,    22,
      23,     0,    24,     0,     0,     0,     0,    73,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    25,     0,     0,
      26,    74,    27,     1,     2,     3,     4,     5,     0,     6,
       7,     8,     9,    10,     0,   208,     0,     0,    11,    12,
      13,    14,     0,     0,     0,    15,     0,     0,     0,     0,
     209,    16,    17,    18,     0,     0,    19,     0,     0,     0,
       0,    20,    21,    22,    23,     0,    24,     1,     2,     3,
       4,     5,     0,     0,     7,     8,     9,    10,     0,     0,
       0,   100,     0,     0,     0,     0,     0,     0,   101,    15,
       0,    25,     0,     0,    26,     0,    27,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      24,     0,     0,     0,     0,     0,     0,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
       0,   116,     0,     0,     0,     0,     0,     0,   117,     0,
      27,     1,     2,     3,     4,     5,     0,     6,     7,     8,
       9,    10,     0,   208,     0,     0,    11,   288,    13,    14,
       0,     0,     0,    15,     0,     0,     0,     0,   209,    16,
      17,    18,     0,     0,    19,     0,     0,     0,     0,    20,
      21,    22,    23,     0,    24,     0,     0,     0,     1,     2,
       3,     4,     5,     0,     6,     7,     8,     9,    10,     0,
       0,     0,     0,    11,    12,    13,    14,     0,     0,    25,
      15,     0,    26,     0,    27,     0,    16,    17,    18,     0,
       0,    19,     0,     0,     0,     0,    20,    21,    22,    23,
       0,    24,     0,     0,     0,     0,   153,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    25,     0,     0,    26,
       0,    27,     1,     2,     3,     4,     5,     0,     6,     7,
       8,     9,    10,     0,     0,     0,     0,    11,    12,    13,
      14,     0,     0,     0,    15,     0,     0,     0,     0,     0,
      16,    17,    18,     0,     0,    19,     0,     0,     0,     0,
      20,    21,    22,    23,     0,    24,     0,     0,     0,     1,
       2,     3,     4,     5,     0,     6,     7,     8,     9,    10,
       0,     0,     0,     0,    11,    12,    13,    14,     0,     0,
      25,    15,   199,    26,     0,    27,     0,    16,    17,    18,
       0,     0,    19,     0,     0,     0,     0,    20,    21,    22,
      23,     0,    24,     0,     0,     0,     1,     2,     3,     4,
       5,     0,     6,     7,     8,     9,    10,     0,     0,     0,
       0,    11,    12,    13,    14,     0,     0,    25,    15,     0,
      26,   246,    27,     0,    16,    17,    18,     0,     0,    19,
       0,     0,     0,     0,    20,    21,    22,    23,     0,    24,
       0,     0,     0,     0,   287,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    25,     0,     0,    26,     0,    27,
       1,     2,     3,     4,     5,     0,     6,     7,     8,     9,
      10,     0,     0,     0,     0,    11,    12,    13,    14,     0,
       0,     0,    15,     0,     0,     0,     0,     0,    16,    17,
      18,     0,     0,    19,     0,     0,     0,     0,    20,    21,
      22,    23,     0,    24,     0,     0,     0,     1,     2,     3,
       4,     5,     0,     6,     7,     8,     9,    10,     0,     0,
       0,     0,    11,    12,    13,    14,     0,     0,    25,    15,
       0,    26,   291,    27,     0,    16,    17,    18,     0,     0,
      19,     0,     0,     0,     0,    20,    21,    22,    23,     0,
      24,     1,     2,     3,     4,     5,     0,     0,     7,     8,
       9,    10,     0,     0,     0,   100,     0,     0,     0,     0,
       0,     0,   101,    15,     0,    25,     0,     0,    26,     0,
      27,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    24,     0,     0,     0,     0,     0,
       0,     0,     0,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,     0,   116,     0,     0,     0,     0,
       0,     0,   117,     0,    27,     1,     2,     3,     4,     5,
       0,     0,     7,     8,     9,    10,     0,     0,     0,   100,
       0,     0,     0,     0,     0,     0,   101,    15,     1,     2,
       3,     4,     5,     0,     0,     7,     8,     9,    10,     0,
       0,     0,   100,     0,     0,     0,     0,     0,    24,   101,
      15,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   111,   112,   113,   114,   115,     0,   116,
       0,    24,     0,     0,     0,     0,   117,     0,    27,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   113,   114,
     115,     0,   116,     0,     0,     0,     0,     0,     0,   117,
       0,    27,     1,     2,     3,     4,     5,     0,     0,     7,
       8,     9,    10,     0,     0,     0,   100,     0,     1,     2,
       3,     4,     5,   101,    15,     7,     8,     9,    10,     0,
       0,     0,   100,     0,     1,     2,     3,     4,     5,   101,
      15,     7,     8,     9,    10,    24,     0,     0,     0,     0,
       1,     2,     3,     4,     5,     0,    15,     7,     8,     9,
      10,    24,     0,     0,   115,     0,   116,     0,     0,     0,
       0,     0,    15,   117,     0,    27,     0,    24,     0,     0,
       0,     0,   116,     0,     0,     0,     0,     0,     0,   117,
       0,    27,     0,    24,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   117,   325,    27,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   117,     0,    27
};

static const yytype_int16 yycheck[] =
{
       0,    13,    23,     7,   233,    23,    11,    23,     7,    11,
       0,     7,    14,    72,    16,     7,     7,   255,    20,    21,
       7,    23,    66,    25,    26,    27,    25,     7,     7,    25,
       7,    23,    23,    77,     7,    66,    30,   144,    25,   146,
      51,   270,    44,    45,    26,    27,    77,     7,    53,   278,
      52,    56,   281,    51,    51,    66,    77,    66,    26,    77,
      73,    77,    44,    45,    51,    25,    75,    79,    66,    66,
      52,    73,    76,    78,    78,    69,    78,    76,     7,    78,
      76,   310,    78,   312,    76,    76,    78,    78,   326,    76,
      77,    73,    73,    73,    73,    73,    25,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,    76,   224,   123,   121,
     122,   232,    24,   125,   126,   127,   233,     7,    24,   131,
     132,   133,    67,    66,   134,   117,   136,   139,    73,     7,
     140,   141,    23,    23,    77,   147,   148,    76,   150,    78,
      23,     7,    66,   155,   261,    66,   161,    25,   269,    73,
      24,    73,    73,     7,     8,    67,    10,    23,    67,   280,
     277,    73,   279,   254,    73,   256,   178,   258,   180,    77,
       3,     4,     5,     6,     7,    77,    29,    10,    11,    12,
      13,    14,    19,    20,    17,    72,    76,   199,    78,    22,
      77,    24,    25,    67,    47,    77,    29,    77,    76,    73,
      78,   213,    15,    72,    57,    58,    19,    20,    77,    77,
      76,   223,    78,    46,    67,    68,   228,    70,     7,    72,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,   245,    67,   247,    25,     7,   248,   249,
     252,    74,    95,    76,    72,    45,   258,   259,   260,    77,
      77,    77,   264,   265,    72,    25,   268,    77,    72,    77,
      77,    77,    51,    77,     3,    77,   288,    67,    77,    69,
      77,   283,   284,    73,     7,     8,    77,    10,    77,    47,
      48,    51,    50,    77,    77,     7,     7,    76,    77,    77,
     302,   303,    76,    51,    75,    23,    73,     7,    73,    19,
      73,    72,    76,    72,    51,    72,    76,   322,    72,    23,
      66,    23,    16,    75,    23,     7,     3,    66,   330,   331,
      75,    19,    23,    23,   334,   335,    23,    23,   268,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   192,
     193,   194,   195,   196,   197,   198,    -1,   200,   201,   233,
      -1,   204,   205,    -1,    -1,    -1,    -1,   210,   211,   212,
      -1,    -1,    -1,   216,   217,   218,    -1,    -1,    -1,    -1,
      -1,    -1,   225,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   240,   241,   242,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   251,    -1,
      -1,    -1,    -1,    -1,   257,    -1,    -1,    -1,    -1,   262,
      -1,    -1,    -1,   266,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,     7,    -1,     9,    10,    11,    12,    13,    -1,
      -1,    -1,    -1,    18,    19,    20,    21,    -1,    -1,   292,
      25,    -1,    -1,    -1,    -1,   298,    31,    32,    33,    -1,
      -1,    36,    37,    38,    -1,    -1,    41,    42,    43,    44,
      -1,    46,    -1,    -1,    -1,   318,   319,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   328,   329,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   337,   338,    71,    -1,    -1,    74,
      -1,    76,    -1,    78,     3,     4,     5,     6,     7,    -1,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    -1,    18,
      19,    20,    21,    -1,    -1,    -1,    25,    -1,    -1,    -1,
      -1,    -1,    31,    32,    33,    -1,    -1,    36,    37,    38,
      -1,    -1,    41,    42,    43,    44,    -1,    46,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,     7,    -1,     9,    10,
      11,    12,    13,    -1,    -1,    -1,    -1,    18,    19,    20,
      21,    -1,    71,    -1,    25,    74,    -1,    76,    -1,    78,
      31,    32,    33,    -1,    -1,    36,    -1,    -1,    -1,    -1,
      41,    42,    43,    44,    -1,    46,     3,     4,     5,     6,
       7,    -1,    -1,    10,    11,    12,    13,    14,    -1,    -1,
      17,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,    -1,
      71,    -1,    29,    74,    -1,    76,    -1,    78,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,
      -1,    -1,    -1,    -1,    -1,    -1,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    -1,    -1,    -1,    -1,    -1,    -1,    74,    -1,    76,
      77,     3,     4,     5,     6,     7,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    -1,    -1,    18,    19,    20,    21,
      -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,    31,
      32,    33,    -1,    -1,    36,    -1,    -1,    -1,    -1,    41,
      42,    43,    44,    -1,    46,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,    -1,     9,    10,    11,    12,    13,
      14,    -1,    -1,    -1,    18,    19,    20,    21,    -1,    71,
      24,    25,    74,    -1,    76,    77,    -1,    31,    32,    33,
      -1,    -1,    36,    -1,    -1,    -1,    -1,    41,    42,    43,
      44,    -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    -1,    67,    -1,    -1,    -1,    71,    -1,    -1,
      74,    -1,    76,     3,     4,     5,     6,     7,    -1,     9,
      10,    11,    12,    13,    14,    -1,    -1,    -1,    18,    19,
      20,    21,    -1,    -1,    24,    25,    -1,    -1,    -1,    -1,
      -1,    31,    32,    33,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    41,    42,    43,    44,    -1,    46,    -1,    -1,    -1,
      -1,    -1,    -1,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    -1,    67,    -1,    -1,
      -1,    71,    -1,    -1,    74,    -1,    76,     3,     4,     5,
       6,     7,    -1,    -1,    10,    11,    12,    13,    14,    -1,
      -1,    17,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,
      -1,    -1,    -1,    29,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      -1,    67,    -1,    -1,    -1,    -1,    -1,    -1,    74,    -1,
      76,     3,     4,     5,     6,     7,    -1,    -1,    10,    11,
      12,    13,    14,    -1,    -1,    17,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    -1,    -1,    -1,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    -1,    67,    -1,    -1,    -1,    -1,
      -1,    -1,    74,    75,    76,     3,     4,     5,     6,     7,
      -1,    -1,    10,    11,    12,    13,    14,    -1,    -1,    17,
      -1,    -1,    -1,    -1,    -1,    -1,    24,    25,    26,    -1,
      -1,    29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    -1,    67,
      -1,    -1,    -1,    -1,    -1,    -1,    74,    -1,    76,     3,
       4,     5,     6,     7,    -1,    -1,    10,    11,    12,    13,
      14,    -1,    -1,    17,    -1,    -1,    -1,    -1,    -1,    -1,
      24,    25,    -1,    -1,    -1,    29,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    40,    -1,    -1,    -1,
      -1,    -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    -1,    67,    -1,    -1,    -1,    -1,    -1,    -1,
      74,    -1,    76,     3,     4,     5,     6,     7,    -1,    -1,
      10,    11,    12,    13,    14,    -1,    -1,    17,    -1,    -1,
      -1,    -1,    -1,    -1,    24,    25,    -1,    -1,    -1,    29,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    46,    -1,    -1,    49,
      -1,    -1,    -1,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    -1,    67,    -1,    -1,
      -1,    -1,    -1,    -1,    74,    -1,    76,     3,     4,     5,
       6,     7,    -1,    -1,    10,    11,    12,    13,    14,    -1,
      -1,    17,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,
      -1,    -1,    -1,    29,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,    -1,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      -1,    67,    -1,    -1,    -1,    -1,    -1,    -1,    74,    -1,
      76,     3,     4,     5,     6,     7,    -1,    -1,    10,    11,
      12,    13,    14,    -1,    -1,    17,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    -1,    -1,    -1,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    -1,    67,    -1,    -1,    -1,    -1,
      -1,    -1,    74,    -1,    76,     3,     4,     5,     6,     7,
      -1,    -1,    10,    11,    12,    13,    -1,    -1,    -1,    17,
      -1,    -1,    -1,    -1,    -1,    -1,    24,    25,    -1,    -1,
      -1,    29,     3,     4,     5,     6,     7,    -1,    -1,    10,
      11,    12,    13,    -1,    -1,    -1,    17,    -1,    46,    -1,
      -1,    -1,    -1,    24,    25,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    -1,    67,
      -1,    -1,    -1,    -1,    -1,    46,    74,    -1,    76,     3,
       4,     5,     6,     7,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    -1,    18,    19,    20,    21,    -1,    -1,
      -1,    25,    -1,    74,    -1,    76,    -1,    31,    32,    33,
      -1,    -1,    36,    -1,    -1,    -1,    -1,    41,    42,    43,
      44,    -1,    46,    -1,    -1,    -1,    -1,    51,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,
      74,    75,    76,     3,     4,     5,     6,     7,    -1,     9,
      10,    11,    12,    13,    -1,    15,    -1,    -1,    18,    19,
      20,    21,    -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,
      30,    31,    32,    33,    -1,    -1,    36,    -1,    -1,    -1,
      -1,    41,    42,    43,    44,    -1,    46,     3,     4,     5,
       6,     7,    -1,    -1,    10,    11,    12,    13,    -1,    -1,
      -1,    17,    -1,    -1,    -1,    -1,    -1,    -1,    24,    25,
      -1,    71,    -1,    -1,    74,    -1,    76,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      -1,    67,    -1,    -1,    -1,    -1,    -1,    -1,    74,    -1,
      76,     3,     4,     5,     6,     7,    -1,     9,    10,    11,
      12,    13,    -1,    15,    -1,    -1,    18,    19,    20,    21,
      -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,    30,    31,
      32,    33,    -1,    -1,    36,    -1,    -1,    -1,    -1,    41,
      42,    43,    44,    -1,    46,    -1,    -1,    -1,     3,     4,
       5,     6,     7,    -1,     9,    10,    11,    12,    13,    -1,
      -1,    -1,    -1,    18,    19,    20,    21,    -1,    -1,    71,
      25,    -1,    74,    -1,    76,    -1,    31,    32,    33,    -1,
      -1,    36,    -1,    -1,    -1,    -1,    41,    42,    43,    44,
      -1,    46,    -1,    -1,    -1,    -1,    51,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,    74,
      -1,    76,     3,     4,     5,     6,     7,    -1,     9,    10,
      11,    12,    13,    -1,    -1,    -1,    -1,    18,    19,    20,
      21,    -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,
      31,    32,    33,    -1,    -1,    36,    -1,    -1,    -1,    -1,
      41,    42,    43,    44,    -1,    46,    -1,    -1,    -1,     3,
       4,     5,     6,     7,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    -1,    18,    19,    20,    21,    -1,    -1,
      71,    25,    73,    74,    -1,    76,    -1,    31,    32,    33,
      -1,    -1,    36,    -1,    -1,    -1,    -1,    41,    42,    43,
      44,    -1,    46,    -1,    -1,    -1,     3,     4,     5,     6,
       7,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      -1,    18,    19,    20,    21,    -1,    -1,    71,    25,    -1,
      74,    75,    76,    -1,    31,    32,    33,    -1,    -1,    36,
      -1,    -1,    -1,    -1,    41,    42,    43,    44,    -1,    46,
      -1,    -1,    -1,    -1,    51,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    71,    -1,    -1,    74,    -1,    76,
       3,     4,     5,     6,     7,    -1,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    -1,    18,    19,    20,    21,    -1,
      -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,    31,    32,
      33,    -1,    -1,    36,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    -1,    46,    -1,    -1,    -1,     3,     4,     5,
       6,     7,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    -1,    18,    19,    20,    21,    -1,    -1,    71,    25,
      -1,    74,    75,    76,    -1,    31,    32,    33,    -1,    -1,
      36,    -1,    -1,    -1,    -1,    41,    42,    43,    44,    -1,
      46,     3,     4,     5,     6,     7,    -1,    -1,    10,    11,
      12,    13,    -1,    -1,    -1,    17,    -1,    -1,    -1,    -1,
      -1,    -1,    24,    25,    -1,    71,    -1,    -1,    74,    -1,
      76,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    -1,    67,    -1,    -1,    -1,    -1,
      -1,    -1,    74,    -1,    76,     3,     4,     5,     6,     7,
      -1,    -1,    10,    11,    12,    13,    -1,    -1,    -1,    17,
      -1,    -1,    -1,    -1,    -1,    -1,    24,    25,     3,     4,
       5,     6,     7,    -1,    -1,    10,    11,    12,    13,    -1,
      -1,    -1,    17,    -1,    -1,    -1,    -1,    -1,    46,    24,
      25,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    61,    62,    63,    64,    65,    -1,    67,
      -1,    46,    -1,    -1,    -1,    -1,    74,    -1,    76,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    63,    64,
      65,    -1,    67,    -1,    -1,    -1,    -1,    -1,    -1,    74,
      -1,    76,     3,     4,     5,     6,     7,    -1,    -1,    10,
      11,    12,    13,    -1,    -1,    -1,    17,    -1,     3,     4,
       5,     6,     7,    24,    25,    10,    11,    12,    13,    -1,
      -1,    -1,    17,    -1,     3,     4,     5,     6,     7,    24,
      25,    10,    11,    12,    13,    46,    -1,    -1,    -1,    -1,
       3,     4,     5,     6,     7,    -1,    25,    10,    11,    12,
      13,    46,    -1,    -1,    65,    -1,    67,    -1,    -1,    -1,
      -1,    -1,    25,    74,    -1,    76,    -1,    46,    -1,    -1,
      -1,    -1,    67,    -1,    -1,    -1,    -1,    -1,    -1,    74,
      -1,    76,    -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    74,    75,    76,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    74,    -1,    76
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     9,    10,    11,    12,
      13,    18,    19,    20,    21,    25,    31,    32,    33,    36,
      41,    42,    43,    44,    46,    71,    74,    76,    80,    81,
      82,    83,    84,    85,    86,    89,    90,    91,    93,    96,
      99,     7,    25,    37,    38,    76,    78,    81,    88,    92,
       7,    25,    76,    87,    88,    25,    87,    81,    81,     7,
       8,    10,     7,     8,    10,     7,   100,    81,    81,     7,
      81,    97,    81,    51,    75,    92,     7,    14,    18,    19,
      24,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    67,    81,    84,    92,     0,    14,
      17,    24,    29,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    67,    74,    82,    30,
      69,    72,    26,    24,    67,    73,    73,    73,    92,     7,
      92,    73,    66,    73,    23,    92,    23,    88,    67,    73,
      23,    23,    22,     7,    73,     7,    73,    73,    39,    47,
      48,    50,    98,    51,    92,    66,    75,    77,    77,     7,
      25,    87,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    72,    77,
      66,    77,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    73,
      81,    81,     3,     7,    81,    81,    88,     7,    15,    30,
      81,    81,    81,    73,    77,    77,    81,    81,    81,    84,
      77,    84,    67,    73,    76,    81,    84,    84,    51,    95,
       7,    25,    51,    76,   101,   102,   103,   104,   105,   103,
      81,    81,    81,    75,    51,    66,    75,    73,    23,    23,
      77,    81,    17,    75,    73,    19,     7,    81,    73,    72,
      72,    76,    81,   103,    72,    72,    81,    94,    51,    45,
      67,    69,    73,   105,   102,   103,   104,    23,    66,    23,
      51,    66,    16,    26,    40,    49,    75,    51,    19,    84,
      84,    75,    81,    86,   101,   103,    15,    86,    81,    86,
     103,    77,    44,    23,    78,    94,   105,   104,     7,     3,
      66,    77,    66,   103,   104,   103,   105,   104,    81,    81,
      75,    25,    87,    77,    77,    75,    19,    77,    81,    81,
      23,    23,    77,    77,    23,    23,   101,    81,    81,    84,
      84,    77,    77
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    79,    80,    80,    80,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    82,    82,
      82,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    84,    84,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    86,
      86,    86,    86,    87,    87,    87,    87,    87,    87,    88,
      88,    88,    88,    89,    89,    89,    90,    90,    90,    91,
      91,    91,    92,    92,    93,    93,    93,    94,    94,    95,
      95,    95,    96,    97,    97,    98,    98,    99,    99,    99,
     100,   100,   101,   101,   102,   102,   103,   103,   104,   104,
     104,   104,   104,   105,   105,   105,   105,   105,   105,   105,
     105,   105
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     2,     2,     3,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     1,     1,
       2,     2,     1,     6,     4,     5,     6,     4,     1,     3,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     6,     6,     9,     9,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     3,     4,
       4,     6,     4,     4,     5,     4,     3,     1,     6,     6,
       2,     2,     2,     2,     2,     2,     6,     6,     8,     5,
       5,     5,     5,     1,     3,     5,     2,     4,     6,     1,
       3,     3,     1,     2,     3,     4,     4,     5,     6,     3,
       3,     4,     1,     3,     4,     6,     4,     1,     3,     4,
       5,     5,     3,     0,     2,     1,     3,     4,     2,     4,
       2,     2,     3,     3,     3,     3,     1,     1,     1,     2,
       3,     1,     2,     1,     3,     3,     3,     3,     4,     4,
       1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: expr_sequence ';'  */
#line 137 "lang/parser.y"
                      { pctx.ast_root = parse_stmt_list(pctx.ast_root, (yyvsp[-1].ast_node_ptr)); }
#line 1936 "lang/y.tab.c"
    break;

  case 3: /* program: expr_sequence  */
#line 138 "lang/parser.y"
                      { pctx.ast_root = parse_stmt_list(pctx.ast_root, (yyvsp[0].ast_node_ptr)); }
#line 1942 "lang/y.tab.c"
    break;

  case 6: /* expr: YIELD expr  */
#line 145 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_yield((yyvsp[0].ast_node_ptr)); }
#line 1948 "lang/y.tab.c"
    break;

  case 7: /* expr: AWAIT expr  */
#line 146 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_await((yyvsp[0].ast_node_ptr)); }
#line 1954 "lang/y.tab.c"
    break;

  case 8: /* expr: expr DOUBLE_AT expr  */
#line 147 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1960 "lang/y.tab.c"
    break;

  case 9: /* expr: expr atom_expr  */
#line 148 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1966 "lang/y.tab.c"
    break;

  case 10: /* expr: expr '+' expr  */
#line 149 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1972 "lang/y.tab.c"
    break;

  case 11: /* expr: expr '-' expr  */
#line 150 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1978 "lang/y.tab.c"
    break;

  case 12: /* expr: expr '*' expr  */
#line 151 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1984 "lang/y.tab.c"
    break;

  case 13: /* expr: expr '/' expr  */
#line 152 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1990 "lang/y.tab.c"
    break;

  case 14: /* expr: expr MODULO expr  */
#line 153 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1996 "lang/y.tab.c"
    break;

  case 15: /* expr: expr '<' expr  */
#line 154 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2002 "lang/y.tab.c"
    break;

  case 16: /* expr: expr '>' expr  */
#line 155 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2008 "lang/y.tab.c"
    break;

  case 17: /* expr: expr DOUBLE_AMP expr  */
#line 156 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_AMP, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2014 "lang/y.tab.c"
    break;

  case 18: /* expr: expr DOUBLE_PIPE expr  */
#line 157 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_PIPE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2020 "lang/y.tab.c"
    break;

  case 19: /* expr: expr GE expr  */
#line 158 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2026 "lang/y.tab.c"
    break;

  case 20: /* expr: expr LE expr  */
#line 159 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2032 "lang/y.tab.c"
    break;

  case 21: /* expr: expr NE expr  */
#line 160 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2038 "lang/y.tab.c"
    break;

  case 22: /* expr: expr EQ expr  */
#line 161 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2044 "lang/y.tab.c"
    break;

  case 23: /* expr: expr PIPE expr  */
#line 162 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[0].ast_node_ptr), (yyvsp[-2].ast_node_ptr)); }
#line 2050 "lang/y.tab.c"
    break;

  case 24: /* expr: expr ':' expr  */
#line 163 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_assoc((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2056 "lang/y.tab.c"
    break;

  case 25: /* expr: expr DOUBLE_DOT expr  */
#line 164 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_range_expression((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2062 "lang/y.tab.c"
    break;

  case 26: /* expr: expr DOUBLE_COLON expr  */
#line 165 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2068 "lang/y.tab.c"
    break;

  case 27: /* expr: let_binding  */
#line 166 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2074 "lang/y.tab.c"
    break;

  case 28: /* expr: match_expr  */
#line 167 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2080 "lang/y.tab.c"
    break;

  case 29: /* expr: type_decl  */
#line 168 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2086 "lang/y.tab.c"
    break;

  case 30: /* expr: THUNK expr  */
#line 169 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_thunk_expr((yyvsp[0].ast_node_ptr)); }
#line 2092 "lang/y.tab.c"
    break;

  case 31: /* expr: BANG expr  */
#line 170 "lang/parser.y"
                                       { (yyval.ast_node_ptr) = ast_application(ast_identifier((ObjString){"!", 1}), (yyvsp[0].ast_node_ptr)); }
#line 2098 "lang/y.tab.c"
    break;

  case 32: /* expr: IDENTIFIER_LIST  */
#line 172 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_typed_empty_list((yyvsp[0].vident)); }
#line 2104 "lang/y.tab.c"
    break;

  case 33: /* expr: FOR IDENTIFIER '=' expr IN expr  */
#line 173 "lang/parser.y"
                                      {
                                          Ast *let = ast_let(ast_identifier((yyvsp[-4].vident)), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));
                                          let->tag = AST_LOOP;
                                          (yyval.ast_node_ptr) = let;

                                      }
#line 2115 "lang/y.tab.c"
    break;

  case 34: /* expr: expr '[' expr ']'  */
#line 179 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = array_index_expression((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr));}
#line 2121 "lang/y.tab.c"
    break;

  case 35: /* expr: expr '[' expr DOUBLE_DOT ']'  */
#line 180 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = array_offset_expression((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr));}
#line 2127 "lang/y.tab.c"
    break;

  case 36: /* expr: expr '[' expr DOUBLE_DOT expr ']'  */
#line 181 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = array_range_expression((yyvsp[-5].ast_node_ptr), (yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr));}
#line 2133 "lang/y.tab.c"
    break;

  case 37: /* expr: expr ':' '=' expr  */
#line 182 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_assignment((yyvsp[-3].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2139 "lang/y.tab.c"
    break;

  case 39: /* atom_expr: atom_expr '.' IDENTIFIER  */
#line 187 "lang/parser.y"
                                      {
                                        Ast *member = ast_identifier((yyvsp[0].vident));
                                        SET_AST_LOC(member, (yylsp[0]));
                                        (yyval.ast_node_ptr) = ast_record_access((yyvsp[-2].ast_node_ptr), member);
                                        SET_AST_LOC((yyval.ast_node_ptr), (yyloc));
                                      }
#line 2150 "lang/y.tab.c"
    break;

  case 40: /* atom_expr: atom_expr AT INTEGER  */
#line 194 "lang/parser.y"
                                 {
                                        // Ast *member = ast_identifier($3);
                                        Ast *member = AST_CONST(AST_INT, (yyvsp[0].vint));
                                        SET_AST_LOC(member, (yylsp[0]));
                                        (yyval.ast_node_ptr) = ast_record_access((yyvsp[-2].ast_node_ptr), member);
                                        SET_AST_LOC((yyval.ast_node_ptr), (yyloc));
                                      }
#line 2162 "lang/y.tab.c"
    break;

  case 41: /* simple_expr: INTEGER  */
#line 204 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[0].vint)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2168 "lang/y.tab.c"
    break;

  case 42: /* simple_expr: UINT64  */
#line 205 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_UINT64, (yyvsp[0].vint64)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2174 "lang/y.tab.c"
    break;

  case 43: /* simple_expr: DOUBLE  */
#line 206 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[0].vdouble)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2180 "lang/y.tab.c"
    break;

  case 44: /* simple_expr: FLOAT  */
#line 207 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_FLOAT, (yyvsp[0].vfloat)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2186 "lang/y.tab.c"
    break;

  case 45: /* simple_expr: TOK_STRING  */
#line 208 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_string((yyvsp[0].vstr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2192 "lang/y.tab.c"
    break;

  case 46: /* simple_expr: TRUE  */
#line 209 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2198 "lang/y.tab.c"
    break;

  case 47: /* simple_expr: FALSE  */
#line 210 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2204 "lang/y.tab.c"
    break;

  case 48: /* simple_expr: IDENTIFIER  */
#line 211 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2210 "lang/y.tab.c"
    break;

  case 49: /* simple_expr: TOK_VOID  */
#line 212 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_void(); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2216 "lang/y.tab.c"
    break;

  case 50: /* simple_expr: list  */
#line 213 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2222 "lang/y.tab.c"
    break;

  case 51: /* simple_expr: array  */
#line 214 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2228 "lang/y.tab.c"
    break;

  case 52: /* simple_expr: tuple  */
#line 215 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2234 "lang/y.tab.c"
    break;

  case 53: /* simple_expr: fstring  */
#line 216 "lang/parser.y"
                          { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[0].ast_node_ptr)); }
#line 2240 "lang/y.tab.c"
    break;

  case 54: /* simple_expr: TOK_CHAR  */
#line 217 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_char((yyvsp[0].vchar)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2246 "lang/y.tab.c"
    break;

  case 55: /* simple_expr: '(' expr_sequence ')'  */
#line 218 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2252 "lang/y.tab.c"
    break;

  case 56: /* simple_expr: '(' FN lambda_args ARROW expr_sequence ')'  */
#line 219 "lang/parser.y"
                                               { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2258 "lang/y.tab.c"
    break;

  case 57: /* simple_expr: '(' FN TOK_VOID ARROW expr_sequence ')'  */
#line 220 "lang/parser.y"
                                               { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2264 "lang/y.tab.c"
    break;

  case 58: /* simple_expr: '(' LET IDENTIFIER '=' FN lambda_args ARROW expr_sequence ')'  */
#line 222 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-6].vident)), ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2270 "lang/y.tab.c"
    break;

  case 59: /* simple_expr: '(' LET IDENTIFIER '=' FN TOK_VOID ARROW expr_sequence ')'  */
#line 224 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-6].vident)), ast_void_lambda((yyvsp[-1].ast_node_ptr)), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2276 "lang/y.tab.c"
    break;

  case 60: /* simple_expr: '(' '+' ')'  */
#line 225 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"+", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2282 "lang/y.tab.c"
    break;

  case 61: /* simple_expr: '(' '-' ')'  */
#line 226 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"-", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2288 "lang/y.tab.c"
    break;

  case 62: /* simple_expr: '(' '*' ')'  */
#line 227 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"*", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2294 "lang/y.tab.c"
    break;

  case 63: /* simple_expr: '(' '/' ')'  */
#line 228 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"/", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2300 "lang/y.tab.c"
    break;

  case 64: /* simple_expr: '(' MODULO ')'  */
#line 229 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"%", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2306 "lang/y.tab.c"
    break;

  case 65: /* simple_expr: '(' '<' ')'  */
#line 230 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2312 "lang/y.tab.c"
    break;

  case 66: /* simple_expr: '(' '>' ')'  */
#line 231 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){">", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2318 "lang/y.tab.c"
    break;

  case 67: /* simple_expr: '(' DOUBLE_AMP ')'  */
#line 232 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"&&", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2324 "lang/y.tab.c"
    break;

  case 68: /* simple_expr: '(' DOUBLE_PIPE ')'  */
#line 233 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"||", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2330 "lang/y.tab.c"
    break;

  case 69: /* simple_expr: '(' GE ')'  */
#line 234 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){">=", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2336 "lang/y.tab.c"
    break;

  case 70: /* simple_expr: '(' LE ')'  */
#line 235 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<=", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2342 "lang/y.tab.c"
    break;

  case 71: /* simple_expr: '(' NE ')'  */
#line 236 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"!=", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2348 "lang/y.tab.c"
    break;

  case 72: /* simple_expr: '(' EQ ')'  */
#line 237 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"==", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2354 "lang/y.tab.c"
    break;

  case 73: /* simple_expr: '(' PIPE ')'  */
#line 238 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"|", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2360 "lang/y.tab.c"
    break;

  case 74: /* simple_expr: '(' ':' ')'  */
#line 239 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){":", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2366 "lang/y.tab.c"
    break;

  case 75: /* simple_expr: '(' DOUBLE_COLON ')'  */
#line 240 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"::", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2372 "lang/y.tab.c"
    break;

  case 76: /* simple_expr: '(' IDENTIFIER ')'  */
#line 241 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_identifier((yyvsp[-1].vident)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2378 "lang/y.tab.c"
    break;

  case 77: /* expr_sequence: expr  */
#line 246 "lang/parser.y"
                                { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2384 "lang/y.tab.c"
    break;

  case 78: /* expr_sequence: expr_sequence ';' expr  */
#line 247 "lang/parser.y"
                                { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2390 "lang/y.tab.c"
    break;

  case 79: /* let_binding: LET TEST_ID '=' expr  */
#line 251 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_test_module((yyvsp[0].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2396 "lang/y.tab.c"
    break;

  case 80: /* let_binding: LET IDENTIFIER '=' expr  */
#line 252 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2402 "lang/y.tab.c"
    break;

  case 81: /* let_binding: LET IDENTIFIER '=' EXTERN FN fn_signature  */
#line 254 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-4].vident)), ast_extern_fn((yyvsp[-4].vident), (yyvsp[0].ast_node_ptr)), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2408 "lang/y.tab.c"
    break;

  case 82: /* let_binding: LET lambda_arg '=' expr  */
#line 256 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2414 "lang/y.tab.c"
    break;

  case 83: /* let_binding: LET expr_list '=' expr  */
#line 258 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_tuple((yyvsp[-2].ast_node_ptr)), (yyvsp[0].ast_node_ptr), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc));}
#line 2420 "lang/y.tab.c"
    break;

  case 84: /* let_binding: LET MUT expr_list '=' expr  */
#line 260 "lang/parser.y"
                                    { Ast *let = ast_let(ast_tuple((yyvsp[-2].ast_node_ptr)), (yyvsp[0].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      SET_AST_LOC(let, (yyloc));
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2430 "lang/y.tab.c"
    break;

  case 85: /* let_binding: LET TOK_VOID '=' expr  */
#line 269 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2436 "lang/y.tab.c"
    break;

  case 86: /* let_binding: let_binding IN expr  */
#line 270 "lang/parser.y"
                                    {
                                      Ast *let = (yyvsp[-2].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[0].ast_node_ptr);
                                      SET_AST_LOC(let, (yyloc));
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2447 "lang/y.tab.c"
    break;

  case 87: /* let_binding: lambda_expr  */
#line 276 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2453 "lang/y.tab.c"
    break;

  case 88: /* let_binding: LET '(' IDENTIFIER ')' '=' lambda_expr  */
#line 279 "lang/parser.y"
                                    {
                                      Ast *id = ast_identifier((yyvsp[-3].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[0].ast_node_ptr), NULL);
                                      SET_AST_LOC((yyval.ast_node_ptr), (yyloc));
                                    }
#line 2464 "lang/y.tab.c"
    break;

  case 89: /* let_binding: LET '(' IDENTIFIER ')' '=' expr  */
#line 287 "lang/parser.y"
                                    {
                                      Ast *id = ast_identifier((yyvsp[-3].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[0].ast_node_ptr), NULL);
                                      SET_AST_LOC((yyval.ast_node_ptr), (yyloc));
                                    }
#line 2475 "lang/y.tab.c"
    break;

  case 90: /* let_binding: IMPORT PATH_IDENTIFIER  */
#line 297 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), false); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2481 "lang/y.tab.c"
    break;

  case 91: /* let_binding: OPEN PATH_IDENTIFIER  */
#line 298 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), true); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2487 "lang/y.tab.c"
    break;

  case 92: /* let_binding: IMPORT IDENTIFIER  */
#line 299 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), false); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2493 "lang/y.tab.c"
    break;

  case 93: /* let_binding: OPEN IDENTIFIER  */
#line 300 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), true); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2499 "lang/y.tab.c"
    break;

  case 94: /* let_binding: IMPORT TOK_STRING  */
#line 301 "lang/parser.y"
                                            { (yyval.ast_node_ptr) = ast_import_from_uri((yyvsp[0].vstr), false); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2505 "lang/y.tab.c"
    break;

  case 95: /* let_binding: OPEN TOK_STRING  */
#line 302 "lang/parser.y"
                                             { (yyval.ast_node_ptr) = ast_import_from_uri((yyvsp[0].vstr), true); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2511 "lang/y.tab.c"
    break;

  case 96: /* let_binding: LET IDENTIFIER ':' IDENTIFIER '=' lambda_expr  */
#line 303 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_trait_impl((yyvsp[-2].vident), (yyvsp[-4].vident), (yyvsp[0].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2517 "lang/y.tab.c"
    break;

  case 97: /* let_binding: LET IDENTIFIER '=' AT IDENTIFIER lambda_expr  */
#line 304 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_decorated_lambda((yyvsp[-1].vident), (yyvsp[-4].vident), (yyvsp[0].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2523 "lang/y.tab.c"
    break;

  case 98: /* let_binding: LET IDENTIFIER '=' AT IDENTIFIER EXTERN FN fn_signature  */
#line 305 "lang/parser.y"
                                                             { (yyval.ast_node_ptr) = ast_decorated_signature((yyvsp[-3].vident), (yyvsp[-6].vident), (yyvsp[0].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2529 "lang/y.tab.c"
    break;

  case 99: /* lambda_expr: FN lambda_args ARROW expr_sequence ';'  */
#line 311 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2535 "lang/y.tab.c"
    break;

  case 100: /* lambda_expr: FN TOK_VOID ARROW expr_sequence ';'  */
#line 312 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2541 "lang/y.tab.c"
    break;

  case 101: /* lambda_expr: MODULE lambda_args ARROW expr_sequence ';'  */
#line 313 "lang/parser.y"
                                              { (yyval.ast_node_ptr) = ast_module(ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr))); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2547 "lang/y.tab.c"
    break;

  case 102: /* lambda_expr: MODULE TOK_VOID ARROW expr_sequence ';'  */
#line 314 "lang/parser.y"
                                              { (yyval.ast_node_ptr) = ast_module(ast_lambda(NULL, (yyvsp[-1].ast_node_ptr))); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2553 "lang/y.tab.c"
    break;

  case 103: /* lambda_args: lambda_arg  */
#line 321 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[0].ast_node_ptr), NULL); }
#line 2559 "lang/y.tab.c"
    break;

  case 104: /* lambda_args: lambda_arg '=' expr  */
#line 322 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list(ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL), NULL); }
#line 2565 "lang/y.tab.c"
    break;

  case 105: /* lambda_args: lambda_arg ':' '(' type_expr ')'  */
#line 323 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[-4].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2571 "lang/y.tab.c"
    break;

  case 106: /* lambda_args: lambda_args lambda_arg  */
#line 324 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); }
#line 2577 "lang/y.tab.c"
    break;

  case 107: /* lambda_args: lambda_args lambda_arg '=' expr  */
#line 325 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-3].ast_node_ptr), ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL), NULL); }
#line 2583 "lang/y.tab.c"
    break;

  case 108: /* lambda_args: lambda_args lambda_arg ':' '(' type_expr ')'  */
#line 326 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-5].ast_node_ptr), (yyvsp[-4].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2589 "lang/y.tab.c"
    break;

  case 109: /* lambda_arg: IDENTIFIER  */
#line 330 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2595 "lang/y.tab.c"
    break;

  case 110: /* lambda_arg: '(' expr_list ')'  */
#line 331 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-1].ast_node_ptr)); }
#line 2601 "lang/y.tab.c"
    break;

  case 111: /* lambda_arg: IDENTIFIER DOUBLE_COLON lambda_arg  */
#line 332 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2607 "lang/y.tab.c"
    break;

  case 112: /* lambda_arg: '_'  */
#line 333 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = Ast_new(AST_PLACEHOLDER_ID); }
#line 2613 "lang/y.tab.c"
    break;

  case 113: /* list: '[' ']'  */
#line 338 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_empty_list(); }
#line 2619 "lang/y.tab.c"
    break;

  case 114: /* list: '[' expr_list ']'  */
#line 339 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2625 "lang/y.tab.c"
    break;

  case 115: /* list: '[' expr_list ',' ']'  */
#line 340 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[-2].ast_node_ptr); }
#line 2631 "lang/y.tab.c"
    break;

  case 116: /* array: '[' '|' '|' ']'  */
#line 344 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_empty_array(); }
#line 2637 "lang/y.tab.c"
    break;

  case 117: /* array: '[' '|' expr_list '|' ']'  */
#line 345 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[-2].ast_node_ptr)); }
#line 2643 "lang/y.tab.c"
    break;

  case 118: /* array: '[' '|' expr_list ',' '|' ']'  */
#line 346 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[-3].ast_node_ptr)); }
#line 2649 "lang/y.tab.c"
    break;

  case 119: /* tuple: '(' expr ')'  */
#line 351 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2655 "lang/y.tab.c"
    break;

  case 120: /* tuple: '(' expr_list ')'  */
#line 352 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-1].ast_node_ptr)); }
#line 2661 "lang/y.tab.c"
    break;

  case 121: /* tuple: '(' expr_list ',' ')'  */
#line 353 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-2].ast_node_ptr)); }
#line 2667 "lang/y.tab.c"
    break;

  case 122: /* expr_list: expr  */
#line 357 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_list((yyvsp[0].ast_node_ptr)); }
#line 2673 "lang/y.tab.c"
    break;

  case 123: /* expr_list: expr_list ',' expr  */
#line 358 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2679 "lang/y.tab.c"
    break;

  case 124: /* match_expr: MATCH expr WITH match_branches  */
#line 362 "lang/parser.y"
                                   { (yyval.ast_node_ptr) = ast_match((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2685 "lang/y.tab.c"
    break;

  case 125: /* match_expr: IF expr THEN expr ELSE expr  */
#line 363 "lang/parser.y"
                                 { (yyval.ast_node_ptr) = ast_if_else((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr) ,(yyvsp[0].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc));}
#line 2691 "lang/y.tab.c"
    break;

  case 126: /* match_expr: IF expr THEN expr  */
#line 364 "lang/parser.y"
                                 { (yyval.ast_node_ptr) = ast_if_else((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc));}
#line 2697 "lang/y.tab.c"
    break;

  case 127: /* match_test_clause: expr  */
#line 368 "lang/parser.y"
         {(yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr);}
#line 2703 "lang/y.tab.c"
    break;

  case 128: /* match_test_clause: expr IF expr  */
#line 369 "lang/parser.y"
                 { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2709 "lang/y.tab.c"
    break;

  case 129: /* match_branches: '|' match_test_clause ARROW expr  */
#line 372 "lang/parser.y"
                                                                           {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2715 "lang/y.tab.c"
    break;

  case 130: /* match_branches: match_branches '|' match_test_clause ARROW expr  */
#line 373 "lang/parser.y"
                                                                           {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2721 "lang/y.tab.c"
    break;

  case 131: /* match_branches: match_branches '|' '_' ARROW expr  */
#line 374 "lang/parser.y"
                                                              {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[-4].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[0].ast_node_ptr));}
#line 2727 "lang/y.tab.c"
    break;

  case 132: /* fstring: FSTRING_START fstring_parts FSTRING_END  */
#line 377 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2733 "lang/y.tab.c"
    break;

  case 133: /* fstring_parts: %empty  */
#line 381 "lang/parser.y"
                                { (yyval.ast_node_ptr) = ast_empty_list(); }
#line 2739 "lang/y.tab.c"
    break;

  case 134: /* fstring_parts: fstring_parts fstring_part  */
#line 382 "lang/parser.y"
                                { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2745 "lang/y.tab.c"
    break;

  case 135: /* fstring_part: FSTRING_TEXT  */
#line 386 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_string((yyvsp[0].vstr)); }
#line 2751 "lang/y.tab.c"
    break;

  case 136: /* fstring_part: FSTRING_INTERP_START expr FSTRING_INTERP_END  */
#line 387 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2757 "lang/y.tab.c"
    break;

  case 137: /* type_decl: TYPE IDENTIFIER '=' type_expr  */
#line 391 "lang/parser.y"
                                  {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    SET_AST_LOC(type_decl, (yyloc));
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
#line 2768 "lang/y.tab.c"
    break;

  case 138: /* type_decl: TYPE IDENTIFIER  */
#line 398 "lang/parser.y"
                                 {
                                      Ast *type_decl = ast_let(ast_identifier((yyvsp[0].vident)), NULL, NULL);
                                      type_decl->tag = AST_TYPE_DECL;
                                      SET_AST_LOC(type_decl, (yyloc));
                                      (yyval.ast_node_ptr) = type_decl;
                                   }
#line 2779 "lang/y.tab.c"
    break;

  case 139: /* type_decl: TYPE type_args '=' type_expr  */
#line 405 "lang/parser.y"
                                 {
                                    Ast *args = (yyvsp[-2].ast_node_ptr);
                                    AstList *name_param = args->data.AST_LAMBDA.params;
                                    Ast *name = name_param->ast;
                                    args->data.AST_LAMBDA.params = name_param->next;
                                    if (args->data.AST_LAMBDA.type_annotations != NULL) {
                                      args->data.AST_LAMBDA.type_annotations =
                                          args->data.AST_LAMBDA.type_annotations->next;
                                    }
                                    args->data.AST_LAMBDA.len--;
                                    args->data.AST_LAMBDA.body = (yyvsp[0].ast_node_ptr);
                                    Ast *type_decl = ast_let(name, args, NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    SET_AST_LOC(type_decl, (yyloc));
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
#line 2800 "lang/y.tab.c"
    break;

  case 140: /* type_args: IDENTIFIER IDENTIFIER  */
#line 424 "lang/parser.y"
                                         { (yyval.ast_node_ptr) = ast_arg_list_push(ast_arg_list(ast_identifier((yyvsp[-1].vident)), NULL), ast_identifier((yyvsp[0].vident)), NULL); }
#line 2806 "lang/y.tab.c"
    break;

  case 141: /* type_args: type_args IDENTIFIER  */
#line 425 "lang/parser.y"
                                         { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), ast_identifier((yyvsp[0].vident)), NULL); }
#line 2812 "lang/y.tab.c"
    break;

  case 142: /* fn_signature: type_expr ARROW type_expr  */
#line 428 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2818 "lang/y.tab.c"
    break;

  case 143: /* fn_signature: fn_signature ARROW type_expr  */
#line 429 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2824 "lang/y.tab.c"
    break;

  case 144: /* tuple_type: type_expr_no_tuple ',' type_expr_no_tuple  */
#line 433 "lang/parser.y"
                                              { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2830 "lang/y.tab.c"
    break;

  case 145: /* tuple_type: tuple_type ',' type_expr_no_tuple  */
#line 434 "lang/parser.y"
                                             { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2836 "lang/y.tab.c"
    break;

  case 146: /* type_expr: type_expr_no_tuple  */
#line 438 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2842 "lang/y.tab.c"
    break;

  case 147: /* type_expr: tuple_type  */
#line 439 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2848 "lang/y.tab.c"
    break;

  case 148: /* type_expr_no_tuple: type_atom  */
#line 443 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2854 "lang/y.tab.c"
    break;

  case 149: /* type_expr_no_tuple: '|' type_atom  */
#line 444 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_list((yyvsp[0].ast_node_ptr)); }
#line 2860 "lang/y.tab.c"
    break;

  case 150: /* type_expr_no_tuple: type_expr_no_tuple '|' type_atom  */
#line 445 "lang/parser.y"
                                     { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2866 "lang/y.tab.c"
    break;

  case 151: /* type_expr_no_tuple: fn_signature  */
#line 446 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_fn_signature_of_list((yyvsp[0].ast_node_ptr)); }
#line 2872 "lang/y.tab.c"
    break;

  case 152: /* type_expr_no_tuple: type_atom TRIPLE_DOT  */
#line 447 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_variadic_expr((yyvsp[-1].ast_node_ptr)); }
#line 2878 "lang/y.tab.c"
    break;

  case 153: /* type_atom: IDENTIFIER  */
#line 451 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2884 "lang/y.tab.c"
    break;

  case 154: /* type_atom: IDENTIFIER '=' INTEGER  */
#line 452 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-2].vident)), AST_CONST(AST_INT, (yyvsp[0].vint)), NULL); }
#line 2890 "lang/y.tab.c"
    break;

  case 155: /* type_atom: IDENTIFIER OF type_atom  */
#line 453 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2896 "lang/y.tab.c"
    break;

  case 156: /* type_atom: IDENTIFIER ':' type_expr_no_tuple  */
#line 454 "lang/parser.y"
                                       { (yyval.ast_node_ptr) = ast_assoc(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2902 "lang/y.tab.c"
    break;

  case 157: /* type_atom: '(' type_expr ')'  */
#line 455 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2908 "lang/y.tab.c"
    break;

  case 158: /* type_atom: '(' type_expr_no_tuple ',' ')'  */
#line 456 "lang/parser.y"
                                   { (yyval.ast_node_ptr) = ast_tuple_type_single((yyvsp[-2].ast_node_ptr)); }
#line 2914 "lang/y.tab.c"
    break;

  case 159: /* type_atom: '(' tuple_type ',' ')'  */
#line 457 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[-2].ast_node_ptr); }
#line 2920 "lang/y.tab.c"
    break;

  case 160: /* type_atom: TOK_VOID  */
#line 458 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_void(); }
#line 2926 "lang/y.tab.c"
    break;

  case 161: /* type_atom: IDENTIFIER '.' IDENTIFIER  */
#line 459 "lang/parser.y"
                              {
                                Ast *record = ast_identifier((yyvsp[-2].vident));
                                Ast *member = ast_identifier((yyvsp[0].vident));
                                SET_AST_LOC(record, (yylsp[-2]));
                                SET_AST_LOC(member, (yylsp[0]));
                                (yyval.ast_node_ptr) = ast_record_access(record, member);
                                SET_AST_LOC((yyval.ast_node_ptr), (yyloc));
                              }
#line 2939 "lang/y.tab.c"
    break;


#line 2943 "lang/y.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
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

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 468 "lang/parser.y"



void yyerror(const char *s) {
  parse_record_error(s, yylineno, yycolumn, yyabsoluteoffset, yytext,
                     pctx.cur_script);
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, pctx.cur_script);
}
#endif _LANG_TAB_H
