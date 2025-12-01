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

#define AST_CONST(type, val)                                            \
    ({                                                                  \
      Ast *prefix = Ast_new(type);                                      \
      prefix->data.type.value = val;                                    \
      prefix;                                                           \
    })


#line 98 "lang/y.tab.c"

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
  YYSYMBOL_DOUBLE_DOT = 15,                /* DOUBLE_DOT  */
  YYSYMBOL_LET = 16,                       /* LET  */
  YYSYMBOL_FN = 17,                        /* FN  */
  YYSYMBOL_MODULE = 18,                    /* MODULE  */
  YYSYMBOL_MATCH = 19,                     /* MATCH  */
  YYSYMBOL_WITH = 20,                      /* WITH  */
  YYSYMBOL_ARROW = 21,                     /* ARROW  */
  YYSYMBOL_DOUBLE_COLON = 22,              /* DOUBLE_COLON  */
  YYSYMBOL_TOK_VOID = 23,                  /* TOK_VOID  */
  YYSYMBOL_IN = 24,                        /* IN  */
  YYSYMBOL_AND = 25,                       /* AND  */
  YYSYMBOL_ASYNC = 26,                     /* ASYNC  */
  YYSYMBOL_DOUBLE_AT = 27,                 /* DOUBLE_AT  */
  YYSYMBOL_THUNK = 28,                     /* THUNK  */
  YYSYMBOL_IMPORT = 29,                    /* IMPORT  */
  YYSYMBOL_OPEN = 30,                      /* OPEN  */
  YYSYMBOL_IMPLEMENTS = 31,                /* IMPLEMENTS  */
  YYSYMBOL_AMPERSAND = 32,                 /* AMPERSAND  */
  YYSYMBOL_TYPE = 33,                      /* TYPE  */
  YYSYMBOL_TEST_ID = 34,                   /* TEST_ID  */
  YYSYMBOL_MUT = 35,                       /* MUT  */
  YYSYMBOL_THEN = 36,                      /* THEN  */
  YYSYMBOL_ELSE = 37,                      /* ELSE  */
  YYSYMBOL_YIELD = 38,                     /* YIELD  */
  YYSYMBOL_AWAIT = 39,                     /* AWAIT  */
  YYSYMBOL_FOR = 40,                       /* FOR  */
  YYSYMBOL_IF = 41,                        /* IF  */
  YYSYMBOL_OF = 42,                        /* OF  */
  YYSYMBOL_FSTRING_START = 43,             /* FSTRING_START  */
  YYSYMBOL_FSTRING_END = 44,               /* FSTRING_END  */
  YYSYMBOL_FSTRING_INTERP_START = 45,      /* FSTRING_INTERP_START  */
  YYSYMBOL_FSTRING_INTERP_END = 46,        /* FSTRING_INTERP_END  */
  YYSYMBOL_FSTRING_TEXT = 47,              /* FSTRING_TEXT  */
  YYSYMBOL_48_ = 48,                       /* '|'  */
  YYSYMBOL_DOUBLE_AMP = 49,                /* DOUBLE_AMP  */
  YYSYMBOL_DOUBLE_PIPE = 50,               /* DOUBLE_PIPE  */
  YYSYMBOL_GE = 51,                        /* GE  */
  YYSYMBOL_LE = 52,                        /* LE  */
  YYSYMBOL_EQ = 53,                        /* EQ  */
  YYSYMBOL_NE = 54,                        /* NE  */
  YYSYMBOL_55_ = 55,                       /* '>'  */
  YYSYMBOL_56_ = 56,                       /* '<'  */
  YYSYMBOL_57_ = 57,                       /* '+'  */
  YYSYMBOL_58_ = 58,                       /* '-'  */
  YYSYMBOL_59_ = 59,                       /* '*'  */
  YYSYMBOL_60_ = 60,                       /* '/'  */
  YYSYMBOL_MODULO = 61,                    /* MODULO  */
  YYSYMBOL_62_ = 62,                       /* ':'  */
  YYSYMBOL_APPLICATION = 63,               /* APPLICATION  */
  YYSYMBOL_64_ = 64,                       /* '.'  */
  YYSYMBOL_UMINUS = 65,                    /* UMINUS  */
  YYSYMBOL_66_ = 66,                       /* ';'  */
  YYSYMBOL_67_ = 67,                       /* '='  */
  YYSYMBOL_68_ = 68,                       /* '['  */
  YYSYMBOL_69_ = 69,                       /* ']'  */
  YYSYMBOL_70_ = 70,                       /* '('  */
  YYSYMBOL_71_ = 71,                       /* ')'  */
  YYSYMBOL_72___ = 72,                     /* '_'  */
  YYSYMBOL_73_ = 73,                       /* ','  */
  YYSYMBOL_YYACCEPT = 74,                  /* $accept  */
  YYSYMBOL_program = 75,                   /* program  */
  YYSYMBOL_expr = 76,                      /* expr  */
  YYSYMBOL_atom_expr = 77,                 /* atom_expr  */
  YYSYMBOL_simple_expr = 78,               /* simple_expr  */
  YYSYMBOL_expr_sequence = 79,             /* expr_sequence  */
  YYSYMBOL_let_binding = 80,               /* let_binding  */
  YYSYMBOL_lambda_expr = 81,               /* lambda_expr  */
  YYSYMBOL_lambda_args = 82,               /* lambda_args  */
  YYSYMBOL_lambda_arg = 83,                /* lambda_arg  */
  YYSYMBOL_list = 84,                      /* list  */
  YYSYMBOL_array = 85,                     /* array  */
  YYSYMBOL_tuple = 86,                     /* tuple  */
  YYSYMBOL_expr_list = 87,                 /* expr_list  */
  YYSYMBOL_match_expr = 88,                /* match_expr  */
  YYSYMBOL_match_test_clause = 89,         /* match_test_clause  */
  YYSYMBOL_match_branches = 90,            /* match_branches  */
  YYSYMBOL_fstring = 91,                   /* fstring  */
  YYSYMBOL_fstring_parts = 92,             /* fstring_parts  */
  YYSYMBOL_fstring_part = 93,              /* fstring_part  */
  YYSYMBOL_type_decl = 94,                 /* type_decl  */
  YYSYMBOL_type_args = 95,                 /* type_args  */
  YYSYMBOL_fn_signature = 96,              /* fn_signature  */
  YYSYMBOL_tuple_type = 97,                /* tuple_type  */
  YYSYMBOL_type_expr = 98,                 /* type_expr  */
  YYSYMBOL_type_atom = 99                  /* type_atom  */
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
#define YYFINAL  92
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1897

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  74
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  26
/* YYNRULES -- Number of rules.  */
#define YYNRULES  147
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  308

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
      70,    71,    59,    57,    73,    58,    64,    60,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    62,    66,
      56,    67,    55,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    68,     2,    69,     2,    72,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    48,     2,     2,     2,     2,     2,
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
      45,    46,    47,    49,    50,    51,    52,    53,    54,    61,
      63,    65
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   124,   124,   125,   126,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   158,   159,   165,   166,   167,   171,   172,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,   207,
     208,   213,   214,   218,   219,   220,   223,   225,   227,   235,
     236,   241,   243,   250,   260,   261,   262,   263,   264,   270,
     271,   272,   273,   280,   281,   282,   283,   284,   285,   289,
     290,   291,   292,   297,   298,   299,   303,   304,   305,   310,
     311,   312,   316,   317,   321,   322,   323,   327,   328,   331,
     332,   333,   336,   340,   341,   345,   346,   350,   356,   362,
     375,   376,   379,   380,   384,   385,   389,   390,   391,   392,
     393,   397,   398,   399,   400,   401,   402,   403
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
  "DOUBLE_DOT", "LET", "FN", "MODULE", "MATCH", "WITH", "ARROW",
  "DOUBLE_COLON", "TOK_VOID", "IN", "AND", "ASYNC", "DOUBLE_AT", "THUNK",
  "IMPORT", "OPEN", "IMPLEMENTS", "AMPERSAND", "TYPE", "TEST_ID", "MUT",
  "THEN", "ELSE", "YIELD", "AWAIT", "FOR", "IF", "OF", "FSTRING_START",
  "FSTRING_END", "FSTRING_INTERP_START", "FSTRING_INTERP_END",
  "FSTRING_TEXT", "'|'", "DOUBLE_AMP", "DOUBLE_PIPE", "GE", "LE", "EQ",
  "NE", "'>'", "'<'", "'+'", "'-'", "'*'", "'/'", "MODULO", "':'",
  "APPLICATION", "'.'", "UMINUS", "';'", "'='", "'['", "']'", "'('", "')'",
  "'_'", "','", "$accept", "program", "expr", "atom_expr", "simple_expr",
  "expr_sequence", "let_binding", "lambda_expr", "lambda_args",
  "lambda_arg", "list", "array", "tuple", "expr_list", "match_expr",
  "match_test_clause", "match_branches", "fstring", "fstring_parts",
  "fstring_part", "type_decl", "type_args", "fn_signature", "tuple_type",
  "type_expr", "type_atom", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-197)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-113)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1757,  -197,  -197,  -197,  -197,  -197,  -197,  -197,  -197,  -197,
     427,    53,    54,  1757,  -197,  1757,    16,   166,    46,  1757,
    1757,    55,  1757,  -197,  1379,   563,    73,  1175,    42,  -197,
      51,   111,  -197,  -197,  -197,  -197,  -197,  -197,  -197,   -11,
       8,    61,  1757,   631,  -197,  1175,    71,   -47,   123,   127,
    1757,   144,   -38,   131,   176,   699,  1175,  -197,  -197,  -197,
    -197,     0,    11,  1175,  1175,    95,   767,   110,  1427,  -197,
      74,    93,    99,   189,   105,   108,   109,   112,   113,   118,
     119,   120,   125,   138,   142,   148,   154,   160,   162,   335,
     -32,   -36,  -197,  1757,  1757,  1757,  1757,  1757,  1757,  1757,
    1757,  1757,  1757,  1757,  1757,  1757,  1757,  1757,  1757,  1757,
    1473,  1379,    42,   188,  1757,  1757,    41,   209,  1516,  1757,
    1757,   -46,   163,    85,  1757,  1757,  1757,  1757,    88,  1757,
     -26,   179,  1757,  1757,  1757,   202,  -197,    -4,  -197,    -4,
    1757,  1757,  -197,  1757,  -197,  -197,   183,   -35,  -197,  1559,
    -197,  -197,   233,   181,  -197,  -197,  -197,  -197,  -197,  -197,
    -197,  -197,  -197,  -197,  -197,  -197,  -197,  -197,  -197,  -197,
    1757,  -197,  -197,   519,  1243,  1333,   195,  1311,  1600,  1600,
     217,   217,   217,   217,   217,   217,    59,    59,  1798,  1798,
    1827,  1757,   195,   835,  -197,  1175,  1175,  -197,   197,   238,
    1175,  1175,  1175,  1757,   199,   200,  1175,  1175,  1175,   190,
    -197,   196,   198,  1757,    -4,  1175,   203,   204,  1757,   223,
     104,  -197,     2,    -4,   259,   210,    -5,   211,    -5,   903,
     971,  1039,  -197,   220,  1668,  -197,  1757,  1757,  -197,  1175,
    1714,  -197,   157,    -4,  1175,  1757,  1757,  1757,    -4,  1175,
     -17,  1757,  1757,  1107,   265,   473,     2,     2,   286,   290,
    -197,     9,    -4,     2,    -4,     2,     2,  1757,  1757,  -197,
    -197,   244,   -18,   -16,  -197,  -197,   259,    -5,  1175,  -197,
     121,  -197,  1757,  1757,   295,   298,  -197,  -197,  -197,  -197,
    -197,    -5,  -197,    -5,  -197,  -197,  1175,  1175,  -197,  -197,
    -197,  -197,  1175,  1175,  1757,  1757,  1175,  1175
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,    38,    39,    40,    44,    31,    41,    50,    42,    43,
       0,     0,     0,     0,    45,     0,     0,     0,     0,     0,
       0,     0,     0,   123,     0,     0,     0,    71,     5,    36,
       3,    27,    81,    46,    47,    48,    28,    49,    29,    44,
      45,     0,     0,     0,   102,   112,     0,     0,    99,     0,
       0,     0,    93,     0,     0,     0,    30,    86,    84,    87,
      85,   128,     0,     6,     7,     0,     0,     0,     0,   103,
       0,    44,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    71,
       0,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     9,     0,     2,     0,     0,     0,     0,     0,
       0,     0,    44,     0,     0,     0,     0,     0,     0,     0,
      96,     0,     0,     0,     0,     0,   130,     0,   131,     0,
       0,     0,   122,     0,   125,   124,     0,     0,   104,     0,
      70,    67,     0,     0,    69,    61,    62,    63,    64,    66,
      65,    60,    59,    54,    55,    56,    57,    58,    68,   109,
       0,    51,   110,     0,    23,    25,    26,     8,    17,    18,
      19,    20,    22,    21,    16,    15,    10,    11,    12,    13,
      14,     0,    24,   112,    37,    72,    80,   101,     0,     0,
      74,    79,    73,     0,    70,   110,    76,    77,   113,     0,
     100,     0,     0,     0,     0,    94,     0,     0,     0,   114,
     141,   146,     0,     0,   139,   140,   127,   136,   129,     0,
     116,     0,   106,     0,     0,   105,     0,     0,   111,    35,
       0,    33,     0,     0,    78,     0,    90,    89,     0,    97,
       0,    92,    91,   117,     0,     0,     0,     0,     0,     0,
     137,     0,     0,     0,     0,     0,     0,     0,     0,   126,
     107,     0,     0,     0,    34,    88,    75,     0,    83,    81,
       0,    95,     0,     0,     0,     0,   143,   144,   147,   142,
     145,   133,   135,   132,   138,   134,    32,   115,   108,    53,
      52,    98,   118,   119,     0,     0,   121,   120
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -197,  -197,   -10,   122,  -197,     7,  -197,  -196,     6,    -9,
    -197,  -197,  -197,   373,  -197,    65,  -197,  -197,  -197,  -197,
    -197,  -197,    78,  -197,  -111,    25
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    26,    27,    28,    29,    90,    31,    32,    51,    52,
      33,    34,    35,    70,    36,   254,   219,    37,    67,   145,
      38,    62,   224,   225,   226,   227
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      45,    46,   220,    55,   264,    56,   136,    30,   220,    63,
      64,   116,    66,   233,    45,    89,   264,   138,    54,   221,
     125,   203,    57,    58,   131,   221,   126,   126,   228,   132,
     264,   265,    45,    89,   170,   172,   212,   173,   234,   171,
      45,   213,   130,   265,   222,   130,   275,    48,   246,   279,
     247,   117,    61,   299,   281,   300,   118,   265,    45,    48,
      48,    65,     1,     2,     3,     4,   223,   137,     6,     7,
       8,     9,   223,    92,    94,   119,    49,    53,   139,   153,
     290,    95,    14,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     192,   193,    23,   250,   195,   196,   113,   197,   200,   201,
     202,    50,   261,    44,   206,   207,   208,   114,   107,   108,
     109,   110,   215,    50,    50,    44,    44,   111,   120,    25,
     229,   230,   277,   231,   209,   115,   211,   280,   124,   208,
     216,   217,   264,   148,   130,   116,   256,   149,   127,   112,
      48,   291,   133,   293,   142,   143,   205,   144,   173,   210,
     195,   126,   140,   208,   150,   129,   257,   112,   258,   265,
     151,   259,    59,    60,    11,    12,   154,   112,   112,   155,
     156,   239,    48,   157,   158,   112,   112,    48,   112,   159,
     160,   161,   301,   244,   194,    48,   162,   134,     1,     2,
       3,     4,   237,   249,     6,     7,     8,     9,   253,   163,
      94,   112,   152,   164,    50,   198,    44,    95,    14,   165,
       1,     2,     3,     4,   208,   166,     6,     7,     8,     9,
     175,   167,    94,   168,   204,   278,   195,   195,    23,    95,
      14,   195,   195,   272,   273,   253,    50,   260,    44,   214,
     218,    50,   232,    44,   236,   243,   246,   296,   297,    50,
      23,    44,   247,   111,   242,    25,   245,  -100,   248,   251,
     252,   255,   302,   303,   105,   106,   107,   108,   109,   110,
     262,   286,   287,   263,   266,   111,   283,    25,   292,   270,
     294,   295,   288,   289,   306,   307,   112,   112,   112,   112,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   112,
     112,   112,   112,   298,   112,   112,   304,   112,   112,   305,
     285,   276,   112,   112,   112,     0,     0,     0,   112,   112,
     112,     0,     0,     0,     0,     0,     0,   112,     1,     2,
       3,     4,     0,     0,     6,     7,     8,     9,    93,     0,
      94,   112,   112,   112,     0,     0,     0,    95,    14,     0,
       0,   112,    96,     0,     0,     0,   112,     0,     0,     0,
       0,   112,     0,     0,     0,   112,     0,     0,    23,     0,
       0,     0,     0,    47,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,    91,     0,
     112,     0,     0,   111,     0,    25,   169,     0,  -112,     0,
       0,     0,     0,     0,     0,   121,   123,     0,   112,   112,
       0,     0,     0,   128,   112,   112,     0,     0,   112,   112,
       1,     2,     3,    39,     0,     5,     6,     7,     8,     9,
       0,   147,     0,    10,    11,    12,    13,     0,     0,     0,
      40,     0,     0,     0,     0,    15,    16,    17,     0,     0,
      18,    41,    42,     0,     0,    19,    20,    21,    22,     0,
      23,     0,     0,     0,     0,     0,     1,     2,     3,     4,
       0,     5,     6,     7,     8,     9,     0,     0,     0,    10,
      11,    12,    13,     0,     0,    24,    14,    43,     0,    44,
       0,    15,    16,    17,     0,     0,    18,     0,     0,     0,
       0,    19,    20,    21,    22,     0,    23,     0,     0,     0,
       0,     0,     1,     2,     3,     4,     0,     5,     6,     7,
       8,     9,     0,     0,     0,    10,    11,    12,    13,     0,
       0,    24,    14,    25,     0,   284,     0,    15,    16,    17,
       0,     0,    18,     0,     0,     0,     0,    19,    20,    21,
      22,     0,    23,     0,     0,     0,     1,     2,     3,    71,
       0,     5,     6,     7,     8,     9,    72,     0,     0,    10,
      73,    12,    13,     0,     0,    74,    14,    24,     0,    25,
     238,    15,    16,    17,     0,     0,    18,     0,     0,     0,
       0,    19,    20,    21,    22,     0,    23,     0,     0,     0,
       0,     0,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,     0,     0,     0,     0,
       0,    24,     0,    25,     1,     2,     3,   122,     0,     5,
       6,     7,     8,     9,    72,     0,     0,    10,    73,    12,
      13,     0,     0,    74,    14,     0,     0,     0,     0,    15,
      16,    17,     0,     0,    18,     0,     0,     0,     0,    19,
      20,    21,    22,     0,    23,     0,     0,     0,     0,     0,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,     0,     0,     0,     0,     0,    24,
       0,    25,     1,     2,     3,     4,     0,     0,     6,     7,
       8,     9,    93,     0,    94,     0,     0,     0,     0,   135,
       0,    95,    14,     0,     0,     0,    96,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    23,     0,     0,     0,     0,     0,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,     0,     0,     0,     0,     0,   111,     0,    25,
       1,     2,     3,     4,     0,     0,     6,     7,     8,     9,
      93,     0,    94,     0,     0,     0,     0,     0,     0,    95,
      14,     0,     0,     0,    96,     0,     0,     0,     0,     0,
       0,     0,     0,   141,     0,     0,     0,     0,     0,     0,
      23,     0,     0,     0,     0,     0,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
       0,     0,     0,     0,     0,   111,     0,    25,     1,     2,
       3,     4,     0,     0,     6,     7,     8,     9,    93,     0,
     240,     0,     0,     0,     0,     0,     0,    95,    14,     0,
       0,     0,    96,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    23,     0,
       0,     0,     0,     0,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,     0,     0,
       0,     0,     0,   111,   241,    25,     1,     2,     3,     4,
       0,     0,     6,     7,     8,     9,    93,     0,    94,     0,
       0,     0,     0,     0,     0,    95,    14,   267,     0,     0,
      96,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    23,     0,     0,     0,
       0,     0,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,     0,     0,     0,     0,
       0,   111,     0,    25,     1,     2,     3,     4,     0,     0,
       6,     7,     8,     9,    93,     0,    94,     0,     0,     0,
       0,     0,     0,    95,    14,     0,     0,     0,    96,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   268,     0,
       0,     0,     0,     0,    23,     0,     0,     0,     0,     0,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,     0,     0,     0,     0,     0,   111,
       0,    25,     1,     2,     3,     4,     0,     0,     6,     7,
       8,     9,    93,     0,    94,     0,     0,     0,     0,     0,
       0,    95,    14,     0,     0,     0,    96,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    23,     0,     0,   269,     0,     0,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,     0,     0,     0,     0,     0,   111,     0,    25,
       1,     2,     3,     4,     0,     0,     6,     7,     8,     9,
      93,     0,    94,     0,     0,     0,     0,     0,     0,    95,
      14,     0,     0,     0,    96,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   282,     0,
      23,     0,     0,     0,     0,     0,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
       0,     0,     0,     0,     0,   111,     0,    25,     1,     2,
       3,     4,     0,     0,     6,     7,     8,     9,    93,     0,
      94,     0,     0,     0,     0,     0,     0,    95,    14,     0,
       0,     0,    96,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    23,     0,
       0,     0,     0,     0,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,     0,     0,
       0,     0,     0,   111,     0,    25,     1,     2,     3,     4,
       0,     0,     6,     7,     8,     9,     0,     0,    94,     0,
       0,     0,     0,     0,     0,    95,    14,     0,     0,     0,
      96,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    23,     0,     0,     0,
       0,     0,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,     0,     0,     0,     0,
       0,   111,     0,    25,     1,     2,     3,     4,     0,     0,
       6,     7,     8,     9,     0,     0,    94,     0,     0,     0,
       0,     0,     0,    95,    14,     0,     1,     2,     3,     4,
       0,     0,     6,     7,     8,     9,     0,     0,     0,     0,
       0,     0,     0,     0,    23,     0,    14,     0,     0,     0,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,     0,     0,    23,     0,     0,   111,
       0,    25,     1,     2,     3,     4,     0,     5,     6,     7,
       8,     9,     0,     0,     0,    10,    11,    12,    13,     0,
       0,   111,    14,    25,     0,     0,     0,    15,    16,    17,
       0,     0,    18,     0,     0,     0,     0,    19,    20,    21,
      22,     0,    23,     0,     0,     0,     0,    68,     0,     0,
       1,     2,     3,     4,     0,     5,     6,     7,     8,     9,
       0,     0,     0,    10,    11,    12,    13,    24,    69,    25,
      14,     0,     0,     0,     0,    15,    16,    17,     0,     0,
      18,     0,     0,     0,     0,    19,    20,    21,    22,     0,
      23,     0,     0,     0,     0,   146,     1,     2,     3,     4,
       0,     5,     6,     7,     8,     9,     0,     0,     0,    10,
      11,    12,    13,     0,     0,    24,    14,    25,     0,     0,
       0,    15,    16,    17,     0,     0,    18,     0,     0,     0,
       0,    19,    20,    21,    22,     0,    23,     0,     0,     1,
       2,     3,     4,     0,     5,     6,     7,     8,     9,     0,
     199,     0,    10,    11,    12,    13,     0,     0,     0,    14,
     191,    24,     0,    25,    15,    16,    17,     0,     0,    18,
       0,     0,     0,     0,    19,    20,    21,    22,     0,    23,
       0,     0,     1,     2,     3,     4,     0,     5,     6,     7,
       8,     9,     0,     0,     0,    10,    11,    12,    13,     0,
       0,     0,    14,     0,    24,     0,    25,    15,    16,    17,
       0,     0,    18,     0,     0,     0,     0,    19,    20,    21,
      22,     0,    23,     1,     2,     3,     4,     0,     0,     6,
       7,     8,     9,     0,     0,    94,     0,     0,     0,     0,
       0,     0,    95,    14,     0,     0,     0,    24,   235,    25,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    23,     0,     0,     0,     0,     0,     0,
       0,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,     0,     0,     0,     0,     0,   111,     0,
      25,     1,     2,     3,     4,     0,     5,     6,     7,     8,
       9,     0,     0,     0,    10,    11,    12,    13,     0,     0,
       0,    14,     0,     0,     0,     0,    15,    16,    17,     0,
       0,    18,     0,     0,     0,     0,    19,    20,    21,    22,
       0,    23,     0,     0,     0,     0,   271,     1,     2,     3,
       4,     0,     5,     6,     7,     8,     9,     0,     0,     0,
      10,    11,    12,    13,     0,     0,    24,    14,    25,     0,
       0,     0,    15,    16,    17,     0,     0,    18,     0,     0,
       0,     0,    19,    20,    21,    22,     0,    23,     0,     0,
       1,     2,     3,     4,     0,     5,     6,     7,     8,     9,
       0,     0,     0,    10,    11,    12,    13,     0,     0,     0,
      14,     0,    24,   274,    25,    15,    16,    17,     0,     0,
      18,     0,     0,     0,     0,    19,    20,    21,    22,     0,
      23,     1,     2,     3,     4,     0,     0,     6,     7,     8,
       9,     0,     0,    94,     0,     0,     0,     0,     0,     0,
      95,    14,     0,     0,     0,    24,     0,    25,     0,     0,
       1,     2,     3,     4,     0,     0,     6,     7,     8,     9,
       0,    23,    94,     0,     0,     0,     0,     0,     0,    95,
      14,     0,     0,     0,     0,     0,     0,     0,     0,   109,
     110,     0,     0,     0,     0,     0,   111,     0,    25,     0,
      23,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   110,
       0,     0,     0,     0,     0,   111,     0,    25
};

