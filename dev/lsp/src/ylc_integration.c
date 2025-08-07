#include "ylc_integration.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Include YLC headers
#include "common.h"
#include "parse.h"
#include "types/inference.h"
#include "types/type.h"

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
  fprintf(stderr, "DEBUG: parse_ylc_document called with filename: %s\n",
          filename ? filename : "NULL");
  fflush(stderr);

  struct json_object *result = json_object_new_object();
  struct json_object *diagnostics = json_object_new_array();

  fprintf(stderr, "DEBUG: Making content copy\n");
  fflush(stderr);

  // Make a mutable copy of content since parse_input may modify it
  char *content_copy = strdup(content);
  if (!content_copy) {
    fprintf(stderr, "DEBUG: Failed to copy content\n");
    fflush(stderr);
    json_object_object_add(result, "diagnostics", diagnostics);
    return result;
  }

  fprintf(stderr, "DEBUG: Processing dirname\n");
  fflush(stderr);

  // Get directory from filename for module resolution
  char *dirname = strdup(filename ? filename : ".");
  char *last_slash = strrchr(dirname, '/');
  if (last_slash) {
    *last_slash = '\0';
  } else {
    strcpy(dirname, ".");
  }

  fprintf(stderr, "DEBUG: About to call parse_input with dirname: %s\n",
          dirname);
  fflush(stderr);

  // Initialize YLC globals before parsing with the correct base directory
  ylc_lsp_init_with_dir(dirname);

  fprintf(stderr,
          "DEBUG: YLC initialized with base_dir=%s, calling parse_input\n",
          dirname);
  fflush(stderr);

  // Parse using YLC's parser
  Ast *ast = parse_input(content_copy, dirname);

  fprintf(stderr, "DEBUG: parse_input returned: %p\n", (void *)ast);
  fflush(stderr);

  if (ast == NULL) {
    // Parse error occurred
    struct json_object *error =
        create_diagnostic(0, 0, 1, "Parse error in YLC code", "parse_error");
    json_object_array_add(diagnostics, error);
  } else {
    // Successfully parsed - could add more sophisticated analysis here
    // For now, just return empty diagnostics for successful parse
  }

  free(content_copy);
  free(dirname);
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

  // Extract word at cursor position
  char *word = extract_word_at_position(content, line, character);
  if (!word) {
    // No word found at position
    struct json_object *contents = json_object_new_object();
    json_object_object_add(contents, "kind",
                           json_object_new_string("markdown"));
    json_object_object_add(contents, "value",
                           json_object_new_string("No symbol found"));
    json_object_object_add(hover, "contents", contents);
    return hover;
  }

  // Parse and analyze the document
  char *dirname = strdup(filename ? filename : ".");
  char *last_slash = strrchr(dirname, '/');
  if (last_slash) {
    *last_slash = '\0';
  } else {
    strcpy(dirname, ".");
  }

  // PERFORMANCE FIX: Skip expensive parsing/type inference for now
  // Provide fast, basic hover information instead
  char hover_text[512];
  
  // Simple heuristic-based hints for common YLC patterns
  if (strcmp(word, "let") == 0) {
    snprintf(hover_text, sizeof(hover_text), "**let**\n\nVariable binding keyword");
  } else if (strcmp(word, "fn") == 0) {
    snprintf(hover_text, sizeof(hover_text), "**fn**\n\nFunction definition keyword");  
  } else if (strcmp(word, "match") == 0) {
    snprintf(hover_text, sizeof(hover_text), "**match**\n\nPattern matching keyword");
  } else if (strcmp(word, "if") == 0) {
    snprintf(hover_text, sizeof(hover_text), "**if**\n\nConditional keyword");
  } else {
    snprintf(hover_text, sizeof(hover_text), "**%s**\n\nYLC identifier", word);
  }

  struct json_object *contents = json_object_new_object();
  json_object_object_add(contents, "kind", json_object_new_string("markdown"));
  json_object_object_add(contents, "value", json_object_new_string(hover_text));
  json_object_object_add(hover, "contents", contents);

  free(word);
  free(dirname);
  return hover;
}

struct json_object *get_ylc_completions_at_position(const char *content,
                                                    const char *filename,
                                                    int line, int character) {
  // TODO: Implement actual completions using YLC symbol table

  struct json_object *completions = json_object_new_array();

  // Add YLC keywords
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

  return completions;
}
