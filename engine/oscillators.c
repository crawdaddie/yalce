#include "./oscillators.h"
#include "./common.h"
#include "signals.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SQ_TABSIZE (1 << 11)
static double sq_table[SQ_TABSIZE] = {0};
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

typedef struct sin_state {
  double phase;
} sin_state;

#define SIN_TABSIZE (1 << 11)
static double sin_table[SIN_TABSIZE];

void maketable_sin(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {
    double val = sin(phase);
    sin_table[i] = val;
    phase += phsinc;
  }
}

void *sin_perform(Node *node, int nframes, double spf) {
  sin_state *state = get_node_state(node);
  Signal in = *get_node_input(node, 0);

  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double *freq;

  int out_layout = node->out.layout;
  double *out = node->out.buf;
  while (nframes--) {
    freq = get_val(&in);
    d_index = state->phase * SIN_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index % SIN_TABSIZE];
    b = sin_table[(index + 1) % SIN_TABSIZE];

    sample = (1.0 - frac) * a + (frac * b);
    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }
    state->phase = fmod((*freq) * spf + (state->phase), 1.0);
  }
  return (char *)node + node->node_size;
}

Node *sin_node(Signal *input) {
  Node *node = node_new();
  sin_state *state = (sin_state *)state_new(sizeof(sin_state));

  int in_offset = (char *)input - (char *)node;
  *node = (Node){.num_ins = 1,
                 .input_offsets = {in_offset},
                 .node_size = sizeof(Node) + sizeof(sin_state),
                 .out = {.size = BUF_SIZE,
                         .layout = 1,
                         .buf = malloc(sizeof(double) * BUF_SIZE)},
                 .node_perform = (perform_func_t)sin_perform,
                 .next = NULL};

  // sin_state *state = node_alloc(mem, sizeof(sin_state));
  *state = (sin_state){0.};

  return node;
}
