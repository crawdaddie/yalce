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

/* Define global variable for the root of AST */
Ast* ast_root = NULL;
#define AST_CONST(type, val)                                            \
    ({                                                                  \
      Ast *prefix = Ast_new(type);                                      \
      prefix->data.type.value = val;                                    \
      prefix;                                                           \
    })


#line 100 "lang/y.tab.c"

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
  YYSYMBOL_IDENTIFIER = 5,                 /* IDENTIFIER  */
  YYSYMBOL_MACRO_IDENTIFIER = 6,           /* MACRO_IDENTIFIER  */
  YYSYMBOL_PATH_IDENTIFIER = 7,            /* PATH_IDENTIFIER  */
  YYSYMBOL_IDENTIFIER_LIST = 8,            /* IDENTIFIER_LIST  */
  YYSYMBOL_TOK_STRING = 9,                 /* TOK_STRING  */
  YYSYMBOL_TOK_CHAR = 10,                  /* TOK_CHAR  */
  YYSYMBOL_TRUE = 11,                      /* TRUE  */
  YYSYMBOL_FALSE = 12,                     /* FALSE  */
  YYSYMBOL_PIPE = 13,                      /* PIPE  */
  YYSYMBOL_EXTERN = 14,                    /* EXTERN  */
  YYSYMBOL_TRIPLE_DOT = 15,                /* TRIPLE_DOT  */
  YYSYMBOL_LET = 16,                       /* LET  */
  YYSYMBOL_FN = 17,                        /* FN  */
  YYSYMBOL_MATCH = 18,                     /* MATCH  */
  YYSYMBOL_WITH = 19,                      /* WITH  */
  YYSYMBOL_ARROW = 20,                     /* ARROW  */
  YYSYMBOL_DOUBLE_COLON = 21,              /* DOUBLE_COLON  */
  YYSYMBOL_TOK_VOID = 22,                  /* TOK_VOID  */
  YYSYMBOL_IN = 23,                        /* IN  */
  YYSYMBOL_AND = 24,                       /* AND  */
  YYSYMBOL_ASYNC = 25,                     /* ASYNC  */
  YYSYMBOL_DOUBLE_AT = 26,                 /* DOUBLE_AT  */
  YYSYMBOL_THUNK = 27,                     /* THUNK  */
  YYSYMBOL_IMPORT = 28,                    /* IMPORT  */
  YYSYMBOL_OPEN = 29,                      /* OPEN  */
  YYSYMBOL_IMPLEMENTS = 30,                /* IMPLEMENTS  */
  YYSYMBOL_AMPERSAND = 31,                 /* AMPERSAND  */
  YYSYMBOL_TYPE = 32,                      /* TYPE  */
  YYSYMBOL_TEST_ID = 33,                   /* TEST_ID  */
  YYSYMBOL_MUT = 34,                       /* MUT  */
  YYSYMBOL_FSTRING_START = 35,             /* FSTRING_START  */
  YYSYMBOL_FSTRING_END = 36,               /* FSTRING_END  */
  YYSYMBOL_FSTRING_INTERP_START = 37,      /* FSTRING_INTERP_START  */
  YYSYMBOL_FSTRING_INTERP_END = 38,        /* FSTRING_INTERP_END  */
  YYSYMBOL_FSTRING_TEXT = 39,              /* FSTRING_TEXT  */
  YYSYMBOL_40_ = 40,                       /* '|'  */
  YYSYMBOL_APPLICATION = 41,               /* APPLICATION  */
  YYSYMBOL_DOUBLE_AMP = 42,                /* DOUBLE_AMP  */
  YYSYMBOL_DOUBLE_PIPE = 43,               /* DOUBLE_PIPE  */
  YYSYMBOL_GE = 44,                        /* GE  */
  YYSYMBOL_LE = 45,                        /* LE  */
  YYSYMBOL_EQ = 46,                        /* EQ  */
  YYSYMBOL_NE = 47,                        /* NE  */
  YYSYMBOL_48_ = 48,                       /* '>'  */
  YYSYMBOL_49_ = 49,                       /* '<'  */
  YYSYMBOL_50_ = 50,                       /* '+'  */
  YYSYMBOL_51_ = 51,                       /* '-'  */
  YYSYMBOL_52_ = 52,                       /* '*'  */
  YYSYMBOL_53_ = 53,                       /* '/'  */
  YYSYMBOL_MODULO = 54,                    /* MODULO  */
  YYSYMBOL_55_ = 55,                       /* ':'  */
  YYSYMBOL_56_ = 56,                       /* '.'  */
  YYSYMBOL_UMINUS = 57,                    /* UMINUS  */
  YYSYMBOL_58_ = 58,                       /* ';'  */
  YYSYMBOL_59_yield_ = 59,                 /* "yield"  */
  YYSYMBOL_60_to_ = 60,                    /* "to"  */
  YYSYMBOL_61_for_ = 61,                   /* "for"  */
  YYSYMBOL_62_ = 62,                       /* '='  */
  YYSYMBOL_63_ = 63,                       /* '('  */
  YYSYMBOL_64_ = 64,                       /* ')'  */
  YYSYMBOL_65_ = 65,                       /* ','  */
  YYSYMBOL_66_module_ = 66,                /* "module"  */
  YYSYMBOL_67_ = 67,                       /* '['  */
  YYSYMBOL_68_ = 68,                       /* ']'  */
  YYSYMBOL_69_if_ = 69,                    /* "if"  */
  YYSYMBOL_70___ = 70,                     /* '_'  */
  YYSYMBOL_71_of_ = 71,                    /* "of"  */
  YYSYMBOL_YYACCEPT = 72,                  /* $accept  */
  YYSYMBOL_program = 73,                   /* program  */
  YYSYMBOL_expr = 74,                      /* expr  */
  YYSYMBOL_simple_expr = 75,               /* simple_expr  */
  YYSYMBOL_custom_binop = 76,              /* custom_binop  */
  YYSYMBOL_expr_sequence = 77,             /* expr_sequence  */
  YYSYMBOL_let_binding = 78,               /* let_binding  */
  YYSYMBOL_lambda_expr = 79,               /* lambda_expr  */
  YYSYMBOL_lambda_args = 80,               /* lambda_args  */
  YYSYMBOL_lambda_arg = 81,                /* lambda_arg  */
  YYSYMBOL_list = 82,                      /* list  */
  YYSYMBOL_array = 83,                     /* array  */
  YYSYMBOL_list_match_expr = 84,           /* list_match_expr  */
  YYSYMBOL_tuple = 85,                     /* tuple  */
  YYSYMBOL_expr_list = 86,                 /* expr_list  */
  YYSYMBOL_match_expr = 87,                /* match_expr  */
  YYSYMBOL_match_test_clause = 88,         /* match_test_clause  */
  YYSYMBOL_match_branches = 89,            /* match_branches  */
  YYSYMBOL_fstring = 90,                   /* fstring  */
  YYSYMBOL_fstring_parts = 91,             /* fstring_parts  */
  YYSYMBOL_fstring_part = 92,              /* fstring_part  */
  YYSYMBOL_type_decl = 93,                 /* type_decl  */
  YYSYMBOL_type_args = 94,                 /* type_args  */
  YYSYMBOL_fn_signature = 95,              /* fn_signature  */
  YYSYMBOL_tuple_type = 96,                /* tuple_type  */
  YYSYMBOL_type_expr = 97,                 /* type_expr  */
  YYSYMBOL_type_atom = 98                  /* type_atom  */
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
#define YYFINAL  94
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2034

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  72
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  150
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  313

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   309


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
      63,    64,    52,    50,    65,    51,    56,    53,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    55,    58,
      49,    62,    48,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    67,     2,    68,     2,    70,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    40,     2,     2,     2,     2,     2,
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
      35,    36,    37,    38,    39,    41,    42,    43,    44,    45,
      46,    47,    54,    57,    59,    60,    61,    66,    69,    71
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   124,   124,   125,   126,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   159,   164,   173,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,   207,   208,   209,   213,
     236,   237,   241,   242,   243,   247,   250,   252,   256,   258,
     274,   275,   280,   282,   289,   299,   300,   301,   302,   323,
     324,   325,   326,   327,   328,   335,   336,   337,   338,   340,
     341,   342,   343,   347,   348,   349,   359,   360,   361,   365,
     366,   367,   371,   372,   376,   377,   378,   382,   383,   387,
     391,   392,   395,   396,   397,   400,   404,   405,   409,   410,
     414,   420,   426,   439,   440,   443,   444,   448,   449,   453,
     454,   455,   456,   457,   461,   462,   463,   464,   465,   466,
     467
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
  "IDENTIFIER", "MACRO_IDENTIFIER", "PATH_IDENTIFIER", "IDENTIFIER_LIST",
  "TOK_STRING", "TOK_CHAR", "TRUE", "FALSE", "PIPE", "EXTERN",
  "TRIPLE_DOT", "LET", "FN", "MATCH", "WITH", "ARROW", "DOUBLE_COLON",
  "TOK_VOID", "IN", "AND", "ASYNC", "DOUBLE_AT", "THUNK", "IMPORT", "OPEN",
  "IMPLEMENTS", "AMPERSAND", "TYPE", "TEST_ID", "MUT", "FSTRING_START",
  "FSTRING_END", "FSTRING_INTERP_START", "FSTRING_INTERP_END",
  "FSTRING_TEXT", "'|'", "APPLICATION", "DOUBLE_AMP", "DOUBLE_PIPE", "GE",
  "LE", "EQ", "NE", "'>'", "'<'", "'+'", "'-'", "'*'", "'/'", "MODULO",
  "':'", "'.'", "UMINUS", "';'", "\"yield\"", "\"to\"", "\"for\"", "'='",
  "'('", "')'", "','", "\"module\"", "'['", "']'", "\"if\"", "'_'",
  "\"of\"", "$accept", "program", "expr", "simple_expr", "custom_binop",
  "expr_sequence", "let_binding", "lambda_expr", "lambda_args",
  "lambda_arg", "list", "array", "list_match_expr", "tuple", "expr_list",
  "match_expr", "match_test_clause", "match_branches", "fstring",
  "fstring_parts", "fstring_part", "type_decl", "type_args",
  "fn_signature", "tuple_type", "type_expr", "type_atom", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-89)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-118)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1704,   -89,   -89,   -13,  1704,   -89,   -89,   -89,   -89,   -89,
    1704,  1281,     0,  1704,   -89,  1704,   153,   165,    25,   -89,
    1704,    33,   651,  1769,   519,    56,  1094,    13,    -3,    53,
     -89,   -89,   -89,   -89,   -89,   -89,   -89,    83,  1094,  1094,
      28,    70,    85,  1834,   716,  1094,    89,   -89,    12,   158,
     160,  1704,     3,     2,   911,  1094,   -89,   -89,   -89,   -89,
     121,    -2,   116,  1094,   126,   -13,   128,     7,   131,   141,
     142,   143,   144,   145,   146,   147,   148,   151,   152,   154,
     155,   156,   157,   846,   164,   -31,    97,   161,   651,   166,
       8,  1346,   -89,    55,   -89,   -89,  1704,  1704,  1704,  1704,
    1704,  1704,  1704,  1704,  1704,  1704,  1704,  1704,  1704,  1704,
    1704,  1704,  1411,  1704,   781,    13,   217,  1704,  1704,   -89,
    1899,  1476,  1704,  1704,   115,   167,    72,   -20,   134,  1704,
    1704,  1704,  1704,   136,  1704,    23,   174,  1704,   190,    10,
     -89,    10,   -89,  1704,   -89,   -89,  1704,   -89,   218,    16,
     -89,   -89,   -89,   -89,   -89,   -89,   -89,   -89,   -89,   -89,
     -89,   -89,   -89,   -89,   -89,   -89,   -89,  1704,   -89,   -89,
    1541,  1704,  1704,   171,   -14,   585,   -89,  1155,  1094,  1216,
    1574,  1574,   222,   222,   222,   222,   222,   222,  1932,  1932,
    1967,  1967,  1324,  1704,  1389,  1094,    19,   -89,  1094,  1094,
     -13,  1094,   223,  1094,  1094,  1094,  1704,  1704,  1704,   179,
      -1,  1094,  1094,  1094,   184,   -89,   192,   188,  1704,    10,
    1094,  1704,   212,    88,   -89,    20,    10,   234,   191,    32,
     193,    32,   972,  1033,  1704,  1704,   -89,   197,   -89,   194,
    1639,   -89,  1094,   239,    26,    10,  1094,  1094,  1094,  1704,
    1704,  1704,    10,  1094,    71,   452,   240,   384,    20,   256,
     260,    20,   -89,   101,    10,    20,    10,    20,    20,   -89,
    1704,     1,    22,  1704,   -89,   196,  1704,  1704,   234,    32,
    1094,   -89,   109,   -89,  1704,  1704,   245,   247,   -89,   -89,
     -89,   -89,   -89,    32,   -89,    32,   -89,   -89,  1094,   -89,
     -89,   -89,    64,    66,   -89,  1094,  1094,  1704,  1704,   -89,
     -89,  1094,  1094
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    36,    37,    41,     0,    32,    38,    47,    39,    40,
       0,     0,     0,     0,    42,     0,     0,     0,     0,   126,
       0,     0,     0,     0,     0,     0,    70,     5,     3,    26,
      82,    43,    44,    45,    27,    46,    28,     0,    33,    30,
      41,    42,     0,     0,     0,   117,     0,   105,     0,   103,
       0,     0,     0,    95,     0,    29,    87,    85,    88,    86,
     131,     0,     0,     6,     0,    41,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    70,     0,     0,     0,    41,     0,     0,
       0,     0,   106,     0,     1,    41,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     0,     2,     0,    31,
       0,     0,     0,     0,    41,     0,     0,    41,     0,     0,
       0,     0,     0,     0,     0,    97,     0,     0,     0,     0,
     134,     0,   125,     0,   128,   127,     0,    64,     0,     0,
      66,    58,    59,    60,    61,    63,    62,    57,    56,    51,
      52,    53,    54,    55,    65,   114,    67,     0,    48,   115,
       0,    94,     0,     0,     0,     0,   107,    22,    25,     7,
      16,    17,    18,    19,    21,    20,    15,    14,     9,    10,
      11,    12,    13,     0,    23,    24,     0,    68,    71,    81,
      41,   113,     0,    73,    80,    72,     0,     0,     0,     0,
     115,    76,    78,   118,     0,   104,     0,     0,     0,     0,
      96,     0,   119,   144,   149,     0,     0,   142,   143,   130,
     139,   132,     0,     0,     0,     0,   116,     0,   109,     0,
       0,   108,    35,     0,     0,     0,    74,    77,    79,     0,
      90,    89,     0,    98,     0,   120,     0,     0,     0,     0,
       0,     0,   140,     0,     0,     0,     0,     0,     0,   129,
       0,     0,     0,    93,   110,     0,     0,     0,    75,     0,
      84,    82,     0,   100,     0,     0,     0,     0,   147,   150,
     145,   146,   148,   136,   138,   135,   141,   137,    34,    50,
      49,   111,     0,     0,   102,   121,   122,     0,     0,    50,
      49,   124,   123
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -89,   -89,    -4,   130,   -89,    14,   -89,    21,   -22,    -9,
     -89,   -89,   -89,   -89,    24,   -89,    11,   -89,   -89,   -89,
     -89,   -89,   -89,    34,   -89,   -88,   -71
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    25,    26,    27,    84,    85,    29,    30,    52,    53,
      31,    32,    47,    33,    86,    34,   256,   222,    35,    62,
     145,    36,    61,   227,   228,   229,   230
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      38,    90,    46,   140,  -104,    49,    39,    45,    49,    54,
      37,    55,    49,    49,    28,   223,    63,    37,    83,  -104,
      45,    49,    50,   134,    49,   223,   239,   167,   172,   148,
      60,    49,   224,   168,   125,    48,   235,    89,    64,    45,
      83,   243,   224,   135,   209,   149,   277,    45,    93,   120,
     225,   240,   266,   231,  -104,   117,    94,   136,    37,   250,
     141,  -104,  -104,    51,   137,   299,    51,   126,   128,   116,
      51,    51,   267,   226,   130,   133,   118,   131,   217,    51,
     251,   135,    51,   226,    83,   218,   300,    45,   119,    51,
     121,   266,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   194,   195,
      83,   267,   128,   198,   199,   174,   201,   203,   204,   205,
     175,   266,   250,   176,   251,   211,   212,   213,   309,   266,
     310,   254,   122,   220,   208,   283,   120,   131,   263,   232,
     135,   267,   233,   258,   259,    37,   214,   123,   216,   267,
     260,   129,   142,   143,   262,   144,   115,   279,    56,   261,
      57,   169,   170,   198,   282,   292,   213,   198,   115,   115,
      58,   213,    59,   304,   244,   115,   293,   206,   295,   120,
     132,  -103,   120,   139,   115,   115,   237,   288,   146,   242,
     291,    37,   147,   115,   294,   150,   296,   297,   210,   170,
     215,   131,   246,   247,   248,   151,   152,   153,   154,   155,
     156,   157,   158,   115,   253,   159,   160,   255,   161,   162,
     163,   164,   197,  -103,   171,     1,     2,    95,   166,   207,
     221,     6,     7,     8,     9,   135,   213,   219,   234,   238,
     245,   249,   250,    97,    14,   280,   198,   198,   271,   272,
     251,   252,   257,   255,   264,   273,   265,    19,   268,   276,
     285,   289,   274,   290,   301,   307,   298,   308,   287,   198,
     281,     0,   107,   108,   109,   110,   111,   112,     0,   278,
     305,   306,   113,     0,     0,   114,     0,     0,     0,    24,
     302,   303,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   311,   312,     0,     0,   115,   115,   115,
     115,   115,   115,   115,   115,   115,   115,   115,   115,   115,
     115,   115,   115,     0,   115,   115,     0,     0,   115,   115,
       0,   115,     0,   115,   115,   115,     0,     0,     0,     0,
       0,   115,   115,   115,     0,     0,     0,     0,     0,     0,
     115,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   115,   115,     0,     0,     0,     0,     0,     0,
       0,     0,   115,     0,     0,     0,   115,   115,   115,     0,
       0,     0,     0,   115,     0,   115,     0,     1,     2,     3,
       4,     0,     5,     6,     7,     8,     9,     0,     0,    10,
      11,    12,    13,     0,     0,     0,    14,     0,     0,     0,
     115,    15,    16,    17,     0,     0,    18,     0,     0,    19,
       0,     0,     0,     0,     0,     0,     0,     0,   115,     0,
       0,     0,     0,     0,     0,   115,   115,     0,     0,     0,
       0,   115,   115,    20,     0,    21,     0,    22,     0,     0,
      23,    24,     0,     0,   286,     1,     2,    95,     0,     0,
       0,     6,     7,     8,     9,    96,     0,     0,     0,     0,
       0,     0,     0,    97,    14,     0,     0,     0,    98,     0,
       0,     0,     0,     0,     0,     0,     0,    19,     0,     0,
       0,     0,     0,     0,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,     0,     0,
       0,     0,   113,     0,     0,   114,     0,     0,     0,    24,
       0,   284,     1,     2,     3,     4,     0,     5,     6,     7,
       8,     9,     0,     0,    10,    11,    12,    13,     0,     0,
       0,    14,     0,     0,     0,     0,    15,    16,    17,     0,
       0,    18,     0,     0,    19,     0,     0,     0,     0,    91,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    20,     0,
      21,     0,    22,     0,     0,    23,    24,    92,     1,     2,
       3,     4,     0,     5,     6,     7,     8,     9,     0,     0,
      10,    11,    12,    13,     0,     0,     0,    14,     0,     0,
       0,     0,    15,    16,    17,     0,     0,    18,     0,     0,
      19,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    20,     0,    21,     0,    22,     0,
       0,    23,    24,   241,     1,     2,    65,     4,     0,     5,
       6,     7,     8,     9,    66,     0,    10,    11,    67,    13,
       0,     0,    68,    14,     0,     0,     0,     0,    15,    16,
      17,     0,     0,    18,     0,     0,    19,     0,     0,     0,
       0,     0,     0,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,     0,     0,     0,
      20,     0,    21,     0,    22,     0,     0,    23,    24,     1,
       2,   127,     4,     0,     5,     6,     7,     8,     9,    66,
       0,    10,    11,    67,    13,     0,     0,    68,    14,     0,
       0,     0,     0,    15,    16,    17,     0,     0,    18,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,     0,     0,     0,    20,     0,    21,     0,    22,
       0,     0,    23,    24,     1,     2,    65,     4,     0,     5,
       6,     7,     8,     9,    66,     0,    10,    11,   196,    13,
       0,     0,    68,    14,     0,     0,     0,     0,    15,    16,
      17,     0,     0,    18,     0,     0,    19,     0,     0,     0,
       0,     0,     0,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,     0,     0,     0,
      20,     0,    21,     0,    22,     0,     0,    23,    24,     1,
       2,    95,     0,     0,     0,     6,     7,     8,     9,    96,
       0,     0,     0,     0,     0,     0,     0,    97,    14,     0,
       0,     0,    98,     0,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,     0,     0,     0,     0,   113,     0,     0,   114,
     165,  -117,     0,    24,     1,     2,    95,     0,     0,     0,
       6,     7,     8,     9,    96,     0,     0,     0,     0,     0,
     138,     0,    97,    14,     0,     0,     0,    98,     0,     0,
       0,     0,     0,     0,     0,     0,    19,     0,     0,     0,
       0,     0,     0,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,     0,     0,     0,
       0,   113,     0,     0,   114,     1,     2,    95,    24,     0,
       0,     6,     7,     8,     9,    96,     0,     0,     0,     0,
       0,     0,     0,    97,    14,     0,     0,     0,    98,     0,
       0,     0,     0,     0,     0,     0,     0,    19,     0,     0,
     269,     0,     0,     0,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,     0,     0,
       0,     0,   113,     0,     0,   114,     1,     2,    95,    24,
       0,     0,     6,     7,     8,     9,    96,     0,     0,     0,
       0,     0,     0,     0,    97,    14,   270,     0,     0,    98,
       0,     0,     0,     0,     0,     0,     0,     0,    19,     0,
       0,     0,     0,     0,     0,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,     0,
       0,     0,     0,   113,     0,     0,   114,     1,     2,    95,
      24,     0,     0,     6,     7,     8,     9,    96,     0,     0,
       0,     0,     0,     0,     0,    97,    14,     0,     0,     0,
      98,     0,     0,     0,     0,     0,     0,     0,     0,    19,
       0,     0,     0,     0,     0,     0,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
       0,     0,     0,     0,   113,     0,     0,   114,     1,     2,
      95,    24,     0,     0,     6,     7,     8,     9,     0,     0,
       0,     0,     0,     0,     0,     0,    97,    14,     0,     0,
       0,    98,     0,     0,     0,     0,     0,     0,     0,     0,
      19,     0,     0,     0,     0,     0,     0,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,     0,     0,     0,     0,   113,     0,     0,   114,     1,
       2,    95,    24,     0,     0,     6,     7,     8,     9,     0,
       0,     0,     0,     0,     0,     0,     0,    97,    14,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    19,     0,     0,     0,     0,     0,     0,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,     0,     0,     0,     0,   113,     0,     0,   114,
       0,     0,     0,    24,     1,     2,    40,     4,     0,     5,
       6,     7,     8,     9,     0,     0,    10,    11,    12,    13,
       0,     0,     0,    41,     0,     0,     0,     0,    15,    16,
      17,     0,     0,    18,    42,    43,    19,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     1,     2,    95,
       0,     0,     0,     6,     7,     8,     9,     0,     0,     0,
      20,     0,    21,     0,    44,    97,    14,    23,    24,     1,
       2,     3,     4,     0,     5,     6,     7,     8,     9,    19,
       0,    10,    11,    12,    13,     0,     0,     0,    14,     0,
       0,     0,     0,    15,    16,    17,     0,     0,    18,   112,
       0,    19,     0,     0,   113,     0,   173,   114,     0,     0,
       0,    24,     1,     2,    95,     0,     0,     0,     6,     7,
       8,     9,     0,     0,     0,    20,     0,    21,     0,    22,
      97,    14,    23,    24,     1,     2,     3,     4,     0,     5,
       6,     7,     8,     9,    19,     0,    10,    11,    12,    13,
       0,     0,     0,    14,     0,     0,     0,     0,    15,    16,
      17,     0,     0,    18,     0,     0,    19,     0,     0,   113,
       0,     0,   114,     0,     0,     0,    24,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      20,     0,    21,   193,    22,     0,     0,    23,    24,     1,
       2,     3,     4,     0,     5,     6,     7,     8,     9,     0,
     202,    10,    11,    12,    13,     0,     0,     0,    14,     0,
       0,     0,     0,    15,    16,    17,     0,     0,    18,     0,
       0,    19,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    20,     0,    21,     0,    22,
       0,     0,    23,    24,     1,     2,     3,     4,     0,     5,
       6,     7,     8,     9,     0,     0,    10,    11,    12,    13,
       0,     0,     0,    14,     0,     0,     0,     0,    15,    16,
      17,     0,     0,    18,     0,     0,    19,     1,     2,    95,
       0,     0,     0,     6,     7,     8,     9,     0,     0,     0,
       0,     0,     0,     0,     0,    97,    14,     0,     0,     0,
      20,     0,    21,     0,    22,   236,     0,    23,    24,    19,
       0,     0,     0,     0,     0,     0,     0,     0,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
       0,     0,     0,     0,   113,     0,     0,   114,     0,     0,
       0,    24,     1,     2,     3,     4,     0,     5,     6,     7,
       8,     9,     0,     0,    10,    11,    12,    13,     0,     0,
       0,    14,     0,     0,     0,     0,    15,    16,    17,     0,
       0,    18,     0,     0,    19,     0,     0,     0,     0,   275,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    20,     0,
      21,     0,    22,     0,     0,    23,    24,     1,     2,     3,
       4,     0,     5,     6,     7,     8,     9,     0,     0,    10,
      11,    12,    13,     0,     0,     0,    14,     0,     0,     0,
       0,    15,    16,    17,     0,     0,    18,     0,     0,    19,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    20,     0,    21,     0,    22,     0,     0,
      23,    24,     1,     2,    87,     4,     0,     5,     6,     7,
       8,     9,     0,     0,    10,    11,    12,    13,     0,     0,
       0,    14,     0,     0,     0,     0,    15,    16,    17,     0,
       0,    18,     0,     0,    19,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    20,     0,
      21,     0,    88,     0,     0,    23,    24,     1,     2,   124,
       4,     0,     5,     6,     7,     8,     9,     0,     0,    10,
      11,    12,    13,     0,     0,     0,    14,     0,     0,     0,
       0,    15,    16,    17,     0,     0,    18,     0,     0,    19,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    20,     0,    21,     0,    88,     0,     0,
      23,    24,     1,     2,   200,     4,     0,     5,     6,     7,
       8,     9,     0,     0,    10,    11,    12,    13,     0,     0,
       0,    14,     0,     0,     0,     0,    15,    16,    17,     0,
       0,    18,     0,     0,    19,     1,     2,    95,     0,     0,
       0,     6,     7,     8,     9,     0,     0,     0,     0,     0,
       0,     0,     0,    97,    14,     0,     0,     0,    20,     0,
      21,     0,    22,     0,     0,    23,    24,    19,     0,     0,
       1,     2,    95,     0,     0,     0,     6,     7,     8,     9,
       0,     0,     0,     0,   109,   110,   111,   112,    97,    14,
       0,     0,   113,     0,     0,   114,     0,     0,     0,    24,
       0,     0,    19,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   111,   112,     0,     0,     0,     0,   113,     0,     0,
     114,     0,     0,     0,    24
};

