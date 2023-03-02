#ifndef _OUT_H
#define _OUT_H
#include "../common.h"
#include "../graph/graph.h"

void perform_add_out(Graph *node, int nframes, double seconds_per_frame);

Graph *add_out(double *node_out, double *channel_out);

void perform_replace_out(Graph *node, int nframes, double seconds_per_frame);

Graph *replace_out(double *node_out, double *channel_out);

#endif
