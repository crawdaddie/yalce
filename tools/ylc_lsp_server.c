#include "../lang/config.h"
#include "../lang/ht.h"
#include "../lang/modules.h"
#include "../lang/parse.h"
#include "../lang/types/builtins.h"
#include "../lang/types/inference.h"
#include "../lang/types/type_ser.h"
#include <ctype.h>
#include <json-c/json.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LSP_SYMBOL_KIND_MODULE 2
#define LSP_SYMBOL_KIND_CLASS 5
#define LSP_COMPLETION_KIND_TEXT 1
#define LSP_COMPLETION_KIND_FUNCTION 3
#define LSP_COMPLETION_KIND_VARIABLE 6
#define LSP_COMPLETION_KIND_MODULE 9
#define LSP_COMPLETION_KIND_KEYWORD 14
#define LSP_SYMBOL_KIND_FUNCTION 12
#define LSP_SYMBOL_KIND_VARIABLE 13
#define LSP_SYMBOL_KIND_NAMESPACE 3
#define LSP_SYMBOL_KIND_OPERATOR 25

#define LSP_SYNC_FULL 1
#define SERVER_CONTINUE 0
#define SERVER_EXIT_OK 1
#define SERVER_EXIT_ERROR 2

typedef struct document {
  char *uri;
  char *path;
  char *text;
  Ast *root;
  TypeEnv *type_env;
  bool typecheck_ok;
  bool analysis_dirty;
  parse_error_info parse_error;
  struct document *next;
} document;

typedef struct {
  document *docs;
  bool initialized;
  bool shutdown_requested;
} lsp_server;

typedef struct {
  Ast *node;
  long long start_offset;
  long long end_offset;
} node_match;

typedef struct definition_binding {
  const char *name;
  Ast *definition_node;
  Ast *value_node;
  struct definition_binding *next;
  struct definition_binding *alloc_next;
} definition_binding;

typedef struct {
  lsp_server *server;
  document *doc;
  Ast *target_node;
  char *target_name;
  long long cursor_offset;
  bool done;
  Ast *definition_node;
  definition_binding *allocated_bindings;
} definition_search;

typedef struct {
  document *doc;
  Ast *target_definition_node;
  const char *new_name;
  struct json_object *edits;
  definition_search bindings;
} rename_collect;

typedef struct {
  lsp_server *server;
  document *doc;
  Ast *target_definition_node;
  bool include_declaration;
  struct json_object *locations;
  definition_search bindings;
} reference_collect;

static const char *completion_keywords[] = {
    "fn",    "let",  "in",     "and",  "extern", "true", "false",
    "match", "with", "import", "open", "yield",  "loop",
};

static const char *completion_trigger_chars[] = {
    ".",
};

static char *xstrdup(const char *s) {
  size_t len = strlen(s);
  char *copy = malloc(len + 1);
  if (!copy) {
    return NULL;
  }
  memcpy(copy, s, len + 1);
  return copy;
}

static int hex_value(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  ch = (char)tolower((unsigned char)ch);
  if (ch >= 'a' && ch <= 'f') {
    return 10 + (ch - 'a');
  }
  return -1;
}

static char *decode_uri_component(const char *input) {
  size_t len = strlen(input);
  char *out = malloc(len + 1);
  size_t j = 0;

  if (!out) {
    return NULL;
  }

  for (size_t i = 0; i < len; i++) {
    if (input[i] == '%' && i + 2 < len) {
      int hi = hex_value(input[i + 1]);
      int lo = hex_value(input[i + 2]);
      if (hi >= 0 && lo >= 0) {
        out[j++] = (char)((hi << 4) | lo);
        i += 2;
        continue;
      }
    }

    if (input[i] == '+') {
      out[j++] = ' ';
    } else {
      out[j++] = input[i];
    }
  }

  out[j] = '\0';
  return out;
}

static char *uri_to_path(const char *uri) {
  const char *file_scheme = "file://";

  if (!uri) {
    return NULL;
  }

  if (strncmp(uri, file_scheme, strlen(file_scheme)) != 0) {
    return xstrdup(uri);
  }

  const char *path = uri + strlen(file_scheme);
  if (path[0] != '/' && path[0] != '\0') {
    const char *slash = strchr(path, '/');
    path = slash ? slash : path;
  }

  return decode_uri_component(path);
}

static document *find_doc(lsp_server *server, const char *uri) {
  for (document *doc = server->docs; doc != NULL; doc = doc->next) {
    if (strcmp(doc->uri, uri) == 0) {
      return doc;
    }
  }
  return NULL;
}

static document *upsert_doc(lsp_server *server, const char *uri) {
  document *doc = find_doc(server, uri);
  if (doc) {
    return doc;
  }

  doc = calloc(1, sizeof(*doc));
  if (!doc) {
    return NULL;
  }

  doc->uri = xstrdup(uri);
  doc->path = uri_to_path(uri);
  doc->next = server->docs;
  server->docs = doc;
  return doc;
}

static void configure_analysis_environment(void) {
  static char cwd[PATH_MAX];
  const char *base_dir = ylc_config.base_libs_dir;

  if (base_dir && base_dir[0] != '\0') {
    return;
  }

  base_dir = getenv("YLC_BASE_DIR");
  if (base_dir && base_dir[0] != '\0') {
    ylc_config.base_libs_dir = base_dir;
    return;
  }

  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    ylc_config.base_libs_dir = cwd;
  }
}

static void remove_doc(lsp_server *server, const char *uri) {
  document **link = &server->docs;
  while (*link) {
    document *doc = *link;
    if (strcmp(doc->uri, uri) == 0) {
      *link = doc->next;
      free(doc->uri);
      free(doc->path);
      free(doc->text);
      free(doc);
      return;
    }
    link = &doc->next;
  }
}

static void line_col_for_offset(const char *src, long long offset,
                                int *line_out, int *col_out) {
  int line = 1;
  int col = 1;

  for (long long i = 0; i < offset && src[i] != '\0'; i++) {
    if (src[i] == '\n') {
      line++;
      col = 1;
    } else {
      col++;
    }
  }

  *line_out = line;
  *col_out = col;
}

static bool stmt_range(Ast *stmt, Ast *next_stmt, const char *src,
                       source_range *out_range);

static bool range_for_node_offsets(const char *src, Ast *node,
                                   long long end_offset,
                                   source_range *out_range) {
  if (!src || !node || !node->loc_info || !out_range) {
    return false;
  }

  out_range->start_offset = node->loc_info->absolute_offset;
  out_range->end_offset = end_offset;
  out_range->start_line = node->loc_info->line;
  out_range->start_col = node->loc_info->col;
  line_col_for_offset(src, end_offset, &out_range->end_line,
                      &out_range->end_col);
  return true;
}

static long long top_level_stmt_end_offset(const char *src,
                                           long long start_offset) {
  long long offset = start_offset;
  int depth = 0;
  bool in_string = false;
  bool escaping = false;
  char string_delim = '\0';
  bool saw_code = false;

  if (!src) {
    return 0;
  }

  while (src[offset] != '\0') {
    char ch = src[offset];

    if (in_string) {
      if (escaping) {
        escaping = false;
      } else if (ch == '\\') {
        escaping = true;
      } else if (ch == string_delim) {
        in_string = false;
        string_delim = '\0';
      }
      offset++;
      continue;
    }

    if (ch == '"' || ch == '\'' || ch == '`') {
      in_string = true;
      string_delim = ch;
      saw_code = true;
      offset++;
      continue;
    }

    if (ch == '#') {
      if (depth == 0) {
        return saw_code ? offset : offset;
      }
      while (src[offset] != '\0' && src[offset] != '\n') {
        offset++;
      }
      continue;
    }

    if (ch == '(' || ch == '[' || ch == '{') {
      depth++;
      saw_code = true;
      offset++;
      continue;
    }

    if (ch == ')' || ch == ']' || ch == '}') {
      if (depth > 0) {
        depth--;
      }
      saw_code = true;
      offset++;
      continue;
    }

    if (ch == ';' && depth == 0) {
      return offset + 1;
    }

    if (ch == '\n' && depth == 0 && saw_code) {
      return offset;
    }

    if (!isspace((unsigned char)ch)) {
      saw_code = true;
    }

    offset++;
  }

  return offset;
}

static bool stmt_range_for_doc(document *doc, Ast *stmt, Ast *next_stmt,
                               source_range *out_range) {
  long long end_offset = 0;

  if (!doc || !stmt_range(stmt, next_stmt, doc->text, out_range)) {
    return false;
  }

  if (next_stmt && next_stmt->loc_info) {
    return true;
  }

  end_offset =
      top_level_stmt_end_offset(doc->text, stmt->loc_info->absolute_offset);
  out_range->end_offset = end_offset;
  line_col_for_offset(doc->text, end_offset, &out_range->end_line,
                      &out_range->end_col);
  return true;
}

static Ast *find_stmt_at_line(document *doc, int line, Ast **next_stmt_out) {
  long long cursor_offset;
  AstList *stmt;

  if (!doc || !doc->root || doc->root->tag != AST_BODY || !doc->text) {
    return NULL;
  }

  cursor_offset = 0;
  for (int current_line = 1;
       doc->text[cursor_offset] != '\0' && current_line < line;
       cursor_offset++) {
    if (doc->text[cursor_offset] == '\n') {
      current_line++;
    }
  }

  stmt = doc->root->data.AST_BODY.stmts;
  while (stmt != NULL) {
    Ast *current = stmt->ast;
    Ast *next_stmt = stmt->next ? stmt->next->ast : NULL;
    source_range range;

    if (!stmt_range_for_doc(doc, current, next_stmt, &range)) {
      stmt = stmt->next;
      continue;
    }

    if (cursor_offset >= range.start_offset &&
        cursor_offset < range.end_offset) {
      if (next_stmt_out) {
        *next_stmt_out = next_stmt;
      }
      return current;
    }

    stmt = stmt->next;
  }

  return NULL;
}

static Ast *
find_selection_target_in_sequence(Ast *node, long long cursor_offset,
                                  long long end_offset,
                                  long long *target_end_offset_out) {
  AstList *stmt;

  if (!node || !node->loc_info) {
    return NULL;
  }

  if (node->tag != AST_BODY) {
    if (target_end_offset_out) {
      *target_end_offset_out = end_offset;
    }
    return node;
  }

  stmt = node->data.AST_BODY.stmts;
  while (stmt != NULL) {
    Ast *current = stmt->ast;
    Ast *next = stmt->next ? stmt->next->ast : NULL;
    long long child_end = end_offset;
    Ast *candidate;

    if (!current || !current->loc_info) {
      stmt = stmt->next;
      continue;
    }

    if (next && next->loc_info && next->loc_info->absolute_offset < child_end) {
      child_end = next->loc_info->absolute_offset;
    }

    if (cursor_offset < current->loc_info->absolute_offset ||
        cursor_offset >= child_end) {
      stmt = stmt->next;
      continue;
    }

    candidate = find_selection_target_in_sequence(
        current, cursor_offset, child_end, target_end_offset_out);
    if (candidate) {
      return candidate;
    }

    if (target_end_offset_out) {
      *target_end_offset_out = child_end;
    }
    return current;
  }

  if (target_end_offset_out) {
    *target_end_offset_out = end_offset;
  }
  return node;
}

static bool should_clamp_selection_end_with_scan(Ast *node) {
  if (!node) {
    return false;
  }

  switch (node->tag) {
  case AST_IMPORT:
  case AST_LET:
  case AST_TYPE_DECL:
  case AST_TRAIT_IMPL:
  case AST_LOOP:
    return false;
  default:
    return true;
  }
}

static long long offset_for_position(const char *src, int target_line,
                                     int target_char) {
  long long offset = 0;
  int line = 0;
  int character = 0;

  if (!src) {
    return 0;
  }

  while (src[offset] != '\0') {
    if (line == target_line && character == target_char) {
      return offset;
    }

    if (src[offset] == '\n') {
      line++;
      character = 0;
    } else {
      character++;
    }
    offset++;
  }

  return offset;
}

static bool is_identifier_char(char ch) {
  return isalnum((unsigned char)ch) || ch == '_';
}

static char *completion_prefix_at_position(const char *src, int line,
                                           int character) {
  long long end_offset = offset_for_position(src, line, character);
  long long start_offset = end_offset;
  size_t len;
  char *prefix;

  while (start_offset > 0 && is_identifier_char(src[start_offset - 1])) {
    start_offset--;
  }

  len = (size_t)(end_offset - start_offset);
  prefix = malloc(len + 1);
  if (!prefix) {
    return NULL;
  }

  memcpy(prefix, src + start_offset, len);
  prefix[len] = '\0';
  return prefix;
}

