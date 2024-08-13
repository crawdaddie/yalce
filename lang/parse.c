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
  if (!stmt) {
    return;
  }

  if (stmt->tag != AST_BODY) {
    Ast **members = body->data.AST_BODY.stmts;
    body->data.AST_BODY.len++;
    int len = body->data.AST_BODY.len;

    body->data.AST_BODY.stmts = realloc(members, sizeof(Ast *) * len);
    body->data.AST_BODY.stmts[len - 1] = stmt;
    return;
  }
  if (stmt->tag == AST_BODY) {
    for (int i = 0; i < stmt->data.AST_BODY.len; i++) {
      ast_body_push(body, stmt->data.AST_BODY.stmts[i]);
    }
    return;
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

Ast *ast_let(Ast *name, Ast *expr, Ast *in_continuation) {
  Ast *node = Ast_new(AST_LET);
  node->data.AST_LET.binding = name;

  if (expr->tag == AST_LAMBDA) {

    const char *chars = name->data.AST_IDENTIFIER.value;
    int length = name->data.AST_IDENTIFIER.length;
    uint64_t hash = hash_string(chars, length);
    ObjString fn_name = {
        .chars = chars,
        .length = length,
        .hash = hash,
    };
    expr->data.AST_LAMBDA.fn_name = fn_name;
  }
  // else if (expr->tag == AST_EXTERN_FN) {
  //   expr->data.AST_EXTERN_FN.fn_name = name;
  // }
  node->data.AST_LET.expr = expr;
  node->data.AST_LET.in_expr = in_continuation;
  // print_ast(node);
  return node;
}

int yy_scan_string(char *);
/* Define the parsing function */
Ast *parse_input(char *input) {
  Ast *prev = NULL;

  if (ast_root != NULL && ast_root->data.AST_BODY.len > 0) {
    prev = ast_root;
  }

  ast_root = Ast_new(AST_BODY);
  ast_root->data.AST_BODY.len = 0;
  ast_root->data.AST_BODY.stmts = malloc(sizeof(Ast *));

  yy_scan_string(input); // Set the input for the lexer
  yyparse();             // Parse the input

  Ast *res = ast_root;
  if (prev != NULL) {
    ast_root = prev;
  }
  return res;
}

Ast *ast_application(Ast *func, Ast *arg) {
  if (func->tag == AST_APPLICATION) {
    Ast *args = func->data.AST_APPLICATION.args;
    func->data.AST_APPLICATION.len++;
    int len = func->data.AST_APPLICATION.len;

    func->data.AST_APPLICATION.args = realloc(args, sizeof(Ast) * len);
    func->data.AST_APPLICATION.args[len - 1] = *arg;
    return func;
  }
  Ast *app = Ast_new(AST_APPLICATION);
  app->data.AST_APPLICATION.function = func;
  app->data.AST_APPLICATION.args = malloc(sizeof(Ast));
  app->data.AST_APPLICATION.args[0] = *arg;
  app->data.AST_APPLICATION.len = 1;
  return app;
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

Ast *ast_arg_list(Ast *arg_id, Ast *def) {
  Ast *lambda = Ast_new(AST_LAMBDA);

  lambda->data.AST_LAMBDA.params = malloc(sizeof(Ast));
  lambda->data.AST_LAMBDA.len = 1;
  lambda->data.AST_LAMBDA.params[0] = *arg_id;
  lambda->data.AST_LAMBDA.defaults = malloc(sizeof(Ast *));

  if (def) {
    lambda->data.AST_LAMBDA.defaults[0] = def;
  }

  return lambda;
}

Ast *ast_arg_list_push(Ast *lambda, Ast *arg_id, Ast *def) {
  ObjString *params = lambda->data.AST_LAMBDA.params;
  lambda->data.AST_LAMBDA.len++;
  size_t len = lambda->data.AST_LAMBDA.len;

  lambda->data.AST_LAMBDA.params = realloc(params, sizeof(Ast) * len);
  lambda->data.AST_LAMBDA.defaults =
      realloc(lambda->data.AST_LAMBDA.defaults, sizeof(Ast *) * len);
  lambda->data.AST_LAMBDA.params[len - 1] = *arg_id;

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
Ast *parse_fstring_expr(Ast *list) {
  list->tag = AST_FMT_STRING;
  return list;
}
Ast *parse_format_expr(ObjString fstring) {
  // TODO: split fstring on { & } and create concatenation expression with
  // identifiers in between { & }
  // eg `hello {x} and {y}` -> "hello " + (str x) + " and " + (str y)

  int seg_start = 0;
  ObjString segment = {.chars = fstring.chars + seg_start};

  for (int i = 0; i < fstring.length; i++) {
    const char *curs = fstring.chars + i;
    if (*curs == '{') {
      if (i == 0) {
      } else if (*(fstring.chars + i - 1) != '\\') {
        int len = i - seg_start;
        segment.length = len - 1;
        Ast *str_segment = ast_string(segment);
        printf("pure string segment: ");
        print_ast(str_segment);

        seg_start = i;
      } else {
        continue;
      }
    } else if (*curs == '}' && i != 0 && *(fstring.chars + i - 1) != '\\') {
      printf("interpolated expression: '%.*s'\n", i - seg_start - 1,
             fstring.chars + seg_start + 1);
      segment = (ObjString){.chars = fstring.chars + i + 1};
      seg_start = i;
    } else if (i == fstring.length - 1) {
      printf("seg_start %d %d\n", seg_start, fstring.length);
      int len = i - seg_start;
      segment.length = len;
      Ast *str_segment = ast_string(segment);
      printf("pure string segment: ");
      print_ast(str_segment);
    }
  }

  Ast *fmt_args = ast_arg_list(
      ast_identifier((ObjString){.chars = "fmt_string", .length = 10}), NULL);

  Ast *fmt_lambda = Ast_new(AST_LAMBDA);

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
Ast *parse_fstring(ObjString fstring) {
  // Remove backticks
  char *content = strndup(fstring.chars + 1, fstring.length - 2);
  int content_length = fstring.length - 2;

  Ast *result = ast_empty_list();
  char *current = content;
  char *end = content + content_length;

  while (current < end) {
    char *next_brace = strchr(current, '{');

    if (next_brace == NULL) {
      // No more expressions, add the rest as a string
      ObjString str = {current, end - current,
                       hash_string(current, end - current)};
      result = ast_list_push(result, ast_string(str));
      break;
    }

    if (next_brace > current) {
      // Add the text before the brace as a string
      ObjString str = {current, next_brace - current,
                       hash_string(current, next_brace - current)};
      result = ast_list_push(result, ast_string(str));
    }

    // Find the closing brace
    char *closing_brace = strchr(next_brace + 1, '}');
    if (closing_brace == NULL) {
      yyerror("Unclosed brace in interpolated string");
      free(content);
      return NULL;
    }

    // Parse the expression inside the braces
    *closing_brace = '\0'; // Temporarily null-terminate the expression
    // YY_BUFFER_STATE buffer = yy_scan_string(next_brace + 1);
    // Ast *expr = parse_expression(); // You'll need to implement this function
    // yy_delete_buffer(buffer);
    *closing_brace = '}'; // Restore the closing brace

    // result = ast_list_push(result, expr);

    current = closing_brace + 1;
  }

  free(content);
  return result;
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
    // Ast **members = list->data.AST_TUPLE.members;
    // list->data.AST_TUPLE;
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

Ast *ast_assoc_extern(Ast *l, ObjString name) {
  Ast *assoc = ast_binop(TOKEN_COLON, l, ast_identifier(name));
  return assoc;
}
Ast *ast_list_prepend(Ast *item, Ast *rest) {
  return ast_binop(TOKEN_DOUBLE_COLON, item, rest);
}

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

int get_let_binding_name(Ast *ast, ObjString *name) {
  Ast *binding = ast->data.AST_LET.binding;
  if (binding->tag != AST_IDENTIFIER) {
    return 1;
  }

  const char *chars = binding->data.AST_IDENTIFIER.value;
  int length = binding->data.AST_IDENTIFIER.length;
  *name = (ObjString){
      .chars = chars, .length = length, .hash = hash_string(chars, length)};
  return 0;
}

Ast *ast_placeholder() {
  char *c = malloc(sizeof(char) * 2);
  *c = '_';
  return ast_identifier((ObjString){.chars = c, .length = 1});
}

Ast *ast_bare_import(ObjString name) {
  Ast *import = Ast_new(AST_IMPORT);
  import->data.AST_IMPORT.module_name = name.chars;
  return import;
}

Ast *ast_record_access(Ast *record, Ast *member) {
  Ast *rec_access = Ast_new(AST_RECORD_ACCESS);
  rec_access->data.AST_RECORD_ACCESS.record = record;
  rec_access->data.AST_RECORD_ACCESS.member = member;
  return rec_access;
}

Ast *ast_char(char ch) {
  Ast *a = Ast_new(AST_CHAR);
  a->data.AST_CHAR.value = ch;
  return a;
}

Ast *ast_sequence(Ast *seq, Ast *new) {
  if (seq->tag == AST_BODY) {
    ast_body_push(seq, new);
    return seq;
  }

  Ast *body = Ast_new(AST_BODY);
  body = Ast_new(AST_BODY);
  body->data.AST_BODY.len = 2;
  body->data.AST_BODY.stmts = malloc(sizeof(Ast *) * 2);
  body->data.AST_BODY.stmts[0] = seq;
  body->data.AST_BODY.stmts[1] = new;
  return body;
}

Ast *macro(Ast *expr) {
  print_ast(expr);
  return NULL;
}

void handle_macro(Ast *root, const char *macro_text) {
  printf("handle macro '%s'\n", macro_text);
}
