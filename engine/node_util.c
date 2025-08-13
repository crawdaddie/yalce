#include "./node_util.h"
#include "audio_graph.h"
#include "lib.h"
// #include <stdio.h>
// #include <stdlib.h>

#define max(a, b) a > b ? a : b

#define NODE_BINOP(name, _meta, _perform)                                      \
  NodeRef name(NodeRef input1, NodeRef input2) {                               \
    AudioGraph *graph = _graph;                                                \
    Node *node = allocate_node_in_graph(graph, 0);                             \
    int max_layout = max(input1->output.layout, input2->output.layout);        \
    *node = (Node){                                                            \
        .perform = (perform_func_t)_perform,                                   \
        .node_index = node->node_index,                                        \
        .num_inputs = 2,                                                       \
        .state_size = 0,                                                       \
        .state_offset = graph ? graph->state_memory_size : 0,                  \
        .output = (Signal){.layout = max_layout,                               \
                           .size = BUF_SIZE,                                   \
                           .buf = allocate_buffer_from_pool(                   \
                               graph, max_layout * BUF_SIZE)},                 \
        .meta = _meta,                                                         \
    };                                                                         \
    plug_input_in_graph(0, node, input1);                                      \
    plug_input_in_graph(1, node, input2);                                      \
    return graph_embed(node);                                                  \
  }

#define INVAL(_sig)                                                            \
  ({                                                                           \
    sample_t *val;                                                             \
    if (_sig.size == 1 && _sig.layout == 1) {                                  \
      val = _sig.buf;                                                          \
    } else {                                                                   \
      val = _sig.buf;                                                          \
      _sig.buf += _sig.layout;                                                 \
    }                                                                          \
    val;                                                                       \
  })
// void *mul_perform(Node *node, void *state, Node *inputs[], int nframes,
//                   sample_t spf) {
//   sample_t *out = node->output.buf;
//   int out_layout = node->output.layout;
//
//   // Get input buffers
//   Signal in1 = inputs[0]->output;
//   Signal in2 = inputs[1]->output;
//
//   // Multiply samples
//   for (int i = 0; i < nframes; i++) {
//     sample_t sample = *INVAL(in1) * *INVAL(in2);
//
//     // Write to all channels in output layout
//     for (int j = 0; j < out_layout; j++) {
//       out[i * out_layout + j] = sample;
//     }
//   }
//
//   return node->output.buf;
// }
void *mul_perform(Node *node, void *state, Node *inputs[], int nframes,
                  sample_t spf) {
  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffers
  Signal in1 = inputs[0]->output;
  Signal in2 = inputs[1]->output;

  int in1_layout = in1.layout;
  int in2_layout = in2.layout;

  // Process each sample
  for (int i = 0; i < nframes; i++) {

    // Process each channel in the output layout
    for (int j = 0; j < out_layout; j++) {
      // Get appropriate input samples based on their layouts
      sample_t in1_sample = in1.buf[i * in1_layout + (j % in1_layout)];
      sample_t in2_sample = in2.buf[i * in2_layout + (j % in2_layout)];

      // Multiply and store result
      out[i * out_layout + j] = in1_sample * in2_sample;
    }
  }

  return node->output.buf;
}

NodeRef mul2_node(NodeRef input1, NodeRef input2) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);
  int max_layout = max(input1->output.layout, input2->output.layout);

  *node = (Node){
      .perform = (perform_func_t)mul_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = 0,
      .state_offset = 0,
      // graph->state_memory_size ? ,
      .output = (Signal){.layout = max_layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, max_layout *
                                                                     BUF_SIZE)},
      .meta = "mul",
  };

  plug_input_in_graph(0, node, input1);
  plug_input_in_graph(1, node, input2);

  return graph_embed(node);
};

void *sum_perform(Node *node, void *state, Node *inputs[], int nframes,
                  sample_t spf) {
  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffers
  Signal in1 = inputs[0]->output;
  Signal in2 = inputs[1]->output;

  int in1_layout = in1.layout;
  int in2_layout = in2.layout;

  // Process each sample
  for (int i = 0; i < nframes; i++) {
    // Process each channel in the output layout
    for (int j = 0; j < out_layout; j++) {
      // Get appropriate input samples based on their layouts
      sample_t in1_sample = in1.buf[i * in1_layout + (j % in1_layout)];
      sample_t in2_sample = in2.buf[i * in2_layout + (j % in2_layout)];

      // Multiply and store result
      out[i * out_layout + j] = in1_sample + in2_sample;
    }
  }

  return node->output.buf;
}

