#include "parse.h"
#include <stdlib.h>

Ast *Ast_new(enum ast_tag tag) {
  Ast *node = malloc(sizeof(Ast));
  node->tag = tag;
  return node;
}

void ast_body_push(Ast *body, Ast *stmt) {
  if (stmt) {
    Ast **members = body->data.AST_BODY.stmts;
    body->data.AST_BODY.len++;
    int len = body->data.AST_BODY.len;

    body->data.AST_BODY.stmts = realloc(members, sizeof(Ast *) * len);
    body->data.AST_BODY.stmts[len - 1] = stmt;
  }
}

Ast *ast_binop(token_type op, Ast *left, Ast *right) {
  Ast *node = Ast_new(AST_BINOP);
  node->data.AST_BINOP.op = op;
  node->data.AST_BINOP.left = left;
  node->data.AST_BINOP.right = right;
  return node;
}

Ast *ast_unop(token_type op, Ast *right) {
  Ast *node = Ast_new(AST_UNOP);
  node->data.AST_BINOP.op = op;
  node->data.AST_BINOP.right = right;
  return node;
}

Ast *ast_identifier(LexId lex_id) {
  char *name = lex_id.chars;
  int length = lex_id.length;
  Ast *node = Ast_new(AST_IDENTIFIER);
  node->data.AST_IDENTIFIER.value = name;
  node->data.AST_IDENTIFIER.length = length;
  return node;
}

Ast *ast_let(LexId name, Ast *expr) {
  Ast *node = Ast_new(AST_LET);
  node->data.AST_LET.name = name;
  if (expr->tag == AST_LAMBDA) {
    expr->data.AST_LAMBDA.fn_name = name;
  }
  node->data.AST_LET.expr = expr;
  // print_ast(node);
  return node;
}

void yy_scan_string(char *);
/* Define the parsing function */
Ast *parse_input(char *input) {
  yy_scan_string(input); // Set the input for the lexer
  yyparse();             // Parse the input

  return ast_root; // Placeholder
}

Ast *ast_application(Ast *func, Ast *arg) {
  Ast *app = Ast_new(AST_APPLICATION);
  app->data.AST_APPLICATION.function = func;
  app->data.AST_APPLICATION.arg = arg;
  return app;
}

Ast *ast_lambda(Ast *lambda, Ast *body) {
  if (lambda == NULL) {
    lambda = Ast_new(AST_LAMBDA);
    lambda->data.AST_LAMBDA.params = NULL;
    lambda->data.AST_LAMBDA.len = 0;
  }
  lambda->data.AST_LAMBDA.body = body;
  return lambda;
}

Ast *ast_arg_list(LexId arg) {
  Ast *lambda = Ast_new(AST_LAMBDA);
  lambda->data.AST_LAMBDA.params = malloc(sizeof(LexId));
  lambda->data.AST_LAMBDA.len = 1;
  lambda->data.AST_LAMBDA.params[0] = arg;
  return lambda;
}

Ast *ast_arg_list_push(Ast *lambda, LexId arg) {
  LexId *params = lambda->data.AST_LAMBDA.params;
  lambda->data.AST_LAMBDA.len++;
  size_t len = lambda->data.AST_LAMBDA.len;

  lambda->data.AST_LAMBDA.params = realloc(params, sizeof(LexId) * len);
  lambda->data.AST_LAMBDA.params[len - 1] = arg;
  return lambda;
}

Ast *parse_stmt_list(Ast *stmts, Ast *new_stmt) {
  if (stmts->tag == AST_BODY) {
    ast_body_push(stmts, new_stmt);
    return stmts;
  }

  Ast *body = Ast_new(AST_BODY);
  body->data.AST_BODY.stmts = malloc(sizeof(Ast *));
  ast_body_push(body, stmts);
  ast_body_push(body, new_stmt);
  return body;
}
Ast *ast_void() { return Ast_new(AST_VOID); }
Ast *ast_string(LexString lex_string) {
  Ast *s = Ast_new(AST_STRING);
  s->data.AST_STRING.value = lex_string.chars;
  s->data.AST_STRING.length = lex_string.length;
  return s;
}
