/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INTEGER = 258,
     DOUBLE = 259,
     FLOAT = 260,
     IDENTIFIER = 261,
     PATH_IDENTIFIER = 262,
     IDENTIFIER_LIST = 263,
     TOK_STRING = 264,
     TOK_CHAR = 265,
     TRUE = 266,
     FALSE = 267,
     PIPE = 268,
     EXTERN = 269,
     DOUBLE_DOT = 270,
     LET = 271,
     FN = 272,
     MODULE = 273,
     MATCH = 274,
     WITH = 275,
     ARROW = 276,
     DOUBLE_COLON = 277,
     TOK_VOID = 278,
     IN = 279,
     AND = 280,
     ASYNC = 281,
     DOUBLE_AT = 282,
     THUNK = 283,
     IMPORT = 284,
     OPEN = 285,
     IMPLEMENTS = 286,
     AMPERSAND = 287,
     TYPE = 288,
     TEST_ID = 289,
     MUT = 290,
     THEN = 291,
     ELSE = 292,
     YIELD = 293,
     AWAIT = 294,
     FOR = 295,
     IF = 296,
     OF = 297,
     FSTRING_START = 298,
     FSTRING_END = 299,
     FSTRING_INTERP_START = 300,
     FSTRING_INTERP_END = 301,
     FSTRING_TEXT = 302,
     DOUBLE_PIPE = 303,
     DOUBLE_AMP = 304,
     NE = 305,
     EQ = 306,
     LE = 307,
     GE = 308,
     MODULO = 309,
     APPLICATION = 310,
     UMINUS = 311
   };
#endif
/* Tokens.  */
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
#define DOUBLE_PIPE 303
#define DOUBLE_AMP 304
#define NE 305
#define EQ 306
#define LE 307
#define GE 308
#define MODULO 309
#define APPLICATION 310
#define UMINUS 311




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 29 "lang/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    float vfloat;
    char vchar;
}
/* Line 1529 of yacc.c.  */
#line 171 "lang/y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE yylloc;
