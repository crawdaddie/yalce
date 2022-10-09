#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Graph{
  struct Graph *next;
  struct Graph *prev;
  struct Graph *_graph;
  void (*perform)(struct Graph *graph);
  
} Graph;
typedef void (*t_perform)(struct Graph *node);


Graph *graph_perform(Graph *graph) {
  if (!graph) {
    return NULL;
  };
  if (graph->_graph) {
    graph->perform(graph->_graph);
  };
  Graph *next = graph->next;
  if (next) {
    return graph_perform(next); // keep going until you return tail
  };
  return graph;
}

Graph *add_before(Graph *graph_node, Graph *new_node) {
  Graph *prev = graph_node->prev;
  if (prev) {
    prev->next = new_node;
  };
  new_node->next = graph_node;
  graph_node->prev = new_node;
  return new_node;
}

Graph *add_after(Graph *graph_node, Graph *new_node) {
  Graph *next = graph_node->next;
  if (next) {
    new_node->next = next;
  };
  new_node->prev = graph_node;
  graph_node->next = new_node;
  return new_node;
}
