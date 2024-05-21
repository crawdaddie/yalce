#include "env.h"
#include "memory.h"
#include "value.h"
#include <stdlib.h>
#include <string.h>

void init_table(Table *table) {}

void free_table(Table *table) {}

bool table_get(Table *table, ObjString *key, Value *value) { return false; }

bool table_set(Table *table, ObjString *key, Value value) { return false; }

bool table_delete(Table *table, ObjString *key) { return false; }
