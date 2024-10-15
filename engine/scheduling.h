#ifndef _ENGINE_SCHEDULING_H
#define _ENGINE_SCHEDULING_H
#include <pthread.h>

typedef void (*SchedulerCallback)(void *, int);

int scheduler_event_loop();

void schedule_event(SchedulerCallback callback, double delay_seconds,
                    void *userdata);

void schedule_event_quant(SchedulerCallback callback, double quantization,
                          void *userdata);

#endif
