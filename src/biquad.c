#include "biquad.h"
#include "common.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void set_common_filter_values(biquad_state *state, double fc, double Q) {

  double K_ = tan(PI * fc / 48000);
  double kSqr_ = K_ * K_;
  double denom_ = 1 / (kSqr_ * Q + K_ + Q);
  state->K_ = K_;
  state->kSqr_ = kSqr_;
  state->denom_ = denom_;

  state->a[1] = 2 * Q * (kSqr_ - 1) * denom_;
  state->a[2] = (kSqr_ * Q - K_ + Q) * denom_;
}

static void set_resonance(biquad_state *state, double frequency, double radius,
                          bool normalize) {

  state->a[2] = radius * radius;
  state->a[1] = -2.0 * radius * cos(2. * PI * frequency / 48000.);

  if (normalize) {
    // Use zeros at +- 1 and normalize the filter peak gain.
    state->b[0] = 0.5 - 0.5 * state->a[2];
    state->b[1] = 0.0;
    state->b[2] = -1. * state->b[0];
  } else {
    state->b[0] = 1.0;
    state->b[1] = 0.0;
    state->b[2] = 0.0;
  }
}

static void set_low_pass(biquad_state *state, double fc, double Q) {
  set_common_filter_values(state, fc, Q);

  state->b[0] = state->kSqr_ * Q * state->denom_;
  state->b[1] = 2 * state->b[0];
  state->b[2] = state->b[0];
}

static void set_high_pass(biquad_state *state, double fc, double Q) {
  set_common_filter_values(state, fc, Q);

  state->b[0] = Q * state->denom_;
  state->b[1] = -2 * state->b[0];
  state->b[2] = state->b[0];
}
//
static void set_band_pass(biquad_state *state, double fc, double Q) {
  set_common_filter_values(state, fc, Q);

  state->b[0] = state->K_ * state->denom_;
  state->b[1] = 0.0;
  state->b[2] = -1 * state->b[0];
}
//
// void BiQuad ::setBandReject(double fc, double Q) {
//   set_common_filter_values(fc, Q);
//
//   b_[0] = Q * (kSqr_ + 1) * denom_;
//   b_[1] = 2 * Q * (kSqr_ - 1) * denom_;
//   b_[2] = b_[0];
// }
//
// void BiQuad ::setAllPass(double fc, double Q) {
//   set_common_filter_values(fc, Q);
//
//   b_[0] = a_[2];
//   b_[1] = a_[1];
//   b_[2] = 1;
// }

static node_perform biquad_perform(Node *node, int nframes, double spf) {
  double *out = node->out->buf;
  int out_layout = node->out->layout;
  double *in = node->ins[0]->buf;
  int in_layout = node->ins[0]->layout;

  biquad_state *state = node->state;
  double *inputs_ = state->inputs_;
  double *outputs_ = state->outputs_;
  double *a_ = state->a;
  double *b_ = state->b;

  while (nframes--) {

    state->inputs_[0] = state->gain * *in;
    *out = b_[0] * state->inputs_[0] + b_[1] * state->inputs_[1] +
           b_[2] * state->inputs_[2];
    *out -= a_[2] * state->outputs_[2] + a_[1] * state->outputs_[1];
    state->inputs_[2] = state->inputs_[1];
    state->inputs_[1] = state->inputs_[0];
    state->outputs_[2] = state->outputs_[1];
    state->outputs_[1] = *out;

    in++;
    out++;
  }
}

Node *biquad_node(Node *in) {
  biquad_state *state = malloc(sizeof(biquad_state));
  state->gain = 1.0;
  state->K_ = 0.0;
  state->kSqr_ = 0.0;
  state->denom_ = 1.0;
  state->b[0] = 1.0;
  state->a[0] = 1.0;
  // state->gain = 2.0;

  // double freq = 100.;
  // set_low_pass(state, freq, 0.8);
  // set_resonance(state, freq, 0.8, true);
  Node *n = node_new(state, (node_perform *)biquad_perform, NULL,
                     get_sig(in->out->layout));
  n->ins = malloc(sizeof(Signal *));
  n->ins[0] = in->out;
  return n;
}

Node *biquad_lp_node(double freq, double res, Node *in) {
  Node *node = biquad_node(in);
  set_low_pass(node->state, freq, 1.0);
  set_resonance(node->state, freq, res, true);
  return node;
}
