#ifndef LSP_SERVER_H
#define LSP_SERVER_H

#include <json-c/json.h>

typedef struct {
  int initialized;
  char *root_uri;
  // Add more server state as needed
} LSPServer;

// Server lifecycle functions
LSPServer *lsp_server_create();
void lsp_server_destroy(LSPServer *server);
int lsp_server_run(LSPServer *server);

#endif // LSP_SERVER_H
