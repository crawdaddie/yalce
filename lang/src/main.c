#include "eval.h"
#include "ht.h"
#include "input.h"
#include "parse.h"
#include "serde.h"
#include "y.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_AST

int eval_script(const char *filename) {
  char *fcontent = read_script(filename);
  if (!fcontent) {
    return 1;
  }

  Ast *prog = parse_input(fcontent);
#ifdef DEBUG_AST
  print_ast(prog);
#endif

  ht stack[STACK_MAX];

  for (int i = 0; i < STACK_MAX; i++) {
    ht_init(stack + i);
  }

  Value *res = eval(prog, NULL, stack, 0);
  // printf("> ");
  // print_value(res);

  free(fcontent);
  return 0; // Return success
}
Ast *peek_body(Ast *body) {
  size_t len = body->data.AST_BODY.len;
  Ast *last = body->data.AST_BODY.stmts[len - 1];
  return last;
}

int main(int argc, char **argv) {

  bool repl = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      repl = true;
    } else {
      return eval_script(argv[i]);
    }
  }

  char *prompt = "\033[1;31mλ \033[1;0m"
                 "\033[1;36m";

  if (repl) {
    printf("\033[1;31m"
           "YLC LANG REPL       \n"
           "--------------------\n"
           "version 0.0.0       \n"
           "\033[1;0m");

    char *input = malloc(sizeof(char) * INPUT_BUFSIZE);

    ht stack[STACK_MAX];
    for (int i = 0; i < STACK_MAX; i++) {
      ht_init(stack + i);
    }
    while (true) {
      repl_input(input, INPUT_BUFSIZE, prompt);
      Ast *prog = parse_input(input);

      Value *val = malloc(sizeof(Value));
      Ast *top = peek_body(prog);
#ifdef DEBUG_AST
      print_ast(top);
#endif

      Value *res = eval(top, val, stack, 0);
      printf("> ");
      print_value(res);
      printf("\n");
    }
    free(input);
  }
  return 0;
}
