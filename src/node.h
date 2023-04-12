#ifndef _NODE_H
#define _NODE_H
#include "audio/signal.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct Node (*node_perform)(struct Node *node, int nframes, double spf);

typedef struct Node {
  Signal *ins;
  int num_ins;

  Signal add;
  Signal mul;
  Signal out;

  node_perform (*perform)(struct Node *node, int nframes, double spf);
  void *data;
  const char *name;

  bool killed;

  struct Node *next;
  struct Node *prev;
  /* struct Node *parent; */
  struct Node *_sub;
  struct Node *
      _sub_tail; // optional pointer to a node before the add_out or replace_out
                 // node found at the end of a Container Node's signal chain
} Node;

#define INS(node) (node)->ins
#define IN(node, enum_name) (INS(node)[enum_name])
#define OUTS(node) node->out
/* + node->num_ins */

#define NUM_INS(node) (node)->num_ins

node_perform perform_graph(struct Node *head, int nframes, double spf);

Node *alloc_node(size_t obj_size, const char *name);

Node *make_node(size_t obj_size, node_perform perform, const char *name);

#define ALLOC_NODE(type, name) alloc_node(sizeof(type), name)
#define NODE_DATA(type, node) ((type *)node->data)
#define MAKE_NODE(type, perform, name) make_node(sizeof(type), perform, name)

Node *node_add_after(Node *before, Node *after);

typedef struct {
  bool write_to_output;
} container_node_data;
Node *container_node(Node *sub);

Node *chain_nodes(Node *container, Node *filter, int dest_sig_idx);
Node *node_write_out(Node *node, int frame, double sample);
#endif
