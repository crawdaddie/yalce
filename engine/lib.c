#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
// Node *sq_node(double freq) { return NULL; }
Node *sin_node(double freq) { return NULL; }
void group_add_tail(Node *group, Node *node) {}

void sum_perform(void *_state, double *out, int num_ins, double **inputs,
                 int nframes, double spf) {

  for (int i = 0; i < nframes; i++) {
    out[i] = 0.;
    for (int j = 0; j < num_ins; j++) {
      out[i] += inputs[j][i];
    }
  }
}

void mul_perform(void *_state, double *out, int num_ins, double **inputs,
                 int nframes, double spf) {

  for (int i = 0; i < nframes; i++) {
    out[i] = inputs[0][i];
    for (int j = 1; j < num_ins; j++) {
      out[i] *= inputs[j][i];
    }
  }
}
