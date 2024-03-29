#ifndef _LIB_AUDIO_SETUP_H
#define _LIB_AUDIO_SETUP_H

#include "ctx.h"
#include "dbg.h"
#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int start_audio();
int stop_audio();

void get_block_time(struct timespec *time);

#endif
