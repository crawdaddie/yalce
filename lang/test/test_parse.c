#include "../src/parse.h"
#include "../src/serde.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool test_parse(char *input, char *expected_sexpr) {

  Ast *prog;
  prog = parse_input(input);

  char *sexpr = malloc(sizeof(char) * 200);
  if (prog == NULL) {

    printf("❌ %s\n", input);
    printf("expected %s\n"
           "     got syntax error\n",
           expected_sexpr);

    free(sexpr);
    free(prog);
    yyrestart(NULL);
    // extern Ast *ast_root;
    ast_root = NULL;
    return false;
  }

  sexpr = ast_to_sexpr(prog->data.AST_BODY.stmts[0], sexpr);
  bool res;
  if (strcmp(sexpr, expected_sexpr) != 0) {
    printf("❌ %s\n", input);
    printf("expected %s\n"
           "     got %s\n",
           expected_sexpr, sexpr);

    res = false;
  } else {
    printf("✅ %s -> %s\n", input, sexpr);
    res = true;
  }
  free(sexpr);
  free(prog);
  yyrestart(NULL);
  // extern Ast *ast_root;
  ast_root = NULL;
  return res;
}

int main() {

  bool status;

  status = test_parse("1 + 2;", "(+ 1 2)"); // single binop expression"
  //
  status &= test_parse("-1;", "-1");
  status &= test_parse("1.;", "1.000000");
  status &= test_parse("-4.;", "-4.000000");
  status &= test_parse("1f;", "1.000000");
  status &= test_parse("-4f;", "-4.000000");

  status &= test_parse("(1 + 2);", "(+ 1 2)");

  // # multiple binop expression",
  status &= test_parse("1 + 2 - 3 * 4 + 5;", "(+ (- (+ 1 2) (* 3 4)) 5)");

  // complex grouped expression -
  // parentheses have higher precedence,
  status &= test_parse("(1 + 2) * 8;", "(* (+ 1 2) 8)");

  status &= test_parse("(1 + 2) * 8 + 5;", "(+ (* (+ 1 2) 8) 5)");

  status &= test_parse("(1 + 2) * (8 + 5);", "(* (+ 1 2) (+ 8 5))");

  status &= test_parse("f 1 2 3 4;", "((((f 1) 2) 3) 4)");

  status &= test_parse("(f 1 2);", "((f 1) 2)");

  status &= test_parse("(f 1 2) + 1;", "(+ ((f 1) 2) 1)");

  status &= test_parse("1 + (f 1 2);", "(+ 1 ((f 1) 2))");

  status &= test_parse("f 1 2 (3 + 1) 4;", "((((f 1) 2) (+ 3 1)) 4)");

  status &= test_parse("f (8 + 5) 2;", "((f (+ 8 5)) 2)");

  // status &= test_parse("3 |> f 1 2;", "(|> ((f 1) 2) 3)");

  status &= test_parse("3 |> f 1 2;", "(((f 1) 2) 3)");
  status &= test_parse("3 |> f;", "(f 3)");
  status &= test_parse("3 + 1 |> f;", "(f (+ 3 1))");
  status &= test_parse("g 3 4 |> f 1 2;", "(((f 1) 2) ((g 3) 4))");
  status &= test_parse("fn x y -> x + y;", "(fn (x y) -> (+ x y))");
  return status ? 0 : 1;
}
