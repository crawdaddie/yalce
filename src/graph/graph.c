#include "graph.h"

void debug_node(Graph *node, char *text) {
  if (text) {
    printf("%s\n", text);
  }
  printf("\tnode &: %p\n", node);
  /* printf("\tnode name: %s\n", node->name); */
  printf("\tnode perform: %p\n", node->perform);
  printf("\tnode next: %p\n", node->next);
  printf("\tnode prev: %p\n", node->next);
  printf("\tnode size: %lu\n", sizeof(*node));
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
  free(node->data);
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
    perform_graph(graph->_graph, nframes, seconds_per_frame);
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

Group group(Graph *after, double *out,
            Graph *(*synths)(Graph *tail, double *out)) {
  if (after == NULL) {
    *after = *new_graph();
  }
  Graph *tail = synths(after, out);
  return (Group){.head = after->next, .tail = tail};
}

void pipe_graph(Graph *from, Graph *to) {
  add_after(from, to);
  to->in[0].data = from->out;
}

/* void write_to_graph_out(Graph *graph, double *sample, int frame) { */
/*   for (int ch = 0; ch < graph->num_outs; ch++) { */
/*     double *out = graph->out + ch; */
/*     *(out + frame) = *sample; */
/*   } */
/* } */
