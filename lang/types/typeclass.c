#include "typeclass.h"
#include "types/type.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKE_FN_TYPE_1(ret_type)                                               \
  {                                                                            \
    T_FN, {                                                                    \
      .T_FN = {.from = NULL, .to = (ret_type) }                                \
    }                                                                          \
  }

#define MAKE_FN_TYPE_2(arg_type, ret_type)                                     \
  {                                                                            \
    T_FN, {                                                                    \
      .T_FN = {.from = (arg_type), .to = &(Type)MAKE_FN_TYPE_1(ret_type) }     \
    }                                                                          \
  }

#define MAKE_FN_TYPE_3(arg1_type, arg2_type, ret_type)                         \
  {                                                                            \
    T_FN, {                                                                    \
      .T_FN = {                                                                \
        .from = (arg1_type),                                                   \
        .to = &(Type)MAKE_FN_TYPE_2(arg2_type, ret_type)                       \
      }                                                                        \
    }                                                                          \
  }

#define OP_INDEX(op) op - TOKEN_PLUS
#define TYPECLASS_ARITHMETIC(t)                                                \
  static Type t##_binop_method_signature =                                     \
      MAKE_FN_TYPE_3(&t_##t, &t_##t, &t_##t);                                  \
                                                                               \
  TypeClassImpl t##_arithmetic_tc_table[] = {                                  \
      [OP_INDEX(TOKEN_PLUS)] =                                                 \
          {                                                                    \
              .name = "+",                                                     \
              .method_signature = &t##_binop_method_signature,                 \
          },                                                                   \
      [OP_INDEX(TOKEN_MINUS)] =                                                \
          {                                                                    \
              .name = "-",                                                     \
              .method_signature = &t##_binop_method_signature,                 \
          },                                                                   \
      [OP_INDEX(TOKEN_STAR)] =                                                 \
          {                                                                    \
              .name = "*",                                                     \
              .method_signature = &t##_binop_method_signature,                 \
          },                                                                   \
      [OP_INDEX(TOKEN_SLASH)] =                                                \
          {                                                                    \
              .name = "/",                                                     \
              .method_signature = &t##_binop_method_signature,                 \
          },                                                                   \
      [OP_INDEX(TOKEN_MODULO)] =                                               \
          {                                                                    \
              .name = "%",                                                     \
              .method_signature = &t##_binop_method_signature,                 \
          },                                                                   \
  };

#define TYPECLASS_ORD(t)                                                       \
  static Type t##_ord_method_signature =                                       \
      MAKE_FN_TYPE_3(&t_##t, &t_##t, &t_bool);                                 \
  TypeClassImpl t##_ord_tc_table[] = {                                         \
      [OP_INDEX(TOKEN_LT)] =                                                   \
          {                                                                    \
              .name = "<",                                                     \
              .method_signature = &t##_ord_method_signature,                   \
          },                                                                   \
      [OP_INDEX(TOKEN_GT)] =                                                   \
          {                                                                    \
              .name = ">",                                                     \
              .method_signature = &t##_ord_method_signature,                   \
          },                                                                   \
      [OP_INDEX(TOKEN_LTE)] =                                                  \
          {                                                                    \
              .name = "<=",                                                    \
              .method_signature = &t##_ord_method_signature,                   \
          },                                                                   \
      [OP_INDEX(TOKEN_GTE)] =                                                  \
          {                                                                    \
              .name = ">=",                                                    \
              .method_signature = &t##_ord_method_signature,                   \
          },                                                                   \
      [OP_INDEX(TOKEN_EQUALITY)] =                                             \
          {                                                                    \
              .name = "==",                                                    \
              .method_signature = &t##_ord_method_signature,                   \
          },                                                                   \
      [OP_INDEX(TOKEN_NOT_EQUAL)] =                                            \
          {                                                                    \
              .name = "!=",                                                    \
              .method_signature = &t##_ord_method_signature,                   \
          },                                                                   \
  }

#define ADD_TYPECLASSES(t)                                                     \
  TYPECLASS_ARITHMETIC(t);                                                     \
  TYPECLASS_ORD(t);                                                            \
  TypeClass t##_traits[] = {                                                   \
      {"arithmetic", 5, t##_tc_table, 0.0},                                    \
      {"ord", 6, t##_tc_table, 0.0},                                           \
  };

ADD_TYPECLASSES(int);
ADD_TYPECLASSES(uint64);
ADD_TYPECLASSES(num);

TypeClassImpl *find_op_impl(TypeClass t, token_type op) {

  if (op >= TOKEN_PLUS && op <= TOKEN_NOT_EQUAL) {
    int opi = op - TOKEN_PLUS;
    if (opi < t.num) {
      TypeClassImpl *tc = t.methods + opi;
      return tc;
    }
    return NULL;
  }

  return NULL;
}
