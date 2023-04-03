#include "sq.h"
#include "../memory.h"
#include "../node.h"
#include "math.h"
#include "signal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// static double scale_val_2(double env_val, // 0-1
//                           double min, double max) {
//   return min + env_val * (max - min);
// }
//
// static double sq_sample(double ramp, double freq) {
//   return scale_val_2((fmod(ramp * freq * 2.0 * PI, 2 * PI) > PI), -1, 1);
// }
//
// static void perform_square(Graph *node, int nframes, double
// seconds_per_frame) {
//   sq_data *data = node->data;
//   double pan = data->pan;
//
//   double *out = node->out;
//
//   int num_outs = node->num_outs;
//
//   for (int frame = 0; frame < nframes; frame++) {
//     if (frame < node->schedule) {
//       // frame <= 0 < numframes
//       // if schedule > 0, only start processing schedule frames into this
//       block
//       // keep breaking until frame reaches schedule - at that point set
//       schedule
//       // to -1 for future blocks if schedule = -1 i will always be greater
//       break;
//     };
//     node->schedule = -1;
//     double freq = unwrap(node->in[0], frame);
//     /* printf("freq: %f\n", freq); */
//
//     out[frame * node->num_outs] = pan * sq_sample(data->ramp, freq) * 0.1;
//     out[frame * node->num_outs + 1] =
//         (1 - pan) * sq_sample(data->ramp, freq * 1.02) * 0.1;
//
//     data->ramp += seconds_per_frame;
//   }
// }
//
// Graph *sq_create(double *out, Signal *freq) {
//
//   Graph *node = calloc(sizeof(Graph), 1);
//   node->perform = perform_square;
//
//   sq_data *data = calloc(sizeof(sq_data), 1);
//   data->ramp = 0;
//   data->pan = 0.5;
//   node->data = data;
//
//   if (out == NULL) {
//     node->out = calloc(sizeof(double), 2 * BUF_SIZE);
//     node->num_outs = 2;
//   } else {
//     node->out = out;
//   }
//   node->in = freq ? freq : new_signal_heap(440, 1);
//
//   return node;
// }
//
//
static double sq_sample(double phase, double freq) {
  return scale_val_2(fmod(phase * freq * 2.0 * PI, 2 * PI) > PI, -1, 1);
}

static node_perform sq_perform(Node *node, int nframes, double spf) {
  sq_data *data = NODE_DATA(sq_data, node);
  double freq = data->freq;
  for (int f = 0; f < nframes; f++) {
    double sample = sq_sample(data->ramp, freq);


    data->ramp += spf;
    data->out->data[f] = sample;
  }
}


Node *sq_node() {
  Node *sq = ALLOC_NODE(sq_data, "square");
  sq->perform = sq_perform;
  sq_data *data = sq->object;
  data->freq = 220;
  data->out = new_signal_heap(BUF_SIZE);
  return sq;
}
