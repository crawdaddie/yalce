#include "../lang/parse.h"
#include "../lang/serde.h"
#include "../lang/types/inference.h"
#include "../lang/types/util.h"
#include <stdlib.h>

bool typecheck_prog(Ast *prog) {
  infer_ast(NULL, prog);
  return true;
}

bool test(Ast *prog, Type **exp_types) {
  int pass = true;
  for (size_t i = 0; i < prog->data.AST_BODY.len; ++i) {

    Ast *stmt = prog->data.AST_BODY.stmts[i];

    if (stmt->tag == AST_BODY) {
      pass &= test(stmt, exp_types + i);
    } else {
      bool t = types_equal(stmt->md, exp_types[i]);
      printf("\e[1mstmt:\e[0m\n");
      print_ast(stmt);

      printf("%s\e[1mtype: \e[0m", t ? "✅" : "❌");
      print_type(stmt->md);
      if (!t) {
        printf(" expected ");
        print_type(exp_types[i]);
      }
      printf("\n");
      pass &= t;
    }
  }
  return pass;
}

bool test_typecheck(char input[], Type **exp_types) {
  Ast *prog;

  prog = parse_input(input);

  char *sexpr = malloc(sizeof(char) * 200);
  if (prog == NULL) {
    return false;
  }

  reset_type_var_counter();

  printf("\e[1minput:\e[0m\n%s\n", input);
  Type *tcheck_val = infer_ast(NULL, prog);
  if (!tcheck_val) {
    printf("❌ Type inference failed\n");
  } else {
    bool pass = test(prog, exp_types);
  }

  printf("-----\n");

  free(sexpr);
  free(prog);
  yyrestart(NULL);
  ast_root = NULL;
  return true;
}

#define T(...)                                                                 \
  (Type *[]) { __VA_ARGS__ }

int main() {
  bool status = true;
  status &= test_typecheck("(1 + 2) * 8", T(&t_int));
  status &= test_typecheck("(1 + 2) * 8. < 2", T(&t_bool));
  status &= test_typecheck("(1 + 2) * 8. <= 2", T(&t_bool));
  status &= test_typecheck("let a = (1 + 2) * 8 in a + 212.", T(&t_bool));
  status &= test_typecheck("let a = (1 + 2) * 8 in a + 212", T(&t_int));
  //
  status &= test_typecheck("let f = fn x -> (1 + 2 ) * 8 - x;",
                           T(&t_bool)); // 'a -> 'a ('a is numeric
  //                                           // or synth / node type)
  //
  // status &=
  //     test_typecheck("let f = fn x y -> x + y;"); // 'a -> 'a ('a is numeric
  //                                                 // or synth / node type)
  // status &= test_typecheck(
  //     "let f = fn x -> (1 + 2 ) * 8 - x > 2; f 1;"); // 'a -> bool ('a is
  //                                                    // numeric or synth /
  //                                                    // node
  // // type)
  //
  // status &= test_typecheck("let f = fn a b -> a + b;;\n"
  //                          "f 1 2");
  //
  // status &= test_typecheck("let m = fn x ->\n"
  //                          "(match x with\n"
  //                          "| 1 -> 1\n"
  //                          "| 2 -> 0\n"
  //                          "| _ -> 3)\n"
  //                          ";");
  //
  // status &= test_typecheck("let m = fn x ->\n"
  //                          "(match x with\n"
  //                          "| 1 -> 1\n"
  //                          "| 2 -> 0\n"
  //                          "| _ -> m (x - 1))\n"
  //                          ";");
  //
  // // currying
  // status &= test_typecheck("let m = fn x y z -> x + y + z;;\n"
  //                          "m 1 2");
  return !status;
}
#undef T
