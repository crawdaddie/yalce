#include "src/lang/lang_runner.h"
#define INPUT_BUFSIZE 2048
void read_chars(char *input, int bufsize) {
  char prev;
  char c;
  int position = 0;
  while (1) {
    prev = c;
    c = getchar();

    if (c == '\\') {
      input[position] = '\n';
    } else if (c == EOF || c == '\n') {
      if (prev == '\\') {
        printf("> ");
        return read_chars(input + position, bufsize);
      } else {
        input[position] = '\0';
        return;
      }
    } else {
      input[position] = c;
    }
    position++;
  }
}

int main(int argc, char **argv) {
  setbuf(stdout, NULL); // turn off line buffering for stdout
  int repl = 0;
  repl = argc == 1;

  init_vm();

  if (argc == 2) {
    char *filename = argv[1];
    run_file(filename);
  }

  if (repl) {
    char *input = malloc(sizeof(char) * INPUT_BUFSIZE);
    for (;;) {

      printf("> ");
      read_chars(input, INPUT_BUFSIZE);
      /* fgets(input, 2048, stdin); */
      /* parse_line(input, 0); */
      interpret(input);
    }
  }
  free_vm();
}
