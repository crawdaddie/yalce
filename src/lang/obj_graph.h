#ifndef _LANG_OBJ_GRAPH_H
#define _LANG_OBJ_GRAPH_H

#include "../graph/graph.h"
#include "obj.h"
typedef struct {
  Object object;
  Graph *graph;
} ObjGraph;
#endif
