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
     TOK_STRING = 261,
     TOK_CHAR = 262,
     TRUE = 263,
     FALSE = 264,
     PIPE = 265,
     EXTERN = 266,
     TRIPLE_DOT = 267,
     LET = 268,
     FN = 269,
     MATCH = 270,
     WITH = 271,
     ARROW = 272,
     DOUBLE_COLON = 273,
     TOK_VOID = 274,
     IN = 275,
     AND = 276,
     ASYNC = 277,
     DOUBLE_AT = 278,
     DOUBLE_AMP = 279,
     DOUBLE_PIPE = 280,
     THUNK = 281,
     IMPORT = 282,
     FSTRING_START = 283,
     FSTRING_END = 284,
     FSTRING_INTERP_START = 285,
     FSTRING_INTERP_END = 286,
     FSTRING_TEXT = 287,
     APPLICATION = 288,
     NE = 289,
     EQ = 290,
     LE = 291,
     GE = 292,
     MODULO = 293,
     UMINUS = 294
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define DOUBLE 259
#define IDENTIFIER 260
#define TOK_STRING 261
#define TOK_CHAR 262
#define TRUE 263
#define FALSE 264
#define PIPE 265
#define EXTERN 266
#define TRIPLE_DOT 267
#define LET 268
#define FN 269
#define MATCH 270
#define WITH 271
#define ARROW 272
#define DOUBLE_COLON 273
#define TOK_VOID 274
#define IN 275
#define AND 276
#define ASYNC 277
#define DOUBLE_AT 278
#define DOUBLE_AMP 279
#define DOUBLE_PIPE 280
#define THUNK 281
#define IMPORT 282
#define FSTRING_START 283
#define FSTRING_END 284
#define FSTRING_INTERP_START 285
#define FSTRING_INTERP_END 286
#define FSTRING_TEXT 287
#define APPLICATION 288
#define NE 289
#define EQ 290
#define LE 291
#define GE 292
#define MODULO 293
#define UMINUS 294




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 30 "lang/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    char vchar;
}
/* Line 1529 of yacc.c.  */
#line 136 "lang/y.tab.h"
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
