#include "ylc_integration.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Include YLC headers
#include "common.h"
#include "input.h"
#include "parse.h"
#include "types/inference.h"
#include "types/type.h"
#include "y.tab.h"

void *yy_scan_string(const char *yystr);

// Forward declaration of our initialization function from ylc_stubs.c
void ylc_lsp_init();
void ylc_lsp_init_with_dir(const char *base_dir);

char *uri_to_filename(const char *uri) {
  if (strncmp(uri, "file://", 7) == 0) {
    return strdup(uri + 7);
  }
  return strdup(uri);
}

struct json_object *create_position(int line, int character) {
  struct json_object *pos = json_object_new_object();
  json_object_object_add(pos, "line", json_object_new_int(line));
  json_object_object_add(pos, "character", json_object_new_int(character));
  return pos;
}

struct json_object *create_range(int start_line, int start_char, int end_line,
                                 int end_char) {
  struct json_object *range = json_object_new_object();
  json_object_object_add(range, "start",
                         create_position(start_line, start_char));
  json_object_object_add(range, "end", create_position(end_line, end_char));
  return range;
}

struct json_object *create_diagnostic(int line, int column, int severity,
                                      const char *message, const char *code) {
  struct json_object *diagnostic = json_object_new_object();

  // LSP diagnostic severity: 1=Error, 2=Warning, 3=Information, 4=Hint
  json_object_object_add(diagnostic, "severity", json_object_new_int(severity));
  json_object_object_add(diagnostic, "message",
                         json_object_new_string(message));

  if (code) {
    json_object_object_add(diagnostic, "code", json_object_new_string(code));
  }

  // Create range for the diagnostic
  json_object_object_add(diagnostic, "range",
                         create_range(line, column, line, column + 1));

  return diagnostic;
}

struct json_object *parse_ylc_document(const char *content,
                                       const char *filename) {

  struct json_object *result = json_object_new_object();
  struct json_object *diagnostics = json_object_new_array();

  char *dirname = get_dirname(filename ? filename : ".");

  ylc_lsp_init_with_dir(dirname);

  yylineno = 1;
  yyabsoluteoffset = 0;

  ast_root = Ast_new(AST_BODY);
  ast_root->data.AST_BODY.len = 0;
  ast_root->data.AST_BODY.stmts = malloc(sizeof(Ast *));

  _cur_script = filename;
  _cur_script_content = content;

  yylineno = 1;
  yyabsoluteoffset = 0;
  yy_scan_string(_cur_script_content);
  yyparse();
  Ast *ast = ast_root;

  TypeEnv *env = NULL;

  initialize_builtin_types();
  TICtx ti_ctx = {.env = env, .scope = 0};

  ti_ctx.err_stream = stderr;

  if (!infer(ast, &ti_ctx)) {
    return NULL;
  }
  if (!solve_program_constraints(ast, &ti_ctx)) {
    return NULL;
  }

  json_object_object_add(result, "diagnostics", diagnostics);
  return result;
}

Ast *get_doc_ast(const char *content, const char *filename) {

  struct json_object *result = json_object_new_object();
  struct json_object *diagnostics = json_object_new_array();

  char *dirname = get_dirname(filename ? filename : ".");

  ylc_lsp_init_with_dir(dirname);

  yylineno = 1;
  yyabsoluteoffset = 0;

  ast_root = Ast_new(AST_BODY);
  ast_root->data.AST_BODY.len = 0;
  ast_root->data.AST_BODY.stmts = malloc(sizeof(Ast *));

  _cur_script = filename;
  _cur_script_content = content;

  yylineno = 1;
  yyabsoluteoffset = 0;
  yy_scan_string(_cur_script_content);
  yyparse();
  Ast *ast = ast_root;

  TypeEnv *env = NULL;

  initialize_builtin_types();
  TICtx ti_ctx = {.env = env, .scope = 0};

  ti_ctx.err_stream = stderr;

  if (!infer(ast, &ti_ctx)) {
    return NULL;
  }
  if (!solve_program_constraints(ast, &ti_ctx)) {
    return NULL;
  }

  return ast;
}

struct json_object *get_ylc_diagnostics(const char *content,
                                        const char *filename) {
  // parse_ylc_document(content, filename);

  return json_object_new_array();
}

// Helper function to find word boundaries at cursor position
char *extract_word_at_position(const char *content, int line, int character) {
  const char *lines = content;
  int current_line = 0;

  // Navigate to the target line
  while (current_line < line && *lines) {
    if (*lines == '\n')
      current_line++;
    lines++;
  }

  if (current_line != line || character < 0)
    return NULL;

  // Navigate to the target character
  const char *pos = lines;
  int current_char = 0;
  while (current_char < character && *pos && *pos != '\n') {
    pos++;
    current_char++;
  }

  if (current_char != character)
    return NULL;

  // Find word boundaries
  const char *start = pos;
  const char *end = pos;

  // Move start backwards to find word start
  while (start > lines && (isalnum(*(start - 1)) || *(start - 1) == '_')) {
    start--;
  }

  // Move end forwards to find word end
  while (*end && (isalnum(*end) || *end == '_')) {
    end++;
  }

  if (start == end)
    return NULL;

  // Extract the word
  size_t len = end - start;
  char *word = malloc(len + 1);
  strncpy(word, start, len);
  word[len] = '\0';

  return word;
}

struct json_object *get_ylc_hover_at_position(const char *content,
                                              const char *filename, int line,
                                              int character) {
  struct json_object *hover = json_object_new_object();
  return hover;
}

struct json_object *get_ylc_completions_at_position(const char *content,
                                                    const char *filename,
                                                    int line, int character) {
  // TODO: Implement actual completions using YLC symbol table

  struct json_object *completions = json_object_new_array();

  return completions;
}
