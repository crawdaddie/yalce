#ifndef _NODE
#define _NODE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static const double PI = 3.14159265358979323846264338328;
typedef struct NodeData {
} NodeData;

typedef void (*t_perform)(struct Node *node, double *out, int frame_count,
                          double seconds_per_frame, double seconds_offset);
typedef void (*t_free_node)(struct Node *node);

typedef struct Node {
  struct Node *next;
  void (*perform)(struct Node *node, double *out, int frame_count,
                  double seconds_per_frame, double seconds_offset);
  char *name;
  NodeData *data;
  void (*free_node)(struct Node *node);
} Node;
void debug_node(Node *node, char *text);
Node *alloc_node(NodeData *data, t_perform perform, char *name,
                 t_free_node free_node);

void free_data(NodeData *data);
void free_node(Node *node);
#endif
