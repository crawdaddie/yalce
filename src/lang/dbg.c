#include "dbg.h"
void printf_color(char *fmt, int ansi, ...) {
  va_list args;
  va_start(args, ansi);
  printf("\033[%dm", ansi);
  vprintf(fmt, args);
  printf("\033[%dm", 37);
  printf("");
  va_end(args);
}

void print_token(token tok) {
  switch (tok.type) {
  case START:
    printf("[>]");
    break;
  case LP:
    printf("[(]");
    break;
  case RP:
    printf("[)]");
    break;
  case COMMA:
    printf("[,]");
    break;
  case DOT:
    printf("[.]");
    break;
  case MINUS:
    printf("[-]");
    break;
  case PLUS:
    printf("[+]");
    break;
  case SLASH:
    printf("[/]");
    break;
  case STAR:
    printf("[*]");
    break;
  case NL:
    printf("[\\n]");
    break;
  case PIPE:
    printf("[->]");
    break;
  case IDENTIFIER:
    printf_color("[%s]", 91, tok.literal.vident);
    break;
  case STRING:
    printf_color("[%s]", 32, tok.literal.vstr);
    break;
  case NUMBER:
    printf_color("[%lf]", 96, tok.literal.vfloat);
    break;
  case INTEGER:
    printf_color("[%d]", 96, tok.literal.vint);
    break;
  case FN:
    printf_color("[%s]", 91, tok.literal.vident);
    break;
  case PRINT:
    printf_color("[%s]", 91, tok.literal.vident);
    break;

  case ASSIGNMENT:
    printf("[=]");
    break;

  case EQUALITY:
    printf("[==]");
    break;
  default:
    printf("[]");
  }
}
