#include "lib.h"
#include "audio_loop.h"
#include "ctx.h"
#include "oscillators.h"
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// Node *sq_node(double freq) { return NULL; }
// Node *sin_node(double freq) { return NULL; }

// node_perform _sum_perform(Node *node, int nframes, double spf) {
//
//   double *out = node->out.buf;
//   int num_ins = node->num_ins;
//   Signal *input_sigs = node->ins;
//   for (int i = 0; i < nframes; i++) {
//     out[i] = 0.;
//     for (int j = 0; j < num_ins; j++) {
//       out[i] += input_sigs[j].buf[i];
//     }
//   }
// }
//
// node_perform mul_perform(Node *node, int nframes, double spf) {
//   double *out = node->out.buf;
//   int num_ins = node->num_ins;
//   Signal *input_sigs = node->ins;
//
//   for (int i = 0; i < nframes; i++) {
//     out[i] = input_sigs[0].buf[i];
//     for (int j = 1; j < num_ins; j++) {
//       out[i] *= input_sigs[j].buf[i];
//     }
//   }
// }
//
Node *play_test_synth() {
  double freq = 100.;
  double cutoff = 500.;
  Node *group = group_new(0);

  Node *sq1 = sq_node(get_sig_default(1, freq));
  group_add_tail(group, sq1);

  Node *sq2 = sq_node(get_sig_default(1, freq * 1.01));
  group_add_tail(group, sq2);

  Node *summed = sum2_node(sq1, sq2);
  group_add_tail(group, summed);
  add_to_dac(summed);

  // return group;

  add_to_dac(group);
  audio_ctx_add(group);
  return group;
}

int get_frame_offset() {
  struct timespec t;
  struct timespec btime = get_block_time();
  set_block_time(&t);
  int frame_offset = get_block_frame_offset(btime, t, 48000);
  // printf("frame offset %d\n", frame_offset);
  return frame_offset;
}

Node *end_chain(Node *s) {
  Node *group = _chain;
  reset_chain();
  add_to_dac(s);
  return group;
}

Node *play(Node *group) {
  add_to_dac(group);

  push_msg(&ctx.msg_queue, (scheduler_msg){NODE_ADD,
                                           get_frame_offset(),
                                           {.NODE_ADD = {.target = group}}});
  return group;
}

Node *play_node(Node *s) {
  Node *group = _chain;
  reset_chain();
  add_to_dac(s);
  add_to_dac(group);

  push_msg(&ctx.msg_queue, (scheduler_msg){NODE_ADD,
                                           get_frame_offset(),
                                           {.NODE_ADD = {.target = group}}});
  // audio_ctx_add(group);
  return group;
}

Node *set_input_scalar(Node *node, int input, double value) {
  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_SET_SCALAR,
                           get_frame_offset(),
                           {.NODE_SET_SCALAR = {node, input, value}}});
  return node;
}

Node *set_input_scalar_offset(Node *node, int input, int frame_offset,
                              double value) {
  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_SET_SCALAR,
                           frame_offset,
                           {.NODE_SET_SCALAR = {node, input, value}}});
  return node;
}

Node *set_input_trig(Node *node, int input) {
  push_msg(&ctx.msg_queue, (scheduler_msg){NODE_SET_TRIG,
                                           get_frame_offset(),
                                           {.NODE_SET_TRIG = {node, input}}});
  return node;
}

Node *set_input_trig_offset(Node *node, int input, int frame_offset) {
  push_msg(&ctx.msg_queue, (scheduler_msg){NODE_SET_TRIG,
                                           frame_offset,
                                           {.NODE_SET_TRIG = {node, input}}});
  return node;
}

void accept_callback(int (*callback)(int, int)) {
  // Function body
  if (callback != NULL) {
    printf("called callback %p %d\n", callback,
           callback(1, 2)); // Call the callback function
  }
}

#define MAX_CHANNELS 6

int _read_file(const char *filename, Signal *signal, int *sf_sample_rate) {
  SNDFILE *infile;
  SF_INFO sfinfo;
  int readcount;
  memset(&sfinfo, 0, sizeof(sfinfo));

  if (!(infile =
            sf_open(filename, SFM_READ,
                    &sfinfo))) { /* Open failed so print an error message. */
    printf("Not able to open input file %s.\n", filename);
    /* Print the error message from libsndfile. */
    puts(sf_strerror(NULL));
    return 1;
  };

  if (sfinfo.channels > MAX_CHANNELS) {
    printf("Not able to process more than %d channels\n", MAX_CHANNELS);
    sf_close(infile);
    return 1;
  };

  size_t total_size = sfinfo.channels * sfinfo.frames;

  double *buf = calloc((int)total_size, sizeof(double));
  // double *buf = signal->buf;

  // reads channels in interleaved
  int read = sf_read_double(infile, buf, total_size);
  if (read != total_size) {
    printf("warning read failure, read %d != total size) %zu", read,
           total_size);
  }
  fprintf(stderr, "read %d frames from '%s' buf %p\n", read, filename, buf);

  sf_close(infile);
  signal->size = total_size;
  signal->layout = sfinfo.channels;
  signal->buf = buf;
  *sf_sample_rate = sfinfo.samplerate;
  return 0;
};

Signal *read_buf(const char *filename) {
  Signal *sig = malloc(sizeof(Signal));
  int sf_sample_rate;
  _read_file(filename, sig, &sf_sample_rate);
  return sig;
}
