#include "node.h"
typedef struct sq_data {
  double freq;
  double target_freq;
} sq_data;

void debug_sq(sq_data *data) {
  printf("freq: %f\n", data->freq);
  printf("-------\n");
}

void perform_sq_detune(Node *node, double *out, int frame_count,
                       double seconds_per_frame, double seconds_offset) {
  sq_data *data = (sq_data *)node->data;

  for (int i = 0; i < frame_count; i++) {
    double freq = data->freq;
    double radians_per_second = freq * 2.0 * PI;
    double sample =
        fmod((seconds_offset + i * seconds_per_frame) * radians_per_second,
             2 * PI) > PI;

    sample += fmod((seconds_offset + i * seconds_per_frame) *
                       radians_per_second * 1.02,
                   2 * PI) > PI;

    out[i] = (2 * sample - 1) * 0.5;
  };
}

void modulate_freq(sq_data *data, int frame_count, double seconds_per_frame,
                   double seconds_offset) {
  double mod_freq = 1.0;
  double radians_per_second = mod_freq * 2.0 * PI;
  double freq = data->freq;
  for (int i = 0; i < frame_count; i++) {

    double mod = freq + (0.005 * sin((seconds_offset + i * seconds_per_frame) *
                                     radians_per_second));
    data->freq = mod;
  }
}

void perform_sq_detune_slide(Node *node, double *out, int frame_count,
                             double seconds_per_frame, double seconds_offset) {
  sq_data *data = (sq_data *)node->data;

  for (int i = 0; i < frame_count; i++) {
    /* slide_parameter(node); */
    double freq = data->freq;

    double radians_per_second = freq * 2.0 * PI;
    double sample =
        fmod((seconds_offset + i * seconds_per_frame) * radians_per_second,
             2 * PI) > PI;

    sample += fmod((seconds_offset + i * seconds_per_frame) *
                       radians_per_second * 1.02,
                   2 * PI) > PI;

    out[i] = (2 * sample - 1) * 0.5;
  };
}

void set_freq(Node *node, double freq) {
  sq_data *node_data = (sq_data *)node->data;
  node_data->freq = freq;
}

Node *get_sq_detune_node(double freq) {
  sq_data *data = malloc(sizeof(sq_data));
  data->freq = freq;
  data->target_freq = freq;
  Node *node = alloc_node((NodeData *)data, (t_perform)perform_sq_detune,
                          "square", NULL);
  return node;
}
