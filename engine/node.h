#ifndef _ENGINE_NODE_H
#define _ENGINE_NODE_H

#include "common.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void *(*perform_func_t)(void *ptr, void *state, void *inputs,
                                int nframes, double spf);
typedef void (*frame_perform_func_t)(void *ptr, void *state, void *inputs,
                                     int frame, double spf);

typedef enum {
  NODE_KIND_BLOCK = 0,
  NODE_KIND_AUDIO_GRAPH,
  NODE_KIND_GROUP,
  NODE_KIND_SUMMED_INLET,
  NODE_KIND_FRAME,
} NodeKind;

// Buffer / Signal information
typedef struct {
  int layout;  // Number of channels in the buffer
  int size;    // Buffer capacity in frames
  double *buf; // Pointer to actual buffer data
} Signal;

typedef struct {
  uint64_t
      source_node_index; // Index of source node in graph - can be a raw pointer
  int input_index;       // Which input slot this connects to
} Connection;

typedef struct Node {
  perform_func_t perform; // Node processing function
  frame_perform_func_t frame_perform;
  NodeKind kind;
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
