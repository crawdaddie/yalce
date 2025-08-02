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
  YYSYMBOL_THEN = 35,                      /* THEN  */
  YYSYMBOL_ELSE = 36,                      /* ELSE  */
  YYSYMBOL_FSTRING_START = 37,             /* FSTRING_START  */
  YYSYMBOL_FSTRING_END = 38,               /* FSTRING_END  */
  YYSYMBOL_FSTRING_INTERP_START = 39,      /* FSTRING_INTERP_START  */
  YYSYMBOL_FSTRING_INTERP_END = 40,        /* FSTRING_INTERP_END  */
  YYSYMBOL_FSTRING_TEXT = 41,              /* FSTRING_TEXT  */
  YYSYMBOL_42_ = 42,                       /* '|'  */
  YYSYMBOL_APPLICATION = 43,               /* APPLICATION  */
  YYSYMBOL_DOUBLE_AMP = 44,                /* DOUBLE_AMP  */
  YYSYMBOL_DOUBLE_PIPE = 45,               /* DOUBLE_PIPE  */
  YYSYMBOL_GE = 46,                        /* GE  */
  YYSYMBOL_LE = 47,                        /* LE  */
  YYSYMBOL_EQ = 48,                        /* EQ  */
  YYSYMBOL_NE = 49,                        /* NE  */
  YYSYMBOL_50_ = 50,                       /* '>'  */
  YYSYMBOL_51_ = 51,                       /* '<'  */
  YYSYMBOL_52_ = 52,                       /* '+'  */
  YYSYMBOL_53_ = 53,                       /* '-'  */
  YYSYMBOL_54_ = 54,                       /* '*'  */
  YYSYMBOL_55_ = 55,                       /* '/'  */
  YYSYMBOL_MODULO = 56,                    /* MODULO  */
  YYSYMBOL_57_ = 57,                       /* ':'  */
  YYSYMBOL_58_ = 58,                       /* '.'  */
  YYSYMBOL_UMINUS = 59,                    /* UMINUS  */
  YYSYMBOL_60_ = 60,                       /* ';'  */
  YYSYMBOL_61_yield_ = 61,                 /* "yield"  */
  YYSYMBOL_62_to_ = 62,                    /* "to"  */
  YYSYMBOL_63_for_ = 63,                   /* "for"  */
  YYSYMBOL_64_ = 64,                       /* '='  */
  YYSYMBOL_65_ = 65,                       /* '('  */
  YYSYMBOL_66_ = 66,                       /* ')'  */
  YYSYMBOL_67_module_ = 67,                /* "module"  */
  YYSYMBOL_68_ = 68,                       /* '['  */
  YYSYMBOL_69_ = 69,                       /* ']'  */
  YYSYMBOL_70_ = 70,                       /* ','  */
  YYSYMBOL_71_if_ = 71,                    /* "if"  */
  YYSYMBOL_72___ = 72,                     /* '_'  */
  YYSYMBOL_73_of_ = 73,                    /* "of"  */
  YYSYMBOL_YYACCEPT = 74,                  /* $accept  */
  YYSYMBOL_program = 75,                   /* program  */
  YYSYMBOL_expr = 76,                      /* expr  */
  YYSYMBOL_simple_expr = 77,               /* simple_expr  */
  YYSYMBOL_custom_binop = 78,              /* custom_binop  */
  YYSYMBOL_expr_sequence = 79,             /* expr_sequence  */
  YYSYMBOL_let_binding = 80,               /* let_binding  */
  YYSYMBOL_lambda_expr = 81,               /* lambda_expr  */
  YYSYMBOL_lambda_args = 82,               /* lambda_args  */
  YYSYMBOL_lambda_arg = 83,                /* lambda_arg  */
  YYSYMBOL_list = 84,                      /* list  */
  YYSYMBOL_array = 85,                     /* array  */
  YYSYMBOL_list_match_expr = 86,           /* list_match_expr  */
  YYSYMBOL_tuple = 87,                     /* tuple  */
  YYSYMBOL_expr_list = 88,                 /* expr_list  */
  YYSYMBOL_match_expr = 89,                /* match_expr  */
  YYSYMBOL_match_test_clause = 90,         /* match_test_clause  */
  YYSYMBOL_match_branches = 91,            /* match_branches  */
  YYSYMBOL_fstring = 92,                   /* fstring  */
  YYSYMBOL_fstring_parts = 93,             /* fstring_parts  */
  YYSYMBOL_fstring_part = 94,              /* fstring_part  */
  YYSYMBOL_type_decl = 95,                 /* type_decl  */
  YYSYMBOL_type_args = 96,                 /* type_args  */
  YYSYMBOL_fn_signature = 97,              /* fn_signature  */
  YYSYMBOL_tuple_type = 98,                /* tuple_type  */
  YYSYMBOL_type_expr = 99,                 /* type_expr  */
  YYSYMBOL_type_atom = 100                 /* type_atom  */
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
#define YYFINAL  95
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2243

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  74
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  151
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  327

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   311


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
      65,    66,    54,    52,    70,    53,    58,    55,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    57,    60,
      51,    64,    50,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    68,     2,    69,     2,    72,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    42,     2,     2,     2,     2,     2,
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
      46,    47,    48,    49,    56,    59,    61,    62,    63,    67,
      71,    73
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
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
  "IMPLEMENTS", "AMPERSAND", "TYPE", "TEST_ID", "MUT", "THEN", "ELSE",
  "FSTRING_START", "FSTRING_END", "FSTRING_INTERP_START",
  "FSTRING_INTERP_END", "FSTRING_TEXT", "'|'", "APPLICATION", "DOUBLE_AMP",
  "DOUBLE_PIPE", "GE", "LE", "EQ", "NE", "'>'", "'<'", "'+'", "'-'", "'*'",
  "'/'", "MODULO", "':'", "'.'", "UMINUS", "';'", "\"yield\"", "\"to\"",
  "\"for\"", "'='", "'('", "')'", "\"module\"", "'['", "']'", "','",
  "\"if\"", "'_'", "\"of\"", "$accept", "program", "expr", "simple_expr",
  "custom_binop", "expr_sequence", "let_binding", "lambda_expr",
  "lambda_args", "lambda_arg", "list", "array", "list_match_expr", "tuple",
  "expr_list", "match_expr", "match_test_clause", "match_branches",
  "fstring", "fstring_parts", "fstring_part", "type_decl", "type_args",
  "fn_signature", "tuple_type", "type_expr", "type_atom", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-71)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-117)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
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

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
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
      57,    58,    59,    60,    62,    61,    56,    55,    50,    51,
      52,    53,    54,    64,   113,    66,     0,    47,   114,     0,
      93,     0,     0,     0,   106,     0,     0,    22,    25,     7,
      16,    17,    18,    19,    21,    20,    15,    14,     9,    10,
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

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -71,   -71,     6,   146,   -71,     0,   -71,   -58,   -22,    -9,
     -71,   -71,   -71,   -71,    -4,   -71,    45,   -71,   -71,   -71,
     -71,   -71,   -71,    58,   -71,   -70,   -36
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    26,    27,    28,    84,    85,    30,    31,    52,    53,
      32,    33,    47,    34,    86,    35,   257,   222,    36,    62,
     144,    37,    61,   227,   228,   229,   230
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
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

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
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

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
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

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
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
#line 1903 "lang/y.tab.c"
    break;

  case 3: /* program: expr_sequence  */
