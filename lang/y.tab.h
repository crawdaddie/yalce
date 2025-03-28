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
     PATH_IDENTIFIER = 261,
     IDENTIFIER_LIST = 262,
     TOK_STRING = 263,
     TOK_CHAR = 264,
     TRUE = 265,
     FALSE = 266,
     PIPE = 267,
     EXTERN = 268,
     TRIPLE_DOT = 269,
     LET = 270,
     FN = 271,
     MATCH = 272,
     WITH = 273,
     ARROW = 274,
     DOUBLE_COLON = 275,
     TOK_VOID = 276,
     IN = 277,
     AND = 278,
     ASYNC = 279,
     DOUBLE_AT = 280,
     THUNK = 281,
     IMPORT = 282,
     OPEN = 283,
     IMPLEMENTS = 284,
     FSTRING_START = 285,
     FSTRING_END = 286,
     FSTRING_INTERP_START = 287,
     FSTRING_INTERP_END = 288,
     FSTRING_TEXT = 289,
     APPLICATION = 290,
     DOUBLE_PIPE = 291,
     DOUBLE_AMP = 292,
     NE = 293,
     EQ = 294,
     LE = 295,
     GE = 296,
     MODULO = 297,
     UMINUS = 298
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define DOUBLE 259
#define IDENTIFIER 260
#define PATH_IDENTIFIER 261
#define IDENTIFIER_LIST 262
#define TOK_STRING 263
#define TOK_CHAR 264
#define TRUE 265
#define FALSE 266
#define PIPE 267
#define EXTERN 268
#define TRIPLE_DOT 269
#define LET 270
#define FN 271
#define MATCH 272
#define WITH 273
#define ARROW 274
#define DOUBLE_COLON 275
#define TOK_VOID 276
#define IN 277
#define AND 278
#define ASYNC 279
#define DOUBLE_AT 280
#define THUNK 281
#define IMPORT 282
#define OPEN 283
#define IMPLEMENTS 284
#define FSTRING_START 285
#define FSTRING_END 286
#define FSTRING_INTERP_START 287
#define FSTRING_INTERP_END 288
#define FSTRING_TEXT 289
#define APPLICATION 290
#define DOUBLE_PIPE 291
#define DOUBLE_AMP 292
#define NE 293
#define EQ 294
#define LE 295
#define GE 296
#define MODULO 297
#define UMINUS 298




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 30 "../lang/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vdouble;
    char vchar;
}
/* Line 1529 of yacc.c.  */
#line 144 "../lang/y.tab.h"
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
