#ifndef _LANG_TYPES_FRESHEN_MAP_H
#define _LANG_TYPES_FRESHEN_MAP_H

typedef struct Type Type;
typedef struct TypeList TypeList;

typedef struct FreshenMap {
  int *src_ids;
  Type **dst_types;
  int len;
  int cap;
} FreshenMap;

void freshen_map_extend(FreshenMap *map, int src_id, Type *dst_type);
Type *freshen_map_lookup(FreshenMap *map, int src_id);
Type *freshen_map_apply_to_type(FreshenMap *map, Type *t);
TypeList *freshen_map_apply_to_typelist(FreshenMap *map, TypeList *params);

#endif
