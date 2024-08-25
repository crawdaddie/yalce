#ifndef _LANG_PARSE_H
#define _LANG_PARSE_H
#include "common.h"
#include "types/type.h"
#include <stdbool.h>
#include <stdio.h>
typedef struct Ast Ast;

// parser prototypes
extern FILE *yyin;
extern char *yytext;
extern int yylineno;
int yyparse();
void yyrestart(FILE *);

void yyerror(const char *s);

int yylex(void);

typedef enum ast_tag {
  AST_INT,
  AST_DOUBLE,
  AST_STRING,
  AST_CHAR,
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
  AST_IMPORT,
  AST_RECORD_ACCESS,
  AST_FMT_STRING,
  AST_TYPE_DECL,
  AST_ASSOC,
  AST_EXTERN_VARIANTS,
  AST_FN_SIGNATURE,
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

    struct AST_DOUBLE {
      double value;
    } AST_DOUBLE;

    struct AST_STRING {
      const char *value;
      size_t length;
    } AST_STRING;

    struct AST_CHAR {
      char value;
    } AST_CHAR;

    struct AST_IDENTIFIER {
      const char *value;
      size_t length;
    } AST_IDENTIFIER;

    struct AST_META {
      char *value;
      size_t length;
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
      size_t len;
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
      bool is_async;
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
      size_t len;
    } AST_MATCH;

    struct AST_IMPORT {
      const char *module_name;
    } AST_IMPORT;

    struct AST_RECORD_ACCESS {
      Ast *record;
      Ast *member;
    } AST_RECORD_ACCESS;
  } data;

  Type md;
  int lineno;
  int col;
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
Ast *ast_void_lambda(Ast *body);
Ast *ast_arg_list(Ast *arg_id, Ast *def);
Ast *ast_arg_list_push(Ast *arg_list, Ast *arg_id, Ast *def);
Ast *parse_stmt_list(Ast *stmts, Ast *new_stmt);
Ast *parse_input(char *input, const char *dirname);
Ast *ast_void();
Ast *ast_string(ObjString lex_string);

Ast *parse_fstring_expr(Ast *list);
Ast *ast_empty_list();
Ast *ast_list(Ast *val);
Ast *ast_list_push(Ast *list, Ast *val);

Ast *ast_match(Ast *expr, Ast *match);
Ast *ast_match_branches(Ast *match, Ast *expr, Ast *result);
Ast *ast_tuple(Ast *list);
Ast *ast_meta(ObjString meta_id, Ast *next);
Ast *ast_extern_decl(Ast);
Ast *ast_assoc(Ast *l, Ast *r);
Ast *ast_list_prepend(Ast *l, Ast *r);
Ast *typed_arg_list(Ast *list, Ast *item);

Ast *extern_typed_signature(Ast *item);
Ast *extern_typed_signature_push(Ast *sig, Ast *def);
Ast *ast_extern_fn(ObjString name, Ast *signature);
bool ast_is_placeholder_id(Ast *ast);

Ast *ast_placeholder();

int get_let_binding_name(Ast *ast, ObjString *name);

Ast *ast_bare_import(ObjString module_name);
Ast *ast_record_access(Ast *record, Ast *member);

Ast *ast_char(char ch);

Ast *ast_sequence(Ast *seq, Ast *new);

Ast *ast_assoc_extern(Ast *l, ObjString name);
// Ast *macro(Ast *expr);
void handle_macro(Ast *root, const char *macro_text);

Ast *ast_await(Ast *awaitable);

Ast *ast_fn_sig(Ast *, Ast *);
Ast *ast_fn_sig_push(Ast *, Ast *);

Ast *ast_tuple_type(Ast *, Ast *);
Ast *ast_tuple_type_push(Ast *, Ast *);

#endif
