#include "./testing.h"
#include "codegen.h"
#include "jit.h"
#include "serde.h"
#include "symbols.h"
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include <string.h>

// Function prototype
void _report_test_result(const char *name, int result);

void _report_test_totals(int num_passes, int num_tests);

// Helper function to create string constant properly
static LLVMValueRef create_string_constant(LLVMBuilderRef builder,
                                           LLVMModuleRef module,
                                           const char *str) {
  // Create global string constant
  LLVMValueRef global_str = LLVMBuildGlobalStringPtr(builder, str, "test_name");
  return global_str;
}

// Helper to create test reporting function declaration
static LLVMValueRef create_report_function(LLVMModuleRef module) {
  LLVMTypeRef param_types[] = {
      LLVMPointerType(LLVMInt8Type(), 0), // char*
      LLVMInt1Type()                      // bool/int1
  };

  LLVMTypeRef report_func_type =
      LLVMFunctionType(LLVMVoidType(), // return type
                       param_types,    // parameter types
                       2,              // parameter count
                       0               // not variadic
      );

  return LLVMAddFunction(module, "_report_test_result", report_func_type);
}

// Helper to create test reporting function declaration
static LLVMValueRef create_totals_function(LLVMModuleRef module) {
  LLVMTypeRef param_types[] = {
      LLVMInt32Type(), // bool/int1
      LLVMInt32Type()  // bool/int1
  };

  LLVMTypeRef report_func_type =
      LLVMFunctionType(LLVMVoidType(), // return type
                       param_types,    // parameter types
                       2,              // parameter count
                       0               // not variadic
      );

  return LLVMAddFunction(module, "_report_test_totals", report_func_type);
}

