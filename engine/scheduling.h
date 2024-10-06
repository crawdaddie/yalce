#ifndef _ENGINE_SCHEDULING_H
#define _ENGINE_SCHEDULING_H
#include <pthread.h>
#include <stdint.h>
#include <time.h>
int scheduler_event_loop();

void schedule_event(void (*callback)(void *, int), double delay_seconds,
                    void *userdata);

void schedule_event_quant(void (*callback)(void *, int), double quantization,
                    void *userdata);

#endif
