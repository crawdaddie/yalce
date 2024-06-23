#include "type_inference/infer.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int unique_id = 0;
char unique_name = 'a';

Type t_int = {T_INT};
Type t_num = {T_NUM};
Type t_string = {T_STRING};
Type t_bool = {T_BOOL};
Type t_void = {T_VOID};

NonGeneric *new_non_generic() {
  NonGeneric *ng = malloc(sizeof(NonGeneric));
  ng->cursor = 0;
  return ng;
}

void add_to_non_generic(NonGeneric *ng, Type *s) { ng->list[ng->cursor++] = s; }

NonGeneric *copy_non_generic(NonGeneric *src) {
  NonGeneric *dst = malloc(sizeof(NonGeneric));
  *dst = *src;
  return dst;
}

static bool is_generic(Type *, NonGeneric *);
static bool occursin(Type *, Type *);
static bool occursin_type(Type *, Type *);

bool error_occurred = false;

bool is_type_variable(Type *ty) { return ty->kind == T_VAR; }

bool is_type_operator(Type *ty) { return ty->kind != T_VAR; }

bool same_type(Type *t1, Type *t2) {
  if (t1 == NULL || t2 == NULL) {
    puts("NULL error");
    return false;
  }

  if (t1->kind != t2->kind) {
    return false;
  }

  if (is_type_operator(t1)) {
    for (int i = 0; i < t1->ntype; i++) {
      if (!same_type(t1->types[i], t2->types[i]))
        return false;
    }
  } else if (is_type_variable(t1)) {
    if (t1->t_data.T_VAR.id != t2->t_data.T_VAR.id)
      return false;
  }

  return true;
}

Type *prune(Type *ty) {
  if (ty == NULL)
    return NULL;

  if (is_type_variable(ty)) {
    if (ty->t_data.T_VAR.instance != NULL) {
      ty->t_data.T_VAR.instance = prune(ty->t_data.T_VAR.instance);
      return ty->t_data.T_VAR.instance;
    }
  }

  return ty;
}

static bool is_generic(Type *tvar, NonGeneric *nongeneric) {
  for (int i = 0; i < nongeneric->cursor; i++) {
    if (occursin_type(tvar, nongeneric->list[i]))
      return false;
  }

  return true;
}

static bool occursin_type(Type *tvar, Type *texp) {
  texp = prune(texp);

  if (is_type_variable(texp)) {
    return same_type(tvar, texp);
  } else if (is_type_operator(texp)) {
    return occursin(tvar, texp);
  } else
    return false;
}

static bool occursin(Type *tyvar, Type *type) {
  for (int i = 0; i < type->ntype; i++) {
    if (occursin_type(tyvar, type->types[i]))
      return true;
  }

  return false;
}

Type *type_map_exist(Map *self, Type *key) {
  for (int i = 0; i < self->key->len; i++) {
    if (same_type((Type *)self->key->data[i], key)) {
      return (Type *)self->value->data[i];
    }
  }

  return NULL;
}

Vector *New_Vector() {
  Vector *vec = malloc(sizeof(Vector));

  vec->data = malloc(sizeof(void *) * 16);
  vec->len = 0;
  vec->reserved = 16;

  return vec;
}

Vector *New_Vector_With_Size(int size) {
  Vector *vec = malloc(sizeof(Vector));

  vec->data = malloc(sizeof(void *) * size);
  vec->len = size;
  vec->reserved = size;

  for (int i = 0; i < size; ++i) {
    vec->data[i] = NULL;
  }

  return vec;
}

void Delete_Vector(Vector *self) {
  free(self->data);

  free(self);
}

void vec_push(Vector *self, void *d) {
  if (self->len == self->reserved) {
    self->reserved *= 2;
    self->data = realloc(self->data, sizeof(void *) * self->reserved);
  }

  self->data[self->len++] = d;
}

void *vec_pop(Vector *self) {
  assert(self->len != 0);

  return self->data[--self->len];
}

