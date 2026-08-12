#include "lowering.h"

#include "audio_jit_symbols.h"
#include "backend_llvm/builtin_functions.h"
#include "backend_llvm/globals.h"
#include "backend_llvm/lib_registry.h"
#include "backend_llvm/module.h"
#include "config.h"
#include "input.h"
#include "mir/mir.h"
#include "modules.h"
#include "parse.h"
#include "types/builtins.h"
#include "types/inference.h"
#include "llvm/lowering.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Transforms/PassBuilder.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define YLC_SCRIPT_GLOBAL_STORAGE_CAPACITY 1024

void *global_storage_array[YLC_SCRIPT_GLOBAL_STORAGE_CAPACITY];
int global_storage_size = YLC_SCRIPT_GLOBAL_STORAGE_CAPACITY;
static int ylc_script_num_globals = 0;

#ifndef YLC_CLAP_AUDIO_JIT_BITCODE_PATH
#define YLC_CLAP_AUDIO_JIT_BITCODE_PATH "libs/audio_jit/build/osc_kernels.bc"
#endif

void ylc_audio_jit_register_current_program(const char *osc_bitcode_path);

static const char *ylc_lower_audio_jit_bitcode_path = NULL;

static const char *ylc_lower_get_audio_jit_bitcode_path(void) {
  const char *env_path = getenv("YLC_AUDIO_JIT_BITCODE");
  if (env_path && env_path[0] != '\0') {
    return env_path;
  }
  return YLC_CLAP_AUDIO_JIT_BITCODE_PATH;
}

static const char *ylc_lower_get_default_base_libs_dir(void) {
  const char *env_path = getenv("YLC_BASE_DIR");
  if (env_path && env_path[0] != '\0') {
    return env_path;
  }

  static char repo_root[1024];
  if (repo_root[0] != '\0') {
    return repo_root;
  }

  const char *bitcode_path = ylc_lower_get_audio_jit_bitcode_path();
  const char suffix[] = "/libs/audio_jit/build/osc_kernels.bc";
  const char *suffix_at = bitcode_path ? strstr(bitcode_path, suffix) : NULL;
  if (suffix_at) {
    const size_t len = (size_t)(suffix_at - bitcode_path);
    if (len > 0 && len < sizeof(repo_root)) {
      memcpy(repo_root, bitcode_path, len);
      repo_root[len] = '\0';
      return repo_root;
    }
  }

  return NULL;
}

static void ylc_lower_register_audio_jit(MirProgram *program, MirCtx *ctx) {
  ylc_audio_jit_register_current_program(ylc_lower_audio_jit_bitcode_path);
  ylc_clap_register_audio_jit_symbols(program, ctx);
}

static void ylc_lower_set_error(char *error, size_t error_size,
                                const char *format, ...) {
  if (!error || error_size == 0 || !format) {
    return;
  }

  va_list args;
  va_start(args, format);
  vsnprintf(error, error_size, format, args);
  va_end(args);
}

static LLVMValueRef ylc_lower_declare_install_helper(LLVMModuleRef module) {
  LLVMValueRef helper =
      LLVMGetNamedFunction(module, "ylc_plugin_install_dummy_jit_program");
  if (helper) {
    return helper;
  }

  LLVMContextRef context = LLVMGetModuleContext(module);
  LLVMTypeRef params[] = {LLVMPointerType(LLVMInt8TypeInContext(context), 0)};
  LLVMTypeRef type =
      LLVMFunctionType(LLVMVoidTypeInContext(context), params, 1, false);
  return LLVMAddFunction(module, "ylc_plugin_install_dummy_jit_program", type);
}

void module_passes(LLVMModuleRef module, LLVMTargetMachineRef target_machine) {
  LLVMPassBuilderOptionsRef options = LLVMCreatePassBuilderOptions();

  const char *opt_passes;
  if (ylc_config.debug_symbols) {
    opt_passes = "default<O0>";
  } else {
    opt_passes = ylc_config.opt_level ? ylc_config.opt_level : "default<O3>";
  }

  LLVMErrorRef err = LLVMRunPasses(module, opt_passes, target_machine, options);
  if (err) {
    char *message = LLVMGetErrorMessage(err);
    fprintf(stderr, "ERROR: Optimization failed: %s\n",
            message ? message : "unknown LLVM error");
    LLVMDisposeErrorMessage(message);
    LLVMConsumeError(err);
  }

  LLVMDisposePassBuilderOptions(options);
}

