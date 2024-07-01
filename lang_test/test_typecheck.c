#include "../lang/parse.h"
#include "../lang/serde.h"
#include "../lang/type_inference.h"
#include <stdlib.h>

bool typecheck_prog(Ast *prog) {

  infer_ast(NULL, prog);
  return true;
}

void display(Ast *prog) {
  for (size_t i = 0; i < prog->data.AST_BODY.len; ++i) {

    Ast *stmt = prog->data.AST_BODY.stmts[i];
    if (stmt->tag == AST_BODY) {
      display(stmt);
    } else {
      printf("\e[1mstmt:\e[0m\n");
      print_ast(stmt);
      printf("\e[1mtype:\e[0m\n");
      print_type(stmt->md);
      printf("\n");
    }
  }
}

bool test_typecheck(char input[]) {
  Ast *prog;

  prog = parse_input(input);

  char *sexpr = malloc(sizeof(char) * 200);
  if (prog == NULL) {
    return false;
  }

  reset_type_var_counter();
  bool tcheck = typecheck_prog(prog);

  printf("-----\n\e[1minput:\e[0m\n%s\n", input);
  display(prog);
  printf("-----\n");

  free(sexpr);
  free(prog);
  yyrestart(NULL);
  ast_root = NULL;
  return true;
}

int main() {
  bool status = true;
  status &= test_typecheck("(1 + 2) * 8.");
  status &= test_typecheck("(1 + 2) * 8. < 2");
  status &= test_typecheck("(1 + 2) * 8. <= 2");
  status &= test_typecheck("let a = (1 + 2) * 8 in a + 212.");
  status &= test_typecheck("let a = (1 + 2) * 8 in a + 212");

  status &= test_typecheck(
      "let f = fn x -> (1 + 2 ) * 8 - x;"); // 'a -> 'a ('a is numeric
                                            // or synth / node type)

  status &=
      test_typecheck("let f = fn x y -> x + y;"); // 'a -> 'a ('a is numeric
                                                  // or synth / node type)
  status &= test_typecheck(
      "let f = fn x -> (1 + 2 ) * 8 - x > 2; f 1;"); // 'a -> bool ('a is
                                                     // numeric or synth /
                                                     // node
  // type)

  status &= test_typecheck("let f = fn a b -> a + b;;\n"
                           "f 1 2");

  status &= test_typecheck("let m = fn x ->\n"
                           "(match x with\n"
                           "| 1 -> 1\n"
                           "| 2 -> 0\n"
                           "| _ -> 3)\n"
                           ";");

  status &= test_typecheck("let m = fn x ->\n"
                           "(match x with\n"
                           "| 1 -> 1\n"
                           "| 2 -> 0\n"
                           "| _ -> m (x - 1))\n"
                           ";");

  // currying
  status &= test_typecheck("let m = fn x y z -> x + y + z;;\n"
                           "m 1 2");
  return !status;
}
