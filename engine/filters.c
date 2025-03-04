#include "./filters.h"
#include "signals.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  double gain;
} tanh_state;

void *tanh_perform(Node *node, int nframes, double spf) {
  printf("tanh perform\n");
  tanh_state *state = get_node_state(node);
  double *out = node->out.buf;
  Signal in = *get_node_input(node, 0);
  while (nframes--) {
    *out = tanh(*get_val(&in) * state->gain);
    out++;
  }
  return (char *)node + node->node_size;
}

Node *tanh_node(double gain, Signal *input) {
  // Allocate memory for node
  Node *node = node_new();
  tanh_state *state = (tanh_state *)state_new(sizeof(tanh_state));

  // Calculate offset from node to input
  int in_offset = (char *)input - (char *)node;

  // Initialize node
  *node = (Node){
      .num_ins = 1,
      .input_offsets = {in_offset},
      .node_size = sizeof(Node) + sizeof(tanh_state),
      .out = {.size = input->size,
              .layout = input->layout,
              .buf = malloc(sizeof(double) * input->size * input->layout)},
      .node_perform = (perform_func_t)tanh_perform,
      .next = NULL};

  // Allocate and initialize state
  *state = (tanh_state){gain};
  return node;
}
