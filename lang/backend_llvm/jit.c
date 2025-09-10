#include "./jit.h"
#include "../modules.h"
#include "./codegen.h"
#include "./common.h"
#include "./globals.h"
#include "builtin_functions.h"
#include "config.h"
#include "escape_analysis.h"
#include "format_utils.h"
#include "input.h"
#include "modules.h"
#include "parse.h"
#include "serde.h"
#include "testing.h"
#include "types/inference.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/LLJIT.h>
#include <llvm-c/Linker.h>
#include <llvm-c/Orc.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static LLVMTargetMachineRef target_machine;
void module_passes(LLVMModuleRef module, LLVMTargetMachineRef target_machine);

typedef struct {
  LLVMModuleRef module;
  const char *filename;
  const char *dirname;
  JITLangCtx *ctx;
  LLVMBuilderRef builder;
} repl_args;

void repl_loop(LLVMModuleRef module, const char *filename, const char *dirname,
               JITLangCtx *ctx, LLVMBuilderRef builder);

void *repl_loop_thread_fn(void *arg) {
  repl_args *args = (repl_args *)arg;
  repl_loop(args->module, args->filename, args->dirname, args->ctx,
            args->builder);
  return NULL;
}

void break_repl_for_gui_loop(LLVMModuleRef module, const char *filename,
                             const char *dirname, JITLangCtx *ctx,
                             LLVMBuilderRef builder) {
  repl_args thread_args = {module, filename, dirname, ctx, builder};

  pthread_t repl_thread;
  __BREAK_REPL_FOR_GUI_LOOP = false;
  if (pthread_create(&repl_thread, NULL, repl_loop_thread_fn, &thread_args) !=
      0) {
    perror("Failed to create REPL thread");
  }
  break_repl_for_gui_loop_cb();
  return;
}
void dump_assembly(LLVMModuleRef module);
#define STACK_MAX 256

static void *eval_script(const char *filename, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder,
                         LLVMContextRef llvm_ctx, TypeEnv **env, Ast **prog);

int prepare_ex_engine(JITLangCtx *ctx, LLVMExecutionEngineRef *engine,
                      LLVMModuleRef module) {
  char *error = NULL;

  // Initialize MCJIT compiler options
  struct LLVMMCJITCompilerOptions options;
  LLVMInitializeMCJITCompilerOptions(&options, sizeof(options));
  options.OptLevel = 2;

  // Create MCJIT execution engine
  if (LLVMCreateMCJITCompilerForModule(engine, module, &options,
                                       sizeof(options), &error) != 0) {
    fprintf(stderr, "Failed to create execution engine: %s\n", error);
    LLVMDisposeMessage(error);
    return 1;
  }

  // Add global mappings for your globals
  LLVMValueRef array_global =
      LLVMGetNamedGlobal(module, "global_storage_array");
  LLVMValueRef size_global = LLVMGetNamedGlobal(module, "global_storage_size");

  if (array_global) {
    LLVMAddGlobalMapping(*engine, array_global, ctx->global_storage_array);
  }
  if (size_global) {
    LLVMAddGlobalMapping(*engine, size_global, ctx->global_storage_capacity);
  }

  return 0;
}

static void *eval_script(const char *filename, JITLangCtx *ctx,
                         LLVMModuleRef module, LLVMBuilderRef builder,
                         LLVMContextRef llvm_ctx, TypeEnv **env, Ast **prog) {

  // __import_current_dir = get_dirname(filename);

  if (config.test_mode) {
    printf("\n# Test %s\n"
           "-----------------------------------------\n",
           filename);
  }

  LLVMSetSourceFileName(module, filename, strlen(filename));

  *prog = parse_input_script(filename);

  if (!(*prog)) {
    return NULL;
  }

  TICtx ti_ctx = {.env = *env, .scope = 0};

  ti_ctx.err_stream = stderr;

  if (!infer(*prog, &ti_ctx)) {
    return NULL;
  }

  escape_analysis(*prog);

  ctx->env = ti_ctx.env;
  ctx->module_name = filename;

  if (config.test_mode) {
    ctx->module_name = filename;
    int res = test_module(*prog, ctx, module, builder, target_machine);
    if (!res) {
      exit(1);
    } else {
      exit(0);
    }
  }

  Type *result_type = body_tail(*prog)->md;

  if (result_type == NULL) {
    printf("typecheck failed\n");
    return NULL;
  }

  LLVMTypeRef top_level_ret_type;

  LLVMValueRef top_level_func =
      codegen_top_level(*prog, &top_level_ret_type, ctx, module, builder);
  if (config.debug_ir_pre) {
    LLVMDumpModule(module);
  }
  module_passes(module, target_machine);

  LLVMExecutionEngineRef engine;
  prepare_ex_engine(ctx, &engine, module);

  if (top_level_func == NULL) {
    return NULL;
  }

  LLVMGenericValueRef exec_args[] = {};
  if (config.debug_ir) {
    LLVMDumpModule(module);
  }

  if (config.debug_codegen) {
    dump_assembly(module);
  }

  const char *func_name = LLVMGetValueName(top_level_func);
  uint64_t func_addr = LLVMGetFunctionAddress(engine, func_name);
  typedef int (*top_level_func_t)(void);
  top_level_func_t func = (top_level_func_t)func_addr;
  int result = func();
  return NULL;
}

