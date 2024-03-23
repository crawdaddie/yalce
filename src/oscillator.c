
#include "oscillator.h"
#include "common.h"
#include "ctx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// ----------------------------- SINE WAVE OSCILLATORS
//
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

node_perform sine_perform(Node *node, int nframes, double spf) {
  sin_state *state = node->state;
  double *input = node->ins[0]->buf;

  // printf("read freq input %p\n", input);

  double *out = node->out->buf;

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

Node *sine(double freq) {

  sin_state *state = malloc(sizeof(sin_state));
  state->phase = 0.0;

  Signal *in = get_sig(1);

  Node *s = node_new(state, (node_perform *)sine_perform, NULL, get_sig(1));
  s->ins = malloc(sizeof(Signal *));
  s->ins[0] = get_sig_default(1, freq);

  return s;
}

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
  double phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {

    for (int harm = 1; harm < SIN_TABSIZE / 2;
         harm += 2) { // summing odd frequencies

      // for (int harm = 1; harm < 100; harm += 2) { // summing odd frequencies
      double val = sin(phase * harm) / harm; // sinewave of different frequency
      sq_table[i] += val;
    }
    // printf("%f\n", sq_table[i]);
    phase += phsinc;
  }
}

// ----------------------------- SQUARE WAVE OSCILLATORS
//

// node_perform sq_perform_(Node *node, int nframes, double spf) {
//   sq_state *state = node->state;
//
//   double *out = node->out->buf;
//   double *input = node->ins[0]->buf;
//
//   while (nframes--) {
//     double samp = sq_sample(state->phase, *input) +
//                   sq_sample(state->phase, *input * 1.01);
//     samp /= 2;
//
//     state->phase += spf;
//     *out = samp;
//     out++;
//     input++;
//   }
// }

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
  sq_state *state = node->state;
  double *input = node->ins[0]->buf;
  double *out = node->out->buf;

  while (nframes--) {
    *out = sq_sample(state->phase);
    // *out += sq_sample(state->phase * 1.5, *input);
    // *out /= 2.;
    state->phase = fmod(*input * spf + (state->phase), 1.0);
    out++;
    input++;
  }
}
// static double sq_sample(double phase, double freq) {
//   return scale_val_2(fmod(phase * freq * 2.0 * PI, 2 * PI) > PI, -1, 1);
// }
//
// node_perform sq_perform(Node *node, int nframes, double spf) {
//   sq_state *state = node->state;
//
//   double *out = node->out->buf;
//   double *in = (*node->ins)->buf;
//
//   while (nframes--) {
//     double samp = sq_sample(state->phase, *in);
//
//     state->phase += spf;
//     *out = samp;
//     out++;
//     in++;
//   }
// }
static char *sq_name = "square";
Node *sq_node(double freq) {
  sq_state *state = malloc(sizeof(sq_state));
  state->phase = 0.0;

  Node *s = node_new(state, (node_perform *)sq_perform, NULL, get_sig(1));
  s->ins = malloc(sizeof(Signal *));
  s->ins[0] = get_sig_default(1, freq);
  s->name = sq_name;
  return s;
}
node_perform blsaw_perform(Node *node, int nframes, double spf) {
  blsaw_state *state = node->state;
  double *in = node->ins[0]->buf;
  double *out = node->out->buf;
  double tmp, denominator;
  double freq;
  while (nframes--) {
    freq = *in;

    state->p = (1.0 / spf) / freq;
    state->C2 = 1 / state->p;
    state->rate = PI * state->C2;
    state->a = state->m / state->p;

    denominator = sin(state->phase);
    tmp = denominator;
    if (fabs(denominator) <= EPSILON)
      tmp = state->a;
    else {
      tmp = sin(state->m * state->phase);
      tmp /= state->p * denominator;
    }

    tmp += state->state - state->C2;
    state->state = tmp * 0.995;

    state->phase += state->rate;
    if (state->phase >= PI)
      state->phase -= PI;

    *out = tmp;
    out++;
    in++;
  }
}
Node *blsaw_node(double freq, int num_harmonics) {

  blsaw_state *state = malloc(sizeof(blsaw_state));

  state->p = ctx_sample_rate() / freq;
  state->C2 = 1 / state->p;
  state->rate = PI * state->C2;
  state->m = 2 * num_harmonics + 1;
  state->num_harmonics = num_harmonics;
  state->a = state->m / state->p;

  Node *s = node_new(state, (node_perform *)blsaw_perform, NULL, get_sig(1));
  s->ins = malloc(sizeof(Signal *));
  s->ins[0] = get_sig_default(1, freq);
  return s;
}

node_perform sawsinc_perform(Node *node, int nframes, double spf) {
  sawsinc_state *state = node->state;

  double *base_freq_in = node->ins[0]->buf;
  double *in = node->ins[1]->buf;
  double *out = node->out->buf;
  double tmp, denominator;
  double freq;

  while (nframes--) {
    freq = *in;

    state->p = (1.0 / spf) / freq;
    state->C2 = 1 / state->p;
    state->rate = PI * state->C2;
    state->a = state->m / state->p;

    denominator = sin(state->phase);
    tmp = denominator;
    if (fabs(denominator) <= EPSILON)
      tmp = state->a;
    else {
      tmp = sin(state->m * state->phase);
      tmp /= state->p * denominator;
    }

    tmp += state->state - state->C2;
    state->state = tmp * 0.995;

    state->phase += state->rate;

    state->p = (1.0 / spf) / freq;
    state->C2 = 1 / state->p;
    double base_rate = PI * *base_freq_in / ((1.0 / spf));

    state->base_phase += base_rate;

    if (state->phase >= PI)
      state->phase -= PI;

    if (state->base_phase >= 2 * PI) {
      state->phase = 0.0;
      state->base_phase -= 2 * PI;
    }

    *out = tmp;

    base_freq_in++;
    out++;
    in++;
  }
}

Node *sawsinc_node(double base_freq, double freq, int num_harmonics) {

  blsaw_state *state = malloc(sizeof(blsaw_state));

  state->p = ctx_sample_rate() / freq;
  state->C2 = 1 / state->p;
  state->rate = PI * state->C2;
  state->m = 2 * num_harmonics + 1;
  state->num_harmonics = num_harmonics;
  state->a = state->m / state->p;

  Node *s = node_new(state, (node_perform *)sawsinc_perform, NULL, get_sig(1));
  s->ins = malloc(sizeof(Signal *) * 2);
  s->ins[0] = get_sig_default(1, base_freq);
  s->ins[1] = get_sig_default(1, freq);
  return s;
}
