#include "./common.h"
#include "./ctx.h"
#include "./filter.h"
#include "./node.h"
#include "audio_graph.h"
#include "node_util.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define FREEVERB_NUM_COMBS 8
#define FREEVERB_NUM_ALLPASSES 4
#define FREEVERB_MUTED 0.0
#define FREEVERB_FIXED_GAIN 0.015
#define FREEVERB_SCALE_WET 3.0
#define FREEVERB_SCALE_DRY 2.0
#define FREEVERB_SCALE_DAMP 0.4
#define FREEVERB_SCALE_ROOM 0.28
#define FREEVERB_STEREO_SPREAD 23
#define FREEVERB_OFFSET_ROOM 0.7
#define FREEVERB_DEFAULT_ROOM 0.5
#define FREEVERB_DEFAULT_DAMP 0.5
#define FREEVERB_DEFAULT_WET (1.0 / FREEVERB_SCALE_WET)
#define FREEVERB_DEFAULT_DRY 0.5
#define FREEVERB_DEFAULT_WIDTH 1.0
#define FREEVERB_DEFAULT_MODE 0.0
#define FREEVERB_DEFAULT_SAMPLE_RATE 44100.0
#define FREEVERB_FREEZE_MODE 0.5

#define undenormalize(n)                                                       \
  {                                                                            \
    if (fabs(n) < 1e-37) {                                                     \
      (n) = 0;                                                                 \
    }                                                                          \
  }
struct ReverbComb {
  double fb;
  double filter_state;
  double damp1;
  double damp2;
  int length;
  int pos;
} ReverbComb;

struct ReverbAllPass {
  double fb;
  int length;
  int pos;
} ReverbAllPass;

typedef struct Reverb {
  /* User parameters */
  double room_size;
  double damp;
  double wet;
  double dry;
  double width;
  double mode;
  double sample_rate;

  /* Derived parameters */
  double gain;
  double room_size_scaled;
  double damp_scaled;
  double wet1;
  double wet2;

  /* Processing elements */
  struct ReverbComb combs_left[FREEVERB_NUM_COMBS];
  struct ReverbComb combs_right[FREEVERB_NUM_COMBS];

  struct ReverbAllPass allpasses_left[FREEVERB_NUM_ALLPASSES];
  struct ReverbAllPass allpasses_right[FREEVERB_NUM_ALLPASSES];
} Reverb;

void *reverb_perform(Node *node, Reverb *reverb, Node *inputs[], int nframes,
                     double spf) {
  if (!inputs) {
    return NULL;
  }

  Signal _out = node->output;
  Signal _in = inputs[0]->output;
  double *in_buf = inputs[0]->output.buf;
  int in_layout = inputs[0]->output.layout;

  char *mem = (char *)(reverb + 1);

  double *comb_buffers_left[FREEVERB_NUM_COMBS];
  double *comb_buffers_right[FREEVERB_NUM_COMBS];

  for (int i = 0; i < FREEVERB_NUM_COMBS; i++) {
    comb_buffers_left[i] = (double *)mem;
    mem += reverb->combs_left[i].length * sizeof(double);

    comb_buffers_right[i] = (double *)mem;
    mem += reverb->combs_right[i].length * sizeof(double);
  }

  double *allpass_buffers_left[FREEVERB_NUM_ALLPASSES];
  double *allpass_buffers_right[FREEVERB_NUM_ALLPASSES];
  for (int i = 0; i < FREEVERB_NUM_ALLPASSES; i++) {
    allpass_buffers_left[i] = (double *)mem;
    mem += reverb->allpasses_left[i].length * sizeof(double);

    allpass_buffers_right[i] = (double *)mem;
    mem += reverb->allpasses_right[i].length * sizeof(double);
  }

  for (int frame = 0; frame < nframes; frame++) {
    double input_sample = 0.;
    for (int i = 0; i < in_layout; i++) {
      input_sample += *in_buf;
      in_buf++;
    }

    double output_left = 0.0;
    double output_right = 0.0;

    double input = input_sample * reverb->gain;

    for (int i = 0; i < FREEVERB_NUM_COMBS; i++) {
      double *buffer_left = comb_buffers_left[i];
      int pos_left = reverb->combs_left[i].pos;
      int size_left = reverb->combs_left[i].length;

      double output_l = buffer_left[pos_left];
      undenormalize(output_l);

      reverb->combs_left[i].filter_state =
          output_l * reverb->combs_left[i].damp2 +
          reverb->combs_left[i].filter_state * reverb->combs_left[i].damp1;

      undenormalize(reverb->combs_left[i].filter_state);

      buffer_left[pos_left] =
          input + reverb->combs_left[i].filter_state * reverb->combs_left[i].fb;

      reverb->combs_left[i].pos = (pos_left + 1) % size_left;

      output_left += output_l;

      double *buffer_right = comb_buffers_right[i];
      int pos_right = reverb->combs_right[i].pos;
      int size_right = reverb->combs_right[i].length;

      double output_r = buffer_right[pos_right];
      undenormalize(output_r);

      reverb->combs_right[i].filter_state =
          output_r * reverb->combs_right[i].damp2 +
          reverb->combs_right[i].filter_state * reverb->combs_right[i].damp1;

      undenormalize(reverb->combs_right[i].filter_state);

      buffer_right[pos_right] = input + reverb->combs_right[i].filter_state *
                                            reverb->combs_right[i].fb;

      reverb->combs_right[i].pos = (pos_right + 1) % size_right;

      output_right += output_r;
    }

    for (int i = 0; i < FREEVERB_NUM_ALLPASSES; i++) {
      double *buffer_left = allpass_buffers_left[i];
      int pos_left = reverb->allpasses_left[i].pos;
      int size_left = reverb->allpasses_left[i].length;

      double delayed_left = buffer_left[pos_left];
      undenormalize(delayed_left);

      double combined_left = -output_left + delayed_left;

      buffer_left[pos_left] =
          output_left + delayed_left * reverb->allpasses_left[i].fb;

      reverb->allpasses_left[i].pos = (pos_left + 1) % size_left;

      output_left = combined_left;

      double *buffer_right = allpass_buffers_right[i];
      int pos_right = reverb->allpasses_right[i].pos;
      int size_right = reverb->allpasses_right[i].length;
      double delayed_right = buffer_right[pos_right];
      undenormalize(delayed_right);

      double combined_right = -output_right + delayed_right;

      buffer_right[pos_right] =
          output_right + delayed_right * reverb->allpasses_right[i].fb;

      reverb->allpasses_right[i].pos = (pos_right + 1) % size_right;

      output_right = combined_right;
    }

    double l = output_left * reverb->wet1 + output_right * reverb->wet2 +
               input_sample * reverb->dry;

    double r = output_right * reverb->wet1 + output_left * reverb->wet2 +
               input_sample * reverb->dry;

    WRITEV(_out, l);
    WRITEV(_out, r);
  }

  return node->output.buf;
}

