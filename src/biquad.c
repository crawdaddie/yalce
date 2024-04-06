#include "biquad.h"
#include "common.h"
#include "ctx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static node_perform biquad_perform(Node *node, int nframes, double spf) {
  double *out = node->out->buf;
  int out_layout = node->out->layout;
  double *in = node->ins[0]->buf;
  int in_layout = node->ins[0]->layout;

  biquad_state *filter = node->state;

  while (nframes--) {
    double input = *in;
    double output = filter->b0 * input + filter->b1 * filter->x1 +
                    filter->b2 * filter->x2 - filter->a1 * filter->y1 -
                    filter->a2 * filter->y2;

    // Update delay elements
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;
    *out = output;

    in++;
    out++;
  }
}

void set_biquad_lp_coefficients(double freq, double res, int fs,
                                biquad_state *state);

static node_perform biquad_dyn_perform(Node *node, int nframes, double spf) {
  double *out = node->out->buf;
  int out_layout = node->out->layout;
  double *in = node->ins[0]->buf;

  int in_layout = node->ins[0]->layout;

  double *freq_in = node->ins[1]->buf;
  double *res_in = node->ins[2]->buf;

  biquad_state *filter = node->state;

  double prev_freq = *freq_in;
  double prev_res = *res_in;
  set_biquad_lp_coefficients(*freq_in, *res_in, (int)(1 / spf), filter);

  while (nframes--) {
    double freq = *freq_in;
    double res = *res_in;
    if (freq != prev_freq) {
      set_biquad_lp_coefficients(freq, res, (int)(1 / spf), filter);
    }
    double input = *in;

    double output = filter->b0 * input + filter->b1 * filter->x1 +
                    filter->b2 * filter->x2 - filter->a1 * filter->y1 -
                    filter->a2 * filter->y2;

    // Update delay elements
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;
    *out = output;

    in++;
    prev_freq = freq;
    freq_in++;

    prev_res = res;
    res_in++;

    out++;
  }
}

Node *biquad_dyn_node(double freq, double res, Node *in) {
  biquad_state *state = malloc(sizeof(biquad_state));

  Node *n = node_new(state, (node_perform *)biquad_dyn_perform, NULL,
                     get_sig(in->out->layout));

  n->ins = malloc(sizeof(Signal *) * 3);
  n->ins[0] = in->out;
  n->ins[1] = get_sig_default(1, freq);
  n->ins[2] = get_sig_default(1, res);
  return n;
}

Node *biquad_node(Node *in) {
  biquad_state *state = malloc(sizeof(biquad_state));

  Node *n = node_new(state, (node_perform *)biquad_perform, NULL,
                     get_sig(in->out->layout));

  n->ins = malloc(sizeof(Signal *));
  n->ins[0] = in->out;
  return n;
}

// Initialize filter coefficients and state variables
void set_biquad_filter_state(biquad_state *filter, double b0, double b1,
                             double b2, double a1, double a2) {
  filter->b0 = b0;
  filter->b1 = b1;
  filter->b2 = b2;
  filter->a1 = a1;
  filter->a2 = a2;
}

// Initialize filter coefficients and state variables
void zero_biquad_filter_state(biquad_state *filter) {

  filter->x1 = 0.0;
  filter->x2 = 0.0;
  filter->y1 = 0.0;
  filter->y2 = 0.0;
}

void set_biquad_lp_coefficients(double freq, double res, int fs,
                                biquad_state *state) {

  // Define filter coefficients (example: 1 kHz low-pass filter with Q factor of
  // 0.707)
  double fc = freq; // Cutoff frequency (Hz)
  double w0 = 2.0 * PI * fc / fs;
  double Q = res; // Quality factor

  // Compute filter coefficients
  double A = sin(w0) / (2 * Q);
  double B = 0.0;
  double C = cos(w0);
  double b0 = (1 - C) / 2;
  double b1 = 1 - C;
  double b2 = (1 - C) / 2;
  double a0 = 1 + A;
  double a1 = -2 * C;
  double a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

Node *biquad_lp_node(double freq, double res, Node *in) {
  Node *node = biquad_node(in);
  set_biquad_lp_coefficients(freq, res, ctx_sample_rate(), node->state);
  zero_biquad_filter_state(node->state);
  return node;
}

void set_biquad_hp_coefficients(double freq, double res, int fs,
                                biquad_state *state) {
  // Define filter coefficients for a resonant high-pass filter (example: 1 kHz
  // high-pass filter with Q factor of 0.707)
  double fc = freq; // Cutoff frequency (Hz)
  double w0 = 2.0 * PI * fc / fs;
  double Q = res; // Quality factor

  // Compute filter coefficients
  double A = sin(w0) / (2 * Q);
  double B = 0.0;
  double C = cos(w0);
  double b0 = (1 + C) / 2;
  double b1 = -(1 + C);
  double b2 = (1 + C) / 2;
  double a0 = 1 + A;
  double a1 = -2 * C;
  double a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

Node *biquad_hp_node(double freq, double res, Node *in) {
  Node *node = biquad_node(in);
  set_biquad_hp_coefficients(freq, res, ctx_sample_rate(), node->state);
  zero_biquad_filter_state(node->state);
  return node;
}
void set_biquad_bp_coefficients(double freq, double res, int fs,
                                biquad_state *state) {
  // Define filter coefficients for a resonant bandpass filter (example: 1 kHz
  // center frequency with Q factor of 1.0)
  double fc = freq; // Center frequency (Hz)
  double w0 = 2.0 * PI * fc / fs;
  double Q = res; // Q factor for resonance

  // Compute filter coefficients
  double A = sin(w0) / (2 * Q);
  double B = 0.0;
  double C = cos(w0);
  double b0 = A;
  double b1 = 0.0;
  double b2 = -A;
  double a0 = 1 + A;
  double a1 = -2 * C;
  double a2 = 1 - A;

  // Initialize filter
  set_biquad_filter_state(state, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

Node *biquad_bp_node(double freq, double res, Node *in) {
  Node *node = biquad_node(in);
  set_biquad_bp_coefficients(freq, res, ctx_sample_rate(), node->state);
  zero_biquad_filter_state(node->state);
  return node;
}

Node *biquad_lp_dyn_node(double freq, double res, Node *in) {
  Node *node = biquad_dyn_node(freq, res, in);
  set_biquad_lp_coefficients(freq, res, ctx_sample_rate(), node->state);
  zero_biquad_filter_state(node->state);
  return node;
}
