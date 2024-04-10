#ifndef _SCHEDULING_H
#define _SCHEDULING_H
#include <time.h>
#include <unistd.h>

#define NS_PER_SECOND 1000000000

extern struct timespec block_time;
extern struct timespec start_time;
void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td);
double timespec_to_secs(struct timespec ts);
int msleep(long msec);
int msleepd(double msec);

double diff_timespec(const struct timespec *time1,
                     const struct timespec *time0);

double timespec_diff(struct timespec a, struct timespec b);

void init_scheduling();
double get_time();

double get_block_diff();

void get_block_time(struct timespec *time);

void print_block_time(double mod);

void set_block_time();

void handle_scheduler_tick(double mod);


typedef struct clock_continuation {
  struct timespec time;
  void *callback;
  struct clock_continuation *next;
} clock_continuation;

#define SCHEDULER_QUEUE_MAX_SIZE 100000

typedef struct {
  clock_continuation *head;
  clock_continuation *tail;
} clock_queue_t;

void _clock_push(clock_queue_t *queue, clock_continuation cont);


void clock_push(void *callback, double wait_time);

extern clock_queue_t clock_queue;
#endif
