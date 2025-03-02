#include "./node.h"
#include "./common.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BlobTemplate *_current_blob = NULL;

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

#define ADD_NODE_TO_CURRENT_BLOB(create_func, ...)                             \
  if (!_current_blob) {                                                        \
    return create_func(NULL, __VA_ARGS__);                                     \
  } else {                                                                     \
    Node *s = create_func(((void **)(&_current_blob->_mem_ptr)), __VA_ARGS__); \
    if (_current_blob->first_node_offset == -1) {                              \
      _current_blob->first_node_offset =                                       \
          (char *)s - (char *)_current_blob->blob_data;                        \
    }                                                                          \
    return s;                                                                  \
  }

void *create_new_blob_template() {
  // Allocate memory for the template
  BlobTemplate *template = malloc(sizeof(BlobTemplate));
  *template = (BlobTemplate){0};
  printf("create new blob template %p\n", template);
  return template;
}

BlobTemplate *start_blob(char *base_memory) {

  BlobTemplate *template = malloc(sizeof(BlobTemplate));
  *template = (BlobTemplate){
      .blob_data = base_memory,
      ._mem_ptr = base_memory,
      .first_node_offset = -1 /*sentinel value before any nodes get added */};
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

Node *create_sin_node(void **mem, Signal *input) {
  Node *node = node_alloc(mem, sizeof(Node));

  int in_offset = (char *)input - (char *)node;
  *node = (Node){.num_ins = 1,
                 .input_offsets = {in_offset},
                 .node_size = sizeof(Node) + sizeof(sin_state),
                 .out = {.size = BUF_SIZE,
                         .layout = 1,
                         .buf = malloc(sizeof(double) * BUF_SIZE)},
                 .node_perform = (perform_func_t)sin_perform,
                 .next = NULL};

  // Allocate and initialize state
  sin_state *state = node_alloc(mem, sizeof(sin_state));
  *state = (sin_state){0.};

  return node;
}

Node *sin_node(Signal *input) {
  ADD_NODE_TO_CURRENT_BLOB(create_sin_node, input)
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

Node *create_tanh_node(void **mem, Signal *input) {
  // Allocate memory for node
  Node *node = node_alloc(mem, sizeof(Node));

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
  tanh_state *state = node_alloc(mem, sizeof(tanh_state));
  *state = (tanh_state){5.};
  return node;
}
Node *tanh_node(Signal *input) {
  ADD_NODE_TO_CURRENT_BLOB(create_tanh_node, input)
}
