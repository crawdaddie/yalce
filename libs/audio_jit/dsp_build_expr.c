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

      JITSymbol *sym =
          new_symbol(STYPE_AUDIO_JIT_INLINE_LAMBDA, expr->type, NULL, NULL);
      // print_ast(expr);
      // print_type(expr->type);
      sym->symbol_data._USER_DEFINED_SYMBOL = expr;

      ht_set_hash(work_ctx->frame->table, chars, hash_string(chars, len), sym);

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
    // Ast *f = ast->data.AST_APPLICATION.function;
    //
    // fprintf(stderr, "dsp_app: %s (scope=%d)\n", f->data.AST_IDENTIFIER.value,
    //         ctx->stack_ptr);
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "spf") == 0) {
    //   return dsp_ctx->spf;
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "sin_osc") == 0) {
    //   return builtin_tab_osc("ylc_sin_table", SIN_TABSIZE, ast, dsp_ctx, ctx,
    //                          module, builder);
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "sq_osc") == 0) {
    //   return builtin_tab_osc("ylc_sq_table", SQ_TABSIZE, ast, dsp_ctx, ctx,
    //                          module, builder);
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "saw_osc") == 0) {
    //   return builtin_tab_osc("ylc_saw_table", SAW_TABSIZE, ast, dsp_ctx, ctx,
    //                          module, builder);
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "phasor") == 0) {
    //   LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args,
    //                                      dsp_ctx, ctx, module, builder);
    //   freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq,
    //   builder); return builtin_phasor(freq, dsp_ctx, ctx, module, builder);
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "phasor_sinc") == 0) {
    //   LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args,
    //                                      dsp_ctx, ctx, module, builder);
    //   freq = ensure_float(ast->data.AST_APPLICATION.args->type, freq,
    //   builder);
    //
    //   LLVMValueRef trig = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                      dsp_ctx, ctx, module, builder);
    //   return builtin_phasor_sinc(freq, trig, dsp_ctx, ctx, module, builder);
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "trig") == 0) {
    //   Ast *freq_ast = ast->data.AST_APPLICATION.args;
    //   LLVMValueRef freq =
    //       dsp_build_expr(freq_ast, dsp_ctx, ctx, module, builder);
    //   freq = ensure_float(freq_ast->type, freq, builder);
    //
    //   bool freq_is_const_zero = ast_is_const_zero(freq_ast, ctx);
    //
    //   return builtin_trig(freq, freq_is_const_zero, dsp_ctx, ctx, module,
    //                       builder);
    // }
    // if (strcmp(f->data.AST_IDENTIFIER.value, "+") == 0) {
    //   LLVMValueRef l =
    //       ensure_float(ast->data.AST_APPLICATION.args->type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args,
    //                    dsp_ctx,
    //                                   ctx, module, builder),
    //                    builder);
    //
    //   LLVMValueRef r =
    //       ensure_float(ast->data.AST_APPLICATION.args[1].type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                   dsp_ctx, ctx, module, builder),
    //                    builder);
    //   return LLVMBuildFAdd(builder, l, r, "signal.add");
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "-") == 0) {
    //
    //   LLVMValueRef l =
    //       ensure_float(ast->data.AST_APPLICATION.args->type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args,
    //                    dsp_ctx,
    //                                   ctx, module, builder),
    //                    builder);
    //
    //   LLVMValueRef r =
    //       ensure_float(ast->data.AST_APPLICATION.args[1].type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                   dsp_ctx, ctx, module, builder),
    //                    builder);
    //
    //   return LLVMBuildFSub(builder, l, r, "signal.sub");
    // }
    // if (strcmp(f->data.AST_IDENTIFIER.value, "*") == 0) {
    //
    //   LLVMValueRef l =
    //       ensure_float(ast->data.AST_APPLICATION.args->type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args,
    //                    dsp_ctx,
    //                                   ctx, module, builder),
    //                    builder);
    //
    //   LLVMValueRef r =
    //       ensure_float(ast->data.AST_APPLICATION.args[1].type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                   dsp_ctx, ctx, module, builder),
    //                    builder);
    //   if (!l || !r) {
    //     fprintf(stderr, "audio_jit: null operand in '*' expression\n");
    //     print_ast(ast);
    //     return NULL;
    //   }
    //
    //   return LLVMBuildFMul(builder, l, r, "signal.mul");
    // }
    // if (strcmp(f->data.AST_IDENTIFIER.value, "/") == 0) {
    //   LLVMValueRef l =
    //       ensure_float(ast->data.AST_APPLICATION.args->type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args,
    //                    dsp_ctx,
    //                                   ctx, module, builder),
    //                    builder);
    //
    //   LLVMValueRef r =
    //       ensure_float(ast->data.AST_APPLICATION.args[1].type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                   dsp_ctx, ctx, module, builder),
    //                    builder);
    //   return LLVMBuildFDiv(builder, l, r, "signal.div");
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "%") == 0) {
    //   LLVMValueRef l =
    //       ensure_float(ast->data.AST_APPLICATION.args->type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args,
    //                    dsp_ctx,
    //                                   ctx, module, builder),
    //                    builder);
    //
    //   LLVMValueRef r =
    //       ensure_float(ast->data.AST_APPLICATION.args[1].type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                   dsp_ctx, ctx, module, builder),
    //                    builder);
    //   return LLVMBuildFRem(builder, l, r, "signal.fmod");
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "white") == 0) {
    //
    //   LLVMTypeRef wn_ty = LLVMFunctionType(
    //       LLVMDoubleType(), (LLVMTypeRef[]){LLVMDoubleType(),
    //       LLVMDoubleType()}, 2, 0);
    //   LLVMValueRef wn_fn = LLVMGetNamedFunction(module, "rand_double_range");
    //   if (!wn_fn) {
    //     wn_fn = LLVMAddFunction(module, "rand_double_range", wn_ty);
    //     LLVMSetLinkage(wn_fn, LLVMExternalLinkage);
    //   }
    //
    //   return LLVMBuildCall2(
    //       builder, wn_ty, wn_fn,
    //       (LLVMValueRef[]){LLVMConstReal(LLVMDoubleType(), -1.),
    //                        LLVMConstReal(LLVMDoubleType(), 1.)},
    //       2, "white_noise.sample");
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "tabread1") == 0) {
    //   LLVMValueRef table = dsp_build_expr(ast->data.AST_APPLICATION.args,
    //                                       dsp_ctx, ctx, module, builder);
    //
    //   LLVMValueRef phase = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                       dsp_ctx, ctx, module, builder);
    //   return build_tabread(table, phase, dsp_ctx, ctx, module, builder);
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "array_set") == 0) {
    //   print_ast(ast);
    //   // print_type((ast->data.AST_APPLICATION.args + 2)->type);
    //
    //   LLVMValueRef arr = dsp_build_expr(ast->data.AST_APPLICATION.args,
    //   dsp_ctx,
    //                                     ctx, module, builder);
    //   LLVMValueRef index = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                       dsp_ctx, ctx, module, builder);
    //   LLVMValueRef value = dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
    //                                       dsp_ctx, ctx, module, builder);
    //
    //   set_array_element(
    //       builder, arr, index, value,
    //       type_to_llvm_type((ast->data.AST_APPLICATION.args + 2)->type, ctx,
    //                         module));
    //   return arr;
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "array_at") == 0) {
    //
    //   LLVMValueRef arr = dsp_build_expr(ast->data.AST_APPLICATION.args,
    //   dsp_ctx,
    //                                     ctx, module, builder);
    //   LLVMValueRef index = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                       dsp_ctx, ctx, module, builder);
    //   // print_type(ast->type);
    //
    //   return get_array_element(builder, arr, index,
    //                            type_to_llvm_type(ast->type, ctx, module));
    // }
    // if (strcmp(f->data.AST_IDENTIFIER.value, "decay") == 0) {
    //
    //   LLVMValueRef T = dsp_build_expr(ast->data.AST_APPLICATION.args,
    //   dsp_ctx,
    //                                   ctx, module, builder);
    //   T = ensure_float(ast->data.AST_APPLICATION.args->type, T, builder);
    //
    //   LLVMValueRef trig = dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                      dsp_ctx, ctx, module, builder);
    //   return build_exp_decay(T, trig, dsp_ctx, ctx, module, builder);
    // }
    // if (strcmp(f->data.AST_IDENTIFIER.value, "scale") == 0) {
    //   // unipolar scale - ie [0,1] -> [a, b]
    //   LLVMValueRef lo =
    //       ensure_float(ast->data.AST_APPLICATION.args->type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args,
    //                    dsp_ctx,
    //                                   ctx, module, builder),
    //                    builder);
    //
    //   LLVMValueRef hi =
    //       ensure_float(ast->data.AST_APPLICATION.args[1].type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                   dsp_ctx, ctx, module, builder),
    //                    builder);
    //
    //   LLVMValueRef v =
    //       ensure_float(ast->data.AST_APPLICATION.args[2].type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
    //                                   dsp_ctx, ctx, module, builder),
    //                    builder);
    //   LLVMValueRef span = LLVMBuildFSub(builder, hi, lo, "scale.span");
    //   LLVMValueRef scaled = LLVMBuildFMul(builder, v, span, "scale.scaled");
    //   return LLVMBuildFAdd(builder, lo, scaled, "scale.out");
    // }
    //
    // if (strcmp(f->data.AST_IDENTIFIER.value, "scale_bp") == 0) {
    //   // bipolar scale - ie [-1,1] -> [a, b]
    //
    //   LLVMValueRef lo =
    //       ensure_float(ast->data.AST_APPLICATION.args->type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args,
    //                    dsp_ctx,
    //                                   ctx, module, builder),
    //                    builder);
    //
    //   LLVMValueRef hi =
    //       ensure_float(ast->data.AST_APPLICATION.args[1].type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args + 1,
    //                                   dsp_ctx, ctx, module, builder),
    //                    builder);
    //
    //   LLVMValueRef v =
    //       ensure_float(ast->data.AST_APPLICATION.args[2].type,
    //                    dsp_build_expr(ast->data.AST_APPLICATION.args + 2,
    //                                   dsp_ctx, ctx, module, builder),
    //                    builder);
    //   LLVMValueRef span = LLVMBuildFSub(builder, hi, lo, "scale.span");
    //
    //   v = LLVMBuildFMul(builder, v, LLVMConstReal(LLVMDoubleType(), 0.5),
    //                     "input.half");
    //
    //   v = LLVMBuildFAdd(builder, v, LLVMConstReal(LLVMDoubleType(), 0.5),
    //                     "input.add_half");
    //   LLVMValueRef scaled = LLVMBuildFMul(builder, v, span, "scale.scaled");
    //   return LLVMBuildFAdd(builder, lo, scaled, "scale.out");
    // }
    //
    // JITSymbol *callable_sym =
    //     lookup_id_ast(ast->data.AST_APPLICATION.function, ctx);
    //
    // if (callable_sym && callable_sym->type == STYPE_AUDIO_JIT_SYM) {
    //
    //   int synth_id = audio_sym_synth_id(callable_sym);
    //   SynthRecord rec = synth_registry_get(synth_id);
    //   printf("use pre-compiled audio func id %d %s\n", synth_id, rec.name);
    //
    //   return call_registered_synth_in_audio_fn(ast, rec, dsp_ctx, ctx,
    //   module,
    //                                            builder);
    // }
    //
    // if (callable_sym && callable_sym->type == STYPE_AUDIO_JIT_INLINE_LAMBDA)
    // {
    //   Ast *lambda_ast = callable_sym->symbol_data._USER_DEFINED_SYMBOL;
    //   STACK_ALLOC_CTX_PUSH(lctx, ctx);
    //   Type *ltype = callable_sym->symbol_type;
    //
    //   bool is_void_fn = is_void_func(ltype);
    //
    //   if (!is_void_fn) {
    //     int idx = 0;
    //     for (AstList *p = lambda_ast->data.AST_LAMBDA.params; p;
    //          p = p->next, idx++) {
    //       Ast *param_ast = p->ast;
    //       Type *param_type = ltype->data.T_FN.from;
    //
    //       LLVMValueRef arg_val =
    //           dsp_build_expr(ast->data.AST_APPLICATION.args + idx, dsp_ctx,
    //                          &lctx, module, builder);
    //       JITSymbol *sym =
    //           new_symbol(STYPE_LOCAL_VAR, param_type, arg_val,
    //                      type_to_llvm_type(param_type, &lctx, module));
    //
    //       const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
    //       int id_len = param_ast->data.AST_IDENTIFIER.length;
    //       ht_set_hash(lctx.frame->table, id_chars,
    //                   hash_string(id_chars, id_len), sym);
    //
    //       ltype = ltype->data.T_FN.to;
    //     }
    //   }
    //
    //   LLVMValueRef res = dsp_build_expr(lambda_ast->data.AST_LAMBDA.body,
    //                                     dsp_ctx, &lctx, module, builder);
    //
    //   destroy_ctx(&lctx);
    //
    //   return res;
    // }
    //
    // if (callable_sym) {
    //   LLVMValueRef callable = callable_sym->val;
    //
    //   if (callable_sym->type == STYPE_LAZY_EXTERN_FUNCTION) {
    //
    //     callable =
    //         instantiate_extern_fn_sym(callable_sym, ctx, module, builder);
    //   }
    //   // printf("application??a\n");
    //   // print_ast(ast);
    //   // print_type(ast->data.AST_APPLICATION.function->type);
    //   // LLVMDumpValue(callable);
    //   int args_len = ast->data.AST_APPLICATION.len;
    //   LLVMValueRef args[args_len];
    //
    //   Type *f = callable_sym->symbol_type;
    //   print_type(f);
    //   print_ast(ast);
    //
    //   for (int i = 0; i < args_len; i++) {
    //     args[i] = dsp_build_expr(ast->data.AST_APPLICATION.args + i, dsp_ctx,
    //                              ctx, module, builder);
    //     Type *t = f->data.T_FN.from;
    //
    //     if (types_equal(t, &t_num)) {
    //       args[i] = ensure_float((ast->data.AST_APPLICATION.args + i)->type,
    //                              args[i], builder);
    //     }
    //     f = f->data.T_FN.to;
    //   }
    //   return LLVMBuildCall2(builder, LLVMGlobalGetValueType(callable),
    //   callable,
    //                         args, args_len, "call.ylc-function");
    // }
    //
    // return NULL;
    // // return NULL;
  }

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

      LLVMValueRef off_i32 = LLVMConstInt(i32_ty, (uint64_t)off, 0);
      LLVMValueRef data_off_i32 = LLVMConstInt(i32_ty, (uint64_t)data_off, 0);
      LLVMValueRef arr_ptr_i8 =
          LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, dsp_ctx->init_state_ptr,
                        &off_i32, 1, "array.ctor.ptr_i8");
      LLVMValueRef arr_ptr = LLVMBuildBitCast(dsp_ctx->init_builder, arr_ptr_i8,
                                              arr_ptr_ty, "array.ctor.ptr");

      LLVMValueRef ctor_base_i8 =
          LLVMBuildGEP2(dsp_ctx->init_builder, i8_ty, dsp_ctx->init_state_ptr,
                        &data_off_i32, 1, "array.ctor.base");

      LLVMValueRef ctor_base = LLVMBuildBitCast(
          dsp_ctx->init_builder, ctor_base_i8, f64_ptr_ty, "array.ctor.data");
      LLVMValueRef arr_init = LLVMGetUndef(arr_ty);
      arr_init = LLVMBuildInsertValue(dsp_ctx->init_builder, arr_init,
                                      LLVMConstInt(i32_ty, (uint64_t)len, 0), 0,
                                      "array.ctor.size");
      arr_init = LLVMBuildInsertValue(dsp_ctx->init_builder, arr_init,
                                      ctor_base, 1, "array.ctor.data_ptr");
      LLVMBuildStore(dsp_ctx->init_builder, arr_init, arr_ptr);

      for (int i = 0; i < len; i++) {
        Ast *item = ast->data.AST_LIST.items + i;
        LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, (uint64_t)i, 0);
        LLVMValueRef elem_ptr =
            LLVMBuildGEP2(dsp_ctx->init_builder, f64_ty, ctor_base, &idx_i64, 1,
                          "array.init.ptr");

        LLVMValueRef elem = codegen(item, ctx, module, dsp_ctx->init_builder);
        elem = ensure_float(item->type, elem, dsp_ctx->init_builder);
        LLVMBuildStore(dsp_ctx->init_builder, elem, elem_ptr);
      }

      LLVMValueRef run_arr_ptr_i8 = LLVMBuildGEP2(
          builder, i8_ty, dsp_ctx->state_ptr, &off_i32, 1, "array.ptr_i8");
      LLVMValueRef run_arr_ptr =
          LLVMBuildBitCast(builder, run_arr_ptr_i8, arr_ptr_ty, "array.ptr");
      return LLVMBuildLoad2(builder, arr_ty, run_arr_ptr, "array.load");
    }

    return NULL;
  }

  case AST_LIST: {
    if (ast_is_const(ast, ctx)) {
      // allocate list in constructor and initialize it to the specified val
    }

    return NULL;
  }
  default: {
    return NULL;
  }
  }
}
