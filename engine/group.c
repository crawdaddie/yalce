#include "group.h"
#include "audio_graph.h"
#include "ext_lib.h"
#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NodeRef group_add(NodeRef node, NodeRef group) {

  ensemble_state *ctx = group + 1;

  // Add to existing chain
  if (ctx->head == NULL) {
    ctx->head = node;
  } else {
    // Find the end of the chain
    Node *current = ctx->head;
    while (current->next != NULL) {
      current = current->next;
    }
    // Append to the end
    current->next = node;
  }
  return node;
}

void *perform_ensemble(Node *node, ensemble_state *state, Node *_inputs[],
                       int nframes, double spf) {

  for (int i = 0; i < 2 * BUF_SIZE; i++) {
    node->output.buf[i] = 0.;
  }

  if (!state->head) {
    return node->output.buf;
  }
  // printf("perform group %p\n", node);
  perform_graph(state->head, nframes, spf, node->output.buf, 2, 0);
  return node->output.buf;
}

NodeRef group_node() {
  int mem_size =
      sizeof(Node) + sizeof(ensemble_state) + (sizeof(double) * BUF_SIZE * 2);
  char *mem = malloc(mem_size);

  Node *node = (Node *)mem;
  mem += sizeof(Node);

  ensemble_state *state = (ensemble_state *)mem;
  mem += sizeof(ensemble_state);
  *state = (ensemble_state){NULL, NULL};

  double *buf = (double *)mem;
  for (int i = 0; i < 2 * BUF_SIZE; i++) {
    buf[i] = 0.;
  }

  *node = (Node){
      .perform = (perform_func_t)perform_ensemble,
      .num_inputs = 0,
      .output = (Signal){.layout = 2, .size = BUF_SIZE, .buf = buf},
      .write_to_output = true,
      .meta = "ensemble",
      .next = NULL,
      .state_ptr = state,
      .state_size = mem_size,
  };
  return node;
}
