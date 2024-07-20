#include "../lang/backend_llvm/common.h"
#include "../lang/parse.h"
#include "codegen_list.h"
#include "codegen_match_values.h"
#include "codegen_types.h"
#include "util.h"
#include "value.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Codegen {
  JITLangCtx ctx;
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMValueRef func;
} Codegen;

Codegen setup_codegen(ht *stack, LLVMTypeRef ret_type) {
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

  LLVMTypeRef funcType = LLVMFunctionType(ret_type, NULL, 0, 0);

  // Create function.
  LLVMValueRef func = LLVMAddFunction(module, "tmp", funcType);

  LLVMSetLinkage(func, LLVMExternalLinkage);

  // Create basic block.
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  JITLangCtx ctx = {.stack = stack, .stack_ptr = 0, env};
  return (Codegen){ctx, context, module, builder, func};
}

void cleanup_codegen(Codegen env) {
  LLVMDisposeModule(env.module);
  LLVMContextDispose(env.context);
}

int _prepare_ex_engine(LLVMExecutionEngineRef *engine, LLVMModuleRef module) {
  char *error = NULL;

  struct LLVMMCJITCompilerOptions *Options =
      malloc(sizeof(struct LLVMMCJITCompilerOptions));
  Options->OptLevel = 2;

  if (LLVMCreateMCJITCompilerForModule(engine, module, Options, 1, &error) !=
      0) {
    fprintf(stderr, "Failed to create execution engine: %s\n", error);
    LLVMDisposeMessage(error);
    return 1;
  }
}

#define _TRUE LLVMConstInt(LLVMInt1Type(), 1, 0)
#define _FALSE LLVMConstInt(LLVMInt1Type(), 0, 0)

LLVMGenericValueRef ex_test_func(Codegen env, LLVMValueRef code) {
  LLVMBuildRet(env.builder, code);
  LLVMExecutionEngineRef engine;
  _prepare_ex_engine(&engine, env.module);
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result = LLVMRunFunction(engine, env.func, 0, exec_args);
}

// Test function
bool test_match_underscore() {
  const char *desc = "match 42 with _ / let _ = 42";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt1Type());

  // Create a dummy LLVMValueRef for testing
  LLVMValueRef match_val = LLVMConstInt(LLVMInt32Type(), 42, 0);

  // Test case 1: AST_IDENTIFIER with '_'
  Ast ast_underscore = {.tag = AST_IDENTIFIER,
                        .data = {.AST_IDENTIFIER = {.value = "_"}},
                        .md = &t_int};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(&ast_underscore, match_val, &t_int, &res,
                                       &env.ctx, env.module, env.builder);
  LLVMBuildRet(env.builder, exp_true);
  LLVMExecutionEngineRef engine;
  _prepare_ex_engine(&engine, env.module);
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result = LLVMRunFunction(engine, env.func, 0, exec_args);
  bool test_res = (int)LLVMGenericValueToInt(result, 0) == 1;

  // bool test_res = (LLVMConstIntGetZExtValue(exp_true) == 1 &&
  //                  LLVMConstIntGetZExtValue(res) == 1);

  if (test_res) {
    printf("✅ %s (%d)\n", desc, (int)LLVMGenericValueToInt(result, 0));
  } else {
    printf("❌ %s (%d)\n", desc, (int)LLVMGenericValueToInt(result, 0));
  };

  cleanup_codegen(env);
  return test_res;
}
bool test_match_value() {
  const char *desc = "match 42 with 42";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt1Type());
  env.ctx.stack_ptr = 1;

  // Create a dummy LLVMValueRef for testing
  LLVMValueRef match_val = LLVMConstInt(LLVMInt32Type(), 42, 0);

  // Test case 1: AST_IDENTIFIER with '_'
  Ast ast = {.tag = AST_INT, .data = {.AST_INT = {.value = 42}}, .md = &t_int};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(&ast, match_val, &t_int, &res, &env.ctx,
                                       env.module, env.builder);

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
  const char *desc = "match 42 with 41 (fails)";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt1Type());
  env.ctx.stack_ptr = 1;

  // Create a dummy LLVMValueRef for testing
  LLVMValueRef match_val = LLVMConstInt(LLVMInt32Type(), 42, 0);

  // Test case 1: AST_IDENTIFIER with '_'
  Ast ast = {.tag = AST_INT, .data = {.AST_INT = {.value = 41}}, .md = &t_int};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(&ast, match_val, &t_int, &res, &env.ctx,
                                       env.module, env.builder);

  LLVMBuildRet(env.builder, exp_true);
  LLVMExecutionEngineRef engine;
  _prepare_ex_engine(&engine, env.module);
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result = LLVMRunFunction(engine, env.func, 0, exec_args);
  bool test_res = (int)LLVMGenericValueToInt(result, 0) == 0;

  // bool test_res = (LLVMConstIntGetZExtValue(exp_true) == 1 &&
  //                  LLVMConstIntGetZExtValue(res) == 1);

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
  Codegen env = setup_codegen(stack, LLVMInt32Type());
  env.ctx.stack_ptr = 1;

  // Create a dummy LLVMValueRef for testing
  LLVMValueRef match_val = LLVMConstInt(LLVMInt32Type(), 42, 0);

  // Test case 1: AST_IDENTIFIER with '_'
  Ast ast_underscore = {.tag = AST_IDENTIFIER,
                        .data = {.AST_IDENTIFIER = {.value = "x", .length = 1}},
                        .md = &t_int};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(&ast_underscore, match_val, &t_int, &res,
                                       &env.ctx, env.module, env.builder);

  bool test_res = true;
  JITSymbol *sym =
      ht_get_hash(env.ctx.stack + env.ctx.stack_ptr, "x", hash_string("x", 1));

  test_res &= sym != NULL;

  if (sym) {
    LLVMBuildRet(env.builder, sym->val);
    LLVMExecutionEngineRef engine;
    _prepare_ex_engine(&engine, env.module);
    LLVMGenericValueRef exec_args[] = {};
    LLVMGenericValueRef result =
        LLVMRunFunction(engine, env.func, 0, exec_args);

    test_res &= ((int)LLVMGenericValueToInt(result, 0) == 42);
  }

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}
LLVMValueRef get_glob_val(LLVMBuilderRef builder, LLVMValueRef alloca,
                          LLVMTypeRef type) {
  return LLVMBuildLoad2(builder, type, alloca, "");
}