Ast *get_test_module_ast(Ast *ast) {
  if (ast->tag == AST_LET && ast->data.AST_LET.binding->tag == AST_IDENTIFIER &&
      strcmp(ast->data.AST_LET.binding->data.AST_IDENTIFIER.value, "test") ==
          0) {
    return ast->data.AST_LET.expr;
  }
  if (ast->tag == AST_BODY) {
    for (int i = 0; i < ast->data.AST_BODY.len; i++) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];

      if (stmt->tag == AST_LET &&
          stmt->data.AST_LET.binding->tag == AST_IDENTIFIER &&
          strcmp(stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value,
                 "test") == 0) {
        return stmt->data.AST_LET.expr;
      }
    }
  }

  return NULL;
}
LLVMValueRef codegen_test_module(Ast *ast, JITLangCtx *ctx,
                                 LLVMModuleRef module, LLVMBuilderRef builder) {
  Ast *test_module_ast = get_test_module_ast(ast);
  if (!test_module_ast) {
    fprintf(stderr,
            "Error: could not find test module of module under test %s\n",
            ctx->module_name);
  }

  LLVMTypeRef ret_type = LLVMInt1Type();
  LLVMTypeRef funcType = LLVMFunctionType(ret_type, NULL, 0, 0);
  LLVMValueRef func = LLVMAddFunction(module, "top", funcType);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef body = codegen(ast, ctx, module, builder);
  if (body == NULL) {
    fprintf(stderr,
            "Error: test runner could not compile module under test %s\n",
            ctx->module_name);
    LLVMDeleteFunction(func);
    return NULL;
  }

  LLVMValueRef test_result = LLVMConstInt(LLVMInt1Type(), 1, 0);
  LLVMValueRef report_func = create_report_function(module);
  LLVMValueRef totals_func = create_totals_function(module);
  LLVMTypeRef test_func_type = LLVMFunctionType(LLVMInt1Type(), NULL, 0, 0);

  // Initialize counters
  LLVMValueRef num_tests = LLVMConstInt(LLVMInt32Type(), 0, 0);
  LLVMValueRef num_passes = LLVMConstInt(LLVMInt32Type(), 0, 0);

  JITSymbol *test_module =
      ht_get_hash(ctx->frame->table, "test", hash_string("test", 4));

  for (int i = 0; i < test_module_ast->data.AST_LAMBDA.body->data.AST_BODY.len;
       i++) {
    Ast *stmt = test_module_ast->data.AST_LAMBDA.body->data.AST_BODY.stmts[i];

    if (stmt->tag == AST_LET) {
      Ast *binding = stmt->data.AST_LET.binding;
      if (!((binding->tag == AST_IDENTIFIER) &&
            (strncmp(binding->data.AST_IDENTIFIER.value, "test", 4) == 0))) {
        continue;
      }

      if (stmt->data.AST_LET.expr->tag == AST_LAMBDA) {
        const char *key = stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value;

        JITSymbol *sym = find_in_ctx(key, strlen(key),
                                     test_module->symbol_data.STYPE_MODULE.ctx);

        // Increment num_tests
        num_tests =
            LLVMBuildAdd(builder, num_tests,
                         LLVMConstInt(LLVMInt32Type(), 1, 0), "num_tests");

        // Call the test function
        LLVMValueRef test_call = LLVMBuildCall2(builder, test_func_type,
                                                sym->val, NULL, 0, "test_call");

        // Increment num_passes if test passed
        LLVMValueRef should_increment = LLVMBuildZExt(
            builder, test_call, LLVMInt32Type(), "should_increment");

        num_passes =
            LLVMBuildAdd(builder, num_passes, should_increment, "num_passes");

        LLVMValueRef name_str = create_string_constant(builder, module, key);

        LLVMValueRef report_args[] = {name_str, test_call};
        LLVMBuildCall2(builder, LLVMGlobalGetValueType(report_func),
                       report_func, report_args, 2, "");

        test_result =
            LLVMBuildAnd(builder, test_result, test_call, "test_result");
      } else if (types_equal(stmt->data.AST_LET.expr->md, &t_bool)) {
        const char *key = stmt->data.AST_LET.binding->data.AST_IDENTIFIER.value;

        JITSymbol *sym = find_in_ctx(key, strlen(key),
                                     test_module->symbol_data.STYPE_MODULE.ctx);

        // Increment num_tests
        num_tests =
            LLVMBuildAdd(builder, num_tests,
                         LLVMConstInt(LLVMInt32Type(), 1, 0), "num_tests");

        // Call the test function
        LLVMValueRef val = sym->val;

        // Increment num_passes if test passed
        LLVMValueRef should_increment =
            LLVMBuildZExt(builder, val, LLVMInt32Type(), "should_increment");

        num_passes =
            LLVMBuildAdd(builder, num_passes, should_increment, "num_passes");

        LLVMValueRef name_str = create_string_constant(builder, module, key);

        LLVMValueRef report_args[] = {name_str, val};
        LLVMBuildCall2(builder, LLVMGlobalGetValueType(report_func),
                       report_func, report_args, 2, "");
      }
    }
  }

  LLVMBuildCall2(builder, LLVMGlobalGetValueType(totals_func), totals_func,
                 (LLVMValueRef[]){num_passes, num_tests}, 2, "");

  LLVMBuildRet(builder, test_result);
  return func;
}

int test_module(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                LLVMBuilderRef builder) {

  LLVMValueRef exec_tests = codegen_test_module(ast, ctx, module, builder);

  if (!exec_tests) {
    return 0;
  }

  LLVMExecutionEngineRef engine;
  if (prepare_ex_engine(ctx, &engine, module) != 0) {
    return 0;
  }

  LLVMGenericValueRef result = LLVMRunFunction(engine, exec_tests, 0, NULL);
  int int_res = (int)LLVMGenericValueToInt(result, 0);

  LLVMDisposeExecutionEngine(engine);
  return int_res;
}

void _report_test_result(const char *name, int result) {
  if (result) {
    printf("✅ %s\n", name);
  } else {
    printf("❌ %s\n", name);
  }
}

void _report_test_totals(int num_passes, int num_tests) {
  printf("%d / %d passed\n", num_passes, num_tests);
}
