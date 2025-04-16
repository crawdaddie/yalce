#ifndef _ENGINE_SCHEDULING_H
#define _ENGINE_SCHEDULING_H
#include <pthread.h>
#include <stdint.h>
#include <time.h>

typedef void (*SchedulerCallback)(void *, int);
typedef void (*DeferQuantCallback)(int);

typedef void (*CoroutineSchedulerCallback)(void);

int scheduler_event_loop();

void schedule_event(SchedulerCallback callback, void *userdata,
                    double delay_seconds);

// void schedule_event_quant(SchedulerCallback callback, double quantization,
//                           void *userdata);

int get_tl_frame_offset();

int get_frame_offset();

void defer_quant(double quantization, DeferQuantCallback cb);

#endif
