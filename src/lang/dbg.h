#ifndef _DBG_H
#define _DBG_H
#include "chunk.h"
#include <stdarg.h>
#include <stdio.h>

void disassemble_chunk(Chunk *chunk, const char *name);
int disassemble_instruction(Chunk *chunk, int offset);
void printf_color(char *fmt, int ansi, ...);
void print_value(Value val);
#endif
