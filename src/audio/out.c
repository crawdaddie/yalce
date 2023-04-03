#include "out.h"
#include <stdlib.h>


static node_perform perform_replace_out(Node *node, int nframes, double seconds_per_frame) {
  Signal *out = NODE_DATA(out_data, node)->out;
  Signal *input = NODE_DATA(out_data, node)->in;

  for (int frame = 0; frame < nframes; frame++) {
    double value = input->data[frame];

    out->data[frame * LAYOUT_CHANNELS] = value;
    out->data[frame * LAYOUT_CHANNELS + 1] = value;
  }
}

Node *replace_out(Signal *in, double *channel_out) {
  Node *out = ALLOC_NODE(out_data, "replace_out");
  out->perform = perform_replace_out;
  out_data *data = out->object;

  data->in = in;
  data->out = malloc(sizeof(Signal));
  data->out->data = channel_out;

  data->out->size = LAYOUT_CHANNELS * BUF_SIZE;

  return out;
}

static node_perform perform_add_out(Node *node, int nframes, double seconds_per_frame) {
  Signal *out = NODE_DATA(out_data, node)->out;
  Signal *input = NODE_DATA(out_data, node)->in;

  for (int frame = 0; frame < nframes; frame++) {
    double value = input->data[frame];

    out->data[frame * LAYOUT_CHANNELS] += value;
    out->data[frame * LAYOUT_CHANNELS + 1] += value;
  }
}

Node *add_out(Signal *in, double *channel_out) {
  Node *out = ALLOC_NODE(out_data, "replace_out");
  out->perform = perform_add_out;
  out_data *data = out->object;

  data->in = in;
  data->out = malloc(sizeof(Signal));
  data->out->data = channel_out;
  data->out->size = LAYOUT_CHANNELS * BUF_SIZE;

  return out;
}
