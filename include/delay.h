#ifndef _DELAY_H
#define _DELAY_H

#include "node.h"
typedef struct {
  double a0;
  double b1;
  double mem;
} op_lp_state;

node_perform op_lp_perform(Node *node, int nframes, double spf);

void set_op_lp_params(op_lp_state *state, double freq);
Node *op_lp_node(double freq, Node *input);

typedef struct {
  double mem;
} op_lp_dyn_state;

node_perform op_lp_dyn_perform(Node *node, int nframes, double spf);

void set_op_dyn_lp_params(op_lp_dyn_state *state, double freq);

Node *op_lp_dyn_node(double freq, Node *input);

typedef struct {
  double *buf;
  int buf_size;
  int read_pos;
  int write_pos;
  double fb;
} comb_state;

node_perform comb_perform(Node *node, int nframes, double spf);

void set_comb_params(comb_state *state, double delay_time,
                     double max_delay_time, double fb);

Node *comb_node(double delay_time, double max_delay_time, double fb,
                Node *input);

typedef struct {
  double *buf;
  int buf_size;
  int write_pos;
  double fb;
} comb_dyn_state;

Node *comb_dyn_node(double delay_time, double max_delay_time, double fb,
                    Node *input);

// typedef struct {
//   comb_dyn_state state;
//   double damp;
// } comb_damp_state;

// ------------------------------------------- ALLPASS DELAY
typedef struct {
  double *buf;
  int buf_size;
  int read_pos;
  int write_pos;
} allpass_state;

node_perform allpass_perform(Node *node, int nframes, double spf);
void set_allpass_params(allpass_state *state, double delay_time);

Node *allpass_node(double delay_time, double max_delay_time, Node *input);
// ------------------------------------------- FREEVERB
//
// double comb_delay_lengths[] = {1617, 1557, 1491, 1422, 1356, 1277, 1188,
// 1116};
// the original values are optimised for 44100 sample rate
//
typedef struct {
  comb_state parallel_combs[16];
  op_lp_state comb_lps[16];
  allpass_state series_ap[8]
} freeverb_state;

node_perform freeverb_perform(Node *node, int nframes, double spf);
Node *freeverb_node(Node *input);
#endif
