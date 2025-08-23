#include "./btree_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal helper functions
static BTreeNode *node_new(const char *key, void *value);
static void node_free(BTreeNode *node);
static void rotate_left(BTreeMap *map, BTreeNode *x);
static void rotate_right(BTreeMap *map, BTreeNode *x);
static void insert_fixup(BTreeMap *map, BTreeNode *z);
static void delete_fixup(BTreeMap *map, BTreeNode *x);
static BTreeNode *tree_minimum(BTreeNode *node);
static BTreeNode *tree_search(BTreeNode *node, const char *key);
static void transplant(BTreeMap *map, BTreeNode *u, BTreeNode *v);

// Create a new BTreeMap
BTreeMap *btreemap_new(void) {
  BTreeMap *map = malloc(sizeof(BTreeMap));
  if (!map)
    return NULL;

  map->root = NULL;
  map->size = 0;
  return map;
}

// Create a new node
static BTreeNode *node_new(const char *key, void *value) {
  BTreeNode *node = malloc(sizeof(BTreeNode));
  if (!node)
    return NULL;

  // Allocate and copy the key string
  node->key = malloc(strlen(key) + 1);
  if (!node->key) {
    free(node);
    return NULL;
  }
  strcpy(node->key, key);

  node->value = value;
  node->color = BT_RED; // New nodes are always red initially
  node->left = NULL;
  node->right = NULL;
  node->parent = NULL;

  return node;
}

// Free a node and its key
static void node_free(BTreeNode *node) {
  if (node) {
    free(node->key);
    free(node);
  }
}

// Search for a node with given key
static BTreeNode *tree_search(BTreeNode *node, const char *key) {
  while (node != NULL) {
    int cmp = strcmp(key, node->key);
    if (cmp == 0) {
      return node;
    } else if (cmp < 0) {
      node = node->left;
    } else {
      node = node->right;
    }
  }
  return NULL;
}

// Left rotation for Red-Black tree balancing
static void rotate_left(BTreeMap *map, BTreeNode *x) {
  BTreeNode *y = x->right;
  x->right = y->left;

  if (y->left != NULL) {
    y->left->parent = x;
  }

  y->parent = x->parent;

  if (x->parent == NULL) {
    map->root = y;
  } else if (x == x->parent->left) {
    x->parent->left = y;
  } else {
    x->parent->right = y;
  }

  y->left = x;
  x->parent = y;
}

// Right rotation for Red-Black tree balancing
static void rotate_right(BTreeMap *map, BTreeNode *x) {
  BTreeNode *y = x->left;
  x->left = y->right;

  if (y->right != NULL) {
    y->right->parent = x;
  }

  y->parent = x->parent;

  if (x->parent == NULL) {
    map->root = y;
  } else if (x == x->parent->right) {
    x->parent->right = y;
  } else {
    x->parent->left = y;
  }

  y->right = x;
  x->parent = y;
}

// Fix Red-Black tree properties after insertion
static void insert_fixup(BTreeMap *map, BTreeNode *z) {
  while (z->parent != NULL && z->parent->color == BT_RED) {
    if (z->parent == z->parent->parent->left) {
      BTreeNode *y = z->parent->parent->right;
      if (y != NULL && y->color == BT_RED) {
        z->parent->color = BT_BLACK;
        y->color = BT_BLACK;
        z->parent->parent->color = BT_RED;
        z = z->parent->parent;
      } else {
        if (z == z->parent->right) {
          z = z->parent;
          rotate_left(map, z);
        }
        z->parent->color = BT_BLACK;
        z->parent->parent->color = BT_RED;
        rotate_right(map, z->parent->parent);
      }
    } else {
      BTreeNode *y = z->parent->parent->left;
      if (y != NULL && y->color == BT_RED) {
        z->parent->color = BT_BLACK;
        y->color = BT_BLACK;
        z->parent->parent->color = BT_RED;
        z = z->parent->parent;
      } else {
        if (z == z->parent->left) {
          z = z->parent;
          rotate_right(map, z);
        }
        z->parent->color = BT_BLACK;
        z->parent->parent->color = BT_RED;
        rotate_left(map, z->parent->parent);
      }
    }
  }
  map->root->color = BT_BLACK;
}

// Insert a key-value pair
void *btreemap_insert(BTreeMap *map, const char *key, void *value) {
  if (!map || !key)
    return NULL;

  BTreeNode *y = NULL;
  BTreeNode *x = map->root;

  // Find the insertion point
  while (x != NULL) {
    y = x;
    int cmp = strcmp(key, x->key);
    if (cmp == 0) {
      // Key already exists, update value and return old value
      void *old_value = x->value;
      x->value = value;
      return old_value;
    } else if (cmp < 0) {
      x = x->left;
    } else {
      x = x->right;
    }
  }

  // Create new node
  BTreeNode *z = node_new(key, value);
  if (!z)
    return NULL;

  z->parent = y;
  if (y == NULL) {
    map->root = z;
  } else if (strcmp(key, y->key) < 0) {
    y->left = z;
  } else {
    y->right = z;
  }

  map->size++;
  insert_fixup(map, z);
  return NULL; // No previous value
}

