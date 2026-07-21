#include "./orc.h"
#include "./codegen.h"
#include "./llvm/lowering.h"
#include "builtin_functions.h"
#include "config.h"
#include "debugging.h"
#include "format_utils.h"
#include "function.h"
#include "globals.h"
#include "input.h"
#include "mir/mir.h"
#include "module.h"
#include "modules.h"
#include "parse.h"
#include "symbols.h"
#include "testing.h"
#include "types.h"
#include "types/builtins.h"
#include "types/inference.h"
#include "types/type_ser.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Error.h>
#include <llvm-c/LLJIT.h>
#include <llvm-c/Orc.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLOBAL_STORAGE_CAPACITY 1024
void *global_storage_array[GLOBAL_STORAGE_CAPACITY];
int global_storage_size = GLOBAL_STORAGE_CAPACITY;
static int num_globals = 0;
static int top_counter = 0;

void module_passes(LLVMModuleRef module, LLVMTargetMachineRef target_machine);

typedef struct {
  LLVMModuleRef module;
  LLVMContextRef context;
  char entry_name[64];
  bool returns_bool;
} ORCCompiledModule;

static void lang_init(ht *table, TypeEnv *env, JITLangCtx *ctx,
                      StackFrame *initial_stack_frame) {
  init_module_registry();
  initialize_builtin_types();
  ht_init(table);
  *initial_stack_frame = (StackFrame){.table = table, .next = NULL};
  *ctx = (JITLangCtx){.stack_ptr = 0,
                      .env = env,
                      .num_globals = &num_globals,
                      .global_storage_array = global_storage_array,
                      .global_storage_capacity = &global_storage_size,
                      .frame = initial_stack_frame};
  initialize_builtin_funcs(ctx, NULL, NULL);
}

static LLVMTargetMachineRef create_target_machine(LLVMOrcLLJITRef jit) {
  const char *triple = LLVMOrcLLJITGetTripleString(jit);
  LLVMTargetRef target = NULL;
  char *error_msg = NULL;

  if (LLVMGetTargetFromTriple(triple, &target, &error_msg)) {
    fprintf(stderr, "Error getting target: %s\n", error_msg);
    LLVMDisposeMessage(error_msg);
    return NULL;
  }

  return LLVMCreateTargetMachine(target, triple, "generic", "",
                                 LLVMCodeGenLevelDefault, LLVMRelocDefault,
                                 LLVMCodeModelDefault);
}

static ORCCompiledModule compiled_module_none(void) {
  return (ORCCompiledModule){.module = NULL, .context = NULL, .entry_name = ""};
}

static ORCCompiledModule dispose_compile_artifacts(LLVMContextRef context,
                                                   LLVMModuleRef module,
                                                   LLVMBuilderRef builder) {
  if (builder) {
    LLVMDisposeBuilder(builder);
  }
  if (module) {
    LLVMDisposeModule(module);
  }
  if (context) {
    LLVMContextDispose(context);
  }
  return compiled_module_none();
}

static void next_top_name(char *buf, size_t buf_len) {
  snprintf(buf, buf_len, "__ylc_top.%d", top_counter++);
}

static LLVMValueRef orc_create_string_constant(LLVMBuilderRef builder,
                                               const char *str) {
  return LLVMBuildGlobalStringPtr(builder, str, "test_name");
}

static LLVMValueRef orc_create_report_function(LLVMModuleRef module) {
  LLVMValueRef report_func =
      LLVMGetNamedFunction(module, "_report_test_result");
  if (report_func) {
    return report_func;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef param_types[] = {
      LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0),
      LLVMInt1TypeInContext(llvm_ctx),
  };
  LLVMTypeRef report_func_type =
      LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), param_types, 2, false);
  return LLVMAddFunction(module, "_report_test_result", report_func_type);
}

static LLVMValueRef orc_create_totals_function(LLVMModuleRef module) {
  LLVMValueRef totals_func =
      LLVMGetNamedFunction(module, "_report_test_totals");
  if (totals_func) {
    return totals_func;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef i32_type = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef param_types[] = {i32_type, i32_type};
  LLVMTypeRef totals_func_type =
      LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx), param_types, 2, false);
  return LLVMAddFunction(module, "_report_test_totals", totals_func_type);
}

