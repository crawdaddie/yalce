#ifndef LSP_PROTOCOL_H
#define LSP_PROTOCOL_H

#include <json-c/json.h>

typedef enum { LSP_REQUEST, LSP_RESPONSE, LSP_NOTIFICATION } MessageType;

typedef struct {
  MessageType type;
  char *method;
  int id;
  struct json_object *params;
} LSPMessage;

// Protocol handling functions
LSPMessage *parse_message(const char *json_str);
void free_message(LSPMessage *msg);
char *create_response(int id, struct json_object *result);
char *create_error_response(int id, int code, const char *message);

#endif // PROTOCOL_H
