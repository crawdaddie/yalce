#include "typeclass.h"
#include "types/type.h"
#include <stdio.h>
#include <string.h>

#define MAKE_FN_TYPE_2(arg_type, ret_type)                                     \
  ((Type){T_FN, {.T_FN = {.from = &arg_type, .to = &ret_type}}})

#define MAKE_FN_TYPE_3(arg1_type, arg2_type, ret_type)                         \
  ((Type){T_FN,                                                                \
          {.T_FN = {.from = &arg1_type,                                        \
                    .to = &MAKE_FN_TYPE_2(arg2_type, ret_type)}}})

// TOKEN_PLUS,
// TOKEN_MINUS,
// TOKEN_STAR,
// TOKEN_SLASH,
// TOKEN_MODULO,
//
// TOKEN_LT,
// TOKEN_GT,
// TOKEN_LTE,
// TOKEN_GTE,
//
// TOKEN_EQUALITY,
// TOKEN_NOT_EQUAL,

#define DERIVE_ARITHMETIC_TYPECLASS(type_name, _rank)                          \
  Type type_name##_arithmetic_fn_sig =                                         \
      MAKE_FN_TYPE_3(t_##type_name, t_##type_name, t_##type_name);             \
  TypeClass TCArithmetic_##type_name = {                                       \
      "arithmetic", .num_methods = 5, .rank = _rank,                           \
      .methods = (Method[]){                                                   \
          {                                                                    \
              "+",                                                             \
              .signature = &type_name##_arithmetic_fn_sig,                     \
          },                                                                   \
          {                                                                    \
              "-",                                                             \
              .signature = &type_name##_arithmetic_fn_sig,                     \
          },                                                                   \
          {                                                                    \
              "*",                                                             \
              .signature = &type_name##_arithmetic_fn_sig,                     \
          },                                                                   \
          {                                                                    \
              "/",                                                             \
              .signature = &type_name##_arithmetic_fn_sig,                     \
          },                                                                   \
          {                                                                    \
              "%",                                                             \
              .signature = &type_name##_arithmetic_fn_sig,                     \
          },                                                                   \
      }}
TypeClass *derive_arithmetic_for_type(Type *t) {
  TypeClass *tc = talloc(sizeof(TypeClass));
  Method *methods = talloc(sizeof(Method) * 5);
  Type *fn_sig = type_fn(type_fn(t, t), t);
  methods[0] = (Method){
      "+",
      .signature = fn_sig,
  };
  methods[1] = (Method){
      "-",
      .signature = fn_sig,
  };
  methods[2] = (Method){
      "*",
      .signature = fn_sig,
  };
  methods[3] = (Method){
      "/",
      .signature = fn_sig,
  };
  methods[4] = (Method){
      "%",
      .signature = fn_sig,
  };

  *tc = (TypeClass){"arithmetic", .num_methods = 5, .methods = methods};
  return tc;
}

#define DERIVE_ORD_TYPECLASS(type_name, _rank)                                 \
  Type type_name##_ord_fn_sig =                                                \
      MAKE_FN_TYPE_3(t_##type_name, t_##type_name, t_bool);                    \
  TypeClass TCOrd_##type_name = {"ord", .num_methods = 4, .rank = _rank,       \
                                 .methods = (Method[]){                        \
                                     {                                         \
                                         ">",                                  \
                                         .signature = &type_name##_ord_fn_sig, \
                                     },                                        \
                                     {                                         \
                                         "<",                                  \
                                         .signature = &type_name##_ord_fn_sig, \
                                     },                                        \
                                     {                                         \
                                         ">=",                                 \
                                         .signature = &type_name##_ord_fn_sig, \
                                     },                                        \
                                     {                                         \
                                         "<=",                                 \
                                         .signature = &type_name##_ord_fn_sig, \
                                     },                                        \
                                 }}

TypeClass *derive_ord_for_type(Type *t) {
  TypeClass *tc = talloc(sizeof(TypeClass));
  Method *methods = talloc(sizeof(Method) * 4);
  Type *fn_sig = type_fn(type_fn(t, t), &t_bool);
  methods[0] = (Method){
      ">",
      .signature = fn_sig,
  };
  methods[1] = (Method){
      "<",
      .signature = fn_sig,
  };
  methods[2] = (Method){
      ">=",
      .signature = fn_sig,
  };
  methods[3] = (Method){
      "<=",
      .signature = fn_sig,
  };

  *tc = (TypeClass){"ord", .num_methods = 4, .methods = methods};
  return tc;
}

#define DERIVE_EQ_TYPECLASS(type_name, _rank)                                  \
  Type type_name##_eq_fn_sig =                                                 \
      MAKE_FN_TYPE_3(t_##type_name, t_##type_name, t_bool);                    \
  TypeClass TCEq_##type_name = {"eq", .num_methods = 2, .rank = _rank,         \
                                .methods = (Method[]){                         \
                                    {                                          \
                                        "==",                                  \
                                        .signature = &type_name##_eq_fn_sig,   \
                                    },                                         \
                                    {                                          \
                                        "!=",                                  \
                                        .signature = &type_name##_eq_fn_sig,   \
                                    },                                         \
                                }}
TypeClass *derive_eq_for_type(Type *t) {
  TypeClass *tc = talloc(sizeof(TypeClass));
  Method *methods = talloc(sizeof(Method) * 4);
  Type *fn_sig = type_fn(type_fn(t, t), &t_bool);
  methods[0] = (Method){
      "==",
      .signature = fn_sig,
  };
  methods[1] = (Method){
      "!=",
      .signature = fn_sig,
  };

  *tc = (TypeClass){"eq", .num_methods = 2, .methods = methods};
  return tc;
}

