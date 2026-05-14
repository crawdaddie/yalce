#include "../lang/modules.h"
#include "../lang/parse.h"
#include <ctype.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LSP_SYMBOL_KIND_MODULE 2
#define LSP_SYMBOL_KIND_CLASS 5
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
  struct document *next;
} document;

typedef struct {
  document *docs;
  bool initialized;
  bool shutdown_requested;
} lsp_server;

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
  for (int current_line = 1; doc->text[cursor_offset] != '\0' && current_line < line;
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

    if (cursor_offset >= range.start_offset && cursor_offset < range.end_offset) {
      if (next_stmt_out) {
        *next_stmt_out = next_stmt;
      }
      return current;
    }

    stmt = stmt->next;
  }

  return NULL;
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
  line_col_for_offset(src, end_offset, &out_range->end_line, &out_range->end_col);
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
    return stmt->data.AST_IMPORT.identifier
               ? stmt->data.AST_IMPORT.identifier
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
  const char *json = json_object_to_json_string_ext(message, JSON_C_TO_STRING_PLAIN);
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

static void publish_empty_diagnostics(document *doc) {
  struct json_object *params = json_object_new_object();

  json_object_object_add(params, "uri", json_object_new_string(doc->uri));
  json_object_object_add(params, "diagnostics", json_object_new_array());
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

static void handle_initialize(lsp_server *server, int id) {
  struct json_object *result = json_object_new_object();
  struct json_object *capabilities = json_object_new_object();
  struct json_object *sync = json_object_new_object();

  (void)server;

  json_object_object_add(sync, "openClose", json_object_new_boolean(1));
  json_object_object_add(sync, "change", json_object_new_int(LSP_SYNC_FULL));

  json_object_object_add(capabilities, "textDocumentSync", sync);
  json_object_object_add(capabilities, "documentSymbolProvider",
                         json_object_new_boolean(1));
  json_object_object_add(capabilities, "selectionRangeProvider",
                         json_object_new_boolean(1));
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
  parse_doc(doc);
  publish_empty_diagnostics(doc);
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
  parse_doc(doc);
  publish_empty_diagnostics(doc);
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
  if (!doc || !doc->root || doc->root->tag != AST_BODY) {
    send_response_int(id, symbols);
    return;
  }

  for (AstList *stmt = doc->root->data.AST_BODY.stmts; stmt; stmt = stmt->next) {
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
    json_object_object_add(symbol, "kind",
                           json_object_new_int(symbol_kind_for_stmt(stmt->ast)));
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
  if (!doc || !doc->root || !doc->text) {
    send_response_int(id, ranges);
    return;
  }

  num_positions = (int)json_object_array_length(positions);
  for (int i = 0; i < num_positions; i++) {
    struct json_object *position = json_object_array_get_idx(positions, i);
    source_range range;
    struct json_object *selection = json_object_new_object();
    int line = json_get_int_default(position, "line", 0) + 1;
    Ast *next_stmt = NULL;
    Ast *stmt = find_stmt_at_line(doc, line, &next_stmt);

    if (stmt && stmt_range_for_doc(doc, stmt, next_stmt, &range)) {
      json_object_object_add(selection, "range", range_to_json(&range));
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
