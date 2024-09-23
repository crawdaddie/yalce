// Simple hash table implemented in C.

#include "ht.h"
#include "common.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 256 // must not be zero

ht *ht_create(void) {
  // Allocate space for hash table struct.
  ht *table = malloc(sizeof(ht));
  if (table == NULL) {
    return NULL;
  }
  table->length = 0;
  table->capacity = INITIAL_CAPACITY;

  // Allocate (zero'd) space for entry buckets.
  table->entries = calloc(table->capacity, sizeof(ht_entry));
  if (table->entries == NULL) {
    free(table); // error, free table before we return!
    return NULL;
  }
  return table;
}

void ht_init(ht *table) {
  // Allocate space for hash table struct.
  // ht *table = malloc(sizeof(ht));
  if (table == NULL) {
    return;
  }
  table->length = 0;
  table->capacity = INITIAL_CAPACITY;

  // Allocate (zero'd) space for entry buckets.
  table->entries = calloc(table->capacity, sizeof(ht_entry));
  if (table->entries == NULL) {
    free(table); // error, free table before we return!
  }
}

void ht_reinit(ht *table) {
  // ht *table = malloc(sizeof(ht));
  if (table == NULL) {
    return;
  }

  table->length = 0;
  table->capacity = INITIAL_CAPACITY;

  // (zero) space for entry buckets.
  for (int i = 0; i < table->capacity; i++) {
    // free((table->entries + i)->value);
    *(table->entries + i) = (ht_entry){NULL, NULL};
  }
}

void ht_destroy(ht *table) {
  // First free allocated keys.
  for (size_t i = 0; i < table->capacity; i++) {
    free((void *)table->entries[i].key);
  }

  // Then free entries array and table itself.
  free(table->entries);
  free(table);
}

void *ht_get_hash(ht *table, const char *key, uint64_t hash) {
  // AND hash with capacity-1 to ensure it's within entries array.
  size_t index = (size_t)(hash & (uint64_t)(table->capacity - 1));

  // Loop till we find an empty entry.
  while (table->entries[index].key != NULL) {
    if (strcmp(key, table->entries[index].key) == 0) {
      // Found key, return value.
      return table->entries[index].value;
    }
    // Key wasn't in this slot, move to next (linear probing).
    index++;
    if (index >= table->capacity) {
      // At end of entries array, wrap around.
      index = 0;
    }
  }
  return NULL;
}
void *ht_get(ht *table, const char *key) {
  // AND hash with capacity-1 to ensure it's within entries array.
  uint64_t hash = hash_key(key);
  return ht_get_hash(table, key, hash);
}

// Internal function to set an entry (without expanding table).
static const char *ht_set_entry_w_hash(ht_entry *entries, size_t capacity,
                                       const char *key, uint64_t hash,
                                       void *value, size_t *plength) {

  // AND hash with capacity-1 to ensure it's within entries array.
  size_t index = (size_t)(hash & (uint64_t)(capacity - 1));

  // Loop till we find an empty entry.
  while (entries[index].key != NULL) {
    if (strcmp(key, entries[index].key) == 0) {
      // printf("update existing entry %s\n", key);
      // Found key (it already exists), update value.
      entries[index].value = value;
      return entries[index].key;
    }
    // Key wasn't in this slot, move to next (linear probing).
    index++;
    if (index >= capacity) {
      // At end of entries array, wrap around.
      index = 0;
    }
  }

  // Didn't find key, allocate+copy if needed, then insert it.
  if (plength != NULL) {
    key = strdup(key);
    if (key == NULL) {
      return NULL;
    }
    (*plength)++;
  }
  entries[index].key = (char *)key;
  entries[index].value = value;
  return key;
}
// Internal function to set an entry (without expanding table).
static const char *ht_set_entry(ht_entry *entries, size_t capacity,
                                const char *key, void *value, size_t *plength) {

  // AND hash with capacity-1 to ensure it's within entries array.
  uint64_t hash = hash_key(key);
  return ht_set_entry_w_hash(entries, capacity, key, hash, value, plength);
}

// Expand hash table to twice its current size. Return true on success,
// false if out of memory.
static bool ht_expand(ht *table) {
  // Allocate new entries array.
  size_t new_capacity = table->capacity * 2;
  if (new_capacity < table->capacity) {
    return false; // overflow (capacity would be too big)
  }
  ht_entry *new_entries = calloc(new_capacity, sizeof(ht_entry));
  if (new_entries == NULL) {
    return false;
  }

  // Iterate entries, move all non-empty ones to new table's entries.
  for (size_t i = 0; i < table->capacity; i++) {
    ht_entry entry = table->entries[i];
    if (entry.key != NULL) {
      ht_set_entry(new_entries, new_capacity, entry.key, entry.value, NULL);
    }
  }

  // Free old entries array and update this table's details.
  free(table->entries);
  table->entries = new_entries;
  table->capacity = new_capacity;
  return true;
}

const char *ht_set(ht *table, const char *key, void *value) {
  // assert(value != NULL);
  if (value == NULL) {
    return NULL;
  }

  // If length will exceed half of current capacity, expand it.
  if (table->length >= table->capacity / 2) {
    if (!ht_expand(table)) {
      return NULL;
    }
  }

  // Set entry and update length.
  return ht_set_entry(table->entries, table->capacity, key, value,
                      &table->length);
}

const char *ht_set_hash(ht *table, const char *key, uint64_t hash,
                        void *value) {

  // printf("ht set key %s %llu %p\n", key, hash, table);
  // printf("set ht key '%s'\n", key);
  // assert(value != NULL);
  if (value == NULL) {
    return NULL;
  }

  // If length will exceed half of current capacity, expand it.
  if (table->length >= table->capacity / 2) {
    if (!ht_expand(table)) {
      // printf("cannot expand\n");
      return NULL;
    }
  }

  // Set entry and update length.
  return ht_set_entry_w_hash(table->entries, table->capacity, key, hash, value,
                             &table->length);
}

size_t ht_length(ht *table) { return table->length; }

hti ht_iterator(ht *table) {
  hti it;
  it._table = table;
  it._index = 0;
  return it;
}

bool ht_next(hti *it) {
  // Loop till we've hit end of entries array.
  ht *table = it->_table;
  while (it->_index < table->capacity) {
    size_t i = it->_index;
    it->_index++;
    if (table->entries[i].key != NULL) {
      // Found next non-empty item, update iterator key and value.
      ht_entry entry = table->entries[i];
      it->key = entry.key;
      it->value = entry.value;
      return true;
    }
  }
  return false;
}
