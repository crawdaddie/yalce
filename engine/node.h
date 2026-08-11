#ifndef _ENGINE_NODE_H
#define _ENGINE_NODE_H

#include "common.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*frame_perform_func_t)(void *ptr, void *state, void *inputs,
                                     int frame, double spf);
typedef void (*node_state_init_func_t)(void *state);

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
  node_state_init_func_t state_init;
  struct Node *bus;
  struct Node *alloc_next; // For host-owned allocation tracking (ylc_clap)
} Node;

typedef Node *NodeRef;
typedef Signal *SignalRef;
typedef Node *Synth;

/* Pluggable node allocator. A host (e.g. ylc_clap) can install its own
   allocator to own node memory/lifetimes instead of using the default
   calloc/free. When the installed allocator is NULL, node creation falls
   back to calloc/free so the standalone engine/audio_jit path is
   unchanged. */
typedef struct ylc_node_allocator {
  void *(*alloc)(size_t size, void *user_data);
  void (*free)(void *ptr, void *user_data);
  void *user_data;
} ylc_node_allocator_t;

const ylc_node_allocator_t *ylc_node_allocator_get(void);
void ylc_node_allocator_set(const ylc_node_allocator_t *allocator);

#endif