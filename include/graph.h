#ifndef _GRAPH_H
#define _GRAPH_H

#include "node.h"

typedef struct {
  Node *head;
  Node *tail;
} Graph;

Node *group_new(int chans);
void group_add_tail(Node *group, Node *node);

void graph_print(Graph *graph, int indent);

Graph *graph_add_tail(Graph *graph, Node *node);
Graph *graph_add_head(Graph *graph, Node *node);
Graph *graph_delete_node(Graph *graph, Node *node);

#endif
