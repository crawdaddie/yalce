#include "./node.h"
#include "./common.h"
#include <math.h>
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

Signal *get_node_input(Node *node, int input) {
  return (void *)((char *)node + (node->input_offsets[input]));
}

double *get_val(Signal *in) {
  if (in->size == 1) {
    return in->buf;
  }
  double *ptr = in->buf;
  in->buf += in->layout;
  return ptr;
}

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
    if (_chain == NULL) {
      _chain = group_new(0);
    } else {
      group_add_tail(_chain, node);
    }

    return node;
  }

  Node *n = (Node *)(_current_blob->_mem_ptr);
  _current_blob->_mem_ptr = (char *)_current_blob->_mem_ptr + sizeof(Node);
  if (_current_blob->first_node_offset == -1) {
    _current_blob->first_node_offset =
        (char *)n - (char *)_current_blob->blob_data;
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

  // Process all nodes in the blob
  while (current <= end) {
    // Get the node's perform function
    perform_func_t perform = ((Node *)current)->node_perform;

    // Check for null function pointer
    if (!perform) {
      printf("Error: NULL perform function at %p\n", current);
      break;
    }

    // Execute the node's perform function
    current = perform(current, nframes, spf);

    // Check if we've gone past the end
    if (current > end) {
      break;
    }
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
    sig->buf = malloc(sizeof(double) * sig->size * sig->layout);
    for (int j = 0; j < sig->size * sig->layout; j++) {
      sig->buf[j] = template->default_vals[i];
    }
  }
  return blob_node;
}

// -- real node instances
typedef struct sin_state {
  double phase;
} sin_state;

#define SIN_TABSIZE (1 << 11)
static double sin_table[SIN_TABSIZE];

void maketable_sin(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SIN_TABSIZE;

  for (int i = 0; i < SIN_TABSIZE; i++) {
    double val = sin(phase);
    sin_table[i] = val;
    phase += phsinc;
  }
}

void *sin_perform(Node *node, int nframes, double spf) {
  sin_state *state = get_node_state(node);
  Signal in = *get_node_input(node, 0);

  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double *freq;

  int out_layout = node->out.layout;
  double *out = node->out.buf;
  while (nframes--) {
    freq = get_val(&in);
    d_index = state->phase * SIN_TABSIZE;
    index = (int)d_index;
    frac = d_index - index;

    a = sin_table[index % SIN_TABSIZE];
    b = sin_table[(index + 1) % SIN_TABSIZE];

    sample = (1.0 - frac) * a + (frac * b);
    for (int i = 0; i < out_layout; i++) {
      *out = sample;
      out++;
    }
    state->phase = fmod((*freq) * spf + (state->phase), 1.0);
  }
  return (char *)node + node->node_size;
}

Node *sin_node(Signal *input) {
  Node *node = node_new();
  sin_state *state = (sin_state *)state_new(sizeof(sin_state));

  int in_offset = (char *)input - (char *)node;
  *node = (Node){.num_ins = 1,
                 .input_offsets = {in_offset},
                 .node_size = sizeof(Node) + sizeof(sin_state),
                 .out = {.size = BUF_SIZE,
                         .layout = 1,
                         .buf = malloc(sizeof(double) * BUF_SIZE)},
                 .node_perform = (perform_func_t)sin_perform,
                 .next = NULL};

  // sin_state *state = node_alloc(mem, sizeof(sin_state));
  *state = (sin_state){0.};

  return node;
}

typedef struct {
  double gain;
} tanh_state;

void *tanh_perform(Node *node, int nframes, double spf) {
  tanh_state *state = get_node_state(node);
  double *out = node->out.buf;
  Signal in = *get_node_input(node, 0);
  while (nframes--) {
    *out = tanh(*get_val(&in) * state->gain);
    out++;
  }
  return (char *)node + node->node_size;
}

Node *tanh_node(double gain, Signal *input) {
  // Allocate memory for node
  Node *node = node_new();
  tanh_state *state = (tanh_state *)state_new(sizeof(tanh_state));

  // Calculate offset from node to input
  int in_offset = (char *)input - (char *)node;

  // Initialize node
  *node = (Node){
      .num_ins = 1,
      .input_offsets = {in_offset},
      .node_size = sizeof(Node) + sizeof(tanh_state),
      .out = {.size = input->size,
              .layout = input->layout,
              .buf = malloc(sizeof(double) * input->size * input->layout)},
      .node_perform = (perform_func_t)tanh_perform,
      .next = NULL};

  // Allocate and initialize state
  *state = (tanh_state){gain};
  return node;
}

#define SQ_TABSIZE (1 << 11)
static double sq_table[SQ_TABSIZE] = {0};
void maketable_sq(void) {
  double phase = 0.0;
  double phsinc = (2. * PI) / SQ_TABSIZE;
  for (int i = 0; i < SQ_TABSIZE; i++) {

    for (int harm = 1; harm < SQ_TABSIZE / 2;
         harm += 2) { // summing odd frequencies

      // for (int harm = 1; harm < 100; harm += 2) { // summing odd frequencies
      double val = sin(phase * harm) / harm; // sinewave of different frequency
      sq_table[i] += val;
    }
    phase += phsinc;
  }
}
