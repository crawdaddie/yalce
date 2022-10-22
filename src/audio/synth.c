#include "node.h"
#include "node_biquad.c"
#include "node_bufplayer.c"
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
                    double seconds_per_frame, double seconds_offset,
                    double schedule) {

  if (!synth_graph) {
    return NULL;
  };

  synth_graph->perform(synth_graph, frame_count, seconds_per_frame,
                       seconds_offset, schedule);

  Node *next = synth_graph->next;
  int **pp = &next;
  if (pp == 0x7fffdc1d6) {
    debug_node(next, "problem");
  }

  if (next) {
    return perform_graph(next, frame_count, seconds_per_frame, seconds_offset,
                         schedule);
  };

  return synth_graph; // return tail
}

void perform_synth_graph(Node *synth, int frame_count, double seconds_per_frame,
                         double seconds_offset) {

  double schedule = synth->schedule;
  synth_data *s_data = (synth_data *)synth->data;
  Node *node = s_data->graph;
  if (!node) {
    return;
  }
  if (node->should_free) {
    return;
  }

  Node *tail = perform_graph(node, frame_count, seconds_per_frame,
                             seconds_offset, schedule);

  for (int i = 0; i < frame_count; i++) {
    sched();
    ptr(tail_out, tail->out);
    double out_val = tail_out[i];
    ptr(bus, s_data->bus);
    bus[i] += out_val;
  }
}

void on_env_free(Node *synth) { synth->should_free = 1; }
void free_synth(Node *synth) {
  free_node(((synth_data *)synth->data)->graph);
  free_node(synth);
}

void dbg_graph(Node *graph) {
  debug_node(graph, "free synth g");

  if (graph->next) {
    printf("↓\n");
    return dbg_graph(graph->next);
  };
  printf("----------\n");
}
Node *remove_from_graph(Node *node, Node *prev) {
  Node *next = node->next;
  prev->next = next;
  /* dbg_graph(((synth_data *)node->data)->graph); */
  node->free_node(node);
  return next;
}
