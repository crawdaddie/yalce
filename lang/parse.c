#include "parse.h"
#include "config.h"
#include "input.h"
#include "serde.h"
#include "y.tab.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *__base_dir = NULL;
void set_base_dir(const char *dir) { __base_dir = dir; }

const char *__filename;
char *_cur_script;
const char *_cur_script_content;

static void *__palloc(size_t size) { return malloc(size); }
static void *__prealloc(void *p, size_t size) { return realloc(p, size); }

AllocatorFnType palloc = __palloc;
ReAllocatorFnType prealloc = __prealloc;
custom_binops_t *__custom_binops = NULL;

void add_custom_binop(const char *binop_name) {
  custom_binops_t *new_custom_binops = malloc(sizeof(custom_binops_t));
  new_custom_binops->binop = binop_name;
  new_custom_binops->next = __custom_binops;
  __custom_binops = new_custom_binops;
}

struct string_list {
  const char *data;
  struct string_list *next;
};

struct string_list *string_list_push_left(struct string_list *list,
                                          const char *data) {
  struct string_list *new = palloc(sizeof(struct string_list));
  new->next = list;
  new->data = data;
  return new;
}

const char *string_list_find(struct string_list *list, const char *data) {

  for (struct string_list *inc = list; inc != NULL; inc = inc->next) {
    if (strcmp(inc->data, data) == 0) {
      // module already included
      return inc->data;
    }
  }
  return NULL;
}

Ast *Ast_new(enum ast_tag tag) {
  Ast *node = palloc(sizeof(Ast));
  loc_info *loc = palloc(sizeof(loc_info));
  loc->line = yylloc.last_line;
  loc->col = yylloc.first_column;
  loc->col_end = yylloc.last_column;
  loc->src = _cur_script;
  loc->src_content = _cur_script_content;
  loc->absolute_offset = yyprevoffset;
  node->tag = tag;
  node->loc_info = loc;
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

    body->data.AST_BODY.stmts = prealloc(members, sizeof(Ast *) * len);
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
  if (op == TOKEN_RANGE_TO) {
    Ast *range = Ast_new(AST_RANGE_EXPRESSION);
    range->data.AST_RANGE_EXPRESSION.from = left;
    range->data.AST_RANGE_EXPRESSION.to = right;
    return range;
  }
  Ast *node = Ast_new(AST_APPLICATION);

  Ast *function;
  switch (op) {
  case TOKEN_ASSIGNMENT: {

    if (left->tag == AST_APPLICATION &&
        strcmp(left->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value,
               "array_at") == 0) {

      Ast *app = ast_application(ast_identifier((ObjString){"array_set", 9}),
                                 left->data.AST_APPLICATION.args);

      app = ast_application(app, left->data.AST_APPLICATION.args + 1);
      app = ast_application(app, right);
      return app;
    }

    Ast *b = Ast_new(AST_BINOP);
    b->data.AST_BINOP.left = left;
    b->data.AST_BINOP.right = right;
    b->data.AST_BINOP.op = op;
    return b;
  }
  case TOKEN_PLUS: {
    function = ast_identifier((ObjString){"+", 1});
    break;
  }

  case TOKEN_MINUS: {
    function = ast_identifier((ObjString){"-", 1});
    break;
  }

  case TOKEN_STAR: {
    function = ast_identifier((ObjString){"*", 1});
    break;
  }

  case TOKEN_SLASH: {
    function = ast_identifier((ObjString){"/", 1});
    break;
  }

  case TOKEN_MODULO: {
    function = ast_identifier((ObjString){"%", 1});
    break;
  }

  case TOKEN_LT: {
    function = ast_identifier((ObjString){"<", 1});
    break;
  }

  case TOKEN_GT: {
    function = ast_identifier((ObjString){">", 1});
    break;
  }

  case TOKEN_LTE: {
    function = ast_identifier((ObjString){"<=", 2});
    break;
  }

  case TOKEN_GTE: {
    function = ast_identifier((ObjString){">=", 2});
    break;
  }

  case TOKEN_EQUALITY: {
    function = ast_identifier((ObjString){"==", 2});
    break;
  }

  case TOKEN_NOT_EQUAL: {
    function = ast_identifier((ObjString){"!=", 2});
    break;
  }

  case TOKEN_DOUBLE_COLON: {
    function = ast_identifier((ObjString){"::", 2});
    break;
  }

  case TOKEN_DOUBLE_AMP: {
    function = ast_identifier((ObjString){"&&", 2});
    break;
  }

  case TOKEN_DOUBLE_PIPE: {
    function = ast_identifier((ObjString){"||", 2});
    break;
  }
  }

  node->data.AST_APPLICATION.function = function;
  node->data.AST_APPLICATION.args = malloc(sizeof(Ast) * 2);
  node->data.AST_APPLICATION.args[0] = *left;
  node->data.AST_APPLICATION.args[1] = *right;
  node->data.AST_APPLICATION.len = 2;
  return node;
}

