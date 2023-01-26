#ifndef _GRAPH_H
#define _GRAPH_H
#include "../audio/signal.c"
#include <stdio.h>
#include <stdlib.h>

typedef struct Graph {
  struct Graph *next;
  struct Graph *prev;
  struct Graph *_graph;

  void (*perform)(struct Graph *node, int nframes, double seconds_per_frame);

  void *data;
  int schedule;

  /* double **out; */
  int chan_offset;

  double *out;
  int num_outs;

  Signal *in;
  int num_ins;

} Graph;

void debug_node(Graph *node, char *text) {
  if (text) {
    printf("%s\n", text);
  }
  printf("\tnode &: %#08x\n", node);
  /* printf("\tnode name: %s\n", node->name); */
  printf("\tnode perform: %#08x\n", node->perform);
  printf("\tnode next: %#08x\n", node->next);
  printf("\tnode prev: %#08x\n", node->next);
  printf("\tnode size: %d\n", sizeof(*node));
  /* printf("\tnode out: %#08x\n", node->out); */
  /* printf("\tnode schedule: %d\n", node->schedule); */
  /* printf("node add: %#08x\n", node->add); */
}

void debug_graph(Graph *graph) {
  debug_node(graph, NULL);

  if (graph->next) {
    printf("↓\n");
    return debug_graph(graph->next);
  };
  printf("----------\n");
}

Graph *new_graph() { return malloc(sizeof(Graph)); }

Graph *add_before(Graph *graph_node, Graph *new_node) {
  if (!graph_node) {
    graph_node = new_node;
    return new_node;
  }

  Graph *prev = graph_node->prev;
  if (prev) {
    prev->next = new_node;
  };
  new_node->next = graph_node;
  graph_node->prev = new_node;
  return new_node;
}

void remove_graph(Graph *node) {
  Graph *prev = node->prev;
  Graph *next = node->next;
  prev->next = next;
  next->prev = prev;
  free(node);
}

Graph *add_after(Graph *graph_node, Graph *new_node) {
  if (!graph_node) {
    graph_node = new_node;
    return graph_node;
  };
  Graph *next = graph_node->next;
  if (next) {
    new_node->next = next;
  };
  new_node->prev = graph_node;
  graph_node->next = new_node;
  return new_node;
}
Graph *add_into(Graph *graph_node, Graph *new_node) {
  if (graph_node->_graph) {
    add_after(graph_node->_graph, new_node);
    return new_node;
  }
  graph_node->_graph = new_node;
  return new_node;
}

void perform_null() {}

int graph_schedule(Graph *graph, int frame) {
  if (frame < graph->schedule) {
    return 0;
  };
  graph->schedule = -1;
  return 1;
}

Graph *perform_graph(Graph *graph, int nframes, double seconds_per_frame) {
  if (!graph) {
    return NULL;
  };

  if (graph->perform) {
    graph->perform(graph, nframes, seconds_per_frame);
  }

  if (graph->_graph) {
    graph->_graph->perform(graph->_graph, nframes,
                           seconds_per_frame); // recurse subgraph
  };

  Graph *next = graph->next;
  if (next) {
    return perform_graph(next, nframes,
                         seconds_per_frame); // keep going until you return tail
  };
  return graph;
}

Graph *set_func(Graph *graph, void (*perform)(Graph *node, int nframes,
                                              double seconds_per_frame)) {
  graph->perform = perform;
  return graph;
}

void iterate_graphs(Graph *head, void (*cb)(Graph *graph)) {
  if (!head) {
    return;
  }
  cb(head);
  return iterate_graphs(head->next, cb);
}
typedef struct Group {
  Graph *head;
  Graph *tail;
  Signal *in;
  int num_ins;
} Group;

Group group(Graph *after, double *out,
            Graph *(*synths)(Graph *tail, double *out)) {
  if (after == NULL) {
    *after = *new_graph();
  }
  Graph *tail = synths(after, out);
  return (Group){.head = after->next, .tail = tail};
}

/* void write_to_graph_out(Graph *graph, double *sample, int frame) { */
/*   for (int ch = 0; ch < graph->num_outs; ch++) { */
/*     double *out = graph->out + ch; */
/*     *(out + frame) = *sample; */
/*   } */
/* } */

#endif