bool test_match_assignment_top_level() {
  const char *desc = "top level: match 42 with x / let x = 42";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt32Type());

  // Create a dummy LLVMValueRef for testing
  LLVMValueRef match_val = LLVMConstInt(LLVMInt32Type(), 42, 0);

  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_underscore = {
      .tag = AST_IDENTIFIER,
      .data = {.AST_IDENTIFIER = {.value = "x", .length = 1}}};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(&ast_underscore, match_val, &t_int, &res,
                                       &env.ctx, env.module, env.builder);

  bool test_res = true;

  JITSymbol *sym =
      ht_get_hash(env.ctx.stack + env.ctx.stack_ptr, "x", hash_string("x", 1));

  test_res &= sym != NULL;
  if (sym) {

    LLVMBuildRet(env.builder,
                 get_glob_val(env.builder, sym->val, LLVMInt32Type()));
    LLVMExecutionEngineRef engine;
    _prepare_ex_engine(&engine, env.module);
    LLVMGenericValueRef exec_args[] = {};
    LLVMGenericValueRef result =
        LLVMRunFunction(engine, env.func, 0, exec_args);

    test_res &= ((int)LLVMGenericValueToInt(result, 0) == 42);
  }

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

LLVMValueRef int_list(int items[], int len, LLVMBuilderRef builder) {
  LLVMTypeRef llvm_el_type = LLVMInt32Type();
  LLVMTypeRef node_type = llnode_type(llvm_el_type);

  if (len == 0) {
    return null_node(node_type);
  }

  LLVMValueRef end_node = null_node(node_type);

  LLVMValueRef head = LLVMBuildArrayMalloc(
      builder, node_type, LLVMConstInt(LLVMInt32Type(), len, 0),
      "list_array_malloc"); // malloc an array all at once since we know we'll
                            // need len nodes off the bat
  LLVMValueRef tail = head;

  LLVMValueRef element_size = LLVMSizeOf(node_type);
  for (int i = 0; i < len; i++) {

    // Set the data
    struct_ptr_set(0, tail, node_type,
                   LLVMConstInt(LLVMInt32Type(), *(items + i), true), builder);

    if (i < len - 1) {
      LLVMValueRef next_tail =
          increment_ptr(tail, node_type, element_size, builder);
      struct_ptr_set(1, tail, node_type, next_tail, builder);

      tail = next_tail;
    } else {
      // Set the final next pointer to null
      struct_ptr_set(1, tail, node_type, end_node, builder);
    }
  }

  return head;
}

