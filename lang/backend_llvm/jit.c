#include "backend_llvm/jit.h"
#include "backend_llvm/codegen.h"
#include "backend_llvm/common.h"
#include "backend_llvm/globals.h"
#include "format_utils.h"
#include "input.h"
#include "parse.h"
#include "serde.h"
#include "symbols.h"
#include "synths.h"
#include "types.h"
#include "types/inference.h"
#include "llvm-c/Transforms/Utils.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Linker.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/InstCombine.h>
#include <llvm-c/Transforms/Scalar.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vecLib/vecLib.h>

#define STACK_MAX 256

void print_result(Type *type, LLVMGenericValueRef result);
static Ast *top_level_ast(Ast *body) {
  size_t len = body->data.AST_BODY.len;
  Ast *last = body->data.AST_BODY.stmts[len - 1];
  return last;
}

static LLVMGenericValueRef eval_script(const char *filename, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder,
                                       LLVMContextRef llvm_ctx, TypeEnv **env,
                                       Ast **prog);

int prepare_ex_engine(JITLangCtx *ctx, LLVMExecutionEngineRef *engine,
                      LLVMModuleRef module) {
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

  LLVMValueRef array_global =
      LLVMGetNamedGlobal(module, "global_storage_array");
  LLVMValueRef size_global = LLVMGetNamedGlobal(module, "global_storage_size");

  LLVMAddGlobalMapping(*engine, array_global, ctx->global_storage_array);
  LLVMAddGlobalMapping(*engine, size_global, ctx->global_storage_capacity);
}

static LLVMGenericValueRef eval_script(const char *filename, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder,
                                       LLVMContextRef llvm_ctx, TypeEnv **env,
                                       Ast **prog) {

  LLVMSetSourceFileName(module, filename, strlen(filename));

  *prog = parse_input_script(filename);
  // print_ast(*prog);
  if (!(*prog)) {
    return NULL;
  }

  infer(*prog, env);
  ctx->env = *env;

  Type *result_type = top_level_ast(*prog)->md;

  if (result_type == NULL) {
    printf("typecheck failed\n");
    return NULL;
  }

  LLVMTypeRef top_level_ret_type;

  LLVMValueRef top_level_func =
      codegen_top_level(*prog, &top_level_ret_type, ctx, module, builder);

  LLVMExecutionEngineRef engine;
  prepare_ex_engine(ctx, &engine, module);

  if (top_level_func == NULL) {
    printf("> ");
    print_result(result_type, NULL);
    return NULL;
  }

  LLVMGenericValueRef exec_args[] = {};

  if (result_type->kind == T_FN) {
    printf("> ");
    print_type(result_type);
    printf("\n");
    return NULL;
  }

  // LLVMDumpModule(module);
  LLVMGenericValueRef result =
      LLVMRunFunction(engine, top_level_func, 0, exec_args);

  printf("> ");
  print_result(result_type, result);
  return result; // Return success
}

typedef struct ll_int_t {
  int32_t el;
  struct ll_int_t *next;
} int_ll_t;

void module_passes(LLVMModuleRef module) {
  LLVMPassManagerRef pass_manager =
      LLVMCreateFunctionPassManagerForModule(module);

  LLVMAddPromoteMemoryToRegisterPass(pass_manager);
  LLVMAddInstructionCombiningPass(pass_manager);
  LLVMAddReassociatePass(pass_manager);
  LLVMAddGVNPass(pass_manager);
  LLVMAddCFGSimplificationPass(pass_manager);
  LLVMAddTailCallEliminationPass(pass_manager);
}

