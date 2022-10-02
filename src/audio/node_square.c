#include "node.h"
typedef struct sq_data {
  double freq;
  double _prev_freq;
} sq_data;

void debug_sq(sq_data *data) {
  printf("freq: %f\n", data->freq);
  printf("-------\n");
}

void perform_sq_detune(Node *node, double *out, int frame_count,
                       double seconds_per_frame, double seconds_offset) {
  sq_data *data = (sq_data *)node->data;
  double target_freq = data->freq;

  double radians_per_second = target_freq * 2.0 * PI;
  for (int i = 0; i < frame_count; i++) {
    double sample =
        fmod((seconds_offset + i * seconds_per_frame) * radians_per_second,
             2 * PI) > PI;

    sample += fmod((seconds_offset + i * seconds_per_frame) *
                       radians_per_second * 1.02,
                   2 * PI) > PI;

    out[i] = (2 * sample - 1) * 0.5;
  };
}

Node *get_sq_detune_node(sq_data *data) {
  return alloc_node((NodeData *)data, (t_perform)perform_sq_detune, "square");
}
