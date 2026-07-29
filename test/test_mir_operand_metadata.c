#include "mir/mir.h"

#include <stdio.h>
#include <string.h>

typedef struct {
  MirValueId value;
  MirOperandRole role;
  MirOperandUse use;
  size_t index;
} ExpectedOperand;

typedef struct {
  const char *label;
  const ExpectedOperand *expected;
  size_t expected_len;
  size_t seen;
  bool failed;
} VisitCtx;

static const char *role_name(MirOperandRole role) {
  switch (role) {
  case MIR_OPERAND_ROLE_VALUE:
    return "value";
  case MIR_OPERAND_ROLE_CALLEE:
    return "callee";
  case MIR_OPERAND_ROLE_SCRUTINEE:
    return "scrutinee";
  case MIR_OPERAND_ROLE_RETURN:
    return "return";
  case MIR_OPERAND_ROLE_CONDITION:
    return "condition";
  case MIR_OPERAND_ROLE_TAG:
    return "tag";
  case MIR_OPERAND_ROLE_CONTAINER:
    return "container";
  case MIR_OPERAND_ROLE_INDEX:
    return "index";
  case MIR_OPERAND_ROLE_ELEMENT:
    return "element";
  case MIR_OPERAND_ROLE_FIELD:
    return "field";
  case MIR_OPERAND_ROLE_FUNCTION:
    return "function";
  case MIR_OPERAND_ROLE_ENV:
    return "env";
  }
  return "?";
}

static const char *use_name(MirOperandUse use) {
  switch (use) {
  case MIR_OPERAND_USE_BORROW:
    return "borrow";
  case MIR_OPERAND_USE_CONSUME:
    return "consume";
  }
  return "?";
}

static bool collect_operand(MirInstr *instr, MirOperand operand, void *ctx_ptr) {
  (void)instr;
  VisitCtx *ctx = ctx_ptr;
  if (!ctx || ctx->seen >= ctx->expected_len) {
    fprintf(stderr, "%s: unexpected operand %u\n",
            ctx ? ctx->label : "<null>", operand.value);
    if (ctx) {
      ctx->failed = true;
    }
    return false;
  }

  ExpectedOperand expected = ctx->expected[ctx->seen];
  if (operand.value != expected.value || operand.role != expected.role ||
      operand.use != expected.use || operand.index != expected.index) {
    fprintf(stderr,
            "%s operand %zu: got value=%u role=%s use=%s index=%zu, "
            "expected value=%u role=%s use=%s index=%zu\n",
            ctx->label, ctx->seen, operand.value, role_name(operand.role),
            use_name(operand.use), operand.index, expected.value,
            role_name(expected.role), use_name(expected.use), expected.index);
    ctx->failed = true;
    return false;
  }

  ctx->seen++;
  return true;
}

static MirValueId rewrite_operand(MirInstr *instr, MirOperand operand,
                                  void *ctx) {
  (void)instr;
  (void)ctx;
  return operand.value + 1000;
}

static bool check_operands(const char *label, MirInstr *instr,
                           const ExpectedOperand *expected,
                           size_t expected_len) {
  VisitCtx ctx = {
      .label = label,
      .expected = expected,
      .expected_len = expected_len,
      .seen = 0,
      .failed = false,
  };
  if (!mir_instr_for_each_operand(instr, collect_operand, &ctx)) {
    fprintf(stderr, "%s: operand visitor returned false\n", label);
    return false;
  }
  if (ctx.failed || ctx.seen != expected_len) {
    fprintf(stderr, "%s: saw %zu operands, expected %zu\n", label, ctx.seen,
            expected_len);
    return false;
  }
  return true;
}

static bool check_rewrite_items(const char *label, MirValueIdVec items,
                                const MirValueId *expected,
                                size_t expected_len) {
  if (items.len != expected_len) {
    fprintf(stderr, "%s: rewritten item len %zu, expected %zu\n", label,
            items.len, expected_len);
    return false;
  }
  for (size_t i = 0; i < expected_len; i++) {
    if (items.items[i] != expected[i]) {
      fprintf(stderr, "%s item %zu: got %u, expected %u\n", label, i,
              items.items[i], expected[i]);
      return false;
    }
  }
  return true;
}

static bool test_construct_items(const char *label, MirConstructKind kind,
                                 MirOperandRole role, MirOperandUse use) {
  MirValueId raw_items[] = {10, 11};
  MirInstr instr = {
      .kind = MIR_CONSTRUCT,
      .data.construct = {
          .kind = kind,
          .items = {.items = raw_items, .len = 2, .cap = 2},
      },
  };
  ExpectedOperand expected[] = {
      {10, role, use, 0},
      {11, role, use, 1},
  };
  MirValueId rewritten[] = {1010, 1011};
  return check_operands(label, &instr, expected, 2) &&
         (mir_instr_rewrite_operands(&instr, rewrite_operand, NULL), true) &&
         check_rewrite_items(label, instr.data.construct.items, rewritten, 2);
}

