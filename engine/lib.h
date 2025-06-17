#ifndef _ENGINE_LIB_H
#define _ENGINE_LIB_H
#include "./audio_graph.h"

typedef struct {
  struct {
    int idx;
    double val;
  } pair;
  struct InValList *next;
} InValList;

Node *instantiate_template(InValList *input_vals, AudioGraph *g);

struct arr {
  int size;
  void *data;
};

NodeRef array_to_buf(struct arr a);
double midi_to_freq(int midi_note);

void write_to_dac(int dac_layout, float *dac_buf, int _layout, float *buf,
                  int output_num, int nframes);
extern AudioGraph *_graph;

void perform_graph(Node *head, int frame_count, float spf, float *dac_buf,
                   int layout, int output_num);

typedef void (*SynthTemplateFunc)();
AudioGraph *compile_blob_template(SynthTemplateFunc fn);
#endif
