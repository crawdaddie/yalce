#include "out.h"

void perform_add_out(Node *node, int nframes, double seconds_per_frame) {
  double *out = node->out;
  double *input = node->in[0].data;
  for (int frame = 0; frame < nframes; frame++) {
    out[frame * node->num_outs] += input[frame * node->num_outs];
    out[frame * node->num_outs + 1] += input[frame * node->num_outs + 1];
  }
}

Node *add_out(double *node_out, double *channel_out) {

  Node *node = calloc(sizeof(Node), 1);
  node->perform = perform_add_out;
  node->in = malloc(sizeof(Signal));
  node->in[0].data = node_out;
  node->in[0].size = BUF_SIZE * 2;
  node->out = channel_out;

  node->num_outs = 2;
  return node;
}

void perform_replace_out(Node *node, int nframes, double seconds_per_frame) {
  double *out = node->out;
  double *input = node->in[0].data;
  for (int frame = 0; frame < nframes; frame++) {
    out[frame * node->num_outs] = input[frame * node->num_outs];
    out[frame * node->num_outs + 1] = input[frame * node->num_outs + 1];
  }
}

Node *replace_out(double *node_out, double *channel_out) {

  Node *node = calloc(sizeof(Node), 1);
  node->perform = perform_replace_out;
  node->in = malloc(sizeof(Signal));
  node->in[0].data = node_out;
  node->in[0].size = BUF_SIZE * 2;
  node->out = channel_out;

  node->num_outs = 2;
  return node;
}