void *vec_last(Vector *self) { return self->data[self->len - 1]; }

Map *New_Map() {
  Map *self = malloc(sizeof(Map));

  self->key = New_Vector();
  self->value = New_Vector();

  return self;
}

void map_push(Map *self, void *key, void *value) {
  vec_push(self->key, key);
  vec_push(self->value, value);
}

Type *type_get_or_put(Map *self, Type *key, Type *default_value) {
  Type *e = type_map_exist(self, key);

  if (e != NULL) {
    return e;
  } else {
    map_push(self, key, default_value);
    return default_value;
  }
}

Type *type_operator0(enum TypeKind k) {
  Type *self = malloc(sizeof(Type));

  self->kind = k;
  self->ntype = 0;
  self->types[0] = NULL;
  self->types[1] = NULL;

  return self;
}

Type *type_operator2(enum TypeKind k, Type *a1, Type *a2) {
  Type *self = malloc(sizeof(Type));

  self->kind = k;
  self->ntype = 2;
  self->types[0] = a1;
  self->types[1] = a2;

  switch (k) {
  // case T_FN:
  //   self->t_data.T_FN.arg = a1;
  //   self->t_data.T_FN.result = a2;
  //   break;
  case T_PAIR:
    self->t_data.T_PAIR.fst = a1;
    self->t_data.T_PAIR.snd = a2;
    break;
  default:
    break;
  }

  return self;
}

Type *type_var() {
  Type *self = type_operator0(T_VAR);

  self->t_data.T_VAR.id = unique_id++;
  self->t_data.T_VAR.name = 0;
  self->t_data.T_VAR.instance = NULL;

  return (Type *)self;
}

Type *freshrec(Type *ty, NonGeneric *nongeneric, Map *mappings) {
  Type *pty = prune(ty);

  if (is_type_variable(pty)) {
    if (is_generic(pty, nongeneric)) {
      return type_get_or_put(mappings, pty, type_var());
    } else
      return pty;
  } else if (is_type_operator(pty)) {
    switch (pty->ntype) {
    case 0:
      return type_operator0(pty->kind);
    case 2:
      return type_operator2(pty->kind,
                            freshrec(pty->types[0], nongeneric, mappings),
                            freshrec(pty->types[1], nongeneric, mappings));
    default:
      puts("????");
    }
  }

  /* unreachable */
  return NULL;
}

Type *fresh(Type *t, NonGeneric *nongeneric) {
  Map *mappings = New_Map();

  return freshrec(t, nongeneric, mappings);
}

Env *new_env() {
  Env *self = malloc(sizeof(Env));
  self->cursor = 0;
  return self;
}

Env *copy_env(Env *src) {
  Env *dst = malloc(sizeof(Env));
  *dst = *src;
  return dst;
}

void add_to_env(Env *self, char *sym, Type *type) {
  self->list[self->cursor].key = sym;
  self->list[self->cursor].type = type;
  self->cursor++;
}

Type *lookup(Env *self, char *key, NonGeneric *nongeneric) {
  for (int i = 0; i < self->cursor; i++) {
    if (strcmp(key, self->list[i].key) == 0) {
      return fresh(self->list[i].type, nongeneric);
    }
  }

  return NULL;
}

static bool is_numeric_type(Type *t) {
  if (t == NULL) {
    return false;
  }
  return (t->kind == T_INT) || (t->kind == T_NUM);
}

Type *type_fn(int arg_len, Type *args, Type *ret) {
  Type *ty = malloc(sizeof(Type));
  ty->kind = T_FN;
  ty->t_data.T_FN.len = arg_len;
  ty->t_data.T_FN.args = args;
  ty->t_data.T_FN.ret_type = ret;
  return ty;
}

