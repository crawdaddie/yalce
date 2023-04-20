#include "bufplayer.h"
#include "../common.h"
#include "math.h"

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

  for (int f = 0; f < nframes; f++) {

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
  osc->perform = bufplayer_perform;

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

Node *bufplayer_timestretch_node(Signal *buf, size_t buf_sample_rate,
                                 double rate, double pitchshift,
                                 double start_pos, int loop) {

  Node *osc = ALLOC_NODE(bufplayer_data, "Bufplayer", BUFPLAYER_NUM_INS);
  osc->perform = bufplayer_perform;

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
