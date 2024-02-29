#include "delay.h"
#include "common.h"
#include "ctx.h"
#include "node.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// ------------------------------------------- FILTERS
//
//
// Process loop (lowpass):
// out = a0*in - b1*tmp;
// tmp = out;
//
// Coefficient calculation:
// x = exp(-2.0*pi*freq/samplerate);
// a0 = 1.0-x;
// b1 = -x;

static inline void op_lp_perform_tick(op_lp_state *state, double in,
                                      double *out) {
  *out = state->a0 * in - state->b1 * state->mem;
  state->mem = *out;
}

static inline double op_lp_perform_tick_return(op_lp_state *state, double in) {
  double out = state->a0 * in - state->b1 * state->mem;
  state->mem = out;
  return out;
}
node_perform op_lp_perform(Node *node, int nframes, double spf) {

  double *in = node->ins[0]->buf;
  double *out = node->out->buf;
  op_lp_state *state = node->state;
  while (nframes--) {
    op_lp_perform_tick(state, *in, out);
    in++;
    out++;
  }
}

void set_op_lp_params(op_lp_state *state, double freq) {
  int SAMPLE_RATE = ctx_sample_rate();
  double x = exp(-2 * PI * freq / SAMPLE_RATE);
  state->a0 = 1.0 - x;
  state->b1 = -x;
  state->mem = 0.0;
}

Node *op_lp_node(double freq, Node *input) {
  op_lp_state *state = malloc(sizeof(op_lp_state));
  set_op_lp_params(state, freq);
  Node *s = node_new(state, op_lp_perform, NULL, get_sig(1));
  s->ins = &input->out;
  return s;
}

static inline void op_lp_dyn_perform_tick(op_lp_dyn_state *state, double in,
                                          double freq, double *out) {

  int SAMPLE_RATE = 48000;
  double x = exp(-2 * PI * freq / SAMPLE_RATE);
  double a0 = 1.0 - x;
  double b1 = -x;
  *out = a0 * in - b1 * state->mem;
  state->mem = *out;
}

static inline double op_lp_dyn_perform_tick_return(op_lp_dyn_state *state,
                                                   double in, double freq) {

  int SAMPLE_RATE = 48000;
  double x = exp(-2 * PI * freq / SAMPLE_RATE);
  double a0 = 1.0 - x;
  double b1 = -x;
  double out = a0 * in - b1 * state->mem;
  state->mem = out;
  return out;
}
node_perform op_lp_dyn_perform(Node *node, int nframes, double spf) {

  double *in = node->ins[0]->buf;
  double *freq = node->ins[1]->buf;
  double *out = node->out->buf;
  op_lp_state *state = node->state;
  while (nframes--) {
    op_lp_dyn_perform_tick(state, *in, *freq, out);
    in++;
    freq++;
    out++;
  }
}

void set_op_dyn_lp_params(op_lp_dyn_state *state, double freq) {
  int SAMPLE_RATE = ctx_sample_rate();
  double x = exp(-2 * PI * freq / SAMPLE_RATE);
  state->mem = 0.0;
}

Node *op_lp_dyn_node(double freq, Node *input) {
  op_lp_state *state = malloc(sizeof(op_lp_state));
  set_op_lp_params(state, freq);
  Node *s = node_new(state, op_lp_dyn_perform, NULL, get_sig(1));
  s->ins = malloc(sizeof(Signal *) * 2);
  s->ins[0] = input->out;
  s->ins[1] = get_sig_default(1, freq);
  return s;
}

Node *op_lp_node_scalar(double freq, double input) {
  op_lp_state *state = malloc(sizeof(op_lp_state));
  set_op_lp_params(state, freq);
  Node *s = node_new(state, op_lp_perform, NULL, get_sig(1));

  s->ins = malloc(sizeof(Signal *) * 2);
  s->ins[0] = get_sig_default(1, input);
  s->ins[1] = get_sig_default(1, freq);
  return s;
}

node_perform comb_perform(Node *node, int nframes, double spf) {
  double *in = node->ins[0]->buf;
  double *out = node->out->buf;
  comb_state *state = node->state;

  double *write_ptr = (state->buf + state->write_pos);
  double *read_ptr = state->buf + state->read_pos;

  while (nframes--) {
    write_ptr = state->buf + state->write_pos;
    read_ptr = state->buf + state->read_pos;

    *out = *in + *read_ptr;

    *write_ptr = state->fb * (*out);

    state->read_pos = (state->read_pos + 1) % state->buf_size;
    state->write_pos = (state->write_pos + 1) % state->buf_size;
    in++;
    out++;
  }
}

