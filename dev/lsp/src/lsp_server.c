#include "lsp_server.h"
#include "protocol.h"
#include "ylc_integration.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LSPServer *lsp_server_create() {
  LSPServer *server = (LSPServer *)malloc(sizeof(LSPServer));
  if (server) {
    server->initialized = 0;
    server->root_uri = NULL;
    server->documents = NULL;
    server->ylc_build_dir = strdup("./build");
  }
  return server;
}

void lsp_server_destroy(LSPServer *server) {
  if (server) {
    free(server->root_uri);
    free(server->ylc_build_dir);

    // Free document list
    DocumentInfo *doc = server->documents;
    while (doc) {
      DocumentInfo *next = doc->next;
      free(doc->uri);
      free(doc->content);
      free(doc);
      doc = next;
    }

    free(server);
  }
}

static char *read_message() {
  char header[256];
  if (!fgets(header, sizeof(header), stdin)) {
    return NULL;
  }

  int content_length = 0;
  if (sscanf(header, "Content-Length: %d\r\n", &content_length) != 1) {
    return NULL;
  }

  if (!fgets(header, sizeof(header), stdin)) {
    return NULL;
  }

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

  json_object_object_add(start, "line", json_object_new_int(0));
  json_object_object_add(start, "character", json_object_new_int(0));

  json_object_object_add(end, "line", json_object_new_int(INT_MAX));
  json_object_object_add(end, "character", json_object_new_int(INT_MAX));

  json_object_object_add(range, "start", start);
  json_object_object_add(range, "end", end);

  return range;
}
static char *format_text(const char *input) { return strdup(input); }
int lsp_server_run(LSPServer *server) {
  fprintf(stderr, "DEBUG: LSP server starting main loop\n");
  fflush(stderr);

  while (1) {
    fprintf(stderr, "DEBUG: Waiting for message\n");
    fflush(stderr);

    char *content = read_message();
    if (!content) {
      fprintf(stderr, "DEBUG: No content received\n");
      fflush(stderr);
      continue;
    }

    fprintf(stderr, "DEBUG: Received content: %.100s...\n", content);
    fflush(stderr);

    LSPMessage *msg = parse_message(content);
    free(content);

    if (!msg) {
      fprintf(stderr, "DEBUG: Failed to parse message\n");
      fflush(stderr);
      continue;
    }

    fprintf(stderr, "DEBUG: Parsed method: %s\n", msg->method);
    fflush(stderr);

    if (strcmp(msg->method, "initialize") == 0) {
      struct json_object *result = json_object_new_object();
      struct json_object *capabilities = json_object_new_object();

      // LSP capabilities
      json_object_object_add(capabilities, "textDocumentSync",
                             json_object_new_int(1)); // Full sync
      json_object_object_add(capabilities, "hoverProvider",
                             json_object_new_boolean(1));
      json_object_object_add(capabilities, "completionProvider",
                             json_object_new_object());
      json_object_object_add(capabilities, "documentFormattingProvider",
                             json_object_new_boolean(1));

      json_object_object_add(result, "capabilities", capabilities);

      char *response = create_response(msg->id, result);
      write_message(response);
      free(response);

      server->initialized = 1;
    }

    // Handle document lifecycle events
    if (strcmp(msg->method, "textDocument/didOpen") == 0) {
      struct json_object *text_document;
      json_object_object_get_ex(msg->params, "textDocument", &text_document);

      struct json_object *uri_obj, *text_obj, *version_obj;
      json_object_object_get_ex(text_document, "uri", &uri_obj);
      json_object_object_get_ex(text_document, "text", &text_obj);
      json_object_object_get_ex(text_document, "version", &version_obj);

      const char *uri = json_object_get_string(uri_obj);
      const char *text = json_object_get_string(text_obj);
      int version = json_object_get_int(version_obj);

      fprintf(stderr, "DEBUG: Adding document: %s\n", uri);
      fflush(stderr);

      add_or_update_document(server, uri, text, version);

      fprintf(stderr, "DEBUG: Analyzing document for diagnostics\n");
      fflush(stderr);

      // Send diagnostics
      struct json_object *diagnostics = analyze_document(server, uri);

      fprintf(stderr, "DEBUG: Got diagnostics, sending notification\n");
      fflush(stderr);
      struct json_object *notification = json_object_new_object();
      json_object_object_add(notification, "jsonrpc",
                             json_object_new_string("2.0"));
      json_object_object_add(
          notification, "method",
          json_object_new_string("textDocument/publishDiagnostics"));

      struct json_object *params = json_object_new_object();
      json_object_object_add(params, "uri", json_object_new_string(uri));
      json_object_object_add(params, "diagnostics", diagnostics);
      json_object_object_add(notification, "params", params);

      const char *notification_str = json_object_to_json_string(notification);
      write_message(notification_str);
      json_object_put(notification);
    }

    if (strcmp(msg->method, "textDocument/didChange") == 0) {
      struct json_object *text_document;
      json_object_object_get_ex(msg->params, "textDocument", &text_document);

      struct json_object *uri_obj, *version_obj;
      json_object_object_get_ex(text_document, "uri", &uri_obj);
      json_object_object_get_ex(text_document, "version", &version_obj);

      const char *uri = json_object_get_string(uri_obj);
      int version = json_object_get_int(version_obj);

      // Get the full text from content changes
      struct json_object *content_changes;
      json_object_object_get_ex(msg->params, "contentChanges",
                                &content_changes);

      if (json_object_array_length(content_changes) > 0) {
        struct json_object *change =
            json_object_array_get_idx(content_changes, 0);
        struct json_object *text_obj;
        json_object_object_get_ex(change, "text", &text_obj);
        const char *text = json_object_get_string(text_obj);

        add_or_update_document(server, uri, text, version);

        // Send updated diagnostics
        struct json_object *diagnostics = analyze_document(server, uri);
        struct json_object *notification = json_object_new_object();
        json_object_object_add(notification, "jsonrpc",
                               json_object_new_string("2.0"));
        json_object_object_add(
            notification, "method",
            json_object_new_string("textDocument/publishDiagnostics"));

        struct json_object *params = json_object_new_object();
        json_object_object_add(params, "uri", json_object_new_string(uri));
        json_object_object_add(params, "diagnostics", diagnostics);
        json_object_object_add(notification, "params", params);

        const char *notification_str = json_object_to_json_string(notification);
        write_message(notification_str);
        json_object_put(notification);
      }
    }

    if (strcmp(msg->method, "textDocument/didClose") == 0) {
      struct json_object *text_document;
      json_object_object_get_ex(msg->params, "textDocument", &text_document);

      struct json_object *uri_obj;
      json_object_object_get_ex(text_document, "uri", &uri_obj);
      const char *uri = json_object_get_string(uri_obj);

      remove_document(server, uri);
    }

    // Handle hover requests
    if (strcmp(msg->method, "textDocument/hover") == 0) {
      struct json_object *text_document, *position;
      json_object_object_get_ex(msg->params, "textDocument", &text_document);
      json_object_object_get_ex(msg->params, "position", &position);

      struct json_object *uri_obj, *line_obj, *char_obj;
      json_object_object_get_ex(text_document, "uri", &uri_obj);
      json_object_object_get_ex(position, "line", &line_obj);
      json_object_object_get_ex(position, "character", &char_obj);

      const char *uri = json_object_get_string(uri_obj);
      int line = json_object_get_int(line_obj);
      int character = json_object_get_int(char_obj);

      struct json_object *hover_info =
          get_hover_info(server, uri, line, character);
      char *response = create_response(msg->id, hover_info);
      write_message(response);
      free(response);
    }

    // Handle completion requests
    if (strcmp(msg->method, "textDocument/completion") == 0) {
      struct json_object *text_document, *position;
      json_object_object_get_ex(msg->params, "textDocument", &text_document);
      json_object_object_get_ex(msg->params, "position", &position);

      struct json_object *uri_obj, *line_obj, *char_obj;
      json_object_object_get_ex(text_document, "uri", &uri_obj);
      json_object_object_get_ex(position, "line", &line_obj);
      json_object_object_get_ex(position, "character", &char_obj);

      const char *uri = json_object_get_string(uri_obj);
      int line = json_object_get_int(line_obj);
      int character = json_object_get_int(char_obj);

      struct json_object *completions =
          get_completions(server, uri, line, character);
      char *response = create_response(msg->id, completions);
      write_message(response);
      free(response);
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

// Document management functions
DocumentInfo *find_document(LSPServer *server, const char *uri) {
  DocumentInfo *doc = server->documents;
  while (doc) {
    if (strcmp(doc->uri, uri) == 0) {
      return doc;
    }
    doc = doc->next;
  }
  return NULL;
}

void add_or_update_document(LSPServer *server, const char *uri,
                            const char *content, int version) {
  DocumentInfo *doc = find_document(server, uri);
  if (doc) {
    // Update existing document - clear cache since content changed
    free(doc->content);
    doc->content = strdup(content);
    doc->version = version;
    doc->cached_ast = NULL;  // Invalidate cache
    doc->cached_type_env = NULL;
  } else {
    // Add new document
    doc = malloc(sizeof(DocumentInfo));
    doc->uri = strdup(uri);
    doc->content = strdup(content);
    doc->version = version;
    doc->cached_ast = NULL;
    doc->cached_type_env = NULL;
    doc->next = server->documents;
    server->documents = doc;
  }
}

void remove_document(LSPServer *server, const char *uri) {
  DocumentInfo **doc_ptr = &server->documents;
  while (*doc_ptr) {
    DocumentInfo *doc = *doc_ptr;
    if (strcmp(doc->uri, uri) == 0) {
      *doc_ptr = doc->next;
      free(doc->uri);
      free(doc->content);
      free(doc);
      return;
    }
    doc_ptr = &doc->next;
  }
}

// Language analysis functions
struct json_object *analyze_document(LSPServer *server, const char *uri) {
  DocumentInfo *doc = find_document(server, uri);
  if (!doc) {
    return json_object_new_array();
  }

  char *filename = uri_to_filename(uri);
  struct json_object *diagnostics = get_ylc_diagnostics(doc->content, filename);
  free(filename);

  return diagnostics;
}

struct json_object *get_hover_info(LSPServer *server, const char *uri, int line,
                                   int character) {
  DocumentInfo *doc = find_document(server, uri);
  if (!doc) {
    return NULL;
  }

  char *filename = uri_to_filename(uri);
  struct json_object *hover =
      get_ylc_hover_at_position(doc->content, filename, line, character);
  free(filename);

  return hover;
}

struct json_object *get_completions(LSPServer *server, const char *uri,
                                    int line, int character) {
  DocumentInfo *doc = find_document(server, uri);
  if (!doc) {
    return json_object_new_array();
  }

  char *filename = uri_to_filename(uri);
  struct json_object *completions =
      get_ylc_completions_at_position(doc->content, filename, line, character);
  free(filename);

  return completions;
}
