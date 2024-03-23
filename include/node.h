#ifndef _NODE_H
#define _NODE_H
#include "signal.h"
#include <stdbool.h>

typedef struct Node (*node_perform)(struct Node *node, int nframes, double spf);
typedef struct Node {
  enum {
    INTERMEDIATE = 0,
    OUTPUT,
    GROUP,
  } type;

  bool killed;

  node_perform perform;
  void *state;
  Signal **ins;
  int num_ins;

  Signal *out;

  struct Node *prev;
  struct Node *next;
  int frame_offset;
  char *name;
} Node;

static char *node_type_names[3] = {
    [INTERMEDIATE] = "m", [OUTPUT] = "~", [GROUP] = "g"};

typedef struct {
  Node *head;
  Node *tail;
} Graph;

void init_sig_ptrs();

Signal *get_sig(int layout);
Node *node_new(void *data, node_perform *perform, Signal *ins, Signal *out);

Node *pipe_output(Node *send, Node *recv);

Node *pipe_sig(Signal *send, Node *recv);
Node *pipe_output_to_idx(int idx, Node *send, Node *recv);
Node *pipe_sig_to_idx(int idx, Signal *send, Node *recv);

Node *add_to_dac(Node *node);

typedef struct {
  Graph *graph;
} group_state;

Node *group_new();

Node *chain_set_out(Node *chain, Node *out);

Node *chain_new();

Node *chain_with_inputs(int num_ins, double *defaults);

Node *node_set_input_signal(Node *node, int num_in, Signal *sig);
Node *add_to_chain(Node *chain, Node *node);

void node_add_after(Node *tail, Node *node);
void node_add_before(Node *head, Node *node);
void graph_add_tail(Graph *graph, Node *node);
void graph_add_head(Graph *graph, Node *node);
void graph_delete_node(Graph *graph, Node *node);

void graph_print(Graph *dll);

#endif
