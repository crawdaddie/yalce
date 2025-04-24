#ifndef _ENGINE_NODE_H
#define _ENGINE_NODE_H

#include "common.h"
#include <stdbool.h>
#include <stddef.h>

typedef void *(*perform_func_t)(void *ptr, void *state, void *inputs,
                                int nframes, double spf);

// Buffer / Signal information
typedef struct {
  int layout;  // Number of channels in the buffer
  int size;    // Buffer capacity in frames
  double *buf; // Pointer to actual buffer data
} Signal;

typedef struct {
  int source_node_index; // Index of source node in graph
  int input_index;       // Which input slot this connects to
} Connection;

typedef struct {
  perform_func_t perform; // Node processing function
  int frame_offset;
  int node_index;                     // Position in the graph array
  int num_inputs;                     // Number of inputs this node has
  Connection connections[MAX_INPUTS]; // Input connections
  Signal output;                      // Output buffer
  int state_size;                     // Size of node-specific state
  int state_offset;                   // Offset to state in state memory pool
  int write_to_output;
  bool trig_end;
  struct Node *next; // For execution ordering
  char *meta;
  void *state_ptr;
  struct Node *bus;
} Node;

typedef Node *NodeRef;
typedef Signal *SignalRef;
typedef Node *Synth;

void offset_node_bufs(Node *node, int frame_offset);
void unoffset_node_bufs(Node *node, int frame_offset);
#endif
