#ifndef _ENGINE_AUDIO_LOOP_H
#define _ENGINE_AUDIO_LOOP_H
#include <stdint.h>
#include <time.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

int init_audio(int *inconf);

void set_block_time(struct timespec *to_set);
struct timespec get_block_time();
struct timespec get_start_time();
uint64_t us_offset(struct timespec start, struct timespec end);
int get_block_frame_offset(struct timespec start, struct timespec end,
                           int sample_rate);

uint64_t get_frame_offset();
#endif