static const yytype_int16 yycheck[] =
{
      10,    10,     6,    13,    21,    15,     6,     0,     6,    19,
      20,    22,    22,    48,    24,    25,    21,     6,    12,    23,
      67,    67,     6,     7,    62,    23,    73,    73,   139,    67,
      21,    48,    42,    43,    66,    71,    62,    73,    73,    71,
      50,    67,    51,    48,    48,    54,   242,     6,    66,   245,
      66,    62,     6,    71,    71,    71,    67,    48,    68,     6,
       6,     6,     3,     4,     5,     6,    70,    67,     9,    10,
      11,    12,    70,     0,    15,    67,    23,    23,    67,    73,
      71,    22,    23,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,    43,   214,   114,   115,    64,   116,   118,   119,
     120,    70,   223,    72,   124,   125,   126,    66,    59,    60,
      61,    62,   132,    70,    70,    72,    72,    68,    67,    70,
     140,   141,   243,   143,   127,    24,   129,   248,    67,   149,
     133,   134,    21,    69,   153,    22,    42,    73,    21,    27,
       6,   262,    21,   264,    44,    45,    71,    47,    73,    71,
     170,    73,    67,   173,    71,    21,    62,    45,    64,    48,
      71,    67,     6,     7,    17,    18,    71,    55,    56,    71,
      71,   191,     6,    71,    71,    63,    64,     6,    66,    71,
      71,    71,    71,   203,     6,     6,    71,    21,     3,     4,
       5,     6,    21,   213,     9,    10,    11,    12,   218,    71,
      15,    89,    23,    71,    70,     6,    72,    22,    23,    71,
       3,     4,     5,     6,   234,    71,     9,    10,    11,    12,
     240,    71,    15,    71,    71,   245,   246,   247,    43,    22,
      23,   251,   252,   236,   237,   255,    70,   222,    72,    70,
      48,    70,    69,    72,    21,    17,    66,   267,   268,    70,
      43,    72,    66,    68,    67,    70,    67,    67,    70,    66,
      66,    48,   282,   283,    57,    58,    59,    60,    61,    62,
      21,   256,   257,    73,    73,    68,    21,    70,   263,    69,
     265,   266,     6,     3,   304,   305,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,    69,   192,   193,    21,   195,   196,    21,
     255,   243,   200,   201,   202,    -1,    -1,    -1,   206,   207,
     208,    -1,    -1,    -1,    -1,    -1,    -1,   215,     3,     4,
       5,     6,    -1,    -1,     9,    10,    11,    12,    13,    -1,
      15,   229,   230,   231,    -1,    -1,    -1,    22,    23,    -1,
      -1,   239,    27,    -1,    -1,    -1,   244,    -1,    -1,    -1,
      -1,   249,    -1,    -1,    -1,   253,    -1,    -1,    43,    -1,
      -1,    -1,    -1,    10,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    25,    -1,
     278,    -1,    -1,    68,    -1,    70,    71,    -1,    73,    -1,
      -1,    -1,    -1,    -1,    -1,    42,    43,    -1,   296,   297,
      -1,    -1,    -1,    50,   302,   303,    -1,    -1,   306,   307,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    68,    -1,    16,    17,    18,    19,    -1,    -1,    -1,
      23,    -1,    -1,    -1,    -1,    28,    29,    30,    -1,    -1,
      33,    34,    35,    -1,    -1,    38,    39,    40,    41,    -1,
      43,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    -1,    -1,    16,
      17,    18,    19,    -1,    -1,    68,    23,    70,    -1,    72,
      -1,    28,    29,    30,    -1,    -1,    33,    -1,    -1,    -1,
      -1,    38,    39,    40,    41,    -1,    43,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    -1,    16,    17,    18,    19,    -1,
      -1,    68,    23,    70,    -1,    72,    -1,    28,    29,    30,
      -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    39,    40,
      41,    -1,    43,    -1,    -1,    -1,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    13,    -1,    -1,    16,
      17,    18,    19,    -1,    -1,    22,    23,    68,    -1,    70,
      71,    28,    29,    30,    -1,    -1,    33,    -1,    -1,    -1,
      -1,    38,    39,    40,    41,    -1,    43,    -1,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    -1,    -1,    -1,    -1,
      -1,    68,    -1,    70,     3,     4,     5,     6,    -1,     8,
       9,    10,    11,    12,    13,    -1,    -1,    16,    17,    18,
      19,    -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    28,
      29,    30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    38,
      39,    40,    41,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    -1,    -1,    -1,    -1,    -1,    68,
      -1,    70,     3,     4,     5,     6,    -1,    -1,     9,    10,
      11,    12,    13,    -1,    15,    -1,    -1,    -1,    -1,    20,
      -1,    22,    23,    -1,    -1,    -1,    27,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    -1,    -1,    -1,    -1,    -1,    68,    -1,    70,
       3,     4,     5,     6,    -1,    -1,     9,    10,    11,    12,
      13,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    -1,    -1,    -1,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,
      43,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      -1,    -1,    -1,    -1,    -1,    68,    -1,    70,     3,     4,
       5,     6,    -1,    -1,     9,    10,    11,    12,    13,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    -1,
      -1,    -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    43,    -1,
      -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    -1,    -1,
      -1,    -1,    -1,    68,    69,    70,     3,     4,     5,     6,
      -1,    -1,     9,    10,    11,    12,    13,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    22,    23,    24,    -1,    -1,
      27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    -1,    -1,    -1,    -1,
      -1,    68,    -1,    70,     3,     4,     5,     6,    -1,    -1,
       9,    10,    11,    12,    13,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    22,    23,    -1,    -1,    -1,    27,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    -1,
      -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    -1,    -1,    -1,    -1,    -1,    68,
      -1,    70,     3,     4,     5,     6,    -1,    -1,     9,    10,
      11,    12,    13,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      -1,    22,    23,    -1,    -1,    -1,    27,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    43,    -1,    -1,    46,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    -1,    -1,    -1,    -1,    -1,    68,    -1,    70,
       3,     4,     5,     6,    -1,    -1,     9,    10,    11,    12,
      13,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    -1,    -1,    -1,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,
      43,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      -1,    -1,    -1,    -1,    -1,    68,    -1,    70,     3,     4,
       5,     6,    -1,    -1,     9,    10,    11,    12,    13,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    -1,
      -1,    -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    43,    -1,
      -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    -1,    -1,
      -1,    -1,    -1,    68,    -1,    70,     3,     4,     5,     6,
      -1,    -1,     9,    10,    11,    12,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    22,    23,    -1,    -1,    -1,
      27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    43,    -1,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    -1,    -1,    -1,    -1,
      -1,    68,    -1,    70,     3,     4,     5,     6,    -1,    -1,
       9,    10,    11,    12,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    22,    23,    -1,     3,     4,     5,     6,
      -1,    -1,     9,    10,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    43,    -1,    23,    -1,    -1,    -1,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    -1,    -1,    43,    -1,    -1,    68,
      -1,    70,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    -1,    16,    17,    18,    19,    -1,
      -1,    68,    23,    70,    -1,    -1,    -1,    28,    29,    30,
      -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    39,    40,
      41,    -1,    43,    -1,    -1,    -1,    -1,    48,    -1,    -1,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    -1,    -1,    16,    17,    18,    19,    68,    69,    70,
      23,    -1,    -1,    -1,    -1,    28,    29,    30,    -1,    -1,
      33,    -1,    -1,    -1,    -1,    38,    39,    40,    41,    -1,
      43,    -1,    -1,    -1,    -1,    48,     3,     4,     5,     6,
      -1,     8,     9,    10,    11,    12,    -1,    -1,    -1,    16,
      17,    18,    19,    -1,    -1,    68,    23,    70,    -1,    -1,
      -1,    28,    29,    30,    -1,    -1,    33,    -1,    -1,    -1,
      -1,    38,    39,    40,    41,    -1,    43,    -1,    -1,     3,
       4,     5,     6,    -1,     8,     9,    10,    11,    12,    -1,
      14,    -1,    16,    17,    18,    19,    -1,    -1,    -1,    23,
      67,    68,    -1,    70,    28,    29,    30,    -1,    -1,    33,
      -1,    -1,    -1,    -1,    38,    39,    40,    41,    -1,    43,
      -1,    -1,     3,     4,     5,     6,    -1,     8,     9,    10,
      11,    12,    -1,    -1,    -1,    16,    17,    18,    19,    -1,
      -1,    -1,    23,    -1,    68,    -1,    70,    28,    29,    30,
      -1,    -1,    33,    -1,    -1,    -1,    -1,    38,    39,    40,
      41,    -1,    43,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    -1,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    22,    23,    -1,    -1,    -1,    68,    69,    70,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    -1,    -1,    -1,    -1,    -1,    68,    -1,
      70,     3,     4,     5,     6,    -1,     8,     9,    10,    11,
      12,    -1,    -1,    -1,    16,    17,    18,    19,    -1,    -1,
      -1,    23,    -1,    -1,    -1,    -1,    28,    29,    30,    -1,
      -1,    33,    -1,    -1,    -1,    -1,    38,    39,    40,    41,
      -1,    43,    -1,    -1,    -1,    -1,    48,     3,     4,     5,
       6,    -1,     8,     9,    10,    11,    12,    -1,    -1,    -1,
      16,    17,    18,    19,    -1,    -1,    68,    23,    70,    -1,
      -1,    -1,    28,    29,    30,    -1,    -1,    33,    -1,    -1,
      -1,    -1,    38,    39,    40,    41,    -1,    43,    -1,    -1,
       3,     4,     5,     6,    -1,     8,     9,    10,    11,    12,
      -1,    -1,    -1,    16,    17,    18,    19,    -1,    -1,    -1,
      23,    -1,    68,    69,    70,    28,    29,    30,    -1,    -1,
      33,    -1,    -1,    -1,    -1,    38,    39,    40,    41,    -1,
      43,     3,     4,     5,     6,    -1,    -1,     9,    10,    11,
      12,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      22,    23,    -1,    -1,    -1,    68,    -1,    70,    -1,    -1,
       3,     4,     5,     6,    -1,    -1,     9,    10,    11,    12,
      -1,    43,    15,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,
      62,    -1,    -1,    -1,    -1,    -1,    68,    -1,    70,    -1,
      43,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    68,    -1,    70
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     8,     9,    10,    11,    12,
      16,    17,    18,    19,    23,    28,    29,    30,    33,    38,
      39,    40,    41,    43,    68,    70,    75,    76,    77,    78,
      79,    80,    81,    84,    85,    86,    88,    91,    94,     6,
      23,    34,    35,    70,    72,    76,    83,    87,     6,    23,
      70,    82,    83,    23,    82,    76,    76,     6,     7,     6,
       7,     6,    95,    76,    76,     6,    76,    92,    48,    69,
      87,     6,    13,    17,    22,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    76,
      79,    87,     0,    13,    15,    22,    27,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    68,    77,    64,    66,    24,    22,    62,    67,    67,
      67,    87,     6,    87,    67,    67,    73,    21,    87,    21,
      83,    62,    67,    21,    21,    20,     6,    67,     6,    67,
      67,    36,    44,    45,    47,    93,    48,    87,    69,    73,
      71,    71,    23,    82,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      66,    71,    71,    73,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    67,    76,    76,     6,    76,    76,    83,     6,    14,
      76,    76,    76,    67,    71,    71,    76,    76,    76,    79,
      71,    79,    62,    67,    70,    76,    79,    79,    48,    90,
       6,    23,    48,    70,    96,    97,    98,    99,    98,    76,
      76,    76,    69,    48,    73,    69,    21,    21,    71,    76,
      15,    69,    67,    17,    76,    67,    66,    66,    70,    76,
      98,    66,    66,    76,    89,    48,    42,    62,    64,    67,
      99,    98,    21,    73,    21,    48,    73,    24,    37,    46,
      69,    48,    79,    79,    69,    81,    96,    98,    76,    81,
      98,    71,    41,    21,    72,    89,    99,    99,     6,     3,
      71,    98,    99,    98,    99,    99,    76,    76,    69,    71,
      71,    71,    76,    76,    21,    21,    76,    76
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    74,    75,    75,    75,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    76,    76,    77,    77,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    79,    79,    80,    80,    80,    80,    80,    80,    80,
      80,    80,    80,    80,    80,    80,    80,    80,    80,    81,
      81,    81,    81,    82,    82,    82,    82,    82,    82,    83,
      83,    83,    83,    84,    84,    84,    85,    85,    85,    86,
      86,    86,    87,    87,    88,    88,    88,    89,    89,    90,
      90,    90,    91,    92,    92,    93,    93,    94,    94,    94,
      95,    95,    96,    96,    97,    97,    98,    98,    98,    98,
      98,    99,    99,    99,    99,    99,    99,    99
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     2,     2,     3,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     1,     1,
       2,     1,     6,     4,     5,     4,     1,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     6,     6,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     1,     3,     4,     4,     6,     4,     4,     5,     4,
       3,     1,     6,     6,     2,     2,     2,     2,     6,     5,
       5,     5,     5,     1,     3,     5,     2,     4,     6,     1,
       3,     3,     1,     2,     3,     4,     4,     5,     6,     3,
       3,     4,     1,     3,     4,     6,     4,     1,     3,     4,
       5,     5,     3,     0,     2,     1,     3,     4,     2,     4,
       2,     2,     3,     3,     3,     3,     1,     2,     3,     1,
       1,     1,     3,     3,     3,     3,     1,     3
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
                      { pctx.ast_root = parse_stmt_list(pctx.ast_root, (yyvsp[-1].ast_node_ptr)); }
#line 1821 "lang/y.tab.c"
    break;

  case 3: /* program: expr_sequence  */
#line 125 "lang/parser.y"
                      { pctx.ast_root = parse_stmt_list(pctx.ast_root, (yyvsp[0].ast_node_ptr)); }
#line 1827 "lang/y.tab.c"
    break;

  case 6: /* expr: YIELD expr  */
#line 132 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_yield((yyvsp[0].ast_node_ptr)); }
#line 1833 "lang/y.tab.c"
    break;

  case 7: /* expr: AWAIT expr  */
#line 133 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_await((yyvsp[0].ast_node_ptr)); }
#line 1839 "lang/y.tab.c"
    break;

  case 8: /* expr: expr DOUBLE_AT expr  */
#line 134 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1845 "lang/y.tab.c"
    break;

  case 9: /* expr: expr atom_expr  */
#line 135 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1851 "lang/y.tab.c"
    break;

  case 10: /* expr: expr '+' expr  */
#line 136 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_PLUS, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1857 "lang/y.tab.c"
    break;

  case 11: /* expr: expr '-' expr  */
#line 137 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_MINUS, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1863 "lang/y.tab.c"
    break;

  case 12: /* expr: expr '*' expr  */
#line 138 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_STAR, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1869 "lang/y.tab.c"
    break;

  case 13: /* expr: expr '/' expr  */
#line 139 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_SLASH, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1875 "lang/y.tab.c"
    break;

  case 14: /* expr: expr MODULO expr  */
#line 140 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_MODULO, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1881 "lang/y.tab.c"
    break;

  case 15: /* expr: expr '<' expr  */
#line 141 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_LT, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1887 "lang/y.tab.c"
    break;

  case 16: /* expr: expr '>' expr  */
#line 142 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_GT, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1893 "lang/y.tab.c"
    break;

  case 17: /* expr: expr DOUBLE_AMP expr  */
#line 143 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_AMP, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1899 "lang/y.tab.c"
    break;

  case 18: /* expr: expr DOUBLE_PIPE expr  */
#line 144 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_DOUBLE_PIPE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1905 "lang/y.tab.c"
    break;

  case 19: /* expr: expr GE expr  */
#line 145 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_GTE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1911 "lang/y.tab.c"
    break;

  case 20: /* expr: expr LE expr  */
#line 146 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_LTE, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1917 "lang/y.tab.c"
    break;

  case 21: /* expr: expr NE expr  */
#line 147 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_NOT_EQUAL, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1923 "lang/y.tab.c"
    break;

  case 22: /* expr: expr EQ expr  */
#line 148 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_binop(TOKEN_EQUALITY, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1929 "lang/y.tab.c"
    break;

  case 23: /* expr: expr PIPE expr  */
#line 149 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_application((yyvsp[0].ast_node_ptr), (yyvsp[-2].ast_node_ptr)); }
#line 1935 "lang/y.tab.c"
    break;

  case 24: /* expr: expr ':' expr  */
#line 150 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_assoc((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1941 "lang/y.tab.c"
    break;

  case 25: /* expr: expr DOUBLE_DOT expr  */
#line 151 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_range_expression((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1947 "lang/y.tab.c"
    break;

  case 26: /* expr: expr DOUBLE_COLON expr  */
#line 152 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_list_prepend((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 1953 "lang/y.tab.c"
    break;

  case 27: /* expr: let_binding  */
#line 153 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 1959 "lang/y.tab.c"
    break;

  case 28: /* expr: match_expr  */
#line 154 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 1965 "lang/y.tab.c"
    break;

  case 29: /* expr: type_decl  */
#line 155 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 1971 "lang/y.tab.c"
    break;

  case 30: /* expr: THUNK expr  */
#line 156 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_thunk_expr((yyvsp[0].ast_node_ptr)); }
#line 1977 "lang/y.tab.c"
    break;

  case 31: /* expr: IDENTIFIER_LIST  */
#line 158 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_typed_empty_list((yyvsp[0].vident)); }
#line 1983 "lang/y.tab.c"
    break;

  case 32: /* expr: FOR IDENTIFIER '=' expr IN expr  */
#line 159 "lang/parser.y"
                                      {
                                          Ast *let = ast_let(ast_identifier((yyvsp[-4].vident)), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));
                                          let->tag = AST_LOOP;
                                          (yyval.ast_node_ptr) = let;

                                      }
