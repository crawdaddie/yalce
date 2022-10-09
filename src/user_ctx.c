#include "user_ctx.h"
UserCtx *get_user_ctx(jack_port_t *input_port, jack_port_t **output_ports, queue_t *msg_queue) {
  UserCtx *ctx = malloc(sizeof(UserCtx));
  ctx->input_port = input_port;
  ctx->output_ports = output_ports;
  ctx->graph = NULL;
  ctx->msg_queue = msg_queue;
  return ctx;
}


void handle_msg(void *msg, Graph *graph) {
  printf("msg: %s\n", (char *)msg);
}

Graph *process_queue(queue_t *queue, Graph *graph) {
  if (!queue) {
    return graph;
  }
  char *item = dequeue(queue);
  handle_msg(item, graph);
  return process_queue(queue, graph);
}

