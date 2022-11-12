#include "user_ctx.h"
Graph *null_graph() {
  Graph *graph = malloc(sizeof(Graph));
  graph->perform = perform_null;
  graph->next = NULL;
  graph->prev = NULL;
  graph->name = "null";
  return graph;
}

t_sample **alloc_buses(int num_buses) {
  t_sample **buses;
  buses = calloc(num_buses, sizeof(t_sample *));
  for (int i = 0; i < num_buses; i++) {
    buses[i] = calloc(BUF_SIZE, sizeof(double));
  };
  return buses;
}

UserCtx *get_user_ctx(jack_port_t *input_port, jack_port_t **output_ports,
                      t_queue *msg_queue) {
  UserCtx *ctx = malloc(sizeof(UserCtx));
  ctx->input_port = input_port;
  ctx->output_ports = output_ports;
  ctx->graph = null_graph();
  /* ctx->graph = NULL; */
  ctx->msg_queue = msg_queue;

  ctx->buses = alloc_buses(INITIAL_BUSNUM);
  ctx->buffers = calloc(1, sizeof(struct buf_info));
  return ctx;
}

t_queue_msg *msg_init(char *msg_string, t_nframes time, void *func, int num_args) {
  t_queue_msg *msg = malloc(sizeof(t_queue_msg));
  msg->msg = msg_string;
  msg->time = time;
  msg->func = (MsgAction)func;

  msg->num_args = num_args;
  msg->args = malloc(num_args * sizeof(void *));
  return msg;
}

void handle_msg(void *msg, Graph *graph) {
  t_queue_msg *m = (t_queue_msg *)msg;
  printf("msg: %s time %d\n", m->msg, m->time);
  m->func(graph, m->time, m->args);
}
void free_msg(t_queue_msg *msg) {
  free(msg->args);
  free(msg);
}
Graph *process_queue(t_queue *queue, Graph *graph) {
  t_queue_msg *item = dequeue(queue);
  while (item) {
    handle_msg(item, graph);
    free_msg(item);
    item = dequeue(queue);
  }
  return graph;
}
