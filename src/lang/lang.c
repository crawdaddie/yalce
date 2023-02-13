#include "lang.h"

execution_ctx *create_execution_ctx() {
  return calloc(sizeof(execution_ctx), 1);
}

#define DEBUG 1
static token last_token = {START};
void process_token(token token) {
  if (DEBUG) {
    /* printf("last: "); */
    /* print_token(last_token); */
    /* printf(": current: "); */
    print_token(token);
    /* printf("\n\n"); */
  }

  last_token = token;
};

int parse_line(char *line, int line_num) {
  return lexer(line, line_num, &process_token);
}
