#include "node.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static const double PI = 3.14159265358979323846264338328;

double get_sin_sample(double in, int frame, double seconds_per_frame,
                      double seconds_offset, double *args) {
  double radians_per_second = args[0] * 2.0 * PI;
  double sample =
      sin((seconds_offset + frame * seconds_per_frame) * radians_per_second);

  return sample;
}

double get_sin_sample_detune(double in, int frame, double seconds_per_frame,
                             double seconds_offset, double *args) {
  double radians_per_second = args[0] * 2.0 * PI;
  double sample =
      sin((seconds_offset + frame * seconds_per_frame) * radians_per_second) +
      sin((seconds_offset + frame * seconds_per_frame) * radians_per_second *
          args[1]);

  return sample * 0.5;
}

struct Node get_sin_node(double freq) {
  double args[1];
  args[0] = freq;

  struct Node node = {.get_sample = get_sin_sample, .args = args};
  return node;
}

struct Node get_sin_node_detune(double *args) {
  struct Node node = {.get_sample = get_sin_sample, .args = args};
  return node;
}

double process_tanh(double in, int frame, double seconds_per_frame,
                    double seconds_offset, double *args) {
  double gain = args[0];
  return tanh(in * gain);
}

struct Node get_tanh_node(double gain) {
  double args[1] = {gain};
  struct Node node = {.get_sample = process_tanh, .args = args};
  return node;
}

void write_node_frame(struct Node *node, double *out, int frame_count,
                      double seconds_per_frame, double seconds_offset) {

  for (int frame = 0; frame < frame_count; frame += 1) {
    double sample = 0.0;
    struct Node *n = node;
    sample = n->get_sample(sample, frame, seconds_per_frame, seconds_offset,
                           n->args);
    while (n->next) {
      n = n->next;
      sample = n->get_sample(sample, frame, seconds_per_frame, seconds_offset,
                             n->args);
    };

    out[frame] = 0.1 * sample;
  }
}
