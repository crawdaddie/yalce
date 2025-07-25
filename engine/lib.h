#ifndef _ENGINE_LIB_H
#define _ENGINE_LIB_H
#include "./audio_graph.h"

typedef struct InValList {
  struct {
    int idx;
    double val;
  } pair;
  struct InValList *next;
} InValList;

typedef struct {
  struct {
    int idx;
    NodeRef sig;
  } pair;
  struct InValList *next;
} InNodeList;

Node *instantiate_template(InValList *input_vals, AudioGraph *g);

struct arr {
  int size;
  void *data;
};

NodeRef array_to_buf(struct arr a);

void write_to_dac(int dac_layout, double *dac_buf, int _layout, double *buf,
                  int output_num, int nframes);
extern AudioGraph *_graph;

void perform_graph(Node *head, int frame_count, double spf, double *dac_buf,
                   int layout, int output_num);

typedef void (*SynthTemplateFunc)();
AudioGraph *compile_blob_template(SynthTemplateFunc fn);
#endif
