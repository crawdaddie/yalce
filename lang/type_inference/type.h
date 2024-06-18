#ifndef _LANG_TYPES_H
#define _LANG_TYPES_H

enum TypeKind {
  /* Type Operator */
  T_INT,
  T_NUM,
  T_STRING,
  T_BOOL,
  T_FN,
  T_PAIR,
  T_TUPLE,
  T_LIST,
  /* Type Variable  */
  T_VAR,
};

typedef struct Type Type;
struct Type {
  enum TypeKind kind;
  int ntype;
  Type *types[2];

  union {
    /* Function */
    struct {
      Type *arg;
      Type *result;
    } T_FN;

    /* Pair */
    struct {
      Type *fst;
      Type *snd;
    } T_PAIR;

    /* Type Variable */
    struct {
      int id;
      char name;
      Type *instance;
    } T_VAR;
  } t_data;
};

typedef struct TupleST TupleST;
typedef struct Env Env;

struct TupleST {
  char *key;
  Type *type;
};

struct Env {
  TupleST list[128];
  int cursor;
};

typedef struct NonGeneric NonGeneric;

struct NonGeneric {
  Type *list[128];
  int cursor;
};
typedef struct Vector {
  void **data;
  int len;
  int reserved;
} Vector;

typedef struct Map {
  Vector *key;
  Vector *value;
} Map;

#endif
