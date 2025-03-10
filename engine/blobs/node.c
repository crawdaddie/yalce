#include "./node.h"
#include "./common.h"
#include "alloc.h"
#include "perform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BlobTemplate *_current_blob = NULL;

perform_func_t get_node_perform(void *node) {
  return *(perform_func_t *)((char *)node + sizeof(int) + sizeof(int) +
                             sizeof(int) * MAX_INPUTS + sizeof(Signal));
}

void *get_node_state(void *node) { return (char *)node + sizeof(Node); }

char *blob_allocator(size_t size) {
  char *ptr = _current_blob->_mem_ptr;
  _current_blob->_mem_ptr = (char *)_current_blob->_mem_ptr + size;
  _current_blob->total_size += size;
  return ptr;
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
  set_allocator(blob_allocator);

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

void group_add_tail(Node *chain, Node *node) {}

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

void blob_register_node(Node *node) {
  if (_current_blob && _current_blob->first_node_offset == -1) {
    _current_blob->first_node_offset =
        (char *)node - (char *)_current_blob->blob_data;
  }
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

  return (char *)node + node->node_size;
}

NodeRef instantiate_blob_template_w_args(void *args, BlobTemplate *template) {
  // Calculate aligned sizes for proper memory layout
  size_t node_size = aligned_size(sizeof(Node));
  size_t blob_state_size = aligned_size(sizeof(blob_state));
  // size_t input_buf_sizes = aligned_size(sizeof(double) * template->num_inputs
  // * BUF_SIZE);
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
    state->input_slot_buf_offsets[i] = template->input_slot_offsets[i];
    Signal *sig =
        (Signal *)((char *)blob_data + state->input_slot_buf_offsets[i]);

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

    blob_node->input_offsets[i] = (char *)sig->buf - (char *)blob_node;
    l = l->next;
  }

  return blob_node;
}
