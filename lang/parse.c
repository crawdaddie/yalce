#include "parse.h"
#include "serde.h"
#include <stdlib.h>
#include <string.h>

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

Ast *ast_identifier(ObjString id) {
  const char *name = id.chars;
  int length = id.length;
  Ast *node = Ast_new(AST_IDENTIFIER);
  node->data.AST_IDENTIFIER.value = name;
  node->data.AST_IDENTIFIER.length = length;
  return node;
}

Ast *ast_let(ObjString name, Ast *expr, Ast *in_continuation) {
  Ast *node = Ast_new(AST_LET);
  node->data.AST_LET.name = name;
  if (expr->tag == AST_LAMBDA) {
    expr->data.AST_LAMBDA.fn_name = name;
  }
  // else if (expr->tag == AST_EXTERN_FN) {
  //   expr->data.AST_EXTERN_FN.fn_name = name;
  // }
  node->data.AST_LET.expr = expr;
  node->data.AST_LET.in_expr = in_continuation;
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
  if (func->tag == AST_IDENTIFIER) {
    Ast *app = Ast_new(AST_APPLICATION);
    app->data.AST_APPLICATION.function = func;
    app->data.AST_APPLICATION.args = malloc(sizeof(Ast));
    app->data.AST_APPLICATION.args[0] = *arg;
    app->data.AST_APPLICATION.len = 1;
    return app;
  }
  if (func->tag == AST_APPLICATION) {
    Ast *args = func->data.AST_APPLICATION.args;
    func->data.AST_APPLICATION.len++;
    int len = func->data.AST_APPLICATION.len;

    func->data.AST_APPLICATION.args = realloc(args, sizeof(Ast) * len);
    func->data.AST_APPLICATION.args[len - 1] = *arg;
    return func;
  }
  return NULL;
}

Ast *ast_lambda(Ast *lambda, Ast *body) {
  if (lambda == NULL) {
    lambda = Ast_new(AST_LAMBDA);
    lambda->data.AST_LAMBDA.params = NULL;
    lambda->data.AST_LAMBDA.len = 0;
    lambda->data.AST_LAMBDA.defaults = NULL;
  }
  lambda->data.AST_LAMBDA.body = body;
  return lambda;
}

Ast *ast_arg_list(ObjString arg_id, Ast *def) {
  Ast *lambda = Ast_new(AST_LAMBDA);

  lambda->data.AST_LAMBDA.params = malloc(sizeof(ObjString));
  lambda->data.AST_LAMBDA.len = 1;
  lambda->data.AST_LAMBDA.params[0] = arg_id;
  lambda->data.AST_LAMBDA.defaults = malloc(sizeof(Ast *));

  if (def) {
    lambda->data.AST_LAMBDA.defaults[0] = def;
  }

  return lambda;
}

Ast *ast_arg_list_push(Ast *lambda, ObjString arg_id, Ast *def) {
  ObjString *params = lambda->data.AST_LAMBDA.params;
  lambda->data.AST_LAMBDA.len++;
  size_t len = lambda->data.AST_LAMBDA.len;

  lambda->data.AST_LAMBDA.params = realloc(params, sizeof(ObjString) * len);
  lambda->data.AST_LAMBDA.defaults =
      realloc(lambda->data.AST_LAMBDA.defaults, sizeof(Ast *) * len);
  lambda->data.AST_LAMBDA.params[len - 1] = arg_id;

  if (def) {
    lambda->data.AST_LAMBDA.defaults[len - 1] = def;
  }

  return lambda;
}

// Ast *ast_extern_declaration(ObjString extern_name, Ast *lambda,
//                             ObjString return_type) {
//
//   size_t len = lambda->data.AST_LAMBDA.len;
//   ObjString *params = lambda->data.AST_LAMBDA.params;
//
//   lambda->tag = AST_EXTERN_FN_DECLARATION;
//
//   lambda->data.AST_EXTERN_FN_DECLARATION.fn_name = extern_name;
//   lambda->data.AST_EXTERN_FN_DECLARATION.len = len;
//   lambda->data.AST_EXTERN_FN_DECLARATION.params = params;
//   lambda->data.AST_EXTERN_FN_DECLARATION.return_type = return_type;
//   return lambda;
// }

