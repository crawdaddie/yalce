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
     TRIPLE_DOT = 269,
     LET = 270,
     FN = 271,
     ARROW = 272,
     VOID = 273,
     DOUBLE_SEMICOLON = 274,
     IN = 275,
     IFX = 276,
     ELSE = 277,
     MODULO = 278,
     NE = 279,
     EQ = 280,
     LE = 281,
     GE = 282,
     UMINUS = 283
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
#define TRIPLE_DOT 269
#define LET 270
#define FN 271
#define ARROW 272
#define VOID 273
#define DOUBLE_SEMICOLON 274
#define IN 275
#define IFX 276
#define ELSE 277
#define MODULO 278
#define NE 279
#define EQ 280
#define LE 281
#define GE 282
#define UMINUS 283




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 28 "src/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    ObjString vident;               /* identifier */
    ObjString vstr;                 /* string */
    int vint;                   /* int val */
    double vfloat;
}
/* Line 1529 of yacc.c.  */
#line 113 "src/y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

