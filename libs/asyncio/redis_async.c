#include "asyncio.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Redis RESP protocol helpers

// Format a RESP array command
// Example: ["GET", "mykey"] -> "*2\r\n$3\r\nGET\r\n$5\r\nmykey\r\n"
char *format_redis_command(char **args, int num_args) {
  // Calculate total size needed
  int total_size = 0;
  total_size += snprintf(NULL, 0, "*%d\r\n", num_args);

  for (int i = 0; i < num_args; i++) {
    int arg_len = strlen(args[i]);
    total_size += snprintf(NULL, 0, "$%d\r\n", arg_len);
    total_size += arg_len + 2; // arg + \r\n
  }

  char *buffer = malloc(total_size + 1);
  if (!buffer)
    return NULL;

  int offset = 0;
  offset += sprintf(buffer + offset, "*%d\r\n", num_args);

  for (int i = 0; i < num_args; i++) {
    int arg_len = strlen(args[i]);
    offset += sprintf(buffer + offset, "$%d\r\n", arg_len);
    memcpy(buffer + offset, args[i], arg_len);
    offset += arg_len;
    buffer[offset++] = '\r';
    buffer[offset++] = '\n';
  }

  buffer[offset] = '\0';
  return buffer;
}

// Parse RESP response (simplified - handles simple strings, bulk strings,
// integers)
typedef struct {
  enum {
    RESP_STRING,
    RESP_BULK_STRING,
    RESP_INTEGER,
    RESP_ERROR,
    RESP_ARRAY,
    RESP_NIL
  } type;
  union {
    char *str;
    int64_t num;
    struct {
      char **elements;
      int count;
    } array;
  } value;
} RESPValue;

RESPValue *parse_redis_response(char *resp) {
  RESPValue *val = malloc(sizeof(RESPValue));
  if (!val)
    return NULL;

  if (resp[0] == '+') {
    // Simple string
    val->type = RESP_STRING;
    char *end = strstr(resp + 1, "\r\n");
    if (end) {
      int len = end - (resp + 1);
      val->value.str = strndup(resp + 1, len);
    }
  } else if (resp[0] == '-') {
    // Error
    val->type = RESP_ERROR;
    char *end = strstr(resp + 1, "\r\n");
    if (end) {
      int len = end - (resp + 1);
      val->value.str = strndup(resp + 1, len);
    }
  } else if (resp[0] == ':') {
    // Integer
    val->type = RESP_INTEGER;
    val->value.num = atoll(resp + 1);
  } else if (resp[0] == '$') {
    // Bulk string
    int len = atoi(resp + 1);
    if (len == -1) {
      val->type = RESP_NIL;
    } else {
      val->type = RESP_BULK_STRING;
      char *start = strstr(resp + 1, "\r\n") + 2;
      val->value.str = strndup(start, len);
    }
  } else if (resp[0] == '*') {
    // Array (simplified - not fully implemented)
    val->type = RESP_ARRAY;
    int count = atoi(resp + 1);
    val->value.array.count = count;
    val->value.array.elements = malloc(sizeof(char *) * count);
    // TODO: parse array elements
  }

  return val;
}

void free_resp_value(RESPValue *val) {
  if (!val)
    return;

  if (val->type == RESP_STRING || val->type == RESP_BULK_STRING ||
      val->type == RESP_ERROR) {
    free(val->value.str);
  } else if (val->type == RESP_ARRAY) {
    for (int i = 0; i < val->value.array.count; i++) {
      free(val->value.array.elements[i]);
    }
    free(val->value.array.elements);
  }
  free(val);
}

// Redis connection
typedef struct {
  int fd;
  char *host;
  int port;
  int connected;
} RedisConnection;

// Blocking connect (for backward compatibility)
RedisConnection *redis_connect(char *host, int port) {
  RedisConnection *conn = malloc(sizeof(RedisConnection));
  if (!conn)
    return NULL;

  conn->host = strdup(host);
  conn->port = port;
  conn->connected = 0;

  conn->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (conn->fd < 0) {
    perror("socket");
    free(conn->host);
    free(conn);
    return NULL;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, host, &addr.sin_addr);

  if (connect(conn->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("connect");
    close(conn->fd);
    free(conn->host);
    free(conn);
    return NULL;
  }

  conn->connected = 1;
  printf("[Redis] Connected to %s:%d (fd=%d)\n", host, port, conn->fd);
  return conn;
}

// Async connect operation
#define ASYNC_CONNECT 3

typedef struct {
  RedisConnection *conn;
  int connect_result;
} ConnectOp;

void *redis_connect_cb(PendingOp *op) {
  ConnectOp *conn_op = (ConnectOp *)op->result_ptr;
  RedisConnection *conn = conn_op->conn;

  // Check if connection succeeded
  int error = 0;
  socklen_t len = sizeof(error);
  if (getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
    perror("getsockopt");
    conn_op->connect_result = -1;
    return NULL;
  }

  if (error != 0) {
    printf("[Redis] Connection failed: %s\n", strerror(error));
    conn_op->connect_result = -1;
    return NULL;
  }

  conn->connected = 1;
  conn_op->connect_result = 0;
  printf("[Redis] Async connected to %s:%d (fd=%d)\n", conn->host, conn->port,
         conn->fd);

  return op;
}

// Async connect - returns immediately with fd in non-blocking mode
void *redis_connect_async(EventLoop *loop, char *host, int port, void *cor,
                          RedisConnection **conn_out) {
  RedisConnection *conn = malloc(sizeof(RedisConnection));
  if (!conn)
    return NULL;

  conn->host = strdup(host);
  conn->port = port;
  conn->connected = 0;

  // Create socket in non-blocking mode
  conn->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (conn->fd < 0) {
    perror("socket");
    free(conn->host);
    free(conn);
    return NULL;
  }

  // Set non-blocking before connect
  set_nonblocking(conn->fd);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, host, &addr.sin_addr);

  // Start non-blocking connect
  int result = connect(conn->fd, (struct sockaddr *)&addr, sizeof(addr));

  // EINPROGRESS is expected for non-blocking connect
  if (result < 0 && errno != EINPROGRESS) {
    perror("connect");
    close(conn->fd);
    free(conn->host);
    free(conn);
    return NULL;
  }

  printf("[Redis] Connecting to %s:%d (fd=%d)...\n", host, port, conn->fd);

  // Store connection in output parameter
  *conn_out = conn;

  // Create connect operation
  ConnectOp *conn_op = malloc(sizeof(ConnectOp));
  conn_op->conn = conn;
  conn_op->connect_result = -2; // Pending

  PendingOp *op = malloc(sizeof(PendingOp));
  op->waiting_fd = conn->fd;
  op->result_ptr = conn_op;
  op->cor = cor;
  op->op_type = ASYNC_CONNECT;
  op->cb = (PromiseCallback)redis_connect_cb;

  event_loop_register_op(loop, op);

  return op;
}

