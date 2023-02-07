#include "parse.c"
#include <pthread.h>
#include <stdio.h>

void parse_loop() {
  char input[2048];
  for (;;) {
    fgets(input, 2048, stdin);
    parse_synth(input, NULL);
    /* printf("you entered : %s\n", input); */
  }
}
int main(int argc, char **argv) {
  /* char synth_def[] = "sq > delay > out(0)"; */
  /* parse(synth_def); */

  for (;;) {
    parse_loop();
  }
  return 0;
}
