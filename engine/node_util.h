#ifndef _ENGINE_NODE_UTIL_H
#define _ENGINE_NODE_UTIL_H
#include "./node.h"

NodeRef sum2_node(NodeRef input1, NodeRef input2);
NodeRef mul2_node(NodeRef input1, NodeRef input2);
NodeRef sub2_node(NodeRef input1, NodeRef input2);
NodeRef mod2_node(NodeRef input1, NodeRef input2);

NodeRef const_sig(double val);
#endif
