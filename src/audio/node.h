#ifndef _NODE
#define _NODE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static const double PI = 3.14159265358979323846264338328;

typedef struct NodeData {
} NodeData;

typedef struct CtrlVal {
  double *val;
  int size;
} CtrlVal;

void set_ctrl_val(CtrlVal *ctrl, int i, double val);
double get_ctrl_val(CtrlVal *ctrl, double i);

double get_sample_interp(double read_ptr, double *buf, int max_frames);

typedef void (*t_perform)(struct Node *node, int frame_count,
                          double seconds_per_frame, double seconds_offset,
                          double *schedule);
typedef void (*t_free_node)(struct Node *node);

typedef struct Node {
  struct Node *next;
  void (*perform)(struct Node *node, int frame_count, double seconds_per_frame,
                  double seconds_offset, double schedule);

  char *name;
  NodeData *data;
  void (*free_node)(struct Node *node);
  double *out;
  double *in;

  double *mul;
  double *add;
  int should_free;
  double schedule;
} Node;
void debug_node(Node *node, char *text);
double *get_buffer();
Node *alloc_node(NodeData *data, double *in, t_perform perform, char *name,
                 t_free_node free_node);

void perform_null(Node *node, int frame_count, double seconds_per_frame,
                  double seconds_offset, double schedule);
void perform_node_mul(Node *node, int frame_count, double seconds_per_frame,
                      double seconds_offset, double schedule);
Node *node_mul(Node *node_a, Node *node_b);
void perform_node_add(Node *node, int frame_count, double seconds_per_frame,
                      double seconds_offset, double schedule);

Node *node_add(Node *node_a, Node *node_b);
Node *node_add_to_tail(Node *node, Node *tail);
void free_data(NodeData *data);
void free_node(Node *node);

int delay_til_schedule_time(double schedule, int frame, double seconds_offset,
                            double seconds_per_frame);

#define sched()                                                                \
  if (delay_til_schedule_time(schedule, i, seconds_offset,                     \
                              seconds_per_frame) == 1) {                       \
    break;                                                                     \
  };

#define ptr(name, member)                                                      \
  double *name;                                                                \
  if (member == NULL) {                                                        \
    return;                                                                    \
  } else                                                                       \
    name = member;
#endif
