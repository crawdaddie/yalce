#include "lang_runner.h"

int main(int argc, char **argv) {
  int repl = 0;
  repl = argc == 1;

  init_vm();

  if (argc == 2) {
    char *filename = argv[1];
    run_file(filename);
  }

  if (repl) {
    char input[2048];
    for (;;) {
      fgets(input, 2048, stdin);
      /* parse_line(input, 0); */
      interpret(input);
    }
  }
  free_vm();
}
