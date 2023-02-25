#ifndef _UTIL_H
#define _UTIL_H
#include "dbg.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void yyerror(char *s);
int divi(int a, int b);
double divf(double a, double b);

Value nadd(Value a, Value b);
Value nsub(Value a, Value b);
Value ndiv(Value a, Value b);
Value nmul(Value a, Value b);
Value nmod(Value a, Value b);

Value nnegate(Value a);
void print_value(Value any);
void print_object(Object *any);

#endif
