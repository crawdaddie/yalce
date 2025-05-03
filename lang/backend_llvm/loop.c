#include "./loop.h"
#include "common.h"
#include "parse.h"
#include "serde.h"
#include "symbols.h"
#include "llvm-c/Core.h"

LLVMValueRef codegen(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                     LLVMBuilderRef builder);

LLVMValueRef codegen_loop_range(Ast *binding, Ast *range, Ast *body,
                                JITLangCtx *ctx, LLVMModuleRef module,
                                LLVMBuilderRef builder) {

  LLVMValueRef current_function =
      LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
  const char *loop_var_name = binding->data.AST_IDENTIFIER.value;

  LLVMBasicBlockRef entry_block = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef cond_block =
      LLVMAppendBasicBlock(current_function, "loop.cond");
  LLVMBasicBlockRef body_block =
      LLVMAppendBasicBlock(current_function, "loop.body");
  LLVMBasicBlockRef inc_block =
      LLVMAppendBasicBlock(current_function, "loop.inc");
  LLVMBasicBlockRef after_block =
      LLVMAppendBasicBlock(current_function, "loop.after");

  LLVMValueRef start_val =
      codegen(range->data.AST_RANGE_EXPRESSION.from, ctx, module, builder);

  if (!start_val)
    return NULL;

  LLVMValueRef end_val =
      codegen(range->data.AST_RANGE_EXPRESSION.to, ctx, module, builder);
  if (!end_val)
    return NULL;

  LLVMValueRef loop_var_alloca =
      LLVMBuildAlloca(builder, LLVMInt32Type(), loop_var_name);
  LLVMBuildStore(builder, start_val, loop_var_alloca);

  LLVMTypeRef llvm_type = LLVMInt32Type();
  JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, &t_int, start_val, llvm_type);
  sym->storage = loop_var_alloca;

  const char *chars = binding->data.AST_IDENTIFIER.value;
  uint64_t id_hash = hash_string(chars, binding->data.AST_IDENTIFIER.length);

  ht_set_hash(ctx->frame->table, chars, id_hash, sym);

  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, cond_block);

  LLVMValueRef current_val =
      LLVMBuildLoad2(builder, LLVMInt32Type(), loop_var_alloca, "current");

  LLVMValueRef cond =
      LLVMBuildICmp(builder, LLVMIntSLE, current_val, end_val, "loopcond");

  LLVMBuildCondBr(builder, cond, body_block, after_block);

  // Set insertion point to the body block
  LLVMPositionBuilderAtEnd(builder, body_block);
  // printf("loop ctx env\n");
  // print_type_env(loop_ctx.env);
  codegen(body, ctx, module, builder);

  LLVMBuildBr(builder, inc_block);

  LLVMPositionBuilderAtEnd(builder, inc_block);

  LLVMValueRef inc_val = LLVMBuildAdd(
      builder, current_val, LLVMConstInt(LLVMInt32Type(), 1, 0), "inc");
  LLVMBuildStore(builder, inc_val, loop_var_alloca);

  LLVMBuildBr(builder, cond_block);

  LLVMPositionBuilderAtEnd(builder, after_block);

  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

LLVMValueRef codegen_loop(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                          LLVMBuilderRef builder) {

  Ast *body = ast->data.AST_LET.in_expr; // Loop body
  Ast *binding = ast->data.AST_LET.binding;
  Ast *iter_expr = ast->data.AST_LET.expr;

  if (iter_expr->tag == AST_RANGE_EXPRESSION) {
    // print_type_env(ctx->env);
    STACK_ALLOC_CTX_PUSH(loop_ctx, ctx);
    return codegen_loop_range(binding, iter_expr, body, &loop_ctx, module,
                              builder);
  }
  fprintf(stderr, "Error: loop type not implemented\n");
  print_ast_err(ast);
  return NULL;
}