static const yytype_int16 yycheck[] =
{
       4,    23,    11,     5,     5,     5,    10,    11,     5,    13,
      30,    15,     5,     5,     0,     5,    20,    30,    22,    20,
      24,     5,    22,    20,     5,     5,    40,    58,    20,    22,
       5,     5,    22,    64,    43,    11,    20,    23,     5,    43,
      44,    22,    22,    52,    64,    67,    20,    51,    24,    21,
      40,    65,    20,   141,    55,    58,     0,    55,    30,    58,
      62,    62,    63,    63,    62,    64,    63,    43,    44,    56,
      63,    63,    40,    63,    62,    51,    23,    65,    55,    63,
      58,    90,    63,    63,    88,    62,    64,    91,     5,    63,
      62,    20,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,    40,    88,   117,   118,    91,   120,   121,   122,   123,
      65,    20,    58,    68,    58,   129,   130,   131,    64,    20,
      64,   219,    62,   137,    62,    64,    21,    65,   226,   143,
     149,    40,   146,    55,    56,    30,   132,    62,   134,    40,
      62,    62,    36,    37,   225,    39,    26,   245,     5,    71,
       7,    64,    65,   167,   252,    64,   170,   171,    38,    39,
       5,   175,     7,    64,   196,    45,   264,    62,   266,    21,
      20,    20,    21,    62,    54,    55,   172,   258,    62,   193,
     261,    30,    64,    63,   265,    64,   267,   268,    64,    65,
      64,    65,   206,   207,   208,    64,    64,    64,    64,    64,
      64,    64,    64,    83,   218,    64,    64,   221,    64,    64,
      64,    64,     5,    62,    58,     3,     4,     5,    64,    62,
      40,     9,    10,    11,    12,   244,   240,    63,    20,    68,
      17,    62,    58,    21,    22,   249,   250,   251,   234,   235,
      58,    63,    40,   257,    20,    58,    65,    35,    65,    20,
      20,     5,    68,     3,    68,    20,   270,    20,   257,   273,
     249,    -1,    50,    51,    52,    53,    54,    55,    -1,   245,
     284,   285,    60,    -1,    -1,    63,    -1,    -1,    -1,    67,
     276,   277,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   307,   308,    -1,    -1,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,    -1,   194,   195,    -1,    -1,   198,   199,
      -1,   201,    -1,   203,   204,   205,    -1,    -1,    -1,    -1,
      -1,   211,   212,   213,    -1,    -1,    -1,    -1,    -1,    -1,
     220,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   232,   233,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   242,    -1,    -1,    -1,   246,   247,   248,    -1,
      -1,    -1,    -1,   253,    -1,   255,    -1,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    -1,    15,
      16,    17,    18,    -1,    -1,    -1,    22,    -1,    -1,    -1,
     280,    27,    28,    29,    -1,    -1,    32,    -1,    -1,    35,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   298,    -1,
      -1,    -1,    -1,    -1,    -1,   305,   306,    -1,    -1,    -1,
      -1,   311,   312,    59,    -1,    61,    -1,    63,    -1,    -1,
      66,    67,    -1,    -1,    70,     3,     4,     5,    -1,    -1,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    26,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,    -1,    -1,
      -1,    -1,    -1,    -1,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    -1,    -1,
      -1,    -1,    60,    -1,    -1,    63,    -1,    -1,    -1,    67,
      -1,    69,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    15,    16,    17,    18,    -1,    -1,
      -1,    22,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,
      -1,    32,    -1,    -1,    35,    -1,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,    -1,
      61,    -1,    63,    -1,    -1,    66,    67,    68,     3,     4,
       5,     6,    -1,     8,     9,    10,    11,    12,    -1,    -1,
      15,    16,    17,    18,    -1,    -1,    -1,    22,    -1,    -1,
      -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,    -1,
      35,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    59,    -1,    61,    -1,    63,    -1,
      -1,    66,    67,    68,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    13,    -1,    15,    16,    17,    18,
      -1,    -1,    21,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    -1,    -1,    32,    -1,    -1,    35,    -1,    -1,    -1,
      -1,    -1,    -1,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    -1,    -1,    -1,
      59,    -1,    61,    -1,    63,    -1,    -1,    66,    67,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    13,
      -1,    15,    16,    17,    18,    -1,    -1,    21,    22,    -1,
      -1,    -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,
      -1,    35,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    -1,    -1,    -1,    59,    -1,    61,    -1,    63,
      -1,    -1,    66,    67,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    13,    -1,    15,    16,    17,    18,
      -1,    -1,    21,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    -1,    -1,    32,    -1,    -1,    35,    -1,    -1,    -1,
      -1,    -1,    -1,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    -1,    -1,    -1,
      59,    -1,    61,    -1,    63,    -1,    -1,    66,    67,     3,
       4,     5,    -1,    -1,    -1,     9,    10,    11,    12,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,
      -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    35,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    -1,    -1,    -1,    -1,    60,    -1,    -1,    63,
      64,    65,    -1,    67,     3,     4,     5,    -1,    -1,    -1,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,
      19,    -1,    21,    22,    -1,    -1,    -1,    26,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    35,    -1,    -1,    -1,
      -1,    -1,    -1,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    -1,    -1,    -1,
      -1,    60,    -1,    -1,    63,     3,     4,     5,    67,    -1,
      -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    26,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,    -1,    -1,
      38,    -1,    -1,    -1,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    -1,    -1,
      -1,    -1,    60,    -1,    -1,    63,     3,     4,     5,    67,
      -1,    -1,     9,    10,    11,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    23,    -1,    -1,    26,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,    -1,
      -1,    -1,    -1,    -1,    -1,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    -1,
      -1,    -1,    -1,    60,    -1,    -1,    63,     3,     4,     5,
      67,    -1,    -1,     9,    10,    11,    12,    13,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,
      26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,
      -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      -1,    -1,    -1,    -1,    60,    -1,    -1,    63,     3,     4,
       5,    67,    -1,    -1,     9,    10,    11,    12,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,
      -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      35,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    -1,    -1,    -1,    -1,    60,    -1,    -1,    63,     3,
       4,     5,    67,    -1,    -1,     9,    10,    11,    12,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    35,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    -1,    -1,    -1,    -1,    60,    -1,    -1,    63,
      -1,    -1,    -1,    67,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    15,    16,    17,    18,
      -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    -1,    -1,    32,    33,    34,    35,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
      -1,    -1,    -1,     9,    10,    11,    12,    -1,    -1,    -1,
      59,    -1,    61,    -1,    63,    21,    22,    66,    67,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    35,
      -1,    15,    16,    17,    18,    -1,    -1,    -1,    22,    -1,
      -1,    -1,    -1,    27,    28,    29,    -1,    -1,    32,    55,
      -1,    35,    -1,    -1,    60,    -1,    40,    63,    -1,    -1,
      -1,    67,     3,     4,     5,    -1,    -1,    -1,     9,    10,
      11,    12,    -1,    -1,    -1,    59,    -1,    61,    -1,    63,
      21,    22,    66,    67,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    35,    -1,    15,    16,    17,    18,
      -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    -1,    -1,    32,    -1,    -1,    35,    -1,    -1,    60,
      -1,    -1,    63,    -1,    -1,    -1,    67,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      59,    -1,    61,    62,    63,    -1,    -1,    66,    67,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      14,    15,    16,    17,    18,    -1,    -1,    -1,    22,    -1,
      -1,    -1,    -1,    27,    28,    29,    -1,    -1,    32,    -1,
      -1,    35,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    59,    -1,    61,    -1,    63,
      -1,    -1,    66,    67,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    -1,    -1,    15,    16,    17,    18,
      -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,    27,    28,
      29,    -1,    -1,    32,    -1,    -1,    35,     3,     4,     5,
      -1,    -1,    -1,     9,    10,    11,    12,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    -1,    -1,
      59,    -1,    61,    -1,    63,    64,    -1,    66,    67,    35,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      -1,    -1,    -1,    -1,    60,    -1,    -1,    63,    -1,    -1,
      -1,    67,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    15,    16,    17,    18,    -1,    -1,
      -1,    22,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,
      -1,    32,    -1,    -1,    35,    -1,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,    -1,
      61,    -1,    63,    -1,    -1,    66,    67,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    -1,    15,
      16,    17,    18,    -1,    -1,    -1,    22,    -1,    -1,    -1,
      -1,    27,    28,    29,    -1,    -1,    32,    -1,    -1,    35,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    59,    -1,    61,    -1,    63,    -1,    -1,
      66,    67,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    15,    16,    17,    18,    -1,    -1,
      -1,    22,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,
      -1,    32,    -1,    -1,    35,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,    -1,
      61,    -1,    63,    -1,    -1,    66,    67,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    -1,    15,
      16,    17,    18,    -1,    -1,    -1,    22,    -1,    -1,    -1,
      -1,    27,    28,    29,    -1,    -1,    32,    -1,    -1,    35,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    59,    -1,    61,    -1,    63,    -1,    -1,
      66,    67,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    15,    16,    17,    18,    -1,    -1,
      -1,    22,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,
      -1,    32,    -1,    -1,    35,     3,     4,     5,    -1,    -1,
      -1,     9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,    -1,    -1,    -1,    59,    -1,
      61,    -1,    63,    -1,    -1,    66,    67,    35,    -1,    -1,
       3,     4,     5,    -1,    -1,    -1,     9,    10,    11,    12,
      -1,    -1,    -1,    -1,    52,    53,    54,    55,    21,    22,
      -1,    -1,    60,    -1,    -1,    63,    -1,    -1,    -1,    67,
      -1,    -1,    35,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    54,    55,    -1,    -1,    -1,    -1,    60,    -1,    -1,
      63,    -1,    -1,    -1,    67
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     8,     9,    10,    11,    12,
      15,    16,    17,    18,    22,    27,    28,    29,    32,    35,
      59,    61,    63,    66,    67,    73,    74,    75,    77,    78,
      79,    82,    83,    85,    87,    90,    93,    30,    74,    74,
       5,    22,    33,    34,    63,    74,    81,    84,    86,     5,
      22,    63,    80,    81,    74,    74,     5,     7,     5,     7,
       5,    94,    91,    74,     5,     5,    13,    17,    21,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    74,    76,    77,    86,     5,    63,    77,
      80,    40,    68,    86,     0,     5,    13,    21,    26,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    60,    63,    75,    56,    58,    23,     5,
      21,    62,    62,    62,     5,    81,    86,     5,    86,    62,
      62,    65,    20,    86,    20,    81,    55,    62,    19,    62,
       5,    62,    36,    37,    39,    92,    62,    64,    22,    80,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    58,    64,    64,
      65,    58,    20,    40,    86,    65,    68,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    62,    74,    74,    17,     5,    74,    74,
       5,    74,    14,    74,    74,    74,    62,    62,    62,    64,
      64,    74,    74,    74,    77,    64,    77,    55,    62,    63,
      74,    40,    89,     5,    22,    40,    63,    95,    96,    97,
      98,    97,    74,    74,    20,    20,    64,    77,    68,    40,
      65,    68,    74,    22,    80,    17,    74,    74,    74,    62,
      58,    58,    63,    74,    97,    74,    88,    40,    55,    56,
      62,    71,    98,    97,    20,    65,    20,    40,    65,    38,
      23,    77,    77,    58,    68,    40,    20,    20,    95,    97,
      74,    79,    97,    64,    69,    20,    70,    88,    98,     5,
       3,    98,    64,    97,    98,    97,    98,    98,    74,    64,
      64,    68,    77,    77,    64,    74,    74,    20,    20,    64,
      64,    74,    74
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    72,    73,    73,    73,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    76,
      77,    77,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    79,
      79,    79,    79,    79,    79,    80,    80,    80,    80,    80,
      80,    80,    80,    81,    81,    81,    82,    82,    82,    83,
      83,    83,    84,    84,    85,    85,    85,    86,    86,    87,
      88,    88,    89,    89,    89,    90,    91,    91,    92,    92,
      93,    93,    93,    94,    94,    95,    95,    96,    96,    97,
      97,    97,    97,    97,    98,    98,    98,    98,    98,    98,
      98
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     2,     3,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     1,     2,
       2,     3,     1,     2,     6,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     6,
       6,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     1,
       1,     3,     4,     4,     5,     6,     4,     5,     4,     5,
       4,     3,     1,     6,     6,     2,     2,     2,     2,     5,
       5,     6,     6,     5,     3,     1,     3,     2,     4,     1,
       5,     2,     6,     1,     3,     1,     2,     3,     4,     4,
       5,     6,     3,     3,     3,     3,     4,     1,     3,     4,
       1,     3,     4,     5,     5,     3,     0,     2,     1,     3,
       4,     2,     4,     1,     2,     3,     3,     3,     3,     1,
       2,     3,     1,     1,     1,     3,     3,     3,     3,     1,
       3
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
#line 124 "lang/parser.y"
                      { parse_stmt_list(ast_root, (yyvsp[-1].ast_node_ptr)); }
#line 1855 "lang/y.tab.c"
    break;

  case 3: /* program: expr_sequence  */
#line 125 "lang/parser.y"
                      { parse_stmt_list(ast_root, (yyvsp[0].ast_node_ptr)); }
#line 1861 "lang/y.tab.c"
    break;

  case 6: /* expr: "yield" expr  */
#line 132 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_yield((yyvsp[0].ast_node_ptr)); }
#line 1867 "lang/y.tab.c"
    break;

  case 7: /* expr: expr DOUBLE_AT expr  */
#line 133 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1873 "lang/y.tab.c"
    break;

  case 8: /* expr: expr simple_expr  */
#line 134 "lang/parser.y"
                                       { (yyval.ast_node_ptr) = ast_application((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1879 "lang/y.tab.c"
    break;

  case 9: /* expr: expr '+' expr  */
#line 135 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1885 "lang/y.tab.c"
    break;

  case 10: /* expr: expr '-' expr  */
#line 136 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1891 "lang/y.tab.c"
    break;

  case 11: /* expr: expr '*' expr  */
#line 137 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1897 "lang/y.tab.c"
    break;

  case 12: /* expr: expr '/' expr  */
#line 138 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1903 "lang/y.tab.c"
    break;

  case 13: /* expr: expr MODULO expr  */
#line 139 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1909 "lang/y.tab.c"
    break;

  case 14: /* expr: expr '<' expr  */
#line 140 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1915 "lang/y.tab.c"
    break;

  case 15: /* expr: expr '>' expr  */
#line 141 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1921 "lang/y.tab.c"
    break;

  case 16: /* expr: expr DOUBLE_AMP expr  */
#line 142 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_AMP, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1927 "lang/y.tab.c"
    break;

  case 17: /* expr: expr DOUBLE_PIPE expr  */
#line 143 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_PIPE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1933 "lang/y.tab.c"
    break;

  case 18: /* expr: expr GE expr  */
#line 144 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1939 "lang/y.tab.c"
    break;

  case 19: /* expr: expr LE expr  */
#line 145 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1945 "lang/y.tab.c"
    break;

  case 20: /* expr: expr NE expr  */
#line 146 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1951 "lang/y.tab.c"
    break;

  case 21: /* expr: expr EQ expr  */
#line 147 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1957 "lang/y.tab.c"
    break;

  case 22: /* expr: expr PIPE expr  */
#line 148 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[0].ast_node_ptr), (yyvsp[-2].ast_node_ptr)); }
#line 1963 "lang/y.tab.c"
    break;

  case 23: /* expr: expr ':' expr  */
#line 149 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_assoc((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1969 "lang/y.tab.c"
    break;

  case 24: /* expr: expr "to" expr  */
#line 150 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_range_expression((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1975 "lang/y.tab.c"
    break;

  case 25: /* expr: expr DOUBLE_COLON expr  */
#line 151 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1981 "lang/y.tab.c"
    break;

  case 26: /* expr: let_binding  */
#line 152 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 1987 "lang/y.tab.c"
    break;

  case 27: /* expr: match_expr  */
#line 153 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 1993 "lang/y.tab.c"
    break;

  case 28: /* expr: type_decl  */
#line 154 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 1999 "lang/y.tab.c"
    break;

  case 29: /* expr: THUNK expr  */
#line 155 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_thunk_expr((yyvsp[0].ast_node_ptr)); }
#line 2005 "lang/y.tab.c"
    break;

  case 30: /* expr: TRIPLE_DOT expr  */
#line 156 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_spread_operator((yyvsp[0].ast_node_ptr)); }
#line 2011 "lang/y.tab.c"
    break;

  case 31: /* expr: IDENTIFIER IMPLEMENTS IDENTIFIER  */
#line 157 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_implements((yyvsp[-2].vident), (yyvsp[0].vident)); }
#line 2017 "lang/y.tab.c"
    break;

  case 32: /* expr: IDENTIFIER_LIST  */
#line 158 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_typed_empty_list((yyvsp[0].vident)); }
#line 2023 "lang/y.tab.c"
    break;

  case 33: /* expr: MACRO_IDENTIFIER expr  */
#line 159 "lang/parser.y"
                                      {
                                        // TODO: not doing anything with macros yet - do we want to??
                                        printf("macro '%s'\n", (yyvsp[-1].vident).chars);
                                        (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr);
                                      }
#line 2033 "lang/y.tab.c"
    break;

  case 34: /* expr: "for" IDENTIFIER '=' expr IN expr  */
#line 164 "lang/parser.y"
                                        {
                                          Ast *let = ast_let(ast_identifier((yyvsp[-4].vident)), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));
                                          let->tag = AST_LOOP;
                                          (yyval.ast_node_ptr) = let;

                                      }
#line 2044 "lang/y.tab.c"
    break;

  case 35: /* expr: expr ':' '=' expr  */
#line 173 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_assignment((yyvsp[-3].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2050 "lang/y.tab.c"
    break;

  case 36: /* simple_expr: INTEGER  */
#line 177 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[0].vint)); }
#line 2056 "lang/y.tab.c"
    break;

  case 37: /* simple_expr: DOUBLE  */
#line 178 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[0].vdouble)); }
#line 2062 "lang/y.tab.c"
    break;

  case 38: /* simple_expr: TOK_STRING  */
#line 179 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_string((yyvsp[0].vstr)); }
#line 2068 "lang/y.tab.c"
    break;

  case 39: /* simple_expr: TRUE  */
#line 180 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); }
#line 2074 "lang/y.tab.c"
    break;

  case 40: /* simple_expr: FALSE  */
#line 181 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); }
#line 2080 "lang/y.tab.c"
    break;

  case 41: /* simple_expr: IDENTIFIER  */
#line 182 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2086 "lang/y.tab.c"
    break;

  case 42: /* simple_expr: TOK_VOID  */
#line 183 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_void(); }
#line 2092 "lang/y.tab.c"
    break;

  case 43: /* simple_expr: list  */
#line 184 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2098 "lang/y.tab.c"
    break;

  case 44: /* simple_expr: array  */
#line 185 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2104 "lang/y.tab.c"
    break;

  case 45: /* simple_expr: tuple  */
#line 186 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2110 "lang/y.tab.c"
    break;

  case 46: /* simple_expr: fstring  */
#line 187 "lang/parser.y"
                          { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[0].ast_node_ptr)); }
#line 2116 "lang/y.tab.c"
    break;

  case 47: /* simple_expr: TOK_CHAR  */
#line 188 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_char((yyvsp[0].vchar)); }
#line 2122 "lang/y.tab.c"
    break;

  case 48: /* simple_expr: '(' expr_sequence ')'  */
#line 189 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2128 "lang/y.tab.c"
    break;

  case 49: /* simple_expr: '(' FN lambda_args ARROW expr_sequence ')'  */
#line 190 "lang/parser.y"
                                               { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2134 "lang/y.tab.c"
    break;

  case 50: /* simple_expr: '(' FN TOK_VOID ARROW expr_sequence ')'  */
#line 191 "lang/parser.y"
                                               { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); }
#line 2140 "lang/y.tab.c"
    break;

  case 51: /* simple_expr: '(' '+' ')'  */
#line 192 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"+", 1}); }
#line 2146 "lang/y.tab.c"
    break;

  case 52: /* simple_expr: '(' '-' ')'  */
#line 193 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"-", 1}); }
#line 2152 "lang/y.tab.c"
    break;

  case 53: /* simple_expr: '(' '*' ')'  */
#line 194 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"*", 1}); }
#line 2158 "lang/y.tab.c"
    break;

  case 54: /* simple_expr: '(' '/' ')'  */
#line 195 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"/", 1}); }
#line 2164 "lang/y.tab.c"
    break;

  case 55: /* simple_expr: '(' MODULO ')'  */
#line 196 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"%", 1}); }
#line 2170 "lang/y.tab.c"
    break;

  case 56: /* simple_expr: '(' '<' ')'  */
#line 197 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<", 1}); }
#line 2176 "lang/y.tab.c"
    break;

  case 57: /* simple_expr: '(' '>' ')'  */
#line 198 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){">", 1}); }
#line 2182 "lang/y.tab.c"
    break;

  case 58: /* simple_expr: '(' DOUBLE_AMP ')'  */
#line 199 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"&&", 2}); }
#line 2188 "lang/y.tab.c"
    break;

  case 59: /* simple_expr: '(' DOUBLE_PIPE ')'  */
#line 200 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"||", 2}); }
#line 2194 "lang/y.tab.c"
    break;

  case 60: /* simple_expr: '(' GE ')'  */
#line 201 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){">=", 2}); }
#line 2200 "lang/y.tab.c"
    break;

  case 61: /* simple_expr: '(' LE ')'  */
#line 202 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<=", 2}); }
#line 2206 "lang/y.tab.c"
    break;

  case 62: /* simple_expr: '(' NE ')'  */
#line 203 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"!=", 2}); }
#line 2212 "lang/y.tab.c"
    break;

  case 63: /* simple_expr: '(' EQ ')'  */
#line 204 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"==", 2}); }
#line 2218 "lang/y.tab.c"
    break;

  case 64: /* simple_expr: '(' PIPE ')'  */
#line 205 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"|", 1}); }
#line 2224 "lang/y.tab.c"
    break;

  case 65: /* simple_expr: '(' ':' ')'  */
#line 206 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){":", 1}); }
#line 2230 "lang/y.tab.c"
    break;

  case 66: /* simple_expr: '(' DOUBLE_COLON ')'  */
#line 207 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"::", 2}); }
#line 2236 "lang/y.tab.c"
    break;

  case 67: /* simple_expr: '(' custom_binop ')'  */
#line 208 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2242 "lang/y.tab.c"
    break;

  case 68: /* simple_expr: simple_expr '.' IDENTIFIER  */
#line 209 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_record_access((yyvsp[-2].ast_node_ptr), ast_identifier((yyvsp[0].vident))); }
#line 2248 "lang/y.tab.c"
    break;

  case 69: /* custom_binop: IDENTIFIER  */
#line 214 "lang/parser.y"
    {
      // Check if the identifier is in the custom_binops list
      bool found = false;
      custom_binops_t* current = __custom_binops;
      while (current != NULL) {
        if (strcmp(current->binop, (yyvsp[0].vident).chars) == 0) {
          found = true;
          break;
        }
        current = current->next;
      }
      
      if (!found) {
        yyerror("Invalid operator in section syntax");
        YYERROR;
      }
      
      (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident));
    }