Node *reverb_node(double room_size, double wet, double dry, double width,
                  Node *input) {
  double sr = (double)ctx_sample_rate();

  const int comb_tunings[FREEVERB_NUM_COMBS] = {1116, 1188, 1277, 1356,
                                                1422, 1491, 1557, 1617};
  const int allpass_tunings[FREEVERB_NUM_ALLPASSES] = {556, 441, 341, 225};
  double rate_scale = sr / 44100.0;

  Reverb reverb = {.room_size = room_size,
                   .damp = 0.0,
                   .wet = wet / FREEVERB_SCALE_WET,
                   .dry = dry,
                   .width = width,
                   .mode = 0.0,
                   .sample_rate = sr,
                   .gain = FREEVERB_FIXED_GAIN};

  reverb.room_size_scaled =
      reverb.room_size * FREEVERB_SCALE_ROOM + FREEVERB_OFFSET_ROOM;
  reverb.damp_scaled = reverb.damp * FREEVERB_SCALE_DAMP;
  reverb.wet1 = reverb.wet * FREEVERB_SCALE_WET * (reverb.width * 0.5 + 0.5);
  reverb.wet2 = reverb.wet * FREEVERB_SCALE_WET * ((1.0 - reverb.width) * 0.5);

  int total_buffer_size = 0;
  struct ReverbComb base_comb = {
      .pos = 0,
      .fb = reverb.room_size_scaled,
      .damp1 = reverb.damp_scaled,
      .damp2 = 1.0 - reverb.damp_scaled,
      .filter_state = 0.0,
  };
  for (int i = 0; i < FREEVERB_NUM_COMBS; i++) {
    *(reverb.combs_left + i) = base_comb;
    reverb.combs_left[i].length = (int)(comb_tunings[i] * rate_scale);
    total_buffer_size += reverb.combs_left[i].length * sizeof(double);

    *(reverb.combs_right + i) = base_comb;
    reverb.combs_right[i].length =
        (int)((comb_tunings[i] + FREEVERB_STEREO_SPREAD) * rate_scale);
    total_buffer_size += reverb.combs_right[i].length * sizeof(double);
  }

  struct ReverbAllPass base_allpass = {
      .pos = 0,
      .fb = 0.5,
  };
  for (int i = 0; i < FREEVERB_NUM_ALLPASSES; i++) {
    *(reverb.allpasses_left + i) = base_allpass;
    reverb.allpasses_left[i].length = (int)(allpass_tunings[i] * rate_scale);
    total_buffer_size += reverb.allpasses_left[i].length * sizeof(double);

    *(reverb.allpasses_right + i) = base_allpass;
    reverb.allpasses_right[i].length =
        (int)((allpass_tunings[i] + FREEVERB_STEREO_SPREAD) * rate_scale);
    total_buffer_size += reverb.allpasses_right[i].length * sizeof(double);
  }

  /* Allocate state memory including space for all buffers */
  int state_size = sizeof(Reverb) + total_buffer_size;
  state_size = (state_size + 7) & ~7; /* Align to 8-byte boundary */

  AudioGraph *graph = _graph;
  Node *node = allocate_node_in_graph(graph, state_size);

  *node = (Node){
      .perform = (perform_func_t)reverb_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = state_size,
      .state_offset = state_offset_ptr_in_graph(graph, state_size),
      .output = (Signal){.layout = 2,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, 2 * BUF_SIZE)},
      .meta = "freeverb",
  };

  /* Initialize state memory */
  char *mem = (graph != NULL)
                  ? (char *)(graph->nodes_state_memory + node->state_offset)
                  : (char *)((Node *)node + 1);
  memset(mem, 0, state_size);
  Reverb *state = (Reverb *)(mem);
  *state = reverb;
  plug_input_in_graph(0, node, input);
  return graph_embed(node);
}
