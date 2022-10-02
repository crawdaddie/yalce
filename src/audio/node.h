#ifndef _NODE
#define _NODE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct NodeData {
} NodeData;

typedef void (*t_perform)(struct Node *node, double *out, int frame_count,
                          double seconds_per_frame, double seconds_offset);

typedef struct Node {
  struct Node *next;
  void (*perform)(struct Node *node, double *out, int frame_count,
                  double seconds_per_frame, double seconds_offset);
  char *name;
  NodeData *data;
} Node;
#endif

void debug_node(Node *node, char *text);
Node *alloc_node(NodeData *data, t_perform perform, char *name);

typedef struct sq_data {
  double freq;
  double _prev_freq;

} sq_data;
void debug_sq(sq_data *data);
Node *get_sq_detune_node();
void perform_sq_detune(Node *node, double *out, int frame_count,
                       double seconds_per_frame, double seconds_offset);

typedef struct tanh_data {
  double gain;
} tanh_data;
Node *get_tanh_node();

typedef struct lp_data {
  double cutoff;
  double resonance;
  double az1;
  double az2;
  double az3;
  double az4;
  double az5;
  double ay1;
  double ay2;
  double ay3;
  double ay4;
  double amf;
} lp_data;
Node *get_lp_node();
