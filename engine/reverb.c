#include "./common.h"
#include "./ctx.h"
#include "./filter.h"
#include "./node.h"
#include "audio_graph.h"
#include <math.h>
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
  struct {
    double fb;
    double filter_state;
    double damp1;
    double damp2;
    int length;
    int pos;
  } combs_left[FREEVERB_NUM_COMBS];

  struct {
    double fb;
    double filter_state;
    double damp1;
    double damp2;
    int length;
    int pos;
  } combs_right[FREEVERB_NUM_COMBS];

  struct {
    double fb;
    int length;
    int pos;
  } allpasses_left[FREEVERB_NUM_ALLPASSES];

  struct {
    double fb;
    int length;
    int pos;
  } allpasses_right[FREEVERB_NUM_ALLPASSES];
} Reverb;

void *reverb_perform(Node *node, Reverb *reverb, Node *inputs[], int nframes,
                     double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;

  /* Get pointers to all buffer memory allocated after the reverb struct */
  char *mem = (char *)(reverb + 1);

  /* Set up pointers to comb filter buffers */
  double *comb_buffers_left[FREEVERB_NUM_COMBS];
  double *comb_buffers_right[FREEVERB_NUM_COMBS];

  for (int i = 0; i < FREEVERB_NUM_COMBS; i++) {
    comb_buffers_left[i] = (double *)mem;
    mem += reverb->combs_left[i].length * sizeof(double);

    comb_buffers_right[i] = (double *)mem;
    mem += reverb->combs_right[i].length * sizeof(double);
  }

  /* Set up pointers to allpass filter buffers */
  double *allpass_buffers_left[FREEVERB_NUM_ALLPASSES];
  double *allpass_buffers_right[FREEVERB_NUM_ALLPASSES];
  for (int i = 0; i < FREEVERB_NUM_ALLPASSES; i++) {
    allpass_buffers_left[i] = (double *)mem;
    mem += reverb->allpasses_left[i].length * sizeof(double);

    allpass_buffers_right[i] = (double *)mem;
    mem += reverb->allpasses_right[i].length * sizeof(double);
  }

  /* Process each frame */
  for (int frame = 0; frame < nframes; frame++) {
    double input_sample = in[frame];
    double output_left = 0.0;
    double output_right = 0.0;

    /* Apply input gain */
    double input = input_sample * reverb->gain;

    /* Process through all comb filters in parallel */
    for (int i = 0; i < FREEVERB_NUM_COMBS; i++) {
      /* Process left channel comb */
      double *buffer_left = comb_buffers_left[i];
      int pos_left = reverb->combs_left[i].pos;
      int size_left = reverb->combs_left[i].length;

      /* Read from buffer */
      double output_l = buffer_left[pos_left];
      // if (fabs(output_l) < 1e-37)
      //   output_l = 0.0;

      /* Apply damping */
      reverb->combs_left[i].filter_state =
          output_l * reverb->combs_left[i].damp2 +
          reverb->combs_left[i].filter_state * reverb->combs_left[i].damp1;
      if (fabs(reverb->combs_left[i].filter_state) < 1e-37)
        reverb->combs_left[i].filter_state = 0.0;

      /* Write to buffer with feedback */
      buffer_left[pos_left] =
          input + reverb->combs_left[i].filter_state * reverb->combs_left[i].fb;

      /* Increment position */
      reverb->combs_left[i].pos = (pos_left + 1) % size_left;

      /* Accumulate output */
      output_left += output_l;

      /* Process right channel comb */
      double *buffer_right = comb_buffers_right[i];
      int pos_right = reverb->combs_right[i].pos;
      int size_right = reverb->combs_right[i].length;

      /* Read from buffer */
      double output_r = buffer_right[pos_right];
      // if (fabs(output_r) < 1e-37)
      //   output_r = 0.0;

      /* Apply damping */
      reverb->combs_right[i].filter_state =
          output_r * reverb->combs_right[i].damp2 +
          reverb->combs_right[i].filter_state * reverb->combs_right[i].damp1;
      if (fabs(reverb->combs_right[i].filter_state) < 1e-37)
        reverb->combs_right[i].filter_state = 0.0;

      /* Write to buffer with feedback */
      buffer_right[pos_right] = input + reverb->combs_right[i].filter_state *
                                            reverb->combs_right[i].fb;

      /* Increment position */
      reverb->combs_right[i].pos = (pos_right + 1) % size_right;

      /* Accumulate output */
      output_right += output_r;
    }

    /* Process through all allpass filters in series */
    for (int i = 0; i < FREEVERB_NUM_ALLPASSES; i++) {
      /* Process left channel allpass */
      double *buffer_left = allpass_buffers_left[i];
      int pos_left = reverb->allpasses_left[i].pos;
      int size_left = reverb->allpasses_left[i].length;

      /* Read from buffer */
      double delayed_left = buffer_left[pos_left];
      // if (fabs(delayed_left) < 1e-37)
      //   delayed_left = 0.0;

      /* Calculate output */
      double combined_left = -output_left + delayed_left;

      /* Write to buffer */
      buffer_left[pos_left] =
          output_left + delayed_left * reverb->allpasses_left[i].fb;

      /* Increment position */
      reverb->allpasses_left[i].pos = (pos_left + 1) % size_left;

      /* Update output */
      output_left = combined_left;

      /* Process right channel allpass */
      double *buffer_right = allpass_buffers_right[i];
      int pos_right = reverb->allpasses_right[i].pos;
      int size_right = reverb->allpasses_right[i].length;

      /* Read from buffer */
      double delayed_right = buffer_right[pos_right];
      // if (fabs(delayed_right) < 1e-37)
      //   delayed_right = 0.0;

      /* Calculate output */
      double combined_right = -output_right + delayed_right;

      /* Write to buffer */
      buffer_right[pos_right] =
          output_right + delayed_right * reverb->allpasses_right[i].fb;

      /* Increment position */
      reverb->allpasses_right[i].pos = (pos_right + 1) % size_right;

      /* Update output */
      output_right = combined_right;
    }

    /* Calculate stereo output with cross-mixing for width control */
    double l = output_left * reverb->wet1 + output_right * reverb->wet2 +
               input_sample * reverb->dry;
    double r = output_right * reverb->wet1 + output_left * reverb->wet2 +
               input_sample * reverb->dry;

    out[frame * 2] = l;
    out[frame * 2 + 1] = r;
  }

  return node->output.buf;
}

