// Simple hash table implemented in C.
#ifndef _LANG_HT_H
#define _LANG_HT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Hash table structure: create with ht_create, free with ht_destroy.
typedef struct ht ht;

// Hash table entry (slot may be filled or empty).
typedef struct {
  const char *key; // key is NULL if this slot is empty
  void *value;
} ht_entry;

// Hash table structure: create with ht_create, free with ht_destroy.
struct ht {
  ht_entry *entries; // hash slots
  size_t capacity;   // size of _entries array
  size_t length;     // number of items in hash table
};

// Create hash table and return pointer to it, or NULL if out of memory.
ht *ht_create(void);

// Free memory allocated for hash table, including allocated keys.
void ht_destroy(ht *table);

// Get item with given key (NUL-terminated) from hash table. Return
// value (which was set with ht_set), or NULL if key not found.
void *ht_get(ht *table, const char *key);

// Set item with given key (NUL-terminated) to value (which must not
// be NULL). If not already present in table, key is copied to newly
// allocated memory (keys are freed automatically when ht_destroy is
// called). Return address of copied key, or NULL if out of memory.
const char *ht_set(ht *table, const char *key, void *value);

// Return number of items in hash table.
size_t ht_length(ht *table);

// Hash table iterator: create with ht_iterator, iterate with ht_next.
typedef struct {
  const char *key; // current key
  void *value;     // current value

  // Don't use these fields directly.
  ht *_table;    // reference to hash table being iterated
  size_t _index; // current index into ht._entries
} hti;

// Return new hash table iterator (for use with ht_next).
hti ht_iterator(ht *table);

// Move iterator to next item in hash table, update iterator's key
// and value to current item, and return true. If there are no more
// items, return false. Don't call ht_set during iteration.
bool ht_next(hti *it);

uint64_t hash_key(const char *key);

const char *ht_set_hash(ht *table, const char *key, uint64_t hash, void *value);

void *ht_get_hash(ht *table, const char *key, uint64_t hash);

void ht_init(ht *table);

void ht_reinit(ht *table);
#endif // _HT_H
