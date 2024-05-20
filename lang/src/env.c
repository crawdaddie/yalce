#include "env.h"
#include "memory.h"
#include "value.h"
#include <stdlib.h>
#include <string.h>

#define TABLE_MAX_LOAD 0.75

void init_table(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void free_table(Table *table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  init_table(table);
}
#define IS_NIL(v) v.type == VALUE_VOID

static Entry *find_entry(Entry *entries, int capacity, ObjString *key) {

  uint32_t index = key->hash % capacity;

  Entry *tombstone = NULL;

  for (;;) {
    Entry *entry = &entries[index];

    if (entry->key == NULL) {

      if (IS_NIL(entry->value)) {
        return tombstone != NULL ? tombstone : entry;
      } else {
        // We found a tombstone.
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (entry->key->hash == key->hash) {
      // We found the key.
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

bool table_get(Table *table, ObjString *key, Value *value) {

  if (table->count == 0)
    return false;

  Entry *entry = find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL)
    return false;

  value->type = entry->value.type;
  value->value = entry->value.value;
  return true;
}

static void adjust_capacity(Table *table, int capacity) {
  Entry *entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = (Value){VALUE_VOID};
  }

  table->count = 0;

  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    if (entry->key == NULL)
      continue;

    Entry *dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;

    table->count++;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);

  table->entries = entries;
  table->capacity = capacity;
}

bool table_set(Table *table, ObjString *key, Value value) {

  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, capacity);
  }

  Entry *entry = find_entry(table->entries, table->capacity, key);
  bool isNewKey = entry->key == NULL;
  /* Hash Tables table-set < Hash Tables set-increment-count
    if (isNewKey) table->count++;
  */

  if (isNewKey && IS_NIL(entry->value))
    table->count++;

  entry->key = key;
  entry->value.type = value.type;
  entry->value.value = value.value;
  return isNewKey;
}

bool table_delete(Table *table, ObjString *key) {
  if (table->count == 0)
    return false;

  // Find the entry.
  Entry *entry = find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL)
    return false;

  // Place a tombstone in the entry.
  entry->key = NULL;
  entry->value = (Value){VALUE_BOOL, .value = {.vbool = true}};
  return true;
}

ObjString *table_find_string(Table *table, const char *chars, int length,
                             uint32_t hash) {
  if (table->count == 0)
    return NULL;

  /* Hash Tables table-find-string < Optimization find-string-index
    uint32_t index = hash % table->capacity;
  */

  uint32_t index = hash % (table->capacity);

  for (;;) {
    Entry *entry = &table->entries[index];
    if (entry->key == NULL) {
      // Stop if we find an empty non-tombstone entry.
      if (IS_NIL(entry->value))
        return NULL;
    } else if (entry->key->length == length && entry->key->hash == hash &&
               memcmp(entry->key->chars, chars, length) == 0) {
      // We found it.
      return entry->key;
    }

    /* Hash Tables table-find-string < Optimization find-string-next
     */
    // index = (index + 1) % table->capacity;

    index = (index + 1) % (table->capacity);
  }
}
