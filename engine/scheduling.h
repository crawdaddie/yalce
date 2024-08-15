#ifndef _ENGINE_SCHEDULING_H
#define _ENGINE_SCHEDULING_H
#include <pthread.h>
#include <stdint.h>
#include <time.h>
int scheduler_event_loop();

void schedule_event(void (*callback)(void *, uint64_t), void *userdata,
                    double delay_seconds);

#endif
