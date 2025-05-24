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
     IDENTIFIER = 260,
     MACRO_IDENTIFIER = 261,
     PATH_IDENTIFIER = 262,
     IDENTIFIER_LIST = 263,
     TOK_STRING = 264,
     TOK_CHAR = 265,
     TRUE = 266,
     FALSE = 267,
     PIPE = 268,
     EXTERN = 269,
     TRIPLE_DOT = 270,
     LET = 271,
     FN = 272,
     MATCH = 273,
     WITH = 274,
     ARROW = 275,
     DOUBLE_COLON = 276,
     TOK_VOID = 277,
     IN = 278,
     AND = 279,
     ASYNC = 280,
     DOUBLE_AT = 281,
     THUNK = 282,
     IMPORT = 283,
     OPEN = 284,
     IMPLEMENTS = 285,
     AMPERSAND = 286,
     TYPE = 287,
     TEST_ID = 288,
     MUT = 289,
     THEN = 290,
     ELSE = 291,
     FSTRING_START = 292,
     FSTRING_END = 293,
     FSTRING_INTERP_START = 294,
     FSTRING_INTERP_END = 295,
     FSTRING_TEXT = 296,
     APPLICATION = 297,
     DOUBLE_PIPE = 298,
     DOUBLE_AMP = 299,
     NE = 300,
     EQ = 301,
     LE = 302,
     GE = 303,
     MODULO = 304,
     UMINUS = 305
   };
#endif
/* Tokens.  */
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
#define THEN 290
#define ELSE 291
#define FSTRING_START 292
#define FSTRING_END 293
#define FSTRING_INTERP_START 294
#define FSTRING_INTERP_END 295
#define FSTRING_TEXT 296
#define APPLICATION 297
#define DOUBLE_PIPE 298
#define DOUBLE_AMP 299
#define NE 300
#define EQ 301
#define LE 302
#define GE 303
#define MODULO 304
#define UMINUS 305




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 31 "../lang/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    char vchar;
}
/* Line 1529 of yacc.c.  */
#line 158 "../lang/y.tab.h"
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
