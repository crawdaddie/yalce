#ifndef _ENGINE_SCHEDULING_H
#define _ENGINE_SCHEDULING_H
#include <pthread.h>
#include <stdint.h>
#include <time.h>

typedef void (*SchedulerCallback)(void *, int);

typedef void (*CoroutineSchedulerCallback)(void);

int scheduler_event_loop();

void schedule_event(SchedulerCallback callback, double delay_seconds,
                    void *userdata);

void schedule_event_quant(SchedulerCallback callback, double quantization,
                          void *userdata);

void schedule_coroutine_driver(CoroutineSchedulerCallback callback,
                               double quantization);

int get_tl_frame_offset(void);
#endif
