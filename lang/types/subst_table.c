#include "./subst_table.h"
#include "./inference.h"
#include <string.h>

static void subst_table_ensure_present_capacity(Subst *subst, int needed) {
  if (!subst || needed <= subst->present_cap) {
    return;
  }

  int new_cap = subst->present_cap > 0 ? subst->present_cap : 8;
  while (new_cap < needed) {
    new_cap *= 2;
  }

  int *present_ids = t_alloc(sizeof(int) * (size_t)new_cap);
  if (subst->present_ids && subst->count > 0) {
    memcpy(present_ids, subst->present_ids, sizeof(int) * (size_t)subst->count);
  }
  subst->present_ids = present_ids;
  subst->present_cap = new_cap;
}

static Subst empty_subst_sentinel = {.bindings = NULL,
                                     .occupied = NULL,
                                     .present_ids = NULL,
                                     .cap = 0,
                                     .count = 0,
                                     .present_cap = 0};

Subst *subst_table_create(int initial_cap) {
  Subst *subst = t_alloc(sizeof(Subst));
  *subst = (Subst){.bindings = NULL,
                   .occupied = NULL,
                   .present_ids = NULL,
                   .cap = 0,
                   .count = 0,
                   .present_cap = 0};
  if (initial_cap > 0) {
    subst->bindings = t_alloc(sizeof(Type *) * (size_t)initial_cap);
    memset(subst->bindings, 0, sizeof(Type *) * (size_t)initial_cap);
    subst->occupied = t_alloc(sizeof(unsigned char) * (size_t)initial_cap);
    memset(subst->occupied, 0, sizeof(unsigned char) * (size_t)initial_cap);
    subst->cap = initial_cap;
  }
  return subst;
}

void subst_table_ensure_capacity(Subst *subst, int var_id) {
  if (!subst || var_id < 0) {
    return;
  }
  if (var_id < subst->cap) {
    return;
  }

  int new_cap = subst->cap > 0 ? subst->cap : 8;
  while (new_cap <= var_id) {
    new_cap *= 2;
  }

  Type **bindings = t_alloc(sizeof(Type *) * (size_t)new_cap);
  memset(bindings, 0, sizeof(Type *) * (size_t)new_cap);
  unsigned char *occupied =
      t_alloc(sizeof(unsigned char) * (size_t)new_cap);
  memset(occupied, 0, sizeof(unsigned char) * (size_t)new_cap);
  if (subst->bindings && subst->cap > 0) {
    memcpy(bindings, subst->bindings, sizeof(Type *) * (size_t)subst->cap);
    memcpy(occupied, subst->occupied,
           sizeof(unsigned char) * (size_t)subst->cap);
  }
  subst->bindings = bindings;
  subst->occupied = occupied;
  subst->cap = new_cap;
}

Subst *subst_table_clone(Subst *subst) {
  if (subst_table_is_empty(subst)) {
    return NULL;
  }
  Subst *copy = subst_table_create(subst->cap);
  if (subst->cap > 0) {
    memcpy(copy->bindings, subst->bindings, sizeof(Type *) * (size_t)subst->cap);
    memcpy(copy->occupied, subst->occupied,
           sizeof(unsigned char) * (size_t)subst->cap);
  }
  if (subst->count > 0) {
    subst_table_ensure_present_capacity(copy, subst->count);
    memcpy(copy->present_ids, subst->present_ids,
           sizeof(int) * (size_t)subst->count);
    copy->count = subst->count;
  }
  return copy;
}

Subst *subst_table_extend(Subst *subst, int var_id, Type *type) {
  if (subst_table_is_empty(subst)) {
    subst = subst_table_create(0);
  }
  subst_table_ensure_capacity(subst, var_id);
  if (!subst->occupied[var_id]) {
    subst_table_ensure_present_capacity(subst, subst->count + 1);
    subst->occupied[var_id] = 1;
    subst->present_ids[subst->count++] = var_id;
  }
  subst->bindings[var_id] = type;
  return subst;
}

Type *subst_table_lookup(Subst *subst, int var_id) {
  if (subst_table_is_empty(subst) || var_id < 0 || var_id >= subst->cap) {
    return NULL;
  }
  return subst->bindings[var_id];
}

int subst_table_binding_count(Subst *subst) {
  if (subst_table_is_empty(subst)) {
    return 0;
  }
  return subst->count;
}

int subst_table_bound_var_id(Subst *subst, int index) {
  if (subst_table_is_empty(subst) || index < 0 || index >= subst->count) {
    return -1;
  }
  return subst->present_ids[index];
}

bool subst_table_is_empty(Subst *subst) {
  return !subst || subst == &empty_subst_sentinel ||
         (subst->cap == 0 && subst->bindings == NULL);
}

Subst *subst_table_empty(void) { return &empty_subst_sentinel; }
