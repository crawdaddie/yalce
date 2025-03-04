#ifndef _ENGINE_SIGNALS_H
#define _ENGINE_SIGNALS_H
#include "node.h"

SignalRef inlet(double default_val);

SignalRef out_sig(NodeRef);

double *get_val(SignalRef);

SignalRef get_sig_default(int layout, double value);
SignalRef signal_of_double(double val);

SignalRef get_node_input(NodeRef node, int input);
#endif
