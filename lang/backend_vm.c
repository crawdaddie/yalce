#include "backend_vm.h"
#include "builtins.h"
#include "eval.h"
#include "ht.h"
#include "input.h"
#include "parse.h"
#include "serde.h"
#include "synth_functions.h"
#include "types.h"
#include "y.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define DEBUG_AST

static int eval_script(const char *filename, LangCtx *ctx) {
  char *fcontent = read_script(filename);
  if (!fcontent) {
    return 1;
  }

  Ast *prog = parse_input(fcontent);
#ifdef DEBUG_AST
  print_ast(prog);
#endif

  Value res = eval(prog, ctx);

  printf("> ");
  print_value(&res);
  printf("\n");

  free(fcontent);
  return 0; // Return success
}

static Ast *peek_body(Ast *body) {
  size_t len = body->data.AST_BODY.len;
  Ast *last = body->data.AST_BODY.stmts[len - 1];
  return last;
}

int interpreter_vm(int argc, char **argv) {
  bool repl = false;
  ht stack[STACK_MAX];

  for (int i = 0; i < STACK_MAX; i++) {
    ht_init(&stack[i]);
  }

  add_type_lookups(stack);
  add_native_functions(stack);
  add_synth_functions(stack);

  LangCtx ctx = {
      .stack = stack,
      .stack_ptr = 0,
      .val_bind = NULL,
  };

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      repl = true;
    } else {
      eval_script(argv[i], &ctx);
    }
  }

  if (repl) {
    char *prompt = "\033[1;31mλ \033[1;0m"
                   "\033[1;36m";
    printf("\033[1;31m"
           "YLC LANG REPL       \n"
           "------------------\n"
           "version 0.0.0       \n"
           "\033[1;0m");

    while (true) {
      char *input = repl_input(prompt);
      Ast *prog = parse_input(input);

      Ast *top = peek_body(prog);
#ifdef DEBUG_AST
      print_ast(top);
#endif

      Value res = eval(top, &ctx);
      printf("> ");
      print_value(&res);
      printf("\n");
    }
  }
  return 0;
}
