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
     STRING = 261,
     TRUE = 262,
     FALSE = 263,
     WHILE = 264,
     IF = 265,
     PRINT = 266,
     PIPE = 267,
     EXTERN = 268,
     LET = 269,
     FN = 270,
     ARROW = 271,
     VOID = 272,
     DOUBLE_SEMICOLON = 273,
     IN = 274,
     IFX = 275,
     ELSE = 276,
     MODULO = 277,
     NE = 278,
     EQ = 279,
     LE = 280,
     GE = 281,
     UMINUS = 282
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define NUMBER 259
#define IDENTIFIER 260
#define STRING 261
#define TRUE 262
#define FALSE 263
#define WHILE 264
#define IF 265
#define PRINT 266
#define PIPE 267
#define EXTERN 268
#define LET 269
#define FN 270
#define ARROW 271
#define VOID 272
#define DOUBLE_SEMICOLON 273
#define IN 274
#define IFX 275
#define ELSE 276
#define MODULO 277
#define NE 278
#define EQ 279
#define LE 280
#define GE 281
#define UMINUS 282




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 27 "src/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    LexId vident;               /* identifier */
    LexString vstr;                 /* string */
    int vint;                   /* int val */
    double vfloat;
}
/* Line 1529 of yacc.c.  */
#line 111 "src/y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

