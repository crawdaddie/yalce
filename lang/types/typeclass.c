#include "typeclass.h"
#include "types/type.h"
#include <stdlib.h>

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

static Type int_binop_method_signature = MAKE_FN_TYPE_3(&t_int, &t_int, &t_int);
static Type int_ord_method_signature = MAKE_FN_TYPE_3(&t_int, &t_int, &t_bool);
TypeClass int_tc_table[] = {
    [OP_INDEX(TOKEN_PLUS)] = {.name = "+",
                              .method_signature = &int_binop_method_signature,
                              .rank = 0.0},
    [OP_INDEX(TOKEN_MINUS)] = {.name = "-",
                               .method_signature = &int_binop_method_signature,
                               .rank = 0.0},
    [OP_INDEX(TOKEN_STAR)] = {.name = "*",
                              .method_signature = &int_binop_method_signature,
                              .rank = 0.0},
    [OP_INDEX(TOKEN_SLASH)] = {.name = "/",
                               .method_signature = &int_binop_method_signature,
                               .rank = 0.0},
    [OP_INDEX(TOKEN_MODULO)] = {.name = "%",
                                .method_signature = &int_binop_method_signature,
                                .rank = 0.0},
    [OP_INDEX(TOKEN_LT)] = {.name = "<",
                            .method_signature = &int_ord_method_signature,
                            .rank = 0.0},
    [OP_INDEX(TOKEN_GT)] = {.name = ">",
                            .method_signature = &int_ord_method_signature,
                            .rank = 0.0},
    [OP_INDEX(TOKEN_LTE)] = {.name = "<=",
                             .method_signature = &int_ord_method_signature,
                             .rank = 0.0},
    [OP_INDEX(TOKEN_GTE)] = {.name = ">=",
                             .method_signature = &int_ord_method_signature,
                             .rank = 0.0},
    [OP_INDEX(TOKEN_EQUALITY)] = {.name = "==",
                                  .method_signature = &int_ord_method_signature,
                                  .rank = 0.0},
    [OP_INDEX(TOKEN_NOT_EQUAL)] = {.name = "!=",
                                   .method_signature =
                                       &int_ord_method_signature,
                                   .rank = 0.0},
};

static Type uint64_binop_method_signature =
    MAKE_FN_TYPE_3(&t_uint64, &t_uint64, &t_uint64);

static Type uint64_ord_method_signature =
    MAKE_FN_TYPE_3(&t_uint64, &t_uint64, &t_bool);

TypeClass uint64_tc_table[] = {
    [OP_INDEX(TOKEN_PLUS)] = {.name = "+",
                              .method_signature =
                                  &uint64_binop_method_signature,
                              .rank = 1.0},
    [OP_INDEX(TOKEN_MINUS)] = {.name = "-",
                               .method_signature =
                                   &uint64_binop_method_signature,
                               .rank = 1.0},
    [OP_INDEX(TOKEN_STAR)] = {.name = "*",
                              .method_signature =
                                  &uint64_binop_method_signature,
                              .rank = 1.0},
    [OP_INDEX(TOKEN_SLASH)] = {.name = "/",
                               .method_signature =
                                   &uint64_binop_method_signature,
                               .rank = 1.0},
    [OP_INDEX(TOKEN_MODULO)] = {.name = "%",
                                .method_signature =
                                    &uint64_binop_method_signature,
                                .rank = 1.0},
    [OP_INDEX(TOKEN_LT)] = {.name = "<",
                            .method_signature = &uint64_ord_method_signature,
                            .rank = 1.0},
    [OP_INDEX(TOKEN_GT)] = {.name = ">",
                            .method_signature = &uint64_ord_method_signature,
                            .rank = 1.0},
    [OP_INDEX(TOKEN_LTE)] = {.name = "<=",
                             .method_signature = &uint64_ord_method_signature,
                             .rank = 1.0},
    [OP_INDEX(TOKEN_GTE)] = {.name = ">=",
                             .method_signature = &uint64_ord_method_signature,
                             .rank = 1.0},
    [OP_INDEX(TOKEN_EQUALITY)] = {.name = "==",
                                  .method_signature =
                                      &uint64_ord_method_signature,
                                  .rank = 1.0},
    [OP_INDEX(TOKEN_NOT_EQUAL)] = {.name = "!=",
                                   .method_signature =
                                       &uint64_ord_method_signature,
                                   .rank = 1.0},
};

static Type num_binop_method_signature = MAKE_FN_TYPE_3(&t_num, &t_num, &t_num);
static Type num_ord_method_signature = MAKE_FN_TYPE_3(&t_num, &t_num, &t_bool);
TypeClass num_tc_table[] = {
    [OP_INDEX(TOKEN_PLUS)] = {.name = "+",
                              .method_signature = &num_binop_method_signature,
                              .rank = 2.0},
    [OP_INDEX(TOKEN_MINUS)] = {.name = "-",
                               .method_signature = &num_binop_method_signature,
                               .rank = 2.0},
    [OP_INDEX(TOKEN_STAR)] = {.name = "*",
                              .method_signature = &num_binop_method_signature,
                              .rank = 2.0},
    [OP_INDEX(TOKEN_SLASH)] = {.name = "/",
                               .method_signature = &num_binop_method_signature,
                               .rank = 2.0},
    [OP_INDEX(TOKEN_MODULO)] = {.name = "%",
                                .method_signature = &num_binop_method_signature,
                                .rank = 2.0},
    [OP_INDEX(TOKEN_LT)] = {.name = "<",
                            .method_signature = &num_ord_method_signature,
                            .rank = 2.0},
    [OP_INDEX(TOKEN_GT)] = {.name = ">",
                            .method_signature = &num_ord_method_signature,
                            .rank = 2.0},
    [OP_INDEX(TOKEN_LTE)] = {.name = "<=",
                             .method_signature = &num_ord_method_signature,
                             .rank = 2.0},
    [OP_INDEX(TOKEN_GTE)] = {.name = ">=",
                             .method_signature = &num_ord_method_signature,
                             .rank = 2.0},
    [OP_INDEX(TOKEN_EQUALITY)] = {.name = "==",
                                  .method_signature = &num_ord_method_signature,
                                  .rank = 2.0},
    [OP_INDEX(TOKEN_NOT_EQUAL)] = {.name = "!=",
                                   .method_signature =
                                       &num_ord_method_signature,
                                   .rank = 2.0},
};

TypeClass *find_op_impl(struct Type t, token_type op) {
  if (op >= TOKEN_PLUS && op <= TOKEN_NOT_EQUAL) {
    int opi = op - TOKEN_PLUS;
    if (opi < t.num_implements) {
      TypeClass *tc = t.implements + opi;
      return tc;
    }
    return NULL;
  }

  return NULL;
}
