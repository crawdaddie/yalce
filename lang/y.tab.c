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
  YYSYMBOL_DOUBLE = 4,                     /* DOUBLE  */
  YYSYMBOL_FLOAT = 5,                      /* FLOAT  */
  YYSYMBOL_IDENTIFIER = 6,                 /* IDENTIFIER  */
  YYSYMBOL_PATH_IDENTIFIER = 7,            /* PATH_IDENTIFIER  */
  YYSYMBOL_IDENTIFIER_LIST = 8,            /* IDENTIFIER_LIST  */
  YYSYMBOL_TOK_STRING = 9,                 /* TOK_STRING  */
  YYSYMBOL_TOK_CHAR = 10,                  /* TOK_CHAR  */
  YYSYMBOL_TRUE = 11,                      /* TRUE  */
  YYSYMBOL_FALSE = 12,                     /* FALSE  */
  YYSYMBOL_PIPE = 13,                      /* PIPE  */
  YYSYMBOL_EXTERN = 14,                    /* EXTERN  */
  YYSYMBOL_TRIPLE_DOT = 15,                /* TRIPLE_DOT  */
  YYSYMBOL_DOUBLE_DOT = 16,                /* DOUBLE_DOT  */
  YYSYMBOL_LET = 17,                       /* LET  */
  YYSYMBOL_FN = 18,                        /* FN  */
  YYSYMBOL_MODULE = 19,                    /* MODULE  */
  YYSYMBOL_MATCH = 20,                     /* MATCH  */
  YYSYMBOL_WITH = 21,                      /* WITH  */
  YYSYMBOL_ARROW = 22,                     /* ARROW  */
  YYSYMBOL_DOUBLE_COLON = 23,              /* DOUBLE_COLON  */
  YYSYMBOL_TOK_VOID = 24,                  /* TOK_VOID  */
  YYSYMBOL_IN = 25,                        /* IN  */
  YYSYMBOL_AND = 26,                       /* AND  */
  YYSYMBOL_ASYNC = 27,                     /* ASYNC  */
  YYSYMBOL_DOUBLE_AT = 28,                 /* DOUBLE_AT  */
  YYSYMBOL_AT = 29,                        /* AT  */
  YYSYMBOL_THUNK = 30,                     /* THUNK  */
  YYSYMBOL_IMPORT = 31,                    /* IMPORT  */
  YYSYMBOL_OPEN = 32,                      /* OPEN  */
  YYSYMBOL_IMPLEMENTS = 33,                /* IMPLEMENTS  */
  YYSYMBOL_AMPERSAND = 34,                 /* AMPERSAND  */
  YYSYMBOL_TYPE = 35,                      /* TYPE  */
  YYSYMBOL_TEST_ID = 36,                   /* TEST_ID  */
  YYSYMBOL_MUT = 37,                       /* MUT  */
  YYSYMBOL_THEN = 38,                      /* THEN  */
  YYSYMBOL_ELSE = 39,                      /* ELSE  */
  YYSYMBOL_YIELD = 40,                     /* YIELD  */
  YYSYMBOL_AWAIT = 41,                     /* AWAIT  */
  YYSYMBOL_FOR = 42,                       /* FOR  */
  YYSYMBOL_IF = 43,                        /* IF  */
  YYSYMBOL_OF = 44,                        /* OF  */
  YYSYMBOL_FSTRING_START = 45,             /* FSTRING_START  */
  YYSYMBOL_FSTRING_END = 46,               /* FSTRING_END  */
  YYSYMBOL_FSTRING_INTERP_START = 47,      /* FSTRING_INTERP_START  */
  YYSYMBOL_FSTRING_INTERP_END = 48,        /* FSTRING_INTERP_END  */
  YYSYMBOL_FSTRING_TEXT = 49,              /* FSTRING_TEXT  */
  YYSYMBOL_50_ = 50,                       /* '|'  */
  YYSYMBOL_MATCH_BODY_PREC = 51,           /* MATCH_BODY_PREC  */
  YYSYMBOL_DOUBLE_AMP = 52,                /* DOUBLE_AMP  */
  YYSYMBOL_DOUBLE_PIPE = 53,               /* DOUBLE_PIPE  */
  YYSYMBOL_GE = 54,                        /* GE  */
  YYSYMBOL_LE = 55,                        /* LE  */
  YYSYMBOL_EQ = 56,                        /* EQ  */
  YYSYMBOL_NE = 57,                        /* NE  */
  YYSYMBOL_58_ = 58,                       /* '>'  */
  YYSYMBOL_59_ = 59,                       /* '<'  */
  YYSYMBOL_60_ = 60,                       /* '+'  */
  YYSYMBOL_61_ = 61,                       /* '-'  */
  YYSYMBOL_62_ = 62,                       /* '*'  */
  YYSYMBOL_63_ = 63,                       /* '/'  */
  YYSYMBOL_MODULO = 64,                    /* MODULO  */
  YYSYMBOL_65_ = 65,                       /* ','  */
  YYSYMBOL_66_ = 66,                       /* ':'  */
  YYSYMBOL_APPLICATION = 67,               /* APPLICATION  */
  YYSYMBOL_68_ = 68,                       /* '.'  */
  YYSYMBOL_UMINUS = 69,                    /* UMINUS  */
  YYSYMBOL_70_ = 70,                       /* ';'  */
  YYSYMBOL_71_ = 71,                       /* '='  */
  YYSYMBOL_72_ = 72,                       /* '['  */
  YYSYMBOL_73_ = 73,                       /* ']'  */
  YYSYMBOL_74_ = 74,                       /* '('  */
  YYSYMBOL_75_ = 75,                       /* ')'  */
  YYSYMBOL_76___ = 76,                     /* '_'  */
  YYSYMBOL_YYACCEPT = 77,                  /* $accept  */
  YYSYMBOL_program = 78,                   /* program  */
  YYSYMBOL_expr = 79,                      /* expr  */
  YYSYMBOL_atom_expr = 80,                 /* atom_expr  */
  YYSYMBOL_simple_expr = 81,               /* simple_expr  */
  YYSYMBOL_expr_sequence = 82,             /* expr_sequence  */
  YYSYMBOL_let_binding = 83,               /* let_binding  */
  YYSYMBOL_lambda_expr = 84,               /* lambda_expr  */
  YYSYMBOL_lambda_args = 85,               /* lambda_args  */
  YYSYMBOL_lambda_arg = 86,                /* lambda_arg  */
  YYSYMBOL_list = 87,                      /* list  */
  YYSYMBOL_array = 88,                     /* array  */
  YYSYMBOL_tuple = 89,                     /* tuple  */
  YYSYMBOL_expr_list = 90,                 /* expr_list  */
  YYSYMBOL_match_expr = 91,                /* match_expr  */
  YYSYMBOL_match_test_clause = 92,         /* match_test_clause  */
  YYSYMBOL_match_branches = 93,            /* match_branches  */
  YYSYMBOL_fstring = 94,                   /* fstring  */
  YYSYMBOL_fstring_parts = 95,             /* fstring_parts  */
  YYSYMBOL_fstring_part = 96,              /* fstring_part  */
  YYSYMBOL_type_decl = 97,                 /* type_decl  */
  YYSYMBOL_type_args = 98,                 /* type_args  */
  YYSYMBOL_fn_signature = 99,              /* fn_signature  */
  YYSYMBOL_tuple_type = 100,               /* tuple_type  */
  YYSYMBOL_type_expr = 101,                /* type_expr  */
  YYSYMBOL_type_expr_no_tuple = 102,       /* type_expr_no_tuple  */
  YYSYMBOL_type_atom = 103                 /* type_atom  */
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
#define YYFINAL  93
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2152

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  77
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  155
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  333

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   314


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
      74,    75,    62,    60,    65,    61,    68,    63,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,    70,
      59,    71,    58,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    72,     2,    73,     2,    76,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    50,     2,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49,    51,    52,    53,    54,    55,
      56,    57,    64,    67,    69
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   134,   134,   135,   136,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   168,   169,   175,   176,   177,   178,   182,   183,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   205,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,   228,   229,   233,   234,   235,   238,
     240,   242,   251,   252,   258,   260,   268,   279,   280,   281,
     282,   283,   284,   290,   291,   292,   293,   300,   301,   302,
     303,   304,   305,   309,   310,   311,   312,   317,   318,   319,
     323,   324,   325,   330,   331,   332,   336,   337,   341,   342,
     343,   347,   348,   351,   352,   353,   356,   360,   361,   365,
     366,   370,   377,   384,   403,   404,   407,   408,   412,   413,
     417,   418,   422,   423,   424,   425,   426,   430,   431,   432,
     433,   434,   435,   436,   437,   438
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
  "\"end of file\"", "error", "\"invalid token\"", "INTEGER", "DOUBLE",
  "FLOAT", "IDENTIFIER", "PATH_IDENTIFIER", "IDENTIFIER_LIST",
  "TOK_STRING", "TOK_CHAR", "TRUE", "FALSE", "PIPE", "EXTERN",
  "TRIPLE_DOT", "DOUBLE_DOT", "LET", "FN", "MODULE", "MATCH", "WITH",
  "ARROW", "DOUBLE_COLON", "TOK_VOID", "IN", "AND", "ASYNC", "DOUBLE_AT",
  "AT", "THUNK", "IMPORT", "OPEN", "IMPLEMENTS", "AMPERSAND", "TYPE",
  "TEST_ID", "MUT", "THEN", "ELSE", "YIELD", "AWAIT", "FOR", "IF", "OF",
  "FSTRING_START", "FSTRING_END", "FSTRING_INTERP_START",
  "FSTRING_INTERP_END", "FSTRING_TEXT", "'|'", "MATCH_BODY_PREC",
  "DOUBLE_AMP", "DOUBLE_PIPE", "GE", "LE", "EQ", "NE", "'>'", "'<'", "'+'",
  "'-'", "'*'", "'/'", "MODULO", "','", "':'", "APPLICATION", "'.'",
  "UMINUS", "';'", "'='", "'['", "']'", "'('", "')'", "'_'", "$accept",
  "program", "expr", "atom_expr", "simple_expr", "expr_sequence",
  "let_binding", "lambda_expr", "lambda_args", "lambda_arg", "list",
  "array", "tuple", "expr_list", "match_expr", "match_test_clause",
  "match_branches", "fstring", "fstring_parts", "fstring_part",
  "type_decl", "type_args", "fn_signature", "tuple_type", "type_expr",
  "type_expr_no_tuple", "type_atom", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-220)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-141)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1887,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
     411,     3,     4,  1887,  -220,  1887,    47,    55,    20,  1887,
    1887,    36,  1887,  -220,  1512,   694,    45,  1342,     8,  -220,
      18,    65,  -220,  -220,  -220,  -220,  -220,  -220,  -220,   123,
      50,    73,  1887,   766,  -220,  1342,    81,    80,   145,   156,
    1887,    13,   -42,   166,    53,   838,  1342,  -220,  -220,  -220,
    -220,    10,    15,  1342,  1342,    98,   910,   -43,  1654,  -220,
     -40,   101,   140,   485,    67,   141,   165,   185,   189,   191,
     194,   196,   197,   198,   201,   213,   215,   216,   219,   221,
     575,    -1,   -28,  -220,  1887,  1887,  1887,  1887,  1887,  1887,
    1887,  1887,  1887,  1887,  1887,  1887,  1887,  1887,  1887,  1887,
    1887,  1703,  1512,     8,   225,  1887,  1887,    -4,   292,  1563,
    1887,  1887,   108,   224,   -15,  1887,  1887,  1887,  1887,    84,
    1887,   134,   226,  1887,  1887,  1887,   251,  -220,   174,  -220,
     174,  1887,  1887,  -220,  1887,  -220,  -220,   229,   -35,  1748,
    -220,  -220,  -220,   135,   281,   110,  -220,  -220,  -220,  -220,
    -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,  -220,
    -220,  -220,  1887,  -220,   648,  -220,  1414,  2078,   151,  1440,
     223,   223,  1930,  1930,  1930,  1930,  1930,  1930,  1960,  1960,
    2004,  2004,  2032,  1887,   151,   982,  -220,  1342,  1342,  -220,
     233,   287,   300,  1342,  1342,  1342,  1887,   236,   237,  1342,
    1342,  1342,   240,  -220,   241,   238,  1887,   174,  1342,   243,
     244,  1887,   265,   186,  -220,   171,   174,   294,   252,   296,
     -14,   306,   296,  1054,  1126,  1198,  -220,   249,  1793,  -220,
    1609,  1887,  1887,  -220,  1342,  1842,  -220,   118,   174,   118,
    1342,  1887,  1887,  1887,   174,  1342,   -10,  1887,  1887,  1270,
     301,   532,   171,   174,   318,   324,  -220,   263,    -8,    -7,
     174,   174,   174,   171,   174,  -220,  1887,  1887,  -220,  -220,
     256,   109,   112,   133,  -220,  2048,  -220,   294,   296,  -220,
    1342,  -220,     9,  -220,  1887,  1887,   325,   328,  -220,   331,
    -220,  -220,   147,  -220,   187,   296,  -220,   296,  -220,  -220,
    1342,  1342,  -220,   332,   116,  -220,  -220,  -220,  -220,  1342,
    1342,  1887,  1887,  -220,  -220,  1887,  1887,  1342,  1342,   137,
     183,  -220,  -220
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    39,    40,    41,    45,    31,    42,    51,    43,    44,
       0,     0,     0,     0,    46,     0,     0,     0,     0,     0,
       0,     0,     0,   127,     0,     0,     0,    74,     5,    37,
       3,    27,    84,    47,    48,    49,    28,    50,    29,    45,
      46,     0,     0,     0,   106,   116,     0,     0,   103,     0,
       0,     0,    97,     0,     0,     0,    30,    89,    87,    90,
      88,   132,     0,     6,     7,     0,     0,     0,     0,   107,
       0,    45,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      74,     0,     0,     1,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     9,     0,     2,     0,     0,     0,     0,
       0,     0,     0,    45,     0,     0,     0,     0,     0,     0,
       0,   100,     0,     0,     0,     0,     0,   134,     0,   135,
       0,     0,     0,   126,     0,   129,   128,     0,     0,     0,
     108,    73,    70,    45,     0,     0,    72,    64,    65,    66,
      67,    69,    68,    63,    62,    57,    58,    59,    60,    61,
      71,   113,     0,    52,     0,   114,    23,    25,    26,     8,
      17,    18,    19,    20,    22,    21,    16,    15,    10,    11,
      12,    13,    14,     0,    24,   116,    38,    75,    83,   105,
       0,     0,     0,    77,    82,    76,     0,    73,   114,    79,
     117,    80,     0,   104,     0,     0,     0,     0,    98,     0,
       0,     0,   118,   147,   154,     0,     0,   145,   141,   131,
     140,   142,   133,     0,   120,     0,   110,     0,     0,   109,
       0,     0,     0,   115,    36,     0,    33,     0,     0,     0,
      81,     0,    94,    93,     0,   101,     0,    96,    95,   121,
       0,     0,     0,     0,     0,     0,   143,   141,     0,   140,
       0,     0,     0,     0,     0,   146,     0,     0,   130,   111,
       0,     0,     0,     0,    34,    25,    91,    78,     0,    92,
      86,    84,     0,    99,     0,     0,     0,     0,   149,   150,
     155,   148,     0,   151,     0,   137,   139,   136,   144,   138,
      32,   119,   112,     0,     0,    54,    53,    35,   102,   122,
     123,     0,     0,   153,   152,     0,     0,   125,   124,     0,
       0,    56,    55
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -220,  -220,    -2,   154,  -220,     0,  -220,    16,   -11,    -5,
    -220,  -220,  -220,    14,  -220,    94,  -220,  -220,  -220,  -220,
    -220,  -220,   113,   130,  -106,  -219,   -60
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    26,    27,    28,    29,    91,    31,    32,    51,    52,
      33,    34,    35,    47,    36,   260,   222,    37,    67,   146,
      38,    62,   227,   228,   288,   230,   231
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      30,    54,    48,   143,   144,    46,   145,   269,    45,    48,
      48,    55,   272,    56,   272,   237,   137,    63,    64,    48,
      66,   139,    45,    90,   132,   149,    61,    49,    53,   133,
     238,   272,   229,   150,   232,   130,   273,   174,    70,    92,
      45,    90,    65,   273,   299,    93,   131,   175,    45,   131,
     174,   274,   306,    57,    58,   309,   122,   124,   304,    48,
     208,    59,    60,   155,   129,   293,    45,   303,    46,   172,
      50,    45,    44,    48,   173,   135,   114,    50,    50,    44,
      44,   138,   148,   306,   318,   309,   140,    50,   115,    44,
     116,   154,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   194,
     195,   256,   199,   197,   198,    48,    48,   203,   204,   205,
     268,   120,    48,   209,   210,   211,    70,    50,   212,    44,
     214,   218,   242,   313,   219,   220,    11,    12,   326,   233,
     234,    50,   235,    44,   121,   126,   117,   210,   292,   126,
     131,   127,   125,   223,     1,     2,     3,     4,   117,   213,
       6,     7,     8,     9,   305,   266,   307,    95,   117,   141,
     197,   224,   210,   126,    96,    14,   151,   223,   128,   206,
     223,   113,   252,    50,    50,    44,    44,   315,   134,   118,
      50,   244,    44,   223,   119,   224,    23,   225,   224,   113,
     215,   118,   298,   253,   250,   216,   240,   252,   316,   113,
     113,   224,   331,   308,   255,   152,   156,   113,   113,   259,
     113,   226,   323,   112,   225,    25,     1,     2,     3,     4,
     262,   196,     6,     7,     8,     9,   210,   225,   203,    95,
     157,   282,   283,   285,   113,   226,    96,    14,   226,   290,
     197,   197,   263,   253,   264,   197,   197,   265,   332,   259,
     158,   226,   324,   286,   159,   289,   160,   291,    23,   161,
     314,   162,   163,   164,   310,   311,   165,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   166,   111,
     167,   168,   319,   320,   169,   112,   170,    25,   200,   207,
     217,   221,   236,   241,   247,   248,   249,   251,  -104,   131,
     252,   253,   254,   257,   258,   261,   270,   271,   272,   327,
     328,   275,   279,   295,   300,   329,   330,   301,   302,   312,
     113,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   113,   113,   321,   113,   113,
     322,   113,   113,  -140,   325,   297,   267,   113,   113,   113,
       0,   287,     0,   113,   113,   113,     0,     0,     0,     0,
       0,     0,   113,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   113,   113,   113,
       0,     0,     0,     0,     0,     0,     0,     0,   113,     0,
       0,     0,     0,     0,   113,     0,     0,     0,     0,   113,
       0,     0,     0,   113,     1,     2,     3,    39,     0,     5,
       6,     7,     8,     9,     0,     0,     0,     0,    10,    11,
      12,    13,     0,     0,     0,    40,     0,     0,     0,   113,
       0,    15,    16,    17,   113,     0,    18,    41,    42,     0,
       0,    19,    20,    21,    22,     0,    23,     0,     0,     0,
       0,     0,     0,     0,   113,   113,     0,     0,     0,     0,
       0,     0,     0,   113,   113,     0,     0,     0,     0,     0,
       0,   113,   113,    24,     0,    43,     0,    44,     1,     2,
       3,   153,     0,     5,     6,     7,     8,     9,     0,     0,
       0,     0,    10,    11,    12,    13,     0,     0,     0,    40,
       0,     0,     0,     0,     0,    15,    16,    17,     0,     0,
      18,    41,    42,     0,     0,    19,    20,    21,    22,     0,
      23,     0,     0,     0,     0,     1,     2,     3,     4,     0,
       5,     6,     7,     8,     9,     0,     0,     0,     0,    10,
      11,    12,    13,     0,     0,     0,    14,    24,     0,    43,
       0,    44,    15,    16,    17,     0,     0,    18,     0,     0,
       0,     0,    19,    20,    21,    22,     0,    23,     1,     2,
       3,     4,     0,     0,     6,     7,     8,     9,    94,     0,
       0,    95,     0,     0,     0,     0,     0,     0,    96,    14,
       0,     0,     0,    97,    24,     0,    25,     0,   296,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      23,     0,     0,     0,     0,     0,     0,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
    -116,   111,     0,     0,     0,     0,     0,   112,     0,    25,
     171,     1,     2,     3,     4,     0,     5,     6,     7,     8,
       9,     0,     0,     0,     0,    10,    11,    12,    13,     0,
       0,     0,    14,     0,     0,     0,     0,     0,    15,    16,
      17,     0,     0,    18,     0,     0,     0,     0,    19,    20,
      21,    22,     0,    23,     0,     0,     0,     1,     2,     3,
      71,     0,     5,     6,     7,     8,     9,    72,     0,     0,
       0,    73,    74,    12,    13,     0,     0,    75,    14,     0,
      24,     0,    25,   243,    15,    16,    17,     0,     0,    18,
       0,     0,     0,     0,    19,    20,    21,    22,     0,    23,
       0,     0,     0,     0,     0,     0,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,     0,
      89,     0,     0,     0,     0,     0,    24,     0,    25,     1,
       2,     3,   123,     0,     5,     6,     7,     8,     9,    72,
       0,     0,     0,    73,    74,    12,    13,     0,     0,    75,
      14,     0,     0,     0,     0,     0,    15,    16,    17,     0,
       0,    18,     0,     0,     0,     0,    19,    20,    21,    22,
       0,    23,     0,     0,     0,     0,     0,     0,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,     0,    89,     0,     0,     0,     0,     0,    24,     0,
      25,     1,     2,     3,     4,     0,     0,     6,     7,     8,
       9,    94,     0,     0,    95,     0,     0,     0,     0,   136,
       0,    96,    14,     0,     0,     0,    97,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    23,     0,     0,     0,     0,     0,     0,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,     0,   111,     0,     0,     0,     0,     0,
     112,     0,    25,     1,     2,     3,     4,     0,     0,     6,
       7,     8,     9,    94,     0,     0,    95,     0,     0,     0,
       0,     0,     0,    96,    14,     0,     0,     0,    97,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   142,     0,
       0,     0,     0,     0,     0,    23,     0,     0,     0,     0,
       0,     0,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,     0,   111,     0,     0,     0,
       0,     0,   112,     0,    25,     1,     2,     3,     4,     0,
       0,     6,     7,     8,     9,    94,     0,     0,   245,     0,
       0,     0,     0,     0,     0,    96,    14,     0,     0,     0,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    23,     0,     0,
       0,     0,     0,     0,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,     0,   111,     0,
       0,     0,     0,     0,   112,   246,    25,     1,     2,     3,
       4,     0,     0,     6,     7,     8,     9,    94,     0,     0,
      95,     0,     0,     0,     0,     0,     0,    96,    14,   276,
       0,     0,    97,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    23,
       0,     0,     0,     0,     0,     0,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,     0,
     111,     0,     0,     0,     0,     0,   112,     0,    25,     1,
       2,     3,     4,     0,     0,     6,     7,     8,     9,    94,
       0,     0,    95,     0,     0,     0,     0,     0,     0,    96,
      14,     0,     0,     0,    97,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   277,     0,     0,     0,     0,
       0,    23,     0,     0,     0,     0,     0,     0,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,     0,   111,     0,     0,     0,     0,     0,   112,     0,
      25,     1,     2,     3,     4,     0,     0,     6,     7,     8,
       9,    94,     0,     0,    95,     0,     0,     0,     0,     0,
       0,    96,    14,     0,     0,     0,    97,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    23,     0,     0,   278,     0,     0,     0,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,     0,   111,     0,     0,     0,     0,     0,
     112,     0,    25,     1,     2,     3,     4,     0,     0,     6,
       7,     8,     9,    94,     0,     0,    95,     0,     0,     0,
       0,     0,     0,    96,    14,     0,     0,     0,    97,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   294,     0,    23,     0,     0,     0,     0,
       0,     0,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,     0,   111,     0,     0,     0,
       0,     0,   112,     0,    25,     1,     2,     3,     4,     0,
       0,     6,     7,     8,     9,    94,     0,     0,    95,     0,
       0,     0,     0,     0,     0,    96,    14,     0,     0,     0,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    23,     0,     0,
       0,     0,     0,     0,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,     0,   111,     0,
       0,     0,     0,     0,   112,     0,    25,     1,     2,     3,
       4,     0,     0,     6,     7,     8,     9,     0,     0,     0,
      95,     0,     0,     0,     0,     0,     0,    96,    14,     0,
       0,     0,    97,     1,     2,     3,     4,     0,     0,     6,
       7,     8,     9,     0,     0,     0,    95,     0,     0,    23,
       0,     0,     0,    96,    14,     0,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,     0,
     111,     0,     0,     0,     0,    23,   112,     0,    25,     0,
       0,     0,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,     0,   111,     0,     0,     0,
       0,     0,   112,     0,    25,     1,     2,     3,     4,     0,
       5,     6,     7,     8,     9,     0,     0,     0,     0,    10,
      11,    12,    13,     0,     0,     0,    14,     0,     0,     0,
       0,     0,    15,    16,    17,     0,     0,    18,     0,     0,
       0,     0,    19,    20,    21,    22,     0,    23,     0,     0,
       0,     0,    68,     0,     0,     0,     1,     2,     3,     4,
       0,     5,     6,     7,     8,     9,     0,   201,     0,     0,
      10,    11,    12,    13,    24,    69,    25,    14,     0,     0,
       0,     0,   202,    15,    16,    17,     0,     0,    18,     0,
       0,     0,     0,    19,    20,    21,    22,     0,    23,     0,
       0,     0,     1,     2,     3,     4,     0,     5,     6,     7,
       8,     9,     0,   201,     0,     0,    10,   281,    12,    13,
       0,     0,     0,    14,     0,    24,     0,    25,   202,    15,
      16,    17,     0,     0,    18,     0,     0,     0,     0,    19,
      20,    21,    22,     0,    23,     0,     0,     1,     2,     3,
       4,     0,     5,     6,     7,     8,     9,     0,     0,     0,
       0,    10,    11,    12,    13,     0,     0,     0,    14,     0,
       0,    24,     0,    25,    15,    16,    17,     0,     0,    18,
       0,     0,     0,     0,    19,    20,    21,    22,     0,    23,
       0,     0,     0,     0,   147,     0,     1,     2,     3,     4,
       0,     5,     6,     7,     8,     9,     0,     0,     0,     0,
      10,    11,    12,    13,     0,     0,    24,    14,    25,     0,
       0,     0,     0,    15,    16,    17,     0,     0,    18,     0,
       0,     0,     0,    19,    20,    21,    22,     0,    23,     0,
       0,     1,     2,     3,     4,     0,     5,     6,     7,     8,
       9,     0,     0,     0,     0,    10,    11,    12,    13,     0,
       0,     0,    14,     0,   193,    24,     0,    25,    15,    16,
      17,     0,     0,    18,     0,     0,     0,     0,    19,    20,
      21,    22,     0,    23,     0,     0,     1,     2,     3,     4,
       0,     5,     6,     7,     8,     9,     0,     0,     0,     0,
      10,    11,    12,    13,     0,     0,     0,    14,     0,     0,
      24,   239,    25,    15,    16,    17,     0,     0,    18,     0,
       0,     0,     0,    19,    20,    21,    22,     0,    23,     0,
       0,     0,     0,   280,     0,     1,     2,     3,     4,     0,
       5,     6,     7,     8,     9,     0,     0,     0,     0,    10,
      11,    12,    13,     0,     0,    24,    14,    25,     0,     0,
       0,     0,    15,    16,    17,     0,     0,    18,     0,     0,
       0,     0,    19,    20,    21,    22,     0,    23,     0,     0,
       1,     2,     3,     4,     0,     5,     6,     7,     8,     9,
       0,     0,     0,     0,    10,    11,    12,    13,     0,     0,
       0,    14,     0,     0,    24,   284,    25,    15,    16,    17,
       0,     0,    18,     0,     0,     0,     0,    19,    20,    21,
      22,     0,    23,     1,     2,     3,     4,     0,     0,     6,
       7,     8,     9,     0,     0,     0,    95,     0,     0,     0,
       0,     0,     0,    96,    14,     0,     0,     0,     0,    24,
       0,    25,     0,     1,     2,     3,     4,     0,     0,     6,
       7,     8,     9,     0,     0,    23,    95,     0,     0,     0,
       0,     0,     0,    96,    14,     0,     0,     0,     0,     0,
     106,   107,   108,   109,   110,     0,   111,     0,     0,     0,
       0,     0,   112,     0,    25,    23,     0,     1,     2,     3,
       4,     0,     0,     6,     7,     8,     9,     0,     0,     0,
      95,     0,   108,   109,   110,     0,   111,    96,    14,     0,
       0,     0,   112,     0,    25,     1,     2,     3,     4,     0,
       0,     6,     7,     8,     9,     0,     0,     0,    95,    23,
       0,     1,     2,     3,     4,    96,    14,     6,     7,     8,
       9,     0,     0,     0,     0,     0,     0,     0,   110,     0,
     111,     0,    14,     0,     0,     0,   112,    23,    25,     0,
       0,     1,     2,     3,     4,     0,     0,     6,     7,     8,
       9,     0,     0,    23,     0,     0,     0,     0,   111,     0,
       0,     0,    14,     0,   112,     0,    25,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     112,   317,    25,    23,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     112,     0,    25
};

