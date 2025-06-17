#ifndef _ENGINE_GLUE_COMP_H
#define _ENGINE_GLUE_COMP_H

#include "node.h"
NodeRef glue_node(float threshold, float ratio, float attack_ms,
                  float release_ms, float makeup_gain, float knee_width,
                  NodeRef input);
#endif
