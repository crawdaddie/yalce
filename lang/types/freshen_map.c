#include "./freshen_map.h"
#include "./inference.h"
#include "./type.h"
#include <string.h>

static void ensure_freshen_map_capacity(FreshenMap *map, int needed_len) {
  if (!map || needed_len <= map->cap) {
    return;
  }
  int new_cap = map->cap > 0 ? map->cap : 4;
  while (new_cap < needed_len) {
    new_cap *= 2;
  }

  int *src_ids = t_alloc(sizeof(int) * (size_t)new_cap);
  Type **dst_types = t_alloc(sizeof(Type *) * (size_t)new_cap);
  if (map->len > 0) {
    memcpy(src_ids, map->src_ids, sizeof(int) * (size_t)map->len);
    memcpy(dst_types, map->dst_types, sizeof(Type *) * (size_t)map->len);
  }
  map->src_ids = src_ids;
  map->dst_types = dst_types;
  map->cap = new_cap;
}

Type *freshen_map_lookup(FreshenMap *map, int src_id) {
  if (!map || src_id < 0) {
    return NULL;
  }
  for (int i = 0; i < map->len; i++) {
    if (map->src_ids[i] == src_id) {
      return map->dst_types[i];
    }
  }
  return NULL;
}

void freshen_map_extend(FreshenMap *map, int src_id, Type *dst_type) {
  if (!map || src_id < 0) {
    return;
  }
  for (int i = 0; i < map->len; i++) {
    if (map->src_ids[i] == src_id) {
      map->dst_types[i] = dst_type;
      return;
    }
  }
  ensure_freshen_map_capacity(map, map->len + 1);
  map->src_ids[map->len] = src_id;
  map->dst_types[map->len] = dst_type;
  map->len++;
}

Type *freshen_map_apply_to_type(FreshenMap *map, Type *t) {
  if (!map || !t) {
    return t;
  }

  switch (t->kind) {
  case T_VAR: {
    Type *found = freshen_map_lookup(map, t->data.T_VAR.id);
    return found ? found : t;
  }
  case T_RECURSIVE_REF:
    return t;
  case T_FN: {
    Type *from = freshen_map_apply_to_type(map, t->data.T_FN.from);
    Type *to = freshen_map_apply_to_type(map, t->data.T_FN.to);
    if (from == t->data.T_FN.from && to == t->data.T_FN.to &&
        !t->closure_meta) {
      return t;
    }
    Type *result = t_alloc(sizeof(Type));
    *result = (Type){T_FN, {.T_FN = {from, to}}};
    result->data.T_FN.attributes = t->data.T_FN.attributes;
    if (t->closure_meta) {
      result->closure_meta = freshen_map_apply_to_type(map, t->closure_meta);
    }
    return result;
  }
  case T_CONS:
  case T_SUM: {
    bool changed = false;
    Type **new_args = NULL;
    if (t->data.T_CONS.num_args > 0) {
      new_args = t_alloc(sizeof(Type *) * (size_t)t->data.T_CONS.num_args);
      for (int i = 0; i < t->data.T_CONS.num_args; i++) {
        new_args[i] = freshen_map_apply_to_type(map, t->data.T_CONS.args[i]);
        if (new_args[i] != t->data.T_CONS.args[i]) {
          changed = true;
        }
      }
    }
    if (!changed) {
      return t;
    }
    if (is_coroutine_type(t)) {
      return create_coroutine_instance_type(new_args[0]);
    }
    Type *result = t_alloc(sizeof(Type));
    *result = *t;
    result->data.T_CONS.args = new_args;
    return result;
  }
  default:
    return t;
  }
}

TypeList *freshen_map_apply_to_typelist(FreshenMap *map, TypeList *params) {
  if (!params) {
    return NULL;
  }
  TypeList *head = NULL;
  TypeList *tail = NULL;
  for (TypeList *tl = params; tl; tl = tl->next) {
    TypeList *node = t_alloc(sizeof(TypeList));
    node->type = freshen_map_apply_to_type(map, tl->type);
    node->next = NULL;
    if (!head) {
      head = node;
    } else {
      tail->next = node;
    }
    tail = node;
  }
  return head;
}
