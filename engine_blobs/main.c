#include "audio_loop.h"
#include "ctx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <math.h>

int rand_int(int range) {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * range;
  return (int)rand_double;
}
void begin_blob() {}
void *end_blob() { return NULL; }

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

perform_func_t get_node_perform(void *node) {
  return *(perform_func_t *)((char *)node + sizeof(int) + sizeof(int) +
                             sizeof(int) * MAX_INPUTS + sizeof(Signal));
}

void *get_node_state(void *node) { return (char *)node + sizeof(Node); }

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
    // printf("malloc instead\n");
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

void perform_graph(Node *head, int frame_count, double spf, double *dac_buf,
                   int layout, int output_num) {

  if (!head) {
    printf("Error: NULL head\n");
    return;
  }
  void *blob = head->node_perform(head, frame_count, spf);

  if (true) {
    write_to_dac(layout, dac_buf, head->out.layout, head->out.buf, output_num,
                 frame_count);
  }
  if (head->next) {
    perform_graph(head->next, frame_count, spf, dac_buf, layout,
                  output_num + 1);
  }
}

double buf_pool[1 << 20];

Signal *create_const_sig(void **mem, double val) {
  // double *buf = malloc(sizeof(double));
  // *buf = val;
  Signal *sig = node_alloc(mem, sizeof(Signal));
  // sig->buf = buf;
  sig->layout = 1;
  sig->size = 1;
  return sig;
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

struct Node *create_sin_node(void **mem, Signal *input) {
  struct Node *node = node_alloc(mem, sizeof(struct Node));

  int in_offset = (char *)input - (char *)node;
  *node = (struct Node){.num_ins = 1,
                        .input_offsets = {in_offset},
                        .node_size = sizeof(struct Node) + sizeof(sin_state),
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

struct Node *create_tanh_node(void **mem, Signal *input) {
  // Allocate memory for node
  struct Node *node = node_alloc(mem, sizeof(struct Node));

  // Calculate offset from node to input
  int in_offset = (char *)input - (char *)node;

  // Initialize node
  *node = (struct Node){
      .num_ins = 1,
      .input_offsets = {in_offset},
      .node_size = sizeof(struct Node) + sizeof(tanh_state),
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

typedef struct {
  int total_size;
  int num_ins;
  void *blob_start;
  void *blob_end;
  int input_slot_offsets[MAX_INPUTS];
} blob_state;

void *blob_perform(struct Node *node, int nframes, double spf) {
  blob_state *state = (blob_state *)((char *)node + sizeof(struct Node));

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
    perform_func_t perform = ((struct Node *)current)->node_perform;

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
    node->out = ((struct Node *)end)->out;
  }

  return (char *)node + node->node_size;
}

struct Node *create_blob_node(void **mem) {
  // Allocate memory for node
  struct Node *node = node_alloc(mem, sizeof(struct Node));

  // Initialize node
  *node = (struct Node){.num_ins = 0,
                        .node_size = sizeof(struct Node) + sizeof(blob_state),
                        .node_perform = (perform_func_t)blob_perform,
                        .next = NULL};

  // Allocate and initialize state
  blob_state *state = node_alloc(mem, sizeof(blob_state));
  *state = (blob_state){0, NULL, NULL};

  return node;
}

void node_add_input(int index, Node *group, Signal *input_sig) {
  int ptr_offset = (char *)input_sig - (char *)group;
  group->num_ins++;
  group->input_offsets[index] = ptr_offset;
}

// Definition for a blob template
typedef struct {
  int total_size;  // Total size of all nodes in the blob
  int num_inputs;  // Number of inputs the blob expects
  void *blob_data; // The actual compiled blob data
  int first_node_offset;
  int last_node_offset;
  int input_slot_offsets[MAX_INPUTS]; // Offsets for where inputs should connect
  double default_vals[MAX_INPUTS];    // Offsets for where inputs should connect
} BlobTemplate;

Signal *add_input_to_template(void *mem_block_start, void **mem,
                              BlobTemplate *tpl, double default_val) {
  int idx = tpl->num_inputs;
  Signal *sig = create_const_sig(mem, default_val);
  tpl->default_vals[idx] = default_val;
  tpl->input_slot_offsets[idx] = (void *)sig - mem_block_start;
  tpl->num_inputs++;

  return sig;
}

// Create a blob template that can be instantiated multiple times
BlobTemplate *compile_sin_tanh_blob_template(void) {
  // Allocate memory for the template
  BlobTemplate *template = malloc(sizeof(BlobTemplate));
  *template = (BlobTemplate){0};

  // We need temporary memory for compilation
  char temp_mem[1 << 12];
  void *mem_ptr = temp_mem;

  // Create a placeholder for the frequency input

  Signal *freq = add_input_to_template(temp_mem, &mem_ptr, template, 100.);

  // Create the nodes
  struct Node *sin_node = create_sin_node(&mem_ptr, freq);
  template->first_node_offset = ((char *)sin_node - temp_mem);

  struct Node *tanh_node = create_tanh_node(&mem_ptr, &(sin_node->out));
  template->last_node_offset = ((char *)tanh_node - temp_mem);

  // Calculate the total size of the blob
  template->total_size = (char *)mem_ptr - temp_mem;

  // Allocate memory for the blob data
  template->blob_data = malloc(template->total_size);
  memcpy(template->blob_data, temp_mem, template->total_size);
  return template;
}

// Instantiate a blob from a template
Node *instantiate_blob_template(void **mem, BlobTemplate *template) {
  if (mem == NULL) {
    void *_mem = (malloc(sizeof(Node) + template->total_size));
    mem = &_mem;
  }

  // Allocate memory for node
  struct Node *blob_node = node_alloc(mem, sizeof(struct Node));

  // Initialize node
  *blob_node =
      (struct Node){.num_ins = 0,
                    .node_size = sizeof(struct Node) + sizeof(blob_state) +
                                 template->total_size,
                    .node_perform = (perform_func_t)blob_perform,
                    .next = NULL};

  // Allocate and initialize state
  blob_state *state = node_alloc(mem, sizeof(blob_state));

  void *blob_data = node_alloc(mem, template->total_size);
  if (!blob_data) {
    fprintf(stderr, "Could not allocate memory for blob node\n");
    return NULL;
  }

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

Signal *get_blob_node_input(Node *node, int index) {
  blob_state *other_state = get_node_state(node);
  char *blob_data = (char *)other_state + sizeof(blob_state);
  Signal *f = (Signal *)(blob_data + other_state->input_slot_offsets[index]);
  return f;
}
// Save a BlobTemplate to a file
void save_blob_template_to_file(BlobTemplate *tpl, const char *path) {
  FILE *f = fopen(path, "wb"); // Open in binary write mode
  if (!f) {
    perror("Failed to open file for writing");
    return;
  }

  // Write header information
  fwrite(&tpl->total_size, sizeof(int), 1, f);
  fwrite(&tpl->num_inputs, sizeof(int), 1, f);

  // Write input slot offsets
  fwrite(tpl->input_slot_offsets, sizeof(int), tpl->num_inputs, f);

  // Write the blob data itself
  fwrite(tpl->blob_data, 1, tpl->total_size, f);

  fclose(f);
  printf(
      "Saved blob template to %s (%d bytes)\n", path,
      (int)(sizeof(int) * 2 + sizeof(int) * tpl->num_inputs + tpl->total_size));
}

// Load a BlobTemplate from a file
BlobTemplate *load_blob_template_from_file(const char *path) {
  FILE *f = fopen(path, "rb"); // Open in binary read mode
  if (!f) {
    perror("Failed to open file for reading");
    return NULL;
  }

  // Allocate memory for the template
  BlobTemplate *tpl = malloc(sizeof(BlobTemplate));
  if (!tpl) {
    perror("Failed to allocate memory for template");
    fclose(f);
    return NULL;
  }

  // Read header information
  if (fread(&tpl->total_size, sizeof(int), 1, f) != 1 ||
      fread(&tpl->num_inputs, sizeof(int), 1, f) != 1) {
    perror("Failed to read template header");
    free(tpl);
    fclose(f);
    return NULL;
  }

  // Validate the read values to prevent issues
  if (tpl->total_size <= 0 || tpl->total_size > 10 * 1024 * 1024 || // 10MB max
      tpl->num_inputs < 0 || tpl->num_inputs > MAX_INPUTS) {
    fprintf(stderr, "Invalid template data in file\n");
    free(tpl);
    fclose(f);
    return NULL;
  }

  // Read input slot offsets
  if (fread(tpl->input_slot_offsets, sizeof(int), tpl->num_inputs, f) !=
      tpl->num_inputs) {
    perror("Failed to read input slots");
    free(tpl);
    fclose(f);
    return NULL;
  }

  // Allocate memory for the blob data
  tpl->blob_data = malloc(tpl->total_size);
  if (!tpl->blob_data) {
    perror("Failed to allocate memory for blob data");
    free(tpl);
    fclose(f);
    return NULL;
  }

  // Read the blob data
  if (fread(tpl->blob_data, 1, tpl->total_size, f) != tpl->total_size) {
    perror("Failed to read blob data");
    free(tpl->blob_data);
    free(tpl);
    fclose(f);
    return NULL;
  }

  fclose(f);
  printf(
      "Loaded blob template from %s (%d bytes)\n", path,
      (int)(sizeof(int) * 2 + sizeof(int) * tpl->num_inputs + tpl->total_size));

  return tpl;
}

int main(int argc, char **argv) {
  maketable_sin();
  init_audio();

  // BlobTemplate *blob_template =
  //     load_blob_template_from_file("cached_blobs/sin_tanh.blob");

  BlobTemplate *blob_template = compile_sin_tanh_blob_template();
  printf("Created blob template of size %d with %d inputs\n",
         blob_template->total_size, blob_template->num_inputs);
  // save_blob_template_to_file(blob_template, "cached_blobs/sin_tanh.blob");

  double *buf_pool_ptr = buf_pool;

  int8_t mem[1 << 12] = {0};

  void *mem_ptr = mem;
  Node *group = instantiate_blob_template(NULL, blob_template);
  Node *other = instantiate_blob_template(NULL, blob_template);

  // printf("mem ptr is at: %ld\n", mem_ptr - (void *)mem);
  Signal *f = get_blob_node_input(other, 0);
  Signal *g = get_blob_node_input(group, 0);

  f->buf[0] = 450.;
  printf("other setting %f\n", f->buf[0]);

  group->next = other;

  Ctx *ctx = get_audio_ctx();
  ctx->head = group;

  double freqs[7] = {400., 450., 500., 200., 150., 100., 600.};
  while (1) {
    sleep(1);
    f->buf[0] = freqs[rand_int(7)];
    sleep(1);
    f->buf[0] = freqs[rand_int(7)];
    g->buf[0] = freqs[rand_int(7)];
  }
  return 0;
}
