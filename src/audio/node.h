#ifndef _NODE
#define _NODE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static const double PI = 3.14159265358979323846264338328;
/* static int node_frame_size = 2048; */
static int node_frame_size = 512;

typedef struct NodeData {
} NodeData;

typedef void (*t_perform)(struct Node *node, int frame_count,
                          double seconds_per_frame, double seconds_offset);
typedef void (*t_free_node)(struct Node *node);

typedef struct Node {
  struct Node *next;
  void (*perform)(struct Node *node, int frame_count, double seconds_per_frame,
                  double seconds_offset);
  void (*perform_mul_add)(struct Node *node, int frame_count,
                          double seconds_per_frame, double seconds_offset);
  char *name;
  NodeData *data;
  void (*free_node)(struct Node *node);
  double *out;
  double *in;

  double *mul;
  double *add;
  int should_free;
} Node;
void debug_node(Node *node, char *text);
double *get_buffer();
Node *alloc_node(NodeData *data, double *in, t_perform perform, char *name,
                 t_free_node free_node);

void perform_null(Node *node, int frame_count, double seconds_per_frame,
                  double seconds_offset);
void perform_node_mul(Node *node, int frame_count, double seconds_per_frame,
                      double seconds_offset);
Node *node_mul(Node *node_a, Node *node_b);
void perform_node_add(Node *node, int frame_count, double seconds_per_frame,
                      double seconds_offset);

Node *node_add(Node *node_a, Node *node_b);
Node *node_add_to_tail(Node *node, Node *tail);
void free_data(NodeData *data);
void free_node(Node *node);
#endif
