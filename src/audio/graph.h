#ifndef _GRAPH
#define _GRAPH
#include <stdio.h>
#include <stdlib.h>

typedef struct Graph{
  struct Graph *next;
  struct Graph *prev;
  struct Graph *_graph;
  void (*perform)(struct Graph *graph);
  
} Graph;
typedef void (*t_perform)(struct Graph *node);


Graph *graph_perform(Graph *graph);
Graph *add_before(Graph *graph_node, Graph *new_node);
Graph *add_after(Graph *graph_node, Graph *new_node);

#endif
