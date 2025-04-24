#ifndef _ENGINE_BUS_H
#define _ENGINE_BUS_H
#include "./node.h"

NodeRef pipe_into(NodeRef node, NodeRef filter);

NodeRef bus(int layout);

#endif