Ast *parse_stmt_list(Ast *stmts, Ast *new_stmt) {
  if (new_stmt == NULL) {
    return stmts;
  }

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
Ast *ast_string(ObjString lex_string) {
  Ast *s = Ast_new(AST_STRING);
  s->data.AST_STRING.value = lex_string.chars;
  s->data.AST_STRING.length = lex_string.length;
  return s;
}

Ast *parse_format_expr(ObjString fstring) {
  // TODO: split fstring on { & } and create concatenation expression with
  // identifiers in between { & }
  // eg `hello {x} and {y}` -> "hello " + (str x) + " and " + (str y)
  Ast *fmt = Ast_new(AST_APPLICATION);
  Ast *func_id = Ast_new(AST_IDENTIFIER);
  func_id->data.AST_IDENTIFIER.value = strdup("_concat");
  fmt->data.AST_APPLICATION.function = func_id;
  char *ch = fstring.chars;

  // while (*ch != '\0') {
  //   ch++;
  // }

  // struct AST_APPLICATION {
  //   Ast *function;
  //   Ast **args;
  //   int len;
  //   // size_t num_args;
  // } AST_APPLICATION;

  return ast_string(fstring);
}

Ast *ast_empty_list() {
  Ast *l = Ast_new(AST_LIST);
  l->data.AST_LIST.len = 0;
  l->data.AST_LIST.items = NULL;
  return l;
}

Ast *ast_list(Ast *val) {
  Ast *l = Ast_new(AST_LIST);
  l->data.AST_LIST.len = 1;
  l->data.AST_LIST.items = val;
  return l;
}
Ast *ast_list_push(Ast *list, Ast *val) {
  Ast *items = list->data.AST_LIST.items;
  list->data.AST_LIST.len++;
  int len = list->data.AST_LIST.len;

  list->data.AST_LIST.items = realloc(items, sizeof(Ast) * len);
  list->data.AST_LIST.items[len - 1] = *val;
  return list;
}

Ast *ast_match(Ast *expr, Ast *match) {
  match->data.AST_MATCH.expr = expr;
  return match;
}
Ast *ast_match_branches(Ast *match, Ast *expr, Ast *result) {
  if (match == NULL) {
    match = Ast_new(AST_MATCH);
    match->data.AST_MATCH.branches = malloc(sizeof(Ast) * 2);
    match->data.AST_MATCH.branches[0] = *expr;
    match->data.AST_MATCH.branches[1] = *result;
    match->data.AST_MATCH.len = 1;
    return match;
  }

  Ast *branches = match->data.AST_MATCH.branches;
  // ite->data.AST_LIST.len++;
  match->data.AST_MATCH.len++;
  int len = match->data.AST_MATCH.len;

  match->data.AST_MATCH.branches = realloc(branches, sizeof(Ast) * len * 2);
  match->data.AST_MATCH.branches[len * 2 - 2] = *expr;
  match->data.AST_MATCH.branches[len * 2 - 1] = *result;
  return match;
}

Ast *ast_tuple(Ast *list) {
  // if (list->tag == AST_LIST && list->data.AST_LIST.len == 1) {
  //   return list->data.AST_LIST.items;
  // }
  // print_ast(list);

  if (list->tag == AST_LIST) {
    list->tag = AST_TUPLE;
    return list;
  }
  return NULL;
}
Ast *ast_meta(ObjString meta_id, Ast *next) {
  Ast *meta = Ast_new(AST_META);
  meta->data.AST_META.value = meta_id.chars;
  meta->data.AST_META.length = meta_id.length;
  meta->data.AST_META.next = next;
  return meta;
}

Ast *ast_extern_fn(ObjString name, Ast *signature) {
  int len = signature->data.AST_LIST.len;
  Ast *param_types = signature->data.AST_LIST.items;
  signature->tag = AST_EXTERN_FN;
  signature->data.AST_EXTERN_FN.len = len;
  signature->data.AST_EXTERN_FN.signature_types = param_types;
  signature->data.AST_EXTERN_FN.fn_name = name;

  return signature;
}
Ast *ast_assoc(Ast *l, Ast *r) { return NULL; }

Ast *typed_arg_list(Ast *list, Ast *item) {
  if (list == NULL) {
    return ast_list(item);
  } else if (list->tag == AST_LIST && item != NULL) {
    return ast_list_push(list, item);
  }
  return NULL;
}

Ast *extern_typed_signature(Ast *item) {
  if (item->tag == AST_VOID) {
    return NULL;
  }
  return ast_list(item);
}

Ast *extern_typed_signature_push(Ast *sig, Ast *item) {
  if (sig == NULL) {
    return ast_list(item);
  }
  ast_list_push(sig, item);
  return sig;
}

bool ast_is_placeholder_id(Ast *ast) {
  if (ast->tag != AST_IDENTIFIER) {
    return false;
  }

  if (*(ast->data.AST_IDENTIFIER.value) == '_') {
    return true;
  }

  return false;
}
