#ifndef _ENGINE_NODE_EXPORTS_H
#define _ENGINE_NODE_EXPORTS_H
#include "./node.h"
NodeRef set_input_scalar(NodeRef node, int input, double value);
NodeRef set_input_trig(NodeRef node, int input);
NodeRef set_input_scalar_offset(NodeRef node, int input, int frame_offset,
                                double value);
NodeRef set_input_trig_offset(NodeRef node, int input, int frame_offset);
#endif