Ast *ast_unop(token_type op, Ast *right) {
  switch (op) {
  case TOKEN_STAR: {
    Ast *node = Ast_new(AST_APPLICATION);
    node->data.AST_APPLICATION.function =
        ast_identifier((ObjString){"deref", 5});
    node->data.AST_APPLICATION.args = malloc(sizeof(Ast));
    node->data.AST_APPLICATION.args[0] = *right;
    node->data.AST_APPLICATION.len = 1;
    return node;
  }
  case TOKEN_AMPERSAND: {
    Ast *node = Ast_new(AST_APPLICATION);
    node->data.AST_APPLICATION.function =
        ast_identifier((ObjString){"addrof", 6});
    node->data.AST_APPLICATION.args = malloc(sizeof(Ast) * 1);
    node->data.AST_APPLICATION.args[0] = *right;
    node->data.AST_APPLICATION.len = 1;
    return node;
  }
  default: {
    Ast *node = Ast_new(AST_UNOP);
    node->data.AST_BINOP.op = op;
    node->data.AST_BINOP.right = right;
    return node;
  }
  }
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
  if (expr == NULL) {
    return node;
  }

  if (expr->tag == AST_LAMBDA) {
    if (!config.test_mode &&
        strncmp(name->data.AST_IDENTIFIER.value, "test_", 5) == 0) {
      return NULL;
    }

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
  node->data.AST_LET.expr = expr;
  node->data.AST_LET.in_expr = in_continuation;
  // print_ast(node);
  return node;
}

Ast *ast_test_module(Ast *expr) {
  if (!config.test_mode) {
    // don't parse this unless in test context
    return NULL;
  }
  Ast *node = Ast_new(AST_LET);
  Ast *id = ast_identifier((ObjString){"test", 4});
  node->data.AST_LET.binding = id;
  node->data.AST_LET.expr = expr;
  return node;
}

int yy_scan_string(char *);

static char *current_dir;
static int current_buf;

char *prepend_current_directory(const char *filename) {
  if (filename == NULL) {
    return NULL;
  }

  if (strncmp(filename, "./", 2) == 0) {
    // If it does, just return a copy of the original string
    return strdup(filename);
  } else {
    // If it doesn't, allocate memory for the new string
    char *new_filename =
        malloc(strlen(filename) + 3); // 2 for "./" and 1 for null terminator
    if (new_filename == NULL) {
      return NULL; // Memory allocation failed
    }

    // Construct the new string
    strcpy(new_filename, "./");
    strcat(new_filename, filename);

    return new_filename;
  }
}

Ast *parse_input_script(const char *filename) {
  __filename = filename;
  char *fcontent = read_script(filename);
  if (!fcontent) {
    return NULL;
  }

  char *dir = get_dirname(filename);

  char *current_dir = get_dirname(filename);

  ast_root = Ast_new(AST_BODY);
  ast_root->data.AST_BODY.len = 0;
  ast_root->data.AST_BODY.stmts = palloc(sizeof(Ast *));

  _cur_script = filename;
  const char *input = fcontent;

  _cur_script_content = input;
  yylineno = 1;
  yyabsoluteoffset = 0;
  yy_scan_string(input);
  yyparse();

  return ast_root;
}

Ast *parse_input(char *input, const char *dirname) {

  Ast *prev = NULL;

  if (ast_root != NULL && ast_root->data.AST_BODY.len > 0) {
    prev = ast_root;
  }

  ast_root = Ast_new(AST_BODY);
  ast_root->data.AST_BODY.len = 0;
  ast_root->data.AST_BODY.stmts = palloc(sizeof(Ast *));

  _cur_script = "tmp.ylc";
  _cur_script_content = input;
  yy_scan_string(input); // Set the input for the lexer
  yyparse();             // Parse the input

  Ast *res = ast_root;
  if (prev != NULL) {
    ast_root = prev;
  }
  return res;
}

bool is_custom_binop(const char *id) {
  custom_binops_t *bb = __custom_binops;
  while (bb) {
    if (strcmp(id, bb->binop) == 0) {
      return true;
    }
    bb = bb->next;
  }
  return false;
}

Ast *ast_application(Ast *func, Ast *arg) {

  if (func->tag == AST_APPLICATION) {

    if (arg->tag == AST_IDENTIFIER &&

        is_custom_binop(arg->data.AST_IDENTIFIER.value)) {

      Ast *app = Ast_new(AST_APPLICATION);
      app->data.AST_APPLICATION.function = arg;
      app->data.AST_APPLICATION.args = func;
      app->data.AST_APPLICATION.len = 1;
      return app;
    }

    Ast *args = func->data.AST_APPLICATION.args;
    func->data.AST_APPLICATION.len++;
    int len = func->data.AST_APPLICATION.len;

    func->data.AST_APPLICATION.args = prealloc(args, sizeof(Ast) * len);
    func->data.AST_APPLICATION.args[len - 1] = *arg;

    return func;
  }

  if (arg->tag == AST_IDENTIFIER &&
      is_custom_binop(arg->data.AST_IDENTIFIER.value)) {

    Ast *app = Ast_new(AST_APPLICATION);
    app->data.AST_APPLICATION.function = arg;
    app->data.AST_APPLICATION.args = palloc(sizeof(Ast));
    app->data.AST_APPLICATION.args[0] = *func;
    app->data.AST_APPLICATION.len = 1;

    return app;
  }

  Ast *app = Ast_new(AST_APPLICATION);
  app->data.AST_APPLICATION.function = func;
  app->data.AST_APPLICATION.args = palloc(sizeof(Ast));
  app->data.AST_APPLICATION.args[0] = *arg;
  app->data.AST_APPLICATION.len = 1;

  return app;
}

Ast *ast_lambda(Ast *lambda, Ast *body) {
  if (lambda == NULL) {
    lambda = Ast_new(AST_LAMBDA);
    lambda->data.AST_LAMBDA.params = NULL;
    lambda->data.AST_LAMBDA.len = 0;
    lambda->data.AST_LAMBDA.type_annotations = NULL;
  }
  if (body->tag != AST_BODY) {
    body->is_body_tail = true;
  }
  lambda->data.AST_LAMBDA.body = body;
  return lambda;
}

AstList *ast_list_extend_left(AstList *list, Ast *n) {
  AstList *new_list = palloc(sizeof(AstList));
  *new_list = (AstList){n, list};
  return new_list;
}

AstList *ast_list_extend_right(AstList *list, Ast *n) {
  if (list == NULL) {
    AstList *list = palloc(sizeof(AstList));
    *list = (AstList){n, NULL};
    return list;
  }

  AstList *l = list;

  while (l->next != NULL) {
    l = l->next;
  }

  AstList *tail = palloc(sizeof(AstList));
  *tail = (AstList){n, NULL};
  l->next = tail;

  return list;
}

Ast *ast_void_lambda(Ast *body) {
  Ast *lambda = Ast_new(AST_LAMBDA);
  lambda->data.AST_LAMBDA.params = ast_list_extend_left(NULL, ast_void());
  lambda->data.AST_LAMBDA.len = 1;
  lambda->data.AST_LAMBDA.type_annotations = NULL;
  lambda->data.AST_LAMBDA.body = body;
  return lambda;
}

Ast *ast_arg_list(Ast *arg_id, Ast *def) {
  Ast *lambda = Ast_new(AST_LAMBDA);

  lambda->data.AST_LAMBDA.len = 1;
  lambda->data.AST_LAMBDA.params = ast_list_extend_right(NULL, arg_id);
  lambda->data.AST_LAMBDA.type_annotations = ast_list_extend_right(NULL, def);

  return lambda;
}

Ast *ast_arg_list_push(Ast *lambda, Ast *arg_id, Ast *def) {

  lambda->data.AST_LAMBDA.len++;
  lambda->data.AST_LAMBDA.params =
      ast_list_extend_right(lambda->data.AST_LAMBDA.params, arg_id);

  lambda->data.AST_LAMBDA.type_annotations = ast_list_extend_right(
      lambda->data.AST_LAMBDA.type_annotations, def ? def : NULL);
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
  body->data.AST_BODY.stmts = palloc(sizeof(Ast *));
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

        seg_start = i;
      } else {
        continue;
      }
    } else if (*curs == '}' && i != 0 && *(fstring.chars + i - 1) != '\\') {
      segment = (ObjString){.chars = fstring.chars + i + 1};
      seg_start = i;
    } else if (i == fstring.length - 1) {
      int len = i - seg_start;
      segment.length = len;
      Ast *str_segment = ast_string(segment);
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

Ast *ast_typed_empty_list(ObjString id) {
  Ast *l = Ast_new(AST_EMPTY_LIST);
  l->data.AST_EMPTY_LIST.type_id = id;
  return l;
}

Ast *ast_empty_array() {
  Ast *l = Ast_new(AST_ARRAY);
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

  list->data.AST_LIST.items = prealloc(items, sizeof(Ast) * len);
  list->data.AST_LIST.items[len - 1] = *val;
  return list;
}
Ast *ast_list_to_array(Ast *list) {
  list->tag = AST_ARRAY;
  return list;
}

Ast *ast_match(Ast *expr, Ast *match) {
  match->data.AST_MATCH.expr = expr;
  return match;
}
Ast *ast_match_branches(Ast *match, Ast *expr, Ast *result) {
  if (match == NULL) {
    match = Ast_new(AST_MATCH);
    match->data.AST_MATCH.branches = palloc(sizeof(Ast) * 2);
    match->data.AST_MATCH.branches[0] = *expr;
    match->data.AST_MATCH.branches[1] = *result;
    match->data.AST_MATCH.len = 1;
    return match;
  }

  Ast *branches = match->data.AST_MATCH.branches;
  // ite->data.AST_LIST.len++;
  match->data.AST_MATCH.len++;
  int len = match->data.AST_MATCH.len;

  match->data.AST_MATCH.branches = prealloc(branches, sizeof(Ast) * len * 2);
  match->data.AST_MATCH.branches[len * 2 - 2] = *expr;
  match->data.AST_MATCH.branches[len * 2 - 1] = *result;
  return match;
}

Ast *ast_if_else(Ast *cond, Ast *then, Ast *elze) {

  if (elze == NULL) {
    Ast *match = Ast_new(AST_MATCH);
    match->data.AST_MATCH.branches = palloc(sizeof(Ast) * 2);
    match->data.AST_MATCH.branches[0] =
        (Ast){AST_BOOL, .data = {.AST_BOOL = {.value = true}}};
    match->data.AST_MATCH.branches[1] = *then;
    match->data.AST_MATCH.len = 1;
    match->data.AST_MATCH.expr = cond;
    return match;
  }

  Ast *match = Ast_new(AST_MATCH);
  match->data.AST_MATCH.branches = palloc(sizeof(Ast) * 4);
  match->data.AST_MATCH.branches[0] =
      (Ast){AST_BOOL, .data = {.AST_BOOL = {.value = true}}};
  match->data.AST_MATCH.branches[1] = *then;

  match->data.AST_MATCH.branches[2] =
      (Ast){AST_BOOL, .data = {.AST_BOOL = {.value = false}}};
  match->data.AST_MATCH.branches[3] = *elze;
  match->data.AST_MATCH.len = 2;
  match->data.AST_MATCH.expr = cond;
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

  Ast *extern_fn = Ast_new(AST_EXTERN_FN);
  extern_fn->data.AST_EXTERN_FN.signature_types =
      ast_fn_signature_of_list(signature);
  extern_fn->data.AST_EXTERN_FN.fn_name = name;

  // Ast *l = signature;
  // int len = 0;
  // while (l->tag == AST_LIST || l->tag == AST_FN_SIGNATURE) {
  //   l = l->data.AST_LIST.items + 1;
  //   len++;
  // }
  // len++;
  //
  // Ast *items = palloc(sizeof(Ast) * len);
  // int i = 0;
  // l = signature;
  // while (l->tag == AST_LIST || l->tag == AST_FN_SIGNATURE) {
  //   *(items + i) = *l->data.AST_LIST.items;
  //   l = l->data.AST_LIST.items + 1;
  //   i++;
  // }
  // *(items + i) = *l;
  // Ast *sig_list = Ast_new(AST_LIST);
  // sig_list->data.AST_LIST.items = items;
  // sig_list->data.AST_LIST.len = len;
  // sig_list->tag = AST_FN_SIGNATURE;
  // extern_fn->data.AST_EXTERN_FN.signature_types = sig_list;

  return extern_fn;
}

Ast *ast_assoc(Ast *l, Ast *r) { return ast_let(l, r, NULL); }

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
  char *c = palloc(sizeof(char) * 2);
  *c = '_';
  return ast_identifier((ObjString){.chars = c, .length = 1});
}

