#include "node.h"
#include <stdlib.h>

// ----------------------------- Node alloc
Node *node_new(void *data, node_perform *perform, Signal *ins, Signal *out) {
  Node *node = malloc(sizeof(Node));
  node->state = data;
  node->ins = &ins;
  node->num_ins = 1;
  node->out = out;
  node->perform = (node_perform)perform;
  return node;
}

Node *pipe_output(Node *send, Node *recv) {
  recv->ins = &send->out;
  return recv;
}

Node *pipe_sig(Signal *send, Node *recv) {
  recv->ins = &send;
  return recv;
}
Node *pipe_output_to_idx(int idx, Node *send, Node *recv) {
  recv->ins[idx] = send->out;
  return recv;
}

Node *pipe_sig_to_idx(int idx, Signal *send, Node *recv) {
  recv->ins[idx] = send;
  return recv;
}

Node *add_to_dac(Node *node) {
  node->type = OUTPUT;
  return node;
}
Node *chain_set_out(Node *chain, Node *out) {
  chain->out = out->out;
  return chain;
}

Node *chain_new() { return node_new(NULL, NULL, &(Signal){}, &(Signal){}); }

Node *chain_with_inputs(int num_ins, double *defaults) {
  Node *chain = node_new(NULL, NULL, NULL, &(Signal){});

  chain->ins = malloc(num_ins * (sizeof(Signal *)));
  chain->num_ins = num_ins;
  for (int i = 0; i < num_ins; i++) {
    chain->ins[i] = get_sig_default(1, defaults[i]);
  }
  return chain;
}

Node *node_set_input_signal(Node *node, int num_in, Signal *sig) {
  node->ins[num_in] = sig;
  return node;
}

Node *add_to_chain(Node *chain, Node *node) {
  if (chain->head == NULL) {
    chain->head = node;
    chain->tail = node;
    return node;
  }

  chain->tail->next = node;
  chain->tail = node;
  return node;
}
