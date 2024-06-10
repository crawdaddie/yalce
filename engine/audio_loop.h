#ifndef _ENGINE_AUDIO_LOOP_H
#define _ENGINE_AUDIO_LOOP_H
#include <stdint.h>
#include <time.h>
int init_audio();

struct timespec get_block_time();
struct timespec get_start_time();
uint64_t us_offset(struct timespec start, struct timespec end);
int block_sample_offset(struct timespec start, struct timespec end,
                        int sample_rate);
#endif
