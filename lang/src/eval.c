#include "eval.h"
#include "serde.h"
#include <stdio.h>
void eval(Ast *top) {
  printf("evaluating top-level: ");
  print_ser_ast(top);
}