static Type *completion_item_type(TypeEnv *type_env) {
  if (!type_env) {
    return NULL;
  }

  return type_env->type;
}

static int completion_kind_for_type(TypeEnv *type_env) {
  Type *type = completion_item_type(type_env);

  if (!type) {
    return LSP_COMPLETION_KIND_TEXT;
  }

  if (type->kind == T_FN) {
    return LSP_COMPLETION_KIND_FUNCTION;
  }

  if (is_module(type)) {
    return LSP_COMPLETION_KIND_MODULE;
  }

  return LSP_COMPLETION_KIND_VARIABLE;
}

static void add_completion_item(struct json_object *items, ht *seen,
                                const char *label, int kind, Type *type) {
  struct json_object *item;
  char *sort_text;

  if (!items || !seen || !label || ht_get(seen, label)) {
    return;
  }

  if (!ht_set(seen, label, (void *)label)) {
    return;
  }

  item = json_object_new_object();
  json_object_object_add(item, "label", json_object_new_string(label));
  json_object_object_add(item, "kind", json_object_new_int(kind));
  json_object_object_add(item, "insertText", json_object_new_string(label));
  json_object_object_add(item, "filterText", json_object_new_string(label));
  sort_text = xstrdup(label);
  if (sort_text) {
    for (size_t i = 0; sort_text[i] != '\0'; i++) {
      sort_text[i] = (char)tolower((unsigned char)sort_text[i]);
    }
    json_object_object_add(item, "sortText", json_object_new_string(sort_text));
    free(sort_text);
  }

  json_object_array_add(items, item);
}

static void add_completion_item_if_matches(struct json_object *items, ht *seen,
                                           const char *label, int kind,
                                           Type *type, const char *prefix) {
  if (!label || !prefix) {
    return;
  }

  if (strncmp(label, prefix, strlen(prefix)) != 0) {
    return;
  }

  add_completion_item(items, seen, label, kind, type);
}

typedef struct completion_builtin_ctx {
  struct json_object *items;
  ht *seen;
  const char *prefix;
} completion_builtin_ctx;

static void add_builtin_completion(const char *name, TypeEnv *entry,
                                   void *ctx_ptr) {
  completion_builtin_ctx *ctx = (completion_builtin_ctx *)ctx_ptr;

  if (!ctx || !entry) {
    return;
  }

  add_completion_item_if_matches(ctx->items, ctx->seen, name,
                                 completion_kind_for_type(entry), entry->type,
                                 ctx->prefix);
}

static node_match choose_better_match(node_match current,
                                      node_match candidate) {
  if (!candidate.node) {
    return current;
  }

  if (!current.node) {
    return candidate;
  }

  if (candidate.start_offset > current.start_offset) {
    return candidate;
  }

  if (candidate.start_offset < current.start_offset) {
    return current;
  }

  if ((candidate.end_offset - candidate.start_offset) <
      (current.end_offset - current.start_offset)) {
    return candidate;
  }

  return current;
}

static node_match find_smallest_node_in_subtree(Ast *node,
                                                long long cursor_offset,
                                                long long end_offset);

static node_match visit_ast_list(AstList *list, long long cursor_offset,
                                 long long end_offset) {
  node_match best = {0};

  for (AstList *item = list; item; item = item->next) {
    Ast *current = item->ast;
    Ast *next = item->next ? item->next->ast : NULL;
    long long child_end = end_offset;
    node_match candidate;

    if (!current || !current->loc_info) {
      continue;
    }

    if (next && next->loc_info && next->loc_info->absolute_offset < child_end) {
      child_end = next->loc_info->absolute_offset;
    }

    candidate =
        find_smallest_node_in_subtree(current, cursor_offset, child_end);
    best = choose_better_match(best, candidate);
  }

  return best;
}

static node_match visit_ast_array(Ast *items, size_t len,
                                  long long cursor_offset,
                                  long long end_offset) {
  node_match best = {0};

  for (size_t i = 0; i < len; i++) {
    Ast *current = items + i;
    Ast *next = (i + 1 < len) ? items + i + 1 : NULL;
    long long child_end = end_offset;
    node_match candidate;

    if (!current || !current->loc_info) {
      continue;
    }

    if (next && next->loc_info && next->loc_info->absolute_offset < child_end) {
      child_end = next->loc_info->absolute_offset;
    }

    candidate =
        find_smallest_node_in_subtree(current, cursor_offset, child_end);
    best = choose_better_match(best, candidate);
  }

  return best;
}

static node_match find_smallest_node_in_subtree(Ast *node,
                                                long long cursor_offset,
                                                long long end_offset) {
  node_match best = {0};
  long long start_offset;
  node_match candidate = {0};
  Ast *children[4] = {0};
  size_t child_count = 0;

  if (!node) {
    return best;
  }

  if (!node->loc_info) {
    if (node->tag == AST_BODY) {
      return visit_ast_list(node->data.AST_BODY.stmts, cursor_offset,
                            end_offset);
    }
    return best;
  }

  start_offset = node->loc_info->absolute_offset;
  if (start_offset > cursor_offset || cursor_offset >= end_offset ||
      start_offset >= end_offset) {
    return best;
  }

  best = (node_match){
      .node = node,
      .start_offset = start_offset,
      .end_offset = end_offset,
  };

  switch (node->tag) {
  case AST_BODY:
    return choose_better_match(best, visit_ast_list(node->data.AST_BODY.stmts,
                                                    cursor_offset, end_offset));
  case AST_APPLICATION:
    candidate = find_smallest_node_in_subtree(
        node->data.AST_APPLICATION.function, cursor_offset,
        node->data.AST_APPLICATION.len > 0 &&
                node->data.AST_APPLICATION.args[0].loc_info
            ? node->data.AST_APPLICATION.args[0].loc_info->absolute_offset
            : end_offset);
    best = choose_better_match(best, candidate);
    candidate = visit_ast_array(node->data.AST_APPLICATION.args,
                                node->data.AST_APPLICATION.len, cursor_offset,
                                end_offset);
    return choose_better_match(best, candidate);
  case AST_LET:
  case AST_TYPE_DECL:
  case AST_LOOP:
    children[child_count++] = node->data.AST_LET.binding;
    children[child_count++] = node->data.AST_LET.expr;
    if (node->data.AST_LET.in_expr) {
      children[child_count++] = node->data.AST_LET.in_expr;
    }
    break;
  case AST_BINOP:
  case AST_ASSOC:
    children[child_count++] = node->data.AST_BINOP.left;
    children[child_count++] = node->data.AST_BINOP.right;
    break;
  case AST_UNOP:
    children[child_count++] = node->data.AST_UNOP.expr;
    break;
  case AST_LAMBDA:
  case AST_MODULE:
    candidate = visit_ast_list(
        node->data.AST_LAMBDA.params, cursor_offset,
        node->data.AST_LAMBDA.body && node->data.AST_LAMBDA.body->loc_info
            ? node->data.AST_LAMBDA.body->loc_info->absolute_offset
            : end_offset);
    best = choose_better_match(best, candidate);
    candidate = visit_ast_list(
        node->data.AST_LAMBDA.type_annotations, cursor_offset,
        node->data.AST_LAMBDA.body && node->data.AST_LAMBDA.body->loc_info
            ? node->data.AST_LAMBDA.body->loc_info->absolute_offset
            : end_offset);
    best = choose_better_match(best, candidate);
    if (node->data.AST_LAMBDA.body) {
      candidate = find_smallest_node_in_subtree(node->data.AST_LAMBDA.body,
                                                cursor_offset, end_offset);
      best = choose_better_match(best, candidate);
    }
    return best;
  case AST_TRAIT_IMPL:
    children[child_count++] = node->data.AST_TRAIT_IMPL.impl;
    break;
  case AST_EXTERN_FN:
    children[child_count++] = node->data.AST_EXTERN_FN.signature_types;
    break;
  case AST_LIST:
  case AST_ARRAY:
  case AST_TUPLE:
  case AST_FMT_STRING:
    candidate =
        visit_ast_array(node->data.AST_LIST.items, node->data.AST_LIST.len,
                        cursor_offset, end_offset);
    return choose_better_match(best, candidate);
  case AST_MATCH:
    if (node->data.AST_MATCH.expr) {
      candidate = find_smallest_node_in_subtree(
          node->data.AST_MATCH.expr, cursor_offset,
          node->data.AST_MATCH.len > 0 &&
                  node->data.AST_MATCH.branches[0].loc_info
              ? node->data.AST_MATCH.branches[0].loc_info->absolute_offset
              : end_offset);
      best = choose_better_match(best, candidate);
    }
    candidate = visit_ast_array(node->data.AST_MATCH.branches,
                                node->data.AST_MATCH.len * 2, cursor_offset,
                                end_offset);
    return choose_better_match(best, candidate);
  case AST_RECORD_ACCESS:
    children[child_count++] = node->data.AST_RECORD_ACCESS.record;
    children[child_count++] = node->data.AST_RECORD_ACCESS.member;
    break;
  case AST_MATCH_GUARD_CLAUSE:
    children[child_count++] = node->data.AST_MATCH_GUARD_CLAUSE.test_expr;
    children[child_count++] = node->data.AST_MATCH_GUARD_CLAUSE.guard_expr;
    break;
  case AST_YIELD:
  case AST_SPREAD_OP:
    children[child_count++] = node->data.AST_YIELD.expr;
    break;
  case AST_RANGE_EXPRESSION:
    children[child_count++] = node->data.AST_RANGE_EXPRESSION.from;
    children[child_count++] = node->data.AST_RANGE_EXPRESSION.to;
    break;
  default:
    return best;
  }

  for (size_t i = 0; i < child_count; i++) {
    Ast *current = children[i];
    Ast *next = NULL;
    long long child_end = end_offset;

    if (!current || !current->loc_info) {
      continue;
    }

    for (size_t j = i + 1; j < child_count; j++) {
      if (children[j] && children[j]->loc_info) {
        next = children[j];
        break;
      }
    }

    if (next && next->loc_info && next->loc_info->absolute_offset < child_end) {
      child_end = next->loc_info->absolute_offset;
    }

    candidate =
        find_smallest_node_in_subtree(current, cursor_offset, child_end);
    best = choose_better_match(best, candidate);
  }

  return best;
}

static bool stmt_range(Ast *stmt, Ast *next_stmt, const char *src,
                       source_range *out_range) {
  long long end_offset = 0;

  if (!stmt || !stmt->loc_info || !src || !out_range) {
    return false;
  }

  out_range->start_offset = stmt->loc_info->absolute_offset;
  out_range->start_line = stmt->loc_info->line;
  out_range->start_col = stmt->loc_info->col;

  if (next_stmt && next_stmt->loc_info) {
    end_offset = next_stmt->loc_info->absolute_offset;
  } else {
    end_offset = (long long)strlen(src);
  }

  out_range->end_offset = end_offset;
  line_col_for_offset(src, end_offset, &out_range->end_line,
                      &out_range->end_col);
  return true;
}

static struct json_object *range_to_json(const source_range *range) {
  struct json_object *json_range = json_object_new_object();
  struct json_object *start = json_object_new_object();
  struct json_object *end = json_object_new_object();

  json_object_object_add(start, "line",
                         json_object_new_int(range->start_line - 1));
  json_object_object_add(start, "character",
                         json_object_new_int(range->start_col - 1));
  json_object_object_add(end, "line", json_object_new_int(range->end_line - 1));
  json_object_object_add(end, "character",
                         json_object_new_int(range->end_col - 1));

  json_object_object_add(json_range, "start", start);
  json_object_object_add(json_range, "end", end);
  return json_range;
}

static bool lsp_path_has_url_scheme(const char *path) {
  return path && strstr(path, "://") != NULL;
}

static char *path_to_file_uri(const char *path) {
  size_t len;
  size_t extra = 0;
  char *uri;
  char *out;

  if (!path) {
    return NULL;
  }

  if (lsp_path_has_url_scheme(path)) {
    return xstrdup(path);
  }

  len = strlen(path);
  for (size_t i = 0; i < len; i++) {
    unsigned char ch = (unsigned char)path[i];
    if (ch <= ' ' || ch == '%' || ch == '#' || ch == '?') {
      extra += 2;
    }
  }

  uri = malloc(strlen("file://") + len + extra + 1);
  if (!uri) {
    return NULL;
  }

  memcpy(uri, "file://", strlen("file://"));
  out = uri + strlen("file://");
  for (size_t i = 0; i < len; i++) {
    unsigned char ch = (unsigned char)path[i];
    if (ch <= ' ' || ch == '%' || ch == '#' || ch == '?') {
      static const char hex[] = "0123456789ABCDEF";
      *out++ = '%';
      *out++ = hex[(ch >> 4) & 0xf];
      *out++ = hex[ch & 0xf];
    } else {
      *out++ = (char)ch;
    }
  }
  *out = '\0';
  return uri;
}

