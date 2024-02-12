#include "bufplayer.h"
#include "audio_math.h"
#include "common.h"
#include <math.h>

static bool handle_trig(Signal trig, int frame) {
  if (unwrap(trig, frame) > 0.0) {
    if (trig.size == 1) {
      *trig.data = 0.0;
    }
    return true;
  }
  return false;
};
static node_perform bufplayer_perform(Node *node, int nframes, double spf) {
  bufplayer_data *data = NODE_DATA(bufplayer_data, node);
  double sample;
  double rate;

  double *buffer = node->ins[0].data;
  int bufsize = node->ins[0].size;

  Signal trig = IN(node, BUFPLAYER_TRIG);
  Signal start_pos = IN(node, BUFPLAYER_STARTPOS);

  for (int f = get_block_offset(node); f < nframes; f++) {

    if (handle_trig(trig, f)) {
      data->read_ptr = *start_pos.data * bufsize;
    }

    sample = get_sample_interp(data->read_ptr, buffer, bufsize);

    rate = unwrap(IN(node, BUFPLAYER_RATE), f);
    data->read_ptr = fmod(data->read_ptr + rate, bufsize);
    node_write_out(node, f, sample);
  }
}

static void init_trig_signal(Signal *trig) {
  trig->data = malloc(sizeof(double));
  trig->data[0] = 1;
  trig->size = 1;
}

Node *bufplayer_node(Signal *buf, size_t buf_sample_rate, double rate,
                     double start_pos, int loop) {

  Node *osc = ALLOC_NODE(bufplayer_data, "Bufplayer", BUFPLAYER_NUM_INS);
  osc->perform = (node_perform)bufplayer_perform;

  INS(osc) = ALLOC_SIGS(BUFPLAYER_NUM_INS);
  Signal *buffer = INS(osc);

  buffer->data = buf->data;
  buffer->size = buf->size;
  buffer->layout = buf->layout;

  init_signal(INS(osc) + BUFPLAYER_RATE, 1, rate);
  init_trig_signal(INS(osc) + BUFPLAYER_TRIG);

  init_signal(INS(osc) + BUFPLAYER_STARTPOS, 1, start_pos);

  init_out_signal(&OUTS(osc), BUF_SIZE, 1);
  bufplayer_data *data = osc->data;
  data->read_ptr = start_pos * buffer->size;

  return osc;
};

static node_perform bufplayer_timestretch_perform(Node *node, int nframes,
                                                  double spf) {

  Signal buffer = IN(node, TIMESTRETCH_BUF);
  int bufsize = buffer.size;
  Signal trig_freq = IN(node, TIMESTRETCH_TRIG_FREQ);
  Signal pitchshift = IN(node, TIMESTRETCH_PITCHSHIFT);
  Signal speed = IN(node, TIMESTRETCH_SPEED);

  timestretch_data *data = node->data;
  double trig_freq_val = unwrap(trig_freq, 0);

  double retrig_after_samps =
      trig_freq_val == 0.0 ? bufsize : 1 / (spf * unwrap(trig_freq, 0));
  double pitchshift_rate;

  for (int f = get_block_offset(node); f < nframes; f++) {
    if (data->trig_ramp == 0.0) {
      data->read_ptr = data->start_pos;
    }
    node_write_out(node, f,
                   get_sample_interp(data->read_ptr, buffer.data, bufsize));

    pitchshift_rate = unwrap(pitchshift, f);
    data->read_ptr = fmod(data->read_ptr + pitchshift_rate, bufsize);
    data->trig_ramp = fmod(data->trig_ramp + 1, retrig_after_samps);
    data->start_pos = fmod(data->start_pos + unwrap(speed, f), bufsize);
  }
}

Node *bufplayer_timestretch_node(Signal *buf, size_t buf_sample_rate,
                                 double rate, double pitchshift,
                                 double trig_freq, double start_pos, int loop) {

  Node *osc = ALLOC_NODE(timestretch_data, "Bufplayer Timestretch",
                         TIMESTRETCH_NUM_INS);

  osc->perform = (node_perform)bufplayer_timestretch_perform;

  INS(osc) = ALLOC_SIGS(TIMESTRETCH_NUM_INS);
  Signal *buffer = INS(osc);

  buffer->data = buf->data;
  buffer->size = buf->size;
  buffer->layout = buf->layout;

  init_signal(INS(osc) + TIMESTRETCH_PITCHSHIFT, 1, pitchshift);
  init_signal(INS(osc) + TIMESTRETCH_SPEED, 1, rate);
  init_signal(INS(osc) + TIMESTRETCH_TRIG_FREQ, 1, trig_freq);
  init_out_signal(&OUTS(osc), BUF_SIZE, 1);
  timestretch_data *data = osc->data;
  data->read_ptr = start_pos * buffer->size;
  data->trig_ramp = 0.0;
  data->start_pos = start_pos * buffer->size;
  return osc;
};
