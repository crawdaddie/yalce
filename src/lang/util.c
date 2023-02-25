#include "util.h"
#include "list.h"
#include "string.h"
#include <math.h>
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
    /* yyerror("invalid operands for +"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) + AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b));
}

Value nsub(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    /* yyerror("invalid operands for -"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) - AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) - AS_NUMBER(b));
}

Value nmul(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    /* yyerror("invalid operands for *"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) * AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) * AS_NUMBER(b));
}

Value ndiv(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    /* yyerror("invalid operands for /"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) / AS_INTEGER(b));
  }
  return NUMBER_VAL(AS_NUMBER(a) / AS_NUMBER(b));
}

Value nmod(Value a, Value b) {
  if (!(IS_NUMERIC(a) && IS_NUMERIC(b))) {
    /* yyerror("invalid operands for %"); */
    return NIL_VAL;
  }
  if (IS_INTEGER(a) && IS_INTEGER(b)) {
    return INTEGER_VAL(AS_INTEGER(a) % AS_INTEGER(b));
  }
  return NUMBER_VAL(fmod(AS_NUMBER(a), AS_NUMBER(b)));
}
Value nnegate(Value a) {
  if (IS_INTEGER(a)) {
    return INTEGER_VAL(-AS_INTEGER(a));
  }
  return NUMBER_VAL(-AS_NUMBER(a));
}
void print_object(Object *object) {
  switch (object->type) {
  case OBJ_STRING: {
    ObjString *str = (ObjString *)object;
    printf_color("%s", 96, str->chars);
    break;
  }
  case OBJ_LIST: {
    ObjList *l = (ObjList *)object;
    for (int i = 0; i < l->length; i++) {
      print_value(l->values[i]);
      printf(", ");
    }
    break;
  }
  default:
    break;
  }
}
void print_value(Value val) {
  switch (val.type) {

  case VAL_BOOL:
    printf(AS_BOOL(val) ? "true" : "false");
    break;
  case VAL_NIL:
    printf("nil");
    break;
  case VAL_NUMBER:
    printf_color("%lf", 96, val.as.number);
    break;
  case VAL_INTEGER:
    printf_color("%d", 96, val.as.integer);
    break;
  case VAL_OBJ:
    print_object(AS_OBJ(val));
    break;
  default:
    break;
  }
}
