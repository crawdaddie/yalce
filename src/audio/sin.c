#include "sin.h"
#include "../common.h"
#include <math.h>

static node_perform sin_perform(Node *node, int nframes, double spf) {
  sin_data *data = NODE_DATA(sin_data, node);

  double freq = data->freq;
  double radians_per_second = freq * 2.0 * PI;

  for (int f = 0; f < nframes; f++) {
    double sample = sin((data->ramp + f * spf) * radians_per_second);

    data->ramp += spf;
    data->out->data[f] = sample;
  }
}

Node *sin_node(double freq) {
  Node *sin = ALLOC_NODE(sin_data, "sin");
  sin->perform = sin_perform;
  sin_data *data = sin->data;
  data->freq = freq;
  data->out = new_signal_heap(BUF_SIZE, 1);
  return sin;
}