#line 125 "lang/parser.y"
                      { parse_stmt_list(ast_root, (yyvsp[0].ast_node_ptr)); }
#line 1909 "lang/y.tab.c"
    break;

  case 6: /* expr: "yield" expr  */
#line 132 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_yield((yyvsp[0].ast_node_ptr)); }
#line 1915 "lang/y.tab.c"
    break;

  case 7: /* expr: expr DOUBLE_AT expr  */
#line 133 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1921 "lang/y.tab.c"
    break;

  case 8: /* expr: expr simple_expr  */
#line 134 "lang/parser.y"
                                       { (yyval.ast_node_ptr) = ast_application((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1927 "lang/y.tab.c"
    break;

  case 9: /* expr: expr '+' expr  */
#line 135 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1933 "lang/y.tab.c"
    break;

  case 10: /* expr: expr '-' expr  */
#line 136 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1939 "lang/y.tab.c"
    break;

  case 11: /* expr: expr '*' expr  */
#line 137 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1945 "lang/y.tab.c"
    break;

  case 12: /* expr: expr '/' expr  */
#line 138 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1951 "lang/y.tab.c"
    break;

  case 13: /* expr: expr MODULO expr  */
#line 139 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1957 "lang/y.tab.c"
    break;

  case 14: /* expr: expr '<' expr  */
#line 140 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1963 "lang/y.tab.c"
    break;

  case 15: /* expr: expr '>' expr  */
#line 141 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1969 "lang/y.tab.c"
    break;

  case 16: /* expr: expr DOUBLE_AMP expr  */
#line 142 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_AMP, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1975 "lang/y.tab.c"
    break;

  case 17: /* expr: expr DOUBLE_PIPE expr  */
#line 143 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_PIPE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1981 "lang/y.tab.c"
    break;

  case 18: /* expr: expr GE expr  */
#line 144 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1987 "lang/y.tab.c"
    break;

  case 19: /* expr: expr LE expr  */
#line 145 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1993 "lang/y.tab.c"
    break;

  case 20: /* expr: expr NE expr  */
#line 146 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1999 "lang/y.tab.c"
    break;

  case 21: /* expr: expr EQ expr  */
#line 147 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2005 "lang/y.tab.c"
    break;

  case 22: /* expr: expr PIPE expr  */
#line 148 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[0].ast_node_ptr), (yyvsp[-2].ast_node_ptr)); }
#line 2011 "lang/y.tab.c"
    break;

  case 23: /* expr: expr ':' expr  */
#line 149 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_assoc((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2017 "lang/y.tab.c"
    break;

  case 24: /* expr: expr "to" expr  */
#line 150 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_range_expression((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2023 "lang/y.tab.c"
    break;

  case 25: /* expr: expr DOUBLE_COLON expr  */
#line 151 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2029 "lang/y.tab.c"
    break;

  case 26: /* expr: let_binding  */
#line 152 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2035 "lang/y.tab.c"
    break;

  case 27: /* expr: match_expr  */
#line 153 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2041 "lang/y.tab.c"
    break;

  case 28: /* expr: type_decl  */
#line 154 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2047 "lang/y.tab.c"
    break;

  case 29: /* expr: THUNK expr  */
#line 155 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_thunk_expr((yyvsp[0].ast_node_ptr)); }
#line 2053 "lang/y.tab.c"
    break;

  case 30: /* expr: TRIPLE_DOT expr  */
#line 156 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_spread_operator((yyvsp[0].ast_node_ptr)); }
#line 2059 "lang/y.tab.c"
    break;

  case 31: /* expr: IDENTIFIER_LIST  */
#line 157 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_typed_empty_list((yyvsp[0].vident)); }
#line 2065 "lang/y.tab.c"
    break;

  case 32: /* expr: MACRO_IDENTIFIER expr  */
#line 158 "lang/parser.y"
                                      {
                                        // TODO: not doing anything with macros yet - do we want to??
                                        printf("macro '%s'\n", (yyvsp[-1].vident).chars);
                                        (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr);
                                      }
#line 2075 "lang/y.tab.c"
    break;

  case 33: /* expr: "for" IDENTIFIER '=' expr IN expr  */
#line 163 "lang/parser.y"
                                        {
                                          Ast *let = ast_let(ast_identifier((yyvsp[-4].vident)), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));
                                          let->tag = AST_LOOP;
                                          (yyval.ast_node_ptr) = let;

                                      }
#line 2086 "lang/y.tab.c"
    break;

  case 34: /* expr: expr ':' '=' expr  */
#line 172 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_assignment((yyvsp[-3].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2092 "lang/y.tab.c"
    break;

  case 35: /* simple_expr: INTEGER  */
#line 176 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[0].vint)); }
#line 2098 "lang/y.tab.c"
    break;

  case 36: /* simple_expr: DOUBLE  */
#line 177 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[0].vdouble)); }
#line 2104 "lang/y.tab.c"
    break;

  case 37: /* simple_expr: TOK_STRING  */
#line 178 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_string((yyvsp[0].vstr)); }
#line 2110 "lang/y.tab.c"
    break;

  case 38: /* simple_expr: TRUE  */
#line 179 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); }
#line 2116 "lang/y.tab.c"
    break;

  case 39: /* simple_expr: FALSE  */
#line 180 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); }
#line 2122 "lang/y.tab.c"
    break;

  case 40: /* simple_expr: IDENTIFIER  */
#line 181 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2128 "lang/y.tab.c"
    break;

  case 41: /* simple_expr: TOK_VOID  */
#line 182 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_void(); }
#line 2134 "lang/y.tab.c"
    break;

  case 42: /* simple_expr: list  */
#line 183 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2140 "lang/y.tab.c"
    break;

  case 43: /* simple_expr: array  */
#line 184 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2146 "lang/y.tab.c"
    break;

  case 44: /* simple_expr: tuple  */
#line 185 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2152 "lang/y.tab.c"
    break;

  case 45: /* simple_expr: fstring  */
#line 186 "lang/parser.y"
                          { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[0].ast_node_ptr)); }