static const yytype_int16 yycheck[] =
{
       0,    12,     6,    46,    47,    10,    49,   226,    10,     6,
       6,    13,    22,    15,    22,    50,     6,    19,    20,     6,
      22,     6,    24,    25,    66,    65,     6,    24,    24,    71,
      65,    22,   138,    73,   140,    22,    50,    65,    24,    25,
      42,    43,     6,    50,   263,     0,    51,    75,    50,    54,
      65,    65,   271,     6,     7,   274,    42,    43,    65,     6,
      75,     6,     7,    74,    50,    75,    68,    75,    73,    70,
      74,    73,    76,     6,    75,    22,    68,    74,    74,    76,
      76,    71,    68,   302,    75,   304,    71,    74,    70,    76,
      25,    24,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   217,   117,   115,   116,     6,     6,   119,   120,   121,
     226,    71,     6,   125,   126,   127,   112,    74,   128,    76,
     130,   133,    22,    24,   134,   135,    18,    19,    22,   141,
     142,    74,   144,    76,    71,    65,    23,   149,   254,    65,
     155,    71,    71,     6,     3,     4,     5,     6,    23,    75,
       9,    10,    11,    12,   270,   225,   272,    16,    23,    71,
     172,    24,   174,    65,    23,    24,    75,     6,    22,    71,
       6,    27,    70,    74,    74,    76,    76,    75,    22,    66,
      74,   193,    76,     6,    71,    24,    45,    50,    24,    45,
      66,    66,   262,    70,   206,    71,    71,    70,    75,    55,
      56,    24,    75,   273,   216,    75,    75,    63,    64,   221,
      66,    74,    75,    72,    50,    74,     3,     4,     5,     6,
      44,     6,     9,    10,    11,    12,   238,    50,   240,    16,
      75,   241,   242,   245,    90,    74,    23,    24,    74,   251,
     252,   253,    66,    70,    68,   257,   258,    71,    75,   261,
      75,    74,    75,   247,    75,   249,    75,   251,    45,    75,
     281,    75,    75,    75,   276,   277,    75,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    75,    66,
      75,    75,   294,   295,    75,    72,    75,    74,     6,    75,
      74,    50,    73,    22,    71,    18,     6,    71,    71,   314,
      70,    70,    74,    70,    70,    50,    22,    65,    22,   321,
     322,    15,    73,    22,     6,   325,   326,     3,    65,    73,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   192,    22,   194,   195,
      22,   197,   198,    22,    22,   261,   226,   203,   204,   205,
      -1,   248,    -1,   209,   210,   211,    -1,    -1,    -1,    -1,
      -1,    -1,   218,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   233,   234,   235,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   244,    -1,
      -1,    -1,    -1,    -1,   250,    -1,    -1,    -1,    -1,   255,
      -1,    -1,    -1,   259,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    -1,    -1,    17,    18,
      19,    20,    -1,    -1,    -1,    24,    -1,    -1,    -1,   285,
      -1,    30,    31,    32,   290,    -1,    35,    36,    37,    -1,
      -1,    40,    41,    42,    43,    -1,    45,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   310,   311,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   319,   320,    -1,    -1,    -1,    -1,    -1,
      -1,   327,   328,    72,    -1,    74,    -1,    76,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    -1,
      -1,    -1,    17,    18,    19,    20,    -1,    -1,    -1,    24,
      -1,    -1,    -1,    -1,    -1,    30,    31,    32,    -1,    -1,
      35,    36,    37,    -1,    -1,    40,    41,    42,    43,    -1,
      45,    -1,    -1,    -1,    -1,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    -1,    -1,    -1,    24,    72,    -1,    74,
      -1,    76,    30,    31,    32,    -1,    -1,    35,    -1,    -1,
      -1,    -1,    40,    41,    42,    43,    -1,    45,     3,     4,
       5,     6,    -1,    -1,     9,    10,    11,    12,    13,    -1,
      -1,    16,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,
      -1,    -1,    -1,    28,    72,    -1,    74,    -1,    76,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      45,    -1,    -1,    -1,    -1,    -1,    -1,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    -1,    -1,    -1,    -1,    -1,    72,    -1,    74,
      75,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    -1,
      -1,    -1,    24,    -1,    -1,    -1,    -1,    -1,    30,    31,
      32,    -1,    -1,    35,    -1,    -1,    -1,    -1,    40,    41,
      42,    43,    -1,    45,    -1,    -1,    -1,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    17,    18,    19,    20,    -1,    -1,    23,    24,    -1,
      72,    -1,    74,    75,    30,    31,    32,    -1,    -1,    35,
      -1,    -1,    -1,    -1,    40,    41,    42,    43,    -1,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    -1,
      66,    -1,    -1,    -1,    -1,    -1,    72,    -1,    74,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    17,    18,    19,    20,    -1,    -1,    23,
      24,    -1,    -1,    -1,    -1,    -1,    30,    31,    32,    -1,
      -1,    35,    -1,    -1,    -1,    -1,    40,    41,    42,    43,
      -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    -1,    66,    -1,    -1,    -1,    -1,    -1,    72,    -1,
      74,     3,     4,     5,     6,    -1,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    16,    -1,    -1,    -1,    -1,    21,
      -1,    23,    24,    -1,    -1,    -1,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    -1,    66,    -1,    -1,    -1,    -1,    -1,
      72,    -1,    74,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    16,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    24,    -1,    -1,    -1,    28,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    -1,    66,    -1,    -1,    -1,
      -1,    -1,    72,    -1,    74,     3,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    16,    -1,
      -1,    -1,    -1,    -1,    -1,    23,    24,    -1,    -1,    -1,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    -1,    66,    -1,
      -1,    -1,    -1,    -1,    72,    73,    74,     3,     4,     5,
       6,    -1,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      16,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    25,
      -1,    -1,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    -1,    -1,    -1,    -1,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    -1,
      66,    -1,    -1,    -1,    -1,    -1,    72,    -1,    74,     3,
       4,     5,     6,    -1,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    16,    -1,    -1,    -1,    -1,    -1,    -1,    23,
      24,    -1,    -1,    -1,    28,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    -1,    66,    -1,    -1,    -1,    -1,    -1,    72,    -1,
      74,     3,     4,     5,     6,    -1,    -1,     9,    10,    11,
      12,    13,    -1,    -1,    16,    -1,    -1,    -1,    -1,    -1,
      -1,    23,    24,    -1,    -1,    -1,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    -1,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    -1,    66,    -1,    -1,    -1,    -1,    -1,
      72,    -1,    74,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    13,    -1,    -1,    16,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    24,    -1,    -1,    -1,    28,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    43,    -1,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    -1,    66,    -1,    -1,    -1,
      -1,    -1,    72,    -1,    74,     3,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    16,    -1,
      -1,    -1,    -1,    -1,    -1,    23,    24,    -1,    -1,    -1,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    -1,    66,    -1,
      -1,    -1,    -1,    -1,    72,    -1,    74,     3,     4,     5,
       6,    -1,    -1,     9,    10,    11,    12,    -1,    -1,    -1,
      16,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    -1,
      -1,    -1,    28,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    -1,    -1,    -1,    16,    -1,    -1,    45,
      -1,    -1,    -1,    23,    24,    -1,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    -1,
      66,    -1,    -1,    -1,    -1,    45,    72,    -1,    74,    -1,
      -1,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    -1,    66,    -1,    -1,    -1,
      -1,    -1,    72,    -1,    74,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    -1,    -1,    -1,    24,    -1,    -1,    -1,
      -1,    -1,    30,    31,    32,    -1,    -1,    35,    -1,    -1,
      -1,    -1,    40,    41,    42,    43,    -1,    45,    -1,    -1,
      -1,    -1,    50,    -1,    -1,    -1,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    14,    -1,    -1,
      17,    18,    19,    20,    72,    73,    74,    24,    -1,    -1,
      -1,    -1,    29,    30,    31,    32,    -1,    -1,    35,    -1,
      -1,    -1,    -1,    40,    41,    42,    43,    -1,    45,    -1,
      -1,    -1,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    14,    -1,    -1,    17,    18,    19,    20,
      -1,    -1,    -1,    24,    -1,    72,    -1,    74,    29,    30,
      31,    32,    -1,    -1,    35,    -1,    -1,    -1,    -1,    40,
      41,    42,    43,    -1,    45,    -1,    -1,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    -1,    -1,
      -1,    17,    18,    19,    20,    -1,    -1,    -1,    24,    -1,
      -1,    72,    -1,    74,    30,    31,    32,    -1,    -1,    35,
      -1,    -1,    -1,    -1,    40,    41,    42,    43,    -1,    45,
      -1,    -1,    -1,    -1,    50,    -1,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,
      17,    18,    19,    20,    -1,    -1,    72,    24,    74,    -1,
      -1,    -1,    -1,    30,    31,    32,    -1,    -1,    35,    -1,
      -1,    -1,    -1,    40,    41,    42,    43,    -1,    45,    -1,
      -1,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    -1,    -1,    -1,    -1,    17,    18,    19,    20,    -1,
      -1,    -1,    24,    -1,    71,    72,    -1,    74,    30,    31,
      32,    -1,    -1,    35,    -1,    -1,    -1,    -1,    40,    41,
      42,    43,    -1,    45,    -1,    -1,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,
      17,    18,    19,    20,    -1,    -1,    -1,    24,    -1,    -1,
      72,    73,    74,    30,    31,    32,    -1,    -1,    35,    -1,
      -1,    -1,    -1,    40,    41,    42,    43,    -1,    45,    -1,
      -1,    -1,    -1,    50,    -1,     3,     4,     5,     6,    -1,
       8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,    17,
      18,    19,    20,    -1,    -1,    72,    24,    74,    -1,    -1,
      -1,    -1,    30,    31,    32,    -1,    -1,    35,    -1,    -1,
      -1,    -1,    40,    41,    42,    43,    -1,    45,    -1,    -1,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    -1,    -1,    -1,    17,    18,    19,    20,    -1,    -1,
      -1,    24,    -1,    -1,    72,    73,    74,    30,    31,    32,
      -1,    -1,    35,    -1,    -1,    -1,    -1,    40,    41,    42,
      43,    -1,    45,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    -1,    -1,    -1,    16,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    24,    -1,    -1,    -1,    -1,    72,
      -1,    74,    -1,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    -1,    -1,    45,    16,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    24,    -1,    -1,    -1,    -1,    -1,
      60,    61,    62,    63,    64,    -1,    66,    -1,    -1,    -1,
      -1,    -1,    72,    -1,    74,    45,    -1,     3,     4,     5,
       6,    -1,    -1,     9,    10,    11,    12,    -1,    -1,    -1,
      16,    -1,    62,    63,    64,    -1,    66,    23,    24,    -1,
      -1,    -1,    72,    -1,    74,     3,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    -1,    -1,    -1,    16,    45,
      -1,     3,     4,     5,     6,    23,    24,     9,    10,    11,
      12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,
      66,    -1,    24,    -1,    -1,    -1,    72,    45,    74,    -1,
      -1,     3,     4,     5,     6,    -1,    -1,     9,    10,    11,
      12,    -1,    -1,    45,    -1,    -1,    -1,    -1,    66,    -1,
      -1,    -1,    24,    -1,    72,    -1,    74,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      72,    73,    74,    45,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      72,    -1,    74
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     8,     9,    10,    11,    12,
      17,    18,    19,    20,    24,    30,    31,    32,    35,    40,
      41,    42,    43,    45,    72,    74,    78,    79,    80,    81,
      82,    83,    84,    87,    88,    89,    91,    94,    97,     6,
      24,    36,    37,    74,    76,    79,    86,    90,     6,    24,
      74,    85,    86,    24,    85,    79,    79,     6,     7,     6,
       7,     6,    98,    79,    79,     6,    79,    95,    50,    73,
      90,     6,    13,    17,    18,    23,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    66,
      79,    82,    90,     0,    13,    16,    23,    28,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    66,    72,    80,    68,    70,    25,    23,    66,    71,
      71,    71,    90,     6,    90,    71,    65,    71,    22,    90,
      22,    86,    66,    71,    22,    22,    21,     6,    71,     6,
      71,    71,    38,    46,    47,    49,    96,    50,    90,    65,
      73,    75,    75,     6,    24,    85,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    70,    75,    65,    75,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      79,    79,    79,    71,    79,    79,     6,    79,    79,    86,
       6,    14,    29,    79,    79,    79,    71,    75,    75,    79,
      79,    79,    82,    75,    82,    66,    71,    74,    79,    82,
      82,    50,    93,     6,    24,    50,    74,    99,   100,   101,
     102,   103,   101,    79,    79,    79,    73,    50,    65,    73,
      71,    22,    22,    75,    79,    16,    73,    71,    18,     6,
      79,    71,    70,    70,    74,    79,   101,    70,    70,    79,
      92,    50,    44,    66,    68,    71,   103,   100,   101,   102,
      22,    65,    22,    50,    65,    15,    25,    39,    48,    73,
      50,    18,    82,    82,    73,    79,    84,    99,   101,    84,
      79,    84,   101,    75,    43,    22,    76,    92,   103,   102,
       6,     3,    65,    75,    65,   101,   102,   101,   103,   102,
      79,    79,    73,    24,    85,    75,    75,    73,    75,    79,
      79,    22,    22,    75,    75,    22,    22,    79,    79,    82,
      82,    75,    75
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    77,    78,    78,    78,    79,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    79,    80,    80,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    82,    82,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    84,    84,    84,    84,    85,    85,    85,
      85,    85,    85,    86,    86,    86,    86,    87,    87,    87,
      88,    88,    88,    89,    89,    89,    90,    90,    91,    91,
      91,    92,    92,    93,    93,    93,    94,    95,    95,    96,
      96,    97,    97,    97,    98,    98,    99,    99,   100,   100,
     101,   101,   102,   102,   102,   102,   102,   103,   103,   103,
     103,   103,   103,   103,   103,   103
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     2,     2,     3,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     1,     1,
       2,     1,     6,     4,     5,     6,     4,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     6,     6,     9,     9,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     1,     3,     4,     4,     6,     4,
       4,     5,     4,     3,     1,     6,     6,     2,     2,     2,
       2,     6,     6,     5,     5,     5,     5,     1,     3,     5,
       2,     4,     6,     1,     3,     3,     1,     2,     3,     4,
       4,     5,     6,     3,     3,     4,     1,     3,     4,     6,
       4,     1,     3,     4,     5,     5,     3,     0,     2,     1,
       3,     4,     2,     4,     2,     2,     3,     3,     3,     3,
       1,     1,     1,     2,     3,     1,     2,     1,     3,     3,
       3,     3,     4,     4,     1,     3
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
#line 134 "lang/parser.y"
                      { pctx.ast_root = parse_stmt_list(pctx.ast_root, (yyvsp[-1].ast_node_ptr)); }
#line 1894 "lang/y.tab.c"
    break;

  case 3: /* program: expr_sequence  */
#line 135 "lang/parser.y"
                      { pctx.ast_root = parse_stmt_list(pctx.ast_root, (yyvsp[0].ast_node_ptr)); }
#line 1900 "lang/y.tab.c"
    break;

  case 6: /* expr: YIELD expr  */
#line 142 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_yield((yyvsp[0].ast_node_ptr)); }
#line 1906 "lang/y.tab.c"
    break;

  case 7: /* expr: AWAIT expr  */
#line 143 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_await((yyvsp[0].ast_node_ptr)); }
#line 1912 "lang/y.tab.c"
    break;

  case 8: /* expr: expr DOUBLE_AT expr  */
#line 144 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1918 "lang/y.tab.c"
    break;

  case 9: /* expr: expr atom_expr  */
#line 145 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1924 "lang/y.tab.c"
    break;

  case 10: /* expr: expr '+' expr  */
#line 146 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1930 "lang/y.tab.c"
    break;

  case 11: /* expr: expr '-' expr  */
#line 147 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1936 "lang/y.tab.c"
    break;

  case 12: /* expr: expr '*' expr  */
#line 148 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1942 "lang/y.tab.c"
    break;

  case 13: /* expr: expr '/' expr  */
#line 149 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1948 "lang/y.tab.c"
    break;

  case 14: /* expr: expr MODULO expr  */
#line 150 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1954 "lang/y.tab.c"
    break;

  case 15: /* expr: expr '<' expr  */
#line 151 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1960 "lang/y.tab.c"
    break;

  case 16: /* expr: expr '>' expr  */
#line 152 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1966 "lang/y.tab.c"
    break;

  case 17: /* expr: expr DOUBLE_AMP expr  */
#line 153 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_AMP, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1972 "lang/y.tab.c"
    break;

  case 18: /* expr: expr DOUBLE_PIPE expr  */
#line 154 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_PIPE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1978 "lang/y.tab.c"
    break;

  case 19: /* expr: expr GE expr  */
#line 155 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1984 "lang/y.tab.c"
    break;

  case 20: /* expr: expr LE expr  */
#line 156 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1990 "lang/y.tab.c"
    break;

  case 21: /* expr: expr NE expr  */
#line 157 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1996 "lang/y.tab.c"
    break;

  case 22: /* expr: expr EQ expr  */
#line 158 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2002 "lang/y.tab.c"
    break;

  case 23: /* expr: expr PIPE expr  */
#line 159 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[0].ast_node_ptr), (yyvsp[-2].ast_node_ptr)); }
#line 2008 "lang/y.tab.c"
    break;

  case 24: /* expr: expr ':' expr  */
#line 160 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_assoc((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2014 "lang/y.tab.c"
    break;

  case 25: /* expr: expr DOUBLE_DOT expr  */
#line 161 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_range_expression((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2020 "lang/y.tab.c"
    break;

  case 26: /* expr: expr DOUBLE_COLON expr  */
#line 162 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2026 "lang/y.tab.c"
    break;

  case 27: /* expr: let_binding  */
#line 163 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2032 "lang/y.tab.c"
    break;

  case 28: /* expr: match_expr  */
#line 164 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2038 "lang/y.tab.c"
    break;

  case 29: /* expr: type_decl  */
#line 165 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2044 "lang/y.tab.c"
    break;

  case 30: /* expr: THUNK expr  */
#line 166 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_thunk_expr((yyvsp[0].ast_node_ptr)); }
#line 2050 "lang/y.tab.c"
    break;

  case 31: /* expr: IDENTIFIER_LIST  */
#line 168 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_typed_empty_list((yyvsp[0].vident)); }
#line 2056 "lang/y.tab.c"
    break;

  case 32: /* expr: FOR IDENTIFIER '=' expr IN expr  */
#line 169 "lang/parser.y"
                                      {
                                          Ast *let = ast_let(ast_identifier((yyvsp[-4].vident)), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));
                                          let->tag = AST_LOOP;
                                          (yyval.ast_node_ptr) = let;

                                      }
