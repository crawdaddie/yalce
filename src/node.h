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

Node *alloc_node(size_t obj_size, const char *name);

Node *make_node(size_t obj_size, node_perform perform, const char *name);

#define ALLOC_NODE(type, name) alloc_node(sizeof(type), name)
#define NODE_DATA(type, node) ((type *)node->object)

#define MAKE_NODE(type, perform, name) make_node(sizeof(type), perform, name)
#endif
