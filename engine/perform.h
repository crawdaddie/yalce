#ifndef _ENGINE_GRAPH_PERFORM_H
#define _ENGINE_GRAPH_PERFORM_H
#include "./node.h"

void perform_graph(Node *head, int frame_count, double spf, double *dac_buf,
                   int layout, int output_num);
#endif
