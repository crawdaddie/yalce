
#include "ylc_integration.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
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

  // Parse using YLC's parser
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

  // Capture parser errors
  int parse_result = yyparse();

  // if (parse_result != 0) {
  //   // Parsing failed - add parse error diagnostic
  //   struct json_object *parse_error = create_diagnostic(
  //       yylineno > 0 ? yylineno - 1 : 0, // Convert to 0-based line numbers
  //       0,                               // Column 0 for parse errors
  //       1,                               // Error severity
  //       "Parse error: invalid syntax", "parse-error");
  //   json_object_array_add(diagnostics, parse_error);
  //
  //   json_object_object_add(result, "diagnostics", diagnostics);
  //   free(dirname);
  //   return result;
  // }

  Ast *ast = ast_root;
  // if (!ast) {
  //   struct json_object *ast_error =
  //       create_diagnostic(0, 0, 1, "Failed to create AST", "ast-error");
  //   json_object_array_add(diagnostics, ast_error);
  //   json_object_object_add(result, "diagnostics", diagnostics);
  //   free(dirname);
  //   return result;
  // }

  // Type checking
  TypeEnv *env = NULL;
  initialize_builtin_types();
  TICtx ti_ctx = {.env = env, .scope = 0};

  infer(ast, &ti_ctx);
  solve_program_constraints(ast, &ti_ctx);

  json_object_object_add(result, "diagnostics", diagnostics);
  return result;
}

struct json_object *get_ylc_diagnostics(const char *content,
                                        const char *filename) {
  struct json_object *parse_result = parse_ylc_document(content, filename);
  struct json_object *diagnostics;

  if (parse_result &&
      json_object_object_get_ex(parse_result, "diagnostics", &diagnostics)) {
    json_object_get(diagnostics); // Increment reference count
    json_object_put(parse_result);
    return diagnostics;
  }

  if (parse_result) {
    json_object_put(parse_result);
  }

  // Return empty diagnostics array if parsing failed
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

  // // Extract word at cursor position
  // char *word = extract_word_at_position(content, line, character);
  // if (!word) {
  //   // No word found at position
  //   struct json_object *contents = json_object_new_object();
  //   json_object_object_add(contents, "kind",
  //                          json_object_new_string("markdown"));
  //   json_object_object_add(contents, "value",
  //                          json_object_new_string("No symbol found"));
  //   json_object_object_add(hover, "contents", contents);
  //   return hover;
  // }
  //
  // char *dirname = get_dirname(filename ? filename : ".");
  // ylc_lsp_init_with_dir(dirname);
  //
  // char hover_text[1024];
  //
  // // Parse document and get type environment
  // yylineno = 1;
  // yyabsoluteoffset = 0;
  //
  // ast_root = Ast_new(AST_BODY);
  // ast_root->data.AST_BODY.len = 0;
  // ast_root->data.AST_BODY.stmts = malloc(sizeof(Ast *));
  //
  // _cur_script = filename;
  // _cur_script_content = content;
  //
  // yylineno = 1;
  // yyabsoluteoffset = 0;
  // yy_scan_string(_cur_script_content);
  // int parse_success = yyparse();
  //
  // if (parse_success == 0) {
  //   Ast *ast = ast_root;
  //   TypeEnv *env = NULL;
  //
  //   initialize_builtin_types();
  //   TICtx ti_ctx = {.env = env, .scope = 0};
  //   ti_ctx.err_stream = stderr;
  //
  //   if (infer(ast, &ti_ctx) && solve_program_constraints(ast, &ti_ctx)) {
  //     // Look up the symbol in the type environment
  //     Type *symbol_type = env_lookup(ti_ctx.env, word);
  //
  //     if (symbol_type) {
  //       char type_buf[256];
  //       type_to_string(symbol_type, type_buf);
  //       snprintf(hover_text, sizeof(hover_text), "**%s** : `%s`\n\nType: %s",
  //                word, type_buf, type_buf);
  //     } else {
  //       // Check if it's a keyword
  //       if (strcmp(word, "let") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**let**\n\nVariable binding keyword");
  //       } else if (strcmp(word, "fn") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**fn**\n\nFunction definition keyword");
  //       } else if (strcmp(word, "match") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**match**\n\nPattern matching keyword");
  //       } else if (strcmp(word, "if") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**if**\n\nConditional keyword");
  //       } else if (strcmp(word, "else") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**else**\n\nConditional keyword");
  //       } else if (strcmp(word, "import") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**import**\n\nModule import keyword");
  //       } else if (strcmp(word, "extern") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**extern**\n\nExternal function declaration keyword");
  //       } else if (strcmp(word, "type") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**type**\n\nType definition keyword");
  //       } else if (strcmp(word, "yield") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**yield**\n\nCoroutine yield keyword");
  //       } else if (strcmp(word, "return") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**return**\n\nFunction return keyword");
  //       } else if (strcmp(word, "true") == 0 || strcmp(word, "false") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**%s** : `bool`\n\nBoolean literal", word);
  //       } else if (strcmp(word, "nil") == 0) {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**nil**\n\nNull/void value");
  //       } else {
  //         snprintf(hover_text, sizeof(hover_text),
  //                  "**%s**\n\nUnresolved identifier", word);
  //       }
  //     }
  //   } else {
  //     // Parsing/type inference failed, fallback to simple hints
  //     snprintf(hover_text, sizeof(hover_text),
  //              "**%s**\n\nSymbol (type inference failed)", word);
  //   }
  // } else {
  //   // Parsing failed, fallback to simple hints
  //   snprintf(hover_text, sizeof(hover_text),
  //            "**%s**\n\nSymbol (parsing failed)", word);
  // }
  //
  // struct json_object *contents = json_object_new_object();
  // json_object_object_add(contents, "kind",
  // json_object_new_string("markdown")); json_object_object_add(contents,
  // "value", json_object_new_string(hover_text)); json_object_object_add(hover,
  // "contents", contents);
  //
  // free(word);
  // free(dirname);
  return hover;
}

