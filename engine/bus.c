#include "./bus.h"
#include "audio_graph.h"
#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct summed_inlet_state {
  NodeRef sig;
  struct summed_inlet_state *next;
} summed_inlet_state;

void *summed_inlet_perform(Node *node, summed_inlet_state *state,
                           Node *inputs[], int nframes, sample_t spf) {
  int output_num = 0;

  Signal out = node->output;
  for (int i = 0; i < out.layout * out.size; i++) {
    out.buf[i] = 0.;
  }

  // printf("%p state\n", state);
  while (state) {
    if (state->sig) {
      write_to_dac(out.layout, out.buf, state->sig->output.layout,
                   state->sig->output.buf, output_num, nframes);
    }
    output_num++;

    state = state->next;
  }
  return out.buf;
}

NodeRef pipe_into(NodeRef filter, int idx, NodeRef node) {
  // if (filter->)
  // printf("pipe %p into %p\n", node, filter);
  AudioGraph *g = filter + 1;
  if (filter->state_ptr) {
    g = filter->state_ptr;
  }
  int inlet_idx = g->inlets[idx];
  NodeRef inlet_node = g->nodes + inlet_idx;
  int layout = inlet_node->output.layout;

  int _layout = node->output.layout;
  // printf("pipe %d node into %d input\n", _layout, layout);

  if (_layout > layout) {
    inlet_node->output = (Signal){
        .layout = _layout,
        .size = BUF_SIZE,
        .buf = malloc(sizeof(BUF_SIZE * _layout)),
    };
  }

  if (inlet_node->perform == NULL) {
    inlet_node->perform = (perform_func_t)summed_inlet_perform;
    inlet_node->state_ptr = malloc(sizeof(summed_inlet_state));
    summed_inlet_state *st = inlet_node->state_ptr;
    *st = (summed_inlet_state){.sig = node, .next = NULL};
    inlet_node->state_ptr = st;

  } else if (((char *)inlet_node->perform) == (char *)summed_inlet_perform) {

    summed_inlet_state *st = inlet_node->state_ptr;
    summed_inlet_state *new_st = malloc(sizeof(summed_inlet_state));
    new_st->sig = node;
    new_st->next = st;
    inlet_node->state_ptr = new_st;
  }
  node->write_to_output = false;

  return filter;
}

NodeRef plug_node(NodeRef filter, int idx, NodeRef node) {
  AudioGraph *g = filter + 1;
  if (filter->state_ptr) {
    g = filter->state_ptr;
  }
  int inlet_idx = g->inlets[idx];
  NodeRef inlet_node = g->nodes + inlet_idx;
  int layout = inlet_node->output.layout;

  int _layout = node->output.layout;
}

void *bus_perform(Node *node, void *state, Node *inputs[], int nframes,

                  sample_t spf) {
  sample_t *out = node->output.buf;
  int layout = node->output.layout;
  memset(out, 0, layout * node->output.size * sizeof(sample_t));
}
