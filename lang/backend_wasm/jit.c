#include "parse.h"
#include "serde.h"
#include <stdlib.h>

Ast *jit(char *input) {
  Ast *prog = parse_input(input);
  print_ast(prog);

  return prog;
}
