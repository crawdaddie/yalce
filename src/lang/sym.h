#ifndef _SYM_H
#define _SYM_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKEN_LENGTH 64
typedef struct Entry {
  char *key;
  uint32_t hash;
  double value;
} Entry;

typedef struct symbol_table {
  int count;
  int capacity;
  Entry *data;
} symbol_table;

void init_table();
void free_table();

int table_set(char *name, double value);
double table_get(char *name);
#endif
