#ifndef _SCHED_H
#define _SCHED_H
#include "config.h"
#include "scheduling.h"

static int current_frame = 0;
static double callback_time = 0;
static double clock_start = 0;

void init_sched() { clock_start = timespec_to_secs(get_time()); }
double sched_clock_start() { return clock_start; }

void sched_incr_time(double time) { callback_time += time; }
double sched_get_time() { return callback_time; }

void sched_set_frame() { current_frame = (current_frame + 1) % BUF_SIZE; }

int sched_get_frame() { return current_frame; }

#endif
