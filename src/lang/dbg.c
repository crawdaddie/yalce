#include "dbg.h"
void printf_color(char *fmt, int ansi, ...) {
  va_list args;
  va_start(args, ansi);
  printf("\033[%dm", ansi);
  vprintf(fmt, args);
  printf("\033[%dm", 37);
  printf("");
  va_end(args);
}
