#ifndef YLC_INTEGRATION_H
#define YLC_INTEGRATION_H

#include "lsp_server.h"
#include "parse.h"
#include <json-c/json.h>

// YLC language integration functions
struct json_object *parse_ylc_document(const char *content,
                                       const char *filename);
struct json_object *parse_ylc_document(const char *content,
                                       const char *filename);
struct json_object *get_ylc_diagnostics(const char *content,
                                        const char *filename);
struct json_object *get_ylc_hover_at_position(const char *content,
                                              const char *filename, int line,
                                              int character);
struct json_object *get_ylc_completions_at_position(const char *content,
                                                    const char *filename,
                                                    int line, int character);

// Helper functions
char *uri_to_filename(const char *uri);
struct json_object *create_diagnostic(int line, int column, int severity,
                                      const char *message, const char *code);
struct json_object *create_range(int start_line, int start_char, int end_line,
                                 int end_char);
struct json_object *create_position(int line, int character);

Ast *get_doc_ast(const char *content, const char *filename);

#endif // YLC_INTEGRATION_H