Ast *ast_record_access(Ast *record, Ast *member) {
  Ast *rec_access = Ast_new(AST_RECORD_ACCESS);
  rec_access->data.AST_RECORD_ACCESS.record = record;
  rec_access->data.AST_RECORD_ACCESS.member = member;
  rec_access->data.AST_RECORD_ACCESS.index = -1;
  return rec_access;
}

Ast *ast_char(char ch) {
  // printf("create ast char %c\n", ch);
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
  body->data.AST_BODY.stmts = palloc(sizeof(Ast *) * 2);
  body->data.AST_BODY.stmts[0] = seq;
  body->data.AST_BODY.stmts[1] = new;
  return body;
}

Ast *ast_await(Ast *awaitable) {
  // printf("parse await\n");
  return awaitable;
}

Ast *ast_fn_sig(Ast *a, Ast *b) {
  a = ast_list(a);
  ast_list_push(a, b);
  // a->tag = AST_FN_SIGNATURE;
  return a;
}
Ast *ast_fn_sig_push(Ast *tuple, Ast *mem) {

  Ast *tup = ast_list_push(tuple, mem);
  return tup;
}

Ast *ast_tuple_type(Ast *a, Ast *b) {
  a = ast_list(a);
  ast_list_push(a, b);
  a->tag = AST_TUPLE;
  return a;
}
Ast *ast_tuple_type_push(Ast *tuple, Ast *mem) {
  Ast *tup = ast_list_push(tuple, mem);
  tup->tag = AST_TUPLE;
  return tup;
}