#line 2067 "lang/y.tab.c"
    break;

  case 33: /* expr: expr '[' expr ']'  */
#line 175 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = array_index_expression((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr));}
#line 2073 "lang/y.tab.c"
    break;

  case 34: /* expr: expr '[' expr DOUBLE_DOT ']'  */
#line 176 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = array_offset_expression((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr));}
#line 2079 "lang/y.tab.c"
    break;

  case 35: /* expr: expr '[' expr DOUBLE_DOT expr ']'  */
#line 177 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = array_range_expression((yyvsp[-5].ast_node_ptr), (yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr));}
#line 2085 "lang/y.tab.c"
    break;

  case 36: /* expr: expr ':' '=' expr  */
#line 178 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_assignment((yyvsp[-3].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2091 "lang/y.tab.c"
    break;

  case 38: /* atom_expr: atom_expr '.' IDENTIFIER  */
#line 183 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_record_access((yyvsp[-2].ast_node_ptr), ast_identifier((yyvsp[0].vident))); }
#line 2097 "lang/y.tab.c"
    break;

  case 39: /* simple_expr: INTEGER  */
#line 187 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[0].vint)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2103 "lang/y.tab.c"
    break;

  case 40: /* simple_expr: DOUBLE  */
#line 188 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[0].vdouble)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2109 "lang/y.tab.c"
    break;

  case 41: /* simple_expr: FLOAT  */
