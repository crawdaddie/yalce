#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LSPMessage *parse_message(const char *json_str) {
  struct json_object *root = json_tokener_parse(json_str);
  if (!root) {
    return NULL;
  }

  LSPMessage *msg = (LSPMessage *)malloc(sizeof(LSPMessage));
  if (!msg) {
    json_object_put(root);
    return NULL;
  }

  // Parse message fields
  struct json_object *jsonrpc;
  if (!json_object_object_get_ex(root, "jsonrpc", &jsonrpc) ||
      strcmp(json_object_get_string(jsonrpc), "2.0") != 0) {
    free(msg);
    json_object_put(root);
    return NULL;
  }

  struct json_object *method;
  if (json_object_object_get_ex(root, "method", &method)) {
    msg->method = strdup(json_object_get_string(method));
  } else {
    msg->method = NULL;
  }

  struct json_object *id;
  if (json_object_object_get_ex(root, "id", &id)) {
    msg->id = json_object_get_int(id);
    msg->type = LSP_REQUEST;
  } else {
    msg->id = -1;
    msg->type = LSP_NOTIFICATION;
  }

  struct json_object *params;
  if (json_object_object_get_ex(root, "params", &params)) {
    msg->params = json_object_get(params);
  } else {
    msg->params = NULL;
  }

  json_object_put(root);
  return msg;
}

void free_message(LSPMessage *msg) {
  if (msg) {
    free(msg->method);
    if (msg->params) {
      json_object_put(msg->params);
    }
    free(msg);
  }
}

char *create_response(int id, struct json_object *result) {
  struct json_object *response = json_object_new_object();

  json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
  json_object_object_add(response, "id", json_object_new_int(id));

  if (result) {
    json_object_object_add(response, "result", result);
  } else {
    json_object_object_add(response, "result", json_object_new_object());
  }

  const char *json_str = json_object_to_json_string(response);
  char *response_str = strdup(json_str);

  json_object_put(response);
  return response_str;
}

char *create_error_response(int id, int code, const char *message) {
  struct json_object *response = json_object_new_object();
  struct json_object *error = json_object_new_object();

  json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
  json_object_object_add(response, "id", json_object_new_int(id));

  json_object_object_add(error, "code", json_object_new_int(code));
  json_object_object_add(error, "message", json_object_new_string(message));

  json_object_object_add(response, "error", error);

  const char *json_str = json_object_to_json_string(response);
  char *response_str = strdup(json_str);

  json_object_put(response);
  return response_str;
}
