#ifndef _OUT_H
#define _OUT_H
#include "../common.h"
#include "../node.h"

void perform_add_out(Node *node, int nframes, double seconds_per_frame);

Node *add_out(double *node_out, double *channel_out);

void perform_replace_out(Node *node, int nframes, double seconds_per_frame);

Node *replace_out(double *node_out, double *channel_out);

#endif