// Get value by key
void *btreemap_get(BTreeMap *map, const char *key) {
  if (!map || !key)
    return NULL;

  BTreeNode *node = tree_search(map->root, key);
  return node ? node->value : NULL;
}

// Find minimum node in subtree
static BTreeNode *tree_minimum(BTreeNode *node) {
  while (node->left != NULL) {
    node = node->left;
  }
  return node;
}

// Transplant one subtree with another
static void transplant(BTreeMap *map, BTreeNode *u, BTreeNode *v) {
  if (u->parent == NULL) {
    map->root = v;
  } else if (u == u->parent->left) {
    u->parent->left = v;
  } else {
    u->parent->right = v;
  }

  if (v != NULL) {
    v->parent = u->parent;
  }
}

// Fix Red-Black tree properties after deletion
static void delete_fixup(BTreeMap *map, BTreeNode *x) {
  while (x != map->root && (x == NULL || x->color == BT_BLACK)) {
    if (x == (x ? x->parent->left : NULL) ||
        (x && x->parent && x == x->parent->left)) {
      BTreeNode *w = x ? x->parent->right : NULL;
      if (w && w->color == BT_RED) {
        w->color = BT_BLACK;
        if (x)
          x->parent->color = BT_RED;
        rotate_left(map, x ? x->parent : NULL);
        w = x ? x->parent->right : NULL;
      }

      if (w && (!w->left || w->left->color == BT_BLACK) &&
          (!w->right || w->right->color == BT_BLACK)) {
        w->color = BT_RED;
        x = x ? x->parent : NULL;
      } else {
        if (w && (!w->right || w->right->color == BT_BLACK)) {
          if (w->left)
            w->left->color = BT_BLACK;
          w->color = BT_RED;
          rotate_right(map, w);
          w = x ? x->parent->right : NULL;
        }

        if (w) {
          w->color = x && x->parent ? x->parent->color : BT_BLACK;
          if (w->right)
            w->right->color = BT_BLACK;
        }
        if (x && x->parent) {
          x->parent->color = BT_BLACK;
          rotate_left(map, x->parent);
        }
        x = map->root;
      }
    } else {
      // Symmetric case (x is right child)
      BTreeNode *w = x && x->parent ? x->parent->left : NULL;
      if (w && w->color == BT_RED) {
        w->color = BT_BLACK;
        if (x && x->parent)
          x->parent->color = BT_RED;
        rotate_right(map, x->parent);
        w = x && x->parent ? x->parent->left : NULL;
      }

      if (w && (!w->right || w->right->color == BT_BLACK) &&
          (!w->left || w->left->color == BT_BLACK)) {
        w->color = BT_RED;
        x = x ? x->parent : NULL;
      } else {
        if (w && (!w->left || w->left->color == BT_BLACK)) {
          if (w->right)
            w->right->color = BT_BLACK;
          w->color = BT_RED;
          rotate_left(map, w);
          w = x && x->parent ? x->parent->left : NULL;
        }

        if (w) {
          w->color = x && x->parent ? x->parent->color : BT_BLACK;
          if (w->left)
            w->left->color = BT_BLACK;
        }
        if (x && x->parent) {
          x->parent->color = BT_BLACK;
          rotate_right(map, x->parent);
        }
        x = map->root;
      }
    }
  }

  if (x)
    x->color = BT_BLACK;
}

// Remove a key-value pair
int btreemap_remove(BTreeMap *map, const char *key) {
  if (!map || !key)
    return 0;

  BTreeNode *z = tree_search(map->root, key);
  if (!z)
    return 0; // Key not found

  BTreeNode *y = z;
  BTreeNode *x;
  Color y_original_color = y->color;

  if (z->left == NULL) {
    x = z->right;
    transplant(map, z, z->right);
  } else if (z->right == NULL) {
    x = z->left;
    transplant(map, z, z->left);
  } else {
    y = tree_minimum(z->right);
    y_original_color = y->color;
    x = y->right;

    if (y->parent == z) {
      if (x)
        x->parent = y;
    } else {
      transplant(map, y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }

    transplant(map, z, y);
    y->left = z->left;
    y->left->parent = y;
    y->color = z->color;
  }

  if (y_original_color == BT_BLACK) {
    delete_fixup(map, x);
  }

  node_free(z);
  map->size--;
  return 1; // Successfully removed
}

// Get the number of elements in the map
size_t btreemap_len(BTreeMap *map) { return map ? map->size : 0; }

// Clear all elements from the map
static void clear_recursive(BTreeNode *node) {
  if (node) {
    clear_recursive(node->left);
    clear_recursive(node->right);
    node_free(node);
  }
}

void btreemap_clear(BTreeMap *map) {
  if (map) {
    clear_recursive(map->root);
    map->root = NULL;
    map->size = 0;
  }
}

// Free the entire map
void btreemap_free(BTreeMap *map) {
  if (map) {
    btreemap_clear(map);
    free(map);
  }
}