DERIVE_ARITHMETIC_TYPECLASS(int, 0.0);
DERIVE_ORD_TYPECLASS(int, 0.0);
DERIVE_EQ_TYPECLASS(int, 0.0);

DERIVE_ARITHMETIC_TYPECLASS(uint64, 0.5);
DERIVE_ORD_TYPECLASS(uint64, 0.5);
DERIVE_EQ_TYPECLASS(uint64, 0.5);

DERIVE_ARITHMETIC_TYPECLASS(num, 1.0);
DERIVE_ORD_TYPECLASS(num, 1.0);
DERIVE_EQ_TYPECLASS(num, 1.0);

int find_typeclass_for_method(Type *t, const char *method_name, TypeClass *itc,
                              Type *method_signature) {

  // if (t->kind == T_INT) {
  // } else if (t->kind == T_UINT64) {
  // } else if (t->kind == T_NUM) {
  // }

  for (int i = 0; i < t->num_implements; i++) {
    TypeClass *tc = t->implements[i];
    for (int j = 0; j < tc->num_methods; j++) {
      Method method = tc->methods[j];
      if (strcmp(method_name, method.name) == 0) {
        *itc = *tc;
        *method_signature = *method.signature;
        return 0;
      }
    }
  }
  return 0;
}

bool implements(Type *t, TypeClass *tc) {
  for (int i = 0; i < t->num_implements; i++) {
    if (strcmp(t->implements[i]->name, tc->name) == 0) {
      return true;
    }
  }
  return false;
}

TypeClass *get_typeclass(Type *t, TypeClass *tc) {
  for (int i = 0; i < t->num_implements; i++) {
    if (strcmp(t->implements[i]->name, tc->name) == 0) {
      return t->implements[i];
    }
  }
  return NULL;
}

TypeClass *get_typeclass_by_name(Type *t, const char *name) {
  for (int i = 0; i < t->num_implements; i++) {
    if (strcmp(t->implements[i]->name, name) == 0) {
      return t->implements[i];
    }
  }
  return NULL;
}

Type *typeclass_method_signature(TypeClass *tc, const char *name) {
  for (int i = 0; i < tc->num_methods; i++) {
    if (strcmp(name, tc->methods[i].name) == 0) {
      return tc->methods[i].signature;
    }
  }
  return NULL;
}

void print_type_class(TypeClass *tc) {
  printf("TypeClass %s:", tc->name);
  for (int i = 0; i < tc->num_methods; i++) {
    char buffer[200];
    printf("\t%s : %s\n", tc->methods[i].name,
           type_to_string(tc->methods[i].signature, buffer));
  }
}

int add_typeclass(Type *t, TypeClass *tc) {
  int num_implements = t->num_implements + 1;
  TypeClass **implements = t->implements;
  TypeClass **new_implements = talloc(sizeof(TypeClass *) * num_implements);

  for (int i = 0; i < num_implements - 1; i++) {
    new_implements[i] = implements[i];
    if (strcmp(implements[i]->name, tc->name) == 0) {
      fprintf(stderr,
              "Error: cannot add typeclass %s to type because it already "
              "implements %s\n",
              tc->name, tc->name);
      return 1;
    }
  }
  new_implements[num_implements - 1] = tc;
  t->num_implements = num_implements;
  t->implements = new_implements;
  tfree(implements);
  return 0;
}

Type *resolve_binop_typeclass(Type *l, Type *r, token_type op) {
  const char *tc_name;
  int index;
  if (op >= TOKEN_PLUS && op <= TOKEN_MODULO) {
    tc_name = "arithmetic";
    index = op - TOKEN_PLUS;
  } else if (op >= TOKEN_LT && op <= TOKEN_GTE) {
    tc_name = "ord";
    index = op - TOKEN_LT;
  } else if (op >= TOKEN_EQUALITY && op <= TOKEN_NOT_EQUAL) {
    tc_name = "eq";
    index = op - TOKEN_EQUALITY;
  }
  // printf("find binary tc %s\n", tc_name);
  // print_type(l);
  // print_type(r);


  TypeClass *tcl = get_typeclass_by_name(l, tc_name);
  TypeClass *tcr = get_typeclass_by_name(r, tc_name);
  if (!tcl || !tcr) {
    fprintf(stderr, "Error: typeclass %s not implemented for operand", tc_name);
    return NULL;
  }

  TypeClass *dominant_tc;

  if (tcl->rank >= tcr->rank) {
    dominant_tc = tcl;
  } else {
    dominant_tc = tcr;
  }
  if (index > dominant_tc->num_methods) {

    fprintf(stderr, "Error: typeclass %s not implemented for operand", tc_name);
    return NULL;
  }

  Method dominant_method = dominant_tc->methods[index];
  return fn_return_type(dominant_method.signature);
}

Type *resolve_op_typeclass_in_type(Type *l, token_type op) {
  const char *tc_name;
  int index;
  if (op >= TOKEN_PLUS && op <= TOKEN_MODULO) {
    tc_name = "arithmetic";
    index = op - TOKEN_PLUS;
  } else if (op >= TOKEN_LT && op <= TOKEN_GTE) {
    tc_name = "ord";
    index = op - TOKEN_LT;
  } else if (op >= TOKEN_EQUALITY && op <= TOKEN_NOT_EQUAL) {
    tc_name = "eq";
    index = op - TOKEN_EQUALITY;
  }
  // printf("find binary tc %s\n", tc_name);
  // print_type(l);
  // print_type(r);


  TypeClass *tcl = get_typeclass_by_name(l, tc_name);
  if (!tcl) {
    fprintf(stderr, "Error: typeclass %s not implemented for operand", tc_name);
    return NULL;
  }

  Method dominant_method = tcl->methods[index];
  return fn_return_type(dominant_method.signature);
}
