#include "impulse.h"
#include "common.h"
#include "ctx.h"
#include "node.h"
#include "signal.h"
#include <stdio.h>
#include <stdlib.h>
Node *trig_node(Node *in) {}

node_perform trig_perform(Node *node, int nframes, double spf) {
  double *out = node->out->buf;
  double *in = node->ins[0]->buf;
  impulse_state *state = node->state;
  while (nframes--) {
    if (state->counter <= 0.0) {
      state->counter = (1.0 / spf) / *in;
      *out = 1.0;
    } else {
      *out = 0.0;
    }
    state->counter = state->counter - 1.0;
    out++;
    in++;
  }
}

Node *trig_node_const(double freq) {
  impulse_state *state = malloc(sizeof(impulse_state));
  state->counter = ctx_sample_rate() / freq;
  Node *n = node_new(state, (node_perform *)trig_perform, NULL, get_sig(1));
  n->ins = malloc(sizeof(Signal *) * 1);
  n->ins[0] = get_sig_default(1, freq);
  return n;
}

node_perform dust_perform_const(Node *node, int nframes, double spf) {
  double *out = node->out->buf;
  dust_state *state = node->state;
  while (nframes--) {
    if (state->counter <= 0.0) {
      state->counter =
          (1.0 / spf) / random_double_range(state->min_freq, state->max_freq);
      *out = 1.0;
    } else {
      *out = 0.0;
    }
    state->counter = state->counter - 1.0;
    out++;
  }
}

node_perform dust_perform(Node *node, int nframes, double spf) {
  double *out = node->out->buf;
  double *min_freq = node->ins[0]->buf;
  double *max_freq = node->ins[1l]->buf;
  impulse_state *state = node->state;
  while (nframes--) {
    if (state->counter <= 0.0) {
      state->counter = (1.0 / spf) / random_double_range(*min_freq, *max_freq);
      *out = 1.0;
    } else {
      *out = 0.0;
    }
    state->counter = state->counter - 1.0;
    out++;
    min_freq++;
    max_freq++;
  }
}

Node *dust_node_const(double min_freq, double max_freq) {
  dust_state *state = malloc(sizeof(dust_state));
  state->counter = ctx_sample_rate() / random_double_range(min_freq, max_freq);
  state->min_freq = min_freq;
  state->max_freq = max_freq;
  Node *n =
      node_new(state, (node_perform *)dust_perform_const, NULL, get_sig(1));
  n->ins = malloc(sizeof(Signal *) * 1);
  // n->ins[0] = get_sig_default(1, min_freq);
  // n->ins[1] = get_sig_default(1, max_freq);
  return n;
}

Node *dust_node(double min_freq, double max_freq) {
  impulse_state *state = malloc(sizeof(dust_state));
  state->counter = ctx_sample_rate() / random_double_range(min_freq, max_freq);

  Node *n =
      node_new(state, (node_perform *)dust_perform_const, NULL, get_sig(1));
  n->ins = malloc(sizeof(Signal *) * 2);
  n->ins[0] = get_sig_default(1, min_freq);
  n->ins[1] = get_sig_default(1, max_freq);
  return n;
}

node_perform blit_perform(Node *node, int nframes, double spf) {
  blit_state *state = node->state;
  double *in = node->ins[0]->buf;
  double *out = node->out->buf;
  double tmp, denominator;
  double freq;

  while (nframes--) {
    freq = *in;
    state->p = (1.0 / spf) / freq;
    state->rate = PI / state->p;

    denominator = sin(state->phase);
    tmp = denominator;
    if (denominator <= EPSILON)
      tmp = 1.0;
    else {
      tmp = sin(state->m * state->phase);
      tmp /= state->m * denominator;
    }

    state->phase += state->rate;
    if (state->phase >= PI)
      state->phase -= PI;

    *out = tmp;
    out++;
    in++;
  }
}
Node *blit_node(double freq, int num_harmonics) {
  blit_state *state = malloc(sizeof(blit_state));
  state->phase = 0.0;
  state->p = ctx_sample_rate() / freq;
  state->m = 2 * num_harmonics + 1;
  state->rate = PI / state->p;

  Node *n = node_new(state, (node_perform *)blit_perform, NULL, get_sig(1));
  n->ins = malloc(sizeof(Signal *) * 1);
  n->ins[0] = get_sig_default(1, freq);
  return n;
}

Node *windowed_impulse_node(double pulse_freq, double freq, int num_harmonics) {
}