#line 1994 "lang/y.tab.c"
    break;

  case 33: /* expr: expr '[' expr ']'  */
#line 165 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = array_index_expression((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr));}
#line 2000 "lang/y.tab.c"
    break;

  case 34: /* expr: expr '[' expr DOUBLE_DOT ']'  */
#line 166 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = array_offset_expression((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr));}
#line 2006 "lang/y.tab.c"
    break;

  case 35: /* expr: expr ':' '=' expr  */
#line 167 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_assignment((yyvsp[-3].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2012 "lang/y.tab.c"
    break;

  case 37: /* atom_expr: atom_expr '.' IDENTIFIER  */
#line 172 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_record_access((yyvsp[-2].ast_node_ptr), ast_identifier((yyvsp[0].vident))); }
#line 2018 "lang/y.tab.c"
    break;

  case 38: /* simple_expr: INTEGER  */
#line 176 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_INT, (yyvsp[0].vint)); }
#line 2024 "lang/y.tab.c"
    break;

  case 39: /* simple_expr: DOUBLE  */
#line 177 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_DOUBLE, (yyvsp[0].vdouble)); }
#line 2030 "lang/y.tab.c"
    break;

  case 40: /* simple_expr: FLOAT  */
#line 178 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_FLOAT, (yyvsp[0].vfloat)); }
#line 2036 "lang/y.tab.c"
    break;

  case 41: /* simple_expr: TOK_STRING  */
