#ifndef _LANG_TYPES_SUBST_TABLE_H
#define _LANG_TYPES_SUBST_TABLE_H

#include <stdbool.h>

typedef struct Type Type;

typedef struct Subst {
  Type **bindings;
  unsigned char *occupied;
  int *present_ids;
  int cap;
  int count;
  int present_cap;
} Subst;

Subst *subst_table_create(int initial_cap);
void subst_table_ensure_capacity(Subst *subst, int var_id);
Subst *subst_table_clone(Subst *subst);
Subst *subst_table_extend(Subst *subst, int var_id, Type *type);
Type *subst_table_lookup(Subst *subst, int var_id);
int subst_table_binding_count(Subst *subst);
int subst_table_bound_var_id(Subst *subst, int index);
bool subst_table_is_empty(Subst *subst);
Subst *subst_table_empty(void);

#endif
