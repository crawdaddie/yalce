#ifndef _ENGINE_GRAPH_PERFORM_H
#define _ENGINE_GRAPH_PERFORM_H
#include "./node.h"

void perform_graph(Node *head, int frame_count, double spf, double *dac_buf,
                   int layout, int output_num);

void reset_buf_ptr();
void set_out_buf(NodeRef node);

void offset_node_bufs(Node *node, int frame_offset);
void unoffset_node_bufs(Node *node, int frame_offset);
#endif
