#include "audio_loop.h"
#include "ctx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Forward declarations for needed constants
#define PI 3.14159265358979323846
#define BUF_SIZE 1024

typedef struct Signal {
  int layout;
  int size;
  double *buf;
} Signal;

typedef void *(*perform_func_t)(void *ptr, int nframes, double spf);
#define MAX_INPUTS 16
struct Node {
  int node_size;
  int num_ins;
  int input_offsets[MAX_INPUTS];
  Signal out;
  perform_func_t node_perform;
  struct Node *next;
};

// Definition for a blob template
typedef struct {
  int total_size;              // Total size of all nodes in the blob
  int num_inputs;              // Number of inputs the blob expects
  void *blob_data;             // The actual compiled blob data
  int input_slots[MAX_INPUTS]; // Offsets for where inputs should connect
} BlobTemplate;

// Definition for a blob instance
typedef struct {
  int total_size;
  int num_ins;
  void *blob_start;
  void *blob_end;
} blob_state;

// Helper functions
perform_func_t get_node_perform(void *node) {
  return *(perform_func_t *)((char *)node + sizeof(int) + sizeof(int) +
                             sizeof(int) * MAX_INPUTS + sizeof(Signal));
}

void *get_node_state(void *node) { return (char *)node + sizeof(struct Node); }

// Sin oscillator
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

double *get_val(Signal *in) {
  if (in->size == 1) {
    return in->buf;
  }
  double *ptr = in->buf;
  in->buf += in->layout;
  return ptr;
}

Signal *get_node_input(struct Node *node, int input) {
  return (void *)((char *)node + (node->input_offsets[input]));
}

void *node_alloc(void **mem, size_t size) {
  if (!mem) {
    return malloc(size);
  }
  void *ptr = *mem;
  *mem = (char *)(*mem) + size;
  return ptr;
}

void write_to_dac(int dac_layout, double *dac_buf, int layout, double *buf,
                  int output_num, int nframes) {
  if (output_num > 0) {
    while (nframes--) {
      for (int i = 0; i < dac_layout; i++) {
        *(dac_buf + i) += *(buf + (i < layout ? i : 0));
      }
      buf += layout;
      dac_buf += dac_layout;
    }
  } else {
    while (nframes--) {
      for (int i = 0; i < dac_layout; i++) {
        *(dac_buf + i) = *(buf + (i < layout ? i : 0));
      }
      buf += layout;
      dac_buf += dac_layout;
    }
  }
}

void perform_graph(struct Node *head, int frame_count, double spf,
                   double *dac_buf, int layout, int output_num) {
  if (!head) {
    printf("Error: NULL head\n");
    return;
  }
  void *blob = head->node_perform(head, frame_count, spf);

  write_to_dac(layout, dac_buf, head->out.layout, head->out.buf, output_num,
               frame_count);

  if (head->next) {
    perform_graph(head->next, frame_count, spf, dac_buf, layout,
                  output_num + 1);
  }
}

double buf_pool[1 << 20];

Signal *create_const_sig(void **mem, double val) {
  double *buf = malloc(sizeof(double));
  *buf = val;
  Signal *sig = node_alloc(mem, sizeof(Signal));
  sig->buf = buf;
  sig->layout = 1;
  sig->size = 1;
  return sig;
}

