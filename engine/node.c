#include "node.h"
#include "oscillators.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

Node *_chain;

void reset_chain() { _chain = NULL; }

void write_to_output_(double *src, int src_layout, double *dest,
                      int dest_layout, int nframes, int output_num) {
  if (dest_layout == 1 && src_layout == dest_layout) {
    for (int f = 0; f < nframes; f++) {
      if (output_num > 0) {
        *dest += *(src + f);
      } else {
        *dest = *(src + f);
      }
      dest++;
    }
  } else if (dest_layout == 2 && src_layout == dest_layout) {
    for (int f = 0; f < nframes; f++) {
      if (output_num > 0) {
        dest[2 * f] += *(src + 2 * f);
      } else {
        dest[2 * f] = *(src + 2 * f);
      }

      if (output_num > 0) {
        dest[2 * f + 1] += *(src + (2 * f) + 1);
      } else {
        dest[2 * f + 1] = *(src + (2 * f) + 1);
      }
    }
  } else if (dest_layout == 2 && src_layout == 1) {
    for (int f = 0; f < nframes; f++) {
      double *samp = (src + f);
      if (output_num > 0) {
        dest[2 * f] += *samp;
      } else {
        dest[2 * f] = *samp;
      }

      if (output_num > 0) {
        dest[2 * f + 1] += *samp;
      } else {
        dest[2 * f + 1] = *samp;
      }
    }
  }
}

void write_to_output(double *src, int src_layout, double *dest, int dest_layout,
                     int nframes, int output_num) {
  int i;
  double *src_end = src + nframes * src_layout;

  if (output_num > 0) {
    if (src_layout == dest_layout) {
      if (dest_layout == 1) {
        // both mono
        for (; src < src_end; src++, dest++) {
          *dest += *src;
        }
      } else if (dest_layout == 2) {
        // both stereo
        for (; src < src_end; src += 2, dest += 2) {
          dest[0] += src[0];
          dest[1] += src[1];
        }
      }
    } else if (dest_layout == 2 && src_layout == 1) {
      for (; src < src_end; src++, dest += 2) {
        dest[0] += *src;
        dest[1] += *src;
      }
    }
  } else {
    if (src_layout == dest_layout) {
      memcpy(dest, src, nframes * dest_layout * sizeof(double));
    } else if (dest_layout == 2 && src_layout == 1) {
      for (i = 0; i < nframes; i++) {
        dest[2 * i] = dest[2 * i + 1] = src[i];
      }
    }
  }
}

static void offset_node_bufs(Node *node, int frame_offset) {

  if (frame_offset == 0) {
    return;
  }

  if (node->ins == NULL) {
    return;
  }

  node->out.buf += frame_offset;
  for (int i = 0; i < node->num_ins; i++) {
    node->ins[i].buf += frame_offset;
  }
}

static void unoffset_node_bufs(Node *node, int frame_offset) {
  if (frame_offset == 0) {
    return;
  }

  node->out.buf -= frame_offset;
  for (int i = 0; i < node->num_ins; i++) {
    node->ins[i].buf -= frame_offset;
  }
  node->frame_offset = 0;
}

Node *perform_graph(Node *head, int nframes, double spf, double *dest_buf,
                    int dest_layout, int output_num) {

  if (!head) {
    return NULL;
  }

  int frame_offset = head->frame_offset;
  // int frame_offset = 0;

  offset_node_bufs(head, frame_offset);
  if (head->perform != NULL) {
    head->perform(head, nframes, spf);
  }

  if (head->type == OUTPUT) {
    write_to_output(head->out.buf, head->out.layout, dest_buf + frame_offset,
                    dest_layout, nframes - frame_offset, output_num);
    output_num++;
  }
  unoffset_node_bufs(head, frame_offset);

  if (head->next != NULL) {
    return perform_graph(head->next, nframes, spf, dest_buf, dest_layout,
                         output_num);
  };
  return head;
}

node_perform group_perform(Node *group_node, int nframes, double spf) {
  group_state *state = group_node->state;
  perform_graph(state->head, nframes, spf, group_node->out.buf,
                group_node->out.layout, 0);
}

Node *group_add_tail(Node *group, Node *node) {
  group_state *group_ctx = group->state;

  if (group_ctx->head == NULL) {
    group_ctx->head = node;
    group_ctx->tail = node;
    return node;
  }

  group_ctx->tail->next = node;
  group_ctx->tail = node;
  return node;
}

Node *node_new(void *data, node_perform *perform, int num_ins, Signal *ins) {
  Node *node = malloc(sizeof(Node));
  node->state = data;
  node->num_ins = num_ins;
  node->ins = ins;
  node->out.layout = 1;
  node->out.size = BUF_SIZE;
  node->out.buf = calloc(BUF_SIZE, sizeof(double));
  node->perform = (node_perform)perform;
  node->frame_offset = 0;

  if (_chain == NULL) {
    _chain = group_new(0);
  }
  group_add_tail(_chain, node);

  return node;
}

