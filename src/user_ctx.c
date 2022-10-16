#include "user_ctx.h"
Graph *null_graph() {
  Graph *graph = malloc(sizeof(Graph));
  graph->perform = perform_null;
  graph->next = NULL;
  graph->prev = NULL;
  graph->name = "null";
  return graph;
}

sample_t **alloc_buses(int num_buses) {
  sample_t **buses;
  buses = calloc(num_buses, sizeof(sample_t *));
  for (int i = 0; i < num_buses; i++) {
    buses[i] = calloc(BUF_SIZE, sizeof(double));
  };
  return buses;
}

UserCtx *get_user_ctx(jack_port_t *input_port, jack_port_t **output_ports,
                      queue_t *msg_queue) {
  UserCtx *ctx = malloc(sizeof(UserCtx));
  ctx->input_port = input_port;
  ctx->output_ports = output_ports;
  ctx->graph = null_graph();
  /* ctx->graph = NULL; */
  ctx->msg_queue = msg_queue;

  ctx->buses = alloc_buses(INITIAL_BUSNUM);
  return ctx;
}

void handle_msg(void *msg, Graph *graph) {
  queue_msg_t *m = (queue_msg_t *)msg;
  printf("msg: %s time %d\n", m->msg, m->time);
  m->func(graph, m->time, m->args);
}
void free_msg(queue_msg_t *msg) {
  free(msg->args);
  free(msg);
}
Graph *process_queue(queue_t *queue, Graph *graph) {
  queue_msg_t *item = dequeue(queue);
  while (item) {
    handle_msg(item, graph);
    free_msg(item);
    item = dequeue(queue);
  }
  return graph;
}
