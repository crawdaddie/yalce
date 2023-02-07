#ifndef _SQ_H
#define _SQ_H
#include "../config.h"
#include "../ctx.c"
#include "../graph/graph.c"
#include "./signal.c"
#include <math.h>

typedef struct sq_data {
  double ramp;
  double pan;
} sq_data;

double scale_val_2(double env_val, // 0-1
                   double min, double max) {
  return min + env_val * (max - min);
}

double sq_sample(double ramp, double freq) {
  return scale_val_2((fmod(ramp * freq * 2.0 * PI, 2 * PI) > PI), -1, 1);
}

void perform_square(Graph *node, int nframes, double seconds_per_frame) {
  sq_data *data = node->data;
  double pan = data->pan;

  double *out = node->out;

  int num_outs = node->num_outs;

  for (int frame = 0; frame < nframes; frame++) {
    if (frame < node->schedule) {
      // frame <= 0 < numframes
      // if schedule > 0, only start processing schedule frames into this block
      // keep breaking until frame reaches schedule - at that point set schedule
      // to -1 for future blocks if schedule = -1 i will always be greater
      break;
    };
    node->schedule = -1;
    double freq = unwrap(node->in[0], frame);
    /* printf("freq: %f\n", freq); */

    out[frame * node->num_outs] = pan * sq_sample(data->ramp, freq) * 0.1;
    out[frame * node->num_outs + 1] =
        (1 - pan) * sq_sample(data->ramp, freq * 1.02) * 0.1;

    data->ramp += seconds_per_frame;
  }
}

Graph *sq_create(double *out) {

  Graph *node = calloc(sizeof(Graph), 1);
  node->perform = perform_square;

  sq_data *data = calloc(sizeof(sq_data), 1);
  data->ramp = 0;
  data->pan = 0.5;
  node->data = data;

  if (out == NULL) {
    node->out = calloc(sizeof(double), 2 * BUF_SIZE);
    node->num_outs = 2;
  } else {
    node->out = out;
  }
  node->in = new_signal_heap(440, 1);

  return node;
}
#endif
