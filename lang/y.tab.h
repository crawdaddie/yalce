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
    FLOAT = 260,                   /* FLOAT  */
    IDENTIFIER = 261,              /* IDENTIFIER  */
    PATH_IDENTIFIER = 262,         /* PATH_IDENTIFIER  */
    IDENTIFIER_LIST = 263,         /* IDENTIFIER_LIST  */
    TOK_STRING = 264,              /* TOK_STRING  */
    TOK_CHAR = 265,                /* TOK_CHAR  */
    TRUE = 266,                    /* TRUE  */
    FALSE = 267,                   /* FALSE  */
    PIPE = 268,                    /* PIPE  */
    EXTERN = 269,                  /* EXTERN  */
    TRIPLE_DOT = 270,              /* TRIPLE_DOT  */
    DOUBLE_DOT = 271,              /* DOUBLE_DOT  */
    LET = 272,                     /* LET  */
    FN = 273,                      /* FN  */
    MODULE = 274,                  /* MODULE  */
    MATCH = 275,                   /* MATCH  */
    WITH = 276,                    /* WITH  */
    ARROW = 277,                   /* ARROW  */
    DOUBLE_COLON = 278,            /* DOUBLE_COLON  */
    TOK_VOID = 279,                /* TOK_VOID  */
    IN = 280,                      /* IN  */
    AND = 281,                     /* AND  */
    ASYNC = 282,                   /* ASYNC  */
    DOUBLE_AT = 283,               /* DOUBLE_AT  */
    AT = 284,                      /* AT  */
    THUNK = 285,                   /* THUNK  */
    IMPORT = 286,                  /* IMPORT  */
    OPEN = 287,                    /* OPEN  */
    IMPLEMENTS = 288,              /* IMPLEMENTS  */
    AMPERSAND = 289,               /* AMPERSAND  */
    TYPE = 290,                    /* TYPE  */
    TEST_ID = 291,                 /* TEST_ID  */
    MUT = 292,                     /* MUT  */
    THEN = 293,                    /* THEN  */
    ELSE = 294,                    /* ELSE  */
    YIELD = 295,                   /* YIELD  */
    AWAIT = 296,                   /* AWAIT  */
    FOR = 297,                     /* FOR  */
    IF = 298,                      /* IF  */
    OF = 299,                      /* OF  */
    FSTRING_START = 300,           /* FSTRING_START  */
    FSTRING_END = 301,             /* FSTRING_END  */
    FSTRING_INTERP_START = 302,    /* FSTRING_INTERP_START  */
    FSTRING_INTERP_END = 303,      /* FSTRING_INTERP_END  */
    FSTRING_TEXT = 304,            /* FSTRING_TEXT  */
    MATCH_BODY_PREC = 305,         /* MATCH_BODY_PREC  */
    DOUBLE_AMP = 306,              /* DOUBLE_AMP  */
    DOUBLE_PIPE = 307,             /* DOUBLE_PIPE  */
    GE = 308,                      /* GE  */
    LE = 309,                      /* LE  */
    EQ = 310,                      /* EQ  */
    NE = 311,                      /* NE  */
    MODULO = 312,                  /* MODULO  */
    APPLICATION = 313,             /* APPLICATION  */
    UMINUS = 314                   /* UMINUS  */
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
#define FLOAT 260
#define IDENTIFIER 261
#define PATH_IDENTIFIER 262
#define IDENTIFIER_LIST 263
#define TOK_STRING 264
#define TOK_CHAR 265
#define TRUE 266
#define FALSE 267
#define PIPE 268
#define EXTERN 269
#define TRIPLE_DOT 270
#define DOUBLE_DOT 271
#define LET 272
#define FN 273
#define MODULE 274
#define MATCH 275
#define WITH 276
#define ARROW 277
#define DOUBLE_COLON 278
#define TOK_VOID 279
#define IN 280
#define AND 281
#define ASYNC 282
#define DOUBLE_AT 283
#define AT 284
#define THUNK 285
#define IMPORT 286
#define OPEN 287
#define IMPLEMENTS 288
#define AMPERSAND 289
#define TYPE 290
#define TEST_ID 291
#define MUT 292
#define THEN 293
#define ELSE 294
#define YIELD 295
#define AWAIT 296
#define FOR 297
#define IF 298
#define OF 299
#define FSTRING_START 300
#define FSTRING_END 301
#define FSTRING_INTERP_START 302
#define FSTRING_INTERP_END 303
#define FSTRING_TEXT 304
#define MATCH_BODY_PREC 305
#define DOUBLE_AMP 306
#define DOUBLE_PIPE 307
#define GE 308
#define LE 309
#define EQ 310
#define NE 311
#define MODULO 312
#define APPLICATION 313
#define UMINUS 314

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 33 "lang/parser.y"

    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    float vfloat;
    char vchar;

#line 195 "lang/y.tab.h"

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
