#include "lib.h"
#include "ctx.h"
#include <stdio.h>
#include <stdlib.h>
// Node *sq_node(double freq) { return NULL; }
// Node *sin_node(double freq) { return NULL; }

// node_perform _sum_perform(Node *node, int nframes, double spf) {
//
//   double *out = node->out.buf;
//   int num_ins = node->num_ins;
//   Signal *input_sigs = node->ins;
//   for (int i = 0; i < nframes; i++) {
//     out[i] = 0.;
//     for (int j = 0; j < num_ins; j++) {
//       out[i] += input_sigs[j].buf[i];
//     }
//   }
// }
//
// node_perform mul_perform(Node *node, int nframes, double spf) {
//   double *out = node->out.buf;
//   int num_ins = node->num_ins;
//   Signal *input_sigs = node->ins;
//
//   for (int i = 0; i < nframes; i++) {
//     out[i] = input_sigs[0].buf[i];
//     for (int j = 1; j < num_ins; j++) {
//       out[i] *= input_sigs[j].buf[i];
//     }
//   }
// }
//
Node *play_test_synth() {
  double freq = 100.;
  double cutoff = 500.;
  Node *group = group_new(0);

  Node *sq1 = sq_node_of_scalar(freq);
  group_add_tail(group, sq1);

  Node *sq2 = sq_node_of_scalar(freq * 1.01);
  group_add_tail(group, sq2);

  Node *summed = sum2_node(sq1, sq2);
  group_add_tail(group, summed);
  add_to_dac(summed);

  // return group;

  add_to_dac(group);
  audio_ctx_add(group);
  return group;
}

Node *play_node(Node *s) {
  Node *group = _chain;
  add_to_dac(s);
  add_to_dac(group);
  audio_ctx_add(group);
  reset_chain();
  return group;
}

void accept_callback(int (*callback)(int, int)) {
  // Function body
  if (callback != NULL) {
    printf("called callback %p %d\n", callback,
           callback(1, 2)); // Call the callback function
  }
}
