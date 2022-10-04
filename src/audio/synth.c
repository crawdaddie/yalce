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

Node *perform_graph(Node *graph, int frame_count, double seconds_per_frame,
                    double seconds_offset) {
  Node *synth_graph = graph;

  if (synth_graph == NULL) {
    return NULL;
  };

  synth_graph->perform(synth_graph, frame_count, seconds_per_frame,
                       seconds_offset);

  if (synth_graph->next) {
    return perform_graph(synth_graph->next, frame_count, seconds_per_frame,
                         seconds_offset);
  };

  return synth_graph; // return head
}

void perform_synth_graph(Node *synth, int frame_count, double seconds_per_frame,
                         double seconds_offset) {

  synth_data *s_data = (synth_data *)synth->data;
  Node *node = s_data->graph;
  Node *tail =
      perform_graph(node, frame_count, seconds_per_frame, seconds_offset);
  for (int i = 0; i < frame_count; i++) {
    s_data->bus[i] = s_data->bus[i] + tail->out[i];
  }
}

void on_free(Node *synth) { synth->should_free = 1; }
void free_synth(Node *synth) {
  printf("free synth %s", synth->name);
  /* free(synth->mul); */
  /* free(synth->data); */
  /* free(synth->in); */
  /* free(synth->out); */
  /* free(synth); */
}

Node *get_synth(double freq, double *bus) {
  printf("play node on bus & %#08x %f\n", bus, freq);

  Node *head = get_sq_detune_node(freq);
  Node *tail = head;
  tail = node_add_to_tail(get_tanh_node(tail->out, 20.0), tail);
  tail = node_add_to_tail(get_biquad_lpf(tail->out, 1000.0, 0.5, 2.0, 48000),
                          tail);
  Node *env = get_env_node(100, 25.0, 1000.0, 0.0);

  tail = node_mul(env, tail);

  synth_data *data = malloc(sizeof(synth_data) + sizeof(bus));
  data->graph = head;
  data->bus = bus;

  Node *out_node =
      alloc_node((NodeData *)data, NULL, (t_perform)perform_synth_graph,
                 "synth", (t_free_node)free_synth);
  /* out_node->out = tail->out; */
  env_set_on_free(env, out_node, on_free);
  return out_node;
}
