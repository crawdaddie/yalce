#include "node.h"
#include "node_biquad.c"
#include "node_delay.c"
#include "node_dist.c"
#include "node_env.c"
#include "node_out.c"
#include "node_sin.c"
#include "node_square.c"

typedef struct synth_data {
  Node *graph;
  double *bus;
} synth_data;

Node *perform_graph(Node *synth_graph, int frame_count,
                    double seconds_per_frame, double seconds_offset) {

  if (synth_graph == NULL) {
    return NULL;
  };

  synth_graph->perform(synth_graph, frame_count, seconds_per_frame,
                       seconds_offset);

  if (synth_graph->next) {
    return perform_graph(synth_graph->next, frame_count, seconds_per_frame,
                         seconds_offset);
  };

  return synth_graph; // return tail
}

void perform_synth_graph(Node *synth, int frame_count, double seconds_per_frame,
                         double seconds_offset) {

  synth_data *s_data = (synth_data *)synth->data;
  Node *node = s_data->graph;
  Node *tail =
      perform_graph(node, frame_count, seconds_per_frame, seconds_offset);

  for (int i = 0; i < frame_count; i++) {
    s_data->bus[i] += tail->out[i];
  }
}

void on_env_free(Node *synth) { synth->should_free = 1; }
void free_synth(Node *synth) {
  printf("free synth %s", synth->name);
  free_node(synth);
}

Node *remove_from_graph(Node *node, Node *prev) {
  Node *next = node->next;
  prev->next = next;
  node->free_node(node);
  return next;
}
