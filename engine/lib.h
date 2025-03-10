#ifndef _ENGINE_LIB_H
#define _ENGINE_LIB_H
#include "./audio_graph.h"

AudioGraph *sin_ensemble();

typedef struct {
  struct {
    int idx;
    double val;
  } pair;
  struct InValList *next;
} InValList;

Node *instantiate_template(AudioGraph *g, InValList *input_vals);
extern AudioGraph *_graph;
#endif
