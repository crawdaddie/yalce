#include "../src/lex.h"
#include "../src/parse.h"
#include "../src/serde.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Ast *parse(const char *input) {

  Ast *prog = Ast_new(AST_BODY);
  prog->data.AST_BODY.len = 0;
  prog->data.AST_BODY.stmts = malloc(sizeof(Ast *));
  Lexer lexer;
  init_lexer(input, &lexer);
  Parser parser;
  init_parser(&parser, &lexer);

  return parse_body(prog);
}


bool test_parse(char *input, char *expected_sexpr) {

  Ast *prog;
  prog = parse(input);


  char *sexpr = malloc(sizeof(char));
  sexpr = ast_to_sexpr(prog->data.AST_BODY.stmts[0], sexpr);
  if (strcmp(sexpr, expected_sexpr) != 0) {
    printf("❌ %s\n", input);
    printf("expected %s\n got %s\n", expected_sexpr, sexpr);
    return false;
  } else {
    printf("✅ %s\n", input);
    return true;
  }
}

int main() {
  bool status;
  status = test_parse(
    "1 + 2;; # single binop expression",
    "(+ 1 2)"
  );
  status &= test_parse(
    "(1 + 2);;",
    "(+ 1 2)"
  );
  status &= test_parse(
    "(1 + 2) * 8;; # complex grouped expression - parentheses have higher precedence",
    "(* (+ 1 2) 8)"
  );

  status &= test_parse(
    "(1 + 2) * 8 + 5", 
    "(+ (* (+ 1 2) 8) 5)"
  );
  return status ? 0 : 1;
}
