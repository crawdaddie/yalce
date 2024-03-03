#include "bufplayer.h"
#include "ctx.h"
#include "node.h"
#include "soundfile.h"
#include <math.h>
#include <rubberband/rubberband-c.h>
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

Node *bufplayer_node(const char *filename) {

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

typedef struct {
  RubberBandState rubberband_state;
  int processed_frames;
  int buf_offset;
  int hopsize;
  SignalFloatDeinterleaved *buf;
  int sfsample_rate;
} bufplayer_pitchshift_state;
// // third parameter is always 0 since we are never expecting a final frame
// rubberband_process(p->rb, (const float* const*)&(in->data), p->hopsize, 0);
// if (rubberband_available(p->rb) >= (int)p->hopsize) {
//   rubberband_retrieve(p->rb, (float* const*)&(out->data), p->hopsize);
// } else {
//   AUBIO_WRN("pitchshift: catching up with zeros"
//       ", only %d available, needed: %d, current pitchscale: %f\n",
//       rubberband_available(p->rb), p->hopsize, p->pitchscale);
//   fvec_zeros(out);
// }
int minimum(int a, int b) {
  if (a <= b) {
    return a;
  }
  return b;
}
node_perform bufplayer_pitchshift_perform(Node *node, int nframes, double spf) {

  bufplayer_pitchshift_state *state = node->state;

  float *buf = state->buf->buf + state->buf_offset;
  int buf_size = state->buf->size;

  double *rate = node->ins[0]->buf;
  float *out = (float *)node->out->buf;

  state->buf_offset = (state->buf_offset + state->hopsize) % buf_size;
  int frames_processed = nframes;
  int out_offset = 0;
  while (frames_processed) {
    rubberband_process(state->rubberband_state, (const float *const *)&buf,
                       state->hopsize, false);
    // printf("available: %d\n", rubberband_available(state->rubberband_state));
    int available = minimum(rubberband_available(state->rubberband_state),
                            nframes - out_offset);
    printf("out_offset %d buf_offset %d available %d\n", out_offset,
           state->buf_offset, available);
    rubberband_retrieve(state->rubberband_state, (float *const *)&out,
                        available);
    out_offset = (out_offset + available) % nframes;
    out = out + out_offset;
    state->buf_offset = (state->buf_offset + state->hopsize) % buf_size;
    buf = state->buf->buf + state->hopsize;
    frames_processed -= available;
  }
}

Node *bufplayer_pitchshift_node(const char *filename) {

  bufplayer_pitchshift_state *state =
      malloc(sizeof(bufplayer_pitchshift_state));
  state->rubberband_state = rubberband_new(
      ctx_sample_rate(), 1, RubberBandOptionTransientsCrisp, 1.0, 2.0);
  state->buf_offset = 0;
  state->hopsize = 256;
  int sfsample_rate;
  SignalFloatDeinterleaved *input_buf =
      malloc(sizeof(SignalFloatDeinterleaved));
  read_file_float_deinterleaved(filename, input_buf, &sfsample_rate);
  state->buf = input_buf;
  state->sfsample_rate = sfsample_rate;
  //

  Node *s =
      node_new(state, (node_perform *)bufplayer_pitchshift_perform, NULL, NULL);

  s->ins = malloc(sizeof(Signal *));
  s->num_ins = 1;
  s->ins[0] = get_sig_default(1, 1.0);
  s->out = get_sig_float(s->ins[0]->layout);
  return s;
}