NodeRef sum2_node(NodeRef input1, NodeRef input2) {
  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);

  int max_layout = max(input1->output.layout, input2->output.layout);
  *node = (Node){
      .perform = (perform_func_t)sum_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = 0,
      .state_offset = 0,
      .output = (Signal){.layout = max_layout,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, max_layout *
                                                                     BUF_SIZE)},
      .meta = "sum",
  };
  // node->connections[0].source_node_index = input1->node_index;
  plug_input_in_graph(0, node, input1);
  // node->connections[1].source_node_index = input2->node_index;
  plug_input_in_graph(1, node, input2);
  return graph_embed(node);
}

void *sub_perform(Node *node, void *state, Node *inputs[], int nframes,
                  sample_t spf) {
  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffers
  Signal in1 = inputs[0]->output;
  Signal in2 = inputs[1]->output;

  int in1_layout = in1.layout;
  int in2_layout = in2.layout;

  // Process each sample
  for (int i = 0; i < nframes; i++) {
    // Process each channel in the output layout
    for (int j = 0; j < out_layout; j++) {
      // Get appropriate input samples based on their layouts
      sample_t in1_sample = in1.buf[i * in1_layout + (j % in1_layout)];
      sample_t in2_sample = in2.buf[i * in2_layout + (j % in2_layout)];

      // Multiply and store result
      out[i * out_layout + j] = in1_sample - in2_sample;
    }
  }

  return node->output.buf;
}

NODE_BINOP(sub2_node, "sub", sub_perform)

// void *mod_perform(Node *node, void *state, Node *inputs[], int nframes,
//                   sample_t spf) {
//   sample_t *out = node->output.buf;
//   int out_layout = node->output.layout;
//   // Get input buffers
//   Signal in1 = inputs[0]->output;
//   Signal in2 = inputs[1]->output;
//
//   sample_t *out_ptr = out;
//   while (nframes--) {
//     sample_t sample = fmod(*INVAL(in1), *INVAL(in2));
//
//     // Write to all channels in output layout
//     for (int i = 0; i < out_layout; i++) {
//       *out_ptr++ = sample;
//     }
//   }
//
//   return node->output.buf;
// }
//
void *mod_perform(Node *node, void *state, Node *inputs[], int nframes,
                  sample_t spf) {
  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffers
  Signal in1 = inputs[0]->output;
  Signal in2 = inputs[1]->output;

  int in1_layout = in1.layout;
  int in2_layout = in2.layout;

  // Process each sample
  for (int i = 0; i < nframes; i++) {
    // Process each channel in the output layout
    for (int j = 0; j < out_layout; j++) {
      // Get appropriate input samples based on their layouts
      sample_t in1_sample = in1.buf[i * in1_layout + (j % in1_layout)];
      sample_t in2_sample = in2.buf[i * in2_layout + (j % in2_layout)];

      // Multiply and store result
      out[i * out_layout + j] = fmodf(in1_sample, in2_sample);
    }
  }

  return node->output.buf;
}

NODE_BINOP(mod2_node, "mod", mod_perform)
static inline sample_t __min(sample_t a, sample_t b) { return a <= b ? a : b; }