Ast *ast_tuple_type_single(Ast *a) {
  a = ast_list(a);
  a->tag = AST_TUPLE;
  return a;
}

Ast *ast_cons_decl(token_type op, Ast *left, Ast *right) {

  Ast *node = Ast_new(AST_BINOP);
  node->data.AST_BINOP.op = op;
  node->data.AST_BINOP.left = left;
  node->data.AST_BINOP.right = right;
  return node;
}

Ast *ast_match_guard_clause(Ast *expr, Ast *guard) {
  Ast *node = Ast_new(AST_MATCH_GUARD_CLAUSE);

  node->data.AST_MATCH_GUARD_CLAUSE.test_expr = expr;
  node->data.AST_MATCH_GUARD_CLAUSE.guard_expr = guard;
  return node;
}

void print_location(Ast *ast) {
  loc_info *loc = ast->loc_info;
  if (!loc || !loc->src || !loc->src_content) {
    print_ast_err(ast);
    return;
  }
  fprintf(stderr, "%s %d:%d\n", loc->src, loc->line, loc->col);

  const char *start = loc->src_content;
  const char *offset = start + loc->absolute_offset;

  // Guard against going before the start of the string
  while (offset > start && *offset != '\n') {
    offset--;
  }

  if (offset > start) {
    offset++; // Move past the newline if we found one
  }

  // Print the line
  while (*offset && *offset != '\n') {
    fputc(*offset, stderr);
    offset++;
  }
  fprintf(stderr, "\n");

  fprintf(stderr, "%*c", loc->col - 1, ' ');
  fprintf(stderr, "^");
  fprintf(stderr, "\n");
}

