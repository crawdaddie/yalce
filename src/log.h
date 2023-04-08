#ifndef _LOG_H
#define _LOG_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern FILE *log_stream;
int logging_setup();
int logging_teardown();
void write_log(const char *fmt, ...);
#endif
