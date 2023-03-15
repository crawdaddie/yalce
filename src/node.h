#ifndef _NODE_H
#define _NODE_H
#include <stdio.h>

typedef struct Node (*node_perform)(struct Node *node, int nframes, double spf);

typedef struct Node {
  node_perform (*perform)(struct Node *node, int nframes, double spf);
  void *object;
  const char *name;
  struct Node *next;
} Node;

node_perform perform_graph(struct Node *head, int nframes, double spf);

Node *make_node();
Node *alloc_node(size_t obj_size, const char *name);
#endif