void *sin_perform(struct Node *node, int nframes, double spf) {
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

struct Node *create_sin_node(void **mem, Signal *input) {
  struct Node *node = node_alloc(mem, sizeof(struct Node));
  int in_offset = (char *)input - (char *)node;

  *node = (struct Node){.num_ins = 1,
                        .input_offsets = {in_offset},
                        .node_size = sizeof(struct Node) + sizeof(sin_state),
                        .out = {.size = BUF_SIZE,
                                .layout = 1,
                                .buf = malloc(sizeof(double) * BUF_SIZE)},
                        .node_perform = sin_perform,
                        .next = NULL};

  sin_state *state = node_alloc(mem, sizeof(sin_state));
  *state = (sin_state){0.};

  return node;
}

typedef struct {
  double gain;
} tanh_state;

void *tanh_perform(struct Node *node, int nframes, double spf) {
  tanh_state *state = get_node_state(node);
  double *out = node->out.buf;
  Signal in = *get_node_input(node, 0);
  while (nframes--) {
    *out = tanh(*get_val(&in) * state->gain);
    out++;
  }
  return (char *)node + node->node_size;
}

struct Node *create_tanh_node(void **mem, Signal *input) {
  struct Node *node = node_alloc(mem, sizeof(struct Node));
  int in_offset = (char *)input - (char *)node;

  *node = (struct Node){
      .num_ins = 1,
      .input_offsets = {in_offset},
      .node_size = sizeof(struct Node) + sizeof(tanh_state),
      .out = {.size = input->size,
              .layout = input->layout,
              .buf = malloc(sizeof(double) * input->size * input->layout)},
      .node_perform = tanh_perform,
      .next = NULL};

  tanh_state *state = node_alloc(mem, sizeof(tanh_state));
  *state = (tanh_state){10.};

  return node;
}

void *blob_perform(struct Node *node, int nframes, double spf) {
  blob_state *state = (blob_state *)((char *)node + sizeof(struct Node));

  void *current = state->blob_start;
  void *end = state->blob_end;

  if (!current || !end) {
    printf("Error: Invalid blob start or end pointer\n");
    return (char *)node + node->node_size;
  }

  while (current <= end) {
    perform_func_t perform = ((struct Node *)current)->node_perform;

    if (!perform) {
      printf("Error: NULL perform function at %p\n", current);
      break;
    }

    current = perform(current, nframes, spf);

    if (current > end) {
      break;
    }
  }

  if (end) {
    node->out = ((struct Node *)end)->out;
  }

  return (char *)node + node->node_size;
}

struct Node *create_blob_node(void **mem) {
  struct Node *node = node_alloc(mem, sizeof(struct Node));

  *node = (struct Node){.num_ins = 0,
                        .node_size = sizeof(struct Node) + sizeof(blob_state),
                        .node_perform = blob_perform,
                        .next = NULL};

  blob_state *state = node_alloc(mem, sizeof(blob_state));
  *state = (blob_state){0, 0, NULL, NULL};

  return node;
}

void node_add_input(int index, struct Node *node, Signal *input_sig) {
  int ptr_offset = (char *)input_sig - (char *)node;
  node->num_ins = (index + 1 > node->num_ins) ? index + 1 : node->num_ins;
  node->input_offsets[index] = ptr_offset;
}

// Create a blob template that can be instantiated multiple times
BlobTemplate *compile_blob_template(void) {
  // Allocate memory for the template
  BlobTemplate *template = malloc(sizeof(BlobTemplate));

  // We need temporary memory for compilation
  char *temp_mem = malloc(1024); // Adjust size as needed
  void *mem_ptr = temp_mem;

  // Create a placeholder for the frequency input
  Signal placeholder = {.layout = 1, .size = 1, .buf = NULL};
  template->num_inputs = 1;

  // Record where the input should connect in the template
  template->input_slots[0] = 0; // We'll compute this later

  // Create the nodes
  struct Node *sin_node = create_sin_node(&mem_ptr, &placeholder);
  struct Node *tanh_node = create_tanh_node(&mem_ptr, &(sin_node->out));

  // Calculate the total size of the blob
  template->total_size = (char *)mem_ptr - temp_mem;

  // Allocate memory for the blob data
  template->blob_data = malloc(template->total_size);

  // Now we need to recreate everything in the final blob memory
  mem_ptr = template->blob_data;

  // Recreate the nodes in the proper memory
  struct Node *final_sin = create_sin_node(&mem_ptr, &placeholder);
  struct Node *final_tanh = create_tanh_node(&mem_ptr, &(final_sin->out));

  // Record the input slot offset
  template->input_slots[0] =
      (char *)&(final_sin->input_offsets[0]) - (char *)template->blob_data;

  // Free temporary memory
  free(temp_mem);

  return template;
}

// Instantiate a blob from a template
struct Node *instantiate_blob(void **mem, BlobTemplate *template,
                              Signal **inputs) {
  // Create the blob node
  struct Node *blob_node = create_blob_node(mem);
  blob_state *state = (blob_state *)get_node_state(blob_node);

  // Allocate memory for the blob instance
  void *blob_mem = node_alloc(mem, template->total_size);

  // Copy the template data to the instance
  memcpy(blob_mem, template->blob_data, template->total_size);

  // Set up the blob state
  state->total_size = template->total_size;
  state->num_ins = template->num_inputs;
  state->blob_start = blob_mem;
  state->blob_end =
      (char *)blob_mem + template->total_size - 1; // Point to the last byte

  // Connect inputs
  for (int i = 0; i < template->num_inputs; i++) {
    if (inputs[i]) {
      // Calculate the offset from the input to the blob start
      int offset = (char *)inputs[i] - (char *)blob_mem;

      // Write this offset to the appropriate slot in the blob
      int *slot_ptr = (int *)((char *)blob_mem + template->input_slots[i]);
      *slot_ptr = offset;
    }
  }

  // Make sure the next and prev node pointers are properly updated
  struct Node *first_node = (struct Node *)blob_mem;
  first_node->next = NULL;

  return blob_node;
}

// Helper function to create multiple blobs with different inputs
void create_multiple_blobs(struct Node **nodes, int count,
                           BlobTemplate *template, void **mem,
                           Signal **input_sets) {
  for (int i = 0; i < count; i++) {
    nodes[i] =
        instantiate_blob(mem, template, &input_sets[i * template->num_inputs]);
  }
}

int main(int argc, char **argv) {
  maketable_sin();
  init_audio();
  struct Ctx *ctx = get_audio_ctx();

  // Create the blob template
  BlobTemplate *blob_template = compile_blob_template();
  printf("Created blob template of size %d with %d inputs\n",
         blob_template->total_size, blob_template->num_inputs);

  // Allocate memory for nodes
  char mem[4096] = {0};
  void *mem_ptr = mem;

  // Create different frequency signals for each blob instance
  Signal *freq1 = create_const_sig(&mem_ptr, 100.0);  // A
  Signal *freq2 = create_const_sig(&mem_ptr, 130.81); // C
  Signal *freq3 = create_const_sig(&mem_ptr, 164.81); // E

  // Create input sets for each blob
  Signal *inputs[] = {freq1, freq2, freq3};

  // Create three blob instances
  struct Node *blobs[3];
  for (int i = 0; i < 3; i++) {
    blobs[i] = instantiate_blob(&mem_ptr, blob_template, &inputs[i]);
    printf("Created blob instance %d at %p\n", i, blobs[i]);
  }

  // Connect the blobs in a chain
  for (int i = 0; i < 2; i++) {
    blobs[i]->next = blobs[i + 1];
  }

  // Set the head node
  ctx->head = blobs[0];

  printf("Audio graph initialized with 3 blob instances\n");

  // Main loop
  while (1) {
    sleep(1);
  }

  // Clean up (this will never be reached, but good practice)
  free(blob_template->blob_data);
  free(blob_template);

  return 0;
}
