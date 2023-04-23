#ifndef _BUFPLAYER_H
#define _BUFPLAYER_H
#include "../node.h"

typedef enum {
  BUFPLAYER_BUF,
  BUFPLAYER_RATE,
  BUFPLAYER_TRIG,
  BUFPLAYER_STARTPOS,
  BUFPLAYER_NUM_INS
} bufplayer_sig_map;

typedef struct {
  double read_ptr;
  size_t buf_sample_rate;
  int loop;
} bufplayer_data;
/*
 ** create a node that plays a buffer
 ** Signal Inputs:
 ** [0] - buf
 ** [1] - rate is a scalar value with 1.0 meaning play at normal speed, 0.5 play
 ** an octave down, -1.0 play backwards
 ** [2] - trig - when trigger is 1 restart the bufplayer at whatever value is
 ** on start_pos
 ** [3] - start_pos value between 0.0 and 1 meaning 0.0 play from the first
 *frame of the buffer
 */
Node *bufplayer_node(Signal *buf, size_t buf_sample_rate, double rate,
                     double start_pos, int loop);

typedef enum {
  TIMESTRETCH_BUF,
  TIMESTRETCH_PITCHSHIFT,
  TIMESTRETCH_TRIG_FREQ,
  TIMESTRETCH_SPEED,
  TIMESTRETCH_NUM_INS
} timestretch_sig_map;

typedef struct {
  double read_ptr;
  double trig_ramp;
  double start_pos;
  size_t buf_sample_rate;
  int loop;
} timestretch_data;

Node *bufplayer_timestretch_node(Signal *buf, size_t buf_sample_rate,
                                 double rate, double pitchshift,
                                 double trig_freq, double start_pos, int loop);
#endif
