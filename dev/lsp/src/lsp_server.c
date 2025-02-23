
#include "lsp_server.h"
#include "protocol.h"
#include <limits.h>
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
static struct json_object *create_full_range() {
  struct json_object *range = json_object_new_object();
  struct json_object *start = json_object_new_object();
  struct json_object *end = json_object_new_object();

  // Start position (0,0)
  json_object_object_add(start, "line", json_object_new_int(0));
  json_object_object_add(start, "character", json_object_new_int(0));

  // End position (max,max) - you might want to calculate actual document end
  json_object_object_add(end, "line", json_object_new_int(INT_MAX));
  json_object_object_add(end, "character", json_object_new_int(INT_MAX));

  json_object_object_add(range, "start", start);
  json_object_object_add(range, "end", end);

  return range;
}
static char *format_text(const char *input) {
  // Implement your actual formatting logic here
  // For now, just return a copy of the input
  return strdup(input);
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

      // Add formatting capability
      json_object_object_add(capabilities, "documentFormattingProvider",
                             json_object_new_boolean(1));

      // Add capabilities to result object
      json_object_object_add(result, "capabilities", capabilities);

      char *response = create_response(msg->id, result);
      write_message(response);
      free(response);

      server->initialized = 1;
    }

    /*
    // Handle formatting request
    if (strcmp(msg->method, "textDocument/formatting") == 0) {
      // Extract document URI and text
      struct json_object *params;
      struct json_object *text_document;
      const char *uri;

      json_object_object_get_ex(msg->params, "textDocument", &text_document);
      json_object_object_get_ex(text_document, "uri", &uri);

      // Create array for formatted text edits
      struct json_object *edits = json_object_new_array();

      // Create a text edit
      struct json_object *edit = json_object_new_object();
      json_object_object_add(
          edit, "range", create_full_range()); // You'll need to implement this
      json_object_object_add(
          edit, "newText",
          json_object_new_string(
              "formatted text")); // Replace with actual formatting

      json_object_array_add(edits, edit);

      // Send response
      char *response = create_response(msg->id, edits);
      write_message(response);
      free(response);
    }
    */

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
