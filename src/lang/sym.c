#include "sym.h"
static symbol_table sym = {};
#define TABLE_MAX_LOAD 0.75
#define INITIAL_TABLE_SIZE 64

// hash util functions
static unsigned int hash_string(const char *string, int length) {
  unsigned int hash = 2166136261u;

  for (int i = 0; i < length; i++) {
    hash *= string[i];
    hash ^= 16777619;
  }

  return hash;
}

void init_table() {
  sym.count = 0;
  sym.capacity = 64;
  sym.data = calloc(sizeof(Entry), INITIAL_TABLE_SIZE);
}
static int grow_table() { return 1; }

void free_table() {
  free(sym.data);
  init_table();
}
static Entry *find_entry(Entry *data, int capacity, uint32_t hash) {
  uint32_t index = hash % capacity;
  for (;;) {
    Entry *entry = &data[index];
    if (entry->hash == hash || entry->key == NULL) {
      return entry;
    }
    index = (index + 1) % capacity;
  }
};
static void adjust_capacity(int capacity) {
  Entry *data = calloc(sizeof(Entry), capacity);
  for (int i = 0; i < capacity; i++) {
    data[i].key = NULL;
    data[i].value = NIL_VAL;
  }
  for (int i = 0; i < capacity; i++) {
    Entry *entry = &sym.data[i];
    if (entry->key == NULL) {
      continue;
    }

    Entry *dest = find_entry(data, capacity, entry->hash);
    dest->key = entry->key;
    dest->hash = entry->hash;
    dest->value = entry->value;
  }
  free(sym.data);
  sym.data = data;
  sym.capacity = capacity;
}

int table_set(char *key, Value value) {
  if (sym.count + 1 > sym.capacity * TABLE_MAX_LOAD) {
    int capacity = 2 * sym.capacity;
    printf("adjusting capacity %d -> %d\n", sym.capacity, capacity);
    adjust_capacity(capacity);
  }
  uint32_t hash = hash_string(key, strlen(key));
  Entry *entry = find_entry(sym.data, sym.capacity, hash);
  int is_new = entry->key == NULL;
  if (is_new)
    sym.count++;
  entry->key = key;
  entry->hash = hash;
  entry->value = value;
  return is_new;
}
Value table_get(char *key) {
  uint32_t hash = hash_string(key, strlen(key));
  Entry *entry = find_entry(sym.data, sym.capacity, hash);
  return entry->value;
}
