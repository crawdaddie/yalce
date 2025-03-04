#include "lib.h"
#include "audio_loop.h"
#include "ctx.h"
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Node *_chain;
int get_frame_offset() {
  struct timespec t;
  struct timespec btime = get_block_time();
  set_block_time(&t);
  int frame_offset = get_block_frame_offset(btime, t, 48000);
  // printf("frame offset %d\n", frame_offset);
  return frame_offset;
}

void reset_chain() { _chain = NULL; }
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
  // printf("play node %p\n", s);
  // Node *group = _chain;
  // reset_chain();
  // add_to_dac(s);
  // add_to_dac(group);

  push_msg(&ctx.msg_queue, (scheduler_msg){NODE_ADD,
                                           get_frame_offset(),
                                           {.NODE_ADD = {.target = s}}});
  // audio_ctx_add(s);
  return s;
}

Node *play_node_offset(int offset, Node *s) {
  printf("play node %p at offset %d\n", s, offset);
  // Node *group = _chain;
  // reset_chain();
  // add_to_dac(s);
  // add_to_dac(group);

  push_msg(&ctx.msg_queue,
           (scheduler_msg){NODE_ADD, offset, {.NODE_ADD = {.target = s}}});
  return s;
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

// void accept_callback(int (*callback)(int, int)) {
//   // Function body
//   if (callback != NULL) {
//     printf("called callback %p %d\n", callback,
//            callback(1, 2)); // Call the callback function
//   }
// }

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
  fprintf(stderr, "read %d frames from '%s' buf %p [%d]\n", read, filename, buf,
          sfinfo.channels);

  sf_close(infile);
  signal->size = total_size;
  signal->layout = sfinfo.channels;
  signal->buf = buf;
  *sf_sample_rate = sfinfo.samplerate;
  return 0;
};

int _read_file_mono(const char *filename, Signal *signal, int *sf_sample_rate) {
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

  double *_buf = calloc((int)total_size, sizeof(double));
  // double *buf = signal->buf;

  // reads channels in interleaved
  int read = sf_read_double(infile, _buf, total_size);
  if (read != total_size) {
    printf("warning read failure, read %d != total size) %zu", read,
           total_size);
  }
  fprintf(stderr, "read %d frames from '%s' buf %p [%d]\n", read, filename,
          _buf, sfinfo.channels);

  sf_close(infile);
  double *buf = calloc((int)total_size / 2, sizeof(double));
  for (int i = 0; i < total_size / 2; i++) {
    buf[i] = (_buf[2 * i] + _buf[2 * i + 1]);
    // * 0.5;
  }
  free(_buf);

  signal->size = total_size / 2;
  signal->layout = 1;
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

Signal *read_buf_mono(const char *filename) {
  Signal *sig = malloc(sizeof(Signal));
  int sf_sample_rate;
  _read_file_mono(filename, sig, &sf_sample_rate);
  return sig;
}

// SignalRef inlet(double default_val) {
//   Signal *sig;
//   if (_chain == NULL) {
//     _chain = group_new(1);
//     sig = _chain->ins;
//   } else {
//     sig = group_add_input(_chain);
//   }
//
//   for (int i = 0; i < sig->size; i++) {
//     sig->buf[i] = default_val;
//     // printf("sig val %f\n", sig->buf[i]);
//   }
//
//   return sig;
// }

double *raw_signal_data(SignalRef sig) { return sig->buf; }
int signal_size(SignalRef sig) { return sig->size; }

SignalRef signal_of_ptr(int size, double *ptr) {
  Signal *sig = malloc(sizeof(Signal));
  sig->buf = ptr;
  sig->size = size;
  sig->layout = 1;
  return sig;
}

SignalRef node_output_sig(NodeRef node) { return &node->out; }
