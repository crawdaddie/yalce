#ifndef _CLOCK
#define _CLOCK
#include "queue.h"

typedef struct Clock {
  queue_t queue;
} Clock;

Clock get_clock() {
  queue_t queue = {0, 0, 200, malloc(sizeof(void *) * 200)};
  Clock clock = {.queue = queue};
  return clock;
}

typedef struct Routine {
} Routine;
typedef struct RoutineQueueItem {
  long time_ms;
} RoutineQueueItem;

void play_on_clock(Routine *routine, Clock *clock) {}

#endif