#line 179 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_string((yyvsp[0].vstr)); }
#line 2042 "lang/y.tab.c"
    break;

  case 42: /* simple_expr: TRUE  */
#line 180 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, true); }
#line 2048 "lang/y.tab.c"
    break;

  case 43: /* simple_expr: FALSE  */
#line 181 "lang/parser.y"
                          { (yyval.ast_node_ptr) = AST_CONST(AST_BOOL, false); }
#line 2054 "lang/y.tab.c"
    break;

  case 44: /* simple_expr: IDENTIFIER  */
#line 182 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2060 "lang/y.tab.c"
    break;

  case 45: /* simple_expr: TOK_VOID  */
#line 183 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_void(); }
#line 2066 "lang/y.tab.c"
    break;

  case 46: /* simple_expr: list  */
#line 184 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2072 "lang/y.tab.c"
    break;

  case 47: /* simple_expr: array  */
#line 185 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2078 "lang/y.tab.c"
    break;

  case 48: /* simple_expr: tuple  */
#line 186 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2084 "lang/y.tab.c"
    break;

  case 49: /* simple_expr: fstring  */
#line 187 "lang/parser.y"
                          { (yyval.ast_node_ptr) = parse_fstring_expr((yyvsp[0].ast_node_ptr)); }
#line 2090 "lang/y.tab.c"
    break;

  case 50: /* simple_expr: TOK_CHAR  */
