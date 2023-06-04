#include "log.h"
#include <stdarg.h>
#include <time.h>

FILE *log_file;
FILE *log_stream;

void write_fd_log(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(log_stream, format, args);
  va_end(args);
  fflush(log_stream);
}

void write_stdout_log(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

int logging_setup() {
  // Open a file for writing
  log_file = fopen("log.txt", "w");
  if (log_file == NULL) {
    perror("Error opening log file");
    exit(EXIT_FAILURE);
  }

  // Create a file stream from the file descriptor
  int log_fd = fileno(log_file);
  log_stream = fdopen(log_fd, "w");
  if (log_stream == NULL) {
    perror("Error creating log stream");
    exit(EXIT_FAILURE);
  }

  return 0;
}
int logging_teardown() {

  fflush(log_stream);

  // Close the log file
  fclose(log_file);
  fclose(log_stream);
  return 0;
}

void (*write_log)(const char *fmt, ...);
