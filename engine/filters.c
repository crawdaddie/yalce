#include "filters.h"
#include "common.h"
#include "ctx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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

#define DEFINE_BIQUAD_DYN_PERFORM_FUNCTION(name, set_coefficients_function)    \
  static node_perform name(Node *node, int nframes, double spf) {              \
    double *out = node->out.buf;                                               \
    int out_layout = node->out.layout;                                         \
    double *in = node->ins[0].buf;                                             \
    int in_layout = node->ins[0].layout;                                       \
    double *freq_in = node->ins[1].buf;                                        \
    double *res_in = node->ins[2].buf;                                         \
    biquad_state *filter = node->state;                                        \
    double prev_freq = *freq_in;                                               \
    double prev_res = *res_in;                                                 \
    set_coefficients_function(*freq_in, *res_in, (int)(1 / spf), filter);      \
    while (nframes--) {                                                        \
      double freq = *freq_in;                                                  \
      double res = *res_in;                                                    \
      if (freq != prev_freq) {                                                 \
        set_coefficients_function(freq, res, (int)(1 / spf), filter);          \
      }                                                                        \
      double input = *in;                                                      \
      double output = filter->b0 * input + filter->b1 * filter->x1 +           \
                      filter->b2 * filter->x2 - filter->a1 * filter->y1 -      \
                      filter->a2 * filter->y2;                                 \
      filter->x2 = filter->x1;                                                 \
      filter->x1 = input;                                                      \
      filter->y2 = filter->y1;                                                 \
      filter->y1 = output;                                                     \
      *out = output;                                                           \
      in++;                                                                    \
      prev_freq = freq;                                                        \
      freq_in++;                                                               \
      prev_res = res;                                                          \
      res_in++;                                                                \
      out++;                                                                   \
    }                                                                          \
  }