void redis_close(RedisConnection *conn) {
  if (conn) {
    if (conn->connected) {
      close(conn->fd);
    }
    free(conn->host);
    free(conn);
  }
}

// Async Redis query structures
typedef struct {
  RedisConnection *conn;
  char *command;
  char *response_buffer;
  RESPValue *parsed_response;
  void (*callback)(RESPValue *result, void *user_data);
  void *user_data;
} RedisQuery;

void *redis_write_cb(PendingOp *op) {
  RedisQuery *query = (RedisQuery *)op->result_ptr;

  ssize_t n = send(op->waiting_fd, query->command, strlen(query->command), 0);

  if (n < 0) {
    perror("redis send");
    return NULL;
  }

  printf("[Redis] Sent %zd bytes: %s\n", n, query->command);
  return op;
}

void *redis_read_cb(PendingOp *op) {
  RedisQuery *query = (RedisQuery *)op->result_ptr;

  ssize_t n = recv(op->waiting_fd, query->response_buffer, 4096, 0);

  if (n < 0) {
    perror("redis recv");
    return NULL;
  }

  query->response_buffer[n] = '\0';
  printf("[Redis] Received %zd bytes: %s\n", n, query->response_buffer);

  // Parse the response
  query->parsed_response = parse_redis_response(query->response_buffer);

  // Call user callback if provided
  if (query->callback) {
    query->callback(query->parsed_response, query->user_data);
  }

  return op;
}

// High-level async Redis command
void *redis_query_async(EventLoop *loop, RedisConnection *conn, char **args,
                        int num_args, void *cor,
                        void (*callback)(RESPValue *result, void *user_data),
                        void *user_data) {

  RedisQuery *query = malloc(sizeof(RedisQuery));
  query->conn = conn;
  query->command = format_redis_command(args, num_args);
  query->response_buffer = malloc(4096);
  query->parsed_response = NULL;
  query->callback = callback;
  query->user_data = user_data;

  // First, register write operation to send the command
  PendingOp *write_op = malloc(sizeof(PendingOp));
  write_op->waiting_fd = conn->fd;
  write_op->result_ptr = query;
  write_op->cor = cor;
  write_op->op_type = ASYNC_WRITE;
  write_op->cb = (PromiseCallback)redis_write_cb;

  event_loop_register_op(loop, write_op);

  // Then, register read operation to receive the response
  PendingOp *read_op = malloc(sizeof(PendingOp));
  read_op->waiting_fd = conn->fd;
  read_op->result_ptr = query;
  read_op->cor = cor;
  read_op->op_type = ASYNC_READ;
  read_op->cb = (PromiseCallback)redis_read_cb;

  event_loop_register_op(loop, read_op);

  return read_op;
}

// Convenience functions for common Redis commands
void *redis_get_async(EventLoop *loop, RedisConnection *conn, char *key,
                      void *cor,
                      void (*callback)(RESPValue *result, void *user_data),
                      void *user_data) {
  char *args[] = {"GET", key};
  return redis_query_async(loop, conn, args, 2, cor, callback, user_data);
}

void *redis_set_async(EventLoop *loop, RedisConnection *conn, char *key,
                      char *value, void *cor,
                      void (*callback)(RESPValue *result, void *user_data),
                      void *user_data) {
  char *args[] = {"SET", key, value};
  return redis_query_async(loop, conn, args, 3, cor, callback, user_data);
}

// Example callback
void redis_get_callback(RESPValue *result, void *user_data) {
  if (!result) {
    printf("[Callback] No result\n");
    return;
  }

  if (result->type == RESP_BULK_STRING) {
    printf("[Callback] GET result: %s\n", result->value.str);
  } else if (result->type == RESP_NIL) {
    printf("[Callback] Key not found\n");
  } else if (result->type == RESP_ERROR) {
    printf("[Callback] Error: %s\n", result->value.str);
  }

  free_resp_value(result);
}
