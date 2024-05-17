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
     token = 259,
     NUMBER = 260,
     IDENTIFIER = 261,
     STRING = 262,
     TRUE = 263,
     FALSE = 264,
     WHILE = 265,
     IF = 266,
     PRINT = 267,
     PIPE = 268,
     LET = 269,
     FN = 270,
     ARROW = 271,
     IFX = 272,
     ELSE = 273,
     MODULO = 274,
     NE = 275,
     EQ = 276,
     LE = 277,
     GE = 278,
     UMINUS = 279
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define token 259
#define NUMBER 260
#define IDENTIFIER 261
#define STRING 262
#define TRUE 263
#define FALSE 264
#define WHILE 265
#define IF 266
#define PRINT 267
#define PIPE 268
#define LET 269
#define FN 270
#define ARROW 271
#define IFX 272
#define ELSE 273
#define MODULO 274
#define NE 275
#define EQ 276
#define LE 277
#define GE 278
#define UMINUS 279




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 27 "src/parser.y"
{
    Ast *ast_node_ptr;          /* node pointer */
    char *vident;               /* identifier */
    char *vstr;                 /* string */
    int vint;                   /* int val */
    double vfloat;
}
/* Line 1529 of yacc.c.  */
#line 105 "src/y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