#line 188 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_char((yyvsp[0].vchar)); }
#line 2096 "lang/y.tab.c"
    break;

  case 51: /* simple_expr: '(' expr_sequence ')'  */
#line 189 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2102 "lang/y.tab.c"
    break;

  case 52: /* simple_expr: '(' FN lambda_args ARROW expr_sequence ')'  */
#line 190 "lang/parser.y"
                                               { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2108 "lang/y.tab.c"
    break;

  case 53: /* simple_expr: '(' FN TOK_VOID ARROW expr_sequence ')'  */
#line 191 "lang/parser.y"
                                               { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); }
#line 2114 "lang/y.tab.c"
    break;

  case 54: /* simple_expr: '(' '+' ')'  */
#line 192 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"+", 1}); }
#line 2120 "lang/y.tab.c"
    break;

  case 55: /* simple_expr: '(' '-' ')'  */
#line 193 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"-", 1}); }
#line 2126 "lang/y.tab.c"
    break;

  case 56: /* simple_expr: '(' '*' ')'  */
#line 194 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"*", 1}); }
#line 2132 "lang/y.tab.c"
    break;

  case 57: /* simple_expr: '(' '/' ')'  */
#line 195 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"/", 1}); }
#line 2138 "lang/y.tab.c"
    break;

  case 58: /* simple_expr: '(' MODULO ')'  */
