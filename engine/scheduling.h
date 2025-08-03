#ifndef _ENGINE_SCHEDULING_H
#define _ENGINE_SCHEDULING_H

#include <stdatomic.h>
#include <stdint.h>
typedef void (*SchedulerCallback)(void *, uint64_t);
typedef void (*DeferQuantCallback)(uint64_t);

typedef void (*CoroutineSchedulerCallback)(void);

int scheduler_event_loop();

void *schedule_event(uint64_t now, double delay_seconds,
                     SchedulerCallback callback, void *userdata);
int get_tl_frame_offset();
extern atomic_ullong global_sample_position;

uint64_t get_current_sample();
uint64_t get_tl_tick();

void defer_quant(double quant, DeferQuantCallback callback);
#endif
