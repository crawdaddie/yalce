
#include "lsp_server.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LSPServer *lsp_server_create() {
  LSPServer *server = (LSPServer *)malloc(sizeof(LSPServer));
  if (server) {
    server->initialized = 0;
    server->root_uri = NULL;
  }
  return server;
}

void lsp_server_destroy(LSPServer *server) {
  if (server) {
    free(server->root_uri);
    free(server);
  }
}

static char *read_message() {
  // Read Content-Length header
  char header[256];
  if (!fgets(header, sizeof(header), stdin)) {
    return NULL;
  }

  int content_length = 0;
  if (sscanf(header, "Content-Length: %d\r\n", &content_length) != 1) {
    return NULL;
  }

  // Skip empty line
  if (!fgets(header, sizeof(header), stdin)) {
    return NULL;
  }

  // Read message content
  char *content = (char *)malloc(content_length + 1);
  if (!content) {
    return NULL;
  }

  if (fread(content, 1, content_length, stdin) != (size_t)content_length) {
    free(content);
    return NULL;
  }
  content[content_length] = '\0';

  return content;
}

static void write_message(const char *message) {
  fprintf(stdout, "Content-Length: %zu\r\n\r\n%s", strlen(message), message);
  fflush(stdout);
}

int lsp_server_run(LSPServer *server) {
  while (1) {
    char *content = read_message();
    if (!content) {
      continue;
    }

    LSPMessage *msg = parse_message(content);
    free(content);

    if (!msg) {
      continue;
    }

    if (strcmp(msg->method, "initialize") == 0) {
      struct json_object *result = json_object_new_object();
      struct json_object *capabilities = json_object_new_object();

      // Add capabilities to result object
      json_object_object_add(result, "capabilities", capabilities);

      char *response = create_response(msg->id, result);
      write_message(response);
      free(response);

      server->initialized = 1;
    }

    // Handle shutdown request
    if (strcmp(msg->method, "shutdown") == 0) {
      char *response = create_response(msg->id, NULL);
      write_message(response);
      free(response);
      free_message(msg);
      break;
    }

    free_message(msg);
  }

  return 0;
}
