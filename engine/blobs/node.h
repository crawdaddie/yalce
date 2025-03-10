#ifndef _ENGINE_NODE_H
#define _ENGINE_NODE_H
#include <stdbool.h>
#include <stddef.h>

typedef struct Signal {
  int layout;
  int size;
  int buf_offset;
  double *buf;
} Signal;

typedef void *(*perform_func_t)(void *ptr, int nframes, double spf);

#define MAX_INPUTS 16

typedef struct {
  int node_size;
  int num_ins;
  int input_offsets[MAX_INPUTS];
  int input_sizes[MAX_INPUTS];
  int input_layouts[MAX_INPUTS];
  int frame_offset;
  bool write_to_dac;
  bool can_free;
  int output_buf_offset;
  int output_size;
  int output_layout;
  perform_func_t node_perform;
  struct Node *next;
} Node;

typedef Node *NodeRef;
typedef Signal *SignalRef;
typedef Node *Synth;

perform_func_t get_node_perform(void *node);
void *get_node_state(void *node);
extern NodeRef _chain_head;
extern NodeRef _chain_tail;
extern NodeRef _chain;

// Definition for a blob template
typedef struct {
  int total_size;  // Total size of all nodes in the blob
  int num_inputs;  // Number of inputs the blob expects
  char *blob_data; // The actual compiled blob data
  int first_node_offset;
  int last_node_offset;
  int input_slot_offsets[MAX_INPUTS]; // Offsets for where bufs of inputs
                                      // should connect
  double default_vals[MAX_INPUTS];    // Offsets for where inputs should connect
  //
  char *_mem_ptr;
} BlobTemplate;

extern BlobTemplate *_current_blob;
void *create_new_blob_template();
BlobTemplate *start_blob(char *base_memory);
BlobTemplate *end_blob(Node *end);

NodeRef instantiate_blob_template(BlobTemplate *template);
NodeRef instantiate_blob_template_w_args(void *args, BlobTemplate *template);

size_t aligned_size(size_t size);

typedef struct {
  int total_size;
  int num_ins;
  void *blob_start;
  void *blob_end;
  int input_slot_buf_offsets[MAX_INPUTS];
} blob_state;

void blob_register_node(Node *node);
#endif