void set_comb_params(comb_state *state, double delay_time,
                     double max_delay_time, double fb) {
  Ctx *ctx = get_audio_ctx();
  int SAMPLE_RATE = ctx->sample_rate;

  if (delay_time >= max_delay_time) {
    printf("Error: cannot set delay time %f longer than the max delay time %f",
           delay_time, max_delay_time);
    return;
  }

  int buf_size = (int)max_delay_time * SAMPLE_RATE;
  state->buf_size = buf_size;
  double *buf = malloc(sizeof(double) * (int)max_delay_time * SAMPLE_RATE);
  double *b = buf;
  while (buf_size--) {
    *b = 0.0;
    b++;
  }

  state->buf = buf;
  state->write_pos = 0;

  int read_pos = state->buf_size - (int)(delay_time * SAMPLE_RATE);
  state->read_pos = read_pos;
  state->fb = fb;
}

Node *comb_node(double delay_time, double max_delay_time, double fb,
                Node *input) {
  comb_state *state = malloc(sizeof(comb_state));
  set_comb_params(state, delay_time, max_delay_time, fb);
  Node *s = node_new(state, comb_perform, NULL, get_sig(1));
  s->ins = &input->out;
  return s;
}

static inline void underguard(double *x) {
  union {
    u_int32_t i;
    double f;
  } ix;
  ix.f = *x;
  if ((ix.i & 0x7f800000) == 0)
    *x = 0.0f;
}
static inline double interpolate_sample(double *in, int in_size, double frame) {

  double frac, a, b, sample;
  int index = (int)frame;
  frac = frame - index;
  underguard(in);

  a = in[index];
  b = in[(index + 1) % in_size];

  return (1.0 - frac) * a + (frac * b);
}

node_perform comb_dyn_perform(Node *node, int nframes, double spf) {
  double *in = node->ins[0]->buf;
  double *delay_time = node->ins[1]->buf;
  double *out = node->out->buf;
  comb_dyn_state *state = node->state;

  double *write_ptr = (state->buf + state->write_pos);

  while (nframes--) {

    double read_pos = state->write_pos - (*delay_time * ctx_sample_rate());
    if (read_pos < 0) {
      read_pos = state->buf_size + read_pos;
    }
    // state->read_pos = read_pos;

    write_ptr = state->buf + state->write_pos;

    double sample = interpolate_sample(state->buf, state->buf_size, read_pos);

    *out = *in + sample;

    *write_ptr = state->fb * (*out);

    state->write_pos = (state->write_pos + 1) % state->buf_size;
    in++;
    out++;
    delay_time++;
  }
}

Node *comb_dyn_node(double delay_time, double max_delay_time, double fb,
                    Node *input) {
  comb_dyn_state *state = malloc(sizeof(comb_dyn_state));

  Ctx *ctx = get_audio_ctx();
  int SAMPLE_RATE = ctx->sample_rate;

  if (delay_time >= max_delay_time) {
    printf("Error: cannot set delay time %f longer than the max delay time %f",
           delay_time, max_delay_time);
    return NULL;
  }

  int buf_size = (int)max_delay_time * SAMPLE_RATE;
  state->buf_size = buf_size;
  double *buf = malloc(sizeof(double) * (int)max_delay_time * SAMPLE_RATE);
  double *b = buf;
  while (buf_size--) {
    *b = 0.0;
    b++;
  }

  state->buf = buf;
  state->write_pos = 0;
  state->fb = fb;

  Node *s = node_new(state, comb_dyn_perform, NULL, get_sig(1));
  s->ins = malloc(sizeof(Signal *) * 2);
  s->ins[0] = input->out;
  s->ins[1] = get_sig_default(1, delay_time);
  return s;
}

// ------------------------------------------- ALLPASS DELAY

node_perform allpass_perform(Node *node, int nframes, double spf) {
  double *in = node->ins[0]->buf;
  double *out = node->out->buf;
  allpass_state *state = node->state;

  double *write_ptr = state->buf + state->write_pos;
  double *read_ptr = state->buf + state->read_pos;

  while (nframes--) {
    write_ptr = state->buf + state->write_pos;
    read_ptr = state->buf + state->read_pos;
    *write_ptr = *in;
    *out = *in + *read_ptr;

    state->read_pos = (state->read_pos + 1) % state->buf_size;
    state->write_pos = (state->write_pos + 1) % state->buf_size;
    in++;
    out++;
  }
}
void set_allpass_params(allpass_state *state, double delay_time) {
  Ctx *ctx = get_audio_ctx();
  int SR = ctx->sample_rate;

  int buf_size = 1 + (int)delay_time * SR;
  state->buf_size = buf_size;
  double *buf = malloc(sizeof(double) * buf_size);
  double *b = buf;
  while (buf_size--) {
    *b = 0.0;
    b++;
  }

  state->buf = buf;
  state->write_pos = 0;

  int read_pos = state->buf_size - (int)(delay_time * SR);
  state->read_pos = read_pos;
}

