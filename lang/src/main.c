#include "eval.h"
#include "lex.h"
#include "parse.h"
#include "serde.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define INPUT_BUFSIZE 2048
void repl_input(char *input, int bufsize, const char *prompt) {
  char prev;
  char c;
  int position = 0;
  if (prompt == NULL) {
    prompt = "\033[1;31mλ \033[1;0m"
             "\033[1;36m";
  }

  printf("%s", prompt);
  while (1) {
    prev = c;
    c = getchar();

    if (c == 'n' && prev == '\\') {
      input[position - 1] = '\n';
      continue;
    }

    if (c == EOF || c == '\n') {
      if (prev == '\\') {
        return repl_input(input + position, bufsize, "  ");
      }
      input[position] = '\n';
      input[++position] = '\0';
      return;
    }
    if (position == bufsize) {
      printf("input exceeds bufsize\n");
      // TODO: increase size of input buffer
    }

    input[position] = c;
    position++;
  }
  printf("\033[1;0m");
}

Ast *ast_body_peek(Ast *body) {
  if (body->data.AST_BODY.len == 0 || body->data.AST_BODY.stmts == NULL) {
    return NULL;
  }
  return body->data.AST_BODY.stmts[body->data.AST_BODY.len - 1];
}

int main(int argc, char **argv) {

  bool repl = true;
  if (repl) {
    printf("\033[1;31m"
           "YLC LANG REPL       \n"
           "--------------------\n"
           "version 0.0.0       \n"
           "\033[1;0m");

    Ast *prog = Ast_new(AST_BODY);
    prog->data.AST_BODY.len = 0;
    prog->data.AST_BODY.stmts = malloc(sizeof(Ast *));
    char *input = (char *)malloc(sizeof(char) * INPUT_BUFSIZE);

    while (true) {
      Lexer lexer;
      repl_input(input, INPUT_BUFSIZE, NULL);
      init_lexer(input, &lexer);

      Parser parser;
      init_parser(&parser, &lexer);

      prog = parse_body(prog);
      print_ser_ast(prog);

      Ast *top = ast_body_peek(prog);
      eval(top);
    }
    free(input);
  }
  return 0;
}
