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

Node *perform_graph(Node *head, int frame_count, double spf, double *dest,
                    int dest_layout, int output_num);

typedef struct {
  Node *head;
  Node *tail;
} group_state;

node_perform group_perform(Node *group, int nframes, double spf);

Node *group_add_tail(Node *group, Node *node);

Node *node_new(void *data, node_perform *perform, int num_ins, Signal *ins);

Signal *get_sig(int layout);
Signal *get_sig_default(int layout, double value);

Node *group_new(int chans);

Node *sq_node(double freq);
Node *sin_node(double freq);

node_perform sum_perform(Node *node, int nframes, double spf);
node_perform mul_perform(Node *node, int nframes, double spf);

Node *sum2_node(Node *a, Node *b);
Node *sub2_node(Node *a, Node *b);
Node *mul2_node(Node *a, Node *b);
Node *div2_node(Node *a, Node *b);
Node *mod2_node(Node *a, Node *b);

Node *node_of_double(double val);
#endif
