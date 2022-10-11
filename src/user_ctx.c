#include "user_ctx.h"
Graph *null_graph() {
  Graph *graph = malloc(sizeof(Graph));
  graph->perform = perform_null;
  graph->next = NULL;
  graph->prev = NULL;
  graph->name = "null";
  return graph;
}

UserCtx *get_user_ctx(jack_port_t *input_port, jack_port_t **output_ports,
                      queue_t *msg_queue) {
  UserCtx *ctx = malloc(sizeof(UserCtx));
  ctx->input_port = input_port;
  ctx->output_ports = output_ports;
  ctx->graph = null_graph();
  /* ctx->graph = NULL; */
  ctx->msg_queue = msg_queue;
  return ctx;
}

void handle_msg(void *msg, Graph *graph) {
  queue_msg_t *m = (queue_msg_t *)msg;
  printf("msg: %s time %d\n", m->msg, m->time);
  m->func(graph, m->time, m->ref);
}

Graph *process_queue(queue_t *queue, Graph *graph) {
  char *item = dequeue(queue);
  while (item) {
    handle_msg(item, graph);
    free(item);
    item = dequeue(queue);
  }
  return graph;
}
