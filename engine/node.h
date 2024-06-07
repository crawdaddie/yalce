#ifndef _ENGINE_NODE_H
#define _ENGINE_NODE_H
#include "common.h"
typedef struct Node Node;

typedef void (*node_perform)(void *state, double *output, int num_ins,
                             double **inputs, int nframes, double spf);
typedef struct Node {
  enum { INTERMEDIATE = 0, OUTPUT } type;
  void *state;
  double output_buf[BUF_SIZE];
  node_perform perform;
  int num_ins;
  double **ins;
  struct Node *next;
  struct Node *prev;
} Node;

#endif