#line 2158 "lang/y.tab.c"
    break;

  case 46: /* simple_expr: TOK_CHAR  */
#line 187 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_char((yyvsp[0].vchar)); }
#line 2164 "lang/y.tab.c"
    break;

  case 47: /* simple_expr: '(' expr_sequence ')'  */
#line 188 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2170 "lang/y.tab.c"
    break;

  case 48: /* simple_expr: '(' FN lambda_args ARROW expr_sequence ')'  */
#line 189 "lang/parser.y"
                                               { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2176 "lang/y.tab.c"
    break;

  case 49: /* simple_expr: '(' FN TOK_VOID ARROW expr_sequence ')'  */
#line 190 "lang/parser.y"
                                               { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); }
#line 2182 "lang/y.tab.c"
    break;

  case 50: /* simple_expr: '(' '+' ')'  */
#line 191 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"+", 1}); }
#line 2188 "lang/y.tab.c"
    break;

  case 51: /* simple_expr: '(' '-' ')'  */
#line 192 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"-", 1}); }
#line 2194 "lang/y.tab.c"
    break;

  case 52: /* simple_expr: '(' '*' ')'  */
#line 193 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"*", 1}); }
#line 2200 "lang/y.tab.c"
    break;

  case 53: /* simple_expr: '(' '/' ')'  */
