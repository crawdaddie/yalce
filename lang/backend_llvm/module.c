#include "backend_llvm/module.h";
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include <stdlib.h>

static LLVMGenericValueRef eval_script(const char *filename, JITLangCtx *ctx,
                                       LLVMModuleRef module,
                                       LLVMBuilderRef builder,
                                       LLVMContextRef llvm_ctx, TypeEnv **env,
                                       Ast **prog);

void import_module(char *dirname, Ast *import, TypeEnv **env, JITLangCtx *ctx,
                   LLVMModuleRef main_module, LLVMContextRef llvm_ctx) {

  const char *module_name = import->data.AST_IMPORT.module_name;
  uint64_t module_name_hash = hash_string(module_name, strlen(module_name));
  if (ht_get_hash(ctx->stack, module_name, module_name_hash)) {
    return;
  }

  int len = strlen(dirname) + 1 + strlen(module_name) + 4;
  char *fully_qualified_name = malloc(sizeof(char) * len);
  snprintf(fully_qualified_name, len + 1, "%s/%s.ylc", dirname, module_name);

  LLVMModuleRef module =
      LLVMModuleCreateWithNameInContext(fully_qualified_name, llvm_ctx);

  LLVMBuilderRef builder = LLVMCreateBuilderInContext(llvm_ctx);

  ht *stack = malloc(sizeof(ht) * STACK_MAX);

  for (int i = 0; i < STACK_MAX; i++) {
    ht_init(stack + i);
  }
  JITLangCtx module_ctx = {
      .stack = stack,
      .stack_ptr = 0,
      .env = ctx->env,
      .num_globals = ctx->num_globals,
      .global_storage_array = ctx->global_storage_array,
      .global_storage_capacity = ctx->global_storage_capacity,
  };
  TypeEnv *module_type_env = NULL;

  // eval_script(fully_qualified_name, &module_ctx, module, builder, llvm_ctx,
  //             &module_type_env, &ast_root);
  //
  // Type *module_type = malloc(sizeof(Type));
  // module_type->kind = T_MODULE;
  // module_type->data.T_MODULE = module_type_env;
  //
  // *env = env_extend(*env, module_name, module_type);
  //
  // stack = realloc(stack, sizeof(ht));
  // // Link the imported module with the main module
  // LLVMBool link_result = LLVMLinkModules2(main_module, module);
  // JITSymbol *sym = malloc(sizeof(JITSymbol));
  //
  // *sym = (JITSymbol){STYPE_MODULE,
  //                    .symbol_data = {.STYPE_MODULE = {.symbols = stack}}};
  //
  // ht_set_hash(ctx->stack, module_name, module_name_hash, sym);
}
