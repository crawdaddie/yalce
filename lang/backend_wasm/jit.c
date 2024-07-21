#include "../parse.h"

// static Ast *top_level_ast(Ast *body) {
//   size_t len = body->data.AST_BODY.len;
//   Ast *last = body->data.AST_BODY.stmts[len - 1];
//   return last;
// }

int jit(char *input) {
  Ast *prog = parse_input(input);
  // Ast *top = top_level_ast(prog);
  return prog == NULL;
}