bool ylc_lower_dummy_installer_module(ylc_orc_session_t *orc,
                                      uint64_t module_id, char *installer_name,
                                      size_t installer_name_size,
                                      LLVMContextRef *context_out,
                                      LLVMModuleRef *module_out, char *error,
                                      size_t error_size) {
  if (!orc || !installer_name || installer_name_size == 0 || !context_out ||
      !module_out) {
    ylc_lower_set_error(error, error_size, "invalid dummy lowering request");
    return false;
  }

  snprintf(installer_name, installer_name_size, "__ylc_install_dummy_jit.%llu",
           (unsigned long long)module_id);

  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module =
      LLVMModuleCreateWithNameInContext("ylc_clap_dummy_program", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
  if (!context || !module || !builder) {
    if (builder) {
      LLVMDisposeBuilder(builder);
    }
    if (module) {
      LLVMDisposeModule(module);
    }
    if (context) {
      LLVMContextDispose(context);
    }
    ylc_lower_set_error(error, error_size, "failed creating LLVM IR module");
    return false;
  }

  LLVMSetTarget(module, ylc_orc_session_triple(orc));
  LLVMSetDataLayout(module, ylc_orc_session_data_layout(orc));

  LLVMTypeRef opaque_ptr = LLVMPointerType(LLVMInt8TypeInContext(context), 0);
  LLVMTypeRef installer_params[] = {opaque_ptr};
  LLVMTypeRef installer_type = LLVMFunctionType(LLVMVoidTypeInContext(context),
                                                installer_params, 1, false);
  LLVMValueRef installer =
      LLVMAddFunction(module, installer_name, installer_type);
  LLVMSetLinkage(installer, LLVMExternalLinkage);

  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(context, installer, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef plugin_state = LLVMGetParam(installer, 0);
  LLVMValueRef helper = ylc_lower_declare_install_helper(module);
  LLVMBuildCall2(builder, LLVMGlobalGetValueType(helper), helper, &plugin_state,
                 1, "");
  LLVMBuildRetVoid(builder);

  char *verify_error = NULL;
  if (LLVMVerifyModule(module, LLVMReturnStatusAction, &verify_error) != 0) {
    ylc_lower_set_error(error, error_size, "LLVM IR verification failed: %s",
                        verify_error ? verify_error
                                     : "unknown verification error");
    LLVMDisposeMessage(verify_error);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(context);
    return false;
  }
  LLVMDisposeMessage(verify_error);
  LLVMDisposeBuilder(builder);

  *context_out = context;
  *module_out = module;
  return true;
}

static void ylc_lower_init_lang_context(JITLangCtx *ctx, ht *table,
                                        StackFrame *frame) {
  init_module_registry();
  initialize_builtin_types();
  ht_init(table);
  *frame = (StackFrame){.table = table, .next = NULL};
  ylc_script_num_globals = 0;
  *ctx = (JITLangCtx){
      .stack_ptr = 0,
      .env = NULL,
      .num_globals = &ylc_script_num_globals,
      .global_storage_array = global_storage_array,
      .global_storage_capacity = &global_storage_size,
      .frame = frame,
  };
  initialize_builtin_funcs(ctx, NULL, NULL);
}

static bool ylc_lower_dispose_script_artifacts(
    LLVMContextRef context, LLVMModuleRef module, LLVMBuilderRef builder,
    MirProgram *mir_program, MirArena *mir_arena, MirArena *durable_arena,
    ht *durable_builtins) {
  if (builder) {
    LLVMDisposeBuilder(builder);
  }
  if (module) {
    LLVMDisposeModule(module);
  }
  if (context) {
    LLVMContextDispose(context);
  }
  if (mir_program) {
    mir_program_destroy(mir_program);
  }
  if (mir_arena) {
    mir_arena_destroy(mir_arena);
  }
  if (durable_builtins) {
    mir_durable_builtins_destroy(durable_builtins);
  }
  if (durable_arena) {
    mir_arena_destroy(durable_arena);
  }
  return false;
}

static bool ylc_lower_verify_module(LLVMModuleRef module, char *error,
                                    size_t error_size) {
  char *verify_error = NULL;
  if (LLVMVerifyModule(module, LLVMReturnStatusAction, &verify_error) != 0) {
    ylc_lower_set_error(error, error_size, "LLVM IR verification failed: %s",
                        verify_error ? verify_error
                                     : "unknown verification error");
    LLVMDisposeMessage(verify_error);
    return false;
  }
  LLVMDisposeMessage(verify_error);
  return true;
}

static void ylc_lower_internalize_script_symbols(LLVMModuleRef module,
                                                 LLVMValueRef entry) {
  if (!module || !entry) {
    return;
  }

  for (LLVMValueRef fn = LLVMGetFirstFunction(module); fn;
       fn = LLVMGetNextFunction(fn)) {
    if (fn == entry || LLVMCountBasicBlocks(fn) == 0) {
      continue;
    }

    LLVMSetLinkage(fn, LLVMInternalLinkage);
  }
}

static void ylc_lower_log_line(ylc_compile_log_fn log_fn, void *user_data,
                               const char *line) {
  if (log_fn && line) {
    log_fn(user_data, line);
  }
}

static void ylc_lower_log_text(ylc_compile_log_fn log_fn, void *user_data,
                               const char *text) {
  if (!log_fn || !text) {
    return;
  }

  const char *line = text;
  while (*line) {
    const char *end = strchr(line, '\n');
    const char *start = line;
    size_t len = end ? (size_t)(end - line) : strlen(line);
    while (len > 0) {
      const size_t chunk_len = len > 900 ? 900 : len;
      char chunk[901];
      memcpy(chunk, line, chunk_len);
      chunk[chunk_len] = '\0';
      ylc_lower_log_line(log_fn, user_data, chunk);
      line += chunk_len;
      len -= chunk_len;
    }
    if (end) {
      if (end == start) {
        ylc_lower_log_line(log_fn, user_data, "");
      }
      line = end + 1;
    } else {
      break;
    }
  }
}

static void ylc_lower_dump_mir_to_log(MirProgram *program,
                                      ylc_compile_log_fn log_fn,
                                      void *log_user_data) {
  if (!program || !log_fn) {
    return;
  }

  char *dump = NULL;
  size_t dump_size = 0;
  FILE *stream = open_memstream(&dump, &dump_size);
  if (!stream) {
    ylc_lower_log_line(log_fn, log_user_data,
                       "----- YLC SCRIPT MIR DUMP FAILED -----");
    return;
  }

  mir_dump_program(program, stream);
  fclose(stream);

  ylc_lower_log_line(log_fn, log_user_data, "----- YLC SCRIPT MIR BEGIN -----");
  ylc_lower_log_text(log_fn, log_user_data, dump ? dump : "");
  ylc_lower_log_line(log_fn, log_user_data, "----- YLC SCRIPT MIR END -----");
  free(dump);
}

static void ylc_lower_dump_llvm_to_log(LLVMModuleRef module,
                                       ylc_compile_log_fn log_fn,
                                       void *log_user_data) {
  if (!module || !log_fn) {
    return;
  }

  char *ir = LLVMPrintModuleToString(module);
  ylc_lower_log_line(log_fn, log_user_data,
                     "----- YLC SCRIPT LLVM IR BEGIN -----");
  ylc_lower_log_text(log_fn, log_user_data, ir ? ir : "");
  ylc_lower_log_line(log_fn, log_user_data,
                     "----- YLC SCRIPT LLVM IR END -----");
  if (ir) {
    LLVMDisposeMessage(ir);
  }
}

static char *ylc_lower_wrap_clap_script_module(char *source,
                                               uint64_t module_id) {
  char module_header[128] = {0};
  static const char module_footer[] = "\n;\n();\n";

  if (!source) {
    return NULL;
  }

  snprintf(module_header, sizeof(module_header),
           "let __ylc_clap_script_%llu = module () ->\n",
           (unsigned long long)module_id);

  const size_t header_len = strlen(module_header);
  const size_t source_len = strlen(source);
  const size_t footer_len = sizeof(module_footer) - 1;
  char *combined = (char *)malloc(header_len + source_len + footer_len + 1);
  if (!combined) {
    free(source);
    return NULL;
  }

  memcpy(combined, module_header, header_len);
  memcpy(combined + header_len, source, source_len);
  memcpy(combined + header_len + source_len, module_footer, footer_len + 1);
  free(source);
  return combined;
}

static ylc_script_entry_return_kind_t
ylc_lower_entry_return_kind(LLVMTypeRef fn_type, LLVMContextRef context) {
  if (!fn_type || LLVMGetTypeKind(fn_type) != LLVMFunctionTypeKind) {
    return YLC_SCRIPT_ENTRY_RET_UNSUPPORTED;
  }

  LLVMTypeRef ret = LLVMGetReturnType(fn_type);
  if (ret == LLVMVoidTypeInContext(context)) {
    return YLC_SCRIPT_ENTRY_RET_VOID;
  }
  if (ret == LLVMInt32TypeInContext(context)) {
    return YLC_SCRIPT_ENTRY_RET_I32;
  }
  if (ret == LLVMDoubleTypeInContext(context)) {
    return YLC_SCRIPT_ENTRY_RET_DOUBLE;
  }
  if (LLVMGetTypeKind(ret) == LLVMPointerTypeKind) {
    return YLC_SCRIPT_ENTRY_RET_PTR;
  }

  return YLC_SCRIPT_ENTRY_RET_UNSUPPORTED;
}

bool ylc_lower_script_file(ylc_orc_session_t *orc, uint64_t module_id,
                           const char *script_path,
                           ylc_lowered_script_t *compiled, char *error,
                           size_t error_size, ylc_compile_log_fn log_fn,
                           void *log_user_data) {
  if (!orc || !script_path || script_path[0] == '\0' || !compiled) {
    ylc_lower_set_error(error, error_size, "invalid script lowering request");
    return false;
  }

  memset(compiled, 0, sizeof(*compiled));

  char *source = read_script(script_path);
  if (!source) {
    ylc_lower_set_error(error, error_size, "failed reading script: %s",
                        script_path);
    return false;
  }
  source = ylc_lower_wrap_clap_script_module(source, module_id);
  if (!source) {
    ylc_lower_set_error(error, error_size,
                        "failed wrapping CLAP script module: %s", script_path);
    return false;
  }

  LLVMContextRef context = LLVMContextCreate();
  LLVMModuleRef module =
      LLVMModuleCreateWithNameInContext("ylc_clap_script", context);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
  if (!context || !module || !builder) {
    free(source);
    ylc_lower_set_error(error, error_size, "failed creating LLVM module");
    return ylc_lower_dispose_script_artifacts(context, module, builder, NULL,
                                              NULL, NULL, NULL);
  }

  LLVMSetTarget(module, ylc_orc_session_triple(orc));
  LLVMSetDataLayout(module, ylc_orc_session_data_layout(orc));
  setup_global_storage(module, builder);
  LLVMSetSourceFileName(module, script_path, strlen(script_path));

  const char *saved_base_libs_dir = ylc_config.base_libs_dir;
  const char *saved_import_current_dir = ylc_config.import_current_dir;
  const bool saved_interactive_mode = ylc_config.interactive_mode;
  const bool saved_test_mode = ylc_config.test_mode;
  ylc_config.base_libs_dir = saved_base_libs_dir
                                 ? saved_base_libs_dir
                                 : ylc_lower_get_default_base_libs_dir();
  ylc_config.interactive_mode = false;
  ylc_config.test_mode = false;

  char *script_dir = get_dirname(script_path);
  ylc_config.import_current_dir = script_dir;
  module_path = script_path;

  ht lang_table;
  StackFrame lang_frame;
  JITLangCtx lang_ctx;
  ylc_lower_init_lang_context(&lang_ctx, &lang_table, &lang_frame);
  lang_ctx.module_name = script_path;

  Ast *ast = parse_input_buffer(script_path, source);
  if (!ast) {
    ylc_lower_set_error(error, error_size, "parse failed: %s", script_path);
    free(script_dir);
    ylc_config.base_libs_dir = saved_base_libs_dir;
    ylc_config.import_current_dir = saved_import_current_dir;
    ylc_config.interactive_mode = saved_interactive_mode;
    ylc_config.test_mode = saved_test_mode;
    return ylc_lower_dispose_script_artifacts(context, module, builder, NULL,
                                              NULL, NULL, NULL);
  }

  TICtx type_ctx = {.env = NULL, .scope = 0, .err_stream = stderr};
  if (!infer(ast, &type_ctx)) {
    ylc_lower_set_error(error, error_size, "typecheck failed: %s", script_path);
    free(script_dir);
    ylc_config.base_libs_dir = saved_base_libs_dir;
    ylc_config.import_current_dir = saved_import_current_dir;
    ylc_config.interactive_mode = saved_interactive_mode;
    ylc_config.test_mode = saved_test_mode;
    return ylc_lower_dispose_script_artifacts(context, module, builder, NULL,
                                              NULL, NULL, NULL);
  }
  lang_ctx.env = type_ctx.env;

  MirArena *mir_arena = mir_arena_create();
  MirArena *durable_arena = mir_arena_create();
  ht *durable_builtins = mir_durable_builtins_create();
  ht mir_root_table;
  MirStackFrame mir_root_frame;
  mir_stack_frame_init(NULL, &mir_root_table, &mir_root_frame, NULL);
  mir_root_frame.durable_arena = durable_arena;
  mir_root_frame.durable_builtins = durable_builtins;
  MirCtx mir_ctx = {.env = type_ctx.env, .frame = &mir_root_frame};
  YlcMirProgramInitFn saved_mir_program_init_fn = ylc_mir_program_init_fn;
  const char *saved_audio_jit_bitcode_path = ylc_lower_audio_jit_bitcode_path;
  ylc_lower_audio_jit_bitcode_path = ylc_lower_get_audio_jit_bitcode_path();
  ylc_mir_program_init_fn = ylc_lower_register_audio_jit;
  MirProgram *mir_program = mir_build_program(mir_arena, ast, &mir_ctx);
  ylc_mir_program_init_fn = saved_mir_program_init_fn;
  ylc_lower_audio_jit_bitcode_path = saved_audio_jit_bitcode_path;
  if (mir_program_had_error(mir_program)) {
    ylc_lower_set_error(error, error_size, "MIR build failed: %s", script_path);
    free(script_dir);
    ylc_config.base_libs_dir = saved_base_libs_dir;
    ylc_config.import_current_dir = saved_import_current_dir;
    ylc_config.interactive_mode = saved_interactive_mode;
    ylc_config.test_mode = saved_test_mode;
    return ylc_lower_dispose_script_artifacts(context, module, builder,
                                              mir_program, mir_arena,
                                              durable_arena, durable_builtins);
  }

  mir_run_passes(mir_program);
  if (mir_program_had_error(mir_program)) {
    ylc_lower_set_error(error, error_size, "MIR passes failed: %s",
                        script_path);
    free(script_dir);
    ylc_config.base_libs_dir = saved_base_libs_dir;
    ylc_config.import_current_dir = saved_import_current_dir;
    ylc_config.interactive_mode = saved_interactive_mode;
    ylc_config.test_mode = saved_test_mode;
    return ylc_lower_dispose_script_artifacts(context, module, builder,
                                              mir_program, mir_arena,
                                              durable_arena, durable_builtins);
  }
  if (ylc_config.dump_mir) {
    mir_dump_program(mir_program, stdout);
  }
  ylc_lower_dump_mir_to_log(mir_program, log_fn, log_user_data);

  LLVMValueRef top = lower_mir(mir_program, module, builder);
  if (!top) {
    ylc_lower_set_error(error, error_size, "LLVM lowering failed: %s",
                        script_path);
    free(script_dir);
    ylc_config.base_libs_dir = saved_base_libs_dir;
    ylc_config.import_current_dir = saved_import_current_dir;
    ylc_config.interactive_mode = saved_interactive_mode;
    ylc_config.test_mode = saved_test_mode;
    return ylc_lower_dispose_script_artifacts(context, module, builder,
                                              mir_program, mir_arena,
                                              durable_arena, durable_builtins);
  }

  snprintf(compiled->entry_name, sizeof(compiled->entry_name),
           "__ylc_script_top.%llu", (unsigned long long)module_id);
  LLVMSetValueName2(top, compiled->entry_name, strlen(compiled->entry_name));
  LLVMSetLinkage(top, LLVMExternalLinkage);
  ylc_lower_internalize_script_symbols(module, top);

  LLVMTypeRef top_type = LLVMGlobalGetValueType(top);
  compiled->entry_return_kind = ylc_lower_entry_return_kind(top_type, context);
  compiled->entry_returns_void =
      compiled->entry_return_kind == YLC_SCRIPT_ENTRY_RET_VOID;

  if (!ylc_lower_verify_module(module, error, error_size)) {
    free(script_dir);
    ylc_config.base_libs_dir = saved_base_libs_dir;
    ylc_config.import_current_dir = saved_import_current_dir;
    ylc_config.interactive_mode = saved_interactive_mode;
    ylc_config.test_mode = saved_test_mode;
    return ylc_lower_dispose_script_artifacts(context, module, builder,
                                              mir_program, mir_arena,
                                              durable_arena, durable_builtins);
  }

  module_passes(module, ylc_orc_session_target_machine(orc));
  ylc_lower_dump_llvm_to_log(module, log_fn, log_user_data);

  LLVMDisposeBuilder(builder);
  mir_program_destroy(mir_program);
  mir_arena_destroy(mir_arena);
  mir_durable_builtins_destroy(durable_builtins);
  mir_arena_destroy(durable_arena);

  compiled->context = context;
  compiled->module = module;

  free(script_dir);
  ylc_config.base_libs_dir = saved_base_libs_dir;
  ylc_config.import_current_dir = saved_import_current_dir;
  ylc_config.interactive_mode = saved_interactive_mode;
  ylc_config.test_mode = saved_test_mode;
  free(source);
  return true;
}
