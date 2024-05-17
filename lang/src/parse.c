#include "parse.h"
#include "serde.h"
#include <stdarg.h>
#include <stdlib.h>

Ast *con(int value) {
  nodeType *p;

  /* allocate node */
  if ((p = malloc(sizeof(nodeType))) == NULL)
    yyerror("out of memory");

  /* copy information */
  p->type = typeCon;
  p->con.value = value;

  return p;
}

Ast *_id(int i) {
  nodeType *p;

  /* allocate node */
  if ((p = malloc(sizeof(nodeType))) == NULL)
    yyerror("out of memory");

  /* copy information */
  p->type = typeId;
  p->id.i = i;

  return p;
}

Ast *opr(int oper, int nops, ...) {
  va_list ap;
  nodeType *p;
  int i;

  /* allocate node, extending op array */
  if ((p = malloc(sizeof(nodeType) + (nops - 1) * sizeof(nodeType *))) == NULL)
    yyerror("out of memory");

  /* copy information */
  p->type = typeOpr;
  p->opr.oper = oper;
  p->opr.nops = nops;
  va_start(ap, nops);
  for (i = 0; i < nops; i++)
    p->opr.op[i] = va_arg(ap, nodeType *);
  va_end(ap);
  return p;
}

void freeNode(nodeType *p) {
  int i;

  if (!p)
    return;
  if (p->type == typeOpr) {
    for (i = 0; i < p->opr.nops; i++)
      freeNode(p->opr.op[i]);
  }
  free(p);
}

// void yyerror(const char *s) { fprintf(stdout, "%s\n", s); }
//

Ast *Ast_new(enum ast_tag tag) {
  Ast *node = malloc(sizeof(Ast));
  node->tag = tag;
  return node;
}

void Ast_body_push(Ast *body, Ast *stmt) {
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

Ast *ast_identifier(char *name) {
  Ast *node = Ast_new(AST_IDENTIFIER);
  node->data.AST_IDENTIFIER.value = name;
  return node;
}

Ast *ast_let(char *name, Ast *expr) {
  printf("let node: %s\n", name);
  Ast *node = Ast_new(AST_LET);
  node->data.AST_LET.name = name;
  node->data.AST_LET.expr = expr;
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

Ast *ast_lambda(Ast *args, Ast *body) {
  printf("lambda\n");
  Ast *lambda = Ast_new(AST_LAMBDA);
  lambda->data.AST_LAMBDA.len = args->data.AST_LAMBDA_ARGS.len;
  lambda->data.AST_LAMBDA.params = args->data.AST_LAMBDA_ARGS.ids;
  return lambda;
}

Ast *ast_arg_list(char *arg) {

  printf("arg list\n");
  Ast *list = Ast_new(AST_LAMBDA_ARGS);
  list->data.AST_LAMBDA_ARGS.ids = malloc(sizeof(char *));
  list->data.AST_LAMBDA_ARGS.len = 1;
  list->data.AST_LAMBDA_ARGS.ids[0] = arg;
  return list;
}
Ast *ast_arg_list_push(Ast *arg_list, char *arg) {
  char **ids = arg_list->data.AST_LAMBDA_ARGS.ids;
  arg_list->data.AST_LAMBDA_ARGS.len++;
  size_t len = arg_list->data.AST_LAMBDA_ARGS.len;

  arg_list->data.AST_LAMBDA_ARGS.ids = realloc(ids, sizeof(char *) * len);
  arg_list->data.AST_LAMBDA_ARGS.ids[len - 1] = arg;
}