Node *allpass_node(double delay_time, double max_delay_time, Node *input) {
  allpass_state *state = malloc(sizeof(allpass_state));
  set_allpass_params(state, delay_time);
  Node *s = node_new(state, allpass_perform, NULL, get_sig(1));
  s->ins = &input->out;
  return s;
}
// ------------------------------------------- FREEVERB
//
// double comb_delay_lengths[] = {1617, 1557, 1491, 1422, 1356, 1277, 1188,
// 1116};
// the original values are optimised for 44100 sample rate
//
static double comb_delay_lengths[] = {1760.00, 1694.69, 1622.86,
                                      1547.76, 1475.92, 1389.93,
                                      1293.06, 1214.69}; // in milliseconds??
static double ap_delay_lengths[] = {
    244.8,
    605.17,
    480.0,
    371.15,
};

static int stereo_spread = 31;

static inline void parallel_comblp_perform(comb_state *state,
                                           op_lp_state *lp_state, double *in,
                                           double *out, int comb_num) {

  double *write_ptr = state->buf + state->write_pos;
  double *read_ptr = state->buf + state->read_pos;
  double del_val = *in + *read_ptr;

  if (comb_num == 0) {
    *out = del_val;
  } else {
    *out += del_val;
  }

  *write_ptr = state->fb * op_lp_perform_tick_return(lp_state, *out);

  state->read_pos = (state->read_pos + 1) % state->buf_size;
  state->write_pos = (state->write_pos + 1) % state->buf_size;
}

static inline void allpass_perform_tick(allpass_state *state, double in,
                                        double *out) {
  int size = state->buf_size;

  double *write_ptr = state->buf + state->write_pos;
  double *read_ptr = state->buf + state->read_pos;

  *write_ptr = in;
  *out = in + *read_ptr;

  state->write_pos = (state->write_pos + 1 % size);
  state->read_pos = (state->read_pos + 1 % size);
}

node_perform freeverb_perform(Node *node, int nframes, double spf) {
  double *in = node->ins[0]->buf;
  double *out = node->out->buf;
  freeverb_state *state = node->state;
  while (nframes--) {
    // double parallel_comb_out = 0;
    int comb = 0;

    double parallel_val = 0.0;

    while (++comb < 8) {
      comb_state *comb_state = state->parallel_combs + comb;
      op_lp_state *lp_state = state->comb_lps + comb;
      parallel_comblp_perform(comb_state, lp_state, in, &parallel_val, comb);
    }
    *out = parallel_val / 8.0;

    parallel_val = 0.0;
    while (++comb < 16) {
      comb_state *comb_state = state->parallel_combs + comb;
      op_lp_state *lp_state = state->comb_lps + comb;
      parallel_comblp_perform(comb_state, lp_state, in, &parallel_val, comb);
    }
    *(out + 1) = parallel_val / 8.0;

    for (int i = 0; i < 4; i++) {
      allpass_state *ap = state->series_ap + i;
      allpass_perform_tick(ap, *out, out);
    }
    out++;

    for (int i = 4; i < 8; i++) {
      allpass_state *ap = state->series_ap + i;
      allpass_perform_tick(ap, *out, out);
    }

    out++;
    in++;
  }
}

Node *freeverb_node(Node *input) {
  double damping_freq = 1000.0;
  double comb_fb = 0.7;

  freeverb_state *state = malloc(sizeof(freeverb_state));
  int SR = ctx_sample_rate();
  for (int i = 0; i < 8; i++) {
    double len = comb_delay_lengths[i];
    set_comb_params(state->parallel_combs + i, len / 1000.0, (len + 1) / 1000.0,
                    comb_fb /*comb gain*/
    );
    set_op_lp_params(state->comb_lps + i, damping_freq);
  }

  for (int i = 0; i < 8; i++) {
    double len = comb_delay_lengths[i] + stereo_spread;
    set_comb_params(state->parallel_combs + i + 8, len / 1000.0,
                    (len + 1) / 1000.0, comb_fb);
    set_op_lp_params(state->comb_lps + i + 8, damping_freq);
  }

  for (int i = 0; i < 4; i++) {
    double len = ap_delay_lengths[i];
    set_allpass_params(state->series_ap + i, len / 1000.0);
  }

  for (int i = 0; i < 4; i++) {
    double len = ap_delay_lengths[i] + stereo_spread;
    set_allpass_params(state->series_ap + i + 4, len / 1000.0);
  }

  Node *s = node_new(state, freeverb_perform, NULL, get_sig(2));
  s->ins = &input->out;
  return s;
}