void dump_assembly(LLVMModuleRef module) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();

  char *triple = LLVMGetDefaultTargetTriple();

  LLVMTargetRef target;
  char *error_msg;
  if (LLVMGetTargetFromTriple(triple, &target, &error_msg)) {
    fprintf(stderr, "Error getting target: %s\n", error_msg);
    LLVMDisposeMessage(error_msg);
    return;
  }

  LLVMMemoryBufferRef asm_buffer;
  if (LLVMTargetMachineEmitToMemoryBuffer(
          target_machine, module, LLVMAssemblyFile, &error_msg, &asm_buffer)) {
    fprintf(stderr, "Error generating assembly: %s\n", error_msg);
    LLVMDisposeMessage(error_msg);
  } else {
    printf("\n=== GENERATED ASSEMBLY ===\n");
    printf("%s\n", LLVMGetBufferStart(asm_buffer));
    printf("=== END ASSEMBLY ===\n\n");
    LLVMDisposeMemoryBuffer(asm_buffer);
  }

  // LLVMDisposeTargetMachine(target_machine);
  LLVMDisposeMessage(triple);
}

void module_passes(LLVMModuleRef module, LLVMTargetMachineRef target_machine) {
  char *error = NULL;

  LLVMPassBuilderOptionsRef options = LLVMCreatePassBuilderOptions();

  LLVMErrorRef err =
      LLVMRunPasses(module, config.opt_level ? config.opt_level : "default<O3>",
                    target_machine, options);

  if (err) {
    char *msg = LLVMGetErrorMessage(err);
    fprintf(stderr, "Pass manager error: %s\n", msg);
    LLVMDisposeErrorMessage(msg);
    LLVMConsumeError(err);
  }

  LLVMDisposePassBuilderOptions(options);
}

#define GLOBAL_STORAGE_CAPACITY 1024

int jit(int argc, char **argv) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  LLVMLinkInMCJIT();

  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module =
      LLVMModuleCreateWithNameInContext("ylc.top-level", context);

  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

  // Create target machine for the new pass manager
  char *triple = LLVMGetDefaultTargetTriple();
  LLVMTargetRef target;
  char *error_msg;

  if (LLVMGetTargetFromTriple(triple, &target, &error_msg)) {
    fprintf(stderr, "Error getting target: %s\n", error_msg);
    LLVMDisposeMessage(error_msg);
    return 1;
  }

  target_machine = LLVMCreateTargetMachine(
      target, triple, "generic", "", LLVMCodeGenLevelDefault, LLVMRelocDefault,
      LLVMCodeModelDefault);

  void *global_storage_array[GLOBAL_STORAGE_CAPACITY];
  int global_storage_capacity = GLOBAL_STORAGE_CAPACITY;
  int num_globals = 0;

  init_module_registry();

  setup_global_storage(module, builder);

  TypeEnv *env = NULL;
  initialize_builtin_schemes();

  ht table;
  ht_init(&table);
  StackFrame initial_stack_frame = {.table = &table, .next = NULL};

  JITLangCtx ctx = {.stack_ptr = 0,
                    .env = env,
                    .num_globals = &num_globals,
                    .global_storage_array = global_storage_array,
                    .global_storage_capacity = &global_storage_capacity,
                    .frame = &initial_stack_frame};

  initialize_builtin_funcs(&ctx, module, builder);
  // initialize_synth_types(&ctx, module, builder);

  int arg_counter = 1;
  config.base_libs_dir = getenv("YLC_BASE_DIR");
  int scripts = 0;
  while (arg_counter < argc) {
    if (strcmp(argv[arg_counter], "-i") == 0) {
      config.interactive_mode = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--debug-codegen") == 0) {
      config.debug_ir = true;
      config.debug_codegen = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--debug-ir") == 0) {
      config.debug_ir = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--debug-ir-pre") == 0) {
      config.debug_ir_pre = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--test") == 0) {
      // run top-level tests for input module
      config.test_mode = true;
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "--base") == 0) {
      arg_counter++;
      config.base_libs_dir = argv[arg_counter];
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-O0") == 0) {
      config.opt_level = "default<O0>";
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-O1") == 0) {
      config.opt_level = "default<O1>";
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-O2") == 0) {
      config.opt_level = "default<O2>";
      arg_counter++;
    } else if (strcmp(argv[arg_counter], "-O3") == 0) {
      config.opt_level = "default<O3>";
      arg_counter++;
    } else {

      Ast *script_prog;

      eval_script(argv[arg_counter], &ctx, module, builder, context, &env,
                  &script_prog);
      arg_counter++;
      scripts = 1;
    }
  }

  if (argc == 1 || !scripts) {
    config.interactive_mode = true;
  }

  if (config.interactive_mode) {

    char dirname[100];
    getcwd(dirname, 100);
    char filename[20];
    time_t current_time = time(NULL);
    snprintf(filename, 20, "tmp_%ld.ylc", current_time);

    // __import_current_dir = dirname;

    printf(COLOR_MAGENTA "YLC LANG REPL     \n"
                         "------------------\n"
                         "version 0.0.0     \n"
                         "module base directory: %s\n"
                         "Opt level: %s\n" STYLE_RESET_ALL,

           config.base_libs_dir == NULL ? "./" : config.base_libs_dir,
           config.opt_level ? config.opt_level : "default<O3>");

    init_readline();

    if (__BREAK_REPL_FOR_GUI_LOOP && break_repl_for_gui_loop_cb != NULL) {
      break_repl_for_gui_loop(module, filename, dirname, &ctx, builder);
      return 0;
    }

    repl_loop(module, filename, dirname, &ctx, builder);
  }

  return 0;
}