#line 189 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_FLOAT, (yyvsp[0].vfloat)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2115 "lang/y.tab.c"
    break;

  case 42: /* simple_expr: TOK_STRING  */
#line 190 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_string((yyvsp[0].vstr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2121 "lang/y.tab.c"
    break;

  case 43: /* simple_expr: TRUE  */
#line 191 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2127 "lang/y.tab.c"
    break;

  case 44: /* simple_expr: FALSE  */
#line 192 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2133 "lang/y.tab.c"
    break;

  case 45: /* simple_expr: IDENTIFIER  */
#line 193 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2139 "lang/y.tab.c"
    break;

  case 46: /* simple_expr: TOK_VOID  */
#line 194 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_void(); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2145 "lang/y.tab.c"
    break;

  case 47: /* simple_expr: list  */
#line 195 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2151 "lang/y.tab.c"
    break;

  case 48: /* simple_expr: array  */
#line 196 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2157 "lang/y.tab.c"
    break;

  case 49: /* simple_expr: tuple  */
#line 197 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2163 "lang/y.tab.c"
    break;

  case 50: /* simple_expr: fstring  */
#line 198 "lang/parser.y"
                          { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[0].ast_node_ptr)); }
#line 2169 "lang/y.tab.c"
    break;

  case 51: /* simple_expr: TOK_CHAR  */
