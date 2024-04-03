#ifndef _GC_H
#define _GC_H
#include "graph.h"
#include "node.h"
Node *cleanup_graph(Graph *graph, Node *head);
void audio_ctx_gc();
void cleanup_job();
#endif
