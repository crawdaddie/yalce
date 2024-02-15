#include "log.h"
#include <stdarg.h>
#include <time.h>

FILE *log_file;
FILE *log_stream;
void write_log(const char *format, ...) {
  va_list args;
  va_start(args, format);
  /* fprintf(log_stream, "[%5.f] ", (double)clock() / (double)CLOCKS_PER_SEC);
   */
  vprintf(format, args);
  // printf(format,

  va_end(args);
  // fflush(log_stream);
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

  /* // Print to the new output stream */
  /* fprintf(log_stream, "This is a message for the log\n"); */
  /*  */
  /* // Flush the output stream */
  /* fflush(log_stream); */
  /*  */
  /* // Close the log file */
  /* fclose(log_file); */

  return 0;
}
int logging_teardown() {

  fflush(log_stream);

  // Close the log file
  fclose(log_file);
  fclose(log_stream);
  return 0;
}
