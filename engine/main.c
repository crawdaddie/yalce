#include "audio_loop.h"
#include "ctx.h"
#include "lib.h"
#include "node.h"
#include "oscillators.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  Node node;
  int *in_sig_ptrs;
  int out_sig_ptrs;
} GroupItem;

typedef struct {
  int num_signals;
  double **signals;
  int num_items;
  GroupItem *items;
} group_state;

void node_array_perform(void *_state, double *out, int num_ins, double **inputs,
                        int nframes, double spf) {}

typedef struct {
  void *blob;
  void *blob_ptr;
  int capacity;
} AllocArena;

void *alloc(AllocArena *alloc_blob, unsigned long size) {
  void *ptr = alloc_blob->blob_ptr;

  unsigned long offset = alloc_blob->blob_ptr - alloc_blob->blob;

  if (offset + size >= alloc_blob->capacity) {
    // printf("resizing %lu %lu\n", offset, size);
    alloc_blob->capacity *= 2;
    alloc_blob->blob = realloc(alloc_blob->blob, alloc_blob->capacity);
    alloc_blob->blob_ptr = alloc_blob->blob + offset;
  }

  alloc_blob->blob_ptr += size;
  return ptr;
}

typedef struct {
  void *blob;
  double **signals;
} SynthBlob;

SynthBlob group_synth() {
  int capacity = 1 << 14;
  void *blob = malloc(capacity);
  double *signals = malloc(sizeof(double) * BUF_SIZE * 10);
  AllocArena arena = {.blob = blob, .blob_ptr = blob, .capacity = capacity};

  int *data_offset = alloc(&arena, sizeof(int));
  *data_offset = sizeof(sq_state);
  sq_state *sq = alloc(&arena, sizeof(sq_state));

  int *num_inputs = alloc(&arena, sizeof(int));
  *num_inputs = 1;
  printf("num inputs addr %p %d\n", num_inputs, *num_inputs);
  double **inputs = alloc(&arena, sizeof(double *) * *num_inputs);
  inputs[0] = signals + 0;
  double **output = alloc(&arena, sizeof(double *));

  return (SynthBlob){blob, signals};
}

int main(int argc, char **argv) {
  init_audio();

  SynthBlob sblob = group_synth();
  void *blob = sblob.blob;

  void *ptr = blob;
  int *data_offset = ptr;
  ptr += sizeof(int);

  sq_state *data = (sq_state *)ptr;
  printf("sq state phase %f\n", data->phase);
  ptr += *data_offset;
  int *num_inputs = ptr;
  printf("sq num inputs %p %d\n", num_inputs, *num_inputs);

  return 0;
}
