#include "signal.h"
#include "../common.h"
#include "stdlib.h"

static double signal_buffer_pool[128][BUF_SIZE] = {0.0};

Signal new_signal(int size) {
  double *data = malloc(sizeof(double) * size);
  for (int i = 0; i < size; i++) {
    data[i] = 0.0;
  }
  return (Signal){.data = data, .size = size};
}

Signal *new_signal_heap(int size, int layout) {
  double *data = calloc(sizeof(double), size);
  Signal *sig = malloc(sizeof(Signal));
  sig->data = data;
  sig->size = size;
  sig->layout = layout;
  return sig;
}

Signal *new_signal_heap_default(int size, int layout, double def) {
  double *data = malloc(sizeof(double) * size);
  for (int i = 0; i < size; i++) {
    data[i] = def;
  }
  Signal *sig = malloc(sizeof(Signal));
  sig->data = data;
  sig->size = size;
  return sig;
}

void set_signal(Signal signal, double value) {
  for (int i = 0; i < signal.size; i++) {
    signal.data[i] = value;
  }
}

void set_signal_ramp(Signal signal, double value, int time) {
  for (int i = 0; i < signal.size; i++) {
    signal.data[i] = value;
  }
}

double unwrap(Signal sig, int frame) {
  return *(sig.data + (frame % sig.size));
}
