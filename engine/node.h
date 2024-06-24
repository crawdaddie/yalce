#ifndef _ENGINE_NODE_H
#define _ENGINE_NODE_H
#include "common.h"
typedef struct Node Node;

// typedef void (*node_perform)(Node *node, int nframes, double spf);

typedef struct {
  double *buf;
  int size;
  int layout;
} Signal;

typedef struct Node (*node_perform)(struct Node *node, int nframes, double spf);

typedef struct Node {
  enum { INTERMEDIATE = 0, OUTPUT } type;
  void *state;
  // double *output_buf;
  Signal out;
  node_perform perform;
  int num_ins;
  // double **ins;
  Signal *ins;
  struct Node *next;
  struct Node *prev;
  int frame_offset;
} Node;

Node *perform_graph(Node *head, int frame_count, double spf, double *output_buf,
                    int output_num);

typedef struct {
  Node *head;
  Node *tail;
} group_state;

node_perform group_perform(Node *group, int nframes, double spf);
#endif
