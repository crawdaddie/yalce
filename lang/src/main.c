#include "eval.h"
#include "parse.h"
#include "y.tab.h"

#include "serde.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char *read_script(const char *filename) {

  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error opening file: %s\n", filename);
    return NULL;
  }

  // Determine the size of the file
  fseek(fp, 0, SEEK_END); // Move the file pointer to the end of the file
  long fsize = ftell(fp); // Get the position, which is the file size
  rewind(fp);

  char *fcontent = (char *)malloc(fsize + 1);

  size_t bytes_read = fread(fcontent, 1, fsize, fp);

  if (bytes_read != fsize) {
    fprintf(stderr, "Error reading file: %s\n", filename);
    fclose(fp);
    free(fcontent); // Don't forget to free the allocated memory
    return NULL;
  }

  // Null-terminate the string
  fcontent[fsize] = '\0';
  fclose(fp);
  return fcontent;
}

int eval_script(const char *filename) {
  char *fcontent = read_script(filename);
  if (!fcontent) {
    return 1;
  }

  Ast *prog = parse_input(fcontent);
  print_ast(prog);

  EnvStack stack;
  for (int i = 0; i < STACK_MAX; i++) {
    (stack.envs + i)->count = 0;
    (stack.envs + i)->capacity = 8;
    (stack.envs + i)->entries = malloc(sizeof(Entry) * 8);
  }
  print_value(eval(prog, NULL, &stack));

  free(fcontent);
  return 0; // Return success
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

    EnvStack stack;
    for (int i = 0; i < STACK_MAX; i++) {
      (stack.envs + i)->count = 0;
      (stack.envs + i)->capacity = 8;
      (stack.envs + i)->entries = malloc(sizeof(Entry) * 8);
      for (int j = 0; j < 8; j++) {

        (stack.envs + i)->entries[j].key = NULL;
        (stack.envs + i)->entries[j].value.type = VALUE_VOID;
      }
    }
    while (true) {
      repl_input(input, INPUT_BUFSIZE, prompt);
      Ast *prog = parse_input(input);

      printf("parsed program:");

      print_ast(prog);

      print_value(eval(prog, NULL, &stack));
      printf("\n");
    }
    free(input);
  }
  return 0;
}
