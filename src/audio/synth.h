#ifndef _SYNTH
#define _SYNTH
#include "node.h"

typedef struct synth_data {
  Node *graph;
} synth_data;

Node *perform_graph(Node *graph, int frame_count, double seconds_per_frame,
                    double seconds_offset);
void perform_synth_graph(Node *synth, int frame_count, double seconds_per_frame,
                         double seconds_offset);

void on_free(Node *synth);
static int synth_sample_rate = 48000;

Node *get_synth(double freq, double *bus);
Node *node_play_synth(Node *head, double freq, double *bus);
#endif
