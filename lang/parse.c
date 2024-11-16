#include "parse.h"
#include "input.h"
#include "serde.h"
#include "y.tab.h"
#include <stdlib.h>
#include <string.h>

bool top_level_tests = false;

char *_cur_script;
const char *_cur_script_content;
// static struct PStorage {
//   void *data;
//   size_t size;
//   size_t capacity;
// } PStorage;
// #define _PSTORAGE_SIZE 200000
// static void *_tstorage_data[_PSTORAGE_SIZE];
//
// static struct PStorage _pstorage = {_tstorage_data, 0, _PSTORAGE_SIZE};

// parser-specific allocation
static void *palloc(size_t size) {
  // if (_pstorage.size + size > _pstorage.capacity) {
  //   fprintf(stderr, "Error allocating memory for type");
  //   return NULL;
  // }
  // void *mem = _pstorage.data + _pstorage.size;
  // _pstorage.size += size;
  // return mem;
  return malloc(size);
}
static void *prealloc(void *p, size_t size) {
  // void *n = palloc(size);
  // memcpy(n, p, size);
  // return n;
  return realloc(p, size);
}

typedef struct __custom_binops_t {
  const char *binop;
  struct __custom_binops_t *next;
} __custom_binops_t;

__custom_binops_t *_custom_binops = NULL;

void add_custom_binop(const char *binop_name) {
  __custom_binops_t *new_custom_binops = malloc(sizeof(__custom_binops_t));
  new_custom_binops->binop = binop_name;
  new_custom_binops->next = _custom_binops;
  _custom_binops = new_custom_binops;
}

struct string_list {
  const char *data;
  struct string_list *next;
};

typedef struct inputs_list {
  const char *data;
  const char *qualified_path;
  struct inputs_list *next;
} inputs_list;

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

struct inputs_list *inputs_list_push_left(struct inputs_list *list,
                                          const char *qualified_path,
                                          const char *data) {
  struct inputs_list *new = palloc(sizeof(struct inputs_list));
  new->next = list;
  new->data = data;
  new->qualified_path = qualified_path;
  return new;
}

const char *inputs_lookup_by_path(struct inputs_list *list, const char *path) {

  for (struct inputs_list *inc = list; inc != NULL; inc = inc->next) {
    if (strcmp(inc->qualified_path, path) == 0) {
      // module already included
      return inc->qualified_path;
    }
  }
  return NULL;
}

