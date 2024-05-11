#include "../src/lex.h"
#include "../src/parse.h"
#include "../src/serde.h"
#include <stdio.h>
#include <stdlib.h>

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

#define TEST(expr, input, action_true, action_false)                           \
  if (!(expr)) {                                                               \
    printf("❌ %s\n", input);                                                  \
    action_true;                                                               \
  } else {                                                                     \
    printf("✅ %s\n", input);                                                  \
    action_false;                                                              \
  }

// Define a function-like macro for printing the AST
#define PRINT_AST(expr) print_ast(expr)

int main() {

  char *input = "1 + 2;;";
  Ast *prog;
  Ast *expr;

  prog = parse(input);
  expr = prog->data.AST_BODY.stmts[0];
  TEST((expr->data.AST_BINOP.op == TOKEN_PLUS &&
        expr->data.AST_BINOP.left->tag == AST_INT &&
        expr->data.AST_BINOP.right->tag == AST_INT),
       input, PRINT_AST(expr),
       /* No action for pass case */);

  free(prog->data.AST_BODY.stmts);
  free(prog);

  input = "(1 + 2) * 8;;";
  prog = parse(input);
  expr = prog->data.AST_BODY.stmts[0];

  TEST((expr->data.AST_BINOP.op == TOKEN_STAR), input, PRINT_AST(expr),
       /* No action for pass case */);
  free(prog->data.AST_BODY.stmts);
  free(prog);

  input = "(1 + 2) * 8 + 5";
  prog = parse(input);
  expr = prog->data.AST_BODY.stmts[0];

  TEST((expr->data.AST_BINOP.op == TOKEN_PLUS), input, PRINT_AST(expr),
       /* No action for pass case */);

  free(prog->data.AST_BODY.stmts);
  free(prog);
}