void unify(Type *t1, Type *t2) {
  t1 = prune(t1);
  t2 = prune(t2);

  // printf("unifying...");
  // typedump_core(t1);
  // printf(", ");
  // typedump_core(t2);
  // puts("");

  if (is_type_variable(t1)) {
    if (!same_type(t1, t2)) {
      if (occursin_type(t1, t2)) {
        printf("recursive unification");
        error_occurred = true;
        return;
      }
      t1->t_data.T_VAR.instance = t2;
    }
  } else if (is_type_operator(t1) && is_type_variable(t2)) {
    unify(t2, t1);
  } else if (is_type_operator(t1) && is_type_operator(t2)) {
    if (t1->kind != t2->kind || t1->ntype != t2->ntype) {
      // printf("type error: ");
      // typedump_core(t1);
      // printf(", ");
      // typedump_core(t2);
      // puts("");

      error_occurred = true;

      return;
    }

    for (int i = 0; i < t1->ntype; i++) {
      unify(t1->types[i], t2->types[i]);
    }
  } else {
    puts("cannot infer");
  }
}

static inline uint32_t max(uint32_t a, uint32_t b) { return a >= b ? a : b; }
Type *infer(Env *env, Ast *e, NonGeneric *nongeneric) {
  if (nongeneric == NULL) {
    nongeneric = new_non_generic();
  }

  if (!e)
    return NULL;

  switch (e->tag) {
  case AST_INT: {
    Type *result = &t_int;
    e->md = result;
    return result;
  }

  case AST_NUMBER: {
    Type *result = &t_num;
    e->md = result;
    return result;
  }

  case AST_STRING: {
    Type *result = &t_string;
    e->md = result;
    return result;
  }

  case AST_BOOL: {
    Type *result = &t_bool;
    e->md = result;
    return result;
  }
  case AST_IDENTIFIER: {
    const char *id = e->data.AST_IDENTIFIER.value;
    Type *ty = lookup(env, id, nongeneric);
    e->md = ty;

    if (ty == NULL) {
      printf("unknown identifer `%s`\n", id);
      return NULL;
    }

    return ty;
  }

    // case AST_LET: {
    // }
  case AST_LET: {

    Type *def = infer(env, e->data.AST_LET.expr, nongeneric);
    Env *new = copy_env(env);
    // char *id_ = malloc(sizeof(char) * e->data.AST_LET.name.length);
    // strcpy(id_, e->data.AST_LET.name.chars);
    // printf("before segfault? %s\n", id_);
    //
    char *id = e->data.AST_LET.name.chars;
    add_to_env(new, id, def);

    if (e->data.AST_LET.in_expr != NULL) {
      Type *result = infer(new, e->data.AST_LET.in_expr, nongeneric);
      e->md = result;
      return result;
    }
    e->md = def;
    return def;
  }

  case AST_BODY: {
    Type *ty;
    for (size_t i = 0; i < e->data.AST_BODY.len; ++i) {
      Ast *stmt = e->data.AST_BODY.stmts[i];
      ty = infer(env, stmt, NULL);
    }
    e->md = ty;
    return ty;
  }

  case AST_BINOP: {
    Type *lt = infer(env, e->data.AST_BINOP.left, nongeneric);
    Type *rt = infer(env, e->data.AST_BINOP.right, nongeneric);

    Type *res;
    if (is_numeric_type(lt) && is_numeric_type(rt)) {
      token_type op = e->data.AST_BINOP.op;
      if (op >= TOKEN_PLUS && op <= TOKEN_MODULO) {
        res = lt->kind >= rt->kind ? lt : rt;
        e->md = res;
        return res;
      } else if (op >= TOKEN_LT && op <= TOKEN_NOT_EQUAL) {
        res = &t_bool;
        e->md = res;
        return res;
      }
    }
    if (is_type_variable(lt) && is_numeric_type(rt)) {
      e->md = lt;
      return lt;
    }

    if (is_type_variable(rt) && is_numeric_type(lt)) {
      e->md = rt;
      return rt;
    }
    e->md = lt;
    return lt;
  }
  case AST_LAMBDA: {
    bool is_recursive = e->data.AST_LAMBDA.fn_name.chars != NULL;
    char *fn_name = e->data.AST_LAMBDA.fn_name.chars;
    Type *fn_name_type = type_var();

    Env *copied_env = copy_env(env);
    int arg_len = e->data.AST_LAMBDA.len;
    Type *arg_types = malloc(sizeof(Type) * arg_len);
    NonGeneric *copied_ng = copy_non_generic(nongeneric);
    for (int i = 0; i < e->data.AST_LAMBDA.len; i++) {
      Type *argty = arg_types + i;
      argty->kind = T_VAR;
      argty->t_data.T_VAR.id = unique_id++;
      argty->t_data.T_VAR.name = 0;
      argty->t_data.T_VAR.instance = NULL;
      add_to_env(copied_env, e->data.AST_LAMBDA.params[i].chars, argty);

      add_to_non_generic(copied_ng, argty);
    }

    // NonGeneric *copied_ng = copy_non_generic(nongeneric);

    add_to_env(copied_env, fn_name, fn_name_type);
    Type *ret = infer(copied_env, e->data.AST_LAMBDA.body, copied_ng);
    Type *result = type_fn(arg_len, arg_types, ret);
    e->md = result;
    return result;
  }
  case AST_APPLICATION: {
    Type *fn = infer(env, e->data.AST_APPLICATION.function, nongeneric);
    int app_len = e->data.AST_APPLICATION.len;
    Type *args = malloc(sizeof(Type) * e->data.AST_APPLICATION.len);
    for (int i = 0; i < e->data.AST_APPLICATION.len; i++) {
      *(args + i) = *infer(env, e->data.AST_APPLICATION.args + i, nongeneric);
    }
    // infer(env, e->arg, nongeneric);
    Type *res = type_var();
    // unify(fn, type_fn(arg, res));
    e->md = res;

    return res;
  }
    // case LETREC: {
    //   Type *new = type_var();
    //
    //   Env *new_env = copy_env(env);
    //   NonGeneric *new_nongeneric = copy_non_generic(nongeneric);
    //
    //   add_to_env(new_env, e->recname, new);
    //   add_to_non_generic(new_nongeneric, new);
    //
    //   Type *def = analyze(new_env, e->recdef, new_nongeneric);
    //
    //   unify(new, def);
    //
    //   Type *result = analyze(new_env, e->recbody, new_nongeneric);
    //   printf("letrec %s: ", e->recname);
    //   typedump(result);
    //
    //   return result;
    // }
    // default:
    //   printf("internal error");
    // }
  }
  return NULL;
}
void print_type(Type *ty) {
  if (ty == NULL) {
    return;
  }

  switch (ty->kind) {
  case T_INT:
    printf("int");
    break;

  case T_NUM:
    printf("double");
    break;

  case T_BOOL:
    printf("bool");
    break;

  case T_STRING:
    printf("string");
    break;

  case T_VOID:
    printf("()");
    break;

  case T_FN: {
    printf("(");
    for (int i = 0; i < ty->t_data.T_FN.len; i++) {
      print_type(ty->t_data.T_FN.args + i);
      printf(" -> ");
    }
    print_type(ty->t_data.T_FN.ret_type);
    printf(")");
    break;
  }
  case T_VAR: {
    if (ty->t_data.T_VAR.instance != NULL) {
      print_type(prune(ty));
    } else if (ty->t_data.T_VAR.name == 0) {
      printf("'%c", ty->t_data.T_VAR.name = unique_name++);
    } else {
      printf("'%c", ty->t_data.T_VAR.name);
    }
    break;
  }
  case T_PAIR: {
    printf("(");
    print_type(ty->t_data.T_PAIR.fst);
    printf(" * ");
    print_type(ty->t_data.T_PAIR.snd);
    printf(")");
    break;
  }
  default:
    printf("Error: Type serialization not implemented");
  }
}
