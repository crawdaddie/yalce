// Simple hash table implemented in C.

#include "ht.h"
#include "common.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 256 // must not be zero
#define HT_ALIGNOF(T) __alignof__(T)

static void *ht_default_alloc(void *ctx, size_t size, size_t align) {
  (void)ctx;
  (void)align;
  return malloc(size);
}

static void ht_default_free(void *ctx, void *ptr) {
  (void)ctx;
  free(ptr);
}

static ht_allocator ht_normalize_allocator(ht_allocator allocator) {
  if (!allocator.alloc) {
    allocator.alloc = ht_default_alloc;
    allocator.free = ht_default_free;
    allocator.ctx = NULL;
  }
  return allocator;
}

static void *ht_alloc_with(ht_allocator *allocator, size_t size, size_t align) {
  return allocator->alloc(allocator->ctx, size, align);
}

static void ht_free_with(ht_allocator *allocator, void *ptr) {
  if (allocator->free && ptr) {
    allocator->free(allocator->ctx, ptr);
  }
}

static void *ht_alloc_zero_with(ht_allocator *allocator, size_t size,
                                size_t align) {
  void *ptr = ht_alloc_with(allocator, size, align);
  if (ptr) {
    memset(ptr, 0, size);
  }
  return ptr;
}

static char *ht_strdup_with(ht *table, const char *key) {
  size_t len = strlen(key);
  char *copy = ht_alloc_with(&table->allocator, len + 1, HT_ALIGNOF(char));
  if (!copy) {
    return NULL;
  }
  memcpy(copy, key, len + 1);
  return copy;
}

ht *ht_create(void) {
  return ht_create_with_allocator((ht_allocator){0});
}

ht *ht_create_with_allocator(ht_allocator allocator) {
  allocator = ht_normalize_allocator(allocator);

  ht *table = ht_alloc_zero_with(&allocator, sizeof(ht), HT_ALIGNOF(ht));
  if (table == NULL) {
    return NULL;
  }

  table->allocator = allocator;
  table->owns_self = true;
  table->length = 0;
  table->capacity = INITIAL_CAPACITY;

  table->entries =
      ht_alloc_zero_with(&table->allocator, table->capacity * sizeof(ht_entry),
                         HT_ALIGNOF(ht_entry));
  if (table->entries == NULL) {
    ht_free_with(&table->allocator, table);
    return NULL;
  }
  return table;
}

void ht_init(ht *table) {
  ht_init_with_allocator(table, (ht_allocator){0});
}

void ht_init_with_allocator(ht *table, ht_allocator allocator) {
  if (table == NULL) {
    return;
  }

  allocator = ht_normalize_allocator(allocator);
  memset(table, 0, sizeof(*table));
  table->allocator = allocator;
  table->owns_self = false;
  table->length = 0;
  table->capacity = INITIAL_CAPACITY;

  table->entries =
      ht_alloc_zero_with(&table->allocator, table->capacity * sizeof(ht_entry),
                         HT_ALIGNOF(ht_entry));
  if (table->entries == NULL) {
    table->capacity = 0;
  }
}

void ht_reinit(ht *table) {
  if (table == NULL) {
    return;
  }

  table->length = 0;
  if (!table->entries || table->capacity == 0) {
    table->capacity = INITIAL_CAPACITY;
    table->entries = ht_alloc_zero_with(
        &table->allocator, table->capacity * sizeof(ht_entry),
        HT_ALIGNOF(ht_entry));
    if (!table->entries) {
      table->capacity = 0;
      return;
    }
  }

  memset(table->entries, 0, table->capacity * sizeof(ht_entry));
}

void ht_destroy(ht *table) {
  if (!table) {
    return;
  }

  if (table->allocator.free) {
    for (size_t i = 0; i < table->capacity; i++) {
      ht_free_with(&table->allocator, (void *)table->entries[i].key);
    }
    ht_free_with(&table->allocator, table->entries);
  }

  if (table->owns_self) {
    ht_free_with(&table->allocator, table);
  } else {
    table->entries = NULL;
    table->capacity = 0;
    table->length = 0;
  }
}

void *ht_get_hash(ht *table, const char *key, uint64_t hash) {
  if (!table || !key || !table->entries || table->capacity == 0) {
    return NULL;
  }

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
  if (!table || !key) {
    return NULL;
  }

  // AND hash with capacity-1 to ensure it's within entries array.
  uint64_t hash = hash_key(key);
  return ht_get_hash(table, key, hash);
}

// Internal function to set an entry (without expanding table).
static const char *ht_set_entry_w_hash(ht *table, ht_entry *entries,
                                       size_t capacity, const char *key,
                                       uint64_t hash, void *value,
                                       size_t *plength) {

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
    key = ht_strdup_with(table, key);
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
static const char *ht_set_entry(ht *table, ht_entry *entries, size_t capacity,
                                const char *key, void *value,
                                size_t *plength) {

  // AND hash with capacity-1 to ensure it's within entries array.
  uint64_t hash = hash_key(key);
  return ht_set_entry_w_hash(table, entries, capacity, key, hash, value,
                             plength);
}

// Expand hash table to twice its current size. Return true on success,
// false if out of memory.
static bool ht_expand(ht *table) {
  // Allocate new entries array.
  size_t new_capacity = table->capacity * 2;
  if (new_capacity < table->capacity) {
    return false; // overflow (capacity would be too big)
  }
  ht_entry *new_entries =
      ht_alloc_zero_with(&table->allocator, new_capacity * sizeof(ht_entry),
                         HT_ALIGNOF(ht_entry));
  if (new_entries == NULL) {
    return false;
  }

  // Iterate entries, move all non-empty ones to new table's entries.
  for (size_t i = 0; i < table->capacity; i++) {
    ht_entry entry = table->entries[i];
    if (entry.key != NULL) {
      ht_set_entry(table, new_entries, new_capacity, entry.key, entry.value,
                   NULL);
    }
  }

  // Free old entries array and update this table's details.
  ht_free_with(&table->allocator, table->entries);
  table->entries = new_entries;
  table->capacity = new_capacity;
  return true;
}

const char *ht_set(ht *table, const char *key, void *value) {
  // assert(value != NULL);
  if (!table || !key || !table->entries || table->capacity == 0 ||
      value == NULL) {
    return NULL;
  }

  // If length will exceed half of current capacity, expand it.
  if (table->length >= table->capacity / 2) {
    if (!ht_expand(table)) {
      return NULL;
    }
  }

  // Set entry and update length.
  return ht_set_entry(table, table->entries, table->capacity, key, value,
                      &table->length);
}

const char *ht_set_hash(ht *table, const char *key, uint64_t hash,
                        void *value) {

  // printf("ht set key %s %llu %p\n", key, hash, table);
  // printf("set ht key '%s'\n", key);
  // assert(value != NULL);
  if (!table || !key || !table->entries || table->capacity == 0 ||
      value == NULL) {
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
  return ht_set_entry_w_hash(table, table->entries, table->capacity, key, hash,
                             value, &table->length);
}

size_t ht_length(ht *table) { return table ? table->length : 0; }

hti ht_iterator(ht *table) {
  hti it;
  memset(&it, 0, sizeof(it));
  it._table = table;
  it._index = 0;
  return it;
}

bool ht_next(hti *it) {
  // Loop till we've hit end of entries array.
  ht *table = it->_table;
  if (!table || !table->entries) {
    return false;
  }
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
