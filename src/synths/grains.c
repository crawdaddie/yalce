#include "../audio/graph.h"
#include "../config.h"
#include "../user_ctx.h"
#include <stdlib.h>
typedef struct grains_data {
} grains_data;
grains_data *alloc_grains_data() {
  grains_data *data = malloc(sizeof(grains_data));
  return data;
}

t_perform perform_grains(Graph *graph, nframes_t nframes) {
  for (int i = 0; i < nframes; i++) {
  }
}

Graph *init_grains_node() {
  grains_data *data = alloc_grains_data();
  Graph *node = alloc_graph((NodeData *)data, NULL, (t_perform)perform_grains, 1); 
  return node;
}
void add_grains_node_msg_handler(Graph *graph, int time, void **args) {
  Graph *node = init_grains_node();
  node->schedule = time;
  add_after(graph, node);
}
void add_grains_node_msg(UserCtx *ctx, nframes_t frame_time) {
  queue_msg_t *msg = msg_init("grains", frame_time, add_grains_node_msg_handler, 1);
  enqueue(ctx->msg_queue, msg);
}