Ast *ast_yield(Ast *expr) {
  Ast *y = Ast_new(AST_YIELD);
  y->data.AST_YIELD.expr = expr;
  return y;
}

Ast *ast_yield_end() {
  Ast *y = Ast_new(AST_YIELD);
  y->data.AST_YIELD.expr = NULL;
  return y;
}
Ast *ast_thunk_expr(Ast *expr) {
  Ast *thunk = ast_lambda(ast_arg_list(ast_void(), NULL), expr);
  return thunk;
}

Ast *ast_spread_operator(Ast *expr) {
  Ast *spread = Ast_new(AST_SPREAD_OP);
  spread->data.AST_SPREAD_OP.expr = expr;
  return spread;
}
Ast *ast_fn_signature_of_list(Ast *l) {
  l->tag = AST_FN_SIGNATURE;
  return l;
}

Ast *ast_implements(ObjString type, ObjString trait) {
  Ast *impl_expr = Ast_new(AST_IMPLEMENTS);
  impl_expr->data.AST_IMPLEMENTS.type_id = type;
  impl_expr->data.AST_IMPLEMENTS.trait_id = trait;
  return impl_expr;
}

AstVisitor *ast_visit(Ast *ast, AstVisitor *visitor) {
  switch (ast->tag) {
  case AST_INT: {
    visitor->visit_int ? visitor->visit_int(ast, visitor)
                       : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_DOUBLE: {
    visitor->visit_double ? visitor->visit_double(ast, visitor)
                          : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_STRING: {
    visitor->visit_string ? visitor->visit_string(ast, visitor)
                          : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_CHAR: {
    visitor->visit_char ? visitor->visit_char(ast, visitor)
                        : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_BOOL: {
    visitor->visit_bool ? visitor->visit_bool(ast, visitor)
                        : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_VOID: {
    visitor->visit_void ? visitor->visit_void(ast, visitor)
                        : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_ARRAY: {
    visitor->visit_array ? visitor->visit_array(ast, visitor)
                         : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_LIST: {
    visitor->visit_list ? visitor->visit_list(ast, visitor)
                        : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_TUPLE: {
    visitor->visit_tuple ? visitor->visit_tuple(ast, visitor)
                         : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_TYPE_DECL: {
    visitor->visit_type_decl ? visitor->visit_type_decl(ast, visitor)
                             : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_FMT_STRING: {
    visitor->visit_fmt_string ? visitor->visit_fmt_string(ast, visitor)
                              : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_BODY: {
    visitor->visit_body ? visitor->visit_body(ast, visitor)
                        : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_IDENTIFIER: {
    visitor->visit_identifier ? visitor->visit_identifier(ast, visitor)
                              : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_APPLICATION: {
    visitor->visit_application ? visitor->visit_application(ast, visitor)
                               : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_LET: {
    visitor->visit_let ? visitor->visit_let(ast, visitor)
                       : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_LAMBDA: {
    visitor->visit_lambda ? visitor->visit_lambda(ast, visitor)
                          : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_MATCH: {
    visitor->visit_match ? visitor->visit_match(ast, visitor)
                         : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_EXTERN_FN: {
    visitor->visit_extern_fn ? visitor->visit_extern_fn(ast, visitor)
                             : visitor->visit_default(ast, visitor);
    break;
  }
  case AST_YIELD: {
    visitor->visit_yield ? visitor->visit_yield(ast, visitor)
                         : visitor->visit_default(ast, visitor);
    break;
  }
  default: {
    visitor->visit_default(ast, visitor);
  }
  }

  return visitor;
}

Ast *ast_module(Ast *lambda) {
  lambda->tag = AST_MODULE;
  return lambda;
}

char *__import_current_dir;

char *check_path(char *fully_qualified_name, const char *rel_path) {
  if (access(fully_qualified_name, F_OK) != 0 &&
      (config.base_libs_dir != NULL)) {
    char *new_filename =
        malloc(strlen(rel_path) + strlen(config.base_libs_dir) + 1);
    sprintf(new_filename, "%s/%s", config.base_libs_dir, rel_path);
    fully_qualified_name = new_filename;

    if (access(fully_qualified_name, F_OK) != 0) {
      return NULL;
    }
  }

  return fully_qualified_name;
}

Ast *ast_import_stmt(ObjString path_identifier, bool import_all) {

  char *mod_name = path_identifier.chars;
  const char *mod_id_chars = get_mod_name_from_path_identifier(mod_name);

  int mod_name_len = strlen(__import_current_dir) + 1 + strlen(mod_name) + 4;
  char *fully_qualified_name = palloc(sizeof(char) * mod_name_len);
  char *rel_path = palloc(sizeof(char) * (strlen(mod_name) + 4));
  sprintf(rel_path, "%s.ylc", mod_name);

  snprintf(fully_qualified_name, mod_name_len + 1, "%s/%s.ylc",
           __import_current_dir, mod_name);

  fully_qualified_name = normalize_path(fully_qualified_name);
  fully_qualified_name = check_path(fully_qualified_name, rel_path);

  if (!fully_qualified_name) {
    fprintf(stderr, "Error module %s not found in path\n",
            fully_qualified_name);
    return NULL;
  }

  Ast *import_ast = Ast_new(AST_IMPORT);
  import_ast->data.AST_IMPORT.identifier = mod_id_chars;
  import_ast->data.AST_IMPORT.fully_qualified_name = fully_qualified_name;
  import_ast->data.AST_IMPORT.import_all = import_all;
  return import_ast;
}

Ast *ast_range_expression(Ast *from, Ast *to) {
  Ast *range = Ast_new(AST_RANGE_EXPRESSION);
  range->data.AST_RANGE_EXPRESSION.from = from;
  range->data.AST_RANGE_EXPRESSION.to = to;
  return range;
}

Ast *ast_for_loop(Ast *binding, Ast *iter_expr, Ast *body) {
  Ast *loop = Ast_new(AST_LOOP);
  loop->data.AST_LET.binding = binding;
  loop->data.AST_LET.expr = iter_expr;
  loop->data.AST_LET.in_expr = body;
  return loop;
}

Ast *ast_assignment(Ast *var, Ast *val) {
  return ast_binop(TOKEN_ASSIGNMENT, var, val);
}

Ast *ast_trait_impl(ObjString trait_name, ObjString type_name, Ast *module) {
  module->data.AST_LAMBDA.fn_name = trait_name;
  Ast *trait_impl = Ast_new(AST_TRAIT_IMPL);
  trait_impl->data.AST_TRAIT_IMPL.trait_name = trait_name;
  trait_impl->data.AST_TRAIT_IMPL.type = type_name;
  trait_impl->data.AST_TRAIT_IMPL.impl = module;
  return trait_impl;
}
