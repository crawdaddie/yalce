#ifndef _SCHEDULING
#define _SCHEDULING
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

enum { NS_PER_SECOND = 1000000000 };

void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td);
struct timespec get_time(void);
double timespec_to_secs(struct timespec ts);
int msleep(long msec);
int msleepd(double msec);

#endif