Signal *get_sig(int layout) {
  Signal *sig = malloc(sizeof(Signal));
  sig->buf = calloc(BUF_SIZE * layout, sizeof(double));
  sig->layout = layout;
  sig->size = BUF_SIZE;
  return sig;
}

Signal *get_sig_default(int layout, double value) {
  Signal *sig = get_sig(layout);
  for (int i = 0; i < BUF_SIZE * layout; i++) {
    sig->buf[i] = value;
  }
  return sig;
}

Node *group_new(int chans) {
  group_state *graph = malloc(sizeof(group_state));

  Node *node = malloc(sizeof(Node));
  node->state = graph;
  node->num_ins = 0;
  node->ins = NULL;
  node->out.layout = 1;
  node->out.size = BUF_SIZE;
  node->out.buf = calloc(BUF_SIZE, sizeof(double));
  node->perform = (node_perform)group_perform;
  node->frame_offset = 0;

  return node;
}

Node *sq_node_of_scalar(double freq) {
  printf("sq node %f\n", freq);
  sq_state *state = malloc(sizeof(sq_state));
  state->phase = 0.0;

  Node *s =
      node_new(state, (node_perform *)sq_perform, 1, get_sig_default(1, freq));

  return s;
}

Node *sq_node(Node *freq) {
  sq_state *state = malloc(sizeof(sq_state));
  state->phase = 0.0;

  Node *s = node_new(state, (node_perform *)sq_perform, 1, &freq->out);

  return s;
}

Node *sin_node_of_scalar(double freq) {
  sin_state *state = malloc(sizeof(sin_state));
  state->phase = 0.0;

  Node *s =
      node_new(state, (node_perform *)sin_perform, 1, get_sig_default(1, freq));
  return s;
}

Node *sin_node(Node *freq) {
  sin_state *state = malloc(sizeof(sin_state));
  state->phase = 0.0;

  Node *s = node_new(state, (node_perform *)sin_perform, 1, &freq->out);
  return s;
}

node_perform sum_perform_(Node *node, int nframes, double spf) {
  int num_ins = node->num_ins;
  double *out = node->out.buf;

  Signal *input_sigs = node->ins;
  for (int i = 0; i < nframes; i++) {
    out[i] = input_sigs[0].buf[i];

    for (int j = 1; j < num_ins; j++) {
      out[i] += input_sigs[j].buf[i];
    }
  }
}

// node_perform sum_perform(Node *node, int nframes, double spf) {
//   int num_ins = node->num_ins;
//   double *out = node->out.buf;
//   Signal *input_sigs = node->ins;
//   double *in0 = input_sigs[0].buf;
//   int i, j;
//
//   if (num_ins == 1) {
//     memcpy(out, in0, nframes * sizeof(double));
//   } else {
//     for (i = 0; i < nframes; i += 4) {
//       double sum0 = in0[i];
//       double sum1 = in0[i + 1];
//       double sum2 = in0[i + 2];
//       double sum3 = in0[i + 3];
//
//       for (j = 1; j < num_ins; j++) {
//         double *in = input_sigs[j].buf + i;
//         sum0 += in[0];
//         sum1 += in[1];
//         sum2 += in[2];
//         sum3 += in[3];
//       }
//
//       out[i] = sum0;
//       out[i + 1] = sum1;
//       out[i + 2] = sum2;
//       out[i + 3] = sum3;
//     }
//
//     // Handle remaining frames if nframes is not divisible by 4
//     for (; i < nframes; i++) {
//       double sum = in0[i];
//       for (j = 1; j < num_ins; j++) {
//         sum += input_sigs[j].buf[i];
//       }
//       out[i] = sum;
//     }
//   }
// }
//
// node_perform mul_perform(Node *node, int nframes, double spf) {
//   int num_ins = node->num_ins;
//   double *out = node->out.buf;
//   Signal *input_sigs = node->ins;
//   double *in0 = input_sigs[0].buf;
//   int i, j;
//
//   if (num_ins == 1) {
//     memcpy(out, in0, nframes * sizeof(double));
//   } else {
//     for (i = 0; i < nframes; i += 4) {
//       double sum0 = in0[i];
//       double sum1 = in0[i + 1];
//       double sum2 = in0[i + 2];
//       double sum3 = in0[i + 3];
//
//       for (j = 1; j < num_ins; j++) {
//         double *in = input_sigs[j].buf + i;
//         sum0 *= in[0];
//         sum1 *= in[1];
//         sum2 *= in[2];
//         sum3 *= in[3];
//       }
//
//       out[i] = sum0;
//       out[i + 1] = sum1;
//       out[i + 2] = sum2;
//       out[i + 3] = sum3;
//     }
//
//     // Handle remaining frames if nframes is not divisible by 4
//     for (; i < nframes; i++) {
//       double sum = in0[i];
//       for (j = 1; j < num_ins; j++) {
//         sum *= input_sigs[j].buf[i];
//       }
//       out[i] = sum;
//     }
//   }
// }
#define OP_PERFORM(name, op)                                                   \
  node_perform name##_perform(Node *node, int nframes, double spf) {           \
    int num_ins = node->num_ins;                                               \
    double *out = node->out.buf;                                               \
    Signal *input_sigs = node->ins;                                            \
    double *in0 = input_sigs[0].buf;                                           \
    int i, j;                                                                  \
                                                                               \
    if (num_ins == 1) {                                                        \
      memcpy(out, in0, nframes * sizeof(double));                              \
    } else {                                                                   \
      for (i = 0; i < nframes; i += 4) {                                       \
        double sum0 = in0[i];                                                  \
        double sum1 = in0[i + 1];                                              \
        double sum2 = in0[i + 2];                                              \
        double sum3 = in0[i + 3];                                              \
                                                                               \
        for (j = 1; j < num_ins; j++) {                                        \
          double *in = input_sigs[j].buf + i;                                  \
          sum0 op in[0];                                                       \
          sum1 op in[1];                                                       \
          sum2 op in[2];                                                       \
          sum3 op in[3];                                                       \
        }                                                                      \
                                                                               \
        out[i] = sum0;                                                         \
        out[i + 1] = sum1;                                                     \
        out[i + 2] = sum2;                                                     \
        out[i + 3] = sum3;                                                     \
      }                                                                        \
                                                                               \
      for (; i < nframes; i++) {                                               \
        double sum = in0[i];                                                   \
        for (j = 1; j < num_ins; j++) {                                        \
          sum op input_sigs[j].buf[i];                                         \
        }                                                                      \
        out[i] = sum;                                                          \
      }                                                                        \
    }                                                                          \
  }

