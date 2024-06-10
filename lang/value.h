#ifndef _LANG_VALUE_H
#define _LANG_VALUE_H
#include "common.h"
#include "ht.h"
#include "parse.h"
#include <node.h>
#include <stdbool.h>

typedef struct Value Value;

#define STACK_MAX 256

typedef Value (*val_bind_fn_t)(Value val);
typedef struct {
  // ht stack[STACK_MAX];
  ht *stack;
  int stack_ptr;
  val_bind_fn_t val_bind;
} LangCtx;

// typedef double (*extern_double)(void *, ...);
// typedef int (*extern_int)(void *, ...);
// typedef void *(*extern_void_ptr)(void *, ...);
// typedef void (*extern_void)(void *, ...);
// typedef char *(*extern_str)(void *, ...);
typedef Value (*native_fn_t)(Value *);
typedef Value (*meta_fn_t)(Ast *ast, LangCtx *ctx);

typedef enum value_type {
  VALUE_INT,
  VALUE_NUMBER,
  VALUE_STRING,
  VALUE_BOOL,
  VALUE_VOID,
  VALUE_OBJ,
  VALUE_LIST,
  VALUE_FN,
  VALUE_CURRIED_FN,
  VALUE_TYPE,
  VALUE_NATIVE_FN,
  VALUE_SYNTH_NODE,
  VALUE_META_FN,
  VALUE_PARTIAL_FN,
} value_type;

// FUNCTIONS
typedef struct {
  int len;
  Value *partial_args;
  int num_partial_args;
  ObjString *params;
  const char *fn_name;
  Ast *body;
  int scope_ptr;
  bool is_recursive_ref;
} Function;

typedef struct {
  int len;
  Value *partial_args;
  int num_partial_args;
  native_fn_t handle;
  value_type *input_types;
  value_type return_type;
} NativeFn;

// typedef struct {
//   enum {
//     FN,
//     NATIVE_FN,
//   } type;
//   union {
//     Function fn;
//     NativeFn native_fn;
//   } function;
//   Value *partial_args;
//   int num_partial_args;
//   int len; // length of underlying function for convenience
// } PartialFn;

// LIST TYPES
typedef struct {
  int len;
  value_type type;
  int *items;
} IntList;

typedef struct {
  int len;
  value_type type;
  double *items;
} NumberList;

typedef struct {
  int len;
  value_type type;
  char **items;
  int *lens;
  uint64_t *hashes;
} StringList;

typedef struct {
  int len;
  value_type type;
  void **items;
} ObjList;

struct Value {
  value_type type;
  union {
    int vint;
    double vnum;
    ObjString vstr;
    bool vbool;
    void *vobj;
    value_type type;
    void *vlist;
    Node *vnode;
    meta_fn_t vmeta_fn;
    Function function;
    Function recursive_ref;
    NativeFn native_fn;
    // PartialFn partial_fn;
  } value;
};

#define INT(i)                                                                 \
  (Value) {                                                                    \
    VALUE_INT, { .vint = i }                                                   \
  }

#define NUM(i)                                                                 \
  (Value) {                                                                    \
    VALUE_NUMBER, { .vnum = i }                                                \
  }

#define STRING(i)                                                              \
  (Value) {                                                                    \
    VALUE_STRING, { .vstr = i }                                                \
  }

#define BOOL(i)                                                                \
  (Value) {                                                                    \
    VALUE_BOOL, { .vbool = i }                                                 \
  }

#define OBJ(i)                                                                 \
  (Value) {                                                                    \
    VALUE_OBJ, { .vobj = i }                                                   \
  }

#define VOID                                                                   \
  (Value) { VALUE_VOID }

#define LIST(l)                                                                \
  (Value) {                                                                    \
    VALUE_LIST, { .vlist = l }                                                 \
  }
#define NATIVE_FN(_handle, _len)                                               \
  &(Value) {                                                                   \
    .type = VALUE_NATIVE_FN, .value = {                                        \
      .native_fn = {.handle = &_handle, .len = _len}                           \
    }                                                                          \
  }

#define TYPE_MAP(_id, _type)                                                   \
  (native_symbol_map) {                                                        \
    .id = _id, &(Value) {                                                      \
      .type = VALUE_TYPE, .value = {.type = _type }                            \
    }                                                                          \
  }
typedef struct {
  const char *id;
  Value *type;
} native_symbol_map;

#endif
