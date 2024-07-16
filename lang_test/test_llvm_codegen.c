#include "../lang/backend_llvm/codegen_match.h"
#include "../lang/backend_llvm/common.h"
#include "../lang/parse.h"
#include "value.h"
#include <llvm-c/Core.h>
#include <stdio.h>
#include <string.h>

typedef struct Codegen {
  JITLangCtx ctx;
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
} Codegen;

Codegen setup_codegen(ht *stack) {
  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module =
      LLVMModuleCreateWithNameInContext("test_module", context);

  LLVMBuilderRef builder =
      LLVMCreateBuilderInContext(LLVMGetModuleContext(module));

  // shared type env
  TypeEnv *env = NULL;

  for (int i = 0; i < STACK_MAX; i++) {
    ht_init(&stack[i]);
  }
  JITLangCtx ctx = {.stack = stack, .stack_ptr = 0, env};
  return (Codegen){ctx, context, module, builder};
}

void cleanup_codegen(Codegen env) {
  LLVMDisposeModule(env.module);
  LLVMContextDispose(env.context);
}

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)
#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

// Test function
bool test_match_underscore() {
  const char *desc = "match 42 with _ / let _ = 42";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack);

  // Create a dummy LLVMValueRef for testing
  LLVMValueRef match_val = LLVMConstInt(LLVMInt32Type(), 42, 0);

  // Test case 1: AST_IDENTIFIER with '_'
  Ast ast_underscore = {.tag = AST_IDENTIFIER,
                        .data = {.AST_IDENTIFIER = {.value = "_"}},
                        .md = &t_int};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(&ast_underscore, match_val, &res,
                                       &env.ctx, env.module, env.builder);

  bool test_res = (LLVMConstIntGetZExtValue(exp_true) == 1 &&
                   LLVMConstIntGetZExtValue(res) == 1);

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
#undef desc
  return test_res;
}
bool test_match_value() {
  const char *desc = "match 42 with 42";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack);
  env.ctx.stack_ptr = 1;

  // Create a dummy LLVMValueRef for testing
  LLVMValueRef match_val = LLVMConstInt(LLVMInt32Type(), 42, 0);

  // Test case 1: AST_IDENTIFIER with '_'
  Ast ast = {.tag = AST_INT, .data = {.AST_INT = {.value = 42}}, .md = &t_int};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true =
      match_values(&ast, match_val, &res, &env.ctx, env.module, env.builder);

  bool test_res = (LLVMConstIntGetZExtValue(exp_true) == 1 &&
                   LLVMConstIntGetZExtValue(res) == 1);
  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_value_fail() {
  const char *desc = "match 42 with 41";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack);
  env.ctx.stack_ptr = 1;

  // Create a dummy LLVMValueRef for testing
  LLVMValueRef match_val = LLVMConstInt(LLVMInt32Type(), 42, 0);

  // Test case 1: AST_IDENTIFIER with '_'
  Ast ast = {.tag = AST_INT, .data = {.AST_INT = {.value = 41}}, .md = &t_int};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true =
      match_values(&ast, match_val, &res, &env.ctx, env.module, env.builder);

  bool test_res = (LLVMConstIntGetZExtValue(exp_true) == 0 &&
                   LLVMConstIntGetZExtValue(res) == 0);
  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_assignment() {
  const char *desc = "match 42 with x / let x = 42";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack);
  env.ctx.stack_ptr = 1;

  // Create a dummy LLVMValueRef for testing
  LLVMValueRef match_val = LLVMConstInt(LLVMInt32Type(), 42, 0);

  // Test case 1: AST_IDENTIFIER with '_'
  Ast ast_underscore = {.tag = AST_IDENTIFIER,
                        .data = {.AST_IDENTIFIER = {.value = "x", .length = 1}},
                        .md = &t_int};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(&ast_underscore, match_val, &res,
                                       &env.ctx, env.module, env.builder);

  bool test_res = (LLVMConstIntGetZExtValue(exp_true) == 1 &&
                   LLVMConstIntGetZExtValue(res) == 1);
  JITSymbol *sym =
      ht_get_hash(env.ctx.stack + env.ctx.stack_ptr, "x", hash_string("x", 1));
  test_res &= sym != NULL;
  if (sym) {
    test_res &= LLVMConstIntGetZExtValue(sym->val) == 42;
  }

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_assignment_top_level() {
  const char *desc = "top level: match 42 with x / let x = 42";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack);

  // Create a dummy LLVMValueRef for testing
  LLVMValueRef match_val = LLVMConstInt(LLVMInt32Type(), 42, 0);

  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_underscore = {
      .tag = AST_IDENTIFIER,
      .data = {.AST_IDENTIFIER = {.value = "x", .length = 1}}};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(&ast_underscore, match_val, &res,
                                       &env.ctx, env.module, env.builder);

  bool test_res = (LLVMConstIntGetZExtValue(exp_true) == 1 &&
                   LLVMConstIntGetZExtValue(res) == 1);

  JITSymbol *sym =
      ht_get_hash(env.ctx.stack + env.ctx.stack_ptr, "x", hash_string("x", 1));
  test_res &= sym != NULL;
  if (sym) {
    test_res &= LLVMConstIntGetZExtValue(LLVMGetInitializer(sym->val)) == 42;
  }

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

int main() {
  bool status = true;

#define TEST_MATCH_ASSIGNMENT(desc, ast_input, match_value,                    \
                              additional_conditions)                           \
  bool status = true;
  status &= test_match_underscore();
  status &= test_match_value();
  status &= test_match_value_fail();
  status &= test_match_assignment();
  status &= test_match_assignment_top_level();
  return status != true;
}