LLVMValueRef int_tuple(Type *type, int items[], int len,
                       LLVMBuilderRef builder) {

  LLVMTypeRef tuple_type = type_to_llvm_type(type, NULL);
  LLVMValueRef tuple = LLVMGetUndef(tuple_type);

  for (int i = 0; i < len; i++) {
    LLVMValueRef tuple_element = LLVMConstInt(LLVMInt32Type(), items[i], 0);
    tuple = LLVMBuildInsertValue(builder, tuple, tuple_element, i, "");
  }
  return tuple;
}

bool test_match_list() {
  const char *desc = "match [1, 2, 3] with [1, 2, 3]";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt1Type());

  Type *list_type = tcons("List", (Type *[]){&t_int}, 1);
  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_list = {
      AST_LIST,
      .data = {.AST_LIST = {.items =
                                (Ast[]){
                                    (Ast){AST_INT,
                                          .data = {.AST_INT = {.value = 1}},
                                          .md = &t_int},
                                    (Ast){AST_INT,
                                          .data = {.AST_INT = {.value = 2}},
                                          .md = &t_int},
                                    (Ast){AST_INT, .data = {.AST_INT = {.value = 3}},
                                          .md = &t_int},
                                },
                            .len = 3}},
      .md = list_type};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true =
      match_values(&ast_list, int_list((int[]){1, 2, 3}, 3, env.builder),
                   list_type, &res, &env.ctx, env.module, env.builder);

  LLVMBuildRet(env.builder, exp_true);
  LLVMExecutionEngineRef engine;
  _prepare_ex_engine(&engine, env.module);
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result = LLVMRunFunction(engine, env.func, 0, exec_args);
  bool test_res = (int)LLVMGenericValueToInt(result, 0) == 1;

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    LLVMDumpModule(env.module);
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_list_fail() {
  const char *desc = "match [1, 2, 3] with [2, 2, 3] (fails)";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt1Type());

  Type *list_type = tcons("List", (Type *[]){&t_int}, 1);
  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_list = {
      AST_LIST,
      .data = {.AST_LIST = {.items =
                                (Ast[]){
                                    (Ast){AST_INT,
                                          .data = {.AST_INT = {.value = 2}},
                                          .md = &t_int},
                                    (Ast){AST_INT,
                                          .data = {.AST_INT = {.value = 2}},
                                          .md = &t_int},
                                    (Ast){AST_INT, .data = {.AST_INT = {.value = 3}},
                                          .md = &t_int},
                                },
                            .len = 3}},
      .md = list_type};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true =
      match_values(&ast_list, int_list((int[]){1, 2, 3}, 3, env.builder),
                   list_type, &res, &env.ctx, env.module, env.builder);

  LLVMBuildRet(env.builder, exp_true);
  LLVMExecutionEngineRef engine;
  _prepare_ex_engine(&engine, env.module);
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result = LLVMRunFunction(engine, env.func, 0, exec_args);
  bool test_res = (int)LLVMGenericValueToInt(result, 0) == 0;

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    LLVMDumpModule(env.module);
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_list_assignment() {
  const char *desc = "match [42, 2, 3] with [x, 2, 3]";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt32Type());
  env.ctx.stack_ptr = 1;
  Type *list_type = tcons("List", (Type *[]){&t_int}, 1);

  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_list = {
      AST_LIST,
      .data = {.AST_LIST = {.items =
                                (Ast[]){
                                    (Ast){AST_IDENTIFIER,
                                          .data =
                                              {.AST_IDENTIFIER = {.value = "x",
                                                                  .length = 1}},
                                          .md = &t_int},
                                    (Ast){AST_INT, .data = {.AST_INT = {.value = 2}},
                                          .md = &t_int},
                                    (Ast){AST_INT, .data = {.AST_INT = {.value = 3}}, .md = &t_int},
                                },
                            .len = 3}},
      .md = list_type};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true =
      match_values(&ast_list, int_list((int[]){42, 2, 3}, 3, env.builder),
                   list_type, &res, &env.ctx, env.module, env.builder);

  JITSymbol *sym =
      ht_get_hash(env.ctx.stack + env.ctx.stack_ptr, "x", hash_string("x", 1));
  bool test_res = true;
  test_res &= sym != NULL;

  if (sym) {
    LLVMBuildRet(env.builder, sym->val);
    LLVMExecutionEngineRef engine;
    _prepare_ex_engine(&engine, env.module);
    LLVMGenericValueRef exec_args[] = {};
    LLVMGenericValueRef result =
        LLVMRunFunction(engine, env.func, 0, exec_args);

    test_res &= ((int)LLVMGenericValueToInt(result, 0) == 42);
  }

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_list_assignment_destructure_success() {
  const char *desc = "match [42, 2, 3] with x::rest -- success";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt1Type());
  env.ctx.stack_ptr = 1;

  Type *list_type = tcons("List", (Type *[]){&t_int}, 1);
  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_list = {AST_BINOP,
                  .data = {.AST_BINOP =
                               {
                                   .op = TOKEN_DOUBLE_COLON,
                                   .left = &(Ast){AST_IDENTIFIER,
                                                  .data = {.AST_IDENTIFIER =
                                                               {.value = "x",
                                                                .length = 1}},
                                                  .md = &t_int},
                                   .right =

                                       &(Ast){AST_IDENTIFIER,
                                              .data = {.AST_IDENTIFIER = {.value =
                                                                              "rest",
                                                                          .length =
                                                                              4}},
                                              .md = &t_int},
                               }},
                  .md = list_type};

  LLVMValueRef res = _TRUE;
  LLVMValueRef match =
      match_values(&ast_list, int_list((int[]){42, 2, 3}, 3, env.builder),
                   list_type, &res, &env.ctx, env.module, env.builder);

  bool test_res = true;

  LLVMBuildRet(env.builder, match);
  LLVMExecutionEngineRef engine;
  _prepare_ex_engine(&engine, env.module);
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result = LLVMRunFunction(engine, env.func, 0, exec_args);

  test_res &= ((int)LLVMGenericValueToInt(result, 0) == 1);

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    LLVMDumpModule(env.module);
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_list_assignment_destructure() {
  const char *desc = "match [42, 2, 3] with x::rest -- x == 42";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt32Type());
  env.ctx.stack_ptr = 1;
  Type *list_type = tcons("List", (Type *[]){&t_int}, 1);
  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_list = {AST_BINOP,
                  .data = {.AST_BINOP =
                               {
                                   .op = TOKEN_DOUBLE_COLON,
                                   .left = &(Ast){AST_IDENTIFIER,
                                                  .data = {.AST_IDENTIFIER =
                                                               {.value = "x",
                                                                .length = 1}},
                                                  .md = &t_int},
                                   .right =

                                       &(Ast){AST_IDENTIFIER,
                                              .data = {.AST_IDENTIFIER = {.value =
                                                                              "rest",
                                                                          .length =
                                                                              4}},
                                              .md = &t_int},
                               }},
                  .md = list_type};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true =
      match_values(&ast_list, int_list((int[]){42, 2, 3}, 3, env.builder),
                   list_type, &res, &env.ctx, env.module, env.builder);

  JITSymbol *sym =
      ht_get_hash(env.ctx.stack + env.ctx.stack_ptr, "x", hash_string("x", 1));
  bool test_res = true;
  test_res &= sym != NULL;

  JITSymbol *sym_rest = ht_get_hash(env.ctx.stack + env.ctx.stack_ptr, "rest",
                                    hash_string("rest", 4));
  test_res &= sym_rest != NULL;

  if (sym) {
    LLVMBuildRet(env.builder, sym->val);
    LLVMExecutionEngineRef engine;
    _prepare_ex_engine(&engine, env.module);
    LLVMGenericValueRef exec_args[] = {};
    LLVMGenericValueRef result =
        LLVMRunFunction(engine, env.func, 0, exec_args);

    test_res &= ((int)LLVMGenericValueToInt(result, 0) == 42);
  }

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    LLVMDumpModule(env.module);
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

