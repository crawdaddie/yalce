#include "node.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

double process_input(double in, int frame, double seconds_per_frame,
                     double seconds_offset, double *args) {
  double gain = args[0];
  return tanh(in * gain);
}

struct Node get_tanh_node(double gain) {
  double args[1] = {gain};
  struct Node node = {.get_sample = process_input, .args = args};
  return node;
}
