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
    UINT64 = 259,                  /* UINT64  */
    DOUBLE = 260,                  /* DOUBLE  */
    FLOAT = 261,                   /* FLOAT  */
    IDENTIFIER = 262,              /* IDENTIFIER  */
    PATH_IDENTIFIER = 263,         /* PATH_IDENTIFIER  */
    IDENTIFIER_LIST = 264,         /* IDENTIFIER_LIST  */
    TOK_STRING = 265,              /* TOK_STRING  */
    TOK_CHAR = 266,                /* TOK_CHAR  */
    TRUE = 267,                    /* TRUE  */
    FALSE = 268,                   /* FALSE  */
    PIPE = 269,                    /* PIPE  */
    EXTERN = 270,                  /* EXTERN  */
    TRIPLE_DOT = 271,              /* TRIPLE_DOT  */
    DOUBLE_DOT = 272,              /* DOUBLE_DOT  */
    LET = 273,                     /* LET  */
    FN = 274,                      /* FN  */
    MODULE = 275,                  /* MODULE  */
    MATCH = 276,                   /* MATCH  */
    WITH = 277,                    /* WITH  */
    ARROW = 278,                   /* ARROW  */
    DOUBLE_COLON = 279,            /* DOUBLE_COLON  */
    TOK_VOID = 280,                /* TOK_VOID  */
    IN = 281,                      /* IN  */
    AND = 282,                     /* AND  */
    ASYNC = 283,                   /* ASYNC  */
    DOUBLE_AT = 284,               /* DOUBLE_AT  */
    AT = 285,                      /* AT  */
    THUNK = 286,                   /* THUNK  */
    IMPORT = 287,                  /* IMPORT  */
    OPEN = 288,                    /* OPEN  */
    IMPLEMENTS = 289,              /* IMPLEMENTS  */
    AMPERSAND = 290,               /* AMPERSAND  */
    TYPE = 291,                    /* TYPE  */
    TEST_ID = 292,                 /* TEST_ID  */
    MUT = 293,                     /* MUT  */
    THEN = 294,                    /* THEN  */
    ELSE = 295,                    /* ELSE  */
    YIELD = 296,                   /* YIELD  */
    AWAIT = 297,                   /* AWAIT  */
    FOR = 298,                     /* FOR  */
    IF = 299,                      /* IF  */
    OF = 300,                      /* OF  */
    FSTRING_START = 301,           /* FSTRING_START  */
    FSTRING_END = 302,             /* FSTRING_END  */
    FSTRING_INTERP_START = 303,    /* FSTRING_INTERP_START  */
    FSTRING_INTERP_END = 304,      /* FSTRING_INTERP_END  */
    FSTRING_TEXT = 305,            /* FSTRING_TEXT  */
    MATCH_BODY_PREC = 306,         /* MATCH_BODY_PREC  */
    DOUBLE_AMP = 307,              /* DOUBLE_AMP  */
    DOUBLE_PIPE = 308,             /* DOUBLE_PIPE  */
    GE = 309,                      /* GE  */
    LE = 310,                      /* LE  */
    EQ = 311,                      /* EQ  */
    NE = 312,                      /* NE  */
    MODULO = 313,                  /* MODULO  */
    APPLICATION = 314,             /* APPLICATION  */
    UMINUS = 315                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define INTEGER 258
#define UINT64 259
#define DOUBLE 260
#define FLOAT 261
#define IDENTIFIER 262
#define PATH_IDENTIFIER 263
#define IDENTIFIER_LIST 264
#define TOK_STRING 265
#define TOK_CHAR 266
#define TRUE 267
#define FALSE 268
#define PIPE 269
#define EXTERN 270
#define TRIPLE_DOT 271
#define DOUBLE_DOT 272
#define LET 273
#define FN 274
#define MODULE 275
#define MATCH 276
#define WITH 277
#define ARROW 278
#define DOUBLE_COLON 279
#define TOK_VOID 280
#define IN 281
#define AND 282
#define ASYNC 283
#define DOUBLE_AT 284
#define AT 285
#define THUNK 286
#define IMPORT 287
#define OPEN 288
#define IMPLEMENTS 289
#define AMPERSAND 290
#define TYPE 291
#define TEST_ID 292
#define MUT 293
#define THEN 294
#define ELSE 295
#define YIELD 296
#define AWAIT 297
#define FOR 298
#define IF 299
#define OF 300
#define FSTRING_START 301
#define FSTRING_END 302
#define FSTRING_INTERP_START 303
#define FSTRING_INTERP_END 304
#define FSTRING_TEXT 305
#define MATCH_BODY_PREC 306
#define DOUBLE_AMP 307
#define DOUBLE_PIPE 308
#define GE 309
#define LE 310
#define EQ 311
#define NE 312
#define MODULO 313
#define APPLICATION 314
#define UMINUS 315

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 33 "lang/parser.y"

    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    uint64_t vint64;                   /* int val */
    double vdouble;
    float vfloat;
    char vchar;

#line 198 "lang/y.tab.h"

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
