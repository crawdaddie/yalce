#include "node.h"
#include "node_biquad.c"
#include "node_delay.c"
#include "node_dist.c"
#include "node_env.c"
#include "node_sin.c"
#include "node_square.c"

typedef struct synth_data {
  Node *graph;
} synth_data;

Node *perform_graph(Node *graph, int frame_count, double seconds_per_frame,
                    double seconds_offset) {
  Node *node = graph;

  if (node == NULL) {
    return NULL;
  };

  node->perform(node, frame_count, seconds_per_frame, seconds_offset);

  if (node->next) {
    return perform_graph(node->next, frame_count, seconds_per_frame,
                         seconds_offset);
  };

  return node;
}

void perform_synth_graph(Node *synth, int frame_count, double seconds_per_frame,
                         double seconds_offset) {

  synth_data *s_data = (synth_data *)synth->data;
  Node *node = s_data->graph;
  perform_graph(node, frame_count, seconds_per_frame, seconds_offset);
}

Node *get_synth(struct SoundIoOutStream *outstream) {
  int sample_rate = outstream->sample_rate;

  Node *head = get_sq_detune_node(220.0);
  Node *tail = head;
  tail = node_add_to_tail(get_tanh_node(tail->out, 20.0), tail);

  tail = node_add_to_tail(
      get_biquad_lpf(tail->out, 1000.0, 0.5, 2.0, sample_rate), tail);

  tail = node_mul(get_env_node(100, 25.0, 500.0, 0.0), tail);

  synth_data *data = malloc(sizeof(synth_data) + sizeof(head));
  data->graph = head;

  Node *out_node = alloc_node((NodeData *)data, NULL,
                              (t_perform)perform_synth_graph, "synth", NULL);
  out_node->out = tail->out;

  return out_node;
}
