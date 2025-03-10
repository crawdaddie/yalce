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

NodeRef play_node_offset(int offset, NodeRef s);

// void accept_callback(int (*callback)(int, int));

SignalRef read_buf(const char *filename);

SignalRef read_buf_mono(const char *filename);

SignalRef out_sig(NodeRef n);

SignalRef input_sig(int i, NodeRef n);
// SignalRef inlet(double default_val);

void reset_chain();

int num_inputs(NodeRef n);

SignalRef signal_of_double(double val);
SignalRef signal_of_int(int val);

NodeRef end_chain(NodeRef s);

NodeRef play(NodeRef group);

int get_frame_offset();

double *raw_signal_data(SignalRef sig);
int signal_size(SignalRef sig);

SignalRef signal_of_ptr(int size, double *ptr);

void *node_state_ptr(NodeRef node);

Node *instantiate_blob_template(BlobTemplate *template);
#endif
