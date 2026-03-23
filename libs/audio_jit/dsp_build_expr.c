#include "./dsp_build_expr.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/function.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/serde.h"
#include "./dsp_fn_application.h"
#include <llvm-c/Types.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

LLVMValueRef dsp_consume_frame_state(DspBuildCtx *dsp_ctx,
                                     LLVMBuilderRef builder, int size,
                                     int align, const char *name);
LLVMValueRef dsp_consume_init_state(DspBuildCtx *dsp_ctx,
                                    LLVMBuilderRef builder, int size,
                                    int align, const char *name);

LLVMValueRef dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (ast->tag) {

  case AST_BODY: {
    LLVMValueRef val;
    AST_LIST_ITER(ast->data.AST_BODY.stmts, ({
                    Ast *stmt = l->ast;
                    val = dsp_build_expr(stmt, dsp_ctx, ctx, module, builder);
                  }));
    // return val;
    return val;
  }
  case AST_LET: {
    Ast *binding = ast->data.AST_LET.binding;
    const char *chars = binding->data.AST_IDENTIFIER.value;
    int len = binding->data.AST_IDENTIFIER.length;
    Ast *expr = ast->data.AST_LET.expr;
    Ast *in_expr = ast->data.AST_LET.in_expr;
    JITLangCtx *work_ctx = ctx;
    JITLangCtx inner_ctx;
    bool pushed_ctx = false;

    if (in_expr) {
      STACK_ALLOC_CTX_PUSH(_inner_ctx, ctx)
      inner_ctx = _inner_ctx;
      work_ctx = &inner_ctx;
      pushed_ctx = true;
    }

    if (expr->tag == AST_LAMBDA) {
      Ast compile_synth_application = (Ast){
          AST_APPLICATION,
          .data = {.AST_APPLICATION = {.len = 1,
                                       .args = ast,
                                       .is_curried_with_constants = false}}};

      int _synth_id = synth_registry_len();
      LLVMValueRef cons_fn = CompileAudioFnHandler(&compile_synth_application,
                                                   ctx, module, builder);
      SynthRecord s = synth_registry_get(_synth_id);
      // printf("synth name %s bytes %d: \n", s.name, s.state_bytes);
      // LLVMDumpValue(s.ctor);
      // printf("\n");
      // LLVMDumpValue(s.frame_fn);
      // printf("\n");
      // LLVMDumpValue(s.perform_fn);
      // printf("\n");

      // JITSymbol *sym =
      //     new_symbol(STYPE_AUDIO_JIT_INLINE_LAMBDA, expr->type, NULL, NULL);

      // print_ast(expr);
      // print_type(expr->type);
      // sym->symbol_data._USER_DEFINED_SYMBOL = expr;

      // ht_set_hash(work_ctx->frame->table, chars, hash_string(chars, len),
      // sym);
      if (in_expr) {
        LLVMValueRef e_val =
            dsp_build_expr(in_expr, dsp_ctx, work_ctx, module, builder);
        if (pushed_ctx) {
          destroy_ctx(work_ctx);
        }
        return e_val;
      }

      return NULL;
    }

    LLVMValueRef val = dsp_build_expr(expr, dsp_ctx, work_ctx, module, builder);

    if (!val) {
      fprintf(stderr, "Error: could not compute dsp val for binding\n");
      print_ast_err(ast);
      return NULL;
    }

    JITSymbol *sym =
        new_symbol(STYPE_LOCAL_VAR, expr->type, val, LLVMTypeOf(val));
    ht_set_hash(work_ctx->frame->table, chars, hash_string(chars, len), sym);

    if (in_expr) {
      LLVMValueRef e_val =
          dsp_build_expr(in_expr, dsp_ctx, work_ctx, module, builder);

      if (pushed_ctx) {
        destroy_ctx(work_ctx);
      }
      return e_val;
    }

    return val;
  }
  case AST_IDENTIFIER: {
    JITSymbol *sym = lookup_id_ast(ast, ctx);
    if (sym && sym->type == (symbol_type)STYPE_AUDIO_JIT_SYNTH_INLET) {
      LLVMValueRef inlet_node = sym->val;
      LLVMTypeRef i64_ty = LLVMInt64Type();
      LLVMTypeRef f64_ty = LLVMDoubleType();
      LLVMTypeRef read_param_tys[] = {GENERIC_PTR, i64_ty};
      LLVMTypeRef read_fn_ty = LLVMFunctionType(f64_ty, read_param_tys, 2, 0);
      LLVMValueRef read_fn =
          LLVMGetNamedFunction(module, "ylc_read_inlet_node");
      if (!read_fn) {
        read_fn = LLVMAddFunction(module, "ylc_read_inlet_node", read_fn_ty);
        LLVMSetLinkage(read_fn, LLVMExternalLinkage);
      }

      LLVMValueRef frame_i64 =
          LLVMBuildSExt(builder, dsp_ctx->frame_idx, i64_ty, "frame_idx.i64");
      LLVMValueRef read_args[] = {inlet_node, frame_i64};
      return LLVMBuildCall2(builder, read_fn_ty, read_fn, read_args, 2,
                            "inlet.sample");
    }
    return codegen(ast, ctx, module, builder);
  }
  case AST_DOUBLE: {
    return codegen(ast, ctx, module, builder);
  }
  case AST_INT: {
    return codegen(ast, ctx, module, builder);
  }
  case AST_RECORD_ACCESS: {
    return codegen(ast, ctx, module, builder);
  }
  case AST_APPLICATION: {
    return dsp_fn_application(ast, dsp_ctx, ctx, module, builder);
  }

  case AST_LIST:
  case AST_ARRAY: {
    if (ast_is_const(ast, ctx)) {
      int len = ast->data.AST_LIST.len;
      int off = (dsp_ctx->state_offset + 7) & ~7;
      int data_off = off + 16; // struct { i32, ptr } in state, then f64 data
      dsp_ctx->state_offset = data_off + (len * 8);

      LLVMTypeRef i8_ty = LLVMInt8Type();
      LLVMTypeRef i32_ty = LLVMInt32Type();
      LLVMTypeRef i64_ty = LLVMInt64Type();
      LLVMTypeRef f64_ty = LLVMDoubleType();
      LLVMTypeRef f64_ptr_ty = LLVMPointerType(f64_ty, 0);
      LLVMTypeRef arr_ty =
          LLVMStructType((LLVMTypeRef[]){i32_ty, f64_ptr_ty}, 2, 0);
      LLVMTypeRef arr_ptr_ty = LLVMPointerType(arr_ty, 0);
      int total_bytes = 16 + (len * 8);

      if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
        LLVMValueRef init_base = dsp_consume_init_state(
            dsp_ctx, dsp_ctx->init_builder, total_bytes, 8, "array.ctor.base");
        LLVMValueRef arr_ptr_i8 = init_base;
        LLVMValueRef arr_ptr = LLVMBuildBitCast(dsp_ctx->init_builder,
                                                arr_ptr_i8, arr_ptr_ty,
                                                "array.ctor.ptr");

        LLVMValueRef ctor_base_i8 = LLVMBuildGEP2(
            dsp_ctx->init_builder, i8_ty, init_base,
            (LLVMValueRef[]){LLVMConstInt(i64_ty, 16, 0)}, 1,
            "array.ctor.base");

        LLVMValueRef ctor_base = LLVMBuildBitCast(
            dsp_ctx->init_builder, ctor_base_i8, f64_ptr_ty, "array.ctor.data");
        LLVMValueRef arr_init = LLVMGetUndef(arr_ty);
        arr_init = LLVMBuildInsertValue(dsp_ctx->init_builder, arr_init,
                                        LLVMConstInt(i32_ty, (uint64_t)len, 0),
                                        0, "array.ctor.size");
        arr_init = LLVMBuildInsertValue(dsp_ctx->init_builder, arr_init,
                                        ctor_base, 1,
                                        "array.ctor.data_ptr");
        LLVMBuildStore(dsp_ctx->init_builder, arr_init, arr_ptr);

        for (int i = 0; i < len; i++) {
          Ast *item = ast->data.AST_LIST.items + i;
          LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, (uint64_t)i, 0);
          LLVMValueRef elem_ptr = LLVMBuildGEP2(dsp_ctx->init_builder, f64_ty,
                                                ctor_base, &idx_i64, 1,
                                                "array.init.ptr");

          LLVMValueRef elem =
              codegen(item, ctx, module, dsp_ctx->init_builder);
          elem = ensure_float(item->type, elem, dsp_ctx->init_builder);
          LLVMBuildStore(dsp_ctx->init_builder, elem, elem_ptr);
        }
      }

      (void)off;
      (void)data_off;
      LLVMValueRef run_arr_ptr_i8 = dsp_consume_frame_state(
          dsp_ctx, builder, total_bytes, 8, "array.ptr");
      LLVMValueRef run_arr_ptr =
          LLVMBuildBitCast(builder, run_arr_ptr_i8, arr_ptr_ty, "array.ptr");
      return LLVMBuildLoad2(builder, arr_ty, run_arr_ptr, "array.load");
    }

    return NULL;
  }

  default: {
    return NULL;
  }
  }
}
