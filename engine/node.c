#include "./node.h"
#include "./common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BlobTemplate *_current_blob = NULL;

void *node_alloc(void **mem, size_t size) {
  if (!mem) {
    // printf("malloc instead\n");
    return malloc(size);
  }
  void *ptr = *mem;
  *mem = (char *)(*mem) + size;
  return ptr;
}

perform_func_t get_node_perform(void *node) {
  return *(perform_func_t *)((char *)node + sizeof(int) + sizeof(int) +
                             sizeof(int) * MAX_INPUTS + sizeof(Signal));
}

void *get_node_state(void *node) { return (char *)node + sizeof(Node); }

void *create_new_blob_template() {
  // Allocate memory for the template
  BlobTemplate *template = malloc(sizeof(BlobTemplate));
  *template = (BlobTemplate){0};
  printf("create new blob template %p\n", template);
  return template;
}

BlobTemplate *start_blob(char *base_memory) {
  printf("started blob\n");

  BlobTemplate *template = malloc(sizeof(BlobTemplate));
  *template = (BlobTemplate){
      .blob_data = base_memory,
      ._mem_ptr = base_memory,
      .first_node_offset = -1 // sentinel value before any nodes get added
      // a node within the blob will always have a positive offset and anything
      // else (eg Signal) is always larger than a byte
  };
  _current_blob = template;

  return template;
}

BlobTemplate *end_blob(Node *end) {
  BlobTemplate *template = _current_blob;
  char *mem = template->blob_data;

  // Calculate the total size of the blob
  template->total_size = (char *)template->_mem_ptr - (char *)mem;

  // Allocate memory for the blob data
  template->blob_data = malloc(template->total_size);
  template->last_node_offset = (char *)end - (char *)mem;
  memcpy(template->blob_data, mem, template->total_size);

  _current_blob = NULL;
  return template;
}

Node *group_new(int ins) {
  // group_state *graph = malloc(sizeof(group_state));
  //
  // Node *node = malloc(sizeof(Node));
  // node->state = graph;
  // node->num_ins = ins;
  // node->ins = ins == 0 ? NULL : malloc(sizeof(Signal) * ins);
  //
  // for (int i = 0; i < ins; i++) {
  //   node->ins[i].buf = calloc(BUF_SIZE, sizeof(double));
  //   node->ins[i].size = BUF_SIZE;
  //   node->ins[i].layout = 1;
  // }
  //
  // node->out.layout = 1;
  // node->out.size = BUF_SIZE;
  // node->out.buf = calloc(BUF_SIZE, sizeof(double));
  // node->perform = (node_perform)group_perform;
  // node->frame_offset = 0;

  Node *node = malloc(sizeof(Node));
  return node;
}
void group_add_tail(Node *chain, Node *node) {}

Node *node_new() {
  if (!_current_blob) {
    Node *node = malloc(sizeof(Node));
    printf("new node %p\n", node);
    return node;
  }

  Node *n = (Node *)(_current_blob->_mem_ptr);
  _current_blob->_mem_ptr = (char *)_current_blob->_mem_ptr + sizeof(Node);
  if (_current_blob->first_node_offset == -1) {

    _current_blob->first_node_offset =
        (char *)n - (char *)_current_blob->blob_data;
    printf("_current_blob first node offset %d\n",
           _current_blob->first_node_offset);
  }
  return n;
}

char *state_new(size_t size) {
  if (!_current_blob) {
    return malloc(size);
  }
  char *state = _current_blob->_mem_ptr;
  _current_blob->_mem_ptr = (char *)_current_blob->_mem_ptr + size;
  return state;
}

typedef struct {
  int total_size;
  int num_ins;
  void *blob_start;
  void *blob_end;
  int input_slot_offsets[MAX_INPUTS];
} blob_state;

