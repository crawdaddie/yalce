
#include <dlfcn.h>
#include <stdio.h>
void *dll = NULL;
void process_input(const char input[2048], int length) {
  dlerror();
  void *sym = dlsym(dll, input);
  char *error;
  int (*func)();

  if ((error = dlerror()) != NULL) {
    fprintf(stderr, "%s\n", error);
    return;
  }
  printf("input: %s found symbol %p\n", input, sym);
};

void setup_lang_ctx() {
  dll = dlopen("./libyalce_synth.so", RTLD_LAZY);
  // dll = dlopen(NULL, RTLD_LAZY);
  printf("loaded dll %p\n", dll);
}
static void repl_input(char *input, int bufsize, const char *prompt,
                       int *length) {
  char prev;
  char c;
  int position = *length;

  printf("%s", prompt);
  while (1) {
    prev = c;
    c = getchar();

    if (c == '\\') {
      input[position] = '\n';
      position++;
      continue;
    }

    if (c == EOF || c == '\n') {
      if (prev == '\\') {

        return repl_input(input + position, bufsize, "  ", length);
      }

      // input[position] = '\n';
      input[position] = '\0';
      return;
    }
    if (position == 2048) {
      // TODO: increase size of input buffer
    }

    input[position] = c;
    position++;
  }
  *length = position;
}
int main(int argc, char **argv) {
  char input[2048];
  int length = 0;
  setup_lang_ctx();
  for (;;) {
    repl_input(input, 2048, "\x1b[32m~ \x1b[0m", &length);
    // printf("input: %s", input);
    process_input(input, length);
  }

  return 0;
}
