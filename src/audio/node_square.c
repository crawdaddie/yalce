#include "node.h"
typedef struct sq_data {
  double freq;
  double phase;
} sq_data;

void debug_sq(sq_data *data) {
  printf("freq: %f\n", data->freq);
  printf("-------\n");
}

void perform_sq_detune(Node *node, int frame_count, double seconds_per_frame,
                       double seconds_offset) {
  double *out = node->out;
  sq_data *data = (sq_data *)node->data;

  double phase = data->phase;
  for (int i = 0; i < frame_count; i++) {
    double freq = data->freq;
    double radians_per_second = freq * 2.0 * PI;
    double sample = 2.0 * (fmod(phase * radians_per_second, 2 * PI) > PI) - 1;

    sample += 2.0 * (fmod(phase * radians_per_second * 1.02, 2 * PI) > PI) - 1;

    out[i] = sample * 0.5;
    phase += seconds_per_frame;
  }
  data->phase = phase;
}

void set_freq(Node *node, double freq) {
  sq_data *node_data = (sq_data *)node->data;
  node_data->freq = freq;
}

Node *get_sq_detune_node(double freq) {
  sq_data *data = malloc(sizeof(sq_data));
  data->freq = freq;
  data->phase = 0.0;
  Node *node = alloc_node((NodeData *)data, NULL, (t_perform)perform_sq_detune,
                          "square", NULL);
  return node;
}
