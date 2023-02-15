#include "util.h"
void yyerror(char *s) { fprintf(stderr, "%s\n", s); }
int divi(int a, int b) {
  if (b == 0) {
    yyerror("can not be divided by zero");
  }
  return a / b;
};

double divf(double a, double b) {
  if (b == 0) {
    yyerror("can not be divided by zero");
  }
  return a / b;
}
char *strconcat(char *a, char *b) {
  int a_len = strlen(a);
  int b_len = strlen(b);
  char *r = realloc(a, a_len + b_len);
  strcpy(r + a_len, b);
  free(b);
  return r;
}
