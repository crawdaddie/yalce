#include "entry.h"
#include "common.h"
#include "node.h"
#include "window.h"
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline void underguard(double *x) {
  union {
    u_int32_t i;
    double f;
  } ix;
  ix.f = *x;
  if ((ix.i & 0x7f800000) == 0)
    *x = 0.0f;
}

Signal *get_sig_default(int layout, double value);

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
  Signal *out = node->out;

  while (nframes--) {
    for (int ch = 0; ch < out->layout; ch++) {
      *out->buf = random_double();
      out->buf++;
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

// ----------------------------- SQUARE WAVE OSCILLATORS
//
double sq_sample(double phase, double freq) {
  return scale_val_2(fmod(phase * freq * 2.0 * PI, 2 * PI) > PI, -1, 1);
}

node_perform sq_perform(Node *node, int nframes, double spf) {
  sq_state *state = node->state;

  double *out = node->out->buf;
  double *in = (*node->ins)->buf;

  while (nframes--) {
    double samp =
        sq_sample(state->phase, *in) + sq_sample(state->phase, *in * 1.01);
    samp /= 2;

    state->phase += spf;
    *out = samp;
    out++;
    in++;
  }
}

Node *sq_node(double freq) {
  sq_state *state = malloc(sizeof(sq_state));
  state->phase = 0.0;
  Signal *in = get_sig(1);
  int n = in->size;
  for (int i = 0; i < n; i++) {
    in->buf[i] = freq;
  }

  Node *s = node_new(state, sq_perform, in, get_sig(1));
  s->ins = &in;
  return s;
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

// ------------------------------------------- DISTORTION
typedef struct {
  double gain;
} tanh_state;

node_perform tanh_perform(Node *node, int nframes, double spf) {
  tanh_state *state = node->state;
  double *in = node->ins[0]->buf;
  double *gain = node->ins[1]->buf;
  double *out = node->out->buf;
  while (nframes--) {
    *out = tanh(*gain * (*in));
    in++;
    gain++;
    out++;
  }
}

Node *tanh_node(double gain, Node *node) {

  tanh_state *state = malloc(sizeof(tanh_state));
  state->gain = gain;
  // Signal *in = node->out;
  Signal *out = node->out;

  Node *s = node_new(NULL, tanh_perform, NULL, out);
  s->ins = malloc(2 * (sizeof(Signal *)));
  s->ins[0] = node->out;
  s->ins[1] = get_sig_default(1, gain);
  return s;
}

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
typedef struct {
  double a0;
  double b1;
  double mem;
} op_lp_state;

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
  int SR = ctx_sample_rate();
  double x = exp(-2 * PI * freq / SR);
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

typedef struct {
  double *buf;
  int buf_size;
  int read_pos;
  int write_pos;
  double fb;
} comb_state;

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
  int SR = ctx->SR;

  if (delay_time >= max_delay_time) {
    printf("Error: cannot set delay time %f longer than the max delay time %f",
           delay_time, max_delay_time);
    return;
  }

  int buf_size = (int)max_delay_time * SR;
  state->buf_size = buf_size;
  double *buf = malloc(sizeof(double) * (int)max_delay_time * SR);
  double *b = buf;
  while (buf_size--) {
    *b = 0.0;
    b++;
  }

  state->buf = buf;
  state->write_pos = 0;

  int read_pos = state->buf_size - (int)(delay_time * SR);
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
// ------------------------------------------- ALLPASS DELAY
typedef struct {
  double *buf;
  int buf_size;
  int read_pos;
  int write_pos;
} allpass_state;

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
  int SR = ctx->SR;

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

typedef struct {
  comb_state parallel_combs[16];
  op_lp_state comb_lps[16];
  allpass_state series_ap[8]
} freeverb_state;

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

    for (int i = 0; i < 4; i++) {
      allpass_state *ap = state->series_ap + i + 4;
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

typedef struct {
  double freq;
  double phase;
  double target;
  double *choices;
  int size;
} rand_choice_state;

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

  return node_new(state, rand_choice_perform, &(Signal){}, get_sig(1));
}

// ------------------------------ SIGNAL ARITHMETIC
//
void *sum_signals(double *out, int out_chans, double *in, int in_chans) {
  for (int ch = 0; ch < out_chans; ch++) {
    *(out + ch) += *(in + (ch % in_chans));
  }
}

node_perform sum_perform(Node *node, int nframes, double spf) {
  int num_ins = node->num_ins;
  double *out = node->out->buf;
  int layout = node->out->layout;
  // printf("out %p [%d] (num_ins %d)\n", out, layout, num_ins);

  for (int i = 0; i < nframes; i++) {
    for (int x = 0; x < num_ins; x++) {
      int in_layout = node->ins[x]->layout;
      double *in = node->ins[x]->buf + (i * in_layout);
      // printf("scalar inbuf %p\n", in);
      sum_signals(out, layout, in, in_layout);
    }
    out += layout;
  }
}

Node *sum_nodes(int num, ...) {
  va_list args; // Define a variable to hold the arguments
  va_start(args, num);
  Node *new_node = node_new(NULL, sum_perform, NULL, NULL);
  new_node->ins = malloc(sizeof(Signal *) * (num - 1));
  new_node->num_ins = num - 1;

  Node *first = va_arg(args, Node *);
  new_node->out = first->out;
  Node *n;
  for (int i = 0; i < num - 1; i++) {
    n = va_arg(args, Node *);
    new_node->ins[i] = n->out;
    (new_node->ins[i])->layout = n->out->layout;
    // new_node->ins[i]->layout = 1;
  }

  // Clean up the argument list
  va_end(args);

  // return sum;
  return new_node;
}

Node *sum_nodes_arr(int num, Node **nodes) {
  Node *new_node = node_new(NULL, sum_perform, NULL, NULL);

  Node *first = *nodes;
  new_node->out = first->out;

  new_node->ins = malloc(sizeof(Signal *) * (num - 1));
  new_node->num_ins = num - 1;
  Node *n;
  for (int i = 1; i < num; i++) {
    n = *(nodes + i);
    new_node->ins[i - 1] = n->out;
  }
  return new_node;
}

Node *mix_nodes_arr(int num, Node **nodes, double *scalars) {
  Node *new_node = node_new(NULL, sum_perform, NULL, NULL);

  Node *first = *nodes;
  new_node->out = first->out;

  new_node->ins = malloc(sizeof(Signal *) * (num - 1));
  new_node->num_ins = num - 1;
  Node *n;
  for (int i = 1; i < num; i++) {
    n = *(nodes + i);
    new_node->ins[i - 1] = n->out;
  }
  return new_node;
}

node_perform mul_perform(Node *node, int nframes, double spf) {

  Signal *b = node->ins[0];
  int in_chans = b->layout;
  double *in = b->buf;

  Signal *a = node->out;
  double *out = a->buf;

  while (nframes--) {
    for (int ch = 0; ch < a->layout; ch++) {
      *out = *(in + (ch % in_chans)) * *out;
      // printf("scalar inbuf %p\n", in);
      out++;
    }
    in = in + in_chans;
  }
}

node_perform div_perform(Node *node, int nframes, double spf) {

  Signal *b = node->ins[0];
  int in_chans = b->layout;
  double *in = b->buf;

  Signal *a = node->out;
  double *out = a->buf;

  while (nframes--) {
    for (int ch = 0; ch < a->layout; ch++) {
      *out = *in / *out;
      out++;
    }
    in++;
  }
}

Node *mul_node(Node *a, Node *b) {
  Node *mul = node_new(NULL, mul_perform, NULL, NULL);

  mul->ins = &b->out;
  mul->out = a->out;
  return mul;
}

Node *mul_scalar_node(double scalar, Node *node) {
  Node *mul = node_new(NULL, mul_perform, NULL, NULL);
  mul->ins = malloc(sizeof(Signal *));
  mul->ins[0] = get_sig_default(1, scalar);
  mul->num_ins = 1;
  mul->out = node->out;
  return mul;
}

Node *add_scalar_node(double scalar, Node *node) {
  Node *add = node_new(NULL, sum_perform, NULL, NULL);
  add->ins = malloc(sizeof(Signal *));
  add->ins[0] = get_sig_default(1, scalar);
  add->num_ins = 1;
  add->out = node->out;
  return add;
}

// ------------------------------ SIGNAL / BUFFER ALLOC
//
static double buf_pool[BUF_SIZE * LAYOUT_CHANNELS * 100];
static double *buf_ptr = buf_pool;

void init_sig_ptrs() { buf_ptr = buf_pool; }

Signal *get_sig(int layout) {
  Signal *sig = malloc(sizeof(Signal));
  sig->buf = buf_ptr;
  sig->layout = layout;
  sig->size = BUF_SIZE;
  buf_ptr += BUF_SIZE * layout;
  return sig;
}

Signal *get_sig_default(int layout, double value) {
  Signal *sig = malloc(sizeof(Signal));
  sig->buf = buf_ptr;
  sig->layout = layout;
  sig->size = BUF_SIZE;
  buf_ptr += BUF_SIZE * layout;
  for (int i = 0; i < BUF_SIZE * layout; i++) {
    sig->buf[i] = value;
  }
  return sig;
}
// ----------------------------- Node alloc
Node *node_new(void *data, node_perform *perform, Signal *ins, Signal *out) {
  Node *node = malloc(sizeof(Node));
  node->state = data;
  node->ins = &ins;
  node->num_ins = 1;
  node->out = out;
  node->perform = (node_perform)perform;
  return node;
}

Node *pipe_output(Node *send, Node *recv) {
  recv->ins = &send->out;
  return recv;
}

Node *pipe_output_to_idx(int idx, Node *send, Node *recv) {
  recv->ins[idx] = send->out;
  return recv;
}
Node *add_to_dac(Node *node) {
  node->type = OUTPUT;
  return node;
}

Node *chain_new() { return node_new(NULL, NULL, &(Signal){}, &(Signal){}); }

Node *add_to_chain(Node *chain, Node *node) {
  if (chain->head == NULL) {
    chain->head = node;
    chain->tail = node;
    return node;
  }

  chain->tail->next = node;
  chain->tail = node;
  return node;
}

static double choices[8] = {220.0,
                            246.94165062806206,
                            261.6255653005986,
                            293.6647679174076,
                            329.6275569128699,
                            349.2282314330039,
                            391.99543598174927,
                            880.0};
void *audio_entry_() {

  Node *chain = chain_new();
  Node *noise = add_to_chain(chain, rand_choice_node(6., 8, choices));
  Node *sig = add_to_chain(chain, sine(100.0));
  sig = pipe_output(noise, sig);
  // Node *gain_mod = add_to_chain(chain, sine(8.));
  sig = add_to_chain(chain, tanh_node(5.0, sig));

  Node *mod_freq = add_to_chain(chain, lfnoise(2., 1., 10.));
  mod_freq = add_to_chain(chain, op_lp_node(10., mod_freq));
  Node *mod = add_to_chain(chain, sine(8.));
  pipe_output(mod_freq, mod);
  sig = add_to_chain(chain, freeverb_node(sig));
  sig = add_to_chain(chain, mul_node(sig, mod));
  // sig = add_to_chain(chain, op_lp_node(50., sig));

  add_to_dac(chain);
  ctx_add(chain);
}

void *audio_entry() {

  Node *chain = chain_new();
  Node *sig1 = add_to_chain(chain, sine(100.0));
  Node *sig2 = add_to_chain(chain, sine(800.0));
  // Node *sig3 = add_to_chain(chain, sine(400.0));
  // Node *sig4 = add_to_chain(chain, sine(800.0));
  // add_to_chain(chain, sum_nodes(sig1, sig2, sig3));
  Node *sum = add_to_chain(chain, sum_nodes(2, sig1, sig2));
  sum = add_to_chain(chain, mul_scalar_node(0.2, sum));

  // sig = add_to_chain(chain, sum_nodes(sig, sig2));
  // sig = add_to_chain(chain, op_lp_node(50., sig));

  add_to_dac(chain);
  ctx_add(chain);
}

int entry() {
  pthread_t thread;
  if (pthread_create(&thread, NULL, (void *)audio_entry, NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
  // Raylib wants to be in the main thread :(
  create_window();

  pthread_join(thread, NULL);
}
