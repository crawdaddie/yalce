#include "lang.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  ctx *ctx = create_ctx();
  if (argc > 1) {
    char const *const filename = argv[1];
    FILE *file = fopen(filename, "r");
    char *line = NULL;
    size_t line_len = 0;
    int i = 0;
    int read;

    while ((read = getline(&line, &line_len, file)) != -1) {
      lexer(line, ctx);
      i++;
    }

    fclose(file);
  }

  char input[2048];
  for (;;) {
    fgets(input, 2048, stdin);
    printf("input: %s\n", input);
    lexer(input, ctx);
  }
}