void *blob_perform(Node *node, int nframes, double spf) {
  blob_state *state = (blob_state *)((char *)node + sizeof(Node));

  // Get the start and end pointers
  void *current = state->blob_start;
  void *end = state->blob_end;

  // Make sure we have valid pointers
  if (!current || !end) {
    printf("Error: Invalid blob start or end pointer\n");
    return (char *)node + node->node_size;
  }

  int node_idx = 0;
  // Process all nodes in the blob
  while (current <= end) {
    Node *node = current;
    // perform_func_t perform = node->node_perform;

    // Check for null function pointer
    if (!node->node_perform) {
      printf("Error: NULL perform function at %p\n", current);
      break;
    }

    current = node->node_perform(node, nframes, spf);

    node_idx++;
  }

  // Copy the output from the last node (blob_end)
  if (end) {
    node->out = ((Node *)end)->out;
  }

  return (char *)node + node->node_size;
}
// Instantiate a blob from a template
Node *instantiate_blob_template(BlobTemplate *template) {
  char *_mem = malloc(sizeof(Node) + sizeof(blob_state) + template->total_size);

  // Allocate memory for node
  Node *blob_node = (Node *)_mem;
  _mem += sizeof(Node);

  // Initialize node
  *blob_node = (Node){.num_ins = 0,
                      .node_size = sizeof(Node) + sizeof(blob_state) +
                                   template->total_size,
                      .node_perform = (perform_func_t)blob_perform,
                      .next = NULL};

  // Allocate and initialize state
  blob_state *state = (blob_state *)_mem;
  _mem += sizeof(blob_state);

  char *blob_data = _mem;

  memcpy(blob_data, template->blob_data, template->total_size);

  state->blob_start = blob_data + template->first_node_offset;
  state->blob_end = blob_data + template->last_node_offset;
  state->total_size = template->total_size;
  state->num_ins = template->num_inputs;
  for (int i = 0; i < state->num_ins; i++) {
    state->input_slot_offsets[i] = template->input_slot_offsets[i];
    Signal *sig = (Signal *)((char *)blob_data + state->input_slot_offsets[i]);
    *sig = (Signal){.size = BUF_SIZE, .layout = 1};
    sig->buf = malloc(sizeof(double) * sig->size * sig->layout);
    for (int j = 0; j < sig->size * sig->layout; j++) {
      sig->buf[j] = template->default_vals[i];
    }

    blob_node->input_offsets[i] = (char *)sig - (char *)blob_node;
  }
  return blob_node;
}
struct double_list {
  double val;
  struct double_list *next;
};

NodeRef instantiate_blob_template_w_args(void *args, BlobTemplate *template) {
  char *_mem = malloc(sizeof(Node) + sizeof(blob_state) + template->total_size);

  // Allocate memory for node
  Node *blob_node = (Node *)_mem;
  _mem += sizeof(Node);

  // Initialize node
  *blob_node = (Node){.num_ins = 0,
                      .node_size = sizeof(Node) + sizeof(blob_state) +
                                   template->total_size,
                      .node_perform = (perform_func_t)blob_perform,
                      .next = NULL};

  // Allocate and initialize state
  blob_state *state = (blob_state *)_mem;
  _mem += sizeof(blob_state);

  char *blob_data = _mem;

  memcpy(blob_data, template->blob_data, template->total_size);

  state->blob_start = blob_data + template->first_node_offset;
  state->blob_end = blob_data + template->last_node_offset;
  state->total_size = template->total_size;
  state->num_ins = template->num_inputs;

  struct double_list *l = args;
  for (int i = 0; i < state->num_ins; i++) {
    state->input_slot_offsets[i] = template->input_slot_offsets[i];
    Signal *sig = (Signal *)((char *)blob_data + state->input_slot_offsets[i]);
    *sig = (Signal){.size = BUF_SIZE, .layout = 1};
    sig->buf = malloc(sizeof(double) * sig->size * sig->layout);
    for (int j = 0; j < sig->size * sig->layout; j++) {
      sig->buf[j] = l->val;
    }
    blob_node->input_offsets[i] = (char *)sig - (char *)blob_node;
    l = l->next;
  }
  return blob_node;
}
