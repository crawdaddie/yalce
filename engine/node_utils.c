#include "./node_utils.h"
#include "./node.h"
#include "common.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>

void *mul_perform(Node *node, int nframes, double spf) {
  printf("mul perform %p\n", node);

  int num_ins = node->num_ins;
  double *out = node->out.buf;

  Signal input_sigs[num_ins];
  for (int ii = 0; ii < num_ins; ii++) {
    input_sigs[ii] = *get_node_input(node, ii);
  }

  double *in0 = input_sigs[0].buf;
  int i = 0;
  int j = 0;
  if (num_ins == 1) {
    __builtin___memcpy_chk(out, in0, nframes * sizeof(double),
                           __builtin_object_size(out, 0));
  } else {
    for (i = 0; i < nframes; i += 4) {
      double sum0 = in0[i];
      double sum1 = in0[i + 1];
      double sum2 = in0[i + 2];
      double sum3 = in0[i + 3];
      for (j = 1; j < num_ins; j++) {
        double *in = input_sigs[j].buf + i;
        sum0 *= in[0];
        sum1 *= in[1];
        sum2 *= in[2];
        sum3 *= in[3];
      }
      out[i] = sum0;
      out[i + 1] = sum1;
      out[i + 2] = sum2;
      out[i + 3] = sum3;
    }
    for (; i < nframes; i++) {
      double sum = in0[i];
      for (j = 1; j < num_ins; j++) {
        sum *= input_sigs[j].buf[i];
      }
      out[i] = sum;
    }
  }
  return (char *)node + sizeof(Node);
}

NodeRef mul2_node(SignalRef a, SignalRef b) {

  Node *node = node_new();

  int in_offset_a = (char *)a - (char *)node;
  int in_offset_b = (char *)b - (char *)node;
  *node = (Node){.num_ins = 2,
                 .input_offsets = {in_offset_a, in_offset_b},
                 .node_size = sizeof(Node),
                 .out = {.size = BUF_SIZE,
                         .layout = 1,
                         .buf = malloc(sizeof(double) * BUF_SIZE)},
                 .node_perform = (perform_func_t)mul_perform,
                 .next = NULL};
  return node;
}
