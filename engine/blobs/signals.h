#ifndef _ENGINE_SIGNALS_H
#define _ENGINE_SIGNALS_H
#include "node.h"

SignalRef inlet(double default_val);

void out_sig(NodeRef node, SignalRef sig);

double *get_val(SignalRef);

SignalRef get_sig_default(int layout, double value);
SignalRef signal_of_double(double val);

double *get_node_input_buf(NodeRef node, int input);
int get_node_input_size(NodeRef node, int input);

int get_node_input_layout(NodeRef node, int input);

double *get_node_out_buf(NodeRef node);

int get_node_out_layout(NodeRef node);
#endif