#line 199 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_char((yyvsp[0].vchar)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2175 "lang/y.tab.c"
    break;

  case 52: /* simple_expr: '(' expr_sequence ')'  */
#line 200 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2181 "lang/y.tab.c"
    break;

  case 53: /* simple_expr: '(' FN lambda_args ARROW expr_sequence ')'  */
#line 201 "lang/parser.y"
                                               { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2187 "lang/y.tab.c"
    break;

  case 54: /* simple_expr: '(' FN TOK_VOID ARROW expr_sequence ')'  */
#line 202 "lang/parser.y"
                                               { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2193 "lang/y.tab.c"
    break;

  case 55: /* simple_expr: '(' LET IDENTIFIER '=' FN lambda_args ARROW expr_sequence ')'  */
#line 204 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-6].vident)), ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2199 "lang/y.tab.c"
    break;

  case 56: /* simple_expr: '(' LET IDENTIFIER '=' FN TOK_VOID ARROW expr_sequence ')'  */
#line 206 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-6].vident)), ast_void_lambda((yyvsp[-1].ast_node_ptr)), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2205 "lang/y.tab.c"
    break;

  case 57: /* simple_expr: '(' '+' ')'  */
#line 207 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"+", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2211 "lang/y.tab.c"
    break;

  case 58: /* simple_expr: '(' '-' ')'  */
#line 208 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"-", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2217 "lang/y.tab.c"
    break;

  case 59: /* simple_expr: '(' '*' ')'  */
#line 209 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"*", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2223 "lang/y.tab.c"
    break;

  case 60: /* simple_expr: '(' '/' ')'  */
#line 210 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"/", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2229 "lang/y.tab.c"
    break;

  case 61: /* simple_expr: '(' MODULO ')'  */
#line 211 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"%", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2235 "lang/y.tab.c"
    break;

  case 62: /* simple_expr: '(' '<' ')'  */
#line 212 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2241 "lang/y.tab.c"
    break;

  case 63: /* simple_expr: '(' '>' ')'  */
#line 213 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){">", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2247 "lang/y.tab.c"
    break;

  case 64: /* simple_expr: '(' DOUBLE_AMP ')'  */
#line 214 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"&&", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2253 "lang/y.tab.c"
    break;

  case 65: /* simple_expr: '(' DOUBLE_PIPE ')'  */
#line 215 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"||", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2259 "lang/y.tab.c"
    break;

  case 66: /* simple_expr: '(' GE ')'  */
#line 216 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){">=", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2265 "lang/y.tab.c"
    break;

  case 67: /* simple_expr: '(' LE ')'  */
#line 217 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<=", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2271 "lang/y.tab.c"
    break;

  case 68: /* simple_expr: '(' NE ')'  */
#line 218 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"!=", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2277 "lang/y.tab.c"
    break;

  case 69: /* simple_expr: '(' EQ ')'  */
#line 219 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"==", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2283 "lang/y.tab.c"
    break;

  case 70: /* simple_expr: '(' PIPE ')'  */
#line 220 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"|", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2289 "lang/y.tab.c"
    break;

  case 71: /* simple_expr: '(' ':' ')'  */
#line 221 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){":", 1}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2295 "lang/y.tab.c"
    break;

  case 72: /* simple_expr: '(' DOUBLE_COLON ')'  */
#line 222 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"::", 2}); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2301 "lang/y.tab.c"
    break;

  case 73: /* simple_expr: '(' IDENTIFIER ')'  */
#line 223 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_identifier((yyvsp[-1].vident)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2307 "lang/y.tab.c"
    break;

  case 74: /* expr_sequence: expr  */
#line 228 "lang/parser.y"
                                { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2313 "lang/y.tab.c"
    break;

  case 75: /* expr_sequence: expr_sequence ';' expr  */
#line 229 "lang/parser.y"
                                { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2319 "lang/y.tab.c"
    break;

  case 76: /* let_binding: LET TEST_ID '=' expr  */
#line 233 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_test_module((yyvsp[0].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2325 "lang/y.tab.c"
    break;

  case 77: /* let_binding: LET IDENTIFIER '=' expr  */
#line 234 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2331 "lang/y.tab.c"
    break;

  case 78: /* let_binding: LET IDENTIFIER '=' EXTERN FN fn_signature  */
#line 236 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-4].vident)), ast_extern_fn((yyvsp[-4].vident), (yyvsp[0].ast_node_ptr)), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2337 "lang/y.tab.c"
    break;

  case 79: /* let_binding: LET lambda_arg '=' expr  */
#line 238 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2343 "lang/y.tab.c"
    break;

  case 80: /* let_binding: LET expr_list '=' expr  */
#line 240 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_tuple((yyvsp[-2].ast_node_ptr)), (yyvsp[0].ast_node_ptr), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc));}
#line 2349 "lang/y.tab.c"
    break;

  case 81: /* let_binding: LET MUT expr_list '=' expr  */
#line 242 "lang/parser.y"
                                    { Ast *let = ast_let(ast_tuple((yyvsp[-2].ast_node_ptr)), (yyvsp[0].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      SET_AST_LOC(let, (yyloc));
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2359 "lang/y.tab.c"
    break;

  case 82: /* let_binding: LET TOK_VOID '=' expr  */
#line 251 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2365 "lang/y.tab.c"
    break;

  case 83: /* let_binding: let_binding IN expr  */
#line 252 "lang/parser.y"
                                    {
                                      Ast *let = (yyvsp[-2].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[0].ast_node_ptr);
                                      SET_AST_LOC(let, (yyloc));
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2376 "lang/y.tab.c"
    break;

  case 84: /* let_binding: lambda_expr  */
#line 258 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2382 "lang/y.tab.c"
    break;

  case 85: /* let_binding: LET '(' IDENTIFIER ')' '=' lambda_expr  */
#line 261 "lang/parser.y"
                                    {
                                      Ast *id = ast_identifier((yyvsp[-3].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[0].ast_node_ptr), NULL);
                                      SET_AST_LOC((yyval.ast_node_ptr), (yyloc));
                                    }
#line 2393 "lang/y.tab.c"
    break;

  case 86: /* let_binding: LET '(' IDENTIFIER ')' '=' expr  */
#line 269 "lang/parser.y"
                                    {
                                      Ast *id = ast_identifier((yyvsp[-3].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[0].ast_node_ptr), NULL);
                                      SET_AST_LOC((yyval.ast_node_ptr), (yyloc));
                                    }
#line 2404 "lang/y.tab.c"
    break;

  case 87: /* let_binding: IMPORT PATH_IDENTIFIER  */
#line 279 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), false); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2410 "lang/y.tab.c"
    break;

  case 88: /* let_binding: OPEN PATH_IDENTIFIER  */
#line 280 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), true); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2416 "lang/y.tab.c"
    break;

  case 89: /* let_binding: IMPORT IDENTIFIER  */
#line 281 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), false); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2422 "lang/y.tab.c"
    break;

  case 90: /* let_binding: OPEN IDENTIFIER  */
#line 282 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), true); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2428 "lang/y.tab.c"
    break;

  case 91: /* let_binding: LET IDENTIFIER ':' IDENTIFIER '=' lambda_expr  */
#line 283 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_trait_impl((yyvsp[-2].vident), (yyvsp[-4].vident), (yyvsp[0].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2434 "lang/y.tab.c"
    break;

  case 92: /* let_binding: LET IDENTIFIER '=' AT IDENTIFIER lambda_expr  */
#line 284 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_decorated_lambda((yyvsp[-1].vident), (yyvsp[-4].vident), (yyvsp[0].ast_node_ptr)); }
#line 2440 "lang/y.tab.c"
    break;

  case 93: /* lambda_expr: FN lambda_args ARROW expr_sequence ';'  */
#line 290 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2446 "lang/y.tab.c"
    break;

  case 94: /* lambda_expr: FN TOK_VOID ARROW expr_sequence ';'  */
#line 291 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2452 "lang/y.tab.c"
    break;

  case 95: /* lambda_expr: MODULE lambda_args ARROW expr_sequence ';'  */
#line 292 "lang/parser.y"
                                              { (yyval.ast_node_ptr) = ast_module(ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr))); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2458 "lang/y.tab.c"
    break;

  case 96: /* lambda_expr: MODULE TOK_VOID ARROW expr_sequence ';'  */
#line 293 "lang/parser.y"
                                              { (yyval.ast_node_ptr) = ast_module(ast_lambda(NULL, (yyvsp[-1].ast_node_ptr))); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2464 "lang/y.tab.c"
    break;

  case 97: /* lambda_args: lambda_arg  */
#line 300 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[0].ast_node_ptr), NULL); }
#line 2470 "lang/y.tab.c"
    break;

  case 98: /* lambda_args: lambda_arg '=' expr  */
#line 301 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list(ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL), NULL); }
#line 2476 "lang/y.tab.c"
    break;

  case 99: /* lambda_args: lambda_arg ':' '(' type_expr ')'  */
#line 302 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[-4].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2482 "lang/y.tab.c"
    break;

  case 100: /* lambda_args: lambda_args lambda_arg  */
#line 303 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); }
#line 2488 "lang/y.tab.c"
    break;

  case 101: /* lambda_args: lambda_args lambda_arg '=' expr  */
#line 304 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-3].ast_node_ptr), ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL), NULL); }
#line 2494 "lang/y.tab.c"
    break;

  case 102: /* lambda_args: lambda_args lambda_arg ':' '(' type_expr ')'  */
#line 305 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-5].ast_node_ptr), (yyvsp[-4].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2500 "lang/y.tab.c"
    break;

  case 103: /* lambda_arg: IDENTIFIER  */
#line 309 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2506 "lang/y.tab.c"
    break;

  case 104: /* lambda_arg: '(' expr_list ')'  */
#line 310 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-1].ast_node_ptr)); }
#line 2512 "lang/y.tab.c"
    break;

  case 105: /* lambda_arg: IDENTIFIER DOUBLE_COLON lambda_arg  */
#line 311 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2518 "lang/y.tab.c"
    break;

  case 106: /* lambda_arg: '_'  */
#line 312 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = Ast_new(AST_PLACEHOLDER_ID); }
#line 2524 "lang/y.tab.c"
    break;

  case 107: /* list: '[' ']'  */
#line 317 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_empty_list(); }
#line 2530 "lang/y.tab.c"
    break;

  case 108: /* list: '[' expr_list ']'  */
#line 318 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2536 "lang/y.tab.c"
    break;

  case 109: /* list: '[' expr_list ',' ']'  */
#line 319 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[-2].ast_node_ptr); }
#line 2542 "lang/y.tab.c"
    break;

  case 110: /* array: '[' '|' '|' ']'  */
#line 323 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_empty_array(); }
#line 2548 "lang/y.tab.c"
    break;

  case 111: /* array: '[' '|' expr_list '|' ']'  */
#line 324 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[-2].ast_node_ptr)); }
#line 2554 "lang/y.tab.c"
    break;

  case 112: /* array: '[' '|' expr_list ',' '|' ']'  */
#line 325 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[-3].ast_node_ptr)); }
#line 2560 "lang/y.tab.c"
    break;

  case 113: /* tuple: '(' expr ')'  */
#line 330 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2566 "lang/y.tab.c"
    break;

  case 114: /* tuple: '(' expr_list ')'  */
#line 331 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-1].ast_node_ptr)); }
#line 2572 "lang/y.tab.c"
    break;

  case 115: /* tuple: '(' expr_list ',' ')'  */
#line 332 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-2].ast_node_ptr)); }
#line 2578 "lang/y.tab.c"
    break;

  case 116: /* expr_list: expr  */
#line 336 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_list((yyvsp[0].ast_node_ptr)); }
#line 2584 "lang/y.tab.c"
    break;

  case 117: /* expr_list: expr_list ',' expr  */
#line 337 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2590 "lang/y.tab.c"
    break;

  case 118: /* match_expr: MATCH expr WITH match_branches  */
#line 341 "lang/parser.y"
                                   { (yyval.ast_node_ptr) = ast_match((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc)); }
#line 2596 "lang/y.tab.c"
    break;

  case 119: /* match_expr: IF expr THEN expr ELSE expr  */
#line 342 "lang/parser.y"
                                 { (yyval.ast_node_ptr) = ast_if_else((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr) ,(yyvsp[0].ast_node_ptr)); SET_AST_LOC((yyval.ast_node_ptr), (yyloc));}
#line 2602 "lang/y.tab.c"
    break;

  case 120: /* match_expr: IF expr THEN expr  */
#line 343 "lang/parser.y"
                                 { (yyval.ast_node_ptr) = ast_if_else((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); SET_AST_LOC((yyval.ast_node_ptr), (yyloc));}
#line 2608 "lang/y.tab.c"
    break;

  case 121: /* match_test_clause: expr  */
#line 347 "lang/parser.y"
         {(yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr);}
#line 2614 "lang/y.tab.c"
    break;

  case 122: /* match_test_clause: expr IF expr  */
#line 348 "lang/parser.y"
                 { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2620 "lang/y.tab.c"
    break;

  case 123: /* match_branches: '|' match_test_clause ARROW expr  */
#line 351 "lang/parser.y"
                                                                           {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2626 "lang/y.tab.c"
    break;

  case 124: /* match_branches: match_branches '|' match_test_clause ARROW expr  */
#line 352 "lang/parser.y"
                                                                           {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2632 "lang/y.tab.c"
    break;

  case 125: /* match_branches: match_branches '|' '_' ARROW expr  */
#line 353 "lang/parser.y"
                                                              {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[-4].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[0].ast_node_ptr));}
#line 2638 "lang/y.tab.c"
    break;

  case 126: /* fstring: FSTRING_START fstring_parts FSTRING_END  */
#line 356 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2644 "lang/y.tab.c"
    break;

  case 127: /* fstring_parts: %empty  */
#line 360 "lang/parser.y"
                                { (yyval.ast_node_ptr) = ast_empty_list(); }
#line 2650 "lang/y.tab.c"
    break;

  case 128: /* fstring_parts: fstring_parts fstring_part  */
#line 361 "lang/parser.y"
                                { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2656 "lang/y.tab.c"
    break;

  case 129: /* fstring_part: FSTRING_TEXT  */
#line 365 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_string((yyvsp[0].vstr)); }
#line 2662 "lang/y.tab.c"
    break;

  case 130: /* fstring_part: FSTRING_INTERP_START expr FSTRING_INTERP_END  */
#line 366 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2668 "lang/y.tab.c"
    break;

  case 131: /* type_decl: TYPE IDENTIFIER '=' type_expr  */
#line 370 "lang/parser.y"
                                  {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    SET_AST_LOC(type_decl, (yyloc));
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
#line 2679 "lang/y.tab.c"
    break;

  case 132: /* type_decl: TYPE IDENTIFIER  */
#line 377 "lang/parser.y"
                                 {
                                      Ast *type_decl = ast_let(ast_identifier((yyvsp[0].vident)), NULL, NULL);
                                      type_decl->tag = AST_TYPE_DECL;
                                      SET_AST_LOC(type_decl, (yyloc));
                                      (yyval.ast_node_ptr) = type_decl;
                                   }
#line 2690 "lang/y.tab.c"
    break;

  case 133: /* type_decl: TYPE type_args '=' type_expr  */
#line 384 "lang/parser.y"
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
#line 2711 "lang/y.tab.c"
    break;

  case 134: /* type_args: IDENTIFIER IDENTIFIER  */
#line 403 "lang/parser.y"
                                         { (yyval.ast_node_ptr) = ast_arg_list_push(ast_arg_list(ast_identifier((yyvsp[-1].vident)), NULL), ast_identifier((yyvsp[0].vident)), NULL); }
#line 2717 "lang/y.tab.c"
    break;

  case 135: /* type_args: type_args IDENTIFIER  */
#line 404 "lang/parser.y"
                                         { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), ast_identifier((yyvsp[0].vident)), NULL); }
#line 2723 "lang/y.tab.c"
    break;

  case 136: /* fn_signature: type_expr ARROW type_expr  */
#line 407 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2729 "lang/y.tab.c"
    break;

  case 137: /* fn_signature: fn_signature ARROW type_expr  */
#line 408 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2735 "lang/y.tab.c"
    break;

  case 138: /* tuple_type: type_expr_no_tuple ',' type_expr_no_tuple  */
#line 412 "lang/parser.y"
                                              { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2741 "lang/y.tab.c"
    break;

  case 139: /* tuple_type: tuple_type ',' type_expr_no_tuple  */
#line 413 "lang/parser.y"
                                             { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2747 "lang/y.tab.c"
    break;

  case 140: /* type_expr: type_expr_no_tuple  */
#line 417 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2753 "lang/y.tab.c"
    break;

  case 141: /* type_expr: tuple_type  */
#line 418 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2759 "lang/y.tab.c"
    break;

  case 142: /* type_expr_no_tuple: type_atom  */
#line 422 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2765 "lang/y.tab.c"
    break;

  case 143: /* type_expr_no_tuple: '|' type_atom  */
#line 423 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_list((yyvsp[0].ast_node_ptr)); }
#line 2771 "lang/y.tab.c"
    break;

  case 144: /* type_expr_no_tuple: type_expr_no_tuple '|' type_atom  */
#line 424 "lang/parser.y"
                                     { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2777 "lang/y.tab.c"
    break;

  case 145: /* type_expr_no_tuple: fn_signature  */
#line 425 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_fn_signature_of_list((yyvsp[0].ast_node_ptr)); }
#line 2783 "lang/y.tab.c"
    break;

  case 146: /* type_expr_no_tuple: type_atom TRIPLE_DOT  */
#line 426 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_variadic_expr((yyvsp[-1].ast_node_ptr)); }
#line 2789 "lang/y.tab.c"
    break;

  case 147: /* type_atom: IDENTIFIER  */
#line 430 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2795 "lang/y.tab.c"
    break;

  case 148: /* type_atom: IDENTIFIER '=' INTEGER  */
#line 431 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-2].vident)), AST_CONST(AST_INT, (yyvsp[0].vint)), NULL); }
#line 2801 "lang/y.tab.c"
    break;

  case 149: /* type_atom: IDENTIFIER OF type_atom  */
#line 432 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2807 "lang/y.tab.c"
    break;

  case 150: /* type_atom: IDENTIFIER ':' type_expr_no_tuple  */
#line 433 "lang/parser.y"
                                       { (yyval.ast_node_ptr) = ast_assoc(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2813 "lang/y.tab.c"
    break;

  case 151: /* type_atom: '(' type_expr ')'  */
#line 434 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2819 "lang/y.tab.c"
    break;

  case 152: /* type_atom: '(' type_expr_no_tuple ',' ')'  */
#line 435 "lang/parser.y"
                                   { (yyval.ast_node_ptr) = ast_tuple_type_single((yyvsp[-2].ast_node_ptr)); }
#line 2825 "lang/y.tab.c"
    break;

  case 153: /* type_atom: '(' tuple_type ',' ')'  */
#line 436 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[-2].ast_node_ptr); }
#line 2831 "lang/y.tab.c"
    break;

  case 154: /* type_atom: TOK_VOID  */
#line 437 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_void(); }
#line 2837 "lang/y.tab.c"
    break;

  case 155: /* type_atom: IDENTIFIER '.' IDENTIFIER  */
#line 438 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_record_access(ast_identifier((yyvsp[-2].vident)), ast_identifier((yyvsp[0].vident))); }
#line 2843 "lang/y.tab.c"
    break;


#line 2847 "lang/y.tab.c"

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

#line 440 "lang/parser.y"



void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, pctx.cur_script);
}
#endif _LANG_TAB_H