static bool test_construct_operands(const char *label, MirConstructKind kind,
                                    const ExpectedOperand *expected,
                                    size_t expected_len) {
  MirInstr instr = {
      .kind = MIR_CONSTRUCT,
      .data.construct = {
          .kind = kind,
          .operands = {20, 21, 22},
          .reuse_token = MIR_NO_VALUE,
      },
  };
  if (!check_operands(label, &instr, expected, expected_len)) {
    return false;
  }
  mir_instr_rewrite_operands(&instr, rewrite_operand, NULL);
  for (size_t i = 0; i < expected_len; i++) {
    if (instr.data.construct.operands[i] != expected[i].value + 1000) {
      fprintf(stderr, "%s operand %zu rewrite failed\n", label, i);
      return false;
    }
  }
  return true;
}

static bool test_extract(const char *label, MirExtractKind kind,
                         MirValueId value, MirValueId index_value,
                         const ExpectedOperand *expected,
                         size_t expected_len) {
  MirInstr instr = {
      .kind = MIR_EXTRACT,
      .data.extract = {
          .kind = kind,
          .value = value,
          .index_value = index_value,
      },
  };
  if (!check_operands(label, &instr, expected, expected_len)) {
    return false;
  }
  mir_instr_rewrite_operands(&instr, rewrite_operand, NULL);
  if (value != MIR_NO_VALUE && instr.data.extract.value != value + 1000) {
    fprintf(stderr, "%s value rewrite failed\n", label);
    return false;
  }
  if (index_value != MIR_NO_VALUE &&
      instr.data.extract.index_value != index_value + 1000) {
    fprintf(stderr, "%s index rewrite failed\n", label);
    return false;
  }
  return true;
}

static bool test_coro_next(void) {
  MirInstr instr = {
      .kind = MIR_CORO_NEXT,
      .data.call = {.callee = 40},
  };
  ExpectedOperand expected[] = {
      {40, MIR_OPERAND_ROLE_CALLEE, MIR_OPERAND_USE_BORROW, 0},
  };
  if (!check_operands("coro.next", &instr, expected, 1)) {
    return false;
  }
  mir_instr_rewrite_operands(&instr, rewrite_operand, NULL);
  if (instr.data.call.callee != 1040) {
    fprintf(stderr, "coro.next callee rewrite failed\n");
    return false;
  }
  return true;
}

static bool test_coro_reset(void) {
  MirInstr instr = {
      .kind = MIR_CORO_RESET,
      .data.call = {.callee = 41},
  };
  ExpectedOperand expected[] = {
      {41, MIR_OPERAND_ROLE_VALUE, MIR_OPERAND_USE_CONSUME, 0},
  };
  if (!check_operands("coro.reset", &instr, expected, 1)) {
    return false;
  }
  mir_instr_rewrite_operands(&instr, rewrite_operand, NULL);
  if (instr.data.call.callee != 1041) {
    fprintf(stderr, "coro.reset operand rewrite failed\n");
    return false;
  }
  return true;
}

