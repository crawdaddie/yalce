#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
// Node *sq_node(double freq) { return NULL; }
Node *sin_node(double freq) { return NULL; }

node_perform _sum_perform(Node *node, int nframes, double spf) {

  double *out = node->out.buf;
  int num_ins = node->num_ins;
  Signal *input_sigs = node->ins;
  for (int i = 0; i < nframes; i++) {
    out[i] = 0.;
    for (int j = 0; j < num_ins; j++) {
      out[i] += input_sigs[j].buf[i];
    }
  }
}

node_perform mul_perform(Node *node, int nframes, double spf) {
  double *out = node->out.buf;
  int num_ins = node->num_ins;
  Signal *input_sigs = node->ins;

  for (int i = 0; i < nframes; i++) {
    out[i] = input_sigs[0].buf[i];
    for (int j = 1; j < num_ins; j++) {
      out[i] *= input_sigs[j].buf[i];
    }
  }
}
