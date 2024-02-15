#include "node.h"
#include "common.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

double random_double() {
  // Generate a random integer between 0 and RAND_MAX
  int rand_int = rand();
  // Scale the integer to a double between 0 and 1
  double rand_double = (double)rand_int / RAND_MAX;
  // Scale and shift the double to be between -1 and 1
  rand_double = rand_double * 2 - 1;
  return rand_double;
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

node_perform noise_perform(Node *node, int nframes, double spf) {
  Signal out = node->out;

  while (nframes--) {
    for (int ch = 0; ch < out.layout; ch++) {
      *out.data = random_double();
      out.data++;
    }
  }
}

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
  sin_data *data = node->data;
  Signal out = node->out;

  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double freq = 200.0;

  for (int f = 0; f < nframes; f++) {
    d_index = data->phase * SIN_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index];
    b = sin_table[(index + 1) % SIN_TABSIZE];

    sample = (1.0 - frac) * a + (frac * b);

    data->phase = fmod(freq * spf + (data->phase), 1.0);

    for (int f = 0; f < nframes; f++) {
      double *dest = get_sig_ptr(out, f, 0);
      *dest = sample;

      // dest = get_sig_ptr(out, f, 1);
      // *dest = sample;
    }
  }
}

// ----------------------------- SQUARE WAVE OSCILLATORS
//
double sq_sample(double phase, double freq) {
  return scale_val_2(fmod(phase * freq * 2.0 * PI, 2 * PI) > PI, -1, 1);
}

node_perform sq_perform(Node *node, int nframes, double spf) {
  sq_data *data = node->data;

  Signal out = node->out;
  Signal in = node->ins;

  while (nframes--) {
    double samp = sq_sample(data->phase, *in.data) +
                  sq_sample(data->phase, *in.data * 1.01);
    samp /= 2;

    data->phase += spf;
    *out.data = samp;
    out.data++;
    in.data++;
  }
}

node_perform lf_noise_perform(Node *node, int nframes, double spf) {
  lf_noise_data *data = node->data;
  double noise_freq = 8.0;
  int trigger_per_ms = (int)(1000 / noise_freq);

  Signal out = node->out;

  while (nframes--) {

    if ((int)(1000 * data->phase) >= trigger_per_ms) {
      data->phase = 0;
      data->target = random_double_range(data->min, data->max);
      // printf("noise target: %f [%f, %f]\n", data->target, data->min,
      // data->max);
    }

    data->phase += spf;
    *out.data = data->target;
    out.data++;
  }
}

node_perform lf_noise_interp_perform(Node *node, int nframes, double spf) {
  double noise_freq = 5.0;
  int trigger_per_ms = (int)(1000 / noise_freq);

  lf_noise_interp_data *data = node->data;

  // printf("noise: %f %f\n", data->val, data->phase);

  Signal out = node->out;

  while (nframes--) {

    // printf("noise: %f %d thresh: %d\n", data->target, (int)data->phase,
    // trigger_per_ms);
    if ((int)(1000 * data->phase) >= trigger_per_ms) {
      data->phase = 0;
      data->target = random_double_range(60, 120);
      // printf("noise: %f\n", data->target);
    }

    data->phase += spf;
    *out.data = data->current;
    data->current = data->current + 0.000001 * (data->target - data->current);
    out.data++;
  }
}

// ------------------------------ SIGNAL / BUFFER ALLOC
//
static double buf_pool[BUF_SIZE * LAYOUT_CHANNELS * 100];
static double *buf_ptr = buf_pool;

void init_sig_ptrs() { buf_ptr = buf_pool; }

Signal get_sig(int layout) {
  Signal sig = {
      .data = buf_ptr,
      .layout = layout,
      .size = BUF_SIZE,
  };
  buf_ptr += BUF_SIZE * layout;
  return sig;
}
// ----------------------------- Node alloc
Node *node_new(void *data, node_perform *perform, Signal ins, Signal out) {
  Node *node = malloc(sizeof(Node));
  node->data = data;
  node->ins = ins;
  node->out = out;
  node->perform = (node_perform)perform;
  return node;
}
