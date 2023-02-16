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
/* Value strconcat(Value a, Value b) { */
/*   int a_len = strlen(a); */
/*   int b_len = strlen(b); */
/*   char *r = realloc(a, a_len + b_len); */
/*   strcpy(r + a_len, b); */
/*   free(b); */
/*   return r; */
/* } */

Value nadd(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    yyerror("invalid operands for +");
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) + AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b));
}

Value nsub(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    yyerror("invalid operands for -");
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) - AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) - AS_NUMBER(b));
}

Value nmul(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    yyerror("invalid operands for *");
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) * AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) * AS_NUMBER(b));
}

Value ndiv(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    yyerror("invalid operands for /");
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) / AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) / AS_NUMBER(b));
}

void print_value(Value val) {
  switch (val.type) {
  case VAL_NUMBER:
    printf_color("[%lf]", 96, val.as.number);
    break;
  case VAL_INTEGER:
    printf_color("[%d]", 96, val.as.integer);
    break;

  case VAL_OBJ:
    printf_color("[%x] - %s", 96, val.as.object, val.as.object->value);
    break;
  default:
    break;
  }
  printf("\n");
}
