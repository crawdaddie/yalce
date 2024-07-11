#include "backend_llvm/jit.h"
#include "backend_llvm/common.h"
#include "codegen.h"
#include "codegen_types.h"
#include "input.h"
#include "parse.h"
#include "serde.h"
#include "types/inference.h"
#include "types/util.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Ast *top_level_ast(Ast *body) {
  size_t len = body->data.AST_BODY.len;
  Ast *last = body->data.AST_BODY.stmts[len - 1];
  return last;
}

static LLVMValueRef codegen_top_level(Ast *ast, LLVMTypeRef *ret_type,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  // Create function type.
  LLVMTypeRef funcType =
      LLVMFunctionType(type_to_llvm_type(ast->md), NULL, 0, 0);

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

static LLVMGenericValueRef eval_script(const char *filename, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder, TypeEnv **env,
                                       Ast **prog) {

  char *fcontent = read_script(filename);
  if (!fcontent) {
    return NULL;
  }

  *prog = parse_input(fcontent);

  infer_ast(env, *prog);

  LLVMTypeRef top_level_ret_type;

  LLVMValueRef top_level_func =
      codegen_top_level(*prog, &top_level_ret_type, ctx, module, builder);

#ifdef DUMP_AST
  print_ast(*prog);
  printf("-----\n");
  LLVMDumpModule(module);
#endif

  LLVMExecutionEngineRef engine;
  prepare_ex_engine(&engine, module);

  if (top_level_func == NULL) {
    fprintf(stderr, "Unable to codegen for node\n");
    return NULL;
  }
  LLVMGenericValueRef exec_args[] = {};
  LLVMGenericValueRef result =
      LLVMRunFunction(engine, top_level_func, 0, exec_args);
  printf("> %d\n", (int)LLVMGenericValueToInt(result, 0));

  free(fcontent);
  return result; // Return success
}
typedef struct int_ll_t {
  int32_t el;
  struct int_ll_t *next;
} int_ll_t;

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

  // shared type env
  TypeEnv *env = NULL;

  JITLangCtx ctx = {
      .stack = stack,
      .stack_ptr = 0,
  };

  bool repl = false;

  Ast *script_prog;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      repl = true;
    } else {
      eval_script(argv[i], &ctx, module, builder, &env, &script_prog);
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

      Type *typecheck_result = infer_ast(&env, top);
      if (typecheck_result == NULL) {
        continue;
      }

      // Generate node.
      LLVMValueRef top_level_func =
          codegen_top_level(top, &top_level_ret_type, &ctx, module, builder);

#ifdef DEBUG_AST
      print_ast(top);
      LLVMDumpModule(module);
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

      printf("> '");
      print_type(top->md);
      Type *top_type = top->md;
      switch (top_type->kind) {
      case T_INT: {
        printf(" %d\n", (int)LLVMGenericValueToInt(result, 0));
        break;
      }

      case T_NUM: {
        printf(" %f\n",
               (double)LLVMGenericValueToFloat(LLVMDoubleType(), result));
        break;
      }

      case T_STRING: {
        printf(" %s\n", (char *)LLVMGenericValueToPointer(result));
        break;
      }

      case T_CONS: {
        if (strcmp(top_type->data.T_CONS.name, "List") == 0 &&
            top_type->data.T_CONS.args[0]->kind == T_INT) {

          int_ll_t *current = (int_ll_t *)LLVMGenericValueToPointer(result);
          int count = 0;
          printf(" [");
          while (current != NULL &&
                 count < 10) { // Limit to prevent infinite loop
            printf("%d, ", current->el);
            current = current->next;
            count++;
          }
          if (count == 10) {
            printf("...");
          }
          printf("]\n");
          break;
        }

        break;
      }

      case T_FN: {
        printf(" %p\n", result);
        break;
      }

      default:
        printf(" %d\n", (int)LLVMGenericValueToInt(result, 0));
        break;
      }
    }
    free(input);
  }

  return 0;
}