static Ast *orc_get_test_module_ast(Ast *ast) {
  if (ast->tag == AST_LET && ast->data.AST_LET.binding->tag == AST_IDENTIFIER &&
      strcmp(ast->data.AST_LET.binding->data.AST_IDENTIFIER.value, "test") ==
          0) {
    return ast->data.AST_LET.expr;
  }

  if (ast->tag == AST_BODY) {
    for (AstList *it = ast->data.AST_BODY.stmts; it; it = it->next) {
      Ast *stmt = it->ast;
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

static LLVMValueRef codegen_orc_test_module(Ast *ast, JITLangCtx *ctx,
                                            LLVMModuleRef module,
                                            LLVMBuilderRef builder) {
  Ast *test_module_ast = orc_get_test_module_ast(ast);
  if (!test_module_ast) {
    fprintf(stderr, "module %s does not contain a test module\n",
            ctx->module_name);
    return NULL;
  }

  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef i1_type = LLVMInt1TypeInContext(llvm_ctx);
  LLVMTypeRef i32_type = LLVMInt32TypeInContext(llvm_ctx);
  LLVMTypeRef func_type = LLVMFunctionType(i1_type, NULL, 0, false);
  LLVMValueRef func = LLVMAddFunction(module, "top", func_type);
  LLVMSetLinkage(func, LLVMExternalLinkage);

  LLVMBasicBlockRef block =
      LLVMAppendBasicBlockInContext(llvm_ctx, func, "entry");
  LLVMPositionBuilderAtEnd(builder, block);

  LLVMValueRef body = codegen(ast, ctx, module, builder);
  if (!body) {
    fprintf(stderr,
            "Error: test runner could not compile module under test %s\n",
            ctx->module_name);
    LLVMDeleteFunction(func);
    return NULL;
  }

  LLVMValueRef test_result = LLVMConstInt(i1_type, 1, false);
  LLVMValueRef report_func = orc_create_report_function(module);
  LLVMValueRef totals_func = orc_create_totals_function(module);
  LLVMTypeRef test_func_type = LLVMFunctionType(i1_type, NULL, 0, false);

  LLVMValueRef num_tests = LLVMConstInt(i32_type, 0, false);
  LLVMValueRef num_passes = LLVMConstInt(i32_type, 0, false);

  JITSymbol *test_module =
      ht_get_hash(ctx->frame->table, "test", hash_string("test", 4));
  if (!test_module || test_module->type != STYPE_MODULE) {
    fprintf(stderr, "Error: test module symbol was not registered\n");
    LLVMDeleteFunction(func);
    return NULL;
  }

  AstList *stmts = NULL;
  if (test_module_ast->data.AST_LAMBDA.body->tag != AST_BODY) {
    AstList *l = malloc(sizeof(AstList));
    *l = (AstList){.ast = test_module_ast->data.AST_LAMBDA.body};
    stmts = l;
  } else {
    stmts = test_module_ast->data.AST_LAMBDA.body->data.AST_BODY.stmts;
  }

  for (AstList *it = stmts; it; it = it->next) {
    Ast *stmt = it->ast;
    if (stmt->tag != AST_LET) {
      continue;
    }

    Ast *binding = stmt->data.AST_LET.binding;
    if (!(binding->tag == AST_IDENTIFIER &&
          strncmp(binding->data.AST_IDENTIFIER.value, "test", 4) == 0)) {
      continue;
    }

    const char *key = binding->data.AST_IDENTIFIER.value;
    JITSymbol *sym = find_in_ctx(key, strlen(key),
                                 test_module->symbol_data.STYPE_MODULE.ctx);
    if (!sym) {
      fprintf(stderr, "Error: test symbol %s was not compiled\n", key);
      LLVMDeleteFunction(func);
      return NULL;
    }

    LLVMValueRef raw_result = NULL;
    if (stmt->data.AST_LET.expr->tag == AST_LAMBDA) {
      LLVMValueRef test_fn = NULL;
      if (sym->type == STYPE_GENERIC_FUNCTION) {
        Type *expected_type =
            specialize_type_for_codegen(stmt->data.AST_LET.expr->type, ctx);
        test_fn = get_specific_callable(
            sym, expected_type, test_module->symbol_data.STYPE_MODULE.ctx,
            module, builder);
      } else {
        test_fn = rematerialize_function_symbol(sym, ctx, module);
      }

      if (!test_fn) {
        fprintf(stderr, "Error: test function %s was not compiled\n", key);
        LLVMDeleteFunction(func);
        return NULL;
      }

      raw_result = LLVMBuildCall2(builder, test_func_type, test_fn, NULL, 0,
                                  "test_call");
    } else if (types_equal(stmt->data.AST_LET.expr->type, &t_bool)) {
      raw_result = sym->val;
    } else {
      continue;
    }

    num_tests = LLVMBuildAdd(builder, num_tests,
                             LLVMConstInt(i32_type, 1, false), "num_tests");

    LLVMValueRef stable_result =
        LLVMBuildFreeze(builder, raw_result, "stable_test_result");
    LLVMValueRef should_increment =
        LLVMBuildZExt(builder, stable_result, i32_type, "should_increment");
    num_passes =
        LLVMBuildAdd(builder, num_passes, should_increment, "num_passes");

    LLVMValueRef name_str = orc_create_string_constant(builder, key);
    LLVMValueRef report_args[] = {name_str, stable_result};
    LLVMBuildCall2(builder, LLVMGlobalGetValueType(report_func), report_func,
                   report_args, 2, "");

    test_result =
        LLVMBuildAnd(builder, test_result, stable_result, "test_result");
  }

  LLVMBuildCall2(builder, LLVMGlobalGetValueType(totals_func), totals_func,
                 (LLVMValueRef[]){num_passes, num_tests}, 2, "");
  LLVMBuildRet(builder, test_result);
  return func;
}

static ORCCompiledModule
compile_source(const char *filename, const char *source, bool print_result,
               TypeEnv **env, JITLangCtx *ctx, LLVMOrcLLJITRef jit,
               LLVMTargetMachineRef target_machine,
               MirStackFrame *mir_root_frame) {
  if (!filename || !source) {
    return compiled_module_none();
  }

  char *source_copy = strdup(source);
  if (!source_copy) {
    return compiled_module_none();
  }

  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module =
      LLVMModuleCreateWithNameInContext("ylc.transaction", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

  LLVMSetTarget(module, LLVMOrcLLJITGetTripleString(jit));
  LLVMSetDataLayout(module, LLVMOrcLLJITGetDataLayoutStr(jit));
  setup_global_storage(module, builder);

  module_path = filename;
  LLVMSetSourceFileName(module, filename, strlen(filename));

  Ast *prog = parse_input_buffer(filename, source_copy);
  if (!prog) {
    return dispose_compile_artifacts(context, module, builder);
  }

  TICtx ti_ctx = {.env = *env, .scope = 0};
  ti_ctx.err_stream = stderr;

  if (!infer(prog, &ti_ctx)) {
    return dispose_compile_artifacts(context, module, builder);
  }

  *env = ti_ctx.env;
  ctx->env = ti_ctx.env;
  ctx->module_name = filename;

  if (ylc_config.debug_symbols) {
    init_debugging(filename, ctx, module);
  }

  Type *result_type = body_tail(prog)->type;
  if (result_type == NULL) {
    printf("typecheck failed\n");
    return dispose_compile_artifacts(context, module, builder);
  }

  // The MIR top-level scope persists across REPL inputs: the root frame
  // is malloc-backed (mir_stack_frame_init(NULL, ...) in orcjit) and
  // held in the session, so top-level bindings (value globals, functions,
  // module members) survive mir_program_destroy + mir_arena_destroy
  // below. mir_build_program honors a caller-supplied ctx->frame, so it
  // reuses this frame instead of allocating a fresh one per input.
  MirArena *mir_arena = mir_arena_create();
  MirCtx mir_ctx = {.env = ti_ctx.env, .frame = mir_root_frame};
  MirProgram *mir_program = mir_build_program(mir_arena, prog, &mir_ctx);
  if (mir_program_had_error(mir_program)) {
    mir_program_destroy(mir_program);
    mir_arena_destroy(mir_arena);
    return dispose_compile_artifacts(context, module, builder);
  }
  mir_run_passes(mir_program);
  if (mir_program_had_error(mir_program)) {
    mir_program_destroy(mir_program);
    mir_arena_destroy(mir_arena);
    return dispose_compile_artifacts(context, module, builder);
  }
  if (ylc_config.dump_mir) {
    mir_dump_program(mir_program, stdout);
  }

  LLVMTypeRef top_level_ret_type;
  LLVMValueRef top_level_func = NULL;

  top_level_func = lower_mir(mir_program, module, builder);
  mir_program_destroy(mir_program);
  mir_arena_destroy(mir_arena);

  // if (ylc_config.test_mode) {
  //   printf("\n# Test %s\n"
  //          "-----------------------------------------\n",
  //          filename);
  //   top_level_func = codegen_orc_test_module(prog, ctx, module, builder);
  // } else {
  //   top_level_func = print_result
  //                        ? codegen_repl_top_level(prog,
  //                        &top_level_ret_type,
  //                                                 ctx, module, builder)
  //                        : codegen_top_level(prog, &top_level_ret_type,
  //                        ctx,
  //                                            module, builder);
  // }

  if (top_level_func == NULL) {
    return dispose_compile_artifacts(context, module, builder);
  }

  ORCCompiledModule compiled = {.module = module,
                                .context = context,
                                .returns_bool = ylc_config.test_mode};

  next_top_name(compiled.entry_name, sizeof(compiled.entry_name));
  LLVMSetValueName2(top_level_func, compiled.entry_name,
                    strlen(compiled.entry_name));

  if (ylc_config.dump_ir_pre) {
    LLVMDumpModule(module);
  }

  if (ylc_config.verify_ir) {
    char *verify_err = NULL;
    if (LLVMVerifyModule(module, LLVMPrintMessageAction, &verify_err)) {
      fprintf(stderr, "IR verification failed: %s\n", verify_err);
      LLVMDisposeMessage(verify_err);
      return dispose_compile_artifacts(context, module, builder);
    }
    LLVMDisposeMessage(verify_err);
  }

  module_passes(module, target_machine);

  if (ylc_config.dump_ir) {
    LLVMDumpModule(module);
  }

  LLVMDisposeBuilder(builder);
  return compiled;
}

static ORCCompiledModule compile_script(const char *filename, TypeEnv **env,
                                        JITLangCtx *ctx, LLVMOrcLLJITRef jit,
                                        LLVMTargetMachineRef target_machine,
                                        MirStackFrame *mir_root_frame) {
  char *source = read_script(filename);
  if (!source) {
    fprintf(stderr, "Error: failed reading input %s\n", filename);
    return compiled_module_none();
  }

  ORCCompiledModule compiled =
      compile_source(filename, source, false, env, ctx, jit, target_machine,
                      mir_root_frame);
  free(source);
  return compiled;
}

typedef void (*top_fn_t)(void);
typedef int (*test_top_fn_t)(void);
static int consume_llvm_error(LLVMErrorRef err, const char *prefix) {
  char *msg = LLVMGetErrorMessage(err);
  fprintf(stderr, "%s: %s\n", prefix, msg);
  LLVMDisposeErrorMessage(msg);
  LLVMConsumeError(err);
  return 1;
}

static int execute_module_top(ORCCompiledModule compiled, LLVMOrcLLJITRef jit,
                              LLVMOrcJITDylibRef jd) {
  if (!compiled.module || !compiled.context || !compiled.entry_name[0]) {
    return 1;
  }

  LLVMOrcThreadSafeContextRef tsc =
      LLVMOrcCreateNewThreadSafeContextFromLLVMContext(compiled.context);

  LLVMOrcThreadSafeModuleRef tsm =
      LLVMOrcCreateNewThreadSafeModule(compiled.module, tsc);
  LLVMErrorRef err;

  err = LLVMOrcLLJITAddLLVMIRModule(jit, jd, tsm);
  if (err) {
    LLVMOrcDisposeThreadSafeModule(tsm);
    LLVMOrcDisposeThreadSafeContext(tsc);
    return consume_llvm_error(err, "Error adding LLVM IR module");
  }
  LLVMOrcDisposeThreadSafeContext(tsc);
  LLVMOrcExecutorAddress addr = 0;
  err = LLVMOrcLLJITLookup(jit, &addr, compiled.entry_name);
  if (err) {
    return consume_llvm_error(err, "Error looking up top-level function");
  }

  if (compiled.returns_bool) {
    int passed = ((test_top_fn_t)(uintptr_t)addr)();
    return (passed & 1) ? 0 : 1;
  }

  ((top_fn_t)(uintptr_t)addr)();
  return 0;
}

static bool repl_input_matches(const char *input, const char *command) {
  size_t command_len = strlen(command);
  return strncmp(input, command, command_len) == 0 &&
         (input[command_len] == '\0' || input[command_len] == '\n');
}

static bool handle_repl_command(const char *input, TypeEnv *env) {
  if (strcmp(input, "\n") == 0) {
    return true;
  }
  if (repl_input_matches(input, "%dump_type_env")) {
    print_type_env(env);
    return true;
  }

  if (repl_input_matches(input, "%dump_ir")) {
    // dump(env);
    return true;
  }
  if (repl_input_matches(input, "%builtins")) {
    print_builtin_types();
    return true;
  }
  return false;
}

int orcjit(int argc, char **argv) {

  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  LLVMOrcLLJITRef jit = NULL;
  LLVMErrorRef err = LLVMOrcCreateLLJIT(&jit, NULL);
  if (err) {
    return consume_llvm_error(err, "Error creating LLJIT");
  }
  LLVMOrcJITDylibRef jd = LLVMOrcLLJITGetMainJITDylib(jit);

  LLVMOrcDefinitionGeneratorRef gen = NULL;
  err = LLVMOrcCreateDynamicLibrarySearchGeneratorForProcess(
      &gen, LLVMOrcLLJITGetGlobalPrefix(jit), NULL, NULL);

  if (err) {
    LLVMOrcDisposeLLJIT(jit);
    return consume_llvm_error(err, "Error creating process symbol generator");
  }

  LLVMOrcJITDylibAddGenerator(jd, gen);

  LLVMTargetMachineRef target_machine = create_target_machine(jit);
  if (!target_machine) {
    LLVMOrcDisposeLLJIT(jit);
    return 1;
  }

  TypeEnv *env = NULL;
  JITLangCtx ctx = {};
  ht table;
  StackFrame initial_stack_frame;
  lang_init(&table, env, &ctx, &initial_stack_frame);

  // Persistent MIR top-level scope: malloc-backed root frame that outlives
  // any single MirProgram, so top-level bindings (values, functions,
  // module members) accumulate across compile_script / compile_source
  // calls. Nested scopes stay program-arena-bound (see mir_scope_arena).
  ht mir_root_table;
  MirStackFrame mir_root_frame;
  mir_stack_frame_init(NULL, &mir_root_table, &mir_root_frame, NULL);

  if (!ylc_config.num_input_scripts && !ylc_config.interactive_mode) {
    LLVMDisposeTargetMachine(target_machine);
    LLVMOrcDisposeLLJIT(jit);
    return 0;
  }
  int result = 0;
  for (int i = 0; i < ylc_config.num_input_scripts; i++) {
    ORCCompiledModule compiled = compile_script(
        ylc_config.input_scripts[i], &env, &ctx, jit, target_machine,
        &mir_root_frame);
    if (ylc_config.test_mode) {
      printf("\n## Test %s\n", ylc_config.input_scripts[i]);
    }
    result = execute_module_top(compiled, jit, jd);
    if (result != 0) {
      break;
    }
  }
  if (ylc_config.interactive_mode) {
    init_readline();
  }

  int repl_counter = 0;
  const char *prompt = COLOR_RED "λ " COLOR_RESET COLOR_CYAN;
  while (ylc_config.interactive_mode) {
    char *input = repl_input(prompt);
    if (!input) {
      break;
    }

    if (repl_input_matches(input, "%quit")) {
      free(input);
      break;
    }

    if (handle_repl_command(input, env)) {
      free(input);
      continue;
    }

    char repl_filename[64];
    snprintf(repl_filename, sizeof(repl_filename), "<repl:%d>", repl_counter++);

    ORCCompiledModule compiled = compile_source(
        repl_filename, input, true, &env, &ctx, jit, target_machine,
        &mir_root_frame);
    free(input);
    if (!compiled.module) {
      continue;
    }

    int repl_result = execute_module_top(compiled, jit, jd);
    if (repl_result != 0) {
      fprintf(stderr, "REPL input failed\n");
    }
    printf("\n");
  }
  if (ylc_config.interactive_mode) {
    save_history();
  }

  LLVMDisposeTargetMachine(target_machine);
  LLVMOrcDisposeLLJIT(jit);
  return result;
}
