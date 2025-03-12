#ifndef _ENGINE_AUDIO_GRAPH_H
#define _ENGINE_AUDIO_GRAPH_H
#include "./node.h"
// Graph container
typedef struct {
  Node *nodes;          // Array of all nodes
  int node_count;       // Number of nodes in the graph
  int capacity;         // Maximum nodes in the graph
  double *buffer_pool;  // Pool of audio buffer memory
  int buffer_pool_size; // Size of buffer pool in doubles
  int buffer_pool_capacity;
  char *nodes_state_memory; // Pool of node state memory
  int state_memory_size;    // Size of state memory pool
  int state_memory_capacity;
  int inlets[MAX_INPUTS]; // index of nodes which are 'inlet' nodes
  double inlet_defaults[MAX_INPUTS];
  int num_inlets;
} AudioGraph;

double *allocate_buffer_from_pool(AudioGraph *graph, int size);
int state_offset_ptr_in_graph(AudioGraph *graph, int size);
char *state_ptr(AudioGraph *graph, NodeRef node);
Node *allocate_node_in_graph(AudioGraph *graph, int state_size);

void print_graph(AudioGraph *g);

void perform_audio_graph(Node *_node, AudioGraph *graph, Node *_inputs[],
                         int nframes, double spf);
extern AudioGraph *_graph;
#endif
