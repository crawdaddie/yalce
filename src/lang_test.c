#include "lang/parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

  char input[2048];
  for (;;) {
    fgets(input, 2048, stdin);
    printf("input: %s\n", input);
    parse_string(input);
  }
}
