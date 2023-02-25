#ifndef _SYM_H
#define _SYM_H
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKEN_LENGTH 64
typedef struct Entry {
  ObjString *key;
  Value value;
} Entry;

typedef struct Table {
  int count;
  int capacity;
  Entry *data;
} Table;

void init_table(Table *sym);
void free_table(Table *sym);

int table_set(Table *sym, ObjString *key, Value value);
bool table_get(Table *sym, ObjString *key, Value *value);

uint32_t hash_string(const char *string, int length);

ObjString *table_find_string(Table *table, char *chars, int length,
                             uint32_t hash);
#endif
