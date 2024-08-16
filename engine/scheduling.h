#ifndef _ENGINE_SCHEDULING_H
#define _ENGINE_SCHEDULING_H
#include <pthread.h>
#include <stdint.h>
#include <time.h>
int scheduler_event_loop();

typedef struct Timer {
  double now;
  int frame_offset;
  uint64_t _abs_time;
  uint64_t _timer_start;
} Timer;

typedef struct {
  void (*callback)(double phase, int frame_offset, void *userdata);
  void *userdata;
  uint64_t timestamp;
  Timer timer;
} ScheduledEvent;

typedef struct {
  ScheduledEvent *events;
  size_t capacity;
  size_t size;
} EventHeap;

void schedule_event(void (*callback)(void *, uint64_t), void *userdata,
                    double delay_seconds);

void _schedule_event(EventHeap *heap, void (*callback)(void *, uint64_t),
                     void *userdata, double delay_seconds, uint64_t now);

void schedule(void (*callback)(double, int, void *), void *userdata,
              double delay_seconds);

Timer new_timer();
int timer_frame_offset(Timer t);
double timer_phase(Timer t);
#endif
