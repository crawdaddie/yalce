#ifndef _LANG_PARSE_H
#define _LANG_PARSE_H
#include "common.h"
#include <stdbool.h>
#include <stdio.h>
typedef struct Ast Ast;

extern bool top_level_tests;
extern const char *__module_to_test;

typedef struct custom_binops_t {
  const char *binop;
  struct custom_binops_t *next;
} custom_binops_t;
extern custom_binops_t *__custom_binops;

// parser prototypes
extern FILE *yyin;
extern char *yytext;
extern int yylineno;

extern int yycolumn;
extern int yyprevcolumn;
extern long long int yyabsoluteoffset;
extern long long int yyprevoffset;

extern char *_cur_script;
int yyparse();
void yyrestart(FILE *);

void yyerror(const char *s);

int yylex(void);

typedef struct loc_info {
  const char *src;
  const char *src_content;
  int line;
  int col;
  int col_end;
  long long absolute_offset;
} loc_info;

typedef struct AstList {
  Ast *ast;
  struct AstList *next;
} AstList;

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
  TOKEN_DOUBLE_COLON,
  TOKEN_SEMICOLON,
  TOKEN_DOUBLE_SEMICOLON,
  TOKEN_IN,
  TOKEN_OF,
  TOKEN_DOUBLE_AMP,
  TOKEN_DOUBLE_PIPE,
  TOKEN_RANGE_TO,
} token_type;

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
  AST_EMPTY_LIST,
  AST_ARRAY,
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
  AST_MATCH_GUARD_CLAUSE,
  AST_YIELD,
  AST_SPREAD_OP,
  AST_IMPLEMENTS,
  AST_MODULE,
  AST_RANGE_EXPRESSION,
  AST_LOOP,
  AST_TYPE_TRAIT_IMPL,
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
      bool crosses_yield_boundary;
      bool is_recursive_fn_ref;
      bool is_fn_param;
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

    struct AST_LAMBDA {
      size_t len;
      Ast *params;
      ObjString fn_name;
      Ast *body;
      Ast **type_annotations;
      bool is_coroutine;
      int num_yields;
      AstList *yield_boundary_crossers;
      int num_yield_boundary_crossers;
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

    struct AST_EMPTY_LIST {
      ObjString type_id;
    } AST_EMPTY_LIST;
    struct AST_MATCH {
      Ast *expr;
      Ast *branches;
      size_t len;
    } AST_MATCH;

    struct AST_RECORD_ACCESS {
      Ast *record;
      Ast *member;
      int index;
    } AST_RECORD_ACCESS;

    struct AST_MATCH_GUARD_CLAUSE {
      Ast *test_expr;
      Ast *guard_expr;
    } AST_MATCH_GUARD_CLAUSE;

    struct AST_YIELD {
      Ast *expr;
    } AST_YIELD;

    struct AST_SPREAD_OP {
      Ast *expr;
    } AST_SPREAD_OP;

    struct AST_IMPLEMENTS {
      ObjString type_id;
      ObjString trait_id;
    } AST_IMPLEMENTS;

    struct AST_IMPORT {
      const char *identifier;
      const char *fully_qualified_name;
      bool import_all;
    } AST_IMPORT;

    struct AST_RANGE_EXPRESSION {
      Ast *from;
      Ast *to;
    } AST_RANGE_EXPRESSION;

    // AST_LOOP has the same structure as a let binding - let binding to items
    // of the iterator and then an in expression as the body struct AST_LOOP {
    //   Ast *binding;
    //   Ast *expr;
    //   Ast *body;
    // } AST_LOOP;
    struct AST_TYPE_TRAIT_IMPL {
      char *type_name;
      char *trait_name;
      Ast *lambda;
    } AST_TYPE_TRAIT_IMPL;
  } data;

  void *md;
  void *loc_info;
  bool is_body_tail;
};

Ast *Ast_new(enum ast_tag tag);

void ast_body_push(Ast *body, Ast *stmt);

/* External declaration of ast root */
extern Ast *ast_root;

Ast *ast_binop(token_type op, Ast *left, Ast *right);
Ast *ast_unop(token_type op, Ast *right);
Ast *ast_identifier(ObjString id);
Ast *ast_let(Ast *name, Ast *expr, Ast *in_continuation);
Ast *ast_test_module(Ast *expr);
Ast *ast_application(Ast *func, Ast *arg);
Ast *ast_lambda(Ast *args, Ast *body);
Ast *ast_void_lambda(Ast *body);
Ast *ast_arg_list(Ast *arg_id, Ast *def);
Ast *ast_arg_list_push(Ast *arg_list, Ast *arg_id, Ast *def);
Ast *parse_stmt_list(Ast *stmts, Ast *new_stmt);
Ast *parse_input(char *input, const char *dirname);
Ast *parse_input_script(const char *filename);
Ast *ast_void();
Ast *ast_string(ObjString lex_string);

Ast *parse_fstring_expr(Ast *list);
Ast *ast_empty_list();
Ast *ast_typed_empty_list(ObjString id);
Ast *ast_empty_array();
Ast *ast_list_to_array(Ast *list);

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

Ast *ast_sequence(Ast *seq, Ast *new_);

// Ast *macro(Ast *expr);

Ast *ast_await(Ast *awaitable);

Ast *ast_fn_sig(Ast *, Ast *);
Ast *ast_fn_sig_push(Ast *, Ast *);

Ast *ast_tuple_type(Ast *, Ast *);
Ast *ast_tuple_type_single(Ast *a);
Ast *ast_tuple_type_push(Ast *, Ast *);

Ast *ast_cons_decl(token_type op, Ast *left, Ast *right);

Ast *ast_match_guard_clause(Ast *expr, Ast *guard);

void add_custom_binop(const char *binop_name);

void print_location(Ast *ast);

Ast *ast_yield(Ast *expr);

Ast *ast_thunk_expr(Ast *expr);
Ast *ast_spread_operator(Ast *expr);
Ast *ast_fn_signature_of_list(Ast *l);

Ast *ast_implements(ObjString, ObjString);

typedef struct AstVisitor {
  // Basic types
  void (*visit_int)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_double)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_string)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_char)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_bool)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_void)(Ast *ast, struct AstVisitor *visitor);

  // Collection types
  void (*visit_array)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_list)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_tuple)(Ast *ast, struct AstVisitor *visitor);

  // Language constructs
  void (*visit_type_decl)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_fmt_string)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_body)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_identifier)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_application)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_let)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_lambda)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_match)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_extern_fn)(Ast *ast, struct AstVisitor *visitor);
  void (*visit_yield)(Ast *ast, struct AstVisitor *visitor);

  // Default handler
  void (*visit_default)(Ast *ast, struct AstVisitor *visitor);

  // Optional: visitor-specific data
  void *data; // Generic pointer for visitor-specific state
} AstVisitor;
Ast *ast_module(Ast *lambda);
extern char *__import_current_dir;
Ast *ast_import_stmt(ObjString path_identifier, bool import_all);
Ast *ast_for_loop(Ast *binding, Ast *iter_expr, Ast *body);

Ast *ast_range_expression(Ast *from, Ast *to);
Ast *ast_typeclass_impl(char *type_name, char *tc_name, Ast *lambda);
#endif
