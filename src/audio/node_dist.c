#include "node.h"
typedef struct tanh_data {
  double gain;
} tanh_data;
void perform_tanh(Node *node, double *out, int frame_count,
                  double seconds_per_frame, double seconds_offset) {
  tanh_data *data = (tanh_data *)node->data;
  for (int i = 0; i < frame_count; i++) {
    double sample = tanh(out[i] * data->gain);
    out[i] = sample;
  };
}
Node *get_tanh_node(double gain) {
  tanh_data *data = malloc(sizeof(tanh_data));
  data->gain = gain;
  return alloc_node((NodeData *)data, (t_perform)perform_tanh, "tanh");
}
