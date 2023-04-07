#include "blip.h"
#include "math.h"
#include <stdlib.h>

static double sq_sample(double phase, double freq) {
  return scale_val_2(fmod(phase * freq * 2.0 * PI, 2 * PI) > PI, -1, 1);
}
static node_perform sq_blip_perform(Node *node, int nframes, double spf) {
  blip_data *data = NODE_DATA(blip_data, node);
  Signal freq = IN(node, 0);
  Signal *out = OUTS(data);

  for (int f = 0; f < nframes; f++) {
    if (data->dur_s <= 0.0) {
      node->killed = true;
      out->data[f] = 0.0;
    }
    double sample = sq_sample(data->phase, unwrap(freq, f));

    data->phase += spf;
    data->dur_s -= spf;
    out->data[f] = sample;
  }
}

Node *sq_blip_node(double freq, double dur_s) {

  Node *osc = ALLOC_NODE(blip_data, "SqBlip");
  osc->perform = sq_blip_perform;
  blip_data *data = NODE_DATA(blip_data, osc);
  data->dur_s = dur_s;

  INS(data) = malloc(sizeof(Signal) * 1);
  NUM_INS(data) = 1;
  init_signal(INS(data), 1, freq);
  /* init_signal(INS(data) + 1, 1, pw); */

  OUTS(data) = new_signal_heap(BUF_SIZE, 1);
  NUM_OUTS(data) = 1;
  return osc;
}
void blip_setup() {}
