#include "backend_llvm/jit.h"
#include "backend_llvm/common.h"
#include "codegen.h"
#include "codegen_globals.h"
#include "codegen_types.h"
#include "format_utils.h"
#include "import.h"
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

LLVMGenericValueRef eval_script(const char *filename, JITLangCtx *ctx,
                                LLVMModuleRef llvm_module,
                                LLVMBuilderRef builder, LLVMContextRef llvm_ctx,
                                TypeEnv **env, Ast **prog) {

  char *fcontent = read_script(filename);
  LLVMSetSourceFileName(llvm_module, filename, strlen(filename));
  if (!fcontent) {
    return NULL;
  }

  const char *dirname = get_dirname(filename);
  if (dirname == NULL) {
    fprintf(stderr, "Error: cannot derive dirname for %s\n", filename);
    return NULL;
  }

  *prog = parse_input(fcontent);
  for (int i = 0; i < (*prog)->data.AST_BODY.len; i++) {
    Ast *tl = *((*prog)->data.AST_BODY.stmts + i);
    if (tl->tag == AST_LET && tl->data.AST_LET.expr->tag == AST_IMPORT) {
      // pre-process / hoist import statements
      Ast *binding_ast = tl->data.AST_LET.binding;
      Ast *import_ast = tl->data.AST_LET.expr;
      import_module(binding_ast, import_ast, dirname, ctx, llvm_module, builder,
                    llvm_ctx);
    }
  }

#ifdef DUMP_AST
  print_ast(*prog);
#endif

  if (!(*prog)) {
    return NULL;
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
      codegen_top_level(*prog, &top_level_ret_type, ctx, llvm_module, builder);

  LLVMExecutionEngineRef engine;
  prepare_ex_engine(ctx, &engine, llvm_module);

  if (top_level_func == NULL) {
    printf("> ");
    print_result(result_type, NULL);
    free(fcontent);
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

  ht_init(&modules);
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
      } else if (strncmp("%dump_type_env", input, 14) == 0) {
        print_type_env(env);
        continue;
      } else if (strncmp("%dump_ast", input, 9) == 0) {
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
    printf(" %d", (int)LLVMGenericValueToInt(result, 0));
    break;
  }

  case T_NUM: {
    printf(" %f", (double)LLVMGenericValueToFloat(LLVMDoubleType(), result));
    break;
  }

  case T_STRING: {
    printf(" %s", (char *)LLVMGenericValueToPointer(result));
    break;
  }

  case T_CHAR: {
    printf(" %c", (int)LLVMGenericValueToInt(result, 0));
    break;
  }

  case T_CONS: {
    if (is_string_type(type)) {
      printf(" %s", (char *)LLVMGenericValueToPointer(result));
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
      printf("]");
      break;
    }

    break;
  }

  case T_FN: {
    printf(" %p", result);
    break;
  }

  default:
    printf(" %d", (int)LLVMGenericValueToInt(result, 0));
    break;
  }
  printf("\n");
}
