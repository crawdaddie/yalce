#ifndef _LANG_PARSE_H
#define _LANG_PARSE_H
#include "common.h"
#include <stdbool.h>
#include <stdio.h>
typedef struct Ast Ast;

// parser prototypes
extern FILE *yyin;
extern char *yytext;
int yyparse();
void yyrestart(FILE *);

void yyerror(const char *s);

int yylex(void);

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
  AST_LAMBDA,
  AST_LAMBDA_ARGS,
  AST_VOID,
  AST_EXTERN_FN,
  AST_LIST,
  AST_MATCH,
  AST_PLACEHOLDER_ID,
  AST_META,
} ast_tag;

struct Ast {
  ast_tag tag;
  union {
    struct AST_BODY {
      size_t len;
      Ast **stmts;
    } AST_BODY;

    struct AST_LET {
      Ast *binding;
      Ast *expr;
      Ast *in_expr;
    } AST_LET;

    struct AST_INT {
      int value;
    } AST_INT;

    struct AST_NUMBER {
      double value;
    } AST_NUMBER;

    struct AST_STRING {
      char *value;
      int length;
    } AST_STRING;

    struct AST_IDENTIFIER {
      const char *value;
      int length;
    } AST_IDENTIFIER;

    struct AST_META {
      char *value;
      int length;
      Ast *next;
    } AST_META;

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
      Ast *args;
      int len;
      // size_t num_args;
    } AST_APPLICATION;

    struct AST_TUPLE {
      size_t len;
      Ast **members;
    } AST_TUPLE;

    struct AST_LAMBDA {
      size_t len;
      Ast *params;
      ObjString fn_name;
      Ast *body;
      Ast **defaults;
    } AST_LAMBDA;

    struct AST_EXTERN_FN {
      size_t len;
      Ast *signature_types;
      ObjString fn_name;
    } AST_EXTERN_FN;

    struct AST_LAMBDA_ARGS {
      Ast **ids;
      size_t len;
    } AST_LAMBDA_ARGS;

    struct AST_LIST {
      Ast *items;
      size_t len;
    } AST_LIST;

    struct AST_MATCH {
      Ast *expr;
      Ast *branches;
      int len;
    } AST_MATCH;

  } data;
  void *md;
};

Ast *Ast_new(enum ast_tag tag);

void ast_body_push(Ast *body, Ast *stmt);

/* External declaration of ast root */
extern Ast *ast_root;

Ast *ast_binop(token_type op, Ast *left, Ast *right);
Ast *ast_unop(token_type op, Ast *right);
Ast *ast_identifier(ObjString id);
Ast *ast_let(Ast *name, Ast *expr, Ast *in_continuation);
Ast *ast_application(Ast *func, Ast *arg);
Ast *ast_lambda(Ast *args, Ast *body);
Ast *ast_arg_list(Ast *arg_id, Ast *def);
Ast *ast_arg_list_push(Ast *arg_list, Ast *arg_id, Ast *def);
Ast *parse_stmt_list(Ast *stmts, Ast *new_stmt);
Ast *parse_input(char *input);
Ast *ast_void();
Ast *ast_string(ObjString lex_string);
Ast *ast_extern_declaration(ObjString extern_name, Ast *arg_list,
                            ObjString return_type);

Ast *parse_format_expr(ObjString fstring);
Ast *ast_empty_list();
Ast *ast_list(Ast *val);
Ast *ast_list_push(Ast *list, Ast *val);

Ast *ast_match(Ast *expr, Ast *match);
Ast *ast_match_branches(Ast *match, Ast *expr, Ast *result);
Ast *ast_tuple(Ast *list);
Ast *ast_meta(ObjString meta_id, Ast *next);
Ast *ast_extern_decl(Ast);
Ast *ast_assoc(Ast *l, Ast *r);
Ast *typed_arg_list(Ast *list, Ast *item);

Ast *extern_typed_signature(Ast *item);
Ast *extern_typed_signature_push(Ast *sig, Ast *def);
Ast *ast_extern_fn(ObjString name, Ast *signature);
bool ast_is_placeholder_id(Ast *ast);
int get_let_binding_name(Ast *ast, ObjString *name);

#endif
