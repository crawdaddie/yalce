#ifndef _LOG_H
#define _LOG_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern FILE *log_stream;
extern void (*write_log)(const char *fmt, ...);
int logging_setup();
int logging_teardown();
// void write_log(const char *fmt, ...);
void write_fd_log(const char *format, ...);
void write_stdout_log(const char *format, ...);
#endif
