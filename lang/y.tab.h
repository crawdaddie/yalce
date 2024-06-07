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
     NUMBER = 259,
     IDENTIFIER = 260,
     META_IDENTIFIER = 261,
     TOK_STRING = 262,
     FSTRING = 263,
     TRUE = 264,
     FALSE = 265,
     WHILE = 266,
     IF = 267,
     PRINT = 268,
     PIPE = 269,
     EXTERN = 270,
     TRIPLE_DOT = 271,
     LET = 272,
     FN = 273,
     MATCH = 274,
     WITH = 275,
     ARROW = 276,
     TOK_VOID = 277,
     IN = 278,
     AND = 279,
     IFX = 280,
     ELSE = 281,
     MODULO = 282,
     NE = 283,
     EQ = 284,
     LE = 285,
     GE = 286,
     UMINUS = 287
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define NUMBER 259
#define IDENTIFIER 260
#define META_IDENTIFIER 261
#define TOK_STRING 262
#define FSTRING 263
#define TRUE 264
#define FALSE 265
#define WHILE 266
#define IF 267
#define PRINT 268
#define PIPE 269
#define EXTERN 270
#define TRIPLE_DOT 271
#define LET 272
#define FN 273
#define MATCH 274
#define WITH 275
#define ARROW 276
#define TOK_VOID 277
#define IN 278
#define AND 279
#define IFX 280
#define ELSE 281
#define MODULO 282
#define NE 283
#define EQ 284
#define LE 285
#define GE 286
#define UMINUS 287




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 27 "lang/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;           /* identifier */
    ObjString vstr;             /* string */
    int vint;                   /* int val */
    double vfloat;
}
/* Line 1529 of yacc.c.  */
#line 121 "lang/y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

