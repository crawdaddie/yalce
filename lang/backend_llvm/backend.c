#include "backend_llvm/backend.h"
#include "backend_llvm/binop.h"
#include "backend_llvm/common.h"
#include "backend_llvm/function.h"
#include "backend_llvm/symbols.h"
#include "input.h"
#include "parse.h"
#include "serde.h"
#include "type_inference/infer.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_AST

static Ast *top_level_ast(Ast *body) {
  size_t len = body->data.AST_BODY.len;
  Ast *last = body->data.AST_BODY.stmts[len - 1];
  return last;
}

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder) {
  switch (ast->tag) {

  case AST_BODY: {
    LLVMValueRef val;
    for (size_t i = 0; i < ast->data.AST_BODY.len; ++i) {
      Ast *stmt = ast->data.AST_BODY.stmts[i];
      val = codegen(stmt, ctx, module, builder);
    }
    return val;
  }
  case AST_INT: {
    return LLVMConstInt(LLVMInt32Type(), ast->data.AST_INT.value, true);
  }

  case AST_NUMBER: {
    return LLVMConstReal(LLVMDoubleType(), ast->data.AST_INT.value);
  }

  case AST_STRING: {
    char *chars = ast->data.AST_STRING.value;
    int length = ast->data.AST_STRING.length;
    ObjString vstr = (ObjString){
        .chars = chars, .length = length, .hash = hash_string(chars, length)};
    return LLVMBuildGlobalStringPtr(builder, chars, ".str");
  }

  case AST_BOOL: {
    return LLVMConstInt(LLVMInt1Type(), ast->data.AST_BOOL.value, false);
  }
  case AST_BINOP: {
    return codegen_binop(ast, ctx, module, builder);
  }

  case AST_LET: {
    return codegen_assignment(ast, ctx, module, builder);
  }
  case AST_IDENTIFIER: {
    return codegen_identifier(ast, ctx, module, builder);
  }
  case AST_LAMBDA: {
    return codegen_lambda(ast, ctx, module, builder);
  }

  case AST_APPLICATION: {
    return codegen_fn_application(ast, ctx, module, builder);
  }
  }

  return NULL;
}

static LLVMValueRef codegen_top_level(Ast *ast, LLVMTypeRef *ret_type,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  // Create function type.
  LLVMTypeRef funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);

  // Create function.
  LLVMValueRef func = LLVMAddFunction(module, "tmp", funcType);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  if (func == NULL) {
    return NULL;
  }

  // Create basic block.
  LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  // Generate body.
  LLVMValueRef body = codegen(ast, ctx, module, builder);

  if (body == NULL) {
    LLVMDeleteFunction(func);
    return NULL;
  }

  *ret_type = LLVMTypeOf(body);
  LLVMBuildRet(builder, body);
  return func;
}

