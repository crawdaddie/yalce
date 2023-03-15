#include "sym.h"
#include "lang_memory.h"
#include "util.h"
#define TABLE_MAX_LOAD 0.75
#define INITIAL_TABLE_SIZE 64

// hash util functions
uint32_t hash_string(const char *string, int length) {
  unsigned int hash = 2166136261u;

  for (int i = 0; i < length; i++) {
    hash *= string[i];
    hash ^= 16777619;
  }

  return hash;
}

static Entry *find_entry(Entry *data, int capacity, ObjString *key) {
  uint32_t index = key->hash % capacity;
  for (;;) {
    Entry *entry = &data[index];
    if (entry->key == key || entry->key == NULL) {
      return entry;
    }
    index = (index + 1) % capacity;
  }
};
static void adjust_capacity(Table *table, int capacity) {
  Entry *entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->data[i];
    if (entry->key == NULL)
      continue;

    Entry *dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(Entry, table->data, table->capacity);
  table->data = entries;
  table->capacity = capacity;
}

void init_table(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->data = NULL;
}

void free_table(Table *sym) {
  FREE_ARRAY(Entry, sym->data, sym->capacity);
  init_table(sym);
}

int table_set(Table *table, ObjString *key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, capacity);
  }
  Entry *entry = find_entry(table->data, table->capacity, key);
  int is_new = entry->key == NULL;
  if (is_new)
    table->count++;
  entry->key = key;
  entry->value = value;
  return is_new;
}

bool table_get(Table *sym, ObjString *key, Value *val) {
  if (sym->count == 0) {
    return false;
  }
  Entry *entry = find_entry(sym->data, sym->capacity, key);
  if (entry->key == NULL) {
    return false;
  }
  *val = entry->value;
  return true;
}

ObjString *table_find_string(Table *table, char *chars, int length,
                             uint32_t hash) {
  if (table->count == 0) {
    return NULL;
  }

  uint32_t index = hash % table->capacity;
  for (;;) {
    Entry *entry = &table->data[index];
    if (entry->key == NULL) {
      // stop if we find empty non-tombstone
      if (IS_NIL(entry->value))
        return NULL;
    } else if (entry->key->length == length && entry->key->hash == hash &&
               memcmp(entry->key->chars, chars, length) == 0) {
      // found the entry
      return entry->key;
    }
    index = (index + 1) % table->capacity;
  }
}
