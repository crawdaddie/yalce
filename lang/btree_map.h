#ifndef _LANG_BTREE_MAP_H
#define _LANG_BTREE_MAP_H
// Red-Black Tree colors
#include <stddef.h>

typedef enum { BT_RED, BT_BLACK } Color;

// Tree node structure
typedef struct BTreeNode {
  char *key;                // null-terminated string key
  void *value;              // void pointer value
  Color color;              // Red-Black tree color
  struct BTreeNode *left;   // left child
  struct BTreeNode *right;  // right child
  struct BTreeNode *parent; // parent node
} BTreeNode;

// BTreeMap structure
typedef struct {
  BTreeNode *root;
  size_t size;
} BTreeMap;

// Function declarations
BTreeMap *btreemap_new(void);
void btreemap_free(BTreeMap *map);
void *btreemap_insert(BTreeMap *map, const char *key, void *value);
void *btreemap_get(BTreeMap *map, const char *key);
int btreemap_remove(BTreeMap *map, const char *key);
size_t btreemap_len(BTreeMap *map);
void btreemap_clear(BTreeMap *map);
#endif
