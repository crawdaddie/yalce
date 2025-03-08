#include "./node.h"
#include "./common.h"
#include "perform.h"
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
char *get_node_out_sig(void *node, size_t state_size) {
  return (char *)node + sizeof(Node) + state_size;
}

void *create_new_blob_template() {
  // Allocate memory for the template
  BlobTemplate *template = malloc(sizeof(BlobTemplate));
  *template = (BlobTemplate){0};
  return template;
}

BlobTemplate *start_blob(char *base_memory) {

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
void debug_blob(unsigned char *b, int size) {

  // Debug output in hex format
  printf("Debug blob (total size: %d bytes)\n", size);

  // Print header for hex dump
  printf(
      "Offset     | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F | ASCII\n");
  printf(
      "-----------|-------------------------------------------------|--------"
      "--------\n");

  for (int i = 0; i < size; i += 16) {
    // Print offset
    printf("0x%08x | ", i);

    // Print hex values
    for (int j = 0; j < 16; j++) {
      if (i + j < size) {
        printf("%02x ", b[i + j]);
      } else {
        printf("   "); // Padding for incomplete line
      }
    }

    // Print ASCII representation
    printf("| ");
    for (int j = 0; j < 16; j++) {
      if (i + j < size) {
        // Print printable ASCII characters, otherwise print a dot
        if (b[i + j] >= 32 && b[i + j] <= 126) {
          printf("%c", b[i + j]);
        } else {
          printf(".");
        }
      } else {
        printf(" "); // Padding for incomplete line
      }
    }

    printf("\n");
  }
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
  debug_blob(template->blob_data, template->total_size);

  printf("Created aligned blob of size %d bytes, first node offset: %d, last "
         "node offset: %d\n",
         template->total_size, template->first_node_offset,
         template->last_node_offset);
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

Node *__node_new() {
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

char *__state_new(size_t size) {
  if (!_current_blob) {
    return malloc(size);
  }
  char *state = _current_blob->_mem_ptr;
  _current_blob->_mem_ptr = (char *)_current_blob->_mem_ptr + size;
  return state;
}
#define MEMORY_ALIGNMENT 8

// Helper function to align a pointer to the specified boundary
static inline void *align_pointer(void *ptr, size_t alignment) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
  return (void *)aligned;
}

// Function to calculate size with alignment padding
size_t aligned_size(size_t size) {
  return (size + MEMORY_ALIGNMENT - 1) & ~(MEMORY_ALIGNMENT - 1);
}

Node *node_new() {
  if (!_current_blob) {

    Node *node = malloc(sizeof(Node));
    printf("new node %p\n", node);
    return node;
  }

  // Inside blob: align the memory pointer before allocation
  _current_blob->_mem_ptr =
      align_pointer(_current_blob->_mem_ptr, MEMORY_ALIGNMENT);

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
    // When allocating outside the blob, use aligned_alloc if available
    return malloc(size);
  }

  // Inside blob: align the memory pointer before allocation
  _current_blob->_mem_ptr =
      align_pointer(_current_blob->_mem_ptr, MEMORY_ALIGNMENT);

  char *state = _current_blob->_mem_ptr;
  _current_blob->_mem_ptr = (char *)_current_blob->_mem_ptr + size;
  return state;
}

char *node_out_buf_new(size_t len, int layout) {
  size_t size = sizeof(double) * len * layout;
  if (!_current_blob) {
    // When allocating outside the blob, use aligned_alloc if available
    return malloc(size);
  }

  // Inside blob: align the memory pointer before allocation
  _current_blob->_mem_ptr =
      align_pointer(_current_blob->_mem_ptr, MEMORY_ALIGNMENT);

  char *buf = _current_blob->_mem_ptr;
  _current_blob->_mem_ptr = (char *)_current_blob->_mem_ptr + size;
  return buf;
}

struct double_list {
  double val;
  struct double_list *next;
};

void *blob_perform(Node *node, int nframes, double spf) {

  // printf("blob perform %p\n", node);
  blob_state *state = (blob_state *)((char *)node + sizeof(Node));

  // Get the start and end pointers
  char *current = state->blob_start;
  char *end = state->blob_end;

  // Make sure we have valid pointers
  if (!current || !end) {
    printf("Error: Invalid blob start or end pointer\n");
    return (char *)node + node->node_size;
  }

  int frame_offset = node->frame_offset;
  int node_idx = 0;
  // Process all nodes in the blob
  while (current <= end) {
    Node *cb = current;
    // printf("blob node> %d offset %ld\n", node_idx,
    //        ((char *)current - (char *)(state->blob_start)));

    // Check for null function pointer
    if (!cb->node_perform) {
      printf("Error: NULL perform function at %p\n", current);
      break;
    }

    set_out_buf(cb);
    cb->frame_offset = frame_offset;
    offset_node_bufs(cb, frame_offset);
    current = cb->node_perform(cb, nframes - frame_offset, spf);
    unoffset_node_bufs(cb, frame_offset);

    // Verify alignment of returned pointer
    if ((uintptr_t)current % MEMORY_ALIGNMENT != 0) {
      printf("Warning: Node perform returned unaligned pointer: %p\n", current);
      // Fix alignment if needed
      current = align_pointer(current, MEMORY_ALIGNMENT);
    }

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
  // Calculate aligned sizes for proper memory layout
  size_t node_size = aligned_size(sizeof(Node));
  size_t blob_state_size = aligned_size(sizeof(blob_state));
  size_t total_required_size =
      node_size + blob_state_size + template->total_size;

  // Allocate aligned memory for the entire structure
  char *_mem = malloc(total_required_size);
  if ((uintptr_t)_mem % MEMORY_ALIGNMENT != 0) {
    printf("Warning: malloc returned unaligned pointer: %p\n", _mem);
  }

  // Allocate memory for node
  Node *blob_node = (Node *)_mem;
  _mem += node_size; // Use aligned size for offset

  // Initialize node
  *blob_node = (Node){.num_ins = 0,
                      .node_size = total_required_size,
                      .node_perform = (perform_func_t)blob_perform,
                      .next = NULL};

  // Allocate and initialize state
  blob_state *state = (blob_state *)_mem;
  _mem += blob_state_size; // Use aligned size for offset

  // Ensure blob data pointer is aligned
  _mem = align_pointer(_mem, MEMORY_ALIGNMENT);
  char *blob_data = _mem;

  memcpy(blob_data, template->blob_data, template->total_size);

  state->blob_start = blob_data + template->first_node_offset;
  state->blob_end = blob_data + template->last_node_offset;
  state->total_size = template->total_size;
  state->num_ins = template->num_inputs;

  for (int i = 0; i < state->num_ins; i++) {
    state->input_slot_offsets[i] = template->input_slot_offsets[i];
    Signal *sig = (Signal *)((char *)blob_data + state->input_slot_offsets[i]);

    // Verify signal is aligned
    if ((uintptr_t)sig % MEMORY_ALIGNMENT != 0) {
      printf("Warning: Signal pointer is not aligned: %p\n", sig);
    }

    *sig = (Signal){.size = BUF_SIZE, .layout = 1};

    // Allocate aligned memory for signal buffer
    sig->buf = malloc(sizeof(double) * sig->size * sig->layout);

    for (int j = 0; j < sig->size * sig->layout; j++) {
      sig->buf[j] = template->default_vals[i];
    }

    blob_node->input_offsets[i] = (char *)sig - (char *)blob_node;
  }

  printf("Instantiated blob: node=%p, state=%p, blob_data=%p (aligned=%d)\n",
         blob_node, state, blob_data,
         ((uintptr_t)blob_data % MEMORY_ALIGNMENT == 0));

  return blob_node;
}

NodeRef instantiate_blob_template_w_args(void *args, BlobTemplate *template) {
  // Calculate aligned sizes for proper memory layout
  size_t node_size = aligned_size(sizeof(Node));
  size_t blob_state_size = aligned_size(sizeof(blob_state));
  // size_t input_buf_sizes = aligned_size(sizeof(double) * template->num_inputs * BUF_SIZE);  
  size_t input_buf_sizes = 0;

  size_t total_required_size =
      node_size + blob_state_size + template->total_size + input_buf_sizes;

  // Allocate aligned memory for the entire structure
  char *_mem = malloc(total_required_size);

  // Allocate memory for node
  Node *blob_node = (Node *)_mem;
  _mem += node_size; // Use aligned size for offset

  // Initialize node
  *blob_node = (Node){.num_ins = 0,
                      .node_size = total_required_size,
                      .node_perform = (perform_func_t)blob_perform,
                      .next = NULL};

  // Allocate and initialize state
  blob_state *state = (blob_state *)_mem;
  _mem += blob_state_size; // Use aligned size for offset

  // Ensure blob data pointer is aligned
  _mem = align_pointer(_mem, MEMORY_ALIGNMENT);
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

    // Verify signal is aligned
    if ((uintptr_t)sig % MEMORY_ALIGNMENT != 0) {
      printf("Warning: Signal pointer is not aligned: %p\n", sig);
    }

    *sig = (Signal){.size = BUF_SIZE, .layout = 1};

    // Allocate aligned memory for signal buffer
    sig->buf = (double *)malloc(sizeof(double) * sig->size * sig->layout);
    // _mem = (char* )_mem + (sizeof(double) * sig->size * sig->layout);

    for (int j = 0; j < sig->size * sig->layout; j++) {
      sig->buf[j] = l->val;
    }

    blob_node->input_offsets[i] = (char *)sig - (char *)blob_node;
    l = l->next;
  }

  return blob_node;
}
