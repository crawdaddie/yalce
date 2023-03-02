#ifndef _GRAPH_H
#define _GRAPH_H
#include "../audio/signal.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct Graph {
  struct Graph *next;
  struct Graph *prev;
  struct Graph *_graph;

  void (*perform)(struct Graph *node, int nframes, double seconds_per_frame);

  void *data;
  double schedule; // secs

  /* double **out; */
  int chan_offset;

  double *out;
  int num_outs;

  Signal *in;
  int num_ins;

} Graph;

void debug_node(Graph *node, char *text);

void debug_graph(Graph *graph);
Graph *new_graph();

Graph *add_before(Graph *graph_node, Graph *new_node);

void remove_graph(Graph *node);

Graph *add_after(Graph *graph_node, Graph *new_node);

Graph *add_into(Graph *graph_node, Graph *new_node);
void perform_null();

int graph_schedule(Graph *graph, int frame);

Graph *perform_graph(Graph *graph, int nframes, double seconds_per_frame);

Graph *set_func(Graph *graph, void (*perform)(Graph *node, int nframes,
                                              double seconds_per_frame));

void iterate_graphs(Graph *head, void (*cb)(Graph *graph));
typedef struct Group {
  Graph *head;
  Graph *tail;
  Signal *in;
  int num_ins;
} Group;

Group group(Graph *after, double *out,
            Graph *(*synths)(Graph *tail, double *out));
void pipe_graph(Graph *from, Graph *to);
#endif
