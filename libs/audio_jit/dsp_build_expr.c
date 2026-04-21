#include "./dsp_build_expr.h"
#include "../../lang/backend_llvm/application.h"
#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/function.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/escape_analysis.h"
#include "../../lang/serde.h"
#include "./dsp_fn_application.h"
#include "types/builtins.h"
#include <llvm-c/Target.h>
#include <llvm-c/Types.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>

DspValue dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                        LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (ast->tag) {

  case AST_BODY: {
    LLVMValueRef val;
    AST_LIST_ITER(
        ast->data.AST_BODY.stmts, ({
          Ast *stmt = l->ast;
          val = dsp_build_expr(stmt, dsp_ctx, ctx, module, builder).scalar;
        }));
    // return val;
    return DSP_SCALAR(val);
  }
  case AST_LET: {
    Ast *binding = ast->data.AST_LET.binding;

    const char *chars = binding->data.AST_IDENTIFIER.value;
    int len = binding->data.AST_IDENTIFIER.length;
    Ast *expr = ast->data.AST_LET.expr;
    Ast *in_expr = ast->data.AST_LET.in_expr;
    JITLangCtx *work_ctx = ctx;
    JITLangCtx inner_ctx;
    ht inner_table;
    StackFrame inner_frame;
    bool pushed_ctx = false;

    if (in_expr) {
      inner_ctx = *ctx;
      ht_init(&inner_table);
      inner_frame =
          (StackFrame){.table = &inner_table, .next = inner_ctx.frame};
      inner_ctx.frame = &inner_frame;
      inner_ctx.stack_ptr = ctx->stack_ptr + 1;
      inner_ctx.coro_ctx = ctx->coro_ctx;
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
      if (in_expr) {
        LLVMValueRef e_val =
            dsp_build_expr(in_expr, dsp_ctx, work_ctx, module, builder).scalar;
        if (pushed_ctx) {
          destroy_ctx(work_ctx);
        }
        return DSP_SCALAR(e_val);
      }

      return DSP_NULL;
    }

    DspValue bound_val;
    LLVMValueRef val;
    DspArrayAttributes array_attr;

    if (is_array_type(expr->type)) {
      DspBuildCtx _dsp_ctx = *dsp_ctx;
      bound_val = dsp_build_expr(expr, &_dsp_ctx, work_ctx, module, builder);
      val = bound_val.scalar;
      array_attr = _dsp_ctx.array_attrs;
      dsp_ctx->state_offset = _dsp_ctx.state_offset;

    } else {
      bound_val = dsp_build_expr(expr, dsp_ctx, work_ctx, module, builder);
      val = bound_val.scalar;
    }

    if (!val) {
      fprintf(stderr, "Error: could not compute dsp val for binding\n");
      print_ast_err(ast);
      return DSP_NULL;
    }

    if (binding->tag == AST_TUPLE) {
      for (int i = 0; i < binding->data.AST_LIST.len; i++) {
        Ast *b = binding->data.AST_LIST.items + i;

        const char *chars = b->data.AST_IDENTIFIER.value;
        int len = strlen(chars);
        if (strcmp(chars, "_") == 0) {
          continue;
        }

        LLVMValueRef ex_val = LLVMBuildExtractValue(builder, val, i, "");
        if (is_array_type(b->type)) {
          JITSymbol *sym =
              new_symbol(STYPE_LOCAL_VAR, b->type, ex_val, LLVMTypeOf(ex_val));
          ht_set_hash(work_ctx->frame->table, chars, hash_string(chars, len),
                      sym);
        } else {
          JITSymbol *sym =
              new_symbol(STYPE_LOCAL_VAR, b->type, ex_val, LLVMTypeOf(ex_val));
          ht_set_hash(work_ctx->frame->table, chars, hash_string(chars, len),
                      sym);
        }
      }
    } else {

      if (is_array_type(expr->type)) {

        JITSymbol *sym = new_symbol((symbol_type)STYPE_AUDIO_JIT_LOCAL_ARRAY,
                                    expr->type, val, LLVMTypeOf(val));
        DspArrayAttributes *attrs = malloc(sizeof(DspArrayAttributes));
        *attrs = array_attr;
        sym->symbol_data._USER_DEFINED_SYMBOL = attrs;
        ht_set_hash(work_ctx->frame->table, chars, hash_string(chars, len),
                    sym);
        // print_ast(ast);
        // printf("binding array to var: size %d\n", attrs->comptime_size);
      } else {
        JITSymbol *sym = new_symbol((symbol_type)STYPE_AUDIO_JIT_DSP_VALUE,
                                    expr->type, val, LLVMTypeOf(val));
        DspValue *stored =
            dsp_ctx ? dsp_tmp_alloc(dsp_ctx, sizeof(DspValue), 8) : NULL;
        if (stored) {
          *stored = bound_val;
          sym->symbol_data._USER_DEFINED_SYMBOL = stored;
        }
        ht_set_hash(work_ctx->frame->table, chars, hash_string(chars, len),
                    sym);
      }
    }

    if (in_expr) {
      LLVMValueRef e_val =
          dsp_build_expr(in_expr, dsp_ctx, work_ctx, module, builder).scalar;

      if (pushed_ctx) {
        destroy_ctx(work_ctx);
      }
      return DSP_SCALAR(e_val);
    }

    return DSP_SCALAR(val);
  }
  case AST_BOOL: {
    return DSP_SCALAR(
        LLVMConstInt(LLVMInt1Type(), ast->data.AST_BOOL.value, 0));
  }
  case AST_IDENTIFIER: {
    const char *id_name = ast->data.AST_IDENTIFIER.value;
    if (dsp_ctx && id_name) {
      LLVMTypeRef f64_ty = LLVMDoubleType();
      LLVMTypeRef f32_ty = LLVMInt32Type();
      if (strcmp(id_name, "sample_rate") == 0) {
        return DSP_SCALAR(LLVMConstReal(f64_ty, (double)dsp_ctx->sample_rate));
      }

      if (strcmp(id_name, "spf") == 0) {
        return DSP_SCALAR(
            LLVMConstReal(f64_ty, 1.0 / (double)dsp_ctx->sample_rate));
      }

      if (strcmp(id_name, "fft_size") == 0) {
        return DSP_SCALAR(LLVMConstInt(f32_ty, dsp_ctx->spectral_fft_size, 0));
      }
    }
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
      return DSP_SCALAR(LLVMBuildCall2(builder, read_fn_ty, read_fn, read_args,
                                       2, "inlet.sample"));
    }

    if (sym && sym->type == (symbol_type)STYPE_AUDIO_JIT_LOCAL_ARRAY) {
      DspArrayAttributes *attr = sym->symbol_data._USER_DEFINED_SYMBOL;
      dsp_ctx->array_attrs = *attr;
      return DSP_SCALAR(sym->val);
    }
    if (sym && sym->type == (symbol_type)STYPE_AUDIO_JIT_DSP_VALUE &&
        sym->symbol_data._USER_DEFINED_SYMBOL) {
      return *(DspValue *)sym->symbol_data._USER_DEFINED_SYMBOL;
    }
    return DSP_SCALAR(codegen(ast, ctx, module, builder));
  }
  case AST_DOUBLE: {
    return DSP_SCALAR(codegen(ast, ctx, module, builder));
  }
  case AST_INT: {
    return DSP_SCALAR(codegen(ast, ctx, module, builder));
  }
  case AST_RECORD_ACCESS: {
    return DSP_SCALAR(codegen(ast, ctx, module, builder));
  }
  case AST_APPLICATION: {
    return dsp_fn_application(ast, dsp_ctx, ctx, module, builder);
  }
  case AST_TUPLE: {
    // printf("tuple??\n");
    // print_ast(ast);
    // print_type(ast->type);

    int len = ast->data.AST_LIST.len;
    int offset = 0;

    LLVMTypeRef tuple_type = type_to_llvm_type(ast->type, ctx, module);
    LLVMValueRef tuple = LLVMGetUndef(tuple_type);
    for (int i = 0; i < len; i++) {

      Ast *mem_ast = ast->data.AST_LIST.items + i;

      LLVMValueRef tuple_element =
          dsp_build_expr(mem_ast, dsp_ctx, ctx, module, builder).scalar;

      tuple = LLVMBuildInsertValue(builder, tuple, tuple_element, offset,
                                   "insertx");

      offset++;
    }

    return DSP_SCALAR(tuple);
  }

  case AST_LIST:
  case AST_ARRAY: {

    bool hoist_to_synth_lifetime =
        ast->ea_md && (ast->ea_md->attributes & EA_ATTR_MUTABLE);
    int len = ast->data.AST_LIST.len;
    dsp_ctx->array_attrs.comptime_size = len;

    LLVMTypeRef i8_ty = LLVMInt8Type();
    LLVMTypeRef i32_ty = LLVMInt32Type();
    LLVMTypeRef i64_ty = LLVMInt64Type();

    Type *el_type = ast->type->data.T_CONS.args[0];
    if (el_type->kind == T_VAR) {
      el_type = &t_num;
    }
    LLVMTypeRef el_llvm_ty = type_to_llvm_type(el_type, ctx, module);
    LLVMTypeRef arr_ty = codegen_array_type(el_llvm_ty);
    LLVMTypeRef arr_ptr_ty = LLVMPointerType(arr_ty, 0);
    LLVMTypeRef el_ptr_ty = LLVMPointerType(el_llvm_ty, 0);

    unsigned long long el_size_ull =
        LLVMABISizeOfType(LLVMGetModuleDataLayout(module), el_llvm_ty);
    if (el_size_ull > (unsigned long long)INT_MAX) {
      fprintf(stderr, "Error: invalid array element ABI size (%llu)\n",
              el_size_ull);
      print_ast_err(ast);
      return DSP_NULL;
    }
    int el_size = (int)el_size_ull;

    unsigned long long arr_size_ull =
        LLVMABISizeOfType(LLVMGetModuleDataLayout(module), arr_ty);
    if (arr_size_ull > (unsigned long long)INT_MAX) {
      fprintf(stderr, "Error: invalid array ABI size (%llu)\n", arr_size_ull);
      print_ast_err(ast);
      return DSP_NULL;
    }
    int arr_size = (int)arr_size_ull;

    if (hoist_to_synth_lifetime) {
      int off = (dsp_ctx->state_offset + 7) & ~7;
      int data_off = off + arr_size;
      int total_bytes = arr_size + (len * el_size);
      dsp_ctx->state_offset = data_off + (len * el_size);

      if (dsp_ctx->init_builder && dsp_ctx->init_state_ptr) {
        LLVMValueRef init_base = dsp_consume_init_state(
            dsp_ctx, dsp_ctx->init_builder, total_bytes, 8, "array.ctor.base");
        LLVMValueRef arr_ptr = LLVMBuildBitCast(
            dsp_ctx->init_builder, init_base, arr_ptr_ty, "array.ctor.ptr");
        LLVMValueRef ctor_base_i8 = LLVMBuildGEP2(
            dsp_ctx->init_builder, i8_ty, init_base,
            (LLVMValueRef[]){LLVMConstInt(i64_ty, (uint64_t)arr_size, 0)}, 1,
            "array.ctor.base");
        LLVMValueRef ctor_base = LLVMBuildBitCast(
            dsp_ctx->init_builder, ctor_base_i8, el_ptr_ty, "array.ctor.data");

        LLVMValueRef arr_init = LLVMGetUndef(arr_ty);
        arr_init = LLVMBuildInsertValue(dsp_ctx->init_builder, arr_init,
                                        LLVMConstInt(i32_ty, (uint64_t)len, 0),
                                        0, "array.ctor.size");
        arr_init = LLVMBuildInsertValue(dsp_ctx->init_builder, arr_init,
                                        ctor_base, 1, "array.ctor.data_ptr");
        LLVMBuildStore(dsp_ctx->init_builder, arr_init, arr_ptr);

        for (int i = 0; i < len; i++) {
          Ast item = *(ast->data.AST_LIST.items + i);
          item.type = el_type;
          LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, (uint64_t)i, 0);
          LLVMValueRef elem_ptr =
              LLVMBuildGEP2(dsp_ctx->init_builder, el_llvm_ty, ctor_base,
                            &idx_i64, 1, "array.init.ptr");
          LLVMValueRef elem =
              dsp_build_expr(&item, dsp_ctx, ctx, module, dsp_ctx->init_builder)
                  .scalar;
          elem = handle_type_conversions(elem, item.type, el_type, ctx, module,
                                         dsp_ctx->init_builder);
          LLVMBuildStore(dsp_ctx->init_builder, elem, elem_ptr);
        }
      }

      LLVMValueRef run_arr_ptr_i8 = dsp_consume_frame_state(
          dsp_ctx, builder, total_bytes, 8, "array.ptr");
      LLVMValueRef run_arr_ptr =
          LLVMBuildBitCast(builder, run_arr_ptr_i8, arr_ptr_ty, "array.ptr");
      return DSP_SCALAR(
          LLVMBuildLoad2(builder, arr_ty, run_arr_ptr, "array.load"));
    }

    LLVMValueRef len_i32 = LLVMConstInt(i32_ty, (uint64_t)len, 0);
    LLVMValueRef frame_data_ptr =
        LLVMBuildArrayAlloca(builder, el_llvm_ty, len_i32, "array.frame.data");

    for (int i = 0; i < len; i++) {
      Ast item = *(ast->data.AST_LIST.items + i);
      item.type = el_type;
      LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, (uint64_t)i, 0);
      LLVMValueRef elem_ptr = LLVMBuildGEP2(builder, el_llvm_ty, frame_data_ptr,
                                            &idx_i64, 1, "array.frame.ptr");
      LLVMValueRef elem =
          dsp_build_expr(&item, dsp_ctx, ctx, module, builder).scalar;
      elem = handle_type_conversions(elem, item.type, el_type, ctx, module,
                                     builder);
      LLVMBuildStore(builder, elem, elem_ptr);
    }

    LLVMValueRef arr_val = LLVMGetUndef(arr_ty);
    arr_val = LLVMBuildInsertValue(builder, arr_val, len_i32, 0, "array.size");
    arr_val =
        LLVMBuildInsertValue(builder, arr_val, frame_data_ptr, 1, "array.data");
    return DSP_SCALAR(arr_val);
  }
  case AST_MATCH: {
    Ast *expr = ast->data.AST_MATCH.expr;
    int len = ast->data.AST_MATCH.len;
    Ast *branches = ast->data.AST_MATCH.branches;

    LLVMValueRef expr_val =
        dsp_build_expr(expr, dsp_ctx, ctx, module, builder).scalar;

    /* Compute all body vals eagerly (stateful DSP must run every frame).
       Build comparisons and chain selects from the last branch (default)
       backwards so the first matching case wins. */
    LLVMValueRef body_vals[len];
    for (int i = 0; i < len; i++) {
      body_vals[i] =
          dsp_build_expr(branches + 2 * i + 1, dsp_ctx, ctx, module, builder)
              .scalar;
    }

    if (expr->type->kind == T_BOOL && len == 2 && branches[0].tag == AST_BOOL &&
        branches[2].tag == AST_BOOL && branches[0].data.AST_BOOL.value) {

      return DSP_SCALAR(LLVMBuildSelect(builder, expr_val, body_vals[0],
                                        body_vals[1], "match.sel"));
    }

    LLVMValueRef cmps[len - 1];
    for (int i = 0; i < len; i++) {

      if (i < len - 1) {
        Ast *case_ast = branches + 2 * i;
        LLVMValueRef case_val =
            dsp_build_expr(branches + 2 * i, dsp_ctx, ctx, module, builder)
                .scalar;

        if (LLVMGetTypeKind(LLVMTypeOf(expr_val)) == LLVMIntegerTypeKind) {
          cmps[i] = LLVMBuildICmp(builder, LLVMIntEQ, expr_val, case_val,
                                  "match.cmp");
        } else
          cmps[i] = LLVMBuildFCmp(builder, LLVMRealOEQ, expr_val, case_val,
                                  "match.cmp");
      }
    }

    LLVMValueRef result = body_vals[len - 1];
    for (int i = len - 2; i >= 0; i--) {
      result =
          LLVMBuildSelect(builder, cmps[i], body_vals[i], result, "match.sel");
    }

    return DSP_SCALAR(result);
  }

  default: {
    return DSP_NULL;
  }
  }
}