// Helper function to traverse type environment and collect symbols
void collect_symbols_from_env(TypeEnv *env, struct json_object *completions) {
  if (!env)
    return;

  // // Traverse the environment to collect all identifiers
  // while (env) {
  //   if (env->name) {
  //     struct json_object *item = json_object_new_object();
  //     json_object_object_add(item, "label",
  //     json_object_new_string(env->name));
  //
  //     if (env->type) {
  //       char type_buf[256];
  //       type_to_string(env->type, type_buf);
  //       json_object_object_add(item, "detail",
  //                              json_object_new_string(type_buf));
  //
  //       // Determine completion item kind based on type
  //       if (strstr(type_buf, "->")) {
  //         json_object_object_add(item, "kind",
  //                                json_object_new_int(3)); // Function
  //       } else {
  //         json_object_object_add(item, "kind",
  //                                json_object_new_int(6)); // Variable
  //       }
  //     } else {
  //       json_object_object_add(item, "kind",
  //                              json_object_new_int(6)); // Variable
  //       json_object_object_add(item, "detail",
  //                              json_object_new_string("variable"));
  //     }
  //
  //     json_object_array_add(completions, item);
  //   }
  //   env = env->next;
  // }
}

struct json_object *get_ylc_completions_at_position(const char *content,
                                                    const char *filename,
                                                    int line, int character) {
  struct json_object *completions = json_object_new_array();

  char *dirname = get_dirname(filename ? filename : ".");
  ylc_lsp_init_with_dir(dirname);

  // Try to parse and infer types to get context-aware completions
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
  int parse_success = yyparse();

  if (parse_success == 0) {
    Ast *ast = ast_root;
    TypeEnv *env = NULL;

    initialize_builtin_types();
    TICtx ti_ctx = {.env = env, .scope = 0};
    ti_ctx.err_stream = stderr;

    if (infer(ast, &ti_ctx) && solve_program_constraints(ast, &ti_ctx)) {
      // Add symbols from the type environment
      collect_symbols_from_env(ti_ctx.env, completions);
    }
  }

  // Always add YLC keywords
  const char *keywords[] = {
      "let",    "fn",    "match", "if",    "else",   "import", "extern", "type",
      "struct", "trait", "impl",  "yield", "return", "true",   "false",  "nil"};
  int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

  for (int i = 0; i < num_keywords; i++) {
    struct json_object *item = json_object_new_object();
    json_object_object_add(item, "label", json_object_new_string(keywords[i]));
    json_object_object_add(item, "kind", json_object_new_int(14)); // Keyword
    json_object_object_add(item, "detail",
                           json_object_new_string("YLC keyword"));
    json_object_array_add(completions, item);
  }

  // Add built-in types
  const char *types[] = {"int", "double", "string", "bool", "array", "list"};
  int num_types = sizeof(types) / sizeof(types[0]);

  for (int i = 0; i < num_types; i++) {
    struct json_object *item = json_object_new_object();
    json_object_object_add(item, "label", json_object_new_string(types[i]));
    json_object_object_add(item, "kind",
                           json_object_new_int(25)); // TypeParameter
    json_object_object_add(item, "detail",
                           json_object_new_string("Built-in type"));
    json_object_array_add(completions, item);
  }

  // Add built-in functions
  const char *builtins[] = {"print", "println", "len",    "head",
                            "tail",  "map",     "filter", "fold"};
  int num_builtins = sizeof(builtins) / sizeof(builtins[0]);

  for (int i = 0; i < num_builtins; i++) {
    struct json_object *item = json_object_new_object();
    json_object_object_add(item, "label", json_object_new_string(builtins[i]));
    json_object_object_add(item, "kind", json_object_new_int(3)); // Function
    json_object_object_add(item, "detail",
                           json_object_new_string("Built-in function"));
    json_object_array_add(completions, item);
  }

  free(dirname);
  return completions;
}