static char *uri_for_definition_node(lsp_server *server, document *fallback_doc,
                                     Ast *node) {
  const char *src_file = NULL;

  if (node && node->loc_info) {
    src_file = node->loc_info->src_file;
  }

  if (!src_file || src_file[0] == '\0') {
    return fallback_doc && fallback_doc->uri ? xstrdup(fallback_doc->uri)
                                             : NULL;
  }

  if (server) {
    for (document *doc = server->docs; doc; doc = doc->next) {
      if (doc->path && strcmp(doc->path, src_file) == 0) {
        return xstrdup(doc->uri);
      }
    }
  }

  return path_to_file_uri(src_file);
}

static const char *identifier_name(Ast *node) {
  if (!node || node->tag != AST_IDENTIFIER) {
    return NULL;
  }
  return node->data.AST_IDENTIFIER.value;
}

static long long identifier_end_offset(Ast *node) {
  if (!node || !node->loc_info) {
    return 0;
  }

  if (node->tag == AST_IDENTIFIER && node->data.AST_IDENTIFIER.length > 0) {
    return node->loc_info->absolute_offset +
           (long long)node->data.AST_IDENTIFIER.length;
  }

  if (node->loc_info->col_end > 0 && node->loc_info->src_content) {
    long long line_start =
        node->loc_info->absolute_offset - (node->loc_info->col - 1);
    return line_start + node->loc_info->col_end;
  }

  return node->loc_info->absolute_offset + 1;
}

static bool range_for_definition_node(Ast *node, source_range *out_range) {
  long long end_offset;

  if (!node || !node->loc_info || !out_range) {
    return false;
  }

  if (node->tag == AST_LET || node->tag == AST_TYPE_DECL) {
    ObjString binding_name = {0};
    const char *src = node->loc_info->src_content;
    long long start = node->loc_info->absolute_offset;
    long long limit;

    if (src && get_let_binding_name(node, &binding_name) == 0 &&
        binding_name.chars) {
      limit = top_level_stmt_end_offset(src, start);
      for (long long pos = start; src[pos] != '\0' && pos < limit; pos++) {
        if ((pos == 0 || !is_identifier_char(src[pos - 1])) &&
            strncmp(src + pos, binding_name.chars,
                    (size_t)binding_name.length) == 0 &&
            !is_identifier_char(src[pos + binding_name.length])) {
          out_range->start_offset = pos;
          out_range->end_offset = pos + binding_name.length;
          line_col_for_offset(src, out_range->start_offset,
                              &out_range->start_line, &out_range->start_col);
          line_col_for_offset(src, out_range->end_offset,
                              &out_range->end_line, &out_range->end_col);
          return true;
        }
      }
    }
  }

  end_offset = identifier_end_offset(node);
  out_range->start_offset = node->loc_info->absolute_offset;
  out_range->end_offset = end_offset;
  out_range->start_line = node->loc_info->line;
  out_range->start_col = node->loc_info->col;
  out_range->end_line = node->loc_info->line;
  out_range->end_col = node->loc_info->col + 1;

  if (node->loc_info->src_content) {
    line_col_for_offset(node->loc_info->src_content, end_offset,
                        &out_range->end_line, &out_range->end_col);
  } else if (node->tag == AST_IDENTIFIER && node->data.AST_IDENTIFIER.length) {
    out_range->end_col =
        node->loc_info->col + (int)node->data.AST_IDENTIFIER.length;
  } else if (node->loc_info->col_end > 0) {
    out_range->end_col = node->loc_info->col_end + 1;
  }

  return true;
}

static struct json_object *definition_location_to_json(lsp_server *server,
                                                       document *doc,
                                                       Ast *node) {
  source_range range;
  char *uri;
  struct json_object *location;

  if (!range_for_definition_node(node, &range)) {
    return NULL;
  }

  uri = uri_for_definition_node(server, doc, node);
  if (!uri) {
    return NULL;
  }

  location = json_object_new_object();
  json_object_object_add(location, "uri", json_object_new_string(uri));
  json_object_object_add(location, "range", range_to_json(&range));
  free(uri);
  return location;
}

static bool json_position_matches(struct json_object *position, int line,
                                  int character) {
  struct json_object *line_obj = NULL;
  struct json_object *character_obj = NULL;

  return position && json_object_object_get_ex(position, "line", &line_obj) &&
         json_object_object_get_ex(position, "character", &character_obj) &&
         json_object_get_int(line_obj) == line &&
         json_object_get_int(character_obj) == character;
}

static bool json_range_matches_source_range(struct json_object *range,
                                            const source_range *source) {
  struct json_object *start = NULL;
  struct json_object *end = NULL;

  return range && source &&
         json_object_object_get_ex(range, "start", &start) &&
         json_object_object_get_ex(range, "end", &end) &&
         json_position_matches(start, source->start_line - 1,
                               source->start_col - 1) &&
         json_position_matches(end, source->end_line - 1,
                               source->end_col - 1);
}

static bool definition_location_exists(struct json_object *locations,
                                       const char *uri,
                                       const source_range *range) {
  if (!locations || !json_object_is_type(locations, json_type_array) || !uri ||
      !range) {
    return false;
  }

  for (size_t i = 0; i < json_object_array_length(locations); i++) {
    struct json_object *location = json_object_array_get_idx(locations, i);
    struct json_object *location_uri = NULL;
    struct json_object *location_range = NULL;
    const char *existing_uri;

    if (!location ||
        !json_object_object_get_ex(location, "uri", &location_uri) ||
        !json_object_object_get_ex(location, "range", &location_range)) {
      continue;
    }

    existing_uri = json_object_get_string(location_uri);
    if (existing_uri && strcmp(existing_uri, uri) == 0 &&
        json_range_matches_source_range(location_range, range)) {
      return true;
    }
  }

  return false;
}

static bool definition_locations_add_unique(lsp_server *server, document *doc,
                                            struct json_object *locations,
                                            Ast *node) {
  source_range range;
  char *uri;
  struct json_object *location;

  if (!locations || !node || !range_for_definition_node(node, &range)) {
    return false;
  }

  uri = uri_for_definition_node(server, doc, node);
  if (!uri) {
    return false;
  }

  if (definition_location_exists(locations, uri, &range)) {
    free(uri);
    return true;
  }

  free(uri);
  location = definition_location_to_json(server, doc, node);
  if (!location) {
    return false;
  }

  json_object_array_add(locations, location);
  return true;
}

static char *identifier_text_at_offset(const char *src, long long offset) {
  long long start = offset;
  long long end = offset;
  size_t len;
  char *name;

  if (!src || offset < 0 || src[offset] == '\0') {
    return NULL;
  }

  if (!is_identifier_char(src[offset]) && offset > 0 &&
      is_identifier_char(src[offset - 1])) {
    start = offset - 1;
    end = offset;
  }

  if (!is_identifier_char(src[start])) {
    return NULL;
  }

  while (start > 0 && is_identifier_char(src[start - 1])) {
    start--;
  }

  while (src[end] != '\0' && is_identifier_char(src[end])) {
    end++;
  }

  len = (size_t)(end - start);
  name = malloc(len + 1);
  if (!name) {
    return NULL;
  }

  memcpy(name, src + start, len);
  name[len] = '\0';
  return name;
}

static bool cursor_on_identifier(definition_search *search, Ast *node) {
  long long start;
  long long end;
  const char *name;

  if (!search || !node || node->tag != AST_IDENTIFIER || !node->loc_info) {
    return false;
  }

  if (search->target_node == node) {
    return true;
  }

  name = identifier_name(node);
  if (!name || !search->target_name || strcmp(name, search->target_name) != 0) {
    return false;
  }

  start = node->loc_info->absolute_offset;
  end = identifier_end_offset(node);
  return search->cursor_offset >= start && search->cursor_offset <= end;
}

static definition_binding *definition_lookup(definition_binding *env,
                                             const char *name) {
  for (definition_binding *binding = env; binding; binding = binding->next) {
    if (binding->name && name && strcmp(binding->name, name) == 0) {
      return binding;
    }
  }
  return NULL;
}

static definition_binding *
definition_push_binding(definition_search *search, definition_binding *env,
                        const char *name, Ast *definition_node,
                        Ast *value_node) {
  definition_binding *binding;

  if (!search || !name || !definition_node) {
    return env;
  }

  binding = calloc(1, sizeof(*binding));
  if (!binding) {
    return env;
  }

  binding->name = name;
  binding->definition_node = definition_node;
  binding->value_node = value_node;
  binding->next = env;
  binding->alloc_next = search->allocated_bindings;
  search->allocated_bindings = binding;
  return binding;
}

static void definition_search_free_bindings(definition_search *search) {
  definition_binding *binding;

  if (!search) {
    return;
  }

  binding = search->allocated_bindings;
  while (binding) {
    definition_binding *next = binding->alloc_next;
    free(binding);
    binding = next;
  }
  search->allocated_bindings = NULL;
}

static Ast *module_body(Ast *module_ast) {
  if (!module_ast) {
    return NULL;
  }

  if (module_ast->tag == AST_MODULE || module_ast->tag == AST_LAMBDA) {
    return module_ast->data.AST_LAMBDA.body;
  }

  return module_ast->tag == AST_BODY ? module_ast : NULL;
}

static Ast *module_definition_node(Ast *module_ast) {
  Ast *body = module_body(module_ast);
  if (!body) {
    return module_ast;
  }
  if (body->tag != AST_BODY) {
    return body;
  }
  if (body->data.AST_BODY.stmts && body->data.AST_BODY.stmts->ast) {
    return body->data.AST_BODY.stmts->ast;
  }
  return module_ast;
}

static Ast *import_module_ast(Ast *import_ast) {
  const char *key;
  YLCModule *module;

  if (!import_ast || import_ast->tag != AST_IMPORT) {
    return NULL;
  }

  key = import_ast->data.AST_IMPORT.fully_qualified_name;
  if (!key) {
    return NULL;
  }

  module = get_module(key);
  return module ? module->ast : NULL;
}

static const char *stmt_binding_name(Ast *stmt) {
  ObjString name = {0};

  if (!stmt) {
    return NULL;
  }

  switch (stmt->tag) {
  case AST_LET:
  case AST_TYPE_DECL:
    if (get_let_binding_name(stmt, &name) == 0) {
      return name.chars;
    }
    return NULL;
  case AST_IMPORT:
    return stmt->data.AST_IMPORT.identifier;
  default:
    return NULL;
  }
}

static Ast *stmt_definition_node(Ast *stmt) {
  if (!stmt) {
    return NULL;
  }

  switch (stmt->tag) {
  case AST_LET:
  case AST_TYPE_DECL:
    return stmt;
  case AST_IMPORT: {
    Ast *mod_ast = import_module_ast(stmt);
    Ast *mod_def = module_definition_node(mod_ast);
    return mod_def ? mod_def : stmt;
  }
  default:
    return NULL;
  }
}

static Ast *stmt_value_node(Ast *stmt) {
  if (!stmt) {
    return NULL;
  }

  if (stmt->tag == AST_IMPORT) {
    return import_module_ast(stmt);
  }

  if ((stmt->tag == AST_LET || stmt->tag == AST_TYPE_DECL) &&
      stmt->data.AST_LET.expr &&
      stmt->data.AST_LET.expr->tag == AST_MODULE) {
    return stmt->data.AST_LET.expr;
  }

  return NULL;
}

static bool find_module_member_definition(Ast *module_ast, const char *name,
                                          Ast **definition_node_out,
                                          Ast **value_node_out) {
  Ast *body = module_body(module_ast);

  if (definition_node_out) {
    *definition_node_out = NULL;
  }
  if (value_node_out) {
    *value_node_out = NULL;
  }

  if (!body || !name) {
    return false;
  }

  if (body->tag != AST_BODY) {
    const char *candidate = stmt_binding_name(body);
    if (!candidate || strcmp(candidate, name) != 0) {
      return false;
    }

    if (definition_node_out) {
      *definition_node_out = stmt_definition_node(body);
    }
    if (value_node_out) {
      *value_node_out = stmt_value_node(body);
    }
    return definition_node_out ? *definition_node_out != NULL : true;
  }

  for (AstList *stmt = body->data.AST_BODY.stmts; stmt; stmt = stmt->next) {
    const char *candidate = stmt_binding_name(stmt->ast);
    if (!candidate || strcmp(candidate, name) != 0) {
      continue;
    }

    if (definition_node_out) {
      *definition_node_out = stmt_definition_node(stmt->ast);
    }
    if (value_node_out) {
      *value_node_out = stmt_value_node(stmt->ast);
    }
    return definition_node_out ? *definition_node_out != NULL : true;
  }

  return false;
}

