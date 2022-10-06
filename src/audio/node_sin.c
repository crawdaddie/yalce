#include "node.h"
typedef struct sin_data {
  double freq;
  double phase;
} sin_data;

void perform_sin_detune(Node *node, int frame_count, double seconds_per_frame,
                        double seconds_offset, double schedule) {
  sin_data *data = (sin_data *)node->data;
  double *out = node->out;

  for (int i = 0; i < frame_count; i++) {
    sched();

    double phase = data->phase;
    double freq = data->freq;
    double radians_per_second = freq * 2.0 * PI;
    double sample = sin(phase * radians_per_second);

    sample += sin(phase * radians_per_second * 1.01);

    out[i] = sample * 0.5;
    phase += seconds_per_frame;
    data->phase = phase;
  };
}

Node *get_sin_detune_node(double freq) {
  sin_data *data = malloc(sizeof(sin_data));
  data->freq = freq;
  data->phase = 0.0;
  Node *node = alloc_node((NodeData *)data, NULL, (t_perform)perform_sin_detune,
                          "sin", NULL);
  return node;
}
