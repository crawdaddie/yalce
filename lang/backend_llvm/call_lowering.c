#include "./call_lowering.h"
#include "application.h"
#include "closures.h"
#include "codegen.h"
#include "types.h"
#include "types/type_ser.h"
#include <stdlib.h>
#include <string.h>

static int flattened_arg_count(Ast *ast) {
  if (!ast || ast->tag != AST_APPLICATION) {
    return 0;
  }

  int nested = 0;
  if (ast->data.AST_APPLICATION.function &&
      ast->data.AST_APPLICATION.function->tag == AST_APPLICATION) {
    nested = flattened_arg_count(ast->data.AST_APPLICATION.function);
  }

  return nested + (int)ast->data.AST_APPLICATION.len;
}

static Ast *flattened_function(Ast *ast) {
  Ast *fn = ast;
  while (fn && fn->tag == AST_APPLICATION) {
    fn = fn->data.AST_APPLICATION.function;
  }
  return fn;
}

static int collect_flattened_args(Ast *ast, Ast **args, int offset) {
  if (!ast || ast->tag != AST_APPLICATION) {
    return offset;
  }

  if (ast->data.AST_APPLICATION.function &&
      ast->data.AST_APPLICATION.function->tag == AST_APPLICATION) {
    offset = collect_flattened_args(ast->data.AST_APPLICATION.function, args,
                                    offset);
  }

  for (size_t i = 0; i < ast->data.AST_APPLICATION.len; i++) {
    args[offset++] = ast->data.AST_APPLICATION.args + i;
  }

  return offset;
}

FlatApplication flatten_application(Ast *app) {
  FlatApplication flat = {.function = app, .args = NULL, .num_args = 0};

  if (!app || app->tag != AST_APPLICATION) {
    return flat;
  }

  flat.function = flattened_function(app);
  flat.num_args = flattened_arg_count(app);

  if (flat.num_args > 0) {
    flat.args = malloc(sizeof(Ast *) * (size_t)flat.num_args);
    collect_flattened_args(app, flat.args, 0);
  }

  return flat;
}

void free_flat_application(FlatApplication *app) {
  if (!app) {
    return;
  }

  free(app->args);
  app->args = NULL;
  app->num_args = 0;
}

int callable_arg_count(Type *type) {
  int count = 0;
  for (Type *t = type; t && t->kind == T_FN; t = t->data.T_FN.to) {
    count++;
  }
  return count;
}

Type *callable_arg_type(Type *type, int index) {
  Type *t = type;
  for (int i = 0; t && t->kind == T_FN; t = t->data.T_FN.to, i++) {
    if (i == index) {
      return t->data.T_FN.from;
    }
  }
  return NULL;
}

Type *callable_return_type_after_n_args(Type *type, int num_args) {
  Type *t = type;
  for (int i = 0; t && t->kind == T_FN && i < num_args; i++) {
    t = t->data.T_FN.to;
  }
  return t;
}

LoweredCallable lower_callable_value(LLVMValueRef callable, Type *callable_type,
                                     JITLangCtx *ctx, LLVMModuleRef module,
                                     LLVMBuilderRef builder) {
  (void)builder;

  LoweredCallable lowered = {
      .fn = callable, .env = NULL, .type = callable_type, .has_env = false};

  if (!callable || !callable_type || !is_closure(callable_type)) {
    return lowered;
  }

  if (LLVMGetTypeKind(LLVMTypeOf(callable)) != LLVMStructTypeKind) {
    return lowered;
  }

  lowered.fn = LLVMBuildExtractValue(builder, callable, 0, "closure_fn_value");
  lowered.env =
      LLVMBuildExtractValue(builder, callable, 1, "closure_env_value");
  lowered.type = deep_copy_type(callable_type);
  lowered.type->closure_meta = NULL;
  lowered.has_env = true;
  return lowered;
}

LLVMTypeRef lowered_callable_llvm_type(LoweredCallable callable,
                                       Type *original_callable_type,
                                       JITLangCtx *ctx, LLVMModuleRef module) {
  if (callable.has_env) {
    LLVMTypeRef llvm_rec_type =
        closure_record_type(original_callable_type, ctx, module);
    return closure_fn_type(original_callable_type, llvm_rec_type, ctx, module);
  }

  return type_to_llvm_type(callable.type, ctx, module);
}

#define is_ident(f, name) strcmp(f->data.AST_IDENTIFIER.value, name) == 0

LLVMValueRef emit_lowered_call(Ast *app, LoweredCallable callable,
                               Type *original_callable_type, JITLangCtx *ctx,
                               LLVMModuleRef module, LLVMBuilderRef builder) {
  if (!app || app->tag != AST_APPLICATION) {
    return callable.fn;
  }

  size_t num_src_args = app->data.AST_APPLICATION.args->tag == AST_VOID
                            ? 0
                            : app->data.AST_APPLICATION.len;
  size_t num_call_args = num_src_args + (callable.has_env ? 1u : 0u);

  LLVMValueRef arg_vals[num_call_args ? num_call_args : 1];

  size_t arg_offset = 0;
  if (callable.has_env) {
    LLVMTypeRef llvm_rec_type =
        closure_record_type(original_callable_type, ctx, module);
    arg_vals[0] = LLVMBuildBitCast(builder, callable.env,
                                   LLVMPointerType(llvm_rec_type, 0), "env");
    arg_offset = 1;
  }

  for (size_t i = 0; i < num_src_args; i++) {
    Ast *arg_ast = app->data.AST_APPLICATION.args + i;
    Type *to_type = callable_arg_type(callable.type, (int)i);

    LLVMValueRef val = codegen(arg_ast, ctx, module, builder);

    Type *from_type = specialize_type_for_codegen(arg_ast->type, ctx);
    if (to_type) {
      to_type = specialize_type_for_codegen(to_type, ctx);
    }
    val =
        handle_type_conversions(val, from_type, to_type, ctx, module, builder);

    arg_vals[arg_offset + i] = val;
  }

  LLVMTypeRef llvm_callable_type =
      lowered_callable_llvm_type(callable, original_callable_type, ctx, module);

  char name[32];
  if (app->data.AST_APPLICATION.function->tag == AST_IDENTIFIER) {
    snprintf(name, 32, "call.%s",
             app->data.AST_APPLICATION.function->data.AST_IDENTIFIER.value);
  } else {
    sprintf(name, "call.record_member");
  }

  LLVMValueRef call = LLVMBuildCall2(builder, llvm_callable_type, callable.fn,
                                    arg_vals, (unsigned)num_call_args, name);

  // A tail call reuses the caller's stack frame, invalidating any stack
  // alloca whose address is passed as an argument before the callee reads it.
  // This is observed with asbytes results (heap-avoidance stack allocas) fed
  // to extern functions like hash_string: LLVM marks the call `tail` and the
  // buffer becomes garbage. Mark such calls notail. We check the LLVM value
  // provenance rather than AST ea_md, since the marker is on the allocation
  // site and not reachable from the argument AST.
  for (unsigned i = 0; i < (unsigned)num_call_args; i++) {
    if (LLVMGetTypeKind(LLVMTypeOf(arg_vals[i])) == LLVMPointerTypeKind) {
      LLVMSetTailCallKind(call, LLVMTailCallKindNoTail);
      break;
    }
  }
  return call;
}