static definition_binding *
definition_push_module_exports(definition_search *search,
                               definition_binding *env, Ast *module_ast) {
  Ast *body = module_body(module_ast);

  if (!body) {
    return env;
  }

  if (body->tag != AST_BODY) {
    const char *name = stmt_binding_name(body);
    Ast *def_node = stmt_definition_node(body);
    Ast *value_node = stmt_value_node(body);
    if (name && def_node) {
      env = definition_push_binding(search, env, name, def_node, value_node);
    }
    return env;
  }

  for (AstList *stmt = body->data.AST_BODY.stmts; stmt; stmt = stmt->next) {
    const char *name = stmt_binding_name(stmt->ast);
    Ast *def_node = stmt_definition_node(stmt->ast);
    Ast *value_node = stmt_value_node(stmt->ast);
    if (name && def_node) {
      env = definition_push_binding(search, env, name, def_node, value_node);
    }
  }

  return env;
}

static bool definition_set_from_binding(definition_search *search,
                                        definition_binding *binding) {
  if (!search || !binding) {
    return false;
  }

  search->definition_node = binding->definition_node;
  search->done = true;
  return true;
}

static bool definition_set_node(definition_search *search, Ast *node) {
  if (!search) {
    return false;
  }

  search->definition_node = node;
  search->done = true;
  return node != NULL;
}

static bool resolve_expr_definition(definition_search *search,
                                    definition_binding *env, Ast *node);

