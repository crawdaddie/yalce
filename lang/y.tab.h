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
    DOUBLE_DOT = 270,              /* DOUBLE_DOT  */
    LET = 271,                     /* LET  */
    FN = 272,                      /* FN  */
    MODULE = 273,                  /* MODULE  */
    MATCH = 274,                   /* MATCH  */
    WITH = 275,                    /* WITH  */
    ARROW = 276,                   /* ARROW  */
    DOUBLE_COLON = 277,            /* DOUBLE_COLON  */
    TOK_VOID = 278,                /* TOK_VOID  */
    IN = 279,                      /* IN  */
    AND = 280,                     /* AND  */
    ASYNC = 281,                   /* ASYNC  */
    DOUBLE_AT = 282,               /* DOUBLE_AT  */
    THUNK = 283,                   /* THUNK  */
    IMPORT = 284,                  /* IMPORT  */
    OPEN = 285,                    /* OPEN  */
    IMPLEMENTS = 286,              /* IMPLEMENTS  */
    AMPERSAND = 287,               /* AMPERSAND  */
    TYPE = 288,                    /* TYPE  */
    TEST_ID = 289,                 /* TEST_ID  */
    MUT = 290,                     /* MUT  */
    THEN = 291,                    /* THEN  */
    ELSE = 292,                    /* ELSE  */
    YIELD = 293,                   /* YIELD  */
    AWAIT = 294,                   /* AWAIT  */
    FOR = 295,                     /* FOR  */
    IF = 296,                      /* IF  */
    OF = 297,                      /* OF  */
    FSTRING_START = 298,           /* FSTRING_START  */
    FSTRING_END = 299,             /* FSTRING_END  */
    FSTRING_INTERP_START = 300,    /* FSTRING_INTERP_START  */
    FSTRING_INTERP_END = 301,      /* FSTRING_INTERP_END  */
    FSTRING_TEXT = 302,            /* FSTRING_TEXT  */
    DOUBLE_AMP = 303,              /* DOUBLE_AMP  */
    DOUBLE_PIPE = 304,             /* DOUBLE_PIPE  */
    GE = 305,                      /* GE  */
    LE = 306,                      /* LE  */
    EQ = 307,                      /* EQ  */
    NE = 308,                      /* NE  */
    MODULO = 309,                  /* MODULO  */
    APPLICATION = 310,             /* APPLICATION  */
    UMINUS = 311                   /* UMINUS  */
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
#define DOUBLE_DOT 270
#define LET 271
#define FN 272
#define MODULE 273
#define MATCH 274
#define WITH 275
#define ARROW 276
#define DOUBLE_COLON 277
#define TOK_VOID 278
#define IN 279
#define AND 280
#define ASYNC 281
#define DOUBLE_AT 282
#define THUNK 283
#define IMPORT 284
#define OPEN 285
#define IMPLEMENTS 286
#define AMPERSAND 287
#define TYPE 288
#define TEST_ID 289
#define MUT 290
#define THEN 291
#define ELSE 292
#define YIELD 293
#define AWAIT 294
#define FOR 295
#define IF 296
#define OF 297
#define FSTRING_START 298
#define FSTRING_END 299
#define FSTRING_INTERP_START 300
#define FSTRING_INTERP_END 301
#define FSTRING_TEXT 302
#define DOUBLE_AMP 303
#define DOUBLE_PIPE 304
#define GE 305
#define LE 306
#define EQ 307
#define NE 308
#define MODULO 309
#define APPLICATION 310
#define UMINUS 311

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 29 "lang/parser.y"

    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    float vfloat;
    char vchar;

#line 189 "lang/y.tab.h"

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
