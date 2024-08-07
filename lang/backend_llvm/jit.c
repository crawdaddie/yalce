#include "backend_llvm/jit.h"
#include "backend_llvm/common.h"
#include "codegen.h"
#include "codegen_globals.h"
#include "codegen_types.h"
#include "format_utils.h"
#include "input.h"
#include "parse.h"
#include "serde.h"
#include "synths.h"
#include "types/inference.h"
#include "types/util.h"
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

static LLVMValueRef codegen_top_level(Ast *ast, LLVMTypeRef *ret_type,
                                      JITLangCtx *ctx, LLVMModuleRef module,
                                      LLVMBuilderRef builder) {

  // Create function type.
  LLVMTypeRef funcType =
      LLVMFunctionType(type_to_llvm_type(ast->md, ctx->env), NULL, 0, 0);

  // Create function.
  LLVMValueRef func = LLVMAddFunction(module, "top", funcType);

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

  *prog = parse_input(fcontent);

#ifdef DUMP_AST
  print_ast(*prog);
#endif

  char *dirname = get_dirname(filename);
  if (dirname == NULL) {
    return NULL;
  }
  if (!(*prog)) {
    return NULL;
  }
  for (int i = 0; i < (*prog)->data.AST_BODY.len; i++) {
    Ast *stmt = *((*prog)->data.AST_BODY.stmts + i);
    if (stmt->tag == AST_IMPORT) {
      ast_root = NULL;
      import_module(dirname, stmt, env, ctx, module, llvm_ctx);
    }
  }

  infer_ast(env, *prog);
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
    // free(fcontent);
    return NULL;
  }
  LLVMGenericValueRef exec_args[] = {};
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
  env = initialize_type_env(env);
  env = initialize_type_env_synth(env);

  JITLangCtx ctx = {.stack = stack,
                    .stack_ptr = 0,
                    .env = env,
                    .num_globals = &num_globals,
                    .global_storage_array = global_storage_array,
                    .global_storage_capacity = &global_storage_capacity};

  bool repl = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      repl = true;
    } else {
      Ast *script_prog;
      eval_script(argv[i], &ctx, module, builder, context, &env, &script_prog);
    }
  }

  if (repl) {

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
      // printf("key: '%s'\n", it.key);

      for (int completion_entry = 0; ht_next(&it); completion_entry++) {
        add_completion_item(it.key, completion_entry);
      };

      char *input = repl_input(prompt);

      if (strcmp("%dump_module\n", input) == 0) {
        printf(STYLE_RESET_ALL "\n");
        LLVMDumpModule(module);
        continue;
      } else if (strcmp("%dump_type_env\n", input) == 0) {
        print_type_env(env);
        continue;
      } else if (strcmp("%dump_ast\n", input) == 0) {
        print_ast(ast_root);
        continue;
      } else if (strcmp("\n", input) == 0) {
        continue;
      }

      Ast *prog = parse_input(input);
      Ast *top = top_level_ast(prog);
      Type *typecheck_result = infer_ast(&env, top);
      ctx.env = env;

      if (typecheck_result == NULL) {
        printf("typecheck failed\n");
        continue;
      }

      // Generate node.
      LLVMValueRef top_level_func =
          codegen_top_level(top, &top_level_ret_type, &ctx, module, builder);

#ifdef DUMP_AST
      printf("%s\n", COLOR_RESET);
      LLVMDumpModule(module);
#endif

      printf(COLOR_GREEN "> ");

      Type *top_type = top->md;

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
    printf("%s", type->alias);
  } else {
    print_type_w_tc(type);
  }

  if (result == NULL) {
    printf("\n");
    return;
  }
  switch (type->kind) {
  case T_INT: {
    printf(" %d\n", (int)LLVMGenericValueToInt(result, 0));
    break;
  }

  case T_NUM: {
    printf(" %f\n", (double)LLVMGenericValueToFloat(LLVMDoubleType(), result));
    break;
  }

  case T_STRING: {
    printf(" %s\n", (char *)LLVMGenericValueToPointer(result));
    break;
  }

  case T_CHAR: {
    printf(" %c\n", (int)LLVMGenericValueToInt(result, 0));
    break;
  }

  case T_CONS: {
    if (is_string_type(type)) {
      printf(" %s\n", (char *)LLVMGenericValueToPointer(result));
      break;
    }
    if (strcmp(type->data.T_CONS.name, "List") == 0 &&
        type->data.T_CONS.args[0]->kind == T_INT) {

      int_ll_t *current = (int_ll_t *)LLVMGenericValueToPointer(result);
      int count = 0;
      printf(" [");
      while (current != NULL && count < 10) { // Limit to prevent infinite loop
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
  printf("\n");
}
