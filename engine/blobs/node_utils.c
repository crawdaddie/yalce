#include "./node_utils.h"
#include "./node.h"
#include "alloc.h"
#include "common.h"
#include "signals.h"
#include <stdio.h>
#include <stdlib.h>

void *mul_perform(Node *node, int nframes, double spf) {
  // printf("mul perform %p\n", node);

  int num_ins = node->num_ins;

  double *out = get_node_out_buf(node);

  double *input_sigs[num_ins];
  for (int i = 0; i < num_ins; i++) {
    input_sigs[i] = get_node_input_buf(node, i);
  }

  while (nframes--) {
    *out = *input_sigs[0];
    input_sigs[0] = input_sigs[0]++;
    for (int i = 1; i < num_ins; i++) {
      *out *= *input_sigs[i];
      input_sigs[i] = input_sigs[i]++;
    }
    out++;
  }

  // double *in0 = input_sigs[0].buf;
  // int i = 0;
  // int j = 0;
  // if (num_ins == 1) {
  //   __builtin___memcpy_chk(out, in0, nframes * sizeof(double),
  //                          __builtin_object_size(out, 0));
  // } else {
  //   for (i = 0; i < nframes; i += 4) {
  //     double sum0 = in0[i];
  //     double sum1 = in0[i + 1];
  //     double sum2 = in0[i + 2];
  //     double sum3 = in0[i + 3];
  //     for (j = 1; j < num_ins; j++) {
  //       double *in = input_sigs[j].buf + i;
  //       sum0 *= in[0];
  //       sum1 *= in[1];
  //       sum2 *= in[2];
  //       sum3 *= in[3];
  //     }
  //     out[i] = sum0;
  //     out[i + 1] = sum1;
  //     out[i + 2] = sum2;
  //     out[i + 3] = sum3;
  //   }
  //   for (; i < nframes; i++) {
  //     double sum = in0[i];
  //     for (j = 1; j < num_ins; j++) {
  //       sum *= input_sigs[j].buf[i];
  //     }
  //     out[i] = sum;
  //   }
  // }
  return (char *)node + sizeof(Node);
}

NodeRef mul2_node(SignalRef a, SignalRef b) {

  Node *node = engine_alloc(sizeof(Node));
  blob_register_node(node);
  double *out = (double *)engine_alloc(sizeof(double) * BUF_SIZE * 1);

  size_t total_size = sizeof(Node) + sizeof(double) * BUF_SIZE;

  int in_offset_a = (char *)a->buf - (char *)node;
  int in_offset_b = (char *)b->buf - (char *)node;
  *node = (Node){.output_buf_offset = (char *)out - (char *)node,
                 .output_size = BUF_SIZE,
                 .output_layout = 1,
                 .num_ins = 2,
                 .input_offsets = {in_offset_a, in_offset_b},
                 .input_sizes = {a->size, b->size},
                 .input_layouts = {a->layout, b->layout},
                 .node_size = total_size,
                 .node_perform = (perform_func_t)mul_perform,
                 .next = NULL};
  // printf("mul2 node %p size %d\n", node, node->node_size);
  return node;
}
