#include "oscillators.h"
#include "common.h"
#include "node.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SQ_TABSIZE (1 << 11)
static double sq_table[SQ_TABSIZE] = {0};

// void maketable_sq(void) {
//   double phase = 0.0;
//   double phsinc = (2. * PI) / SIN_TABSIZE;
//
//   for (int i = 0; i < SIN_TABSIZE; i++) {
//     // for (int harm = 1; harm < 5; harm++) {
//     // }
//     double val = sin(phase);
//
//     sin_table[i] += val;
//     printf("%f\n", sin_table[i]);
//
//     phase += phsinc;
//   }
// }
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

double sq_sample(double phase) {
  phase = fmod(phase, 1.0);

  double d_index;
  int index;
  double frac, a, b;
  double sample;

  d_index = phase * SQ_TABSIZE;
  index = (int)d_index;
  frac = d_index - index;

  a = sq_table[index];
  b = sq_table[(index + 1) % SQ_TABSIZE];

  sample = (1.0 - frac) * a + (frac * b);
  return sample;
  // *out = sample;
}

node_perform sq_perform(Node *node, int nframes, double spf) {

  sq_state *state = (sq_state *)node->state;

  double *input = node->ins[0];
  double *out = node->output_buf;

  while (nframes--) {
    *out = sq_sample(state->phase);
    // *out += sq_sample(state->phase * 1.5, *input);
    // *out /= 2.;
    // printf("sq perform *out %f\n", *out);
    state->phase = fmod(*input * spf + (state->phase), 1.0);
    out++;
    input++;
  }
}

#define SIN_TABSIZE (1 << 11)
static double sin_table[SIN_TABSIZE];

void maketable_sin(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {
    double val = sin(phase);

    // printf("%f\n", val);
    sin_table[i] = val;
    phase += phsinc;
  }
}

node_perform sin_perform(Node *node, int nframes, double spf) {

  sin_state *state = (sin_state *)node->state;
  double *input = node->ins[0];
  double *out = node->output_buf;

  // printf("read freq input %p\n", input);

  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double freq;

  while (nframes--) {
    freq = *input;
    d_index = state->phase * SIN_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index];
    b = sin_table[(index + 1) % SIN_TABSIZE];

    sample = (1.0 - frac) * a + (frac * b);
    *out = sample;
    state->phase = fmod(freq * spf + (state->phase), 1.0);

    out++;
    input++;
  }
}
