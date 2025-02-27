#include "audio_loop.h"
#include "ctx.h"
#include <stdio.h>
#include <unistd.h>
void begin_blob() {}
void *end_blob() { return NULL; }

struct Node {
  void *blob;
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

typedef void *(*perform_func_t)(void *ptr, int out_layout, double *out,
                                int nframes, double spf);

void *sin_perform(void *ptr, int out_layout, double *out, int nframes,
                  double spf) {

  sin_state *state = (sin_state *)ptr;
  ptr += sizeof(sin_state);

  double d_index;
  int index = 0;

  double frac, a, b, sample;
  double freq;

  while (nframes--) {
    freq = 100.;
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
    state->phase = fmod(freq * spf + (state->phase), 1.0);

    // input++;
  }
  return ptr;
}

static double buf_pool[BUF_SIZE * 16];

void perform_graph(Node *head, int frame_count, double spf, double *dac_buf,
                   int layout, int output_num) {
  if (!head || !head->blob) {
    printf("Error: NULL head or blob\n");
    return;
  }
  void *blob = head->blob;
  perform_func_t perform_func = *(perform_func_t *)blob;
  if (!perform_func) {
    printf("Error: NULL function pointer\n");
    return;
  }
  blob += sizeof(perform_func_t);
  blob = perform_func(blob, layout, buf_pool, frame_count, spf);
  double *b = buf_pool;
  if (output_num > 0) {
    while (frame_count--) {
      for (int i = 0; i < LAYOUT; i++) {
        *dac_buf += *b;
        b++;
        dac_buf++;
      }
    }
  } else {
    while (frame_count--) {
      for (int i = 0; i < LAYOUT; i++) {
        *dac_buf = *b;
        b++;
        dac_buf++;
      }
    }
  }
}

int main(int argc, char **argv) {
  maketable_sin();
  init_audio();
  Ctx *ctx = get_audio_ctx();

  int8_t blob[1 << 8] = {0};
  Node node = {0};
  node.blob = blob;

  void *blob_ptr = blob;
  ctx->head = &node;

  // Store the function pointer at the beginning of the blob
  perform_func_t *fn_ptr = (perform_func_t *)blob_ptr;
  *fn_ptr = sin_perform;
  blob_ptr += sizeof(perform_func_t);

  sin_state *state_ptr = (sin_state *)blob_ptr;
  *state_ptr = (sin_state){.phase = 0.0};
  state_ptr->phase = 0.0;
  blob_ptr += sizeof(sin_state);

  // Print the address to verify it's properly set
  printf("Function pointer: %p\n", fn_ptr);
  printf("State pointer: %p\n", state_ptr);
  printf("Initial phase: %f\n", state_ptr->phase);

  while (1) {
    sleep(1);
  }
  return 0;
}
