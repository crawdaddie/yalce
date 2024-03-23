#include "entry.h"
#include "delay.h"
#include "node.h"
#include "noise.h"
#include "oscillator.h"
#include "scheduling.h"
#include "signal.h"
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
      counter = (int)(state->lagtime / spf);
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

Node *lag_sig(double lagtime, Signal *in) {
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
typedef struct {
  double min;
  double max;
} scale_state;

// perform scaling of an input which is between 0-1 to min-max
node_perform scale_perform(Node *node, int nframes, double spf) {
  scale_state *state = node->state;
  double *in = node->ins[0]->buf;
  double *out = node->out->buf;
  double min = state->min;
  double max = state->max;
  while (nframes--) {
    double val = *in;
    val *= max - min;
    val += min;
    *out = val;
    out++;
    in++;
  }
}

// perform scaling of an input which is between -1-1 to min-max
node_perform scale2_perform(Node *node, int nframes, double spf) {
  scale_state *state = node->state;
  double *in = node->ins[0]->buf;
  double *out = node->out->buf;
  double min = state->min;
  double max = state->max;
  while (nframes--) {
    double val = (*in * (0.5)) + 0.5; // scale to 0-1 first
    val *= max - min;
    val += min;
    *out = val;
    out++;
    in++;
  }
}
// scales a node with outputs between 0-1 to values between min & max (linear)
Node *scale_node(double min, double max, Node *in) {
  scale_state *state = malloc(sizeof(scale_state));
  state->min = min;
  state->max = max;
  Node *s = node_new(state, (node_perform *)scale_perform, NULL, NULL);
  s->ins = malloc(sizeof(Signal *));
  s->ins[0] = in->out;
  s->out = get_sig(in->out->layout);
  return s;
}

// scales a node with outputs between -1-1 to values between min & max (linear)
Node *scale2_node(double min, double max, Node *in) {
  scale_state *state = malloc(sizeof(scale_state));
  state->min = min;
  state->max = max;
  Node *s = node_new(state, (node_perform *)scale2_perform, NULL, NULL);
  s->ins = malloc(sizeof(Signal *));
  s->ins[0] = in->out;
  s->out = get_sig(in->out->layout);
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

int get_block_offset() {

  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  return offset;
}

void set_node_scalar_at(Node *target, int offset, int input, double value) {

  Ctx *ctx = get_audio_ctx();
  scheduler_msg msg = {NODE_SET_SCALAR,
                       offset,
                       {.NODE_SET_SCALAR = (struct NODE_SET_SCALAR){
                            target,
                            input,
                            value,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void set_node_trig_at(Node *target, int offset, int input) {
  Ctx *ctx = get_audio_ctx();
  scheduler_msg msg = {NODE_SET_TRIG,
                       offset,
                       {.NODE_SET_TRIG = (struct NODE_SET_TRIG){
                            target,
                            input,
                        }}};
  push_msg(&ctx->msg_queue, msg);
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

void set_node_trig(Node *target, int input) {
  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  scheduler_msg msg = {NODE_SET_TRIG,
                       offset,
                       {.NODE_SET_TRIG = (struct NODE_SET_TRIG){
                            target,
                            input,
                        }}};
  push_msg(&ctx->msg_queue, msg);
}

void push_msgs(int num_msgs, scheduler_msg *scheduler_msgs) {
  Ctx *ctx = get_audio_ctx();
  int offset = (int)(get_block_diff() * ctx->sample_rate);
  for (int i = 0; i < num_msgs; i++) {
    printf("push msgs %d %p type: %d\n", offset, scheduler_msgs + i,
           (scheduler_msgs + i)->type);
    scheduler_msg msg = scheduler_msgs[i];
    msg.frame_offset = offset;
    push_msg(&ctx->msg_queue, msg);
  }
}

// void *audio_entry() {
//   Node *chain = chain_new();
//   Node *noise = add_to_chain(chain, windowed_impulse_node(10., 100, 0.05));
//   double levels[3] = {0.0, 1.0, 0.0};
//   double times[2] = {0.002, 0.80};
//   Node *env = add_to_chain(chain, env_node(2, levels, times));
//   noise = add_to_chain(chain, mul_node(noise, env));
//   // noise = add_to_chain(chain, op_lp_node(1000., noise));
//   add_to_dac(chain);
//   ctx_add(chain);
//
//   while (true) {
//     // set_node_scalar(freq, 0, random_double_range(200., 1000.));
//     set_node_trig(env, 0);
//     msleep(1000);
//   }
// }
//
// void *audio_entry() {
//   Node *mod = sine(0.3);
//   ctx_add(mod);
//   mod = mul_scalar_node(3.0, mod);
//   ctx_add(mod);
//   mod = add_scalar_node(100., mod);
//   ctx_add(mod);
//
//   Node *sq;
//   for (int i = 0; i < 4; i++) {
//     double freq = 300. * (1. + 0.01 * i);
//     sq = sawsinc_node(freq, freq * 0.99, 100);
//     pipe_output_to_idx(1, mod, sq);
//     add_to_dac(sq);
//     ctx_add(sq);
//   }
//
//   while (true) {
//     msleep(1000);
//   }
// }
//
//
void *audio_entry() {

  Node *sq;
  for (int i = 0; i < 4; i++) {
    double freq = 300. * (1. + 0.01 * i);
    sq = sq_node(freq);
    add_to_dac(sq);
    ctx_add(sq);
  }

  msleep(2000);
  Node *t = ctx_get_tail();
  while (t) {
    ctx_rm_node(t);
    t = ctx_get_tail();
    msleep(2000);
  }
}
int entry() {
  pthread_t thread;
  if (pthread_create(&thread, NULL, (void *)audio_entry, NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
  // Raylib wants to be in the main thread :(
  // create_spectrogram_window();
  // create_window();

  pthread_join(thread, NULL);
}