#line 196 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"%", 1}); }
#line 2144 "lang/y.tab.c"
    break;

  case 59: /* simple_expr: '(' '<' ')'  */
#line 197 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<", 1}); }
#line 2150 "lang/y.tab.c"
    break;

  case 60: /* simple_expr: '(' '>' ')'  */
#line 198 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){">", 1}); }
#line 2156 "lang/y.tab.c"
    break;

  case 61: /* simple_expr: '(' DOUBLE_AMP ')'  */
#line 199 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"&&", 2}); }
#line 2162 "lang/y.tab.c"
    break;

  case 62: /* simple_expr: '(' DOUBLE_PIPE ')'  */
#line 200 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"||", 2}); }
#line 2168 "lang/y.tab.c"
    break;

  case 63: /* simple_expr: '(' GE ')'  */
#line 201 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){">=", 2}); }
#line 2174 "lang/y.tab.c"
    break;

  case 64: /* simple_expr: '(' LE ')'  */
#line 202 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"<=", 2}); }
#line 2180 "lang/y.tab.c"
    break;

  case 65: /* simple_expr: '(' NE ')'  */
#line 203 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"!=", 2}); }
#line 2186 "lang/y.tab.c"
    break;

  case 66: /* simple_expr: '(' EQ ')'  */
#line 204 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"==", 2}); }
#line 2192 "lang/y.tab.c"
    break;

  case 67: /* simple_expr: '(' PIPE ')'  */
#line 205 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"|", 1}); }
#line 2198 "lang/y.tab.c"
    break;

  case 68: /* simple_expr: '(' ':' ')'  */
#line 206 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){":", 1}); }
#line 2204 "lang/y.tab.c"
    break;

  case 69: /* simple_expr: '(' DOUBLE_COLON ')'  */
#line 207 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_identifier((ObjString){"::", 2}); }
#line 2210 "lang/y.tab.c"
    break;

  case 70: /* simple_expr: '(' IDENTIFIER ')'  */
#line 208 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_identifier((yyvsp[-1].vident)); }
#line 2216 "lang/y.tab.c"
    break;

  case 71: /* expr_sequence: expr  */
