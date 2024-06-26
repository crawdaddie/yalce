#include "../src/parse.h"
#include "../src/serde.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int yylineno;
bool test_parse(char input[], char *expected_sexpr) {

  Ast *prog;
  // printf("test input: %s\n", input);
  prog = parse_input(input);

  char *sexpr = malloc(sizeof(char) * 200);
  if (prog == NULL && expected_sexpr != NULL) {

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
  bool res;
  if (expected_sexpr == NULL && prog == NULL) {
    printf("✅ %s :: parse error\n", input, sexpr);
    res = true;
    free(sexpr);
    free(prog);
    yyrestart(NULL);
    // extern Ast *ast_root;
    ast_root = NULL;
    return res;
  }

  sexpr = ast_to_sexpr(prog->data.AST_BODY.stmts[0], sexpr);
  if (strcmp(sexpr, expected_sexpr) != 0) {
    printf("❌ %s\n", input);
    printf("expected %s\n"
           "     got %s\n",
           expected_sexpr, sexpr);

    res = false;
  } else {
    printf("✅ %s :: %s\n", input, sexpr);
    res = true;
  }
  free(sexpr);
  free(prog);
  yyrestart(NULL);
  // extern Ast *ast_root;
  ast_root = NULL;
  return res;
}

bool test_parse_body(char *input, char *expected_sexpr) {

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

  sexpr = ast_to_sexpr(prog, sexpr);
  bool res;
  if (strcmp(sexpr, expected_sexpr) != 0) {
    printf("❌ %s\n", input);
    printf("expected %s\n"
           "     got %s\n",
           expected_sexpr, sexpr);

    res = false;
  } else {
    printf("✅ %s :: %s\n", input, sexpr);
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

  status = test_parse("1 + 2", "(+ 1 2)"); // single binop expression"
  //
  status &= test_parse("-1", "-1");
  status &= test_parse("1.", "1.000000");
  status &= test_parse("-4.", "-4.000000");
  status &= test_parse("1f", "1.000000");
  status &= test_parse("-4f", "-4.000000");
  status &= test_parse("(1 + 2)", "(+ 1 2)");
  status &= test_parse("x + y", "(+ x y)");
  status &= test_parse("x - y", "(- x y)");
  status &= test_parse("1 + -2.", "(+ 1 -2.000000)");
  status &= test_parse("()", "()");

  // # multiple binop expression",
  status &= test_parse("1 + 2 - 3 * 4 + 5", "(+ (- (+ 1 2) (* 3 4)) 5)");

  // complex grouped expression -
  // parentheses have higher precedence,
  status &= test_parse("(1 + 2) * 8", "(* (+ 1 2) 8)");

  status &= test_parse("(1 + 2) * 8 + 5", "(+ (* (+ 1 2) 8) 5)");

  status &= test_parse("(1 + 2) * (8 + 5)", "(* (+ 1 2) (+ 8 5))");

  status &= test_parse("2 % 7", "(% 2 7)");

  status &= test_parse("f 1 2 3 4", "((((f 1) 2) 3) 4)");

  status &= test_parse("(f 1 2)", "((f 1) 2)");
  status &= test_parse("f ()", "(f ())");

  status &= test_parse("(f 1 2) + 1", "(+ ((f 1) 2) 1)");

  status &= test_parse("1 + (f 1 2)", "(+ 1 ((f 1) 2))");

  status &= test_parse("f 1 2 (3 + 1) 4", "((((f 1) 2) (+ 3 1)) 4)");

  status &= test_parse("f (8 + 5) 2", "((f (+ 8 5)) 2)");

  // status &= test_parse("3 |> f 1 2;", "(|> ((f 1) 2) 3)");

  status &= test_parse("3 |> f 1 2", "(((f 1) 2) 3)");

  status &= test_parse("3 |> f", "(f 3)");

  status &= test_parse("3 + 1 |> f", "(f (+ 3 1))");

  status &= test_parse("(g 3 4) |> f 1 2", "(((f 1) 2) ((g 3) 4))");

  status &= test_parse("(g 3 4 + 1) |> f 1 2", "(((f 1) 2) ((g 3) (+ 4 1)))");

  status &= test_parse("x + y;\nx + z", "\n(+ x y)\n(+ x z)");

  // lambda declaration
  status &= test_parse("fn x y -> x + y;", "(x y -> (+ x y))\n");

  status &= test_parse("fn () -> x + y;", "(() -> (+ x y))\n");
  status &= test_parse("fn x y z -> x + y + z;", "(x y z -> (+ (+ x y) z))\n");

  status &= test_parse("fn x y z -> \n"
                       "  x + y + z;\n"
                       "  x + y\n"
                       ";",
                       "(x y z -> \n"
                       "(+ (+ x y) z)\n"
                       "(+ x y))\n");

  status &= test_parse_body("\n"
                            "let sum3 = fn x y z ->\n"
                            "  1 + 1;\n"
                            "  x + y + z\n"
                            ";;\n"
                            "1 + 1",
                            "\n\n(let sum3 (sum3 x y z -> \n"
                            "(+ 1 1)\n"
                            "(+ (+ x y) z))\n"
                            ")\n"
                            "(+ 1 1)");
  // let declaration
  status &= test_parse("let x = 1 + y", "(let x (+ 1 y))");

  // status &=
  //     test_parse("`hello {x} {y}`", "(((_format \"hello {x} {y}\") x) y)");

  status &= test_parse("[1, 2, 3, 4]", "[1, 2, 3, 4]");
  status &= test_parse("(1, )", NULL);
  status &= test_parse("(1, 2)", "(1, 2)");
  status &= test_parse("(1, 2, 3)", "(1, 2, 3)");
  status &= test_parse("match x with\n"
                       "| 1 -> 1\n"
                       "| 2 -> 0\n"
                       "| _ -> 3",
                       "(match x with\n"
                       "\t1 -> 1\n"
                       "\t2 -> 0\n"
                       "\t_ -> 3\n"
                       ")");

  status &= test_parse_body("let m = fn x ->\n"
                            "(match x with\n"
                            "| 1 -> 1\n"
                            "| 2 -> 0\n"
                            "| _ -> 3)\n"
                            ";;",
                            "\n\n(let m (m x ->\n"
                            "(match x with\n"
                            "\t1 -> 1\n"
                            "\t2 -> 0\n"
                            "\t_ -> 3\n"
                            "))\n)");

  // more complex match expr
  status &= test_parse("match x + y with\n"
                       "| 1 -> 1\n"
                       "| 2 -> let a = 1; a + 1\n"
                       "| _ -> 3",
                       "(match (+ x y) with\n"
                       "\t1 -> 1\n"
                       "\t2 -> "
                       "\n(let a 1)\n(+ a 1)"
                       "\n\t_ -> 3\n"
                       ")");
  return status ? 0 : 1;
}
