#include "filters.h"
#include "common.h"
#include "ctx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
typedef struct {
  double b0, b1, b2; // Feedforward coefficients
  double a1, a2;     // Feedback coefficients
  double x1, x2;     // Input delay elements (x[n-1] and x[n-2])
  double y1, y2;     // Output delay elements (y[n-1] and y[n-2])
} biquad_state;
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
// #include <math.h>

typedef struct {
  double current_value;
  double target_value;
  double coeff;
  double lag_time;
} lag_state;

node_perform lag_perform(Node *node, int nframes, double spf) {
  double *out = node->out.buf;
  double *in = node->ins[0].buf;

  lag_state *state = (lag_state *)node->state;
  double lag_time = state->lag_time;

  if (fabs(lag_time - state->coeff) > 1e-6) {
    state->coeff = exp(-1.0 / (lag_time * (1.0 / spf)));
  }

  while (nframes--) {
    state->target_value = *in;

    state->current_value =
        state->current_value +
        (state->target_value - state->current_value) * (1.0 - state->coeff);

    // Write the output
    *out = state->current_value;

    out++;
    in++;
  }
}
Node *lag_node(double lag_time, Signal *in) {
  lag_state *state = malloc(sizeof(lag_state));
  state->current_value = 0.0;
  state->target_value = 0.0;
  state->lag_time = lag_time;
  int SAMPLE_RATE = ctx_sample_rate();
  double spf = 1. / SAMPLE_RATE;
  state->coeff = exp(-1.0 / (lag_time * (1.0 / spf)));
  Node *lag = node_new(state, lag_perform, 1, in);
  return lag;
}

typedef struct {
  double gain;
} tanh_state;

node_perform tanh_perform(Node *node, int nframes, double spf) {

  double *out = node->out.buf;
  double *in = node->ins[0].buf;
  tanh_state *state = node->state;
  while (nframes--) {
    *out = tanh(*in * state->gain);
    out++;
    in++;
  }
}
Node *tanh_node(double gain, Signal *in) {
  tanh_state *state = malloc(sizeof(tanh_state));
  state->gain = gain;

  Node *lag = node_new(state, tanh_perform, 1, in);
  return lag;
}

#define MAX_GRAINS 32
#define MIN_GRAIN_LENGTH 1000
#define MAX_GRAIN_LENGTH 2000
#define OVERLAP 0.1

typedef struct {
  int start;
  int length;
  int position;
  double rate;
  double amp;
} Grain;

typedef struct {
  double *buf;
  int buf_size;
  int read_pos;
  int write_pos;
  double fb;
  double pitchshift;

  // Granular processing state
  Grain grains[MAX_GRAINS];
  int active_grains;
  int next_free_grain;
} grain_delay_state;
static inline double interpolate(double *buf, int buf_size, double index) {
  int index_floor = (int)index;
  double frac = index - index_floor;
  double a = buf[index_floor % buf_size];
  double b = buf[(index_floor + 1) % buf_size];
  return a + frac * (b - a);
}

static inline void init_grain(grain_delay_state *state, int index, int start,
                              int length, double rate) {
  state->grains[index].start = start;
  state->grains[index].length = length;
  state->grains[index].position = 0;
  state->grains[index].rate = rate;
  state->grains[index].amp = 0.0;
  state->active_grains++;
}

static inline double process_grains(grain_delay_state *state) {
  double out = 0.0;
  for (int i = 0; i < MAX_GRAINS; i++) {
    Grain *grain = &state->grains[i];
    if (grain->length > 0) {
      double index = grain->start + grain->position * grain->rate;
      double sample =
          interpolate(state->buf, state->buf_size, index) * grain->amp;
      out += sample;

      grain->position++;
      if (grain->position >= grain->length) {
        grain->length = 0;
        state->active_grains--;
        if (i < state->next_free_grain) {
          state->next_free_grain = i;
        }
      } else {
        // Apply envelope
        double env_pos = (double)grain->position / grain->length;
        if (env_pos < OVERLAP) {
          grain->amp = env_pos / OVERLAP;
        } else if (env_pos > (1.0 - OVERLAP)) {
          grain->amp = (1.0 - env_pos) / OVERLAP;
        } else {
          grain->amp = 1.0;
        }
      }
    }
  }
  return out / (state->active_grains > 0 ? state->active_grains : 1);
}

node_perform grain_delay_perform(Node *node, int nframes, double spf) {
  double *out = node->out.buf;
  double *in = node->ins[0].buf;
  grain_delay_state *state = node->state;

  double rate = state->pitchshift;

  while (nframes--) {
    // Write input to buffer
    state->write_pos = (state->write_pos + 1) % state->buf_size;

    // Trigger new grain
    if (state->active_grains < MAX_GRAINS &&
        (rand() % 100 < 5)) { // 5% chance of new grain
      int grain_length =
          MIN_GRAIN_LENGTH + rand() % (MAX_GRAIN_LENGTH - MIN_GRAIN_LENGTH);
      init_grain(state, state->next_free_grain, state->read_pos, grain_length,
                 rate);

      // Find next free grain
      do {
        state->next_free_grain = (state->next_free_grain + 1) % MAX_GRAINS;
      } while (state->grains[state->next_free_grain].length != 0 &&
               state->next_free_grain != state->active_grains - 1);
    }

    // Process grains
    *out = process_grains(state);

    // Update read position
    state->read_pos = (state->read_pos + 1) % state->buf_size;

    state->buf[state->write_pos] = *in + state->fb * (*out);

    in++;
    out++;
  }
}

Node *grain_delay_node(double delay_time, double max_delay_time, double fb,
                       double pitchshift, Signal *input) {
  int SAMPLE_RATE = ctx_sample_rate();

  int bufsize = (int)(max_delay_time * SAMPLE_RATE);
  int bufsize_bytes = (bufsize * sizeof(double));
  int total_size = sizeof(grain_delay_state) + bufsize_bytes + sizeof(Signal);

  void *mem = calloc(total_size, sizeof(char));

  if (!mem) {
    fprintf(stderr, "memory alloc for grain_delay node failed\n");
    return NULL;
  }

  double *buf = mem;
  mem += bufsize_bytes;

  grain_delay_state *state = mem;
  mem += sizeof(grain_delay_state);

  Signal *ins = mem;

  ins->buf = input->buf;
  ins->size = input->size;
  ins->layout = input->layout;

  state->buf = buf;
  state->buf_size = bufsize;
  state->fb = fb;
  state->pitchshift = pitchshift;

  state->write_pos = 0;
  state->read_pos = state->buf_size - (int)(delay_time * SAMPLE_RATE);

  state->active_grains = 0;
  state->next_free_grain = 0;

  return node_new(state, grain_delay_perform, 1, ins);
}
