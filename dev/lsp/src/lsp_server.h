#ifndef LSP_SERVER_H
#define LSP_SERVER_H

#include <json-c/json.h>

typedef struct DocumentInfo {
  char *uri;
  char *content;
  int version;
  void *cached_ast;  // Cache parsed AST to avoid re-parsing
  void *cached_type_env;  // Cache type environment
  struct DocumentInfo *next;
} DocumentInfo;

typedef struct {
  int initialized;
  char *root_uri;
  DocumentInfo *documents;
  
  // Language integration 
  char *ylc_build_dir;
} LSPServer;

// Server lifecycle functions
LSPServer *lsp_server_create();
void lsp_server_destroy(LSPServer *server);
int lsp_server_run(LSPServer *server);

// Document management functions
DocumentInfo *find_document(LSPServer *server, const char *uri);
void add_or_update_document(LSPServer *server, const char *uri, const char *content, int version);
void remove_document(LSPServer *server, const char *uri);

// Language analysis functions
struct json_object *analyze_document(LSPServer *server, const char *uri);
struct json_object *get_hover_info(LSPServer *server, const char *uri, int line, int character);
struct json_object *get_completions(LSPServer *server, const char *uri, int line, int character);

#endif // LSP_SERVER_H
