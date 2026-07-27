#ifndef _ENGINE_NODE_H
#define _ENGINE_NODE_H

#include "common.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*frame_perform_func_t)(void *ptr, void *state, void *inputs,
                                     int frame, double spf);

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
  frame_perform_func_t frame_perform;
  int frame_offset;
  int num_inputs;                     // Number of inputs this node has
  Connection connections[MAX_INPUTS]; // Input connections
  Signal output;                      // Output buffer
  int state_size;                     // Size of node-specific state
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

#endif