#line 2272 "lang/y.tab.c"
    break;

  case 70: /* expr_sequence: expr  */
#line 236 "lang/parser.y"
                                { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2278 "lang/y.tab.c"
    break;

  case 71: /* expr_sequence: expr_sequence ';' expr  */
#line 237 "lang/parser.y"
                                { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2284 "lang/y.tab.c"
    break;

  case 72: /* let_binding: LET TEST_ID '=' expr  */
#line 241 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_test_module((yyvsp[0].ast_node_ptr));}
#line 2290 "lang/y.tab.c"
    break;

  case 73: /* let_binding: LET IDENTIFIER '=' expr  */
#line 242 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL); }
#line 2296 "lang/y.tab.c"
    break;

  case 74: /* let_binding: LET MUT IDENTIFIER '=' expr  */
#line 243 "lang/parser.y"
                                    { Ast *let = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2305 "lang/y.tab.c"
    break;

  case 75: /* let_binding: LET IDENTIFIER '=' EXTERN FN fn_signature  */
#line 248 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-4].vident)), ast_extern_fn((yyvsp[-4].vident), (yyvsp[0].ast_node_ptr)), NULL); }
#line 2311 "lang/y.tab.c"
    break;

  case 76: /* let_binding: LET lambda_arg '=' expr  */
#line 250 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); }
#line 2317 "lang/y.tab.c"
    break;

  case 77: /* let_binding: LET MUT lambda_arg '=' expr  */
#line 252 "lang/parser.y"
                                    { Ast *let = ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2326 "lang/y.tab.c"
    break;

  case 78: /* let_binding: LET expr_list '=' expr  */
#line 256 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_tuple((yyvsp[-2].ast_node_ptr)), (yyvsp[0].ast_node_ptr), NULL);}
#line 2332 "lang/y.tab.c"
    break;

  case 79: /* let_binding: LET MUT expr_list '=' expr  */
#line 258 "lang/parser.y"
                                    { Ast *let = ast_let(ast_tuple((yyvsp[-2].ast_node_ptr)), (yyvsp[0].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2341 "lang/y.tab.c"
    break;

  case 80: /* let_binding: LET TOK_VOID '=' expr  */
#line 274 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2347 "lang/y.tab.c"
    break;

  case 81: /* let_binding: let_binding IN expr  */
#line 275 "lang/parser.y"
                                    {
                                      Ast *let = (yyvsp[-2].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[0].ast_node_ptr);
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2357 "lang/y.tab.c"
    break;

  case 82: /* let_binding: lambda_expr  */
#line 280 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2363 "lang/y.tab.c"
    break;

  case 83: /* let_binding: LET '(' IDENTIFIER ')' '=' lambda_expr  */
#line 283 "lang/parser.y"
                                    {
                                      Ast *id = ast_identifier((yyvsp[-3].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[0].ast_node_ptr), NULL);
                                    }
#line 2373 "lang/y.tab.c"
    break;

  case 84: /* let_binding: LET '(' IDENTIFIER ')' '=' expr  */
#line 290 "lang/parser.y"
                                    {
                                      Ast *id = ast_identifier((yyvsp[-3].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[0].ast_node_ptr), NULL);
                                    }
#line 2383 "lang/y.tab.c"
    break;

  case 85: /* let_binding: IMPORT PATH_IDENTIFIER  */
#line 299 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), false); }
#line 2389 "lang/y.tab.c"
    break;

  case 86: /* let_binding: OPEN PATH_IDENTIFIER  */
#line 300 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), true); }
#line 2395 "lang/y.tab.c"
    break;

  case 87: /* let_binding: IMPORT IDENTIFIER  */
#line 301 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), false); }
#line 2401 "lang/y.tab.c"
    break;

  case 88: /* let_binding: OPEN IDENTIFIER  */
#line 302 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), true); }
#line 2407 "lang/y.tab.c"
    break;

  case 89: /* lambda_expr: FN lambda_args ARROW expr_sequence ';'  */
#line 323 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2413 "lang/y.tab.c"
    break;

  case 90: /* lambda_expr: FN TOK_VOID ARROW expr_sequence ';'  */
#line 324 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); }
#line 2419 "lang/y.tab.c"
    break;

  case 91: /* lambda_expr: '(' FN lambda_args ARROW expr_sequence ')'  */
#line 325 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2425 "lang/y.tab.c"
    break;

  case 92: /* lambda_expr: '(' FN TOK_VOID ARROW expr_sequence ')'  */
#line 326 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); }
#line 2431 "lang/y.tab.c"
    break;

  case 93: /* lambda_expr: "module" lambda_args ARROW expr_sequence ';'  */
#line 327 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_module(ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr))); }
#line 2437 "lang/y.tab.c"
    break;

  case 94: /* lambda_expr: "module" expr_sequence ';'  */
#line 328 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_module(ast_lambda(NULL, (yyvsp[-1].ast_node_ptr))); }
#line 2443 "lang/y.tab.c"
    break;

  case 95: /* lambda_args: lambda_arg  */
#line 335 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[0].ast_node_ptr), NULL); }
#line 2449 "lang/y.tab.c"
    break;

  case 96: /* lambda_args: lambda_arg '=' expr  */
#line 336 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2455 "lang/y.tab.c"
    break;

  case 97: /* lambda_args: lambda_args lambda_arg  */
#line 337 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); }
#line 2461 "lang/y.tab.c"
    break;

  case 98: /* lambda_args: lambda_args lambda_arg '=' expr  */
#line 338 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-3].ast_node_ptr), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2467 "lang/y.tab.c"
    break;

  case 99: /* lambda_args: lambda_arg  */
#line 340 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[0].ast_node_ptr), NULL); }
#line 2473 "lang/y.tab.c"
    break;

  case 100: /* lambda_args: lambda_arg ':' '(' type_expr ')'  */
#line 341 "lang/parser.y"
                                     { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[-4].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2479 "lang/y.tab.c"
    break;

  case 101: /* lambda_args: lambda_args lambda_arg  */
#line 342 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); }
#line 2485 "lang/y.tab.c"
    break;

  case 102: /* lambda_args: lambda_args lambda_arg ':' '(' type_expr ')'  */
#line 343 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-5].ast_node_ptr), (yyvsp[-4].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2491 "lang/y.tab.c"
    break;

  case 103: /* lambda_arg: IDENTIFIER  */
#line 347 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2497 "lang/y.tab.c"
    break;

  case 104: /* lambda_arg: '(' expr_list ')'  */
#line 348 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-1].ast_node_ptr)); }
#line 2503 "lang/y.tab.c"
    break;

  case 105: /* lambda_arg: list_match_expr  */
#line 349 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2509 "lang/y.tab.c"
    break;

  case 106: /* list: '[' ']'  */
#line 359 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_empty_list(); }
#line 2515 "lang/y.tab.c"
    break;

  case 107: /* list: '[' expr_list ']'  */
#line 360 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2521 "lang/y.tab.c"
    break;

  case 108: /* list: '[' expr_list ',' ']'  */
#line 361 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[-2].ast_node_ptr); }
#line 2527 "lang/y.tab.c"
    break;

  case 109: /* array: '[' '|' '|' ']'  */
#line 365 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_empty_array(); }
#line 2533 "lang/y.tab.c"
    break;

  case 110: /* array: '[' '|' expr_list '|' ']'  */
#line 366 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[-2].ast_node_ptr)); }
#line 2539 "lang/y.tab.c"
    break;

  case 111: /* array: '[' '|' expr_list ',' '|' ']'  */
#line 367 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[-3].ast_node_ptr)); }
#line 2545 "lang/y.tab.c"
    break;

  case 112: /* list_match_expr: IDENTIFIER DOUBLE_COLON IDENTIFIER  */
#line 371 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[-2].vident)), ast_identifier((yyvsp[0].vident))); }
#line 2551 "lang/y.tab.c"
    break;

  case 113: /* list_match_expr: IDENTIFIER DOUBLE_COLON expr  */
#line 372 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2557 "lang/y.tab.c"
    break;

  case 114: /* tuple: '(' expr ')'  */
#line 376 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2563 "lang/y.tab.c"
    break;

  case 115: /* tuple: '(' expr_list ')'  */
#line 377 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-1].ast_node_ptr)); }
#line 2569 "lang/y.tab.c"
    break;

  case 116: /* tuple: '(' expr_list ',' ')'  */
#line 378 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-2].ast_node_ptr)); }
#line 2575 "lang/y.tab.c"
    break;

  case 117: /* expr_list: expr  */
#line 382 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_list((yyvsp[0].ast_node_ptr)); }
#line 2581 "lang/y.tab.c"
    break;

  case 118: /* expr_list: expr_list ',' expr  */
#line 383 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2587 "lang/y.tab.c"
    break;

  case 119: /* match_expr: MATCH expr WITH match_branches  */
#line 387 "lang/parser.y"
                                   { (yyval.ast_node_ptr) = ast_match((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2593 "lang/y.tab.c"
    break;

  case 120: /* match_test_clause: expr  */
#line 391 "lang/parser.y"
         {(yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr);}
#line 2599 "lang/y.tab.c"
    break;

  case 121: /* match_test_clause: expr "if" expr  */
#line 392 "lang/parser.y"
                   { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2605 "lang/y.tab.c"
    break;

  case 122: /* match_branches: '|' match_test_clause ARROW expr  */
#line 395 "lang/parser.y"
                                                     {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2611 "lang/y.tab.c"
    break;

  case 123: /* match_branches: match_branches '|' match_test_clause ARROW expr  */
#line 396 "lang/parser.y"
                                                     {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2617 "lang/y.tab.c"
    break;

  case 124: /* match_branches: match_branches '|' '_' ARROW expr  */
#line 397 "lang/parser.y"
                                        {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[-4].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[0].ast_node_ptr));}
#line 2623 "lang/y.tab.c"
    break;

  case 125: /* fstring: FSTRING_START fstring_parts FSTRING_END  */
#line 400 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2629 "lang/y.tab.c"
    break;

  case 126: /* fstring_parts: %empty  */
#line 404 "lang/parser.y"
                                { (yyval.ast_node_ptr) = ast_empty_list(); }
#line 2635 "lang/y.tab.c"
    break;

  case 127: /* fstring_parts: fstring_parts fstring_part  */
#line 405 "lang/parser.y"
                                { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2641 "lang/y.tab.c"
    break;

  case 128: /* fstring_part: FSTRING_TEXT  */
#line 409 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_string((yyvsp[0].vstr)); }
#line 2647 "lang/y.tab.c"
    break;

  case 129: /* fstring_part: FSTRING_INTERP_START expr FSTRING_INTERP_END  */
#line 410 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2653 "lang/y.tab.c"
    break;

  case 130: /* type_decl: TYPE IDENTIFIER '=' type_expr  */
#line 414 "lang/parser.y"
                                  {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
#line 2663 "lang/y.tab.c"
    break;

  case 131: /* type_decl: TYPE IDENTIFIER  */
#line 420 "lang/parser.y"
                                 {
                                      Ast *type_decl = ast_let(ast_identifier((yyvsp[0].vident)), NULL, NULL);
                                      type_decl->tag = AST_TYPE_DECL;
                                      (yyval.ast_node_ptr) = type_decl;
                                   }
#line 2673 "lang/y.tab.c"
    break;

  case 132: /* type_decl: TYPE type_args '=' type_expr  */
#line 426 "lang/parser.y"
                                 {
                                    Ast *args = (yyvsp[-2].ast_node_ptr);
                                    Ast *name = args->data.AST_LAMBDA.params;
                                    args->data.AST_LAMBDA.params = args->data.AST_LAMBDA.params + 1;
                                    args->data.AST_LAMBDA.len--;
                                    args->data.AST_LAMBDA.body = (yyvsp[0].ast_node_ptr);
                                    Ast *type_decl = ast_let(name, args, NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
#line 2688 "lang/y.tab.c"
    break;

  case 133: /* type_args: IDENTIFIER  */
#line 439 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list(ast_identifier((yyvsp[0].vident)), NULL); }
#line 2694 "lang/y.tab.c"
    break;

  case 134: /* type_args: type_args IDENTIFIER  */
#line 440 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), ast_identifier((yyvsp[0].vident)), NULL); }
#line 2700 "lang/y.tab.c"
    break;

  case 135: /* fn_signature: type_expr ARROW type_expr  */
#line 443 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2706 "lang/y.tab.c"
    break;

  case 136: /* fn_signature: fn_signature ARROW type_expr  */
#line 444 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2712 "lang/y.tab.c"
    break;

  case 137: /* tuple_type: type_atom ',' type_atom  */
#line 448 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2718 "lang/y.tab.c"
    break;

  case 138: /* tuple_type: tuple_type ',' type_atom  */
#line 449 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2724 "lang/y.tab.c"
    break;

  case 139: /* type_expr: type_atom  */
#line 453 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2730 "lang/y.tab.c"
    break;

  case 140: /* type_expr: '|' type_atom  */
#line 454 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_list((yyvsp[0].ast_node_ptr)); }
#line 2736 "lang/y.tab.c"
    break;

  case 141: /* type_expr: type_expr '|' type_atom  */
#line 455 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2742 "lang/y.tab.c"
    break;

  case 142: /* type_expr: fn_signature  */
#line 456 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_fn_signature_of_list((yyvsp[0].ast_node_ptr)); }
#line 2748 "lang/y.tab.c"
    break;

  case 143: /* type_expr: tuple_type  */
#line 457 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2754 "lang/y.tab.c"
    break;

  case 144: /* type_atom: IDENTIFIER  */
#line 461 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2760 "lang/y.tab.c"
    break;

  case 145: /* type_atom: IDENTIFIER '=' INTEGER  */
#line 462 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-2].vident)), AST_CONST(AST_INT, (yyvsp[0].vint)), NULL); }
#line 2766 "lang/y.tab.c"
    break;

  case 146: /* type_atom: IDENTIFIER "of" type_atom  */
#line 463 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2772 "lang/y.tab.c"
    break;

  case 147: /* type_atom: IDENTIFIER ':' type_atom  */
#line 464 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_assoc(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2778 "lang/y.tab.c"
    break;

  case 148: /* type_atom: '(' type_expr ')'  */
#line 465 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2784 "lang/y.tab.c"
    break;

  case 149: /* type_atom: TOK_VOID  */
#line 466 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_void(); }
#line 2790 "lang/y.tab.c"
    break;

  case 150: /* type_atom: IDENTIFIER '.' IDENTIFIER  */
#line 467 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_record_access(ast_identifier((yyvsp[-2].vident)), ast_identifier((yyvsp[0].vident))); }
#line 2796 "lang/y.tab.c"
    break;


#line 2800 "lang/y.tab.c"

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

#line 469 "lang/parser.y"



void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, _cur_script);
}
#endif _LANG_TAB_Hparse