static definition_binding *
definition_push_pattern_bindings(definition_search *search,
                                 definition_binding *env, Ast *pattern,
                                 Ast *value_node) {
  if (!pattern) {
    return env;
  }

  if (pattern->tag == AST_LET) {
    return definition_push_pattern_bindings(search, env,
                                            pattern->data.AST_LET.binding,
                                            pattern->data.AST_LET.expr);
  }

  if (pattern->tag == AST_IDENTIFIER) {
    const char *name = identifier_name(pattern);
    if (name && strcmp(name, "_") != 0) {
      return definition_push_binding(search, env, name, pattern, value_node);
    }
    return env;
  }

  if (pattern->tag == AST_TUPLE || pattern->tag == AST_LIST ||
      pattern->tag == AST_ARRAY) {
    for (size_t i = 0; i < pattern->data.AST_LIST.len; i++) {
      env = definition_push_pattern_bindings(
          search, env, pattern->data.AST_LIST.items + i, NULL);
    }
  }

  if (pattern->tag == AST_BINOP &&
      pattern->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {
    env = definition_push_pattern_bindings(search, env,
                                           pattern->data.AST_BINOP.left, NULL);
    env = definition_push_pattern_bindings(search, env,
                                           pattern->data.AST_BINOP.right, NULL);
  }

  return env;
}

static bool definition_target_in_pattern(definition_search *search,
                                         Ast *pattern) {
  if (!pattern) {
    return false;
  }

  if (pattern->tag == AST_LET) {
    return definition_target_in_pattern(search, pattern->data.AST_LET.binding);
  }

  if (pattern->tag == AST_IDENTIFIER && cursor_on_identifier(search, pattern)) {
    return definition_set_node(search, pattern);
  }

  if (pattern->tag == AST_TUPLE || pattern->tag == AST_LIST ||
      pattern->tag == AST_ARRAY) {
    for (size_t i = 0; i < pattern->data.AST_LIST.len; i++) {
      if (definition_target_in_pattern(search,
                                       pattern->data.AST_LIST.items + i)) {
        return true;
      }
    }
  }

  if (pattern->tag == AST_BINOP &&
      pattern->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {
    return definition_target_in_pattern(search, pattern->data.AST_BINOP.left) ||
           definition_target_in_pattern(search, pattern->data.AST_BINOP.right);
  }

  return false;
}

static bool definition_target_in_stmt_binding(definition_search *search,
                                              Ast *stmt) {
  const char *name;

  if (!search || !stmt || search->target_node != stmt ||
      !search->target_name) {
    return false;
  }

  name = stmt_binding_name(stmt);
  if (!name || strcmp(name, search->target_name) != 0) {
    return false;
  }

  return definition_set_node(search, stmt);
}

static Ast *module_ast_for_expr(definition_binding *env, Ast *expr) {
  if (!expr) {
    return NULL;
  }

  if (expr->tag == AST_IDENTIFIER) {
    definition_binding *binding =
        definition_lookup(env, expr->data.AST_IDENTIFIER.value);
    return binding ? binding->value_node : NULL;
  }

  if (expr->tag == AST_RECORD_ACCESS) {
    Ast *parent_module =
        module_ast_for_expr(env, expr->data.AST_RECORD_ACCESS.record);
    Ast *member = expr->data.AST_RECORD_ACCESS.member;
    Ast *member_def = NULL;
    Ast *member_value = NULL;
    const char *member_name = identifier_name(member);

    if (find_module_member_definition(parent_module, member_name, &member_def,
                                      &member_value)) {
      (void)member_def;
      return member_value;
    }
  }

  return NULL;
}

static bool resolve_record_access_definition(definition_search *search,
                                             definition_binding *env,
                                             Ast *node) {
  Ast *record;
  Ast *member;

  if (!node || node->tag != AST_RECORD_ACCESS) {
    return false;
  }

  record = node->data.AST_RECORD_ACCESS.record;
  member = node->data.AST_RECORD_ACCESS.member;

  if (cursor_on_identifier(search, member)) {
    Ast *module_ast = module_ast_for_expr(env, record);
    Ast *member_def = NULL;
    Ast *member_value = NULL;
    const char *member_name = identifier_name(member);
    if (find_module_member_definition(module_ast, member_name, &member_def,
                                      &member_value)) {
      (void)member_value;
      return definition_set_node(search, member_def);
    }
    search->done = true;
    return true;
  }

  return resolve_expr_definition(search, env, record);
}

static bool resolve_body_definition(definition_search *search,
                                    definition_binding *env, Ast *body);

static bool resolve_lambda_definition(definition_search *search,
                                      definition_binding *env, Ast *lambda) {
  definition_binding *body_env = env;

  if (!lambda) {
    return false;
  }

  for (AstList *param = lambda->data.AST_LAMBDA.params; param;
       param = param->next) {
    if (definition_target_in_pattern(search, param->ast)) {
      return true;
    }
    if (param->ast && param->ast->tag == AST_LET &&
        resolve_expr_definition(search, body_env,
                                param->ast->data.AST_LET.expr)) {
      return true;
    }
    body_env =
        definition_push_pattern_bindings(search, body_env, param->ast, NULL);
  }

  return resolve_expr_definition(search, body_env,
                                 lambda->data.AST_LAMBDA.body);
}

static bool resolve_import_definition(definition_search *search, Ast *node) {
  Ast *module_ast;
  Ast *module_def;

  if (!search || !node || search->target_node != node ||
      node->tag != AST_IMPORT || !search->target_name ||
      !node->data.AST_IMPORT.identifier ||
      strcmp(search->target_name, node->data.AST_IMPORT.identifier) != 0) {
    return false;
  }

  module_ast = import_module_ast(node);
  module_def = module_definition_node(module_ast);
  definition_set_node(search, module_def ? module_def : node);
  return true;
}

static bool resolve_let_definition(definition_search *search,
                                   definition_binding *env, Ast *node) {
  Ast *binding;
  Ast *expr;
  Ast *body;
  definition_binding *expr_env = env;

  if (!node) {
    return false;
  }

  binding = node->data.AST_LET.binding;
  expr = node->data.AST_LET.expr;
  body = node->data.AST_LET.in_expr;

  if (definition_target_in_stmt_binding(search, node)) {
    return true;
  }

  if (binding && binding->tag == AST_IDENTIFIER &&
      cursor_on_identifier(search, binding)) {
    return definition_set_node(search, node);
  }

  if (definition_target_in_pattern(search, binding)) {
    return true;
  }

  if (binding && binding->tag == AST_IDENTIFIER && expr &&
      expr->tag == AST_LAMBDA) {
    expr_env = definition_push_binding(search, env, identifier_name(binding),
                                       node, expr);
  }

  if (resolve_expr_definition(search, expr_env, expr)) {
    return true;
  }

  if (body) {
    definition_binding *body_env =
        definition_push_pattern_bindings(search, env, binding, stmt_value_node(node));
    return resolve_expr_definition(search, body_env, body);
  }

  return false;
}

static bool resolve_match_definition(definition_search *search,
                                     definition_binding *env, Ast *node) {
  if (resolve_expr_definition(search, env, node->data.AST_MATCH.expr)) {
    return true;
  }

  for (size_t i = 0; i < node->data.AST_MATCH.len; i++) {
    Ast *pattern = node->data.AST_MATCH.branches + (i * 2);
    Ast *body = node->data.AST_MATCH.branches + (i * 2) + 1;
    definition_binding *branch_env;

    if (definition_target_in_pattern(search, pattern)) {
      return true;
    }

    branch_env = definition_push_pattern_bindings(search, env, pattern, NULL);
    if (resolve_expr_definition(search, branch_env, body)) {
      return true;
    }
  }

  return false;
}

static bool resolve_expr_definition(definition_search *search,
                                    definition_binding *env, Ast *node) {
  if (!search || search->done || !node) {
    return search && search->done;
  }

  switch (node->tag) {
  case AST_IDENTIFIER:
    if (cursor_on_identifier(search, node)) {
      definition_binding *binding =
          definition_lookup(env, node->data.AST_IDENTIFIER.value);
      if (binding) {
        return definition_set_from_binding(search, binding);
      }
      search->done = true;
      return true;
    }
    return false;

  case AST_BODY:
    return resolve_body_definition(search, env, node);

  case AST_IMPORT:
    return resolve_import_definition(search, node);

  case AST_LET:
  case AST_TYPE_DECL:
  case AST_LOOP:
    return resolve_let_definition(search, env, node);

  case AST_LAMBDA:
  case AST_MODULE:
    return resolve_lambda_definition(search, env, node);

  case AST_APPLICATION:
    if (resolve_expr_definition(search, env,
                                node->data.AST_APPLICATION.function)) {
      return true;
    }
    for (size_t i = 0; i < node->data.AST_APPLICATION.len; i++) {
      if (resolve_expr_definition(search, env,
                                  node->data.AST_APPLICATION.args + i)) {
        return true;
      }
    }
    return false;

  case AST_BINOP:
  case AST_ASSOC:
    return resolve_expr_definition(search, env, node->data.AST_BINOP.left) ||
           resolve_expr_definition(search, env, node->data.AST_BINOP.right);

  case AST_UNOP:
    return resolve_expr_definition(search, env, node->data.AST_UNOP.expr);

  case AST_TUPLE:
  case AST_LIST:
  case AST_ARRAY:
  case AST_FMT_STRING:
    for (size_t i = 0; i < node->data.AST_LIST.len; i++) {
      if (resolve_expr_definition(search, env, node->data.AST_LIST.items + i)) {
        return true;
      }
    }
    return false;

  case AST_MATCH:
    return resolve_match_definition(search, env, node);

  case AST_RECORD_ACCESS:
    return resolve_record_access_definition(search, env, node);

  case AST_MATCH_GUARD_CLAUSE:
    return resolve_expr_definition(
               search, env, node->data.AST_MATCH_GUARD_CLAUSE.test_expr) ||
           resolve_expr_definition(
               search, env, node->data.AST_MATCH_GUARD_CLAUSE.guard_expr);

  case AST_YIELD:
  case AST_SPREAD_OP:
    return resolve_expr_definition(search, env, node->data.AST_YIELD.expr);

  case AST_RANGE_EXPRESSION:
    return resolve_expr_definition(
               search, env, node->data.AST_RANGE_EXPRESSION.from) ||
           resolve_expr_definition(search, env,
                                   node->data.AST_RANGE_EXPRESSION.to);

  case AST_TRAIT_IMPL:
    return resolve_expr_definition(search, env, node->data.AST_TRAIT_IMPL.impl);

  case AST_EXTERN_FN:
    return resolve_expr_definition(search, env,
                                   node->data.AST_EXTERN_FN.signature_types);

  default:
    return false;
  }
}

static definition_binding *
definition_push_stmt_binding(definition_search *search, definition_binding *env,
                             Ast *stmt) {
  const char *name;
  Ast *def_node;
  Ast *value_node;

  if (!stmt) {
    return env;
  }

  if (stmt->tag == AST_IMPORT && stmt->data.AST_IMPORT.import_all) {
    return definition_push_module_exports(search, env, import_module_ast(stmt));
  }

  name = stmt_binding_name(stmt);
  def_node = stmt_definition_node(stmt);
  value_node = stmt_value_node(stmt);

  if (!name || !def_node) {
    return env;
  }

  return definition_push_binding(search, env, name, def_node, value_node);
}

static bool resolve_body_definition(definition_search *search,
                                    definition_binding *env, Ast *body) {
  if (!body || body->tag != AST_BODY) {
    return resolve_expr_definition(search, env, body);
  }

  for (AstList *stmt = body->data.AST_BODY.stmts; stmt; stmt = stmt->next) {
    if (resolve_expr_definition(search, env, stmt->ast)) {
      return true;
    }
    env = definition_push_stmt_binding(search, env, stmt->ast);
  }

  return false;
}

static bool rename_definition_matches(Ast *lhs, Ast *rhs) {
  return lhs && rhs && lhs == rhs;
}

static bool rename_node_in_current_doc(rename_collect *collect, Ast *node) {
  if (!collect || !collect->doc || !node || !node->loc_info) {
    return false;
  }

  if (!node->loc_info->src_file || !collect->doc->path) {
    return true;
  }

  return strcmp(node->loc_info->src_file, collect->doc->path) == 0;
}

static void rename_add_edit(rename_collect *collect, Ast *node) {
  source_range range;
  struct json_object *edit;

  if (!collect || !collect->edits || !collect->new_name ||
      !rename_node_in_current_doc(collect, node) ||
      !range_for_definition_node(node, &range)) {
    return;
  }

  edit = json_object_new_object();
  json_object_object_add(edit, "range", range_to_json(&range));
  json_object_object_add(edit, "newText",
                         json_object_new_string(collect->new_name));
  json_object_array_add(collect->edits, edit);
}

static definition_binding *
rename_push_pattern_bindings(rename_collect *collect, definition_binding *env,
                             Ast *pattern, Ast *value_node) {
  if (!pattern) {
    return env;
  }

  if (pattern->tag == AST_LET) {
    return rename_push_pattern_bindings(collect, env,
                                        pattern->data.AST_LET.binding,
                                        pattern->data.AST_LET.expr);
  }

  if (pattern->tag == AST_IDENTIFIER) {
    const char *name = identifier_name(pattern);
    if (name && strcmp(name, "_") != 0) {
      if (rename_definition_matches(pattern, collect->target_definition_node)) {
        rename_add_edit(collect, pattern);
      }
      return definition_push_binding(&collect->bindings, env, name, pattern,
                                     value_node);
    }
    return env;
  }

  if (pattern->tag == AST_TUPLE || pattern->tag == AST_LIST ||
      pattern->tag == AST_ARRAY) {
    for (size_t i = 0; i < pattern->data.AST_LIST.len; i++) {
      env = rename_push_pattern_bindings(collect, env,
                                         pattern->data.AST_LIST.items + i,
                                         NULL);
    }
  }

  if (pattern->tag == AST_BINOP &&
      pattern->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {
    env = rename_push_pattern_bindings(collect, env,
                                       pattern->data.AST_BINOP.left, NULL);
    env = rename_push_pattern_bindings(collect, env,
                                       pattern->data.AST_BINOP.right, NULL);
  }

  return env;
}

static bool rename_collect_expr(rename_collect *collect, definition_binding *env,
                                Ast *node);

static void rename_collect_lambda(rename_collect *collect,
                                  definition_binding *env, Ast *lambda) {
  definition_binding *body_env = env;

  if (!lambda) {
    return;
  }

  for (AstList *param = lambda->data.AST_LAMBDA.params; param;
       param = param->next) {
    if (param->ast && param->ast->tag == AST_LET) {
      rename_collect_expr(collect, body_env, param->ast->data.AST_LET.expr);
    }
    body_env = rename_push_pattern_bindings(collect, body_env, param->ast,
                                            NULL);
  }

  rename_collect_expr(collect, body_env, lambda->data.AST_LAMBDA.body);
}

static void rename_collect_record_access(rename_collect *collect,
                                         definition_binding *env, Ast *node) {
  Ast *record;
  Ast *member;
  Ast *module_ast;
  Ast *member_def = NULL;
  Ast *member_value = NULL;
  const char *member_name;

  if (!node || node->tag != AST_RECORD_ACCESS) {
    return;
  }

  record = node->data.AST_RECORD_ACCESS.record;
  member = node->data.AST_RECORD_ACCESS.member;

  rename_collect_expr(collect, env, record);

  member_name = identifier_name(member);
  module_ast = module_ast_for_expr(env, record);
  if (find_module_member_definition(module_ast, member_name, &member_def,
                                    &member_value) &&
      rename_definition_matches(member_def, collect->target_definition_node)) {
    (void)member_value;
    rename_add_edit(collect, member);
  }
}

static void rename_collect_match(rename_collect *collect,
                                 definition_binding *env, Ast *node) {
  if (!node || node->tag != AST_MATCH) {
    return;
  }

  rename_collect_expr(collect, env, node->data.AST_MATCH.expr);
  for (size_t i = 0; i < node->data.AST_MATCH.len; i++) {
    Ast *pattern = node->data.AST_MATCH.branches + (i * 2);
    Ast *body = node->data.AST_MATCH.branches + (i * 2) + 1;
    definition_binding *branch_env =
        rename_push_pattern_bindings(collect, env, pattern, NULL);
    rename_collect_expr(collect, branch_env, body);
  }
}

static void rename_collect_let(rename_collect *collect, definition_binding *env,
                               Ast *node) {
  Ast *binding;
  Ast *expr;
  Ast *body;
  definition_binding *expr_env = env;

  if (!node) {
    return;
  }

  binding = node->data.AST_LET.binding;
  expr = node->data.AST_LET.expr;
  body = node->data.AST_LET.in_expr;

  if (rename_definition_matches(node, collect->target_definition_node)) {
    rename_add_edit(collect, node);
  }

  if (!body) {
    (void)rename_push_pattern_bindings(collect, env, binding,
                                       stmt_value_node(node));
  }

  if (binding && binding->tag == AST_IDENTIFIER && expr &&
      expr->tag == AST_LAMBDA) {
    expr_env = definition_push_binding(&collect->bindings, env,
                                       identifier_name(binding), node, expr);
  }

  rename_collect_expr(collect, expr_env, expr);

  if (body) {
    definition_binding *body_env = rename_push_pattern_bindings(
        collect, env, binding, stmt_value_node(node));
    rename_collect_expr(collect, body_env, body);
  }
}

static bool rename_collect_expr(rename_collect *collect, definition_binding *env,
                                Ast *node) {
  if (!collect || !node) {
    return false;
  }

  switch (node->tag) {
  case AST_IDENTIFIER: {
    definition_binding *binding =
        definition_lookup(env, node->data.AST_IDENTIFIER.value);
    if (binding &&
        rename_definition_matches(binding->definition_node,
                                  collect->target_definition_node)) {
      rename_add_edit(collect, node);
    }
    return true;
  }

  case AST_BODY: {
    definition_binding *body_env = env;
    for (AstList *stmt = node->data.AST_BODY.stmts; stmt; stmt = stmt->next) {
      rename_collect_expr(collect, body_env, stmt->ast);
      body_env = definition_push_stmt_binding(&collect->bindings, body_env,
                                              stmt->ast);
    }
    return true;
  }

  case AST_IMPORT:
    return true;

  case AST_LET:
  case AST_TYPE_DECL:
  case AST_LOOP:
    rename_collect_let(collect, env, node);
    return true;

  case AST_LAMBDA:
  case AST_MODULE:
    rename_collect_lambda(collect, env, node);
    return true;

  case AST_APPLICATION:
    rename_collect_expr(collect, env, node->data.AST_APPLICATION.function);
    for (size_t i = 0; i < node->data.AST_APPLICATION.len; i++) {
      rename_collect_expr(collect, env, node->data.AST_APPLICATION.args + i);
    }
    return true;

  case AST_BINOP:
  case AST_ASSOC:
    rename_collect_expr(collect, env, node->data.AST_BINOP.left);
    rename_collect_expr(collect, env, node->data.AST_BINOP.right);
    return true;

  case AST_UNOP:
    rename_collect_expr(collect, env, node->data.AST_UNOP.expr);
    return true;

  case AST_TUPLE:
  case AST_LIST:
  case AST_ARRAY:
  case AST_FMT_STRING:
    for (size_t i = 0; i < node->data.AST_LIST.len; i++) {
      rename_collect_expr(collect, env, node->data.AST_LIST.items + i);
    }
    return true;

  case AST_MATCH:
    rename_collect_match(collect, env, node);
    return true;

  case AST_RECORD_ACCESS:
    rename_collect_record_access(collect, env, node);
    return true;

  case AST_MATCH_GUARD_CLAUSE:
    rename_collect_expr(collect, env,
                        node->data.AST_MATCH_GUARD_CLAUSE.test_expr);
    rename_collect_expr(collect, env,
                        node->data.AST_MATCH_GUARD_CLAUSE.guard_expr);
    return true;

  case AST_YIELD:
  case AST_SPREAD_OP:
    rename_collect_expr(collect, env, node->data.AST_YIELD.expr);
    return true;

  case AST_RANGE_EXPRESSION:
    rename_collect_expr(collect, env, node->data.AST_RANGE_EXPRESSION.from);
    rename_collect_expr(collect, env, node->data.AST_RANGE_EXPRESSION.to);
    return true;

  case AST_TRAIT_IMPL:
    rename_collect_expr(collect, env, node->data.AST_TRAIT_IMPL.impl);
    return true;

  case AST_EXTERN_FN:
    rename_collect_expr(collect, env, node->data.AST_EXTERN_FN.signature_types);
    return true;

  default:
    return false;
  }
}

static void reference_add_location(reference_collect *collect, Ast *node) {
  if (!collect || !collect->locations || !node) {
    return;
  }

  definition_locations_add_unique(collect->server, collect->doc,
                                  collect->locations, node);
}

static definition_binding *
reference_push_pattern_bindings(reference_collect *collect,
                                definition_binding *env, Ast *pattern,
                                Ast *value_node) {
  if (!pattern) {
    return env;
  }

  if (pattern->tag == AST_LET) {
    return reference_push_pattern_bindings(collect, env,
                                           pattern->data.AST_LET.binding,
                                           pattern->data.AST_LET.expr);
  }

  if (pattern->tag == AST_IDENTIFIER) {
    const char *name = identifier_name(pattern);
    if (name && strcmp(name, "_") != 0) {
      if (collect->include_declaration &&
          rename_definition_matches(pattern, collect->target_definition_node)) {
        reference_add_location(collect, pattern);
      }
      return definition_push_binding(&collect->bindings, env, name, pattern,
                                     value_node);
    }
    return env;
  }

  if (pattern->tag == AST_TUPLE || pattern->tag == AST_LIST ||
      pattern->tag == AST_ARRAY) {
    for (size_t i = 0; i < pattern->data.AST_LIST.len; i++) {
      env = reference_push_pattern_bindings(
          collect, env, pattern->data.AST_LIST.items + i, NULL);
    }
  }

  if (pattern->tag == AST_BINOP &&
      pattern->data.AST_BINOP.op == TOKEN_DOUBLE_COLON) {
    env = reference_push_pattern_bindings(collect, env,
                                          pattern->data.AST_BINOP.left, NULL);
    env = reference_push_pattern_bindings(collect, env,
                                          pattern->data.AST_BINOP.right, NULL);
  }

  return env;
}

static bool reference_collect_expr(reference_collect *collect,
                                   definition_binding *env, Ast *node);

static void reference_collect_lambda(reference_collect *collect,
                                     definition_binding *env, Ast *lambda) {
  definition_binding *body_env = env;

  if (!lambda) {
    return;
  }

  for (AstList *param = lambda->data.AST_LAMBDA.params; param;
       param = param->next) {
    if (param->ast && param->ast->tag == AST_LET) {
      reference_collect_expr(collect, body_env, param->ast->data.AST_LET.expr);
    }
    body_env =
        reference_push_pattern_bindings(collect, body_env, param->ast, NULL);
  }

  reference_collect_expr(collect, body_env, lambda->data.AST_LAMBDA.body);
}

static void reference_collect_record_access(reference_collect *collect,
                                            definition_binding *env,
                                            Ast *node) {
  Ast *record;
  Ast *member;
  Ast *module_ast;
  Ast *member_def = NULL;
  Ast *member_value = NULL;
  const char *member_name;

  if (!node || node->tag != AST_RECORD_ACCESS) {
    return;
  }

  record = node->data.AST_RECORD_ACCESS.record;
  member = node->data.AST_RECORD_ACCESS.member;

  reference_collect_expr(collect, env, record);

  member_name = identifier_name(member);
  module_ast = module_ast_for_expr(env, record);
  if (find_module_member_definition(module_ast, member_name, &member_def,
                                    &member_value) &&
      rename_definition_matches(member_def, collect->target_definition_node)) {
    (void)member_value;
    reference_add_location(collect, member);
  }
}

static void reference_collect_match(reference_collect *collect,
                                    definition_binding *env, Ast *node) {
  if (!node || node->tag != AST_MATCH) {
    return;
  }

  reference_collect_expr(collect, env, node->data.AST_MATCH.expr);
  for (size_t i = 0; i < node->data.AST_MATCH.len; i++) {
    Ast *pattern = node->data.AST_MATCH.branches + (i * 2);
    Ast *body = node->data.AST_MATCH.branches + (i * 2) + 1;
    definition_binding *branch_env =
        reference_push_pattern_bindings(collect, env, pattern, NULL);
    reference_collect_expr(collect, branch_env, body);
  }
}

static void reference_collect_let(reference_collect *collect,
                                  definition_binding *env, Ast *node) {
  Ast *binding;
  Ast *expr;
  Ast *body;
  definition_binding *expr_env = env;

  if (!node) {
    return;
  }

  binding = node->data.AST_LET.binding;
  expr = node->data.AST_LET.expr;
  body = node->data.AST_LET.in_expr;

  if (collect->include_declaration &&
      rename_definition_matches(node, collect->target_definition_node)) {
    reference_add_location(collect, node);
  }

  if (!body) {
    (void)reference_push_pattern_bindings(collect, env, binding,
                                          stmt_value_node(node));
  }

  if (binding && binding->tag == AST_IDENTIFIER && expr &&
      expr->tag == AST_LAMBDA) {
    expr_env = definition_push_binding(&collect->bindings, env,
                                       identifier_name(binding), node, expr);
  }

  reference_collect_expr(collect, expr_env, expr);

  if (body) {
    definition_binding *body_env = reference_push_pattern_bindings(
        collect, env, binding, stmt_value_node(node));
    reference_collect_expr(collect, body_env, body);
  }
}

static bool reference_collect_expr(reference_collect *collect,
                                   definition_binding *env, Ast *node) {
  if (!collect || !node) {
    return false;
  }

  switch (node->tag) {
  case AST_IDENTIFIER: {
    definition_binding *binding =
        definition_lookup(env, node->data.AST_IDENTIFIER.value);
    if (binding &&
        rename_definition_matches(binding->definition_node,
                                  collect->target_definition_node)) {
      reference_add_location(collect, node);
    }
    return true;
  }

  case AST_BODY: {
    definition_binding *body_env = env;
    for (AstList *stmt = node->data.AST_BODY.stmts; stmt; stmt = stmt->next) {
      reference_collect_expr(collect, body_env, stmt->ast);
      body_env = definition_push_stmt_binding(&collect->bindings, body_env,
                                              stmt->ast);
    }
    return true;
  }

  case AST_IMPORT:
    return true;

  case AST_LET:
  case AST_TYPE_DECL:
  case AST_LOOP:
    reference_collect_let(collect, env, node);
    return true;

  case AST_LAMBDA:
  case AST_MODULE:
    reference_collect_lambda(collect, env, node);
    return true;

  case AST_APPLICATION:
    reference_collect_expr(collect, env, node->data.AST_APPLICATION.function);
    for (size_t i = 0; i < node->data.AST_APPLICATION.len; i++) {
      reference_collect_expr(collect, env, node->data.AST_APPLICATION.args + i);
    }
    return true;

  case AST_BINOP:
  case AST_ASSOC:
    reference_collect_expr(collect, env, node->data.AST_BINOP.left);
    reference_collect_expr(collect, env, node->data.AST_BINOP.right);
    return true;

  case AST_UNOP:
    reference_collect_expr(collect, env, node->data.AST_UNOP.expr);
    return true;

  case AST_TUPLE:
  case AST_LIST:
  case AST_ARRAY:
  case AST_FMT_STRING:
    for (size_t i = 0; i < node->data.AST_LIST.len; i++) {
      reference_collect_expr(collect, env, node->data.AST_LIST.items + i);
    }
    return true;

  case AST_MATCH:
    reference_collect_match(collect, env, node);
    return true;

  case AST_RECORD_ACCESS:
    reference_collect_record_access(collect, env, node);
    return true;

  case AST_MATCH_GUARD_CLAUSE:
    reference_collect_expr(collect, env,
                           node->data.AST_MATCH_GUARD_CLAUSE.test_expr);
    reference_collect_expr(collect, env,
                           node->data.AST_MATCH_GUARD_CLAUSE.guard_expr);
    return true;

  case AST_YIELD:
  case AST_SPREAD_OP:
    reference_collect_expr(collect, env, node->data.AST_YIELD.expr);
    return true;

  case AST_RANGE_EXPRESSION:
    reference_collect_expr(collect, env, node->data.AST_RANGE_EXPRESSION.from);
    reference_collect_expr(collect, env, node->data.AST_RANGE_EXPRESSION.to);
    return true;

  case AST_TRAIT_IMPL:
    reference_collect_expr(collect, env, node->data.AST_TRAIT_IMPL.impl);
    return true;

  case AST_EXTERN_FN:
    reference_collect_expr(collect, env,
                           node->data.AST_EXTERN_FN.signature_types);
    return true;

  default:
    return false;
  }
}

static int symbol_kind_for_stmt(Ast *stmt) {
  if (!stmt) {
    return LSP_SYMBOL_KIND_VARIABLE;
  }

  switch (stmt->tag) {
  case AST_IMPORT:
    return LSP_SYMBOL_KIND_MODULE;
  case AST_TYPE_DECL:
    return LSP_SYMBOL_KIND_CLASS;
  case AST_TRAIT_IMPL:
    return LSP_SYMBOL_KIND_NAMESPACE;
  case AST_LET:
    if (stmt->data.AST_LET.expr && stmt->data.AST_LET.expr->tag == AST_MODULE) {
      return LSP_SYMBOL_KIND_MODULE;
    }
    if (stmt->data.AST_LET.expr && stmt->data.AST_LET.expr->tag == AST_LAMBDA) {
      return LSP_SYMBOL_KIND_FUNCTION;
    }
    return LSP_SYMBOL_KIND_VARIABLE;
  default:
    return LSP_SYMBOL_KIND_OPERATOR;
  }
}

static const char *symbol_name_for_stmt(Ast *stmt, char *buffer,
                                        size_t buffer_size) {
  ObjString name = {0};

  if (!stmt) {
    return "unknown";
  }

  switch (stmt->tag) {
  case AST_LET:
  case AST_TYPE_DECL:
    if (get_let_binding_name(stmt, &name) == 0 && name.chars) {
      return name.chars;
    }
    return stmt->tag == AST_TYPE_DECL ? "type" : "let";
  case AST_IMPORT:
    return stmt->data.AST_IMPORT.identifier ? stmt->data.AST_IMPORT.identifier
                                            : "import";
  case AST_TRAIT_IMPL:
    snprintf(buffer, buffer_size, "%s for %s",
             stmt->data.AST_TRAIT_IMPL.trait_name.chars
                 ? stmt->data.AST_TRAIT_IMPL.trait_name.chars
                 : "trait",
             stmt->data.AST_TRAIT_IMPL.type.chars
                 ? stmt->data.AST_TRAIT_IMPL.type.chars
                 : "type");
    return buffer;
  default:
    return "expression";
  }
}

static void parse_doc(document *doc) {
  if (!doc || !doc->path || !doc->text) {
    return;
  }

  doc->root = parse_input_buffer(doc->path, doc->text);
  doc->parse_error = *parse_last_error();
}

static void analyze_doc(document *doc) {
  TICtx ctx = {0};

  if (!doc) {
    return;
  }

  configure_analysis_environment();
  ht_reinit(&module_registry);
  initialize_builtin_types();
  reset_type_var_counter();
  parse_doc(doc);
  doc->analysis_dirty = false;

  if (!doc->root) {
    doc->type_env = NULL;
    doc->typecheck_ok = false;
    return;
  }

  ctx.err_stream = stderr;
  if (!infer(doc->root, &ctx)) {
    doc->typecheck_ok = false;
    return;
  }

  doc->type_env = ctx.env;
  doc->typecheck_ok = true;
}

static bool ensure_doc_analysis(document *doc) {
  if (!doc) {
    return false;
  }

  if (doc->analysis_dirty) {
    analyze_doc(doc);
  }

  return doc->root != NULL;
}

static int read_message(char **out_content) {
  char *line = NULL;
  size_t cap = 0;
  ssize_t line_len;
  int content_length = -1;

  *out_content = NULL;

  while ((line_len = getline(&line, &cap, stdin)) != -1) {
    if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0) {
      break;
    }

    if (strncmp(line, "Content-Length:", 15) == 0) {
      content_length = atoi(line + 15);
    }
  }

  free(line);

  if (line_len == -1 || content_length < 0) {
    return 0;
  }

  *out_content = malloc((size_t)content_length + 1);
  if (!*out_content) {
    return -1;
  }

  if (fread(*out_content, 1, (size_t)content_length, stdin) !=
      (size_t)content_length) {
    free(*out_content);
    *out_content = NULL;
    return -1;
  }

  (*out_content)[content_length] = '\0';
  return 1;
}

static void write_json_message(struct json_object *message) {
  const char *json =
      json_object_to_json_string_ext(message, JSON_C_TO_STRING_PLAIN);
  fprintf(stdout, "Content-Length: %zu\r\n\r\n%s", strlen(json), json);
  fflush(stdout);
}

static void send_response_int(int id, struct json_object *result) {
  struct json_object *response = json_object_new_object();

  json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
  json_object_object_add(response, "id", json_object_new_int(id));
  json_object_object_add(response, "result",
                         result ? result : json_object_new_null());

  write_json_message(response);
  json_object_put(response);
}

static void send_error_int(int id, int code, const char *message) {
  struct json_object *response = json_object_new_object();
  struct json_object *error = json_object_new_object();

  json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
  json_object_object_add(response, "id", json_object_new_int(id));
  json_object_object_add(error, "code", json_object_new_int(code));
  json_object_object_add(error, "message", json_object_new_string(message));
  json_object_object_add(response, "error", error);

  write_json_message(response);
  json_object_put(response);
}

static void send_notification(const char *method, struct json_object *params) {
  struct json_object *message = json_object_new_object();

  json_object_object_add(message, "jsonrpc", json_object_new_string("2.0"));
  json_object_object_add(message, "method", json_object_new_string(method));
  json_object_object_add(message, "params",
                         params ? params : json_object_new_object());

  write_json_message(message);
  json_object_put(message);
}

static struct json_object *syntax_diagnostic_to_json(document *doc) {
  struct json_object *diagnostic;
  source_range range;
  int line;
  int col;
  int token_len;

  if (!doc || !doc->parse_error.has_error) {
    return NULL;
  }

  line = doc->parse_error.line > 0 ? doc->parse_error.line : 1;
  col = doc->parse_error.col > 0 ? doc->parse_error.col : 1;
  token_len = (int)strlen(doc->parse_error.near_text);
  if (token_len <= 0) {
    token_len = 1;
  }

  range = (source_range){
      .start_line = line,
      .start_col = col,
      .end_line = line,
      .end_col = col + token_len,
  };

  diagnostic = json_object_new_object();
  json_object_object_add(diagnostic, "range", range_to_json(&range));
  json_object_object_add(diagnostic, "severity", json_object_new_int(1));
  json_object_object_add(diagnostic, "source", json_object_new_string("ylc"));

  if (doc->parse_error.near_text[0] != '\0') {
    char message[256];
    snprintf(message, sizeof(message), "%s near '%s'",
             doc->parse_error.message, doc->parse_error.near_text);
    json_object_object_add(diagnostic, "message",
                           json_object_new_string(message));
  } else {
    json_object_object_add(diagnostic, "message",
                           json_object_new_string(doc->parse_error.message));
  }

  return diagnostic;
}

static void publish_diagnostics(document *doc) {
  struct json_object *params = json_object_new_object();
  struct json_object *diagnostics = json_object_new_array();
  struct json_object *syntax_diagnostic = syntax_diagnostic_to_json(doc);

  if (syntax_diagnostic) {
    json_object_array_add(diagnostics, syntax_diagnostic);
  }

  json_object_object_add(params, "uri", json_object_new_string(doc->uri));
  json_object_object_add(params, "diagnostics", diagnostics);
  send_notification("textDocument/publishDiagnostics", params);
}

static const char *json_get_string(struct json_object *obj, const char *key) {
  struct json_object *field = NULL;

  if (!obj || !json_object_object_get_ex(obj, key, &field) ||
      !json_object_is_type(field, json_type_string)) {
    return NULL;
  }

  return json_object_get_string(field);
}

static int json_get_int_default(struct json_object *obj, const char *key,
                                int default_value) {
  struct json_object *field = NULL;

  if (!obj || !json_object_object_get_ex(obj, key, &field)) {
    return default_value;
  }

  return json_object_get_int(field);
}

static bool json_get_bool_default(struct json_object *obj, const char *key,
                                  bool default_value) {
  struct json_object *field = NULL;

  if (!obj || !json_object_object_get_ex(obj, key, &field)) {
    return default_value;
  }

  return json_object_get_boolean(field);
}

static void handle_initialize(lsp_server *server, int id) {
  struct json_object *result = json_object_new_object();
  struct json_object *capabilities = json_object_new_object();
  struct json_object *completion_provider = json_object_new_object();
  struct json_object *sync = json_object_new_object();
  struct json_object *trigger_characters = json_object_new_array();

  (void)server;

  json_object_object_add(sync, "openClose", json_object_new_boolean(1));
  json_object_object_add(sync, "change", json_object_new_int(LSP_SYNC_FULL));

  json_object_object_add(capabilities, "textDocumentSync", sync);
  json_object_object_add(capabilities, "documentSymbolProvider",
                         json_object_new_boolean(1));
  json_object_object_add(capabilities, "selectionRangeProvider",
                         json_object_new_boolean(1));
  json_object_object_add(capabilities, "hoverProvider",
                         json_object_new_boolean(1));
  json_object_object_add(capabilities, "definitionProvider",
                         json_object_new_boolean(1));
  json_object_object_add(capabilities, "referencesProvider",
                         json_object_new_boolean(1));
  json_object_object_add(capabilities, "renameProvider",
                         json_object_new_boolean(1));
  for (size_t i = 0; i < sizeof(completion_trigger_chars) /
                             sizeof(completion_trigger_chars[0]);
       i++) {
    json_object_array_add(trigger_characters,
                          json_object_new_string(completion_trigger_chars[i]));
  }
  json_object_object_add(completion_provider, "resolveProvider",
                         json_object_new_boolean(0));
  json_object_object_add(completion_provider, "triggerCharacters",
                         trigger_characters);
  json_object_object_add(capabilities, "completionProvider",
                         completion_provider);
  json_object_object_add(result, "capabilities", capabilities);

  send_response_int(id, result);
  server->initialized = true;
}

static void handle_did_open(lsp_server *server, struct json_object *params) {
  struct json_object *text_document = NULL;
  const char *uri;
  const char *text;
  document *doc;

  if (!json_object_object_get_ex(params, "textDocument", &text_document)) {
    return;
  }

  uri = json_get_string(text_document, "uri");
  text = json_get_string(text_document, "text");
  if (!uri || !text) {
    return;
  }

  doc = upsert_doc(server, uri);
  if (!doc) {
    return;
  }

  free(doc->text);
  doc->text = xstrdup(text);
  analyze_doc(doc);
  publish_diagnostics(doc);
}

static void handle_did_change(lsp_server *server, struct json_object *params) {
  struct json_object *text_document = NULL;
  struct json_object *changes = NULL;
  struct json_object *change = NULL;
  const char *uri;
  const char *text;
  document *doc;

  if (!json_object_object_get_ex(params, "textDocument", &text_document) ||
      !json_object_object_get_ex(params, "contentChanges", &changes) ||
      json_object_array_length(changes) == 0) {
    return;
  }

  uri = json_get_string(text_document, "uri");
  change = json_object_array_get_idx(changes, 0);
  text = json_get_string(change, "text");
  if (!uri || !text) {
    return;
  }

  doc = upsert_doc(server, uri);
  if (!doc) {
    return;
  }

  free(doc->text);
  doc->text = xstrdup(text);
  analyze_doc(doc);
  publish_diagnostics(doc);
}

static void handle_did_close(lsp_server *server, struct json_object *params) {
  struct json_object *text_document = NULL;
  const char *uri;

  if (!json_object_object_get_ex(params, "textDocument", &text_document)) {
    return;
  }

  uri = json_get_string(text_document, "uri");
  if (!uri) {
    return;
  }

  remove_doc(server, uri);
}

static void handle_document_symbol(lsp_server *server, int id,
                                   struct json_object *params) {
  struct json_object *text_document = NULL;
  struct json_object *symbols = json_object_new_array();
  const char *uri;
  document *doc;

  if (!json_object_object_get_ex(params, "textDocument", &text_document)) {
    send_error_int(id, -32602, "missing textDocument");
    json_object_put(symbols);
    return;
  }

  uri = json_get_string(text_document, "uri");
  doc = uri ? find_doc(server, uri) : NULL;
  if (!doc || !ensure_doc_analysis(doc) || !doc->root ||
      doc->root->tag != AST_BODY) {
    send_response_int(id, symbols);
    return;
  }

  for (AstList *stmt = doc->root->data.AST_BODY.stmts; stmt;
       stmt = stmt->next) {
    Ast *next_stmt = stmt->next ? stmt->next->ast : NULL;
    source_range range;
    char name_buffer[256];
    const char *name;
    struct json_object *symbol;
    struct json_object *json_range;

    if (!stmt_range_for_doc(doc, stmt->ast, next_stmt, &range)) {
      continue;
    }

    name = symbol_name_for_stmt(stmt->ast, name_buffer, sizeof(name_buffer));
    json_range = range_to_json(&range);
    symbol = json_object_new_object();

    json_object_object_add(symbol, "name", json_object_new_string(name));
    json_object_object_add(
        symbol, "kind", json_object_new_int(symbol_kind_for_stmt(stmt->ast)));
    json_object_object_add(symbol, "range", json_range);
    json_object_object_add(symbol, "selectionRange", range_to_json(&range));
    json_object_array_add(symbols, symbol);
  }

  send_response_int(id, symbols);
}

static void handle_selection_range(lsp_server *server, int id,
                                   struct json_object *params) {
  struct json_object *text_document = NULL;
  struct json_object *positions = NULL;
  struct json_object *ranges = json_object_new_array();
  const char *uri;
  document *doc;
  int num_positions;

  if (!json_object_object_get_ex(params, "textDocument", &text_document) ||
      !json_object_object_get_ex(params, "positions", &positions)) {
    send_error_int(id, -32602, "missing selectionRange params");
    json_object_put(ranges);
    return;
  }

  uri = json_get_string(text_document, "uri");
  doc = uri ? find_doc(server, uri) : NULL;
  if (!doc || !doc->text || !ensure_doc_analysis(doc) || !doc->root) {
    send_response_int(id, ranges);
    return;
  }

  num_positions = (int)json_object_array_length(positions);
  for (int i = 0; i < num_positions; i++) {
    struct json_object *position = json_object_array_get_idx(positions, i);
    source_range range;
    struct json_object *selection = json_object_new_object();
    int line = json_get_int_default(position, "line", 0) + 1;
    int character = json_get_int_default(position, "character", 0);
    long long cursor_offset;
    long long target_end_offset = 0;
    Ast *next_stmt = NULL;
    Ast *stmt = find_stmt_at_line(doc, line, &next_stmt);

    if (stmt && stmt_range_for_doc(doc, stmt, next_stmt, &range)) {
      Ast *target;

      cursor_offset = offset_for_position(doc->text, line - 1, character);
      target = find_selection_target_in_sequence(
          stmt, cursor_offset, range.end_offset, &target_end_offset);
      if (target && target->loc_info &&
          should_clamp_selection_end_with_scan(target)) {
        long long scanned_end = top_level_stmt_end_offset(
            doc->text, target->loc_info->absolute_offset);
        if (scanned_end > target->loc_info->absolute_offset &&
            scanned_end < target_end_offset) {
          target_end_offset = scanned_end;
        }
      }
      if (target && range_for_node_offsets(doc->text, target, target_end_offset,
                                           &range)) {
        json_object_object_add(selection, "range", range_to_json(&range));
      } else {
        json_object_object_add(selection, "range", range_to_json(&range));
      }
    } else {
      source_range empty_range = {
          .start_offset = 0,
          .end_offset = 0,
          .start_line = line,
          .start_col = 1,
          .end_line = line,
          .end_col = 1,
      };
      json_object_object_add(selection, "range", range_to_json(&empty_range));
    }

    json_object_array_add(ranges, selection);
  }

  send_response_int(id, ranges);
}

static void handle_hover(lsp_server *server, int id,
                         struct json_object *params) {
  struct json_object *text_document = NULL;
  struct json_object *position = NULL;
  struct json_object *result = json_object_new_object();
  struct json_object *contents = json_object_new_object();
  const char *uri;
  document *doc;
  int line;
  int character;
  long long cursor_offset;
  Ast *stmt;
  Ast *hover_node;
  Ast *next_stmt = NULL;
  source_range range;
  char *type_str;
  char *value;
  const char *name = NULL;
  ObjString binding_name = {0};

  if (!json_object_object_get_ex(params, "textDocument", &text_document) ||
      !json_object_object_get_ex(params, "position", &position)) {
    send_error_int(id, -32602, "missing hover params");
    json_object_put(result);
    return;
  }

  uri = json_get_string(text_document, "uri");
  doc = uri ? find_doc(server, uri) : NULL;
  if (!doc || !ensure_doc_analysis(doc) || !doc->root || !doc->typecheck_ok) {
    json_object_put(result);
    send_response_int(id, json_object_new_null());
    return;
  }

  line = json_get_int_default(position, "line", 0) + 1;
  character = json_get_int_default(position, "character", 0);
  stmt = find_stmt_at_line(doc, line, &next_stmt);
  if (!stmt || !stmt->type ||
      !stmt_range_for_doc(doc, stmt, next_stmt, &range)) {
    json_object_put(result);
    send_response_int(id, json_object_new_null());
    return;
  }

  cursor_offset = offset_for_position(doc->text, line - 1, character);
  hover_node =
      find_smallest_node_in_subtree(stmt, cursor_offset, range.end_offset).node;
  if (!hover_node || !hover_node->type) {
    hover_node = stmt;
  }

  type_str = type_to_string_dynamic(hover_node->type);
  if (!type_str) {
    json_object_put(result);
    send_response_int(id, json_object_new_null());
    return;
  }

  if ((hover_node->tag == AST_LET || hover_node->tag == AST_TYPE_DECL) &&
      get_let_binding_name(hover_node, &binding_name) == 0) {
    name = binding_name.chars;
  } else if (hover_node->tag == AST_IDENTIFIER) {
    name = hover_node->data.AST_IDENTIFIER.value;
  } else if (hover_node->tag == AST_IMPORT) {
    name = hover_node->data.AST_IMPORT.identifier;
  }

  if (name) {
    size_t len = strlen(name) + strlen(type_str) + 16;
    value = malloc(len);
    snprintf(value, len, "`%s : %s`", name, type_str);
  } else {
    size_t len = strlen(type_str) + 8;
    value = malloc(len);
    snprintf(value, len, "`%s`", type_str);
  }

  json_object_object_add(contents, "kind", json_object_new_string("markdown"));
  json_object_object_add(contents, "value", json_object_new_string(value));
  json_object_object_add(result, "contents", contents);
  if (hover_node->loc_info) {
    source_range hover_range = {
        .start_offset = hover_node->loc_info->absolute_offset,
        .end_offset = hover_node->loc_info->absolute_offset,
        .start_line = hover_node->loc_info->line,
        .start_col = hover_node->loc_info->col,
        .end_line = hover_node->loc_info->line,
        .end_col = hover_node->loc_info->col_end > 0
                       ? hover_node->loc_info->col_end + 1
                       : hover_node->loc_info->col + 1,
    };
    line_col_for_offset(doc->text, hover_range.start_offset,
                        &hover_range.start_line, &hover_range.start_col);
    json_object_object_add(result, "range", range_to_json(&hover_range));
  } else {
    json_object_object_add(result, "range", range_to_json(&range));
  }
  send_response_int(id, result);

  free(value);
  free(type_str);
}

static Ast *resolve_definition_at_position(lsp_server *server, document *doc,
                                           int line, int character,
                                           char **target_name_out) {
  long long cursor_offset;
  Ast *stmt;
  Ast *next_stmt = NULL;
  source_range stmt_range;
  node_match target_match = {0};
  char *target_name = NULL;
  definition_search search = {0};
  Ast *definition_node = NULL;

  if (!doc || !doc->text || !ensure_doc_analysis(doc) || !doc->root) {
    return NULL;
  }

  cursor_offset = offset_for_position(doc->text, line, character);
  stmt = find_stmt_at_line(doc, line + 1, &next_stmt);
  if (stmt && stmt_range_for_doc(doc, stmt, next_stmt, &stmt_range)) {
    target_match = find_smallest_node_in_subtree(
        stmt, cursor_offset, stmt_range.end_offset);
  }

  if (target_match.node && target_match.node->tag == AST_IDENTIFIER) {
    const char *name = identifier_name(target_match.node);
    target_name = name ? xstrdup(name) : NULL;
  }

  if (!target_name) {
    target_name = identifier_text_at_offset(doc->text, cursor_offset);
  }

  if (!target_name) {
    return NULL;
  }

  search = (definition_search){
      .server = server,
      .doc = doc,
      .target_node = target_match.node,
      .target_name = target_name,
      .cursor_offset = cursor_offset,
  };

  resolve_body_definition(&search, NULL, doc->root);
  definition_node = search.definition_node;
  definition_search_free_bindings(&search);

  if (target_name_out) {
    *target_name_out = target_name;
  } else {
    free(target_name);
  }

  return definition_node;
}

static void handle_definition(lsp_server *server, int id,
                              struct json_object *params) {
  struct json_object *text_document = NULL;
  struct json_object *position = NULL;
  const char *uri;
  document *doc;
  int line;
  int character;
  char *target_name = NULL;
  Ast *definition_node;
  struct json_object *locations;

  if (!json_object_object_get_ex(params, "textDocument", &text_document) ||
      !json_object_object_get_ex(params, "position", &position)) {
    send_error_int(id, -32602, "missing definition params");
    return;
  }

  uri = json_get_string(text_document, "uri");
  doc = uri ? find_doc(server, uri) : NULL;
  line = json_get_int_default(position, "line", 0);
  character = json_get_int_default(position, "character", 0);
  definition_node =
      resolve_definition_at_position(server, doc, line, character, &target_name);

  locations = json_object_new_array();
  if (definition_node) {
    definition_locations_add_unique(server, doc, locations, definition_node);
  }

  if (json_object_array_length(locations) > 0) {
    send_response_int(id, locations);
  } else {
    json_object_put(locations);
    send_response_int(id, json_object_new_null());
  }

  free(target_name);
}

static void handle_references(lsp_server *server, int id,
                              struct json_object *params) {
  struct json_object *text_document = NULL;
  struct json_object *position = NULL;
  struct json_object *context = NULL;
  const char *uri;
  document *doc;
  int line;
  int character;
  bool include_declaration;
  char *target_name = NULL;
  Ast *definition_node;
  struct json_object *locations;
  reference_collect collect = {0};

  if (!json_object_object_get_ex(params, "textDocument", &text_document) ||
      !json_object_object_get_ex(params, "position", &position)) {
    send_error_int(id, -32602, "missing references params");
    return;
  }

  json_object_object_get_ex(params, "context", &context);
  include_declaration =
      json_get_bool_default(context, "includeDeclaration", true);

  uri = json_get_string(text_document, "uri");
  doc = uri ? find_doc(server, uri) : NULL;
  line = json_get_int_default(position, "line", 0);
  character = json_get_int_default(position, "character", 0);
  definition_node =
      resolve_definition_at_position(server, doc, line, character, &target_name);

  locations = json_object_new_array();
  if (definition_node && doc && doc->root) {
    if (include_declaration) {
      definition_locations_add_unique(server, doc, locations, definition_node);
    }

    collect = (reference_collect){
        .server = server,
        .doc = doc,
        .target_definition_node = definition_node,
        .include_declaration = include_declaration,
        .locations = locations,
    };
    reference_collect_expr(&collect, NULL, doc->root);
    definition_search_free_bindings(&collect.bindings);
  }

  send_response_int(id, locations);
  free(target_name);
}

static bool is_valid_rename_name(const char *name) {
  if (!name || name[0] == '\0') {
    return false;
  }

  if (!(isalpha((unsigned char)name[0]) || name[0] == '_')) {
    return false;
  }

  for (size_t i = 1; name[i] != '\0'; i++) {
    if (!is_identifier_char(name[i])) {
      return false;
    }
  }

  return true;
}

static bool definition_node_belongs_to_doc(document *doc, Ast *node) {
  if (!doc || !node || !node->loc_info) {
    return false;
  }

  if (!node->loc_info->src_file || !doc->path) {
    return true;
  }

  return strcmp(node->loc_info->src_file, doc->path) == 0;
}

static void handle_rename(lsp_server *server, int id,
                          struct json_object *params) {
  struct json_object *text_document = NULL;
  struct json_object *position = NULL;
  const char *uri;
  const char *new_name;
  document *doc;
  int line;
  int character;
  char *target_name = NULL;
  Ast *definition_node;
  struct json_object *edits;
  rename_collect collect = {0};

  if (!json_object_object_get_ex(params, "textDocument", &text_document) ||
      !json_object_object_get_ex(params, "position", &position)) {
    send_error_int(id, -32602, "missing rename params");
    return;
  }

  new_name = json_get_string(params, "newName");
  if (!is_valid_rename_name(new_name)) {
    send_error_int(id, -32602, "invalid rename target");
    return;
  }

  uri = json_get_string(text_document, "uri");
  doc = uri ? find_doc(server, uri) : NULL;
  line = json_get_int_default(position, "line", 0);
  character = json_get_int_default(position, "character", 0);
  definition_node =
      resolve_definition_at_position(server, doc, line, character, &target_name);

  if (!definition_node || !definition_node_belongs_to_doc(doc, definition_node)) {
    free(target_name);
    send_response_int(id, json_object_new_null());
    return;
  }

  edits = json_object_new_array();
  collect = (rename_collect){
      .doc = doc,
      .target_definition_node = definition_node,
      .new_name = new_name,
      .edits = edits,
  };
  rename_collect_expr(&collect, NULL, doc->root);
  definition_search_free_bindings(&collect.bindings);

  if (json_object_array_length(edits) == 0) {
    json_object_put(edits);
    free(target_name);
    send_response_int(id, json_object_new_null());
    return;
  }

  struct json_object *workspace_edit = json_object_new_object();
  struct json_object *changes = json_object_new_object();
  json_object_object_add(changes, doc->uri, edits);
  json_object_object_add(workspace_edit, "changes", changes);
  send_response_int(id, workspace_edit);
  free(target_name);
}

static void handle_completion(lsp_server *server, int id,
                              struct json_object *params) {
  struct json_object *result = json_object_new_object();
  struct json_object *text_document = NULL;
  struct json_object *position = NULL;
  struct json_object *items = json_object_new_array();
  const char *uri;
  document *doc;
  int line;
  int character;
  char *prefix;
  ht seen;

  if (!json_object_object_get_ex(params, "textDocument", &text_document) ||
      !json_object_object_get_ex(params, "position", &position)) {
    send_error_int(id, -32602, "missing completion params");
    json_object_put(result);
    json_object_put(items);
    return;
  }

  uri = json_get_string(text_document, "uri");
  doc = uri ? find_doc(server, uri) : NULL;
  if (!doc || !doc->text) {
    json_object_object_add(result, "isIncomplete", json_object_new_boolean(0));
    json_object_object_add(result, "items", items);
    send_response_int(id, result);
    return;
  }

  line = json_get_int_default(position, "line", 0);
  character = json_get_int_default(position, "character", 0);
  prefix = completion_prefix_at_position(doc->text, line, character);
  if (!prefix) {
    json_object_object_add(result, "isIncomplete", json_object_new_boolean(0));
    json_object_object_add(result, "items", items);
    send_response_int(id, result);
    return;
  }

  ht_init(&seen);

  if (doc->analysis_dirty) {
    ensure_doc_analysis(doc);
  }

  if (doc->typecheck_ok && doc->type_env) {
    for (TypeEnv *env = doc->type_env; env; env = env->next) {
      add_completion_item_if_matches(items, &seen, env->name,
                                     completion_kind_for_type(env),
                                     env->type, prefix);
    }
  }

  completion_builtin_ctx builtin_ctx = {
      .items = items,
      .seen = &seen,
      .prefix = prefix,
  };
  builtin_env_foreach(add_builtin_completion, &builtin_ctx);

  for (size_t i = 0;
       i < sizeof(completion_keywords) / sizeof(completion_keywords[0]); i++) {
    add_completion_item_if_matches(items, &seen, completion_keywords[i],
                                   LSP_COMPLETION_KIND_KEYWORD, NULL, prefix);
  }

  free(prefix);
  json_object_object_add(result, "isIncomplete", json_object_new_boolean(0));
  json_object_object_add(result, "items", items);
  send_response_int(id, result);
}

static int handle_request(lsp_server *server, struct json_object *message) {
  struct json_object *method_obj = NULL;
  struct json_object *params = NULL;
  struct json_object *id_obj = NULL;
  const char *method = NULL;
  int id = -1;

  if (!json_object_object_get_ex(message, "method", &method_obj)) {
    return 0;
  }

  method = json_object_get_string(method_obj);
  json_object_object_get_ex(message, "params", &params);

  if (json_object_object_get_ex(message, "id", &id_obj)) {
    id = json_object_get_int(id_obj);
  }

  if (strcmp(method, "initialize") == 0 && id >= 0) {
    handle_initialize(server, id);
    return 0;
  }

  if (strcmp(method, "initialized") == 0) {
    return 0;
  }

  if (strcmp(method, "$/cancelRequest") == 0) {
    return 0;
  }

  if (strcmp(method, "textDocument/didOpen") == 0) {
    handle_did_open(server, params);
    return 0;
  }

  if (strcmp(method, "textDocument/didChange") == 0) {
    handle_did_change(server, params);
    return 0;
  }

  if (strcmp(method, "textDocument/didClose") == 0) {
    handle_did_close(server, params);
    return 0;
  }

  if (strcmp(method, "textDocument/documentSymbol") == 0 && id >= 0) {
    handle_document_symbol(server, id, params);
    return 0;
  }

  if (strcmp(method, "textDocument/selectionRange") == 0 && id >= 0) {
    handle_selection_range(server, id, params);
    return 0;
  }

  if (strcmp(method, "textDocument/hover") == 0 && id >= 0) {
    handle_hover(server, id, params);
    return 0;
  }

  if (strcmp(method, "textDocument/definition") == 0 && id >= 0) {
    handle_definition(server, id, params);
    return 0;
  }

  if (strcmp(method, "textDocument/references") == 0 && id >= 0) {
    handle_references(server, id, params);
    return 0;
  }

  if (strcmp(method, "textDocument/rename") == 0 && id >= 0) {
    handle_rename(server, id, params);
    return 0;
  }

  if (strcmp(method, "textDocument/completion") == 0 && id >= 0) {
    handle_completion(server, id, params);
    return 0;
  }

  if (strcmp(method, "shutdown") == 0 && id >= 0) {
    server->shutdown_requested = true;
    send_response_int(id, json_object_new_null());
    return SERVER_CONTINUE;
  }

  if (strcmp(method, "exit") == 0) {
    return server->shutdown_requested ? SERVER_EXIT_OK : SERVER_EXIT_ERROR;
  }

  if (id >= 0) {
    send_error_int(id, -32601, "method not found");
  }

  return 0;
}

int main(void) {
  lsp_server server = {0};

  init_module_registry();

  while (1) {
    char *content = NULL;
    int read_status = read_message(&content);
    struct json_object *message;
    int result;

    if (read_status == 0) {
      break;
    }

    if (read_status < 0) {
      continue;
    }

    message = json_tokener_parse(content);
    free(content);

    if (!message) {
      continue;
    }

    result = handle_request(&server, message);
    json_object_put(message);

    if (result == SERVER_EXIT_OK) {
      break;
    }

    if (result == SERVER_EXIT_ERROR) {
      return 1;
    }
  }

  return 0;
}
