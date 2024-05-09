#include "eval.h"
#include "serde.h"
#include <stdio.h>
void eval(Ast *top) {
  printf("\nevaluating top-level: ");
  print_ser_ast(top);
  printf("\n");
}