Node *reverb_node(double room_size, double wet, double dry, double width,
                  Node *input) {
  double sr = (double)ctx_sample_rate();

  /* Freeverb's tuned delay line lengths at 44.1kHz */
  const int comb_tunings[FREEVERB_NUM_COMBS] = {1116, 1188, 1277, 1356,
                                                1422, 1491, 1557, 1617};
  const int allpass_tunings[FREEVERB_NUM_ALLPASSES] = {556, 441, 341, 225};
  double rate_scale = sr / 44100.0;

  /* Initialize reverb with default values */
  Reverb reverb = {.room_size = room_size,
                   .damp = 0.0,
                   .wet = wet / FREEVERB_SCALE_WET,
                   .dry = dry,
                   .width = width,
                   .mode = 0.0,
                   .sample_rate = sr,
                   .gain = FREEVERB_FIXED_GAIN};

  /* Calculate derived parameters */
  reverb.room_size_scaled =
      reverb.room_size * FREEVERB_SCALE_ROOM + FREEVERB_OFFSET_ROOM;
  reverb.damp_scaled = reverb.damp * FREEVERB_SCALE_DAMP;
  reverb.wet1 = reverb.wet * FREEVERB_SCALE_WET * (reverb.width * 0.5 + 0.5);
  reverb.wet2 = reverb.wet * FREEVERB_SCALE_WET * ((1.0 - reverb.width) * 0.5);

  /* Calculate buffer sizes and initialize filter states */
  int total_buffer_size = 0;

  /* Set up comb filters */
  for (int i = 0; i < FREEVERB_NUM_COMBS; i++) {
    /* Left channel */
    reverb.combs_left[i].length = (int)(comb_tunings[i] * rate_scale);
    reverb.combs_left[i].pos = 0;
    reverb.combs_left[i].fb = reverb.room_size_scaled;
    reverb.combs_left[i].damp1 = reverb.damp_scaled;
    reverb.combs_left[i].damp2 = 1.0 - reverb.damp_scaled;
    reverb.combs_left[i].filter_state = 0.0;
    total_buffer_size += reverb.combs_left[i].length * sizeof(double);

    /* Right channel (with stereo spread) */
    reverb.combs_right[i].length =
        (int)((comb_tunings[i] + FREEVERB_STEREO_SPREAD) * rate_scale);
    reverb.combs_right[i].pos = 0;
    reverb.combs_right[i].fb = reverb.room_size_scaled;
    reverb.combs_right[i].damp1 = reverb.damp_scaled;
    reverb.combs_right[i].damp2 = 1.0 - reverb.damp_scaled;
    reverb.combs_right[i].filter_state = 0.0;
    total_buffer_size += reverb.combs_right[i].length * sizeof(double);
  }

  /* Set up allpass filters */
  for (int i = 0; i < FREEVERB_NUM_ALLPASSES; i++) {
    /* Left channel */
    reverb.allpasses_left[i].length = (int)(allpass_tunings[i] * rate_scale);
    reverb.allpasses_left[i].pos = 0;
    reverb.allpasses_left[i].fb = 0.5; /* Fixed allpass feedback */
    total_buffer_size += reverb.allpasses_left[i].length * sizeof(double);

    /* Right channel (with stereo spread) */
    reverb.allpasses_right[i].length =
        (int)((allpass_tunings[i] + FREEVERB_STEREO_SPREAD) * rate_scale);
    reverb.allpasses_right[i].pos = 0;
    reverb.allpasses_right[i].fb = 0.5; /* Fixed allpass feedback */
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
  char *mem = graph->nodes_state_memory + node->state_offset;
  memset(mem, 0, state_size);
  Reverb *state = (Reverb *)(mem);
  *state = reverb;

  node->connections[0].source_node_index = input->node_index;
  return node;
}
