#include "./node_util.h"
#include "audio_graph.h"
#include "lib.h"
#include <stdio.h>

#define NODE_BINOP(name, _meta, _perform)                                      \
  NodeRef name(NodeRef input1, NodeRef input2) {                               \
    AudioGraph *graph = _graph;                                                \
    Node *node = allocate_node_in_graph(graph);                                \
    *node = (Node){                                                            \
        .perform = (perform_func_t)_perform,                                   \
        .node_index = node->node_index,                                        \
        .num_inputs = 2,                                                       \
        .state_size = 0,                                                       \
        .state_offset = graph->state_memory_size,                              \
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

void *__mul_perform(Node *node, void *state, Node *inputs[], int nframes,
                    double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffers
  double *in1 = inputs[0]->output.buf;
  double *in2 = inputs[1]->output.buf;

  // Multiply samples
  double *out_ptr = out;
  while (nframes--) {
    double sample = (*in1++) * (*in2++);
    // Write to all channels in output layout
    for (int i = 0; i < out_layout; i++) {
      *out_ptr++ = sample;
    }
  }

  return node->output.buf;
}
void *mul_perform(Node *node, void *state, Node *inputs[], int nframes,
                  double spf) {
  double *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffers
  double *in1 = inputs[0]->output.buf;
  double *in2 = inputs[1]->output.buf;

  // Multiply samples
  for (int i = 0; i < nframes; i++) {
    double sample = in1[i] * in2[i];

    // Write to all channels in output layout
    for (int j = 0; j < out_layout; j++) {
      out[i * out_layout + j] = sample;
    }
  }

  return node->output.buf;
}

NodeRef mul2_node(NodeRef input1, NodeRef input2) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph);

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
  double *in1 = inputs[0]->output.buf;
  double *in2 = inputs[1]->output.buf;

  double *out_ptr = out;
  while (nframes--) {
    double sample = (*in1++) + (*in2++);

    // Write to all channels in output layout
    for (int i = 0; i < out_layout; i++) {
      *out_ptr++ = sample;
    }
  }

  return node->output.buf;
}

NodeRef sum2_node(NodeRef input1, NodeRef input2) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph);
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
  double *in1 = inputs[0]->output.buf;
  double *in2 = inputs[1]->output.buf;

  double *out_ptr = out;
  while (nframes--) {
    double sample = (*in1++) - (*in2++);

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
  double *in1 = inputs[0]->output.buf;
  double *in2 = inputs[1]->output.buf;

  double *out_ptr = out;
  while (nframes--) {
    double sample = fmod((*in1++), (*in2++));

    // Write to all channels in output layout
    for (int i = 0; i < out_layout; i++) {
      *out_ptr++ = sample;
    }
  }

  return node->output.buf;
}

NODE_BINOP(mod2_node, "mod", mod_perform)

Node *const_sig(double val) {
  // printf("const sig node %f\n", val);
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph);

  // Initialize node
  *node = (Node){
      .perform = NULL,
      .node_index = node->node_index,
      .num_inputs = 0,
      // Allocate state memory
      .state_size = 0,
      .state_offset = graph->state_memory_size,
      // Allocate output buffer
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
