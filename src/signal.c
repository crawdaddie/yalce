#include "signal.h"
#include "common.h"
#include "ctx.h"
#include "soundfile.h"
#include <stdlib.h>
// ------------------------------ SIGNAL / BUFFER ALLOC
//
static double buf_pool[BUF_SIZE * LAYOUT_CHANNELS * 200];
double *get_buf_pool_start() { return buf_pool; }
static double *buf_ptr = buf_pool;

void init_sig_ptrs() { buf_ptr = buf_pool; }

Signal *get_sig(int layout) {
  Signal *sig = malloc(sizeof(Signal));
  // sig->buf = buf_ptr;
  sig->buf = calloc(sizeof(double), BUF_SIZE * layout);
  sig->layout = layout;
  sig->size = BUF_SIZE;
  // buf_ptr += BUF_SIZE * layout;
  return sig;
}

Signal *get_sig_of_soundfile(const char *filename) {
  int sf_sr;
  Signal *sig = malloc(sizeof(Signal));
  read_file(filename, sig, &sf_sr);
  return sig;
}

Signal *get_sig_float(int layout) {
  Signal *sig = malloc(sizeof(SignalFloat));
  // sig->buf = buf_ptr;
  sig->buf = malloc(sizeof(double) * BUF_SIZE * layout);
  sig->layout = layout;
  sig->size = BUF_SIZE;
  // buf_ptr += BUF_SIZE * layout;
  return sig;
}

Signal *get_sig_default(int layout, double value) {
  Signal *sig = malloc(sizeof(Signal));
  // sig->buf = buf_ptr;

  sig->buf = malloc(sizeof(double) * BUF_SIZE * layout);
  sig->layout = layout;
  sig->size = BUF_SIZE;
  // buf_ptr += BUF_SIZE * layout;
  for (int i = 0; i < BUF_SIZE * layout; i++) {
    sig->buf[i] = value;
  }
  return sig;
}