// void *div_perform(Node *node, void *state, Node *inputs[], int nframes,
//                   sample_t spf) {
//   sample_t *out = node->output.buf;
//   int out_layout = node->output.layout;
//   // Get input buffers
//   Signal in1 = inputs[0]->output;
//   Signal in2 = inputs[1]->output;
//
//   sample_t *out_ptr = out;
//   while (nframes--) {
//     sample_t sample = (*INVAL(in1) / __min(*INVAL(in2), 0.0001));
//
//     // Write to all channels in output layout
//     for (int i = 0; i < out_layout; i++) {
//       *out_ptr++ = sample;
//     }
//   }
//
//   return node->output.buf;
// }
//
void *div_perform(Node *node, void *state, Node *inputs[], int nframes,
                  sample_t spf) {
  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffers
  Signal in1 = inputs[0]->output;
  Signal in2 = inputs[1]->output;

  int in1_layout = in1.layout;
  int in2_layout = in2.layout;

  // Process each sample
  for (int i = 0; i < nframes; i++) {
    // Process each channel in the output layout
    for (int j = 0; j < out_layout; j++) {
      // Get appropriate input samples based on their layouts
      sample_t in1_sample = in1.buf[i * in1_layout + (j % in1_layout)];
      sample_t in2_sample = in2.buf[i * in2_layout + (j % in2_layout)];

      // Multiply and store result
      out[i * out_layout + j] = in1_sample / in2_sample;
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
    node->output.buf[i] = (sample_t)val;
    // printf("const node val %f\n", node->output.buf[i]);
  }
  return graph_embed(node);
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
    node->output.buf[i] = (sample_t)val;
  }
  return graph_embed(node);
}

typedef struct {
  sample_t phase;
} xfade_state;

void *xfade_perform(Node *node, xfade_state *state, Node *inputs[], int nframes,
                    sample_t spf) {}
NodeRef replace_node(sample_t xfade_time, NodeRef a, NodeRef b) {}

void *stereo_perform(Node *node, void *state, Node *inputs[], int nframes,
                     sample_t spf) {
  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;

  sample_t *in = inputs[0]->output.buf;

  // Process each sample
  for (int i = 0; i < nframes; i++) {
    *out = *in;
    out++;
    *out = *in;
    out++;
    in++;
  }

  return node->output.buf;
}
NodeRef stereo_node(NodeRef input) {

  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);

  *node = (Node){
      .perform = (perform_func_t)stereo_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      .output = (Signal){.layout = 2,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, 2 * BUF_SIZE)},
      .meta = "stereo",
  };
  plug_input_in_graph(0, node, input);
  return graph_embed(node);
}

void *panner_perform(Node *node, void *state, Node *inputs[], int nframes,
                     sample_t spf) {

  sample_t *out = node->output.buf;
  int out_layout = node->output.layout;

  sample_t *in = inputs[0]->output.buf;
  sample_t *pan_ = inputs[1]->output.buf;

  // Process each sample
  for (int i = 0; i < nframes; i++) {
    sample_t pan = *pan_;
    pan_++;
    *out = *in * (1. - (pan * 0.5 + 0.5));
    out++;
    *out = *in * (pan * 0.5 + 0.5);
    out++;
    in++;
  }

  return node->output.buf;
}
NodeRef pan_node(NodeRef pan, NodeRef input) {

  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);

  *node = (Node){
      .perform = (perform_func_t)panner_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      .output = (Signal){.layout = 2,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, 2 * BUF_SIZE)},
      .meta = "stereo",
  };

  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, pan);
  return graph_embed(node);
}
typedef struct sah_state {
  sample_t current_val;
  sample_t prev_trig;
} sah_state;

void *sah_perform(Node *node, sah_state *state, Node *inputs[], int nframes,
                  sample_t spf) {

  sample_t *out = node->output.buf;
  sample_t *input_ = inputs[0]->output.buf;
  sample_t *trig_ = inputs[1]->output.buf;
  while (nframes--) {
    sample_t in = *input_;
    input_++;
    sample_t trig = *trig_;
    trig_++;

    if (trig > 0.5 && state->prev_trig <= 0.5) {
      state->current_val = in;
    }

    *out = state->current_val;
    out++;
    state->prev_trig = trig;
  }
}

NodeRef sah_node(NodeRef trig, NodeRef input) {

  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, 0);

  *node = (Node){
      .perform = (perform_func_t)sah_perform,
      .node_index = node->node_index,
      .num_inputs = 2,
      .state_size = 0,
      .state_offset = graph ? graph->state_memory_size : 0,
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "sah",
  };
  plug_input_in_graph(0, node, input);
  plug_input_in_graph(1, node, trig);
  return graph_embed(node);
}

// NodeRef set_math(void *math_fn, NodeRef n) {
//   n->node_math = math_fn;
//   return n;
// }
//
NodeRef empty_synth() { return NULL; }
