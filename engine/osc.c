#include "./osc.h"
#include "./common.h"
#include "./node.h"
#include "audio_graph.h"
#include "lib.h"
#include <stdio.h>

#define SQ_TABSIZE (1 << 11)
#define SIN_TABSIZE (1 << 11)
double sq_table[SQ_TABSIZE];
void maketable_sq(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SQ_TABSIZE;
  for (int i = 0; i < SQ_TABSIZE; i++) {

    for (int harm = 1; harm < SQ_TABSIZE / 2;
         harm += 2) { // summing odd frequencies

      // for (int harm = 1; harm < 100; harm += 2) { // summing odd frequencies
      double val = sin(phase * harm) / harm; // sinewave of different frequency
      sq_table[i] += val;
    }
    phase += phsinc;
  }
}

double sin_table[SIN_TABSIZE];

void maketable_sin(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {
    double val = sin(phase);
    sin_table[i] = val;
    phase += phsinc;
  }
}

typedef struct sin_state {
  double phase;
} sin_state;

void *sin_perform(Node *node, sin_state *state, Node *inputs[], int nframes,
                  double spf) {

  double *out = node->output.buf;
  int out_layout = node->output.layout;

  // Get input buffer (frequency control) if connected
  double *in = inputs[0]->output.buf;
  double d_index;
  int index = 0;
  double frac, a, b, sample;
  double freq;

  while (nframes--) {
    freq = *in;
    in++;

    d_index = state->phase * (1 << 11);
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index % (1 << 11)];
    b = sin_table[(index + 1) % (1 << 11)];

    sample = (1.0 - frac) * a + (frac * b);

    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }

    state->phase = fmod(state->phase + freq * spf, 1.0);
  }

  return node->output.buf;
}

Node *sin_node(Node *input) {
  printf("create sin node in: %p\n", input);
  AudioGraph *graph = _graph;
  // Find next available slot in nodes array
  Node *node = allocate_node_in_graph(graph);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)sin_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      // Allocate state memory
      .state_size = sizeof(sin_state),
      .state_offset = allocate_state_memory(graph, sizeof(sin_state)),
      // Allocate output buffer
      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "sin",
  };

  // Initialize state
  sin_state *state =
      (sin_state *)(graph->nodes_state_memory + node->state_offset);

  *state = (sin_state){.phase = 0.0};

  if (input) {
    node->connections[0].source_node_index = input->node_index;
  }

  return node;
}
