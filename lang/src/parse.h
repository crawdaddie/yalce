#ifndef _LANG_PARSE_H
#define _LANG_PARSE_H
#include "lex.h"
#include <stdbool.h>
#include <stddef.h>
typedef struct {
  Lexer *lexer;
  token previous;
  token current;
} Parser;

typedef struct Ast Ast;
typedef enum ast_tag {
  AST_BODY,
  AST_LET,
  AST_INT,
  AST_NUMBER,
  AST_STRING,
  AST_BOOL,
  AST_IDENTIFIER,
  AST_BINOP,
  AST_UNOP,
  AST_APPLICATION,
  AST_TUPLE,
  AST_FN_DECLARATION
} ast_tag;

struct Ast {
  ast_tag tag;
  union {
    struct AST_BODY {
      size_t len;
      Ast **members;
    } AST_BODY;

    struct AST_LET {
      char *name;
      Ast *expr;
    } AST_LET;
    struct AST_INT {
      int value;
    } AST_INT;

    struct AST_NUMBER {
      double value;
    } AST_NUMBER;

    struct AST_STRING {
      char *value;
    } AST_STRING;

    struct AST_IDENTIFIER {
      char *value;
    } AST_IDENTIFIER;

    struct AST_BOOL {
      bool value;
    } AST_BOOL;

    struct AST_UNOP {
      token_type op;
      Ast *expr;
    } AST_UNOP;

    struct AST_BINOP {
      token_type op;
      Ast *left;
      Ast *right;
    } AST_BINOP;

    struct AST_APPLICATION {
      Ast *applicable;
      Ast *arg;
    } AST_APPLICATION;

    struct _AST_APPLICATION {
      size_t len;
      Ast **args;
    } _AST_APPLICATION;

    struct AST_TUPLE {
      size_t len;
      Ast **members;
    } AST_TUPLE;

    struct AST_FN_DECLARATION {
      size_t params_len;
      const char **params;
      const char *fn_name;
      Ast *body;
    } AST_FN_DECLARATION;
  } data;
};

void init_parser(Parser *parser, Lexer *lexer);
void advance();

Ast *rec_parse(Ast *ast);

Ast *parse_body(Ast *body);

Ast *Ast_new(enum ast_tag tag);
Ast *body_return(Ast *body);

typedef enum ParserPrecedence {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // OR
  PREC_AND,        // AND
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_PIPE,       // ->
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_INDEX,      // []
  PREC_PRIMARY,
} ParserPrecedence;
#endif
