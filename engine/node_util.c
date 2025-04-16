#include "./node_util.h"
#include "audio_graph.h"
#include "lib.h"
#include <stdio.h>
#include <stdlib.h>

#define NODE_BINOP(name, _meta, _perform)                                      \
  NodeRef name(NodeRef input1, NodeRef input2) {                               \
    AudioGraph *graph = _graph;                                                \
    Node *node = allocate_node_in_graph(graph, 0);                             \
    *node = (Node){                                                            \
        .perform = (perform_func_t)_perform,                                   \
        .node_index = node->node_index,                                        \
        .num_inputs = 2,                                                       \
        .state_size = 0,                                                       \
        .state_offset = graph ? graph->state_memory_size : 0,                  \
        .output = (Signal){.layout = 1,                                        \
                           .size = BUF_SIZE,                                   \
                           .buf = allocate_buffer_from_pool(graph, BUF_SIZE)}, \
        .meta = _meta,                                                         \
    };                                                                         \
    if (input1) {                                                              \
      node->connections[0].source_node_index = input1->node_index;             \
    }                                                                          \
    if (input2) {                                                              \
      node->connections[1].source_node_index = input2->node_index;             \
    }                                                                          \
    return node;                                                               \
  }

#define INVAL(_sig)                                                            \
  ({                                                                           \
    double *val;                                                               \
    if (_sig.size == 1 && _sig.layout == 1) {                                  \
      val = _sig.buf;                                                          \
    } else {                                                                   \
      val = _sig.buf;                                                          \
      _sig.buf += _sig.layout;                                                 \
    }                                                                          \
    val;                                                                       \
  })
void *mul_perform(Node *node, void *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffers
  Signal in1 = inputs[0]->output;
  Signal in2 = inputs[1]->output;

  // Multiply samples
  for (int i = 0; i < nframes; i++) {
    double sample = *INVAL(in1) * *INVAL(in2);

    // Write to all channels in output layout
    for (int j = 0; j < out_layout; j++) {
      out[i * out_layout + j] = sample;
    }
  }

  return node->output.buf;
}

NodeRef mul2_node(NodeRef input1, NodeRef input2) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);

  *node = (Node){
      .perform = (perform_func_t)mul_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = 0,
      .state_offset = graph->state_memory_size,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "mul",
  };

  node->connections[0].source_node_index = input1->node_index;
  node->connections[1].source_node_index = input2->node_index;

  return node;
};

void *sum_perform(Node *node, void *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffers
  Signal in1 = inputs[0]->output;
  Signal in2 = inputs[1]->output;

  double *out_ptr = out;
  while (nframes--) {
    double sample = *INVAL(in1) + *INVAL(in2);

    // Write to all channels in output layout
    for (int i = 0; i < out_layout; i++) {
      *out_ptr++ = sample;
    }
  }

  return node->output.buf;
}

NodeRef sum2_node(NodeRef input1, NodeRef input2) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);
  *node = (Node){
      .perform = (perform_func_t)sum_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = 0,
      .state_offset = 0,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "sum",
  };
  if (input1) {
    node->connections[0].source_node_index = input1->node_index;
  }
  if (input2) {
    node->connections[1].source_node_index = input2->node_index;
  }
  return node;
}

void *sub_perform(Node *node, void *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffers
  Signal in1 = inputs[0]->output;
  Signal in2 = inputs[1]->output;

  double *out_ptr = out;
  while (nframes--) {

    double sample = *INVAL(in1) - *INVAL(in2);

    // Write to all channels in output layout
    for (int i = 0; i < out_layout; i++) {
      *out_ptr++ = sample;
    }
  }

  return node->output.buf;
}

NODE_BINOP(sub2_node, "sub", sub_perform)

void *mod_perform(Node *node, void *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  // Get input buffers
  Signal in1 = inputs[0]->output;
  Signal in2 = inputs[1]->output;

  double *out_ptr = out;
  while (nframes--) {
    double sample = fmod(*INVAL(in1), *INVAL(in2));

    // Write to all channels in output layout
    for (int i = 0; i < out_layout; i++) {
      *out_ptr++ = sample;
    }
  }

  return node->output.buf;
}

NODE_BINOP(mod2_node, "mod", mod_perform)
static inline double __min(double a, double b) { return a <= b ? a : b; }
void *div_perform(Node *node, void *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;
  // Get input buffers
  Signal in1 = inputs[0]->output;
  Signal in2 = inputs[1]->output;

  double *out_ptr = out;
  while (nframes--) {
    double sample = (*INVAL(in1) / __min(*INVAL(in2), 0.0001));

    // Write to all channels in output layout
    for (int i = 0; i < out_layout; i++) {
      *out_ptr++ = sample;
    }
  }

  return node->output.buf;
}

NODE_BINOP(div2_node, "div", div_perform)

Node *const_sig(double val) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);

  *node = (Node){
      .perform = NULL,
      .node_index = node->node_index,
      .num_inputs = 0,
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},

      .meta = "const",
  };

  for (int i = 0; i < BUF_SIZE; i++) {
    node->output.buf[i] = val;
    // printf("const node val %f\n", node->output.buf[i]);
  }
  return node;
}

Node *const_buf(double val, int layout, int size) {
  AudioGraph *graph = _graph;

  Node *node = allocate_node_in_graph(graph, 0);

  *node = (Node){
      .perform = NULL,
      .node_index = node->node_index,
      .num_inputs = 0,
      // Allocate state memory
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      // Allocate output buffer
      .output =
          (Signal){.layout = layout,
                   .size = size,
                   .buf = allocate_buffer_from_pool(graph, size * layout)},

      .meta = "const_buf",
  };

  for (int i = 0; i < size * layout; i++) {
    node->output.buf[i] = val;
  }
  return node;
}

typedef struct {
  double phase;
} xfade_state;

void *xfade_perform(Node *node, xfade_state *state, Node *inputs[], int nframes,
                    double spf) {}
NodeRef replace_node(double xfade_time, NodeRef a, NodeRef b) {}

// NodeRef set_math(void *math_fn, NodeRef n) {
//   n->node_math = math_fn;
//   return n;
// }
