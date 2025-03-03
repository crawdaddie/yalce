#ifndef _ENGINE_NODE_H
#define _ENGINE_NODE_H

typedef struct Signal {
  int layout;
  int size;
  double *buf;
} Signal;

typedef void *(*perform_func_t)(void *ptr, int nframes, double spf);

#define MAX_INPUTS 16

typedef struct {
  int node_size;
  int num_ins;
  int input_offsets[MAX_INPUTS];
  Signal out;
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

#define MAX_INPUTS 16
// Definition for a blob template
typedef struct {
  int total_size;  // Total size of all nodes in the blob
  int num_inputs;  // Number of inputs the blob expects
  char *blob_data; // The actual compiled blob data
  int first_node_offset;
  int last_node_offset;
  int input_slot_offsets[MAX_INPUTS]; // Offsets for where inputs should connect
  double default_vals[MAX_INPUTS];    // Offsets for where inputs should connect
  char *_mem_ptr;
} BlobTemplate;

extern BlobTemplate *_current_blob;
void *create_new_blob_template();
BlobTemplate *start_blob(char *base_memory);
BlobTemplate *end_blob(Node *end);

NodeRef instantiate_blob_template(BlobTemplate *template);
#endif
