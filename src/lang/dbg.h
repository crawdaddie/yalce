#ifndef _DBG_H
#define _DBG_H
#include "lexer.h"
#include <stdarg.h>
#include <stdio.h>
void printf_color(char *fmt, int ansi, ...);
void print_token(token token);
#endif
