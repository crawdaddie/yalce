#ifndef _ENGINE_NODE_UTIL_H
#define _ENGINE_NODE_UTIL_H
#include "./node.h"
NodeRef mul2_node(NodeRef input1, NodeRef input2);
NodeRef sum2_node(NodeRef input1, NodeRef input2);
NodeRef sum2_node(NodeRef input1, NodeRef input2);
NodeRef mod2_node(NodeRef input1, NodeRef input2);

Node *const_sig(double val);
#endif
