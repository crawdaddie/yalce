#include "type_hash.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { KEY_INITIAL_CAP = 128, KEY_GROWTH = 2, KEY_MAX_DEPTH = 512,
       NUMBER_BUFFER_SIZE = 32 };
static const uint64_t fnv_offset = UINT64_C(14695981039346656037);
static const uint64_t fnv_prime = UINT64_C(1099511628211);

typedef struct {
  char *text;
  size_t length;
  size_t capacity;
  const Type *active[KEY_MAX_DEPTH];
  size_t depth;
  TypeHashStatus status;
} KeyWriter;

// Length prefixes keep punctuation and nested constructors unambiguous.
static void key_append(KeyWriter *writer, const char *text, size_t length) {
  if (writer->status != TYPE_HASH_OK) {
    return;
  }
  if (length >= SIZE_MAX - writer->length) {
    writer->status = TYPE_HASH_NO_MEMORY;
    return;
  }
  size_t needed = writer->length + length + 1;
  if (needed > writer->capacity) {
    size_t capacity = writer->capacity ? writer->capacity : KEY_INITIAL_CAP;
    while (capacity < needed) {
      if (capacity > SIZE_MAX / KEY_GROWTH) {
        capacity = needed;
        break;
      }
      capacity *= KEY_GROWTH;
    }
    char *text_buffer = realloc(writer->text, capacity);
    if (!text_buffer) {
      writer->status = TYPE_HASH_NO_MEMORY;
      return;
    }
    writer->text = text_buffer;
    writer->capacity = capacity;
  }
  memcpy(writer->text + writer->length, text, length);
  writer->length += length;
  writer->text[writer->length] = '\0';
}

static void key_text(KeyWriter *writer, const char *text) {
  key_append(writer, text, strlen(text));
}

static void key_number(KeyWriter *writer, uint64_t value) {
  char text[NUMBER_BUFFER_SIZE];
  snprintf(text, sizeof(text), "%" PRIu64 ";", value);
  key_text(writer, text);
}

static void key_name(KeyWriter *writer, const char *name) {
  if (!name) {
    key_text(writer, "null;");
    return;
  }
  size_t length = strlen(name);
  key_number(writer, length);
  key_append(writer, name, length);
}

static void key_type(KeyWriter *writer, const Type *type);

// Resolve explicit back-edges to active binders; never encode pointer addresses.
static void key_recursive(KeyWriter *writer, const Type *type) {
  if (type->kind == T_RECURSIVE_REF && type->data.T_RECURSIVE_REF.decl &&
      type->data.T_RECURSIVE_REF.decl->type) {
    const Type *target = type->data.T_RECURSIVE_REF.decl->type;
    if (target == type || target->kind == T_RECURSIVE_REF ||
        (target->kind == T_VAR && target->is_recursive_type_ref)) {
      writer->status = TYPE_HASH_INVALID;
      return;
    }
    key_type(writer, target);
    return;
  }

  const char *name = type->kind == T_RECURSIVE_REF
                         ? type->data.T_RECURSIVE_REF.name
                         : type->data.T_VAR.name;
  for (size_t i = writer->depth; name && i > 0; i--) {
    const Type *target = writer->active[i - 1];
    const char *target_name = target->alias;
    if (!target_name && (target->kind == T_CONS || target->kind == T_SUM)) {
      target_name = target->data.T_CONS.name;
    }
    if (target_name && strcmp(name, target_name) == 0) {
      key_type(writer, target);
      return;
    }
  }
  writer->status = TYPE_HASH_INVALID;
}

static void key_payload(KeyWriter *writer, const Type *type) {
  switch (type->kind) {
  case T_INT:
  case T_UINT64:
  case T_NUM:
  case T_CHAR:
  case T_BOOL:
  case T_VOID:
  case T_STRING:
  case T_EMPTY_LIST:
    return;
  case T_VAR:
    if (type->data.T_VAR.id < 0) {
      writer->status = TYPE_HASH_INVALID;
      return;
    }
    key_number(writer, (uint64_t)type->data.T_VAR.id);
    return;
  case T_FN:
    key_number(writer, type->data.T_FN.attributes);
    key_type(writer, type->data.T_FN.from);
    key_type(writer, type->data.T_FN.to);
    return;
  case T_CONS:
  case T_SUM:
    if (type->data.T_CONS.num_args < 0 ||
        (type->data.T_CONS.num_args && !type->data.T_CONS.args)) {
      writer->status = TYPE_HASH_INVALID;
      return;
    }
    key_name(writer, type->data.T_CONS.name);
    key_number(writer, (uint64_t)type->data.T_CONS.num_args);
    for (int i = 0; i < type->data.T_CONS.num_args; i++) {
      key_name(writer, type->data.T_CONS.names ? type->data.T_CONS.names[i]
                                             : NULL);
      key_type(writer, type->data.T_CONS.args[i]);
      if (writer->status != TYPE_HASH_OK) {
        return;
      }
    }
    return;
  default:
    writer->status = TYPE_HASH_INVALID;
    return;
  }
}

static void key_type(KeyWriter *writer, const Type *type) {
  if (writer->status != TYPE_HASH_OK) {
    return;
  }
  if (!type) {
    key_text(writer, "null;");
    return;
  }
  if (type->kind == T_RECURSIVE_REF ||
      (type->kind == T_VAR && type->is_recursive_type_ref)) {
    key_recursive(writer, type);
    return;
  }
  for (size_t i = 0; i < writer->depth; i++) {
    if (writer->active[i] == type) {
      key_text(writer, "$");
      key_number(writer, i);
      return;
    }
  }
  if (writer->depth == KEY_MAX_DEPTH) {
    writer->status = TYPE_HASH_TOO_DEEP;
    return;
  }

  // Only active ancestors are references; shared and copied subtrees agree.
  writer->active[writer->depth++] = type;
  key_text(writer, "(");
  key_number(writer, type->kind);
  key_number(writer, type->is_coroutine_instance);
  key_type(writer, type->closure_meta);
  key_payload(writer, type);
  key_text(writer, ")");
  writer->depth--;
}

TypeHashStatus type_hash_create(const Type *type, TypeHashKey *key) {
  if (!key) {
    return TYPE_HASH_INVALID;
  }
  *key = (TypeHashKey){0};
  if (!type) {
    return TYPE_HASH_INVALID;
  }
  KeyWriter writer = {0};
  key_text(&writer, "type-key-v1;");
  key_type(&writer, type);
  if (writer.status != TYPE_HASH_OK) {
    free(writer.text);
    return writer.status;
  }

  uint64_t hash = fnv_offset;
  for (size_t i = 0; i < writer.length; i++) {
    hash ^= (unsigned char)writer.text[i];
    hash *= fnv_prime;
  }
  *key = (TypeHashKey){.encoding = writer.text, .hash = hash};
  return TYPE_HASH_OK;
}

void type_hash_destroy(TypeHashKey *key) {
  if (!key) {
    return;
  }
  free(key->encoding);
  *key = (TypeHashKey){0};
}