void repl_loop(LLVMModuleRef module, const char *filename, const char *dirname,
               JITLangCtx *ctx, LLVMBuilderRef builder) {

  LLVMTypeRef top_level_ret_type;
  char *prompt = COLOR_RED "λ " COLOR_RESET COLOR_CYAN;

  while (true) {

    char *input = repl_input(prompt);

    if (strncmp("%dump_module", input, 12) == 0) {
      printf(STYLE_RESET_ALL "\n");
      LLVMDumpModule(module);
      continue;
    } else if (strncmp("%dump_type_env", input, 14) == 0) {
      print_type_env(ctx->env);
      continue;
    } else if (strncmp("%dump_ast", input, 9) == 0) {
      print_ast(pctx.ast_root);
      continue;
    } else if (strncmp("%builtins", input, 8) == 0) {
      print_builtin_types();
      continue;
    } else if (strncmp("%plot", input, 5) == 0) {
      continue;
    } else if (strcmp("\n", input) == 0) {
      continue;
    } else if (strcmp("%quit", input) == 0) {
      break;
    }

    LLVMSetSourceFileName(module, filename, strlen(filename));
    Ast *prog = parse_input(input, dirname);

    TICtx ti_ctx = {.env = ctx->env, .scope = 0};

    Type *typecheck_result = infer(prog, &ti_ctx);

    escape_analysis(prog);

    ctx->env = ti_ctx.env;

    if (typecheck_result == NULL) {
      continue;
    }

    if (typecheck_result->kind == T_VAR) {
      Ast *top = body_tail(prog);
      fprintf(stderr, "value not found: ");
      print_ast_err(top);
      print_location(top);
      continue;
    }

    // Generate node.
    LLVMValueRef top_level_func =
        codegen_top_level(prog, &top_level_ret_type, ctx, module, builder);

    printf(COLOR_GREEN "> ");

    Type *top_type = prog->md;

    if (top_level_func == NULL) {
      print_type(top_type);
      continue;
    } else {
      // Run optimization passes before execution
      module_passes(module, target_machine);

      LLVMExecutionEngineRef engine;
      prepare_ex_engine(ctx, &engine, module);
      const char *func_name = LLVMGetValueName(top_level_func);
      uint64_t func_addr = LLVMGetFunctionAddress(engine, func_name);
      typedef int (*top_level_func_t)(void);
      top_level_func_t func = (top_level_func_t)func_addr;
      print_type(top_type);
      int result = func();
    }
    printf(COLOR_RESET);

    if (__BREAK_REPL_FOR_GUI_LOOP && break_repl_for_gui_loop_cb != NULL) {
      return break_repl_for_gui_loop(module, filename, dirname, ctx, builder);
    }
  }
}
