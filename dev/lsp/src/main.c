#include "lsp_server.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  LSPServer *server = lsp_server_create();
  if (!server) {
    fprintf(stderr, "Failed to create LSP server\n");
    return 1;
  }

  int result = lsp_server_run(server);
  lsp_server_destroy(server);
  return result;
}