OP_PERFORM(sum, +=)
OP_PERFORM(sub, -=)
OP_PERFORM(mul, *=)
OP_PERFORM(div, /=)

node_perform mod_perform(Node *node, int nframes, double spf) {
  int num_ins = node->num_ins;
  double *out = node->out.buf;
  Signal *input_sigs = node->ins;
  double *in0 = input_sigs[0].buf;
  int i, j;

  if (num_ins == 1) {
    memcpy(out, in0, nframes * sizeof(double));
  } else {
    for (i = 0; i < nframes; i += 4) {
      double sum0 = in0[i];
      double sum1 = in0[i + 1];
      double sum2 = in0[i + 2];
      double sum3 = in0[i + 3];

      for (j = 1; j < num_ins; j++) {
        double *in = input_sigs[j].buf + i;
        sum0 = fmod(sum0, in[0]);
        sum1 = fmod(sum1, in[1]);
        sum2 = fmod(sum2, in[2]);
        sum3 = fmod(sum3, in[3]);
      }

      out[i] = sum0;
      out[i + 1] = sum1;
      out[i + 2] = sum2;
      out[i + 3] = sum3;
    }

    // Handle remaining frames if nframes is not divisible by 4
    for (; i < nframes; i++) {
      double sum = in0[i];
      for (j = 1; j < num_ins; j++) {
        sum *= input_sigs[j].buf[i];
      }
      out[i] = sum;
    }
  }
}

/*
Node *sum2_node(Node *a, Node *b) {
Signal *ins = malloc(sizeof(double *) * 2);
Node *sum = node_new(NULL, (node_perform *)sum_perform, 2, ins);
sum->ins[0] = a->out;
sum->ins[1] = b->out;
return sum;
}

Node *mul2_node(Node *a, Node *b) {
Signal *ins = malloc(sizeof(double *) * 2);
Node *sum = node_new(NULL, (node_perform *)mul_perform, 2, ins);
sum->ins[0] = a->out;
sum->ins[1] = b->out;
return sum;
}
*/

#define BINARY_OP_NODE(name, perform_func)                                     \
  Node *name(Node *a, Node *b) {                                               \
    int num_ins = 2;                                                           \
    Signal *ins = malloc(sizeof(Signal) * num_ins);                            \
    Node *op_node =                                                            \
        node_new(NULL, (node_perform *)perform_func, num_ins, ins);            \
    op_node->ins[0] = a->out;                                                  \
    op_node->ins[1] = b->out;                                                  \
    return op_node;                                                            \
  }

BINARY_OP_NODE(sum2_node, sum_perform)
BINARY_OP_NODE(sub2_node, sub_perform)
BINARY_OP_NODE(mul2_node, mul_perform)
BINARY_OP_NODE(div2_node, div_perform)
BINARY_OP_NODE(mod2_node, mod_perform)

Node *node_of_double(double val) {
  printf("node of double %f\n", val);
  Node *const_node = node_new(NULL, NULL, 0, NULL);
  for (int i = 0; i < const_node->out.size; i++) {
    const_node->out.buf[i] = val;
  }

  return const_node;
}