#define GLOBAL_STORAGE_CAPACITY 1024
int jit(int argc, char **argv) {

  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  LLVMLinkInMCJIT();

  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module =
      LLVMModuleCreateWithNameInContext("ylc.top-level", context);

  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
  module_passes(module);

  // void **global_storage_array = calloc(GLOBAL_STORAGE_CAPACITY, sizeof(void
  // *));
  void *global_storage_array[GLOBAL_STORAGE_CAPACITY];
  int global_storage_capacity = GLOBAL_STORAGE_CAPACITY;
  int num_globals = 0;

  setup_global_storage(module, builder);

  ht stack[STACK_MAX];

  for (int i = 0; i < STACK_MAX; i++) {
    ht_init(&stack[i]);
  }

  TypeEnv *env = NULL;
  initialize_builtin_numeric_types(env);
  env = initialize_builtin_funcs(stack, env);
  env = initialize_types(env);
  env = initialize_type_env_synth(env);

  t_option_of_var.alias = "Option";
  env = env_extend(env, "Option", &t_option_of_var);


  JITLangCtx ctx = {.stack = stack,
                    .stack_ptr = 0,
                    .env = env,
                    .num_globals = &num_globals,
                    .global_storage_array = global_storage_array,
                    .global_storage_capacity = &global_storage_capacity};

  bool repl = false;
  // print_type_env(env);

  int arg_counter = 1;
  while (arg_counter < argc) {
    if (strcmp(argv[arg_counter], "-i") == 0) {
      repl = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--test") == 0) {
      // run top-level tests for input module
      top_level_tests = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-type-memory") == 0) {
      // TODO: implement specific limits for typechecker storage
      arg_counter++;
      printf("-- type storage allocation: %d\n", atoi(argv[arg_counter]));
      arg_counter++;
    } else {
      Ast *script_prog;
      eval_script(argv[arg_counter], &ctx, module, builder, context, &env,
                  &script_prog);
      arg_counter++;
    }
  }

  if (repl) {
    // printf("start repl: ## SYNTH??: ");
    // print_type(&t_synth);

    // print_type_env(env);

    char dirname[100];
    getcwd(dirname, 100);

    printf(COLOR_MAGENTA "YLC LANG REPL     \n"
                         "------------------\n"
                         "version 0.0.0     \n" STYLE_RESET_ALL);

    init_readline();

    // char *input = malloc(sizeof(char) * INPUT_BUFSIZE);
    LLVMTypeRef top_level_ret_type;
    char *prompt = COLOR_RED "λ " COLOR_RESET COLOR_CYAN;

    while (true) {


      char *input = repl_input(prompt);

      if (strncmp("%dump_module", input, 12) == 0) {
        printf(STYLE_RESET_ALL "\n");
        LLVMDumpModule(module);
        continue;
      } else if (strncmp("%dump_type_env", input, 14) == 0) {
        print_type_env(env);
        continue;
      } else if (strncmp("%dump_ast", input, 9) == 0) {
        print_ast(ast_root);
        continue;
      } else if (strncmp("%quit", input, 5) == 0) {
        print_ast(ast_root);
        continue;
      } else if (strcmp("\n", input) == 0) {
        continue;
      }

      Ast *prog;
      if (strncmp("%include", input, 8) == 0) {
        prog = parse_repl_include(input);
        // print_ast(prog);
      } else {
        prog = parse_input(input, dirname);
      }

      Type *typecheck_result = infer(prog, &env);
      // print_type(typecheck_result);

      ctx.env = env;

      if (typecheck_result == NULL) {
        continue;
      }

      if (typecheck_result->kind == T_VAR) {
        Ast *top = top_level_ast(prog);
        fprintf(stderr, "value not found: ");
        print_ast_err(top);
        print_location(top);
        continue;
      }

      // Generate node.
      LLVMValueRef top_level_func =
          codegen_top_level(prog, &top_level_ret_type, &ctx, module, builder);

      printf(COLOR_GREEN "> ");

      Type *top_type = prog->md;

      if (top_level_func == NULL) {
        print_result(top_type, NULL);
        continue;
      } else {
        LLVMExecutionEngineRef engine;
        prepare_ex_engine(&ctx, &engine, module);
        LLVMGenericValueRef exec_args[] = {};
        LLVMGenericValueRef result =
            LLVMRunFunction(engine, top_level_func, 0, exec_args);
        print_result(top_type, result);
      }
      printf(COLOR_RESET);
    }
  }

  return 0;
}

void print_result(Type *type, LLVMGenericValueRef result) {
  printf("`");
  if (type->alias != NULL) {
    printf("%s\n", type->alias);
  } else {
    print_type(type);
  }

  if (result == NULL) {
    printf("\n");
    return;
  }

  switch (type->kind) {
  case T_INT: {
    printf("%d", (int)LLVMGenericValueToInt(result, 0));
    break;
  }

  case T_BOOL: {
    printf("%d", (int)LLVMGenericValueToInt(result, 0));
    break;
  }

  case T_NUM: {
    printf("%f", (double)LLVMGenericValueToFloat(LLVMDoubleType(), result));
    break;
  }

  case T_STRING: {
    printf("%s", (char *)LLVMGenericValueToPointer(result));
    break;
  }

  case T_CHAR: {
    printf("%c", (int)LLVMGenericValueToInt(result, 0));
    break;
  }

  case T_CONS: {
    if (is_string_type(type)) {
      printf("%s", (char *)LLVMGenericValueToPointer(result));
      break;
    }
    if (strcmp(type->data.T_CONS.name, "List") == 0 &&
        type->data.T_CONS.args[0]->kind == T_INT) {

      int_ll_t *current = (int_ll_t *)LLVMGenericValueToPointer(result);
      int count = 0;
      printf("[");
      while (current != NULL && count < 10) { // Limit to prevent infinite loop
        printf("%d, ", current->el);
        current = current->next;
        count++;
      }
      if (count == 10) {
        printf("...");
      }
      printf("]");
      break;
    }

    printf("%s %p", type->data.T_CONS.name, LLVMGenericValueToPointer(result));
    break;
  }

  case T_FN: {
    printf("%p", result);
    break;
  }

  default:
    printf("%d", (int)LLVMGenericValueToInt(result, 0));
    break;
  }
  printf("\n");
}
