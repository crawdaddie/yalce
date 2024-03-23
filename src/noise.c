#include "noise.h"
#include <stdlib.h>

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

node_perform noise_perform(Node *node, int nframes, double spf) {
  int layout = node->out->layout;
  double *out = node->out->buf;

  while (nframes--) {
    for (int ch = 0; ch < layout; ch++) {
      *out = random_double();
      out++;
    }
  }
}

Node *white_noise() {
  Node *n = node_new(NULL, (node_perform *)noise_perform, NULL, get_sig(1));
  return n;
}

node_perform lf_noise_perform(Node *node, int nframes, double spf) {
  lf_noise_state *state = node->state;

  double noise_freq = state->freq;
  int trigger_per_ms = (int)(1000 / noise_freq);
  double *out = node->out->buf;
  // Signal in = node->ins;

  while (nframes--) {

    if ((int)(1000 * state->phase) >= trigger_per_ms) {
      state->phase = 0;
      state->target = random_double_range(state->min, state->max);
      // printf("noise target: %f [%f, %f]\n", data->target, data->min,
      // data->max);
    }

    state->phase += spf;
    *out = state->target;
    out++;
    // in.data++;
  }
}

// ------------------------------------------- NOISE UGENS

Node *lfnoise(double freq, double min, double max) {
  lf_noise_state *state = malloc(sizeof(lf_noise_state));
  state->phase = 0.0;
  state->target = random_double_range(min, max);
  state->min = min;
  state->max = max;
  state->freq = freq;

  return node_new(state, lf_noise_perform, &(Signal){}, get_sig(1));
}

node_perform lf_noise_interp_perform(Node *node, int nframes, double spf) {
  double noise_freq = 5.0;
  int trigger_per_ms = (int)(1000 / noise_freq);

  lf_noise_interp_state *state = node->state;

  // printf("noise: %f %f\n", data->val, data->phase);

  double *out = node->out->buf;

  while (nframes--) {

    if ((int)(1000 * state->phase) >= trigger_per_ms) {
      state->phase = 0;
      state->target = random_double_range(60, 120);
    }

    state->phase += spf;
    *out = state->current;
    state->current =
        state->current + 0.000001 * (state->target - state->current);
    out++;
  }
}

node_perform rand_choice_perform(Node *node, int nframes, double spf) {
  rand_choice_state *state = node->state;

  double noise_freq = state->freq;
  int trigger_per_ms = (int)(1000 / noise_freq);
  double *out = node->out->buf;
  // Signal in = node->ins;

  while (nframes--) {

    if ((int)(1000 * state->phase) >= trigger_per_ms) {
      state->phase = 0;
      int r = rand_int(state->size);
      state->target = state->choices[r];
    }

    state->phase += spf;
    *out = state->target;

    out++;
    // in.data++;
  }
}
Node *rand_choice_node(double freq, int size, double *choices) {
  rand_choice_state *state = malloc(sizeof(rand_choice_state));
  state->phase = 0.0;
  state->target = choices[rand_int(size)];
  state->choices = choices;
  state->size = size;
  state->freq = freq;

  return node_new(state, rand_choice_perform, NULL, get_sig(1));
}