static struct string_list *_included = NULL;
static struct inputs_List *included = NULL;

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
  // Ast *node = Ast_new(AST_BINOP);
  // node->data.AST_BINOP.op = op;
  // node->data.AST_BINOP.left = left;
  // node->data.AST_BINOP.right = right;
  // return node;
  Ast *node = Ast_new(AST_APPLICATION);

  Ast *function;
  switch (op) {
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

  if (expr->tag == AST_LAMBDA) {
    if (!top_level_tests &&
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

int yy_scan_string(char *);

static char *current_dir;
static int current_buf;
static struct inputs_list *input_stack = NULL;

char *prepend_current_directory(const char *filename) {
  if (filename == NULL) {
    return NULL;
  }

  // Check if the filename already starts with "./"
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
inputs_list *preprocess_includes(char *current_dir, const char *_input,
                                 inputs_list *stack) {
  int total_len = strlen(_input);
  const char *input;
  // process includes backwards so they're on the stack in the correct order
  while (total_len >= 0) {
    input = _input + total_len;
    if ((strncmp("%include ", input, 9) == 0) &&
        (*(input - 2) != '#')    // ignore if preceded by comment
        && (*(input - 1) != '#') // ignore if preceded by comment
    ) {
      int line_len = 0;
      const char *line = input;
      while (*line != '\n') {
        line_len++;
        line++;
      }
      char *include_line = strndup(input, line_len);
      char *mod_name = include_line + 9;
      int mod_name_len = strlen(current_dir) + 1 + strlen(mod_name) + 4;
      char *fully_qualified_name = palloc(sizeof(char) * mod_name_len);

      snprintf(fully_qualified_name, mod_name_len + 1, "%s/%s.ylc", current_dir,
               mod_name);
      fully_qualified_name = normalize_path(fully_qualified_name);
      fully_qualified_name = prepend_current_directory(fully_qualified_name);

      const char *previously_imported =
          inputs_lookup_by_path(stack, fully_qualified_name);

      if (previously_imported == NULL) {
        const char *import_content =
            read_script(fully_qualified_name,
                        false); // never include tests in imported code
        if (!import_content) {
          return NULL;
        }
        stack =
            inputs_list_push_left(stack, fully_qualified_name, import_content);

        char *_current_dir = get_dirname(stack->qualified_path);

        stack = preprocess_includes(_current_dir, import_content, stack);
      }
    }
    total_len--;
  }
  return stack;
}

void dbg_input_stack(struct inputs_list *stack) {
  int i = 0;
  while (stack) {
    printf("%d %s: \n%s", i, stack->qualified_path, stack->data);
    stack = stack->next;
    i++;
  }
}

static struct inputs_list *_stack = NULL;
//
// void parse_stack(inputs_list *current, inputs_list **last_processed) {
//
//   inputs_list *__stack = current;
//   while (__stack != _stack) {
//     const char *input = __stack->data;
//     yy_scan_string(input);
//     yyparse();
//
//     __stack = __stack->next;
//   }
//
//   *last_processed = current;
// }

Ast *parse_repl_include(const char *fcontent) {
  char *filename = strdup("./");
  filename = prepend_current_directory(filename);
  char *dir = get_dirname(filename);

  inputs_list *stack = _stack;
  stack = inputs_list_push_left(stack, filename, fcontent);
  char *current_dir = get_dirname(stack->qualified_path);
  stack = preprocess_includes(current_dir, fcontent, stack);

  Ast *prev = NULL;

  if (ast_root != NULL && ast_root->data.AST_BODY.len > 0) {
    prev = ast_root;
  }

  ast_root = Ast_new(AST_BODY);
  ast_root->data.AST_BODY.len = 0;
  ast_root->data.AST_BODY.stmts = palloc(sizeof(Ast *));

  inputs_list *__stack = stack;
  while (__stack != _stack) {
    const char *input = __stack->data;
    _cur_script = __stack->qualified_path;
    _cur_script_content = input;
    yylineno = 1;
    yyabsoluteoffset = 0;

    yy_scan_string(input);
    yyparse();

    __stack = __stack->next;
  }

  _stack = stack;

  Ast *res = ast_root;
  if (prev != NULL) {
    ast_root = prev;
  }

  // ugly hack - includes that only contain extern declarations
  // and don't emit any instructions crash the jit
  Ast *dummy = Ast_new(AST_INT);
  dummy->data.AST_INT.value = 1;
  ast_body_push(res, dummy);
  return res;
}

Ast *parse_input_script(const char *filename) {
  filename = prepend_current_directory(filename);
  char *dir = get_dirname(filename);
  char *fcontent = read_script(filename, top_level_tests);
  if (!fcontent) {
    return NULL;
  }
  inputs_list *stack = _stack;
  stack = inputs_list_push_left(stack, filename, fcontent);
  char *current_dir = get_dirname(stack->qualified_path);
  stack = preprocess_includes(current_dir, fcontent, stack);

  ast_root = Ast_new(AST_BODY);
  ast_root->data.AST_BODY.len = 0;
  ast_root->data.AST_BODY.stmts = palloc(sizeof(Ast *));

  // dbg_input_stack(stack);
  inputs_list *__stack = stack;
  while (__stack != _stack) {
    _cur_script = __stack->qualified_path;
    const char *input = __stack->data;
    _cur_script_content = input;
    yylineno = 1;
    yyabsoluteoffset = 0;
    yy_scan_string(input);
    yyparse();
    __stack = __stack->next;
  }
  _stack = stack;

  return ast_root;
}

/* Define the parsing function */
Ast *parse_input(char *input, const char *dirname) {
  current_dir = dirname;

  Ast *prev = NULL;

  if (ast_root != NULL && ast_root->data.AST_BODY.len > 0) {
    prev = ast_root;
  }

  ast_root = Ast_new(AST_BODY);
  ast_root->data.AST_BODY.len = 0;
  ast_root->data.AST_BODY.stmts = palloc(sizeof(Ast *));

  yy_scan_string(input); // Set the input for the lexer
  yyparse();             // Parse the input

  Ast *res = ast_root;
  if (prev != NULL) {
    ast_root = prev;
  }
  return res;
}

bool is_custom_binop(const char *id) {
  __custom_binops_t *bb = _custom_binops;
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
    lambda->data.AST_LAMBDA.defaults = NULL;
  }
  lambda->data.AST_LAMBDA.body = body;
  return lambda;
}

Ast *ast_void_lambda(Ast *body) {
  Ast *lambda = Ast_new(AST_LAMBDA);
  lambda->data.AST_LAMBDA.params = ast_void();
  lambda->data.AST_LAMBDA.len = 1;
  lambda->data.AST_LAMBDA.defaults = NULL;
  lambda->data.AST_LAMBDA.body = body;
  return lambda;
}

Ast *ast_arg_list(Ast *arg_id, Ast *def) {
  Ast *lambda = Ast_new(AST_LAMBDA);

  lambda->data.AST_LAMBDA.params = palloc(sizeof(Ast));
  lambda->data.AST_LAMBDA.len = 1;
  lambda->data.AST_LAMBDA.params[0] = *arg_id;
  lambda->data.AST_LAMBDA.defaults = palloc(sizeof(Ast *));

  if (def) {
    lambda->data.AST_LAMBDA.defaults[0] = def;
  }

  return lambda;
}

Ast *ast_arg_list_push(Ast *lambda, Ast *arg_id, Ast *def) {

  Ast *params = lambda->data.AST_LAMBDA.params;

  lambda->data.AST_LAMBDA.len++;
  size_t len = lambda->data.AST_LAMBDA.len;

  lambda->data.AST_LAMBDA.params = prealloc(params, sizeof(Ast) * len);
  lambda->data.AST_LAMBDA.defaults =
      prealloc(lambda->data.AST_LAMBDA.defaults, sizeof(Ast *) * len);
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
  extern_fn->data.AST_EXTERN_FN.signature_types = signature;
  extern_fn->data.AST_EXTERN_FN.fn_name = name;
  return extern_fn;
}
Ast *ast_assoc(Ast *l, Ast *r) { return ast_let(l, r, NULL); }

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
  char *c = palloc(sizeof(char) * 2);
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
  body->data.AST_BODY.stmts = palloc(sizeof(Ast *) * 2);
  body->data.AST_BODY.stmts[0] = seq;
  body->data.AST_BODY.stmts[1] = new;
  return body;
}

Ast *ast_await(Ast *awaitable) {
  // printf("parse await\n");
  return awaitable;
}

Ast *ast_fn_sig(Ast *arg, Ast *arg2) {
  Ast *a = ast_list(arg);
  a->tag = AST_FN_SIGNATURE;
  return a;
}

Ast *ast_fn_sig_push(Ast *l, Ast *a) {
  Ast *sig = ast_list_push(l, a);
  a->tag = AST_FN_SIGNATURE;
  return sig;
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

  // Print the caret
  if (loc->col_end - loc->col > 2) {
    fprintf(stderr, "%*c", loc->col - 1, ' ');
    fprintf(stderr, "^");
  }
  fprintf(stderr, "\n");
}
Ast *ast_yield(Ast *expr) {
  Ast *y = Ast_new(AST_YIELD);
  y->data.AST_YIELD.expr = expr;
  return y;
}

Ast *ast_thunk_expr(Ast *expr) {
  Ast *thunk = ast_lambda(ast_arg_list(ast_void(), NULL), expr);
  return thunk;
}