int main(void) {
  bool ok = true;

  ok &= test_construct_items("construct.tuple", MIR_CONSTRUCT_TUPLE,
                             MIR_OPERAND_ROLE_FIELD,
                             MIR_OPERAND_USE_CONSUME);
  ok &= test_construct_items("construct.variant", MIR_CONSTRUCT_VARIANT,
                             MIR_OPERAND_ROLE_FIELD,
                             MIR_OPERAND_USE_CONSUME);
  ok &= test_construct_items("construct.closure_env",
                             MIR_CONSTRUCT_CLOSURE_ENV,
                             MIR_OPERAND_ROLE_FIELD,
                             MIR_OPERAND_USE_CONSUME);
  ok &= test_construct_items("construct.array_literal",
                             MIR_CONSTRUCT_ARRAY_LITERAL,
                             MIR_OPERAND_ROLE_ELEMENT,
                             MIR_OPERAND_USE_CONSUME);

  ExpectedOperand list_cons[] = {
      {20, MIR_OPERAND_ROLE_ELEMENT, MIR_OPERAND_USE_CONSUME, 0},
      {21, MIR_OPERAND_ROLE_CONTAINER, MIR_OPERAND_USE_CONSUME, 1},
  };
  ok &= test_construct_operands("construct.list_cons",
                                MIR_CONSTRUCT_LIST_CONS, list_cons, 2);

  ExpectedOperand array_fill_const[] = {
      {20, MIR_OPERAND_ROLE_VALUE, MIR_OPERAND_USE_BORROW, 0},
      {21, MIR_OPERAND_ROLE_ELEMENT, MIR_OPERAND_USE_CONSUME, 1},
  };
  ok &= test_construct_operands("construct.array_fill_const",
                                MIR_CONSTRUCT_ARRAY_FILL_CONST,
                                array_fill_const, 2);

  ExpectedOperand array_fill[] = {
      {20, MIR_OPERAND_ROLE_VALUE, MIR_OPERAND_USE_BORROW, 0},
      {21, MIR_OPERAND_ROLE_FUNCTION, MIR_OPERAND_USE_BORROW, 1},
  };
  ok &= test_construct_operands("construct.array_fill",
                                MIR_CONSTRUCT_ARRAY_FILL, array_fill, 2);

  ExpectedOperand array_range[] = {
      {20, MIR_OPERAND_ROLE_INDEX, MIR_OPERAND_USE_BORROW, 0},
      {21, MIR_OPERAND_ROLE_VALUE, MIR_OPERAND_USE_BORROW, 1},
      {22, MIR_OPERAND_ROLE_CONTAINER, MIR_OPERAND_USE_BORROW, 2},
  };
  ok &= test_construct_operands("construct.array_range",
                                MIR_CONSTRUCT_ARRAY_RANGE, array_range, 3);

  ExpectedOperand closure[] = {
      {20, MIR_OPERAND_ROLE_FUNCTION, MIR_OPERAND_USE_BORROW, 0},
      {21, MIR_OPERAND_ROLE_ENV, MIR_OPERAND_USE_CONSUME, 1},
  };
  ok &= test_construct_operands("construct.closure", MIR_CONSTRUCT_CLOSURE,
                                closure, 2);

  MirInstr empty = {
      .kind = MIR_CONSTRUCT,
      .data.construct = {.kind = MIR_CONSTRUCT_LIST_EMPTY},
  };
  ok &= check_operands("construct.list_empty", &empty, NULL, 0);
  mir_instr_rewrite_operands(&empty, rewrite_operand, NULL);

  ExpectedOperand container0[] = {
      {30, MIR_OPERAND_ROLE_CONTAINER, MIR_OPERAND_USE_BORROW, 0},
  };
  ok &= test_extract("extract.field", MIR_EXTRACT_FIELD, 30, MIR_NO_VALUE,
                     container0, 1);
  ok &= test_extract("extract.variant_payload", MIR_EXTRACT_VARIANT_PAYLOAD,
                     30, MIR_NO_VALUE, container0, 1);
  ok &= test_extract("extract.list_head", MIR_EXTRACT_LIST_HEAD, 30,
                     MIR_NO_VALUE, container0, 1);
  ok &= test_extract("extract.list_tail", MIR_EXTRACT_LIST_TAIL, 30,
                     MIR_NO_VALUE, container0, 1);
  ok &= test_extract("extract.array_succ", MIR_EXTRACT_ARRAY_SUCC, 30,
                     MIR_NO_VALUE, container0, 1);

  ExpectedOperand value0[] = {
      {30, MIR_OPERAND_ROLE_VALUE, MIR_OPERAND_USE_BORROW, 0},
  };
  ok &= test_extract("extract.variant_tag", MIR_EXTRACT_VARIANT_TAG, 30,
                     MIR_NO_VALUE, value0, 1);
  ok &= test_extract("extract.closure_fn", MIR_EXTRACT_CLOSURE_FN, 30,
                     MIR_NO_VALUE, value0, 1);
  ok &= test_extract("extract.closure_env", MIR_EXTRACT_CLOSURE_ENV, 30,
                     MIR_NO_VALUE, value0, 1);

  ExpectedOperand array_at[] = {
      {30, MIR_OPERAND_ROLE_CONTAINER, MIR_OPERAND_USE_BORROW, 0},
      {31, MIR_OPERAND_ROLE_INDEX, MIR_OPERAND_USE_BORROW, 1},
  };
  ok &= test_extract("extract.array_at", MIR_EXTRACT_ARRAY_AT, 30, 31,
                     array_at, 2);

  ExpectedOperand array_offset[] = {
      {31, MIR_OPERAND_ROLE_INDEX, MIR_OPERAND_USE_BORROW, 0},
      {30, MIR_OPERAND_ROLE_CONTAINER, MIR_OPERAND_USE_BORROW, 1},
  };
  ok &= test_extract("extract.array_offset", MIR_EXTRACT_ARRAY_OFFSET, 30, 31,
                     array_offset, 2);
  ok &= test_coro_next();
  ok &= test_coro_reset();

  if (!ok) {
    return 1;
  }

  puts("ok mir operand metadata");
  return 0;
}
