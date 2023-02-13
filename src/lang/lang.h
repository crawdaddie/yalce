#ifndef _LANG_H
#define _LANG_H
#include "dbg.h"
#include "lexer.h"
#include <stdlib.h>

typedef struct stack_frame {
  void *scope;
} stack_frame;

typedef struct ex_stack {

} ex_stack;

typedef struct execution_ctx {
  ex_stack stack;
} execution_ctx;

execution_ctx *create_execution_ctx();

void process_token(token token);
int parse_line(char *line, int line_num);

#endif
