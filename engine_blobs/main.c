#include "audio_loop.h"
#include "ctx.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
void begin_blob() {}
void *end_blob() { return NULL; }
typedef struct Signal {
  int layout;
  int size;
  double *buf;
} Signal;

struct Node {
  void *blob;
  int layout;
  double *out;
};

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

    // printf("%f\n", val);
    sin_table[i] = val;
    phase += phsinc;
  }
}

typedef void *(*perform_func_t)(void *ptr, Signal *inputs, int out_layout,
                                double *out, int nframes, double spf);

double *get_val(Signal *in) {
  if (in->size == 1) {
    return in->buf;
  }
  double *ptr = in->buf;
  in->buf += in->layout;
  return ptr;
}

void *sin_perform(void *ptr, Signal *inputs, int out_layout, double *out,
                  int nframes, double spf) {

  sin_state *state = (sin_state *)ptr;
  ptr += sizeof(sin_state);

  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double *freq;

  Signal in = *inputs;

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

    // input++;
  }
  return ptr;
}

void *node_alloc(void **mem, size_t size) {
  if (!mem) {
    return malloc(size);
  }
  void *ptr = *mem;
  *mem = (*mem + size);
  return ptr;
}

void *create_sin_node(void **mem, Signal *input) {
  perform_func_t *fn_ptr = node_alloc(mem, sizeof(perform_func_t));
  *fn_ptr = sin_perform;
  sin_state *state = node_alloc(mem, sizeof(sin_state));
  *state = (sin_state){.phase = 0.};
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

  if (!head || !head->blob) {
    printf("Error: NULL head or blob\n");
    return;
  }

  void *blob = head->blob;
  perform_func_t perform = *(perform_func_t *)blob;
  blob += sizeof(perform_func_t);

  Signal ins[1];
  double freq = 150.;

  ins[0] = (Signal){
      .layout = 1,
      .size = 1,
      .buf = &freq,
  };

  blob = perform(blob, ins, head->layout, head->out, frame_count, spf);
  write_to_dac(layout, dac_buf, head->layout, head->out, output_num,
               frame_count);
}

double buf_pool[1 << 20];
// Node constructor function type
typedef void *(*NodeConstructorFn)(void **mem, ...);

typedef struct G {
  enum { NODE, PACKED_NODE, SIGNAL, NODE_REF } node_type;
  union {
    struct {
      // Function pointer for node construction
      NodeConstructorFn constructor;
      int num_inputs;
      struct G *inputs;
    } NODE;

    struct {
      int layout;
      int size;
      double *default_val;
    } SIGNAL;

    struct G *NODE_REF;
  } data;
  char *name;
} G;

void print_graph(G *graph) {
  switch (graph->node_type) {
  case NODE: {
    printf("%s ", graph->name);
    printf("\n  ");
    for (int i = 0; i < graph->data.NODE.num_inputs; i++) {
      print_graph(graph->data.NODE.inputs + i);
    }
    break;
  }

  case SIGNAL: {
    if (graph->name) {
      printf("%s [%d] %d ", graph->name, graph->data.SIGNAL.layout,
             graph->data.SIGNAL.size);
    } else {
      printf("%p [%d] %d ", graph, graph->data.SIGNAL.layout,
             graph->data.SIGNAL.size);
    }
    if (graph->data.SIGNAL.size == 1) {
      printf("(");
      for (int i = 0; i < graph->data.SIGNAL.layout; i++) {
        printf("%f, ", graph->data.SIGNAL.default_val[i]);
      }
      printf(")");
    }
    break;
  }
  }
}

#define MAKE_NODE(cons, _name, _ins, ...)                                      \
  (G) {                                                                        \
    NODE,                                                                      \
        {.NODE = {.constructor = (NodeConstructorFn)cons,                      \
                  .num_inputs = _ins,                                          \
                  .inputs = (G[]){__VA_ARGS__}}},                              \
        .name = _name                                                          \
  }

#define MAKE_SIGNAL(_lay, _size, _name, ...)                                   \
  (G) {                                                                        \
    SIGNAL, {.SIGNAL = {_lay, _size, (double[]){__VA_ARGS__}}}, .name = _name  \
  }

int main(int argc, char **argv) {
  // maketable_sin();
  // init_audio();
  // Ctx *ctx = get_audio_ctx();
  // double *buf_pool_ptr = buf_pool;
  //
  // int8_t blob[1 << 8] = {0};
  //
  // void *blob_ptr = blob;
  //
  // double *sin_out = buf_pool_ptr;
  // buf_pool_ptr += BUF_SIZE;
  // Node node = {blob_ptr, 1, sin_out};
  // ctx->head = &node;
  //
  // // Store the function pointer at the beginning of the blob
  // create_sin_node(&blob_ptr);
  //
  // while (1) {
  //   sleep(1);
  // }
  G graph = MAKE_NODE(create_sin_node, "sin", 1,
                      MAKE_SIGNAL(2, 1, "freq", 150., 165.));

  print_graph(&graph);
  return 0;
}
