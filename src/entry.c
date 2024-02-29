#include "entry.h"
#include "bufplayer.h"
#include "delay.h"
#include "node.h"
#include "oscillator.h"
#include "scheduling.h"
#include "signal.h"
#include "start_audio.h"
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

  return node_new(state, rand_choice_perform, NULL, get_sig(1));
}

// ------------------------------ SIGNAL ARITHMETIC

typedef struct {
  double lagtime;
  double target;
  double level;
  double slope;
  int counter;
} lag_state;

node_perform lag_perform(Node *node, int nframes, double spf) {
  double *out = node->out->buf;
  double *in = node->ins[0]->buf;

  lag_state *state = node->state;
  int counter = 0;

  while (nframes--) {
    if (*in != state->level) {
      counter = (int)(state->lagtime * 48000);
      state->counter = counter;
      state->target = *in;
      state->slope = (state->target - state->level) / counter;
    }

    if (state->counter > 0) {
      state->counter--;
      state->level += state->slope;

      state->slope = (state->target - state->level) / state->counter;
      *out = state->level;
    } else {
      state->counter = 0;
      *out = *in;
      state->level = *in;
    }
    out++;
    in++;
  }
}

Node *lag_node(double lagtime, Signal *in) {
  lag_state *state = malloc(sizeof(lag_state));
  state->lagtime = lagtime;
  state->counter = 0;
  state->level = in->buf[0];
  state->target = in->buf[0];

  Node *s = node_new(state, (node_perform *)lag_perform, NULL, NULL);
  s->ins = malloc(sizeof(Signal *));
  s->ins[0] = in;
  s->out = get_sig(1);

  return s;
}

//
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
double get_block_diff() {
  struct timespec current_time;
  clock_gettime(CLOCK_REALTIME, &current_time);

  struct timespec audio_block_time;
  get_block_time(&audio_block_time);

  return timespec_diff(current_time, audio_block_time);
}

Node *add_to_dac(Node *node) {
  node->type = OUTPUT;
  return node;
}
Node *chain_set_out(Node *chain, Node *out) {
  chain->out = out->out;
  return chain;
}

Node *chain_new() { return node_new(NULL, NULL, &(Signal){}, &(Signal){}); }

Node *chain_with_inputs(int num_ins, double *defaults) {
  Node *chain = node_new(NULL, NULL, NULL, &(Signal){});

  chain->ins = malloc(num_ins * (sizeof(Signal *)));
  for (int i = 0; i < num_ins; i++) {
    chain->ins[i] = get_sig_default(1, defaults[i]);
  }
  return chain;
}

Node *node_set_input_signal(Node *node, int num_in, Signal *sig) {
  node->ins[num_in] = sig;
  return node;
}

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
  sig = add_to_chain(chain, tanh_node(2.0, sig));
  sig = add_to_chain(chain, freeverb_node(sig));

  add_to_dac(chain);
  ctx_add(chain);
}

void set_node_scalar(Node *target, int input, double value) {
  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  scheduler_msg msg = {NODE_SET_SCALAR,
                       offset,
                       {.NODE_SET_SCALAR = (struct NODE_SET_SCALAR){
                            target,
                            input,
                            value,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void *audio_entry() {
  Node *chain = chain_new();
  Signal *in_sig = get_sig_default(1, 110.);
  Node *freq = add_to_chain(chain, lag_node(0.2, in_sig));
  Node *sig1 = add_to_chain(chain, sine(100.0));
  pipe_output(freq, sig1);
  add_to_dac(chain);
  ctx_add(chain);

  while (true) {
    set_node_scalar(freq, 0, random_double_range(200., 1000.));
    msleep(500);
  }
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
