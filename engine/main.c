#include "audio_loop.h"
#include "ctx.h"
#include "lib.h"
#include "node.h"
#include "oscillators.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

Node *group_add_tail(Node *group, Node *node) {
  group_state *group_ctx = group->state;

  if (group_ctx->head == NULL) {
    group_ctx->head = node;
    group_ctx->tail = node;
    return node;
  }

  group_ctx->tail->next = node;
  group_ctx->tail = node;
  return node;
}

Node *node_new(void *data, node_perform *perform, int num_ins, Signal *ins) {
  Node *node = malloc(sizeof(Node));
  node->state = data;
  node->num_ins = num_ins;
  node->ins = ins;
  node->out.layout = 1;
  node->out.size = BUF_SIZE;
  node->out.buf = calloc(BUF_SIZE, sizeof(double));
  node->perform = (node_perform)perform;
  node->frame_offset = 0;
  return node;
}

Signal *get_sig(int layout) {
  Signal *sig = malloc(sizeof(Signal));
  sig->buf = calloc(BUF_SIZE * layout, sizeof(double));
  sig->layout = layout;
  sig->size = BUF_SIZE;
  return sig;
}

Signal *get_sig_default(int layout, double value) {
  Signal *sig = get_sig(layout);
  for (int i = 0; i < BUF_SIZE * layout; i++) {
    sig->buf[i] = value;
  }
  return sig;
}

Node *group_new(int chans) {
  group_state *graph = malloc(sizeof(group_state));
  Node *g = node_new((void *)graph, (node_perform *)group_perform, 0, NULL);
  return g;
}

Node *sq_node(double freq) {
  sq_state *state = malloc(sizeof(sq_state));
  state->phase = 0.0;

  Node *s =
      node_new(state, (node_perform *)sq_perform, 1, get_sig_default(1, freq));
  return s;
}

node_perform sum_perform(Node *node, int nframes, double spf) {
  int num_ins = node->num_ins;
  double *out = node->out.buf;

  Signal *input_sigs = node->ins;
  for (int i = 0; i < nframes; i++) {
    out[i] = input_sigs[0].buf[i];

    for (int j = 1; j < num_ins; j++) {
      out[i] += input_sigs[j].buf[i];
    }
  }
}

Node *sum_nodes2(Node *a, Node *b) {
  Signal *ins = malloc(sizeof(double *) * 2);
  Node *sum = node_new(NULL, (node_perform *)sum_perform, 2, ins);
  sum->ins[0] = a->out;
  sum->ins[1] = b->out;
  return sum;
}

Node *synth(double freq, double cutoff) {
  Node *group = group_new(1);
  group->num_ins = 0;
  Node *sq1 = sq_node(freq);
  group_add_tail(group, sq1);

  Node *sq2 = sq_node(freq * 1.01);
  group_add_tail(group, sq2);

  Node *summed = sum_nodes2(sq1, sq2);
  group_add_tail(group, summed);
  add_to_dac(summed);

  return group;
}

int main(int argc, char **argv) {
  init_audio();
  Node *s = synth(100., 500.);

  add_to_dac(s);
  audio_ctx_add(s);

  while (1) {
    // print_graph();
  }
  return 0;
}
