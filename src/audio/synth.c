#include "synth.h"

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

  return node; // return head
}

void perform_synth_graph(Node *synth, int frame_count, double seconds_per_frame,
                         double seconds_offset) {

  synth_data *s_data = (synth_data *)synth->data;
  Node *node = s_data->graph;
  perform_graph(node, frame_count, seconds_per_frame, seconds_offset);
}

void on_free(Node *synth) { synth->should_free = 1; }

Node *get_synth(double freq, double *bus) {

  Node *head = get_sq_detune_node(freq);
  Node *tail = head;
  tail = node_add_to_tail(get_tanh_node(tail->out, 20.0), tail);

  tail = node_add_to_tail(
      get_biquad_lpf(tail->out, 1000.0, 0.5, 2.0, synth_sample_rate), tail);
  Node *env = get_env_node(100, 25.0, 500.0, 0.0);

  tail = node_mul(env, tail);
  tail = node_add_to_tail(node_out(tail->out, bus), tail);

  synth_data *data = malloc(sizeof(synth_data) + sizeof(head));
  data->graph = head;

  Node *out_node = alloc_node((NodeData *)data, NULL,
                              (t_perform)perform_synth_graph, "synth", NULL);
  out_node->out = tail->out;
  set_on_free(env, out_node, on_free);
  return out_node;
}

/* Node *node_play_synth(Node *head, double freq, double *bus) { */
/*   Node *next = head->next; */
/*   Node *synth = get_synth(freq, bus); */
/*   head->next = synth; */
/*   synth->next = next; */
/*   next->in = synth->out; */
/*   return head; */
/* } */