typedef struct ll_int_t {
  int32_t el;
  struct ll_int_t *next;
} ll_int_t;

bool test_match_list_assignment_destructure2() {
  const char *desc = "match [42, 2, 3] with x::rest -- rest == [2,3]";
  ht stack[STACK_MAX];
  Type *list_type = tcons("List", (Type *[]){&t_int}, 1);
  Codegen env = setup_codegen(stack, type_to_llvm_type(list_type, NULL));
  env.ctx.stack_ptr = 1;

  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_list = {AST_BINOP,
                  .data = {.AST_BINOP =
                               {
                                   .op = TOKEN_DOUBLE_COLON,
                                   .left = &(Ast){AST_IDENTIFIER,
                                                  .data = {.AST_IDENTIFIER =
                                                               {.value = "x",
                                                                .length = 1}},
                                                  .md = &t_int},
                                   .right =

                                       &(Ast){AST_IDENTIFIER,
                                              .data = {.AST_IDENTIFIER = {.value =
                                                                              "rest",
                                                                          .length =
                                                                              4}},
                                              .md = &t_int},
                               }},
                  .md = list_type};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true =
      match_values(&ast_list, int_list((int[]){42, 2, 3}, 3, env.builder),
                   list_type, &res, &env.ctx, env.module, env.builder);

  JITSymbol *sym = ht_get_hash(env.ctx.stack + env.ctx.stack_ptr, "rest",
                               hash_string("rest", 4));
  bool test_res = true;
  test_res &= sym != NULL;

  if (sym) {
    LLVMBuildRet(env.builder, sym->val);
    LLVMExecutionEngineRef engine;
    _prepare_ex_engine(&engine, env.module);
    LLVMGenericValueRef exec_args[] = {};
    LLVMGenericValueRef result =
        LLVMRunFunction(engine, env.func, 0, exec_args);

    ll_int_t *current = (ll_int_t *)LLVMGenericValueToPointer(result);
    test_res &= current->el == 2;
    test_res &= current->next->el == 3;
    test_res &= current->next->next == NULL;
  }

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_tuple() {
  const char *desc = "match (1, 2, 3) with (1, 2, 3)";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt1Type());
  Type *tuple_type = tcons("Tuple", (Type *[]){&t_int, &t_int, &t_int}, 3);

  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_list = {
      AST_TUPLE,
      .data = {.AST_LIST = {.items =
                                (Ast[]){
                                    (Ast){AST_INT,
                                          .data = {.AST_INT = {.value = 1}},
                                          .md = &t_int},
                                    (Ast){AST_INT,
                                          .data = {.AST_INT = {.value = 2}},
                                          .md = &t_int},
                                    (Ast){AST_INT, .data = {.AST_INT = {.value = 3}},
                                          .md = &t_int},
                                },
                            .len = 3}},
      .md = tuple_type};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(
      &ast_list, int_tuple(tuple_type, (int[]){1, 2, 3}, 3, env.builder),
      tuple_type, &res, &env.ctx, env.module, env.builder);

  LLVMBuildRet(env.builder, exp_true);
  LLVMExecutionEngineRef engine;
  _prepare_ex_engine(&engine, env.module);
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result = LLVMRunFunction(engine, env.func, 0, exec_args);
  bool test_res = (int)LLVMGenericValueToInt(result, 0) == 1;

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    LLVMDumpModule(env.module);
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_tuple_fail() {
  const char *desc = "match (1, 2, 3) with (2, 2, 3) (fails)";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt1Type());
  Type *tuple_type = tcons("Tuple", (Type *[]){&t_int, &t_int, &t_int}, 3);

  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_list = {
      AST_TUPLE,
      .data = {.AST_LIST = {.items =
                                (Ast[]){
                                    (Ast){AST_INT,
                                          .data = {.AST_INT = {.value = 2}},
                                          .md = &t_int},
                                    (Ast){AST_INT,
                                          .data = {.AST_INT = {.value = 2}},
                                          .md = &t_int},
                                    (Ast){AST_INT, .data = {.AST_INT = {.value = 3}},
                                          .md = &t_int},
                                },
                            .len = 3}},
      .md = tuple_type};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(
      &ast_list, int_tuple(tuple_type, (int[]){1, 2, 3}, 3, env.builder),
      tuple_type, &res, &env.ctx, env.module, env.builder);

  LLVMBuildRet(env.builder, exp_true);
  LLVMExecutionEngineRef engine;
  _prepare_ex_engine(&engine, env.module);
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result = LLVMRunFunction(engine, env.func, 0, exec_args);
  bool test_res = (int)LLVMGenericValueToInt(result, 0) == 1;

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    LLVMDumpModule(env.module);
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_tuple_assignment() {
  const char *desc = "match (42, 2, 3) with (x, 2, 3) -- x == 42";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt1Type());
  env.ctx.stack_ptr = 1;
  Type *tuple_type = tcons("Tuple", (Type *[]){&t_int, &t_int, &t_int}, 3);

  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_list = {
      AST_TUPLE,
      .data = {.AST_LIST = {.items =
                                (Ast[]){
                                    (Ast){AST_IDENTIFIER,
                                          .data =
                                              {.AST_IDENTIFIER = {.value = "x",
                                                                  .length = 1}},
                                          .md = &t_int},
                                    (Ast){AST_INT, .data = {.AST_INT = {.value = 2}},
                                          .md = &t_int},
                                    (Ast){AST_INT, .data = {.AST_INT = {.value = 3}}, .md = &t_int},
                                },
                            .len = 3}},
      .md = tuple_type};

  LLVMValueRef res = _TRUE;
  LLVMValueRef exp_true = match_values(
      &ast_list, int_tuple(tuple_type, (int[]){42, 2, 3}, 3, env.builder),
      tuple_type, &res, &env.ctx, env.module, env.builder);

  JITSymbol *sym =
      ht_get_hash(env.ctx.stack + env.ctx.stack_ptr, "x", hash_string("x", 1));
  bool test_res = true;
  test_res &= sym != NULL;

  if (sym) {
    LLVMBuildRet(env.builder, sym->val);
    LLVMExecutionEngineRef engine;
    _prepare_ex_engine(&engine, env.module);
    LLVMGenericValueRef exec_args[] = {};
    LLVMGenericValueRef result =
        LLVMRunFunction(engine, env.func, 0, exec_args);

    test_res &= ((int)LLVMGenericValueToInt(result, 0) == 42);
  }

  if (test_res) {
    printf("✅ %s\n", desc);
  } else {
    LLVMDumpModule(env.module);
    printf("❌ %s\n", desc);
  };

  cleanup_codegen(env);
  return test_res;
}

