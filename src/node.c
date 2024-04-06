#include "node.h"
#include "common.h"
#include "ctx.h"
#include "signal.h"
#include <stdio.h>
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
  free(recv->ins[idx]->buf);
  free(recv->ins[idx]);
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

Node *chain_new() { return node_new(NULL, NULL, &(Signal){}, get_sig(1)); }

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
  printf("set input sig %p\n", sig);
  Signal *old_sig = node->ins[num_in];
  node->ins[num_in] = sig;
  free(old_sig->buf);
  free(old_sig);
  return node;
}

void node_add_after(Node *tail, Node *node) {
  if (tail == NULL) {
    printf("Previous node cannot be NULL\n");
    return;
  }
  node->next = tail->next;
  if (tail->next != NULL) {
    tail->next->prev = node;
  }
  tail->next = node;
  node->prev = tail;
};

// Function to add a node before a particular node
void node_add_before(Node *head, Node *node) {
  if (head == NULL) {
    printf("Next node cannot be NULL\n");
    return;
  }
  node->prev = head->prev;
  if (head->prev != NULL) {
    head->prev->next = node;
  }
  head->prev = node;
  node->next = head;
}

void free_node(Node *node) {
  if (node->destroy != NULL) {
    return node->destroy(node);
  }
  if (node->num_ins) {
    free(node->ins);
  }
  free(node->state);
  free(node->out);
  free(node);
}
Signal *get_input_sig(Node *node, int i) { return node->ins[i]; }
Signal *get_output_sig(Node *node) { return node->out; }