int prepare_ex_engine(LLVMExecutionEngineRef *engine, LLVMModuleRef module) {
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

static int eval_script(const char *filename, JITLangCtx *ctx,
                       LLVMModuleRef module, LLVMBuilderRef builder) {
  char *fcontent = read_script(filename);
  if (!fcontent) {
    return 1;
  }

  Ast *prog = parse_input(fcontent);
  print_ast(prog);
  printf("-----\n");
  LLVMTypeRef top_level_ret_type;

  LLVMValueRef top_level_func =
      codegen_top_level(prog, &top_level_ret_type, ctx, module, builder);

#ifdef DEBUG_AST
  LLVMDumpModule(module);
#endif

  LLVMExecutionEngineRef engine;
  prepare_ex_engine(&engine, module);

  if (top_level_func == NULL) {
    fprintf(stderr, "Unable to codegen for node\n");
    return 1;
  }
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result =
      LLVMRunFunction(engine, top_level_func, 0, exec_args);
  printf("> %d\n", (int)LLVMGenericValueToInt(result, 0));

  free(fcontent);
  return 0; // Return success
}

int jit(int argc, char **argv) {
  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  LLVMLinkInMCJIT();

  LLVMContextRef context = LLVMGetGlobalContext();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("ylc", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

  LLVMPassManagerRef pass_manager =
      LLVMCreateFunctionPassManagerForModule(module);

  LLVMAddPromoteMemoryToRegisterPass(pass_manager);
  LLVMAddInstructionCombiningPass(pass_manager);
  LLVMAddReassociatePass(pass_manager);
  LLVMAddGVNPass(pass_manager);
  LLVMAddCFGSimplificationPass(pass_manager);
  LLVMAddTailCallEliminationPass(pass_manager);

  ht stack[STACK_MAX];

  for (int i = 0; i < STACK_MAX; i++) {
    ht_init(&stack[i]);
  }

  // add_type_lookups(stack);
  llvm_add_native_functions(stack, module);
  // add_synth_functions(stack);

  JITLangCtx ctx = {
      .stack = stack,
      .stack_ptr = 0,
  };

  bool repl = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      repl = true;
    } else {
      eval_script(argv[i], &ctx, module, builder);
    }
  }

  if (repl) {
    char *prompt = "\033[1;31mλ \033[1;0m"
                   "\033[1;36m";
    printf("\033[1;31m"
           "YLC LANG REPL       \n"
           "------------------\n"
           "version 0.0.0       \n"
           "\033[1;0m");

    char *input = malloc(sizeof(char) * INPUT_BUFSIZE);

    LLVMTypeRef top_level_ret_type;
    while (true) {
      repl_input(input, INPUT_BUFSIZE, prompt);
      Ast *prog = parse_input(input);

      Ast *top = top_level_ast(prog);

      Env *env = new_env();
      infer(env, top, NULL);

      // Generate node.

      LLVMValueRef top_level_func =
          codegen_top_level(top, &top_level_ret_type, &ctx, module, builder);

#ifdef DEBUG_AST
      print_ast(top);
      LLVMDumpModule(module);
      print_type((Type *)top->md);
      printf("\n");
#endif

      LLVMExecutionEngineRef engine;
      prepare_ex_engine(&engine, module);

      if (top_level_func == NULL) {
        fprintf(stderr, "Unable to codegen for node\n");
        continue;
      }
      LLVMGenericValueRef exec_args[] = {};
      LLVMGenericValueRef result =
          LLVMRunFunction(engine, top_level_func, 0, exec_args);
      printf("> %d\n", (int)LLVMGenericValueToInt(result, 0));
      // switch (LLVMGetTypeKind(top_level_ret_type)) {

      // typedef enum {
      //   LLVMVoidTypeKind,      /**< type with no size */
      //   LLVMHalfTypeKind,      /**< 16 bit floating point type */
      //   LLVMFloatTypeKind,     /**< 32 bit floating point type */
      //   LLVMDoubleTypeKind,    /**< 64 bit floating point type */
      //   LLVMX86_FP80TypeKind,  /**< 80 bit floating point type (X87) */
      //   LLVMFP128TypeKind,     /**< 128 bit floating point type (112-bit
      //   mantissa)*/ LLVMPPC_FP128TypeKind, /**< 128 bit floating point type
      //   (two 64-bits) */ LLVMLabelTypeKind,     /**< Labels */
      //   LLVMIntegerTypeKind,   /**< Arbitrary bit width integers */
      //   LLVMFunctionTypeKind,  /**< Functions */
      //   LLVMStructTypeKind,    /**< Structures */
      //   LLVMArrayTypeKind,     /**< Arrays */
      //   LLVMPointerTypeKind,   /**< Pointers */
      //   LLVMVectorTypeKind,    /**< Fixed width SIMD vector type */
      //   LLVMMetadataTypeKind,  /**< Metadata */
      //   LLVMX86_MMXTypeKind,   /**< X86 MMX */
      //   LLVMTokenTypeKind,     /**< Tokens */
      //   LLVMScalableVectorTypeKind, /**< Scalable SIMD vector type */
      //   LLVMBFloatTypeKind,         /**< 16 bit brain floating point type
      //   */ LLVMX86_AMXTypeKind,        /**< X86 AMX */
      //   LLVMTargetExtTypeKind,      /**< Target extension type */
      // } LLVMTypeKind;
      //
      // default:
      // }
    }
    free(input);
  }

  return 0;
}
