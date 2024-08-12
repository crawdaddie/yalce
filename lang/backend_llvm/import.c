#include "backend_llvm/import.h"
#include "codegen.h"
#include "input.h"
#include "serde.h"
#include "synths.h"
#include "types/inference.h"
#include "types/type.h"
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Linker.h"
#include <string.h>

LLVMGenericValueRef eval_script(const char *filename, JITLangCtx *ctx,
                                LLVMModuleRef llvm_module,
                                LLVMBuilderRef builder, LLVMContextRef llvm_ctx,
                                TypeEnv **env, Ast **prog);

void _import_module(char *dirname, Ast *import, TypeEnv **env, JITLangCtx *ctx,
                    LLVMModuleRef main_module, LLVMContextRef llvm_ctx) {

  const char *module_name = import->data.AST_IMPORT.module_name;
  uint64_t module_name_hash = hash_string(module_name, strlen(module_name));
  if (ht_get_hash(ctx->stack, module_name, module_name_hash)) {
    return;
  }

  int len =
      strlen(dirname) + 1 + strlen(module_name) + 4; // account for .ylc ext
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

  eval_script(fully_qualified_name, &module_ctx, module, builder, llvm_ctx,
              &module_type_env, &ast_root);

  Type *module_type = malloc(sizeof(Type));
  module_type->kind = T_MODULE;
  module_type->data.T_MODULE = module_type_env;

  *env = env_extend(*env, module_name, module_type);

  stack = realloc(stack, sizeof(ht));

  // Link the imported module with the main module
  LLVMBool link_result = LLVMLinkModules2(main_module, module);
  JITSymbol *sym = malloc(sizeof(JITSymbol));

  *sym = (JITSymbol){STYPE_MODULE,
                     .symbol_data = {.STYPE_MODULE = {.symbols = stack}}};

  ht_set_hash(ctx->stack, module_name, module_name_hash, sym);
}

Ast *parse_module_type(Ast *binding, Ast *import,
                       const char *fully_qualified_name, TypeEnv **env) {

  char *fcontent = read_script(fully_qualified_name);
  Ast *mod_root = parse_input(fcontent);

  *env = initialize_type_env(*env);
  *env = initialize_type_env_synth(*env);
  infer_ast(env, mod_root);

  Type *mod_type = malloc(sizeof(Type));
  mod_type->kind = T_MODULE;
  mod_type->data.T_MODULE = ht_create();
  ht_init(mod_type->data.T_MODULE);

  for (int i = 0; i < mod_root->data.AST_BODY.len; i++) {
    Ast *top_level = *(mod_root->data.AST_BODY.stmts + i);
    if (top_level->tag == AST_LET) {
      Ast *binding = top_level->data.AST_LET.binding;

      if (binding->tag == AST_IDENTIFIER) {
        uint64_t hash = hash_string(binding->data.AST_IDENTIFIER.value,
                                    binding->data.AST_IDENTIFIER.length);
        ht_set_hash(mod_type->data.T_MODULE, binding->data.AST_IDENTIFIER.value,
                    hash, top_level->md);
      }
    }
  }

  mod_type->alias = fully_qualified_name;

  mod_root->md = mod_type;
  return mod_root;
}

ht modules;

typedef struct YalceModule {
  Type *type;
  const char *path;
} YalceModule;

void compile_import(Ast *module_binding, const char *fully_qualified_name,
                    Ast *module_ast, TypeEnv *env, JITLangCtx *ctx,
                    LLVMContextRef llvm_ctx, LLVMModuleRef main_module) {

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

  codegen(module_ast, &module_ctx, module, builder);
  stack = realloc(stack, sizeof(ht));

  // Link the imported module with the main module
  LLVMBool link_result = LLVMLinkModules2(main_module, module);
  printf("compiled module %s\n", fully_qualified_name);
  LLVMDumpModule(main_module);


  JITSymbol *sym = malloc(sizeof(JITSymbol));

  *sym = (JITSymbol){STYPE_MODULE,
                     .symbol_data = {.STYPE_MODULE = {.symbols = stack}}};
  const char *module_binding_name = module_binding->data.AST_IDENTIFIER.value;
  uint64_t module_binding_name_hash = hash_string(
      module_binding_name, module_binding->data.AST_IDENTIFIER.length);
  ht_set_hash(ctx->stack, module_binding_name, module_binding_name_hash, sym);
}

void import_module(Ast *binding, Ast *import_name, const char *dirname,
                   JITLangCtx *ctx, LLVMModuleRef llvm_module,
                   LLVMBuilderRef builder, LLVMContextRef llvm_ctx) {

  const char *module_name = import_name->data.AST_IMPORT.module_name;
  int len =
      strlen(dirname) + 1 + strlen(module_name) + 4; // account for .ylc ext
  char *fully_qualified_name = malloc(sizeof(char) * len);
  snprintf(fully_qualified_name, len + 1, "%s/%s.ylc", dirname, module_name);

  uint64_t fully_qualiified_name_hash = hash_string(fully_qualified_name, len);

  YalceModule *existing_mod =
      ht_get_hash(&modules, fully_qualified_name, fully_qualiified_name_hash);

  if (existing_mod != NULL) {
    printf("module exists!");
    binding->md = existing_mod->type;
    import_name->md = existing_mod->type;
    return;
  }
  TypeEnv *env = NULL;

  Ast *mod_ast =
      parse_module_type(binding, import_name, fully_qualified_name, &env);

  // this will apply type information to the AST
  binding->md = mod_ast->md;
  import_name->md = mod_ast->md;

  YalceModule *mod = malloc(sizeof(YalceModule));
  mod->type = mod_ast->md;
  mod->path = fully_qualified_name;

  compile_import(binding, fully_qualified_name, mod_ast, env, ctx, llvm_ctx,
                 llvm_module);
  free_type_env(env);
}
