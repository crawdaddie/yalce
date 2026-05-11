#include "../lang/parse.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct document {
  char *path;
  char *text;
  Ast *root;
  struct document *next;
} document;

static document *g_docs = NULL;

static char *xstrdup(const char *s) {
  size_t len = strlen(s);
  char *copy = malloc(len + 1);
  memcpy(copy, s, len + 1);
  return copy;
}

static document *find_doc(const char *path) {
  for (document *doc = g_docs; doc != NULL; doc = doc->next) {
    if (strcmp(doc->path, path) == 0) {
      return doc;
    }
  }
  return NULL;
}

static document *upsert_doc(const char *path) {
  document *doc = find_doc(path);
  if (doc) {
    return doc;
  }

  doc = calloc(1, sizeof(document));
  doc->path = xstrdup(path);
  doc->next = g_docs;
  g_docs = doc;
  return doc;
}

static const char *find_key(const char *json, const char *key) {
  static char pattern[128];
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  return strstr(json, pattern);
}

static int extract_int_field(const char *json, const char *key, int *out) {
  const char *p = find_key(json, key);
  if (!p) {
    return 0;
  }
  p = strchr(p, ':');
  if (!p) {
    return 0;
  }
  p++;
  while (*p && isspace((unsigned char)*p)) {
    p++;
  }
  *out = atoi(p);
  return 1;
}

static char *decode_json_string(const char *start, size_t len) {
  char *out = malloc(len + 1);
  size_t j = 0;

  for (size_t i = 0; i < len; i++) {
    char ch = start[i];
    if (ch == '\\' && i + 1 < len) {
      i++;
      switch (start[i]) {
      case 'n':
        out[j++] = '\n';
        break;
      case 'r':
        out[j++] = '\r';
        break;
      case 't':
        out[j++] = '\t';
        break;
      case '\\':
        out[j++] = '\\';
        break;
      case '"':
        out[j++] = '"';
        break;
      default:
        out[j++] = start[i];
        break;
      }
    } else {
      out[j++] = ch;
    }
  }

  out[j] = '\0';
  return out;
}

static char *extract_string_field(const char *json, const char *key) {
  const char *p = find_key(json, key);
  if (!p) {
    return NULL;
  }
  p = strchr(p, ':');
  if (!p) {
    return NULL;
  }
  p++;
  while (*p && isspace((unsigned char)*p)) {
    p++;
  }
  if (*p != '"') {
    return NULL;
  }
  p++;
  const char *start = p;
  int escaped = 0;

  while (*p) {
    if (!escaped && *p == '"') {
      break;
    }
    if (!escaped && *p == '\\') {
      escaped = 1;
    } else {
      escaped = 0;
    }
    p++;
  }

  if (*p != '"') {
    return NULL;
  }

  return decode_json_string(start, (size_t)(p - start));
}

static void print_error(int id, const char *error) {
  printf("{\"id\":%d,\"ok\":false,\"error\":\"%s\"}\n", id, error);
  fflush(stdout);
}

static void handle_open_like(const char *json, int id) {
  char *path = extract_string_field(json, "path");
  char *text = extract_string_field(json, "text");
  if (!path || !text) {
    print_error(id, "missing path or text");
    free(path);
    free(text);
    return;
  }

  document *doc = upsert_doc(path);
  free(doc->text);
  doc->text = text;
  doc->root = parse_input_buffer(path, doc->text);

  printf("{\"id\":%d,\"ok\":true}\n", id);
  fflush(stdout);
  free(path);
}

static void handle_top_level_at(const char *json, int id) {
  char *path = extract_string_field(json, "path");
  int line = 0;
  if (!path || !extract_int_field(json, "line", &line)) {
    print_error(id, "missing path or line");
    free(path);
    return;
  }

  document *doc = find_doc(path);
  if (!doc || !doc->text || !doc->root) {
    print_error(id, "document not loaded");
    free(path);
    return;
  }

  source_range range;
  if (!find_top_level_range_at_line(doc->root, doc->text, line, &range)) {
    print_error(id, "no top-level node at line");
    free(path);
    return;
  }

  printf("{\"id\":%d,\"ok\":true,\"start_offset\":%lld,\"end_offset\":%lld,"
         "\"start_line\":%d,\"start_col\":%d,\"end_line\":%d,\"end_col\":%d}\n",
         id, range.start_offset, range.end_offset, range.start_line,
         range.start_col, range.end_line, range.end_col);
  fflush(stdout);
  free(path);
}

int main(void) {
  char *line = NULL;
  size_t cap = 0;

  while (getline(&line, &cap, stdin) != -1) {
    int id = 0;
    char *method = extract_string_field(line, "method");
    extract_int_field(line, "id", &id);

    if (!method) {
      print_error(id, "missing method");
      continue;
    }

    if (strcmp(method, "open") == 0 || strcmp(method, "update") == 0) {
      handle_open_like(line, id);
    } else if (strcmp(method, "top_level_at") == 0) {
      handle_top_level_at(line, id);
    } else {
      print_error(id, "unknown method");
    }

    free(method);
  }

  free(line);
  return 0;
}
