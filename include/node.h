#ifndef _NODE_H
#define _NODE_H
#include "signal.h"
#include <stdbool.h>
typedef void (*node_destroy)(struct Node *node);
typedef struct Node (*node_perform)(struct Node *node, int nframes, double spf);
typedef struct Node {
  enum { INTERMEDIATE = 0, OUTPUT, KILLED } type;

  char *name;
  bool killed;
  bool is_group;
  int frame_offset;

  void *state;
  node_perform perform;
  int num_ins;
  Signal **ins;

  Signal *out;

  struct Node *prev;
  struct Node *next;
  struct Node *parent;
  node_destroy destroy;
} Node;

static char *node_type_names[3] = {
    [INTERMEDIATE] = "m", [OUTPUT] = "~", [KILLED] = "x"};

void init_sig_ptrs();

Signal *get_sig(int layout);
Node *node_new(void *data, node_perform *perform, Signal *ins, Signal *out);

Node *pipe_output(Node *send, Node *recv);

Node *pipe_sig(Signal *send, Node *recv);
Node *pipe_output_to_idx(int idx, Node *send, Node *recv);
Node *pipe_sig_to_idx(int idx, Signal *send, Node *recv);

Node *add_to_dac(Node *node);

Node *chain_set_out(Node *chain, Node *out);

Node *chain_new();

Node *chain_with_inputs(int num_ins, double *defaults);

Node *node_set_input_signal(Node *node, int num_in, Signal *sig);
Node *add_to_chain(Node *chain, Node *node);

void node_add_after(Node *tail, Node *node);
void node_add_before(Node *head, Node *node);

void free_node(Node *node);

Signal *get_input_sig(Node *node, int i);
Signal *get_output_sig(Node *node);
#endif
