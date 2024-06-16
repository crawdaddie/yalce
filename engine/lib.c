#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
// Node *sq_node(double freq) { return NULL; }
Node *sin_node(double freq) { return NULL; }
void group_add_tail(Node *group, Node *node) {}

node_perform sum_perform(Node *node, int nframes, double spf) {

  double *out = node->output_buf;
  int num_ins = node->num_ins;
  double **inputs = node->ins;
  for (int i = 0; i < nframes; i++) {
    out[i] = 0.;
    for (int j = 0; j < num_ins; j++) {
      out[i] += inputs[j][i];
    }
  }
}

node_perform mul_perform(Node *node, int nframes, double spf) {
  double *out = node->output_buf;
  int num_ins = node->num_ins;
  double **inputs = node->ins;

  for (int i = 0; i < nframes; i++) {
    out[i] = inputs[0][i];
    for (int j = 1; j < num_ins; j++) {
      out[i] *= inputs[j][i];
    }
  }
}
