/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_LANG_Y_TAB_H_INCLUDED
# define YY_YY_LANG_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    INTEGER = 258,                 /* INTEGER  */
    DOUBLE = 259,                  /* DOUBLE  */
    IDENTIFIER = 260,              /* IDENTIFIER  */
    MACRO_IDENTIFIER = 261,        /* MACRO_IDENTIFIER  */
    PATH_IDENTIFIER = 262,         /* PATH_IDENTIFIER  */
    IDENTIFIER_LIST = 263,         /* IDENTIFIER_LIST  */
    TOK_STRING = 264,              /* TOK_STRING  */
    TOK_CHAR = 265,                /* TOK_CHAR  */
    TRUE = 266,                    /* TRUE  */
    FALSE = 267,                   /* FALSE  */
    PIPE = 268,                    /* PIPE  */
    EXTERN = 269,                  /* EXTERN  */
    TRIPLE_DOT = 270,              /* TRIPLE_DOT  */
    LET = 271,                     /* LET  */
    FN = 272,                      /* FN  */
    MATCH = 273,                   /* MATCH  */
    WITH = 274,                    /* WITH  */
    ARROW = 275,                   /* ARROW  */
    DOUBLE_COLON = 276,            /* DOUBLE_COLON  */
    TOK_VOID = 277,                /* TOK_VOID  */
    IN = 278,                      /* IN  */
    AND = 279,                     /* AND  */
    ASYNC = 280,                   /* ASYNC  */
    DOUBLE_AT = 281,               /* DOUBLE_AT  */
    THUNK = 282,                   /* THUNK  */
    IMPORT = 283,                  /* IMPORT  */
    OPEN = 284,                    /* OPEN  */
    IMPLEMENTS = 285,              /* IMPLEMENTS  */
    AMPERSAND = 286,               /* AMPERSAND  */
    TYPE = 287,                    /* TYPE  */
    TEST_ID = 288,                 /* TEST_ID  */
    MUT = 289,                     /* MUT  */
    FSTRING_START = 290,           /* FSTRING_START  */
    FSTRING_END = 291,             /* FSTRING_END  */
    FSTRING_INTERP_START = 292,    /* FSTRING_INTERP_START  */
    FSTRING_INTERP_END = 293,      /* FSTRING_INTERP_END  */
    FSTRING_TEXT = 294,            /* FSTRING_TEXT  */
    APPLICATION = 295,             /* APPLICATION  */
    DOUBLE_AMP = 296,              /* DOUBLE_AMP  */
    DOUBLE_PIPE = 297,             /* DOUBLE_PIPE  */
    GE = 298,                      /* GE  */
    LE = 299,                      /* LE  */
    EQ = 300,                      /* EQ  */
    NE = 301,                      /* NE  */
    MODULO = 302,                  /* MODULO  */
    UMINUS = 303                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
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
#define MUT 289
#define FSTRING_START 290
#define FSTRING_END 291
#define FSTRING_INTERP_START 292
#define FSTRING_INTERP_END 293
#define FSTRING_TEXT 294
#define APPLICATION 295
#define DOUBLE_AMP 296
#define DOUBLE_PIPE 297
#define GE 298
#define LE 299
#define EQ 300
#define NE 301
#define MODULO 302
#define UMINUS 303

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 31 "lang/parser.y"

    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    char vchar;

#line 172 "lang/y.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE yylval;
extern YYLTYPE yylloc;

int yyparse (void);


#endif /* !YY_YY_LANG_Y_TAB_H_INCLUDED  */