static node_perform biquad_perform(Node *node, int nframes, double spf) {
  double *out = node->out.buf;
  int out_layout = node->out.layout;
  double *in = node->ins[0].buf;
  int in_layout = node->ins[0].layout;

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

DEFINE_BIQUAD_DYN_PERFORM_FUNCTION(biquad_lp_dyn_perform,
                                   set_biquad_lp_coefficients)

Node *biquad_lp_node(Signal *freq, Signal *res, Signal *in) {
  biquad_state *state = malloc(sizeof(biquad_state));

  Signal *ins = malloc(sizeof(Signal *) * 3);

  ins[0] = *in;
  ins[1] = *freq;
  ins[2] = *res;
  Node *n = node_new(state, (node_perform *)biquad_lp_dyn_perform, 3, ins);
  return n;
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

DEFINE_BIQUAD_DYN_PERFORM_FUNCTION(biquad_bp_dyn_perform,
                                   set_biquad_bp_coefficients)

Node *biquad_bp_node(Signal *freq, Signal *res, Signal *in) {

  biquad_state *state = malloc(sizeof(biquad_state));

  Signal *ins = malloc(sizeof(Signal *) * 3);

  ins[0] = *in;
  ins[1] = *freq;
  ins[2] = *res;
  Node *n = node_new(state, (node_perform *)biquad_bp_dyn_perform, 3, ins);
  return n;
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

DEFINE_BIQUAD_DYN_PERFORM_FUNCTION(biquad_hp_dyn_perform,
                                   set_biquad_hp_coefficients)

Node *biquad_hp_node(Signal *freq, Signal *res, Signal *in) {

  biquad_state *state = malloc(sizeof(biquad_state));

  Signal *ins = malloc(sizeof(Signal *) * 3);

  ins[0] = *in;
  ins[1] = *freq;
  ins[2] = *res;
  Node *n = node_new(state, (node_perform *)biquad_hp_dyn_perform, 3, ins);

  return n;
}

void set_butterworth_hp_coefficients(double freq, int fs, biquad_state *state) {

  double fc = freq; // Cutoff frequency (Hz)
  double w0 = 2.0 * PI * fc / fs;
  double wc = tan(w0 / 2);
  double k = wc * wc;
  double sqrt2 = sqrt(2.0);

  // Compute filter coefficients
  double b0 = 1 / (1 + sqrt2 * wc + k);
  double b1 = -2 * b0;
  double b2 = b0;
  double a1 = 2 * (k - 1) * b0;
  double a2 = (1 - sqrt2 * wc + k) * b0;

  // Initialize filter
  set_biquad_filter_state(state, b0, b1, b2, a1, a2);
};

static node_perform butterworth_hp_dyn_perform(Node *node, int nframes,
                                               double spf) {
  double *out = node->out.buf;
  int out_layout = node->out.layout;
  double *in = node->ins[0].buf;

  int in_layout = node->ins[0].layout;

  double *freq_in = node->ins[1].buf;

  biquad_state *filter = node->state;

  double prev_freq = *freq_in;
  set_butterworth_hp_coefficients(*freq_in, (int)(1 / spf), filter);

  while (nframes--) {
    double freq = *freq_in;
    if (freq != prev_freq) {
      set_butterworth_hp_coefficients(freq, (int)(1 / spf), filter);
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
    out++;
  }
}

// Node *butterworth_hp_dyn_node(double freq, Node *in) {
//
//   biquad_state *state = malloc(sizeof(biquad_state));
//
//   Node *node = node_new(state, (node_perform *)butterworth_hp_dyn_perform,
//   NULL,
//                         get_sig(in->out->layout));
//
//   node->ins = malloc(sizeof(Signal *) * 2);
//   node->ins[0] = in->out;
//   node->ins[1] = get_sig_default(1, freq);
//   set_butterworth_hp_coefficients(freq, ctx_sample_rate(), node->state);
//   zero_biquad_filter_state(node->state);
//   return node;
// }
//
//
//

typedef struct {
  double *buf;
  int buf_size;
  int read_pos;
  int write_pos;
  double fb;
} comb_state;

typedef struct {
  double *buf;
  int buf_size;
  int write_pos;
  double fb;
} comb_dyn_state;

Node *comb_dyn_node(double delay_time, double max_delay_time, double fb,
                    Node *input);

node_perform comb_perform(Node *node, int nframes, double spf) {
  double *out = node->out.buf;
  double *in = node->ins[0].buf;
  comb_state *state = node->state;

  double *write_ptr = (state->buf + state->write_pos);
  double *read_ptr = state->buf + state->read_pos;

  while (nframes--) {
    // printf("nframes %d bufsize %d %d %d\n", nframes, state->buf_size,
    //        state->read_pos, state->write_pos);
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

Node *comb_node(double delay_time, double max_delay_time, double fb,
                Signal *input) {

  int SAMPLE_RATE = ctx_sample_rate();

  int bufsize = (int)(max_delay_time * SAMPLE_RATE);
  int bufsize_bytes = (bufsize * sizeof(double));
  int total_size = sizeof(comb_state) + bufsize_bytes + sizeof(Signal);
  // +
  //                  sizeof(Node) + (sizeof(double) * BUF_SIZE);

  void *mem = calloc(total_size, sizeof(char));

  if (!mem) {
    fprintf(stderr, "memory alloc for comb node failed\n");
    return NULL;
  }

  double *buf = mem;
  mem += bufsize_bytes;

  comb_state *state = mem;
  mem += sizeof(comb_state);

  Signal *ins = mem;

  ins->buf = input->buf;
  ins->size = input->size;
  ins->layout = input->layout;

  state->buf = buf;
  state->buf_size = bufsize;
  state->fb = fb;

  state->write_pos = 0;
  state->read_pos = state->buf_size - (int)(delay_time * SAMPLE_RATE);

  return node_new(state, comb_perform, 1, ins);
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
//
// node_perform comb_dyn_perform(Node *node, int nframes, double spf) {
//   double *in = node->ins[0]->buf;
//   double *delay_time = node->ins[1]->buf;
//   double *out = node->out->buf;
//   comb_dyn_state *state = node->state;
//
//   double *write_ptr = (state->buf + state->write_pos);
//
//   while (nframes--) {
//
//     double read_pos = state->write_pos - (*delay_time * ctx_sample_rate());
//     if (read_pos < 0) {
//       read_pos = state->buf_size + read_pos;
//     }
//     // state->read_pos = read_pos;
//
//     write_ptr = state->buf + state->write_pos;
//
//     double sample = interpolate_sample(state->buf, state->buf_size,
//     read_pos);
//
//     *out = *in + sample;
//
//     *write_ptr = state->fb * (*out);
//
//     state->write_pos = (state->write_pos + 1) % state->buf_size;
//     in++;
//     out++;
//     delay_time++;
//   }
// }
//
// Node *comb_dyn_node(double delay_time, double max_delay_time, double fb,
//                     Node *input) {
//   comb_dyn_state *state = malloc(sizeof(comb_dyn_state));
//
//   Ctx *ctx = get_audio_ctx();
//   int SAMPLE_RATE = ctx->sample_rate;
//
//   if (delay_time >= max_delay_time) {
//     printf("Error: cannot set delay time %f longer than the max delay time
//     %f",
//            delay_time, max_delay_time);
//     return NULL;
//   }
//
//   int buf_size = (int)(max_delay_time * SAMPLE_RATE);
//
//   state->buf_size = buf_size;
//   double *buf = malloc(sizeof(double) * buf_size);
//   double *b = buf;
//   while (buf_size--) {
//     *b = 0.0;
//     b++;
//   }
//
//   state->buf = buf;
//   state->write_pos = 0;
//   state->fb = fb;
//
//   Node *s = node_new(state, comb_dyn_perform, NULL, get_sig(1));
//   s->ins = malloc(sizeof(Signal *) * 2);
//   s->ins[0] = input->out;
//   s->ins[1] = get_sig_default(1, delay_time);
//   return s;
// }
