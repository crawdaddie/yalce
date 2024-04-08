#ifndef _AUDIO_LOOP_H
#define _AUDIO_LOOP_H
#include <time.h>

void get_block_time(struct timespec *time);

int stop_audio();

int start_audio();
#endif
