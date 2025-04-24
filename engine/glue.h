#ifndef _ENGINE_GLUE_COMP_H
#define _ENGINE_GLUE_COMP_H

#include "node.h"
NodeRef glue_node(double threshold, double ratio, double attack_ms,
                  double release_ms, double makeup_gain, double knee_width,
                  NodeRef input);
#endif
