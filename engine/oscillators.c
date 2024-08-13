#include "oscillators.h"
#include "common.h"
#include "node.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct sq_state {
  double phase;
} sq_state;
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

  double *input = node->ins[0].buf;
  double *out = node->out.buf;

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

Node *sq_node(Signal *freq) {
  sq_state *state = malloc(sizeof(sq_state));
  state->phase = 0.0;

  Node *s = node_new(state, (node_perform *)sq_perform, 1, freq);

  return s;
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

    // printf("%f\n", val);
    sin_table[i] = val;
    phase += phsinc;
  }
}

node_perform sin_perform(Node *node, int nframes, double spf) {

  sin_state *state = (sin_state *)node->state;
  double *input = node->ins[0].buf;
  double *out = node->out.buf;

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

Node *sin_node(Signal *freq) {
  sin_state *state = malloc(sizeof(sin_state));
  state->phase = 0.0;

  Node *s = node_new(state, (node_perform *)sin_perform, 1, freq);
  return s;
}

typedef struct bufplayer_state {
  double phase;
} bufplayer_state;

node_perform bufplayer_perform(Node *node, int nframes, double spf) {

  bufplayer_state *state = (bufplayer_state *)node->state;
  double *buf = node->ins[0].buf;
  int buf_size = node->ins[0].size;

  double *rate = node->ins[1].buf;

  double *out = node->out.buf;

  double d_index, frac, a, b, sample;
  int index;
  while (nframes--) {
    d_index = state->phase * buf_size;
    index = (int)d_index;
    frac = d_index - index;

    a = buf[index];
    b = buf[(index + 1) % buf_size];

    sample = (1.0 - frac) * a + (frac * b);

    state->phase = fmod(state->phase + (*rate) / buf_size, 1.0);

    *out = sample;

    out++;
    rate++;
  }
}

Node *bufplayer_node(Signal *buf, Signal *rate) {
  bufplayer_state *state = malloc(sizeof(bufplayer_state));
  state->phase = 0.0;

  Signal *sigs = malloc(sizeof(Signal) * 2);
  sigs[0].buf = buf->buf;
  sigs[0].size = buf->size;
  sigs[0].layout = buf->layout;

  sigs[1].buf = rate->buf;
  sigs[1].size = rate->size;
  sigs[1].layout = rate->layout;

  Node *bufplayer = node_new(state, (node_perform *)bufplayer_perform, 2, sigs);

  // printf("create bufplayer node %p with buf signal %p\n", bufplayer, buf);
  return bufplayer;
}

double random_double() {
  // Generate a random integer between 0 and RAND_MAX
  int rand_int = rand();
  // Scale the integer to a double between 0 and 1
  double rand_double = (double)rand_int / RAND_MAX;
  // Scale and shift the double to be between -1 and 1
  rand_double = rand_double * 2 - 1;
  return rand_double;
}

int rand_int(int range) {
  // Generate a random integer between 0 and RAND_MAX
  int rand_int = rand();
  // Scale the integer to a double between 0 and 1
  double rand_double = (double)rand_int / RAND_MAX;
  // Scale and shift the double to be between -1 and 1
  rand_double = rand_double * range;
  return (int)rand_double;
}

double random_double_range(double min, double max) {
  // Generate a random integer between 0 and RAND_MAX
  int rand_int = rand();
  // Scale the integer to a double between 0 and 1
  double rand_double = (double)rand_int / RAND_MAX;
  // Scale and shift the double to be between -1 and 1
  rand_double = rand_double * (max - min) + min;
  return rand_double;
}
node_perform white_noise_perform(Node *node, int nframes, double spf) {
  double *out = node->out.buf;
  while (nframes--) {
    *out = random_double_range(-1.0, 1.0);
    out++;
  }
}

Node *white_noise_node() {

  Node *noise = node_new(NULL, (node_perform *)white_noise_perform, 0, NULL);
  return noise;
}

typedef struct brown_noise_state {
  double last;
} brown_noise_state;

node_perform brown_noise_perform(Node *node, int nframes, double spf) {
  double scale = sqrt(spf);
  // double scale = spf;
  brown_noise_state *st = node->state;
  double *out = node->out.buf;

  while (nframes--) {
    double add = scale * random_double_range(-1.0, 1.0);
    st->last += add; 
    // Prevent unbounded drift
    if (st->last > 1.0 || st->last < -1.0) {
      st->last = st->last * 0.999;
    }
    *out = st->last;
    out++;
  }
}

Node *brown_noise_node() {
  brown_noise_state *st = malloc(sizeof(brown_noise_state));
  Node *noise = node_new(st, (node_perform *)brown_noise_perform, 0, NULL);
  return noise;
}

Node *lfnoise_node() {}
