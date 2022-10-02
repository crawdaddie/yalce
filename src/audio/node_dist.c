#include "node.h"
typedef struct tanh_data {
  double gain;
} tanh_data;
void perform_tanh(Node *node, double *out, int frame_count,
                  double seconds_per_frame, double seconds_offset) {
  for (int i = 0; i < frame_count; i++) {
    double sample = tanh(out[i] * 10.0);
    out[i] = sample;
  };
}
Node *get_tanh_node(tanh_data *data) {
  return alloc_node((NodeData *)data, (t_perform)perform_tanh, "tanh");
}