#line 194 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"/", 1}); }
#line 2206 "lang/y.tab.c"
    break;

  case 54: /* simple_expr: '(' MODULO ')'  */
#line 195 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"%", 1}); }
#line 2212 "lang/y.tab.c"
    break;

  case 55: /* simple_expr: '(' '<' ')'  */
#line 196 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<", 1}); }
#line 2218 "lang/y.tab.c"
    break;

  case 56: /* simple_expr: '(' '>' ')'  */
#line 197 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){">", 1}); }
#line 2224 "lang/y.tab.c"
    break;

  case 57: /* simple_expr: '(' DOUBLE_AMP ')'  */
#line 198 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"&&", 2}); }
#line 2230 "lang/y.tab.c"
    break;

  case 58: /* simple_expr: '(' DOUBLE_PIPE ')'  */
#line 199 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"||", 2}); }
#line 2236 "lang/y.tab.c"
    break;

  case 59: /* simple_expr: '(' GE ')'  */
#line 200 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){">=", 2}); }
#line 2242 "lang/y.tab.c"
    break;

  case 60: /* simple_expr: '(' LE ')'  */
#line 201 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<=", 2}); }
#line 2248 "lang/y.tab.c"
    break;

  case 61: /* simple_expr: '(' NE ')'  */
#line 202 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"!=", 2}); }
#line 2254 "lang/y.tab.c"
    break;

  case 62: /* simple_expr: '(' EQ ')'  */
