#include "out.h"
#include "../ctx.h"
#include <stdlib.h>

static node_perform perform_replace_out(Node *node, int nframes,
                                        double seconds_per_frame) {
  out_data *data = NODE_DATA(out_data, node);
  Signal *out = data->out_channel;
  Signal *input = data->in;

  for (int layout = 0; layout < out->layout; layout++) {
    for (int frame = 0; frame < nframes; frame++) {
      double value = input->data[frame];
      out->data[out->layout * frame + layout] = value;
    }
  }
}

Node *replace_out(Signal *in, Signal *out_channel) {
  Node *out = ALLOC_NODE(out_data, "replace_out");
  out->perform = perform_replace_out;
  out_data *data = out->data;

  data->in = in;
  data->out_channel = out_channel;

  return out;
}

static node_perform perform_add_out(Node *node, int nframes,
                                    double seconds_per_frame) {

  out_data *data = NODE_DATA(out_data, node);
  Signal *out = data->out_channel;
  Signal *input = data->in;

  for (int layout = 0; layout < out->layout; layout++) {
    for (int frame = 0; frame < nframes; frame++) {
      double value = input->data[frame];
      out->data[out->layout * frame + layout] += value;
    }
  }
}

Node *add_out(Signal *in, Signal *out_channel) {
  Node *out = ALLOC_NODE(out_data, "replace_out");
  out->perform = perform_add_out;
  out_data *data = out->data;

  data->in = in;
  data->out_channel = out_channel;

  return out;
}
