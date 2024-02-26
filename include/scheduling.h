#ifndef _SCHEDULING_H
#define _SCHEDULING_H
#include <time.h>
#include <unistd.h>

#define NS_PER_SECOND 1000000000
void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td);
double timespec_to_secs(struct timespec ts);
int msleep(long msec);
int msleepd(double msec);

double diff_timespec(const struct timespec *time1,
                     const struct timespec *time0);

double timespec_diff(struct timespec a, struct timespec b);

void init_scheduling();
double get_time();
// int thread_func();
#endif