bool test_match_list_assignment_destructure_success_global_scope() {
  const char *desc = "match [42, 2, 3] with x::rest -- success";
  ht stack[STACK_MAX];
  Codegen env = setup_codegen(stack, LLVMInt1Type());
  env.ctx.stack_ptr = 0;
  Type *int_list_type = tcons("List", (Type *[]){&t_int}, 1);

  // Test case : AST_IDENTIFIER with 'x'
  Ast ast_list = {AST_BINOP,
                  .data = {.AST_BINOP =
                               {
                                   .op = TOKEN_DOUBLE_COLON,
                                   .left = &(Ast){AST_IDENTIFIER,
                                                  .data = {.AST_IDENTIFIER =
                                                               {.value = "_",
                                                                .length = 1}},
                                                  .md = &t_int},
                                   .right =

                                       &(Ast){AST_IDENTIFIER,
                                              .data = {.AST_IDENTIFIER = {.value =
                                                                              "rest",
                                                                          .length =
                                                                              4}},
                                              .md = int_list_type},
                               }},
                  .md = int_list_type};

  LLVMValueRef res = _TRUE;
  LLVMValueRef match =
      match_values(&ast_list, int_list((int[]){42, 2, 3}, 3, env.builder),
                   int_list_type, &res, &env.ctx, env.module, env.builder);

  bool test_res = true;

  LLVMBuildRet(env.builder, match);
  LLVMExecutionEngineRef engine;
  _prepare_ex_engine(&engine, env.module);
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result = LLVMRunFunction(engine, env.func, 0, exec_args);

  test_res &= ((int)LLVMGenericValueToInt(result, 0) == 1);

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

  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  LLVMLinkInMCJIT();
  status &= test_match_underscore();
  status &= test_match_value();
  status &= test_match_assignment();
  status &= test_match_value_fail();
  status &= test_match_assignment_top_level();
  status &= test_match_list();
  status &= test_match_list_fail();
  status &= test_match_list_assignment();
  status &= test_match_list_assignment_destructure();
  status &= test_match_list_assignment_destructure_success();
  status &= test_match_list_assignment_destructure2();
  status &= test_match_tuple();
  status &= test_match_tuple_fail();
  status &= test_match_tuple_assignment();
  status &= test_match_list_assignment_destructure_success_global_scope();
  return status != true;
}
