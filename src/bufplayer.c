#include "bufplayer.h"
#include "common.h"
#include "ctx.h"
#include "node.h"
#include "soundfile.h"
#include <math.h>
#include <stdlib.h>

node_perform bufplayer_perform(Node *node, int nframes, double spf) {
  bufplayer_state *state = node->state;
  int chans = node->ins[0]->layout;
  double *buf = node->ins[0]->buf;
  double *rate = node->ins[1]->buf;
  double *trig = node->ins[2]->buf;
  double *start_pos = node->ins[3]->buf;

  int buf_size = node->ins[0]->size;
  double *out = node->out->buf;

  double d_index, frac, a, b, sample;
  int index;
  while (nframes--) {
    if (*trig == 1.0) {
      state->phase = 0;
    }

    d_index = (fmod(state->phase + *start_pos, 1.0)) * buf_size;
    index = (int)d_index;
    frac = d_index - index;

    a = buf[index];
    b = buf[(index + 1) % buf_size];

    sample = (1.0 - frac) * a + (frac * b);
    state->phase =
        fmod(state->phase + state->sample_rate_scaling * *rate / buf_size, 1.0);
    *out = sample;

    out++;
    rate++;
    trig++;
    start_pos++;
  }
}

Node *bufplayer_node_(const char *filename) {

  bufplayer_state *state = malloc(sizeof(bufplayer_state));
  state->phase = 0.0;

  Node *s = node_new(state, (node_perform *)bufplayer_perform, NULL, NULL);
  Signal *input_buf = malloc(sizeof(Signal));
  int sf_sample_rate;
  read_file(filename, input_buf, &sf_sample_rate);
  state->sample_rate_scaling = (double)sf_sample_rate / ctx_sample_rate();

  int num_ins = 4;
  s->ins = malloc(sizeof(Signal *) * num_ins);
  s->num_ins = num_ins;
  s->ins[0] = input_buf;
  s->ins[1] = get_sig_default(1, 1.0); // playback rate
  s->ins[2] = get_sig_default(1, 0.0); // trigger
  s->ins[2]->buf[0] = 1.0;
  s->ins[3] = get_sig_default(1, 0.0); // startpos 0-1.0
  s->out = get_sig(s->ins[0]->layout);
  return s;
}

Node *bufplayer_node(Signal *input_buf_sig) {

  bufplayer_state *state = malloc(sizeof(bufplayer_state));
  state->phase = 0.0;
  Node *s = node_new(state, (node_perform *)bufplayer_perform, NULL, NULL);
  state->sample_rate_scaling = 1.;

  int num_ins = 4;
  s->ins = malloc(sizeof(Signal *) * num_ins);
  s->num_ins = num_ins;
  s->ins[0] = input_buf_sig;
  s->ins[1] = get_sig_default(1, 1.0); // playback rate
  s->ins[2] = get_sig_default(1, 0.0); // trigger
  s->ins[2]->buf[0] = 1.0;             // autotrig???
  s->ins[3] = get_sig_default(1, 0.0); // startpos 0-1.0
  s->out = get_sig(s->ins[0]->layout);
  return s;
}

node_perform bufplayer_autotrig_perform(Node *node, int nframes, double spf) {
  bufplayer_autotrig_state *state = node->state;

  double *buf = state->buf;
  int buf_size = state->frames; // num frames
  int buf_layout = state->layout;
  double *out = node->out->buf;

  double d_index, frac, a, b, sample;
  int index;
  while (nframes--) {

    d_index = (fmod(state->phase + state->start_pos, 1.0)) * buf_size;
    for (int ch = 0; ch < buf_layout; ch++) {

      index = (int)d_index;
      frac = d_index - index;

      a = buf[index * buf_layout + ch];
      b = buf[((index + 1) * buf_layout + ch) % (buf_size * buf_layout)];

      sample = (1.0 - frac) * a + (frac * b);
      *out = sample;
      out++;
    }
    state->phase =
        fmod(state->phase + state->sample_rate_scaling * state->rate / buf_size,
             1.0);
  }
}

// Node *bufplayer_autotrig_node(int buf_slot, double rate,
//                               double start_pos) {
//
//   Ctx *ctx = get_audio_ctx();
//   Signal *input_buf_sig = ctx->bufs + buf_slot;
//   printf("autotrig buf node input: %d %d", input_buf_sig->layout,
//          input_buf_sig->size);
//   bufplayer_autotrig_state *state = malloc(sizeof(bufplayer_state));
//
//   state->phase = 0.0;
//   state->rate = rate;
//   state->start_pos = start_pos;
//   state->buf = input_buf_sig;
//
//   Node *s =
//       node_new(state, (node_perform *)bufplayer_autotrig_perform, NULL,
//       NULL);
//   state->sample_rate_scaling = 1.;
//
//   // s->ins = malloc(sizeof(Signal *));
//   // s->num_ins = 1;
//   // s->ins[0] = input_buf_sig;
//   s->out = get_sig(input_buf_sig->layout);
//   return s;
// }
//
static node_destroy bufplayer_autotrig_destroy(Node *node) {
  if (node->num_ins > 0) {
    free(node->ins);
  }

  // bufplayer_autotrig_state *state = node->state;
  free(node->state);
  free(node->out);
  free(node);
}

// warning! highly unstable
Node *bufplayer_autotrig_node(Signal *buf, double rate, double start_pos) {

  Signal *input_buf_sig = buf;

  bufplayer_autotrig_state *state = malloc(sizeof(bufplayer_state));

  state->phase = 0.0;
  state->rate = rate;
  state->start_pos = start_pos;
  state->buf = input_buf_sig->buf;
  state->layout = input_buf_sig->layout;
  state->frames = input_buf_sig->size;

  Node *s =
      node_new(state, (node_perform *)bufplayer_autotrig_perform, NULL, NULL);
  state->sample_rate_scaling = 1.;
  // shared_buf->sample_rate_scaling;
  s->ins = NULL;
  s->num_ins = 0;

  s->out = get_sig(input_buf_sig->layout);
  s->destroy = (node_destroy)bufplayer_autotrig_destroy;

  return s;
}

int buf_alloc(const char *filename) {
  // Ctx *ctx = get_audio_ctx();
  // int buf_counter = ctx->_buf_counter;
  // SharedBuf *buf = ctx->shared_bufs + buf_counter;
  // int buf_sr;
  // read_file(filename, &buf->signal, &buf_sr);
  // buf->sample_rate_scaling = ((double)ctx->sample_rate / buf_sr);
  // ctx->_buf_counter++;
  // return buf_counter;
  return 0;
}
static double _buf[200000];

// warning! highly unstable
Signal *buf_alloc_to_sig(const char *filename) {
  Signal *buf_sig = malloc(sizeof(Signal));
  buf_sig->buf = _buf;
  int buf_sr;
  read_file(filename, buf_sig, &buf_sr);
  return buf_sig;
}
