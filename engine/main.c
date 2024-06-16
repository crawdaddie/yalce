#include "audio_loop.h"
#include "ctx.h"
#include "lib.h"
#include "node.h"
#include "oscillators.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  Node node;
  int *in_sig_ptrs;
  int out_sig_ptrs;
} GroupItem;


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

static Node *group_add(group_state *group, Node *node) {
  // printf("adding %p\n", node);
  if (group->head == NULL) {
    group->head = node;
    // group->tail = node;
    return node;
  }
  // group->tail->next = node;
  // group->tail = node;
  return node;
}

int main(int argc, char **argv) {
  init_audio();
  Node *sq = malloc(sizeof(Node));
  sq->state = malloc(sizeof(sq_state));
  sq->perform = sq_perform;
  sq->frame_offset = 0;

  sq->num_ins = 1;
  sq->ins = malloc(sizeof(double *));
  sq->ins[0] = malloc(sizeof(double) * BUF_SIZE);

  for (int i = 0; i < BUF_SIZE; i++) {
    sq->ins[0][i] = 100.;
  }
  sq->type = OUTPUT;
  sq->next = NULL;

  group_state *gr = malloc(sizeof(group_state));
  gr->head = sq;

  Node *group = malloc(sizeof(Node));
  group->state = gr; 
  group->perform = group_perform;
  group->is_group = true;
  group->num_ins = 0;
  group->ins = NULL;
  group->frame_offset = 0;



  printf("sq %p frame offset %d perform %p\n", sq, sq->frame_offset, sq->perform);
  // group_add(gr, sq);

  printf("group %p output %p\n", group, group->output_buf);


  group->is_group = 1;
  group->type = OUTPUT;
  audio_ctx_add(group);

  while (1) {}
  return 0;
}
