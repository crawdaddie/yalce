#ifndef _ENGINE_EXT_LIB_H
#define _ENGINE_EXT_LIB_H
#include "./audio_graph.h"

void start_blob();
AudioGraph *end_blob();

NodeRef inlet(double default_val);

#endif
