#ifndef _ENGINE_LIB_H
#define _ENGINE_LIB_H
#include "node.h"

// NodeRef sq_node(double freq);
// NodeRef sin_node(double freq);
// // void group_add_tail(NodeRef group, NodeRef node);
// NodeRef sum_nodes(NodeRef l, NodeRef r);
//
// node_perform sum_perform(NodeRef node, int nframes, double spf);
//
// node_perform mul_perform(NodeRef node, int nframes, double spf);
//
NodeRef play_test_synth();

NodeRef play_node(NodeRef s);

// void accept_callback(int (*callback)(int, int));

SignalRef read_buf(const char *filename);

SignalRef read_buf_mono(const char *filename);

SignalRef out_sig(NodeRef n);

SignalRef input_sig(int i, NodeRef n);
SignalRef inlet(double default_val);

void reset_chain();

int num_inputs(NodeRef n);

SignalRef signal_of_double(double val);
SignalRef signal_of_int(int val);

NodeRef set_input_scalar(NodeRef node, int input, double value);

NodeRef set_input_trig(NodeRef node, int input);

NodeRef set_input_scalar_offset(NodeRef node, int input, int frame_offset,
                                double value);
NodeRef set_input_trig_offset(NodeRef node, int input, int frame_offset);
NodeRef end_chain(NodeRef s);

NodeRef play(NodeRef group);

int get_frame_offset();

#endif