struct json_object *get_ylc_definition_at_position(const char *content,
                                                   const char *filename,
                                                   int line, int character) {
  // Extract word at cursor position
  char *word = extract_word_at_position(content, line, character);
  if (!word) {
    return NULL; // No definition found
  }

  char *dirname = get_dirname(filename ? filename : ".");
  ylc_lsp_init_with_dir(dirname);

  // For now, implement a simple approach that searches for definitions in the
  // same file
  // TODO: Extend to cross-file definitions and imported modules

  const char *lines = content;
  int current_line = 0;
  int def_line = -1;
  int def_character = -1;

  // Search for function definitions: "fn <word>"
  char fn_pattern[256];
  snprintf(fn_pattern, sizeof(fn_pattern), "fn %s", word);

  char *fn_pos = strstr(content, fn_pattern);
  if (fn_pos) {
    // Calculate line and character position
    const char *pos = content;
    current_line = 0;
    while (pos < fn_pos) {
      if (*pos == '\n')
        current_line++;
      pos++;
    }
    def_line = current_line;

    // Find character position within the line
    const char *line_start = fn_pos;
    while (line_start > content && *(line_start - 1) != '\n') {
      line_start--;
    }
    def_character = (fn_pos + 3) - line_start; // Position after "fn "
  } else {
    // Search for variable definitions: "let <word>"
    char let_pattern[256];
    snprintf(let_pattern, sizeof(let_pattern), "let %s", word);

    char *let_pos = strstr(content, let_pattern);
    if (let_pos) {
      // Calculate line and character position
      const char *pos = content;
      current_line = 0;
      while (pos < let_pos) {
        if (*pos == '\n')
          current_line++;
        pos++;
      }
      def_line = current_line;

      // Find character position within the line
      const char *line_start = let_pos;
      while (line_start > content && *(line_start - 1) != '\n') {
        line_start--;
      }
      def_character = (let_pos + 4) - line_start; // Position after "let "
    }
  }

  if (def_line >= 0) {
    // Create location object
    struct json_object *location = json_object_new_object();

    // Convert filename back to URI
    char uri[512];
    if (strncmp(filename, "file://", 7) == 0) {
      strncpy(uri, filename, sizeof(uri) - 1);
    } else {
      snprintf(uri, sizeof(uri), "file://%s", filename);
    }

    json_object_object_add(location, "uri", json_object_new_string(uri));
    json_object_object_add(location, "range",
                           create_range(def_line, def_character, def_line,
                                        def_character + strlen(word)));

    free(word);
    free(dirname);
    return location;
  }

  free(word);
  free(dirname);
  return NULL; // No definition found
}
