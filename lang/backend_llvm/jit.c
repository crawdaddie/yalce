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
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// YALCE STDLIB

// uniformly distributed integer between 0 and range-1
int rand_int(int range) {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * range;
  return (int)rand_double;
}

// uniformly distributed double between 0 and 1.0
double rand_double() {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * 2 - 1;
  return rand_double;
}

// uniformly distributed double between min and max
double rand_double_range(double min, double max) {
  int rand_int = rand();
  double rand_double = (double)rand_int / RAND_MAX;
  rand_double = rand_double * (max - min) + min;
  return rand_double;
}

double amp_db(double amplitude) { return 20.0f * log10(amplitude); }
double db_amp(double db) { return pow(10.0f, db / 20.0f); }

FILE *get_stderr() { return stderr; }
FILE *get_stdout() { return stdout; }

#define STACK_MAX 256

void print_result(Type *type, LLVMGenericValueRef result);
static Ast *top_level_ast(Ast *body) {
  size_t len = body->data.AST_BODY.len;
  Ast *last = body->data.AST_BODY.stmts[len - 1];
  return last;
}

// ---------------------------

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

  char *fcontent = read_script(filename);
  LLVMSetSourceFileName(module, filename, strlen(filename));
  if (!fcontent) {
    return NULL;
  }
  char *dirname = get_dirname(filename);
  if (dirname == NULL) {
    return NULL;
  }

  *prog = parse_input(fcontent, dirname);

#ifdef DUMP_AST
  print_ast(*prog);
#endif

  if (!(*prog)) {
    return NULL;
  }

  infer(*prog, env);
  // print_ast(*prog);
  ctx->env = *env;

#ifdef DUMP_AST
  LLVMDumpModule(module);
#endif

  Type *result_type = top_level_ast(*prog)->md;

  if (result_type == NULL) {
    printf("typecheck failed\n");
    free(fcontent);
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
    free(fcontent);
    return NULL;
  }

  LLVMGenericValueRef exec_args[] = {};

  if (result_type->kind == T_FN) {
    printf("> ");
    print_type(result_type);
    printf("\n");
    return NULL;
  }

  LLVMGenericValueRef result =
      LLVMRunFunction(engine, top_level_func, 0, exec_args);

  printf("> ");
  print_result(result_type, result);
  free(fcontent);
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

  // shared type env
  TypeEnv *env = NULL;
  initialize_builtin_numeric_types(env);
  initialize_builtin_binops(stack, env);
  // env = initialize_type_env(env);

  env = initialize_type_env_synth(env);

  JITLangCtx ctx = {.stack = stack,
                    .stack_ptr = 0,
                    .env = env,
                    .num_globals = &num_globals,
                    .global_storage_array = global_storage_array,
                    .global_storage_capacity = &global_storage_capacity};

  bool repl = false;

  int arg_counter = 1;
  while (arg_counter < argc) {
    if (strcmp(argv[arg_counter], "-i") == 0) {
      repl = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-type-memory") == 0) {
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

      int top_level_size = ctx.stack->length;

      hti it = ht_iterator(ctx.stack);

      for (int completion_entry = 0; ht_next(&it); completion_entry++) {
        add_completion_item(it.key, completion_entry);
      };

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
      } else if (strcmp("\n", input) == 0) {
        continue;
      }

      Ast *prog = parse_input(input, dirname);
      Type *typecheck_result = infer(prog, &env);

      ctx.env = env;

      if (typecheck_result == NULL) {
        continue;
      }

      if (typecheck_result->kind == T_VAR) {
        Ast *top = top_level_ast(prog);
        fprintf(stderr, "value not found: ");
        print_ast_err(top);
        continue;
      }

      // Generate node.
      LLVMValueRef top_level_func =
          codegen_top_level(prog, &top_level_ret_type, &ctx, module, builder);

#ifdef DUMP_AST
      printf("%s\n", COLOR_RESET);
      LLVMDumpModule(module);
#endif

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