#line 203 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"==", 2}); }
#line 2260 "lang/y.tab.c"
    break;

  case 63: /* simple_expr: '(' PIPE ')'  */
#line 204 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"|", 1}); }
#line 2266 "lang/y.tab.c"
    break;

  case 64: /* simple_expr: '(' ':' ')'  */
#line 205 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){":", 1}); }
#line 2272 "lang/y.tab.c"
    break;

  case 65: /* simple_expr: '(' DOUBLE_COLON ')'  */
#line 206 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"::", 2}); }
#line 2278 "lang/y.tab.c"
    break;

  case 66: /* simple_expr: '(' custom_binop ')'  */
#line 207 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2284 "lang/y.tab.c"
    break;

  case 67: /* simple_expr: simple_expr '.' IDENTIFIER  */
#line 208 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_record_access((yyvsp[-2].ast_node_ptr), ast_identifier((yyvsp[0].vident))); }
#line 2290 "lang/y.tab.c"
    break;

  case 68: /* custom_binop: IDENTIFIER  */
#line 213 "lang/parser.y"
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
#line 2314 "lang/y.tab.c"
    break;

  case 69: /* expr_sequence: expr  */
#line 235 "lang/parser.y"
                                { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2320 "lang/y.tab.c"
    break;

  case 70: /* expr_sequence: expr_sequence ';' expr  */
#line 236 "lang/parser.y"
                                { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2326 "lang/y.tab.c"
    break;

  case 71: /* let_binding: LET TEST_ID '=' expr  */
#line 240 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_test_module((yyvsp[0].ast_node_ptr));}
#line 2332 "lang/y.tab.c"
    break;

  case 72: /* let_binding: LET IDENTIFIER '=' expr  */
#line 241 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL); }
#line 2338 "lang/y.tab.c"
    break;

  case 73: /* let_binding: LET IDENTIFIER '=' EXTERN FN fn_signature  */
#line 243 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-4].vident)), ast_extern_fn((yyvsp[-4].vident), (yyvsp[0].ast_node_ptr)), NULL); }
#line 2344 "lang/y.tab.c"
    break;

  case 74: /* let_binding: LET lambda_arg '=' expr  */
#line 245 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); }
#line 2350 "lang/y.tab.c"
    break;

  case 75: /* let_binding: LET MUT lambda_arg '=' expr  */
#line 247 "lang/parser.y"
                                    { Ast *let = ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2359 "lang/y.tab.c"
    break;

  case 76: /* let_binding: LET expr_list '=' expr  */
#line 251 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_tuple((yyvsp[-2].ast_node_ptr)), (yyvsp[0].ast_node_ptr), NULL);}
#line 2365 "lang/y.tab.c"
    break;

  case 77: /* let_binding: LET MUT expr_list '=' expr  */
#line 253 "lang/parser.y"
                                    { Ast *let = ast_let(ast_tuple((yyvsp[-2].ast_node_ptr)), (yyvsp[0].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2374 "lang/y.tab.c"
    break;

  case 78: /* let_binding: LET TOK_VOID '=' expr  */
#line 261 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2380 "lang/y.tab.c"
    break;

  case 79: /* let_binding: let_binding IN expr  */
#line 262 "lang/parser.y"
                                    {
                                      Ast *let = (yyvsp[-2].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[0].ast_node_ptr);
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2390 "lang/y.tab.c"
    break;

  case 80: /* let_binding: lambda_expr  */
#line 267 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2396 "lang/y.tab.c"
    break;

  case 81: /* let_binding: LET '(' IDENTIFIER ')' '=' lambda_expr  */
#line 270 "lang/parser.y"
                                    {
                                      Ast *id = ast_identifier((yyvsp[-3].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[0].ast_node_ptr), NULL);
                                    }
#line 2406 "lang/y.tab.c"
    break;

  case 82: /* let_binding: LET '(' IDENTIFIER ')' '=' expr  */
#line 277 "lang/parser.y"
                                    {
                                      Ast *id = ast_identifier((yyvsp[-3].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[0].ast_node_ptr), NULL);
                                    }
#line 2416 "lang/y.tab.c"
    break;

  case 83: /* let_binding: IMPORT PATH_IDENTIFIER  */
#line 286 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), false); }
#line 2422 "lang/y.tab.c"
    break;

  case 84: /* let_binding: OPEN PATH_IDENTIFIER  */
#line 287 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), true); }
#line 2428 "lang/y.tab.c"
    break;

  case 85: /* let_binding: IMPORT IDENTIFIER  */
#line 288 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), false); }
#line 2434 "lang/y.tab.c"
    break;

  case 86: /* let_binding: OPEN IDENTIFIER  */
#line 289 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), true); }
#line 2440 "lang/y.tab.c"
    break;

  case 87: /* let_binding: LET IDENTIFIER ':' IDENTIFIER '=' lambda_expr  */
#line 291 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_trait_impl((yyvsp[-4].vident), (yyvsp[-2].vident), (yyvsp[0].ast_node_ptr)); }
#line 2446 "lang/y.tab.c"
    break;

  case 88: /* lambda_expr: FN lambda_args ARROW expr_sequence ';'  */
#line 303 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2452 "lang/y.tab.c"
    break;

  case 89: /* lambda_expr: FN TOK_VOID ARROW expr_sequence ';'  */
#line 304 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); }
#line 2458 "lang/y.tab.c"
    break;

  case 90: /* lambda_expr: '(' FN lambda_args ARROW expr_sequence ')'  */
#line 305 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2464 "lang/y.tab.c"
    break;

  case 91: /* lambda_expr: '(' FN TOK_VOID ARROW expr_sequence ')'  */
#line 306 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); }
#line 2470 "lang/y.tab.c"
    break;

  case 92: /* lambda_expr: "module" lambda_args ARROW expr_sequence ';'  */
#line 307 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_module(ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr))); }
#line 2476 "lang/y.tab.c"
    break;

  case 93: /* lambda_expr: "module" expr_sequence ';'  */
#line 308 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_module(ast_lambda(NULL, (yyvsp[-1].ast_node_ptr))); }
#line 2482 "lang/y.tab.c"
    break;

  case 94: /* lambda_args: lambda_arg  */
#line 315 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[0].ast_node_ptr), NULL); }
#line 2488 "lang/y.tab.c"
    break;

  case 95: /* lambda_args: lambda_arg '=' expr  */
#line 316 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2494 "lang/y.tab.c"
    break;

  case 96: /* lambda_args: lambda_args lambda_arg  */
#line 317 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); }
#line 2500 "lang/y.tab.c"
    break;

  case 97: /* lambda_args: lambda_args lambda_arg '=' expr  */
#line 318 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-3].ast_node_ptr), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2506 "lang/y.tab.c"
    break;

  case 98: /* lambda_args: lambda_arg  */
#line 320 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[0].ast_node_ptr), NULL); }
#line 2512 "lang/y.tab.c"
    break;

  case 99: /* lambda_args: lambda_arg ':' '(' type_expr ')'  */
#line 321 "lang/parser.y"
                                     { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[-4].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2518 "lang/y.tab.c"
    break;

  case 100: /* lambda_args: lambda_args lambda_arg  */
#line 322 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); }
#line 2524 "lang/y.tab.c"
    break;

  case 101: /* lambda_args: lambda_args lambda_arg ':' '(' type_expr ')'  */
#line 323 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-5].ast_node_ptr), (yyvsp[-4].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2530 "lang/y.tab.c"
    break;

  case 102: /* lambda_arg: IDENTIFIER  */
#line 327 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2536 "lang/y.tab.c"
    break;

  case 103: /* lambda_arg: '(' expr_list ')'  */
#line 328 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-1].ast_node_ptr)); }
#line 2542 "lang/y.tab.c"
    break;

  case 104: /* lambda_arg: list_match_expr  */
#line 329 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2548 "lang/y.tab.c"
    break;

  case 105: /* list: '[' ']'  */
#line 339 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_empty_list(); }
#line 2554 "lang/y.tab.c"
    break;

  case 106: /* list: '[' expr_list ']'  */
#line 340 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2560 "lang/y.tab.c"
    break;

  case 107: /* list: '[' expr_list ',' ']'  */
#line 341 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[-2].ast_node_ptr); }
#line 2566 "lang/y.tab.c"
    break;

  case 108: /* array: '[' '|' '|' ']'  */
#line 345 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_empty_array(); }
#line 2572 "lang/y.tab.c"
    break;

  case 109: /* array: '[' '|' expr_list '|' ']'  */
#line 346 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[-2].ast_node_ptr)); }
#line 2578 "lang/y.tab.c"
    break;

  case 110: /* array: '[' '|' expr_list ',' '|' ']'  */
#line 347 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[-3].ast_node_ptr)); }
#line 2584 "lang/y.tab.c"
    break;

  case 111: /* list_match_expr: IDENTIFIER DOUBLE_COLON IDENTIFIER  */
#line 351 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[-2].vident)), ast_identifier((yyvsp[0].vident))); }
#line 2590 "lang/y.tab.c"
    break;

  case 112: /* list_match_expr: IDENTIFIER DOUBLE_COLON expr  */
#line 352 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2596 "lang/y.tab.c"
    break;

  case 113: /* tuple: '(' expr ')'  */
#line 356 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2602 "lang/y.tab.c"
    break;

  case 114: /* tuple: '(' expr_list ')'  */
#line 357 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-1].ast_node_ptr)); }
#line 2608 "lang/y.tab.c"
    break;

  case 115: /* tuple: '(' expr_list ',' ')'  */
#line 358 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-2].ast_node_ptr)); }
#line 2614 "lang/y.tab.c"
    break;

  case 116: /* expr_list: expr  */
#line 362 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_list((yyvsp[0].ast_node_ptr)); }
#line 2620 "lang/y.tab.c"
    break;

  case 117: /* expr_list: expr_list ',' expr  */
#line 363 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2626 "lang/y.tab.c"
    break;

  case 118: /* match_expr: MATCH expr WITH match_branches  */
#line 367 "lang/parser.y"
                                   { (yyval.ast_node_ptr) = ast_match((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2632 "lang/y.tab.c"
    break;

  case 119: /* match_expr: "if" expr THEN expr ELSE expr  */
#line 368 "lang/parser.y"
                                   { (yyval.ast_node_ptr) = ast_if_else((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr) ,(yyvsp[0].ast_node_ptr));}
#line 2638 "lang/y.tab.c"
    break;

  case 120: /* match_expr: "if" expr THEN expr  */
#line 369 "lang/parser.y"
                                   { (yyval.ast_node_ptr) = ast_if_else((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL);}
#line 2644 "lang/y.tab.c"
    break;

  case 121: /* match_test_clause: expr  */
#line 373 "lang/parser.y"
         {(yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr);}
#line 2650 "lang/y.tab.c"
    break;

  case 122: /* match_test_clause: expr "if" expr  */
#line 374 "lang/parser.y"
                   { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2656 "lang/y.tab.c"
    break;

  case 123: /* match_branches: '|' match_test_clause ARROW expr  */
#line 377 "lang/parser.y"
                                                     {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2662 "lang/y.tab.c"
    break;

  case 124: /* match_branches: match_branches '|' match_test_clause ARROW expr  */
#line 378 "lang/parser.y"
                                                     {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2668 "lang/y.tab.c"
    break;

  case 125: /* match_branches: match_branches '|' '_' ARROW expr  */
#line 379 "lang/parser.y"
                                        {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[-4].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[0].ast_node_ptr));}
#line 2674 "lang/y.tab.c"
    break;

  case 126: /* fstring: FSTRING_START fstring_parts FSTRING_END  */
#line 382 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2680 "lang/y.tab.c"
    break;

  case 127: /* fstring_parts: %empty  */
#line 386 "lang/parser.y"
                                { (yyval.ast_node_ptr) = ast_empty_list(); }
#line 2686 "lang/y.tab.c"
    break;

  case 128: /* fstring_parts: fstring_parts fstring_part  */
#line 387 "lang/parser.y"
                                { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2692 "lang/y.tab.c"
    break;

  case 129: /* fstring_part: FSTRING_TEXT  */
#line 391 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_string((yyvsp[0].vstr)); }
#line 2698 "lang/y.tab.c"
    break;

  case 130: /* fstring_part: FSTRING_INTERP_START expr FSTRING_INTERP_END  */
#line 392 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2704 "lang/y.tab.c"
    break;

  case 131: /* type_decl: TYPE IDENTIFIER '=' type_expr  */
#line 396 "lang/parser.y"
                                  {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
#line 2714 "lang/y.tab.c"
    break;

  case 132: /* type_decl: TYPE IDENTIFIER  */
#line 402 "lang/parser.y"
                                 {
                                      Ast *type_decl = ast_let(ast_identifier((yyvsp[0].vident)), NULL, NULL);
                                      type_decl->tag = AST_TYPE_DECL;
                                      (yyval.ast_node_ptr) = type_decl;
                                   }
#line 2724 "lang/y.tab.c"
    break;

  case 133: /* type_decl: TYPE type_args '=' type_expr  */
#line 408 "lang/parser.y"
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
#line 2739 "lang/y.tab.c"
    break;

  case 134: /* type_args: IDENTIFIER  */
#line 421 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list(ast_identifier((yyvsp[0].vident)), NULL); }
#line 2745 "lang/y.tab.c"
    break;

  case 135: /* type_args: type_args IDENTIFIER  */
#line 422 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), ast_identifier((yyvsp[0].vident)), NULL); }
#line 2751 "lang/y.tab.c"
    break;

  case 136: /* fn_signature: type_expr ARROW type_expr  */
#line 425 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2757 "lang/y.tab.c"
    break;

  case 137: /* fn_signature: fn_signature ARROW type_expr  */
#line 426 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2763 "lang/y.tab.c"
    break;

  case 138: /* tuple_type: type_atom ',' type_atom  */
#line 430 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2769 "lang/y.tab.c"
    break;

  case 139: /* tuple_type: tuple_type ',' type_atom  */
#line 431 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2775 "lang/y.tab.c"
    break;

  case 140: /* type_expr: type_atom  */
#line 435 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2781 "lang/y.tab.c"
    break;

  case 141: /* type_expr: '|' type_atom  */
#line 436 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_list((yyvsp[0].ast_node_ptr)); }
#line 2787 "lang/y.tab.c"
    break;

  case 142: /* type_expr: type_expr '|' type_atom  */
#line 437 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2793 "lang/y.tab.c"
    break;

  case 143: /* type_expr: fn_signature  */
#line 438 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_fn_signature_of_list((yyvsp[0].ast_node_ptr)); }
#line 2799 "lang/y.tab.c"
    break;

  case 144: /* type_expr: tuple_type  */
#line 439 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2805 "lang/y.tab.c"
    break;

  case 145: /* type_atom: IDENTIFIER  */
#line 443 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2811 "lang/y.tab.c"
    break;

  case 146: /* type_atom: IDENTIFIER '=' INTEGER  */
#line 444 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-2].vident)), AST_CONST(AST_INT, (yyvsp[0].vint)), NULL); }
#line 2817 "lang/y.tab.c"
    break;

  case 147: /* type_atom: IDENTIFIER "of" type_atom  */
#line 445 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2823 "lang/y.tab.c"
    break;

  case 148: /* type_atom: IDENTIFIER ':' type_atom  */
#line 446 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_assoc(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2829 "lang/y.tab.c"
    break;

  case 149: /* type_atom: '(' type_expr ')'  */
#line 447 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2835 "lang/y.tab.c"
    break;

  case 150: /* type_atom: TOK_VOID  */
#line 448 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_void(); }
#line 2841 "lang/y.tab.c"
    break;

  case 151: /* type_atom: IDENTIFIER '.' IDENTIFIER  */
#line 449 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_record_access(ast_identifier((yyvsp[-2].vident)), ast_identifier((yyvsp[0].vident))); }
#line 2847 "lang/y.tab.c"
    break;


#line 2851 "lang/y.tab.c"

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

#line 451 "lang/parser.y"



void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, _cur_script);
}
#endif _LANG_TAB_Hparse
