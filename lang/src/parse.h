#ifndef _LANG_PARSE_H
#define _LANG_PARSE_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
typedef enum { typeCon, typeId, typeOpr } nodeEnum;

typedef struct Ast Ast;

/* constants */
typedef struct {
  int value; /* value of constant */
} conNodeType;

/* identifiers */
typedef struct {
  int i; /* subscript to sym array */
} idNodeType;

/* operators */
typedef struct {
  int oper;                  /* operator */
  int nops;                  /* number of operands */
  struct nodeTypeTag *op[1]; /* operands, extended at runtime */
} oprNodeType;

typedef struct nodeTypeTag {
  nodeEnum type; /* type of node */

  union {
    conNodeType con; /* constants */
    idNodeType id;   /* identifiers */
    oprNodeType opr; /* operators */
  };
} nodeType;

extern int sym[26];

// parser prototypes
extern FILE *yyin;
int yyparse();
void yyrestart(FILE *);

void yyerror(const char *s);

int yylex(void);

Ast *opr(int oper, int nops, ...);
Ast *_id(int i);
Ast *con(int value);

void freeNode(nodeType *p);
int ex(nodeType *p);

typedef enum token_type {
  TOKEN_START, // dummy token
  TOKEN_LP,    // parens
  TOKEN_RP,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_LEFT_SQ,
  TOKEN_RIGHT_SQ,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_TRIPLE_DOT,
  // OPERATORS
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_MODULO,
  TOKEN_LT,
  TOKEN_GT,
  TOKEN_LTE,
  TOKEN_GTE,
  TOKEN_EQUALITY,
  TOKEN_NOT_EQUAL,
  // finish operators

  TOKEN_BANG,
  TOKEN_ASSIGNMENT,
  TOKEN_NL,    // statement terminator
  TOKEN_PIPE,  // special pipe operator |>
  TOKEN_ARROW, // special fn arrow operator |>
  TOKEN_IDENTIFIER,
  TOKEN_STRING, // literal
  TOKEN_NUMBER,
  TOKEN_INTEGER,
  TOKEN_FN, // keywords
  TOKEN_RETURN,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_LET,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_NIL, // end keywords
  TOKEN_COMMENT,
  TOKEN_WS,
  TOKEN_ERROR,
  TOKEN_EOF,
  TOKEN_BAR,
  TOKEN_MATCH,
  TOKEN_EXTERN,
  TOKEN_STRUCT,
  TOKEN_TYPE,
  TOKEN_IMPORT,
  TOKEN_AMPERSAND,
  TOKEN_LOGICAL_AND,
  TOKEN_LOGICAL_OR,
  TOKEN_QUESTION,
  TOKEN_COLON,
  TOKEN_SEMICOLON,
  TOKEN_DOUBLE_SEMICOLON,
  TOKEN_IN,
} token_type;

typedef enum ast_tag {
  AST_INT,
  AST_NUMBER,
  AST_STRING,
  AST_BOOL,
  AST_IDENTIFIER,
  AST_BODY,
  AST_LET,
  AST_BINOP,
  AST_UNOP,
  AST_APPLICATION,
  AST_TUPLE,
  AST_FN_DECLARATION,
  AST_LAMBDA,
  AST_LAMBDA_ARGS
} ast_tag;

struct Ast {
  ast_tag tag;
  union {
    struct AST_BODY {
      size_t len;
      Ast **stmts;
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
      Ast *function;
      Ast *arg;
      // size_t num_args;
    } AST_APPLICATION;

    struct AST_TUPLE {
      size_t len;
      Ast **members;
    } AST_TUPLE;

    struct AST_FN_DECLARATION {
      size_t len;
      const char **params;
      const char *fn_name;
      Ast *body;
    } AST_FN_DECLARATION;

    struct AST_LAMBDA {
      size_t len;
      const char **params;
      const char *fn_name;
      Ast *body;
    } AST_LAMBDA;
    struct AST_LAMBDA_ARGS {
      char **ids;
      size_t len;
    } AST_LAMBDA_ARGS;
  } data;
};

Ast *Ast_new(enum ast_tag tag);

void Ast_body_push(Ast *body, Ast *stmt);

/* External declaration of ast root */
extern Ast *ast_root;

Ast *ast_binop(token_type op, Ast *left, Ast *right);
Ast *ast_unop(token_type op, Ast *right);
Ast *ast_identifier(char *name);
Ast *ast_let(char *name, Ast *expr);
Ast *ast_application(Ast *func, Ast *arg);
Ast *ast_lambda(Ast *args, Ast *body);
Ast *ast_arg_list(char *arg);
Ast *ast_arg_list_push(Ast *arg_list, char *arg);

Ast *parse_input(char *input);
#endif