#line 213 "lang/parser.y"
                                { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2222 "lang/y.tab.c"
    break;

  case 72: /* expr_sequence: expr_sequence ';' expr  */
#line 214 "lang/parser.y"
                                { (yyval.ast_node_ptr) = parse_stmt_list((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2228 "lang/y.tab.c"
    break;

  case 73: /* let_binding: LET TEST_ID '=' expr  */
#line 218 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_test_module((yyvsp[0].ast_node_ptr));}
#line 2234 "lang/y.tab.c"
    break;

  case 74: /* let_binding: LET IDENTIFIER '=' expr  */
#line 219 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL); }
#line 2240 "lang/y.tab.c"
    break;

  case 75: /* let_binding: LET IDENTIFIER '=' EXTERN FN fn_signature  */
#line 221 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-4].vident)), ast_extern_fn((yyvsp[-4].vident), (yyvsp[0].ast_node_ptr)), NULL); }
#line 2246 "lang/y.tab.c"
    break;

  case 76: /* let_binding: LET lambda_arg '=' expr  */
#line 223 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); }
#line 2252 "lang/y.tab.c"
    break;

  case 77: /* let_binding: LET expr_list '=' expr  */
#line 225 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_let(ast_tuple((yyvsp[-2].ast_node_ptr)), (yyvsp[0].ast_node_ptr), NULL);}
#line 2258 "lang/y.tab.c"
    break;

  case 78: /* let_binding: LET MUT expr_list '=' expr  */
#line 227 "lang/parser.y"
                                    { Ast *let = ast_let(ast_tuple((yyvsp[-2].ast_node_ptr)), (yyvsp[0].ast_node_ptr), NULL);
                                      let->data.AST_LET.is_mut = true;
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2267 "lang/y.tab.c"
    break;

  case 79: /* let_binding: LET TOK_VOID '=' expr  */
#line 235 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2273 "lang/y.tab.c"
    break;

  case 80: /* let_binding: let_binding IN expr  */
#line 236 "lang/parser.y"
                                    {
                                      Ast *let = (yyvsp[-2].ast_node_ptr);
                                      let->data.AST_LET.in_expr = (yyvsp[0].ast_node_ptr);
                                      (yyval.ast_node_ptr) = let;
                                    }
#line 2283 "lang/y.tab.c"
    break;

  case 81: /* let_binding: lambda_expr  */
#line 241 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2289 "lang/y.tab.c"
    break;

  case 82: /* let_binding: LET '(' IDENTIFIER ')' '=' lambda_expr  */
#line 244 "lang/parser.y"
                                    {
                                      Ast *id = ast_identifier((yyvsp[-3].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[0].ast_node_ptr), NULL);
                                    }
#line 2299 "lang/y.tab.c"
    break;

  case 83: /* let_binding: LET '(' IDENTIFIER ')' '=' expr  */
#line 251 "lang/parser.y"
                                    {
                                      Ast *id = ast_identifier((yyvsp[-3].vident));
                                      add_custom_binop(id->data.AST_IDENTIFIER.value);
                                      (yyval.ast_node_ptr) = ast_let(id, (yyvsp[0].ast_node_ptr), NULL);
                                    }
#line 2309 "lang/y.tab.c"
    break;

  case 84: /* let_binding: IMPORT PATH_IDENTIFIER  */
#line 260 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), false); }
#line 2315 "lang/y.tab.c"
    break;

  case 85: /* let_binding: OPEN PATH_IDENTIFIER  */
#line 261 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), true); }
#line 2321 "lang/y.tab.c"
    break;

  case 86: /* let_binding: IMPORT IDENTIFIER  */
#line 262 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), false); }
#line 2327 "lang/y.tab.c"
    break;

  case 87: /* let_binding: OPEN IDENTIFIER  */
#line 263 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = ast_import_stmt((yyvsp[0].vident), true); }
#line 2333 "lang/y.tab.c"
    break;

  case 88: /* let_binding: LET IDENTIFIER ':' IDENTIFIER '=' lambda_expr  */
#line 264 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_trait_impl((yyvsp[-2].vident), (yyvsp[-4].vident), (yyvsp[0].ast_node_ptr)); }
#line 2339 "lang/y.tab.c"
    break;

  case 89: /* lambda_expr: FN lambda_args ARROW expr_sequence ';'  */
#line 270 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2345 "lang/y.tab.c"
    break;

  case 90: /* lambda_expr: FN TOK_VOID ARROW expr_sequence ';'  */
#line 271 "lang/parser.y"
                                                { (yyval.ast_node_ptr) = ast_void_lambda((yyvsp[-1].ast_node_ptr)); }
#line 2351 "lang/y.tab.c"
    break;

  case 91: /* lambda_expr: MODULE lambda_args ARROW expr_sequence ';'  */
#line 272 "lang/parser.y"
                                              { (yyval.ast_node_ptr) = ast_module(ast_lambda((yyvsp[-3].ast_node_ptr), (yyvsp[-1].ast_node_ptr))); }
#line 2357 "lang/y.tab.c"
    break;

  case 92: /* lambda_expr: MODULE TOK_VOID ARROW expr_sequence ';'  */
#line 273 "lang/parser.y"
                                              { (yyval.ast_node_ptr) = ast_module(ast_lambda(NULL, (yyvsp[-1].ast_node_ptr))); }
#line 2363 "lang/y.tab.c"
    break;

  case 93: /* lambda_args: lambda_arg  */
#line 280 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[0].ast_node_ptr), NULL); }
#line 2369 "lang/y.tab.c"
    break;

  case 94: /* lambda_args: lambda_arg '=' expr  */
#line 281 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2375 "lang/y.tab.c"
    break;

  case 95: /* lambda_args: lambda_arg ':' '(' type_expr ')'  */
#line 282 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list((yyvsp[-4].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2381 "lang/y.tab.c"
    break;

  case 96: /* lambda_args: lambda_args lambda_arg  */
#line 283 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL); }
#line 2387 "lang/y.tab.c"
    break;

  case 97: /* lambda_args: lambda_args lambda_arg '=' expr  */
#line 284 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-3].ast_node_ptr), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2393 "lang/y.tab.c"
    break;

  case 98: /* lambda_args: lambda_args lambda_arg ':' '(' type_expr ')'  */
#line 285 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-5].ast_node_ptr), (yyvsp[-4].ast_node_ptr), (yyvsp[-1].ast_node_ptr)); }
#line 2399 "lang/y.tab.c"
    break;

  case 99: /* lambda_arg: IDENTIFIER  */
#line 289 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2405 "lang/y.tab.c"
    break;

  case 100: /* lambda_arg: '(' expr_list ')'  */
#line 290 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-1].ast_node_ptr)); }
#line 2411 "lang/y.tab.c"
    break;

  case 101: /* lambda_arg: IDENTIFIER DOUBLE_COLON lambda_arg  */
#line 291 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_list_prepend(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2417 "lang/y.tab.c"
    break;

  case 102: /* lambda_arg: '_'  */
#line 292 "lang/parser.y"
                                      { (yyval.ast_node_ptr) = Ast_new(AST_PLACEHOLDER_ID); }
#line 2423 "lang/y.tab.c"
    break;

  case 103: /* list: '[' ']'  */
#line 297 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_empty_list(); }
#line 2429 "lang/y.tab.c"
    break;

  case 104: /* list: '[' expr_list ']'  */
#line 298 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2435 "lang/y.tab.c"
    break;

  case 105: /* list: '[' expr_list ',' ']'  */
#line 299 "lang/parser.y"
                            { (yyval.ast_node_ptr) = (yyvsp[-2].ast_node_ptr); }
#line 2441 "lang/y.tab.c"
    break;

  case 106: /* array: '[' '|' '|' ']'  */
#line 303 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_empty_array(); }
#line 2447 "lang/y.tab.c"
    break;

  case 107: /* array: '[' '|' expr_list '|' ']'  */
#line 304 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[-2].ast_node_ptr)); }
#line 2453 "lang/y.tab.c"
    break;

  case 108: /* array: '[' '|' expr_list ',' '|' ']'  */
#line 305 "lang/parser.y"
                                  { (yyval.ast_node_ptr) = ast_list_to_array((yyvsp[-3].ast_node_ptr)); }
#line 2459 "lang/y.tab.c"
    break;

  case 109: /* tuple: '(' expr ')'  */
#line 310 "lang/parser.y"
                          { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2465 "lang/y.tab.c"
    break;

  case 110: /* tuple: '(' expr_list ')'  */
#line 311 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-1].ast_node_ptr)); }
#line 2471 "lang/y.tab.c"
    break;

  case 111: /* tuple: '(' expr_list ',' ')'  */
#line 312 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_tuple((yyvsp[-2].ast_node_ptr)); }
#line 2477 "lang/y.tab.c"
    break;

  case 112: /* expr_list: expr  */
#line 316 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_list((yyvsp[0].ast_node_ptr)); }
#line 2483 "lang/y.tab.c"
    break;

  case 113: /* expr_list: expr_list ',' expr  */
#line 317 "lang/parser.y"
                          { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2489 "lang/y.tab.c"
    break;

  case 114: /* match_expr: MATCH expr WITH match_branches  */
#line 321 "lang/parser.y"
                                   { (yyval.ast_node_ptr) = ast_match((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2495 "lang/y.tab.c"
    break;

  case 115: /* match_expr: IF expr THEN expr ELSE expr  */
#line 322 "lang/parser.y"
                                 { (yyval.ast_node_ptr) = ast_if_else((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr) ,(yyvsp[0].ast_node_ptr));}
#line 2501 "lang/y.tab.c"
    break;

  case 116: /* match_expr: IF expr THEN expr  */
#line 323 "lang/parser.y"
                                 { (yyval.ast_node_ptr) = ast_if_else((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr), NULL);}
#line 2507 "lang/y.tab.c"
    break;

  case 117: /* match_test_clause: expr  */
#line 327 "lang/parser.y"
         {(yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr);}
#line 2513 "lang/y.tab.c"
    break;

  case 118: /* match_test_clause: expr IF expr  */
#line 328 "lang/parser.y"
                 { (yyval.ast_node_ptr) = ast_match_guard_clause((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2519 "lang/y.tab.c"
    break;

  case 119: /* match_branches: '|' match_test_clause ARROW expr  */
#line 331 "lang/parser.y"
                                                     {(yyval.ast_node_ptr) = ast_match_branches(NULL, (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2525 "lang/y.tab.c"
    break;

  case 120: /* match_branches: match_branches '|' match_test_clause ARROW expr  */
#line 332 "lang/parser.y"
                                                     {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[-4].ast_node_ptr), (yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr));}
#line 2531 "lang/y.tab.c"
    break;

  case 121: /* match_branches: match_branches '|' '_' ARROW expr  */
#line 333 "lang/parser.y"
                                        {(yyval.ast_node_ptr) = ast_match_branches((yyvsp[-4].ast_node_ptr), Ast_new(AST_PLACEHOLDER_ID), (yyvsp[0].ast_node_ptr));}
#line 2537 "lang/y.tab.c"
    break;

  case 122: /* fstring: FSTRING_START fstring_parts FSTRING_END  */
#line 336 "lang/parser.y"
                                                 { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2543 "lang/y.tab.c"
    break;

  case 123: /* fstring_parts: %empty  */
#line 340 "lang/parser.y"
                                { (yyval.ast_node_ptr) = ast_empty_list(); }
#line 2549 "lang/y.tab.c"
    break;

  case 124: /* fstring_parts: fstring_parts fstring_part  */
#line 341 "lang/parser.y"
                                { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-1].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2555 "lang/y.tab.c"
    break;

  case 125: /* fstring_part: FSTRING_TEXT  */
#line 345 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = ast_string((yyvsp[0].vstr)); }
#line 2561 "lang/y.tab.c"
    break;

  case 126: /* fstring_part: FSTRING_INTERP_START expr FSTRING_INTERP_END  */
#line 346 "lang/parser.y"
                                                  { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2567 "lang/y.tab.c"
    break;

  case 127: /* type_decl: TYPE IDENTIFIER '=' type_expr  */
#line 350 "lang/parser.y"
                                  {
                                    Ast *type_decl = ast_let(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr), NULL);
                                    type_decl->tag = AST_TYPE_DECL;
                                    (yyval.ast_node_ptr) = type_decl;
                                  }
#line 2577 "lang/y.tab.c"
    break;

  case 128: /* type_decl: TYPE IDENTIFIER  */
#line 356 "lang/parser.y"
                                 {
                                      Ast *type_decl = ast_let(ast_identifier((yyvsp[0].vident)), NULL, NULL);
                                      type_decl->tag = AST_TYPE_DECL;
                                      (yyval.ast_node_ptr) = type_decl;
                                   }
#line 2587 "lang/y.tab.c"
    break;

  case 129: /* type_decl: TYPE type_args '=' type_expr  */
#line 362 "lang/parser.y"
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
#line 2602 "lang/y.tab.c"
    break;

  case 130: /* type_args: IDENTIFIER IDENTIFIER  */
#line 375 "lang/parser.y"
                                         { (yyval.ast_node_ptr) = ast_arg_list_push(ast_arg_list(ast_identifier((yyvsp[-1].vident)), NULL), ast_identifier((yyvsp[0].vident)), NULL); }
#line 2608 "lang/y.tab.c"
    break;

  case 131: /* type_args: type_args IDENTIFIER  */
#line 376 "lang/parser.y"
                                         { (yyval.ast_node_ptr) = ast_arg_list_push((yyvsp[-1].ast_node_ptr), ast_identifier((yyvsp[0].vident)), NULL); }
#line 2614 "lang/y.tab.c"
    break;

  case 132: /* fn_signature: type_expr ARROW type_expr  */
#line 379 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_fn_sig((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2620 "lang/y.tab.c"
    break;

  case 133: /* fn_signature: fn_signature ARROW type_expr  */
#line 380 "lang/parser.y"
                                        { (yyval.ast_node_ptr) = ast_fn_sig_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2626 "lang/y.tab.c"
    break;

  case 134: /* tuple_type: type_atom ',' type_atom  */
#line 384 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_tuple_type((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2632 "lang/y.tab.c"
    break;

  case 135: /* tuple_type: tuple_type ',' type_atom  */
#line 385 "lang/parser.y"
                                    { (yyval.ast_node_ptr) = ast_tuple_type_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2638 "lang/y.tab.c"
    break;

  case 136: /* type_expr: type_atom  */
#line 389 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2644 "lang/y.tab.c"
    break;

  case 137: /* type_expr: '|' type_atom  */
#line 390 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_list((yyvsp[0].ast_node_ptr)); }
#line 2650 "lang/y.tab.c"
    break;

  case 138: /* type_expr: type_expr '|' type_atom  */
#line 391 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_list_push((yyvsp[-2].ast_node_ptr), (yyvsp[0].ast_node_ptr)); }
#line 2656 "lang/y.tab.c"
    break;

  case 139: /* type_expr: fn_signature  */
#line 392 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_fn_signature_of_list((yyvsp[0].ast_node_ptr)); }
#line 2662 "lang/y.tab.c"
    break;

  case 140: /* type_expr: tuple_type  */
#line 393 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[0].ast_node_ptr); }
#line 2668 "lang/y.tab.c"
    break;

  case 141: /* type_atom: IDENTIFIER  */
#line 397 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_identifier((yyvsp[0].vident)); }
#line 2674 "lang/y.tab.c"
    break;

  case 142: /* type_atom: IDENTIFIER '=' INTEGER  */
#line 398 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_let(ast_identifier((yyvsp[-2].vident)), AST_CONST(AST_INT, (yyvsp[0].vint)), NULL); }
#line 2680 "lang/y.tab.c"
    break;

  case 143: /* type_atom: IDENTIFIER OF type_atom  */
#line 399 "lang/parser.y"
                            { (yyval.ast_node_ptr) = ast_cons_decl(TOKEN_OF, ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2686 "lang/y.tab.c"
    break;

  case 144: /* type_atom: IDENTIFIER ':' type_atom  */
#line 400 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_assoc(ast_identifier((yyvsp[-2].vident)), (yyvsp[0].ast_node_ptr)); }
#line 2692 "lang/y.tab.c"
    break;

  case 145: /* type_atom: '(' type_expr ')'  */
#line 401 "lang/parser.y"
                              { (yyval.ast_node_ptr) = (yyvsp[-1].ast_node_ptr); }
#line 2698 "lang/y.tab.c"
    break;

  case 146: /* type_atom: TOK_VOID  */
#line 402 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_void(); }
#line 2704 "lang/y.tab.c"
    break;

  case 147: /* type_atom: IDENTIFIER '.' IDENTIFIER  */
#line 403 "lang/parser.y"
                              { (yyval.ast_node_ptr) = ast_record_access(ast_identifier((yyvsp[-2].vident)), ast_identifier((yyvsp[0].vident))); }
#line 2710 "lang/y.tab.c"
    break;


#line 2714 "lang/y.tab.c"

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

#line 405 "lang/parser.y"



void yyerror(const char *s) {
  fprintf(stderr, "Error: %s at %d:%d near '%s' in %s\n", s, yylineno, yycolumn, yytext, pctx.cur_script);
}
#endif _LANG_TAB_H
