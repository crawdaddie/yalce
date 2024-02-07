#ifndef _LANG_FFI_H
#define _LANG_FFI_H
#include "../node.h"
Node *sq_detune(double freq);
Node *impulse(double freq);
Node *lpf(double freq, double bw);

Node *chain(int sig_idx, Node *dest, Node *src);
Node *play_node(Node *node);

void node_set(Node *n, int index, double value);

void print_node(Node *n);
#endif
