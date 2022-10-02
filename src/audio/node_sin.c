#include "node.h"
typedef struct sin_data {
  double freq;
} sin_data;

void perform_sin_detune(Node *node, double *out, int frame_count,
                        double seconds_per_frame, double seconds_offset) {
  sin_data *data = (sin_data *)node->data;

  for (int i = 0; i < frame_count; i++) {
    double freq = data->freq;
    double radians_per_second = freq * 2.0 * PI;
    double sample =
        sin((seconds_offset + i * seconds_per_frame) * radians_per_second);

    sample += sin((seconds_offset + i * seconds_per_frame) *
                  radians_per_second * 1.01);

    out[i] = sample * 0.5;
  };
}

Node *get_sin_detune_node(double freq) {
  sin_data *data = malloc(sizeof(sin_data));
  data->freq = freq;
  Node *node = malloc(sizeof(Node) + sizeof(data));
  node->name = "square";
  node->perform = perform_sin_detune;
  node->next = NULL;
  node->data = (NodeData *)data;
  return node;
}
