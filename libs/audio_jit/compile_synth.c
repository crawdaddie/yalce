#include "./compile_synth.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/serde.h"
#include "./audio_jit.h"

#include "../../engine/common.h"
#include "../../engine/ctx.h"
#include "../../engine/node.h"
#include "../../lang/backend_llvm/application.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/common.h"
#include "../../lang/escape_analysis.h"
#include "../../lang/ht.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "./dsp_build_expr.h"
#include <stdlib.h>
#include <string.h>

static SynthRegistry synth_registry;

static DspSynthArgKind classify_synth_arg_kind(Type *type) {
  if (!type) {
    return DSP_SYNTH_ARG_SCALAR_ONLY;
  }
  if (is_array_type(type)) {
    return DSP_SYNTH_ARG_ARRAY_ONLY;
  }
  if (types_equal(type, &t_num) || types_equal(type, &t_int) ||
      type->kind == T_VAR) {
    return DSP_SYNTH_ARG_SCALAR_OR_MULTICHANNEL;
  }
  return DSP_SYNTH_ARG_SCALAR_ONLY;
}

static size_t align_up_size(size_t value, size_t align) {
  if (align <= 1) {
    return value;
  }
  return (value + align - 1) & ~(align - 1);
}

void dsp_tmp_allocator_init(DspTmpAllocator *alloc, size_t initial_bytes) {
  if (!alloc) {
    return;
  }
  alloc->total_bytes = initial_bytes;
  alloc->current_bytes = 0;
  alloc->bytes = initial_bytes ? malloc(initial_bytes) : NULL;
}

void dsp_tmp_allocator_free(DspTmpAllocator *alloc) {
  if (!alloc) {
    return;
  }
  free(alloc->bytes);
  alloc->bytes = NULL;
  alloc->total_bytes = 0;
  alloc->current_bytes = 0;
}

void *dsp_tmp_alloc(DspBuildCtx *dsp_ctx, size_t size, size_t align) {
  if (!dsp_ctx || !dsp_ctx->tmp_alloc || size == 0) {
    return NULL;
  }

  DspTmpAllocator *alloc = dsp_ctx->tmp_alloc;
  size_t offset = align_up_size(alloc->current_bytes, align);
  size_t needed = offset + size;

  if (needed > alloc->total_bytes) {
    size_t new_total = alloc->total_bytes ? alloc->total_bytes : 256;
    while (new_total < needed) {
      new_total *= 2;
    }
    unsigned char *new_bytes = realloc(alloc->bytes, new_total);
    if (!new_bytes) {
      fprintf(
          stderr,
          "audio_jit: dsp temporary allocator failed to grow to %zu bytes\n",
          new_total);
      return NULL;
    }
    alloc->bytes = new_bytes;
    alloc->total_bytes = new_total;
  }

  void *ptr = alloc->bytes + offset;
  alloc->current_bytes = needed;
  return ptr;
}

static LLVMValueRef call_dsp_symbol(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module_ref,
                                    LLVMBuilderRef builder) {

  Ast *sym_id = ast->data.AST_APPLICATION.function;
  JITSymbol *sym = lookup_id_ast(sym_id, ctx);

  if (!sym) {
    fprintf(stderr, "audio_jit: unresolved compiled synth symbol\n");
    print_ast_err(sym_id);
    return LLVMConstNull(GENERIC_PTR);
  }
  int synth_id = audio_sym_synth_id(sym);
  if (synth_id < 0 || synth_id >= synth_registry_len()) {
    fprintf(stderr, "audio_jit: synth id out of range: %d\n", synth_id);
    return LLVMConstNull(GENERIC_PTR);
  }
  SynthRecord synth_rec = synth_registry_get(synth_id);

  if (!synth_rec.ctor) {
    fprintf(stderr, "audio_jit: missing ctor for synth id: %d\n", synth_id);
    return LLVMConstNull(GENERIC_PTR);
  }

  LLVMValueRef ctor_fn = synth_rec.ctor;
  LLVMTypeRef ctor_fn_ty = LLVMGlobalGetValueType(ctor_fn);
  int arg_count = ast->data.AST_APPLICATION.len;
  unsigned formal_count = LLVMCountParamTypes(ctor_fn_ty);
  LLVMTypeRef *formal_tys =
      formal_count ? alloca(sizeof(LLVMTypeRef) * formal_count) : NULL;
  LLVMValueRef *ctor_args =
      formal_count ? alloca(sizeof(LLVMValueRef) * formal_count) : NULL;

  if (formal_count) {
    LLVMGetParamTypes(ctor_fn_ty, formal_tys);
  }

  for (unsigned i = 0; i < formal_count; i++) {
    if ((int)i < arg_count) {
      Ast *arg_ast = ast->data.AST_APPLICATION.args + i;
      LLVMValueRef arg_val = codegen(arg_ast, ctx, module_ref, builder);
      if (LLVMGetTypeKind(formal_tys[i]) == LLVMDoubleTypeKind) {
        arg_val = handle_type_conversions(arg_val, arg_ast->type, &t_num, ctx,
                                          module_ref, builder);
      }
      ctor_args[i] = arg_val;
    } else {
      ctor_args[i] = LLVMConstNull(formal_tys[i]);
    }
  }

  // hack to stop Synth converting the output to a const_sig because it sees it
  // as a Double
  ast->type = &t_ptr;

  // Emit an indirect call through ylc_get_synth_ctor so that redefining the
  // synth (updating synth_registry.records[synth_id].ctor) is picked up by
  // already-compiled call sites (e.g. coroutines) at runtime.
  LLVMTypeRef get_ctor_fn_ty =
      LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
  LLVMValueRef get_ctor_fn =
      LLVMGetNamedFunction(module_ref, "ylc_get_synth_ctor");
  if (!get_ctor_fn) {
    get_ctor_fn =
        LLVMAddFunction(module_ref, "ylc_get_synth_ctor", get_ctor_fn_ty);
    LLVMSetLinkage(get_ctor_fn, LLVMExternalLinkage);
  }
  LLVMValueRef synth_id_val =
      LLVMConstInt(LLVMInt32Type(), (unsigned long long)synth_id, 0);
  LLVMValueRef current_ctor = LLVMBuildCall2(
      builder, get_ctor_fn_ty, get_ctor_fn, &synth_id_val, 1, "ctor.ptr");

  LLVMValueRef node = LLVMBuildCall2(builder, ctor_fn_ty, current_ctor,
                                     ctor_args, formal_count, "audio_jit.node");
  // if (formal_tys) {
  //   free(formal_tys);
  // }
  // if (ctor_args) {
  //   free(ctor_args);
  // }
  return node;
}

void init_synth_registry() {
  synth_registry.capacity = 128;
  synth_registry.records = calloc(128, sizeof(SynthRecord));
  synth_registry.length = 0;
}

int extend_synth_registry(SynthRecord record) {
  int id = synth_registry.length;

  if (id == synth_registry.capacity) {
    int new_cap = synth_registry.capacity * 2;
    synth_registry.records =
        realloc(synth_registry.records, new_cap * sizeof(SynthRecord));
    synth_registry.capacity = new_cap;
  }

  synth_registry.records[id] = record;
  synth_registry.length++;
  // printf("extended synth registry %d [%s]\n", id, record.name);
  return id;
}
int audio_sym_synth_id(JITSymbol *sym) {
  return sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr;
}

static LLVMBasicBlockRef dsp_build_perform_loop(LLVMValueRef perform_fn,
                                                DspBuildCtx *dsp_ctx,
                                                LLVMContextRef llvm_ctx) {
  LLVMBuilderRef builder = dsp_ctx->perform_builder;
  LLVMTypeRef i32_ty = LLVMInt32TypeInContext(llvm_ctx);

  dsp_ctx->node_ptr = LLVMGetParam(perform_fn, 0);
  dsp_ctx->state_ptr = LLVMGetParam(perform_fn, 1);
  dsp_ctx->state_base_ptr = dsp_ctx->state_ptr;
  dsp_ctx->inputs_ptr = LLVMGetParam(perform_fn, 2);
  dsp_ctx->spf = LLVMGetParam(perform_fn, 4);

  LLVMValueRef nframes = LLVMGetParam(perform_fn, 3);
  dsp_ctx->frame_idx_ptr = LLVMBuildAlloca(builder, i32_ty, "frame_idx.ptr");
  LLVMBuildStore(builder, LLVMConstInt(i32_ty, 0, 0), dsp_ctx->frame_idx_ptr);

  LLVMBasicBlockRef cond_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, perform_fn, "frames.cond");
  LLVMBasicBlockRef body_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, perform_fn, "frames.body");
  LLVMBasicBlockRef exit_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, perform_fn, "frames.exit");
  dsp_ctx->frame_cond_bb = cond_bb;

  LLVMBuildBr(builder, cond_bb);

  LLVMPositionBuilderAtEnd(builder, cond_bb);
  LLVMValueRef frame_idx =
      LLVMBuildLoad2(builder, i32_ty, dsp_ctx->frame_idx_ptr, "frame_idx");
  LLVMValueRef has_more =
      LLVMBuildICmp(builder, LLVMIntSLT, frame_idx, nframes, "frame_idx.lt");

  LLVMBuildCondBr(builder, has_more, body_bb, exit_bb);

  LLVMPositionBuilderAtEnd(builder, body_bb);
  dsp_ctx->frame_idx = frame_idx;
  return exit_bb;
}

LLVMTypeRef synth_frame_fn_type(Ast *lambda, JITLangCtx *ctx,
                                LLVMModuleRef module, LLVMBuilderRef builder) {

  int num_inputs = 0;
  bool is_void_fn = is_void_func(lambda->type);

  if (!is_void_fn) {
    for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
      if (p->ast->tag == AST_IDENTIFIER) {
        num_inputs++;
      } else if (p->ast->tag == AST_TUPLE) {
        num_inputs += p->ast->data.AST_LIST.len;
      }
    }
  }
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);

  LLVMTypeRef *frame_param_tys =
      malloc(sizeof(LLVMTypeRef) * (size_t)(num_inputs + 3));
  frame_param_tys[0] = GENERIC_PTR; // state ptr
  frame_param_tys[1] = GENERIC_PTR; // enclosing node ptr
  frame_param_tys[2] = LLVMPointerType(LLVMDoubleTypeInContext(llvm_ctx), 0);
  // output ptr
  for (int i = 0; i < num_inputs; i++) {

    frame_param_tys[i + 3] = LLVMDoubleType();
  }
  LLVMTypeRef frame_ty = LLVMFunctionType(LLVMVoidTypeInContext(llvm_ctx),
                                          frame_param_tys, num_inputs + 3, 0);
  return frame_ty;
}

SynthRecord compile_lambda_to_synth_record(Ast *lambda, const char *name,
                                           LLVMTypeRef frame_ty,
                                           DspBuildCtx *enclosing_dsp_ctx,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,
                                           LLVMBuilderRef builder) {
  escape_analysis(lambda);

  int num_inputs = 0;
  bool is_void_fn = is_void_func(lambda->type);
  //
  if (!is_void_fn) {
    for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
      if (p->ast->tag == AST_IDENTIFIER) {
        num_inputs++;
      } else if (p->ast->tag == AST_TUPLE) {
        num_inputs += p->ast->data.AST_LIST.len;
      }
    }
  }
  DspSynthArgKind *arg_kinds =
      num_inputs ? malloc(sizeof(DspSynthArgKind) * (size_t)num_inputs) : NULL;
  int arg_kind_idx = 0;
  Type *input_type = lambda->type;
  if (!is_void_fn && arg_kinds) {
    for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
      if (!input_type || input_type->kind != T_FN) {
        break;
      }
      Type *param_type = input_type->data.T_FN.from;
      if (p->ast->tag == AST_TUPLE && param_type &&
          param_type->kind == T_CONS) {
        for (int j = 0; j < p->ast->data.AST_LIST.len; j++) {
          Type *field_type = j < param_type->data.T_CONS.num_args
                                 ? param_type->data.T_CONS.args[j]
                                 : NULL;
          arg_kinds[arg_kind_idx++] = classify_synth_arg_kind(field_type);
        }
      } else {
        arg_kinds[arg_kind_idx++] = classify_synth_arg_kind(param_type);
      }
      input_type = input_type->data.T_FN.to;
    }
  }
  // Type *ftype = lambda->type;
  //
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  // Build Synth Perform func scaffold
  LLVMTypeRef perf_ty =
      LLVMFunctionType(GENERIC_PTR,
                       (LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR, GENERIC_PTR,
                                       LLVMInt32Type(), LLVMDoubleType()},
                       5, 0);
  //
  // LLVMTypeRef *frame_param_tys =
  //     malloc(sizeof(LLVMTypeRef) * (size_t)(num_inputs + 2));
  // frame_param_tys[0] = GENERIC_PTR; // state ptr
  // frame_param_tys[1] = GENERIC_PTR; // enclosing node ptr
  // for (int i = 0; i < num_inputs; i++) {
  //
  //   frame_param_tys[i + 2] = LLVMDoubleType();
  // }

  char perf_name[32];
  sprintf(perf_name, "%s.perform", name);
  LLVMValueRef perf_fn = LLVMAddFunction(module, perf_name, perf_ty);
  char frame_name[32];
  sprintf(frame_name, "%s.frame", name);
  LLVMValueRef frame_fn = LLVMAddFunction(module, frame_name, frame_ty);
  char init_name[32];
  sprintf(init_name, "%s.init", name);
  LLVMTypeRef init_ty =
      LLVMFunctionType(LLVMVoidType(), (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);
  LLVMValueRef init_fn = LLVMAddFunction(module, init_name, init_ty);

  // Build synth cons func scaffold
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef i64_ty = LLVMInt64Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();
  LLVMTypeRef *cons_param_tys =
      num_inputs ? malloc(sizeof(LLVMTypeRef) * num_inputs) : NULL;
  for (int i = 0; i < num_inputs; i++) {
    cons_param_tys[i] = f64_ty;
  }
  LLVMTypeRef cons_ty =
      LLVMFunctionType(GENERIC_PTR, cons_param_tys, num_inputs, 0);
  char cons_name[32];
  sprintf(cons_name, "%s.cons", name);
  LLVMValueRef cons_fn = LLVMAddFunction(module, cons_name, cons_ty);
  LLVMSetLinkage(cons_fn, LLVMExternalLinkage);
  LLVMBasicBlockRef ctor_alloc_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, cons_fn, "alloc");
  LLVMBasicBlockRef ctor_init_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, cons_fn, "init");

  LLVMBuilderRef ctor_b = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMPositionBuilderAtEnd(ctor_b, ctor_alloc_bb);

  LLVMBasicBlockRef perf_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, perf_fn, "entry");
  LLVMBasicBlockRef frame_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, frame_fn, "entry");
  LLVMBasicBlockRef init_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, init_fn, "entry");
  LLVMBuilderRef init_b = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMPositionBuilderAtEnd(init_b, init_bb);

  int compile_sample_rate = ctx_sample_rate();

  if (compile_sample_rate <= 0) {
    compile_sample_rate = 48000;
  }

  double compile_spf = ctx_spf();
  if (compile_spf <= 0.0) {
    compile_spf = 1.0 / (double)compile_sample_rate;
  }

  DspTmpAllocator tmp_alloc = {0};
  dsp_tmp_allocator_init(&tmp_alloc, 1024);

  DspBuildCtx dsp_ctx = {
      .ctor_builder = ctor_b,
      .init_builder = init_b,
      .perform_builder = LLVMCreateBuilderInContext(llvm_ctx),
      .perf_fn = perf_fn,
      .sample_rate = compile_sample_rate,
      .spf_scalar = compile_spf,
      .spectral_fft_size =
          enclosing_dsp_ctx ? enclosing_dsp_ctx->spectral_fft_size : 0,
      .spectral_hop_size =
          enclosing_dsp_ctx ? enclosing_dsp_ctx->spectral_hop_size : 0,
      .tmp_alloc = &tmp_alloc,
  };

  DspBuildCtx frame_ctx = {
      .ctor_builder = ctor_b,
      .init_builder = init_b,
      .perform_builder = LLVMCreateBuilderInContext(llvm_ctx),
      .perf_fn = frame_fn,
      .sample_rate = compile_sample_rate,
      .spf_scalar = compile_spf,
      .spectral_fft_size =
          enclosing_dsp_ctx ? enclosing_dsp_ctx->spectral_fft_size : 0,
      .spectral_hop_size =
          enclosing_dsp_ctx ? enclosing_dsp_ctx->spectral_hop_size : 0,
      .tmp_alloc = &tmp_alloc,
  };

  LLVMTypeRef create_param_tys[] = {GENERIC_PTR, i32_ty, i32_ty, i32_ty,
                                    GENERIC_PTR};
  LLVMTypeRef create_fn_ty =
      LLVMFunctionType(GENERIC_PTR, create_param_tys, 5, 0);
  LLVMValueRef create_fn =
      LLVMGetNamedFunction(module, "ylc_create_audio_node");
  if (!create_fn) {
    create_fn = LLVMAddFunction(module, "ylc_create_audio_node", create_fn_ty);
    LLVMSetLinkage(create_fn, LLVMExternalLinkage);
  }

  LLVMValueRef meta_str =
      LLVMBuildGlobalStringPtr(ctor_b, name ? name : "?", "synth.meta");
  LLVMValueRef create_args[] = {
      perf_fn,
      LLVMConstInt(i32_ty, (uint64_t)num_inputs, 0),
      LLVMConstInt(i32_ty, 1, 0),
      LLVMConstInt(i32_ty, 0, 0),
      meta_str,
  };
  LLVMValueRef node_val =
      LLVMBuildCall2(ctor_b, create_fn_ty, create_fn, create_args, 5, "node");
  LLVMBuildBr(ctor_b, ctor_init_bb);

  LLVMPositionBuilderAtEnd(ctor_b, ctor_init_bb);
  LLVMTypeRef i8_ty = LLVMInt8TypeInContext(llvm_ctx);
  LLVMValueRef node_i8 =
      LLVMBuildPointerCast(ctor_b, node_val, GENERIC_PTR, "node.i8");
  LLVMValueRef state_base_off = LLVMConstInt(i32_ty, (uint64_t)sizeof(Node), 0);
  LLVMValueRef ctor_state_ptr =
      LLVMBuildGEP2(ctor_b, i8_ty, node_i8, &state_base_off, 1, "node.state");
  LLVMValueRef init_state_ptr = LLVMGetParam(init_fn, 0);

  dsp_ctx.create_call = node_val;
  frame_ctx.create_call = node_val;

  dsp_ctx.init_state_ptr = init_state_ptr;
  dsp_ctx.init_state_base_ptr = init_state_ptr;
  frame_ctx.init_state_ptr = init_state_ptr;
  frame_ctx.init_state_base_ptr = init_state_ptr;

  {
    LLVMTypeRef i8_ptr_ty = LLVMPointerType(i8_ty, 0);
    frame_ctx.init_state_cursor_ptr =
        LLVMBuildAlloca(init_b, i8_ptr_ty, "init.state_cursor");
    LLVMBuildStore(init_b, init_state_ptr, frame_ctx.init_state_cursor_ptr);
  }

  LLVMPositionBuilderAtEnd(dsp_ctx.perform_builder, perf_bb);
  LLVMBasicBlockRef perf_exit_bb =
      dsp_build_perform_loop(perf_fn, &dsp_ctx, llvm_ctx);

  LLVMPositionBuilderAtEnd(frame_ctx.perform_builder, frame_bb);
  frame_ctx.state_ptr = LLVMGetParam(frame_fn, 0);
  frame_ctx.state_base_ptr = frame_ctx.state_ptr;
  frame_ctx.node_ptr = LLVMGetParam(frame_fn, 1);
  LLVMValueRef frame_out_ptr = LLVMGetParam(frame_fn, 2);
  {
    LLVMTypeRef i8_ptr_ty = LLVMPointerType(i8_ty, 0);
    frame_ctx.state_cursor_ptr = LLVMBuildAlloca(
        frame_ctx.perform_builder, i8_ptr_ty, "frame.state_cursor");
    LLVMBuildStore(frame_ctx.perform_builder, frame_ctx.state_ptr,
                   frame_ctx.state_cursor_ptr);
  }
  {
    LLVMTypeRef f64_ty_local = LLVMDoubleType();
    LLVMTypeRef spf_fn_ty =
        LLVMFunctionType(f64_ty_local, (LLVMTypeRef[]){}, 0, 0);
    LLVMValueRef spf_fn = LLVMGetNamedFunction(module, "ctx_spf");
    if (!spf_fn) {
      spf_fn = LLVMAddFunction(module, "ctx_spf", spf_fn_ty);
      LLVMSetLinkage(spf_fn, LLVMExternalLinkage);
    }
    frame_ctx.spf = LLVMBuildCall2(frame_ctx.perform_builder, spf_fn_ty, spf_fn,
                                   NULL, 0, "ctx_spf");
  }

  STACK_ALLOC_CTX_PUSH(fn_ctx, ctx)

  Type *fn_type = lambda->type;
  int idx = 0;

  if (!is_void_fn) {
    for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next, idx++) {
      Ast *param_ast = p->ast;
      Type *param_type = fn_type->data.T_FN.from;
      LLVMValueRef arg_val = LLVMGetParam(frame_fn, idx + 3);

      if (param_ast->tag == AST_TUPLE) {
        // Destructure tuple param: extract each field and bind its identifier
        int nfields = param_ast->data.AST_LIST.len;
        for (int j = 0; j < nfields; j++) {
          Ast *field_ast = param_ast->data.AST_LIST.items + j;
          Type *field_type = param_type->data.T_CONS.args[j];
          LLVMValueRef field_val = LLVMBuildExtractValue(
              frame_ctx.perform_builder, arg_val, (unsigned)j, "tuple.field");
          LLVMTypeRef field_llvm_ty = LLVMTypeOf(field_val);
          JITSymbol *field_sym =
              new_symbol(STYPE_LOCAL_VAR, field_type, field_val, field_llvm_ty);
          const char *field_chars = field_ast->data.AST_IDENTIFIER.value;
          int field_len = field_ast->data.AST_IDENTIFIER.length;
          ht_set_hash(fn_ctx.frame->table, field_chars,
                      hash_string(field_chars, field_len), field_sym);
        }
      } else {
        JITSymbol *sym =
            new_symbol(STYPE_LOCAL_VAR, param_type, arg_val, f64_ty);
        const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
        int id_len = param_ast->data.AST_IDENTIFIER.length;
        ht_set_hash(fn_ctx.frame->table, id_chars,
                    hash_string(id_chars, id_len), sym);
      }

      fn_type = fn_type->data.T_FN.to;
    }
  }

  // Bind closed-over values from extra frame params (appended after lambda
  // params). The closed_vals list and closure_meta->args are in the same order.
  if (lambda->tag == AST_LAMBDA) {
    int cap_idx = idx;
    for (AstList *cv = lambda->data.AST_LAMBDA.closed_vals; cv;
         cv = cv->next, cap_idx++) {
      Ast *cl = cv->ast;
      if (cl->tag == AST_IDENTIFIER) {
        LLVMValueRef cap_val = LLVMGetParam(frame_fn, cap_idx + 3);
        LLVMTypeRef cap_llvm_ty = LLVMTypeOf(cap_val);
        JITSymbol *sym =
            new_symbol(STYPE_LOCAL_VAR, cl->type, cap_val, cap_llvm_ty);
        const char *name = cl->data.AST_IDENTIFIER.value;
        int len = cl->data.AST_IDENTIFIER.length;
        ht_set_hash(fn_ctx.frame->table, name, hash_string(name, len), sym);
      }
    }
  }

  DspValue expr = dsp_build_expr(lambda->data.AST_LAMBDA.body, &frame_ctx,
                                 &fn_ctx, module, frame_ctx.perform_builder);
  int output_lanes = expr.lanes > 0 ? expr.lanes : 1;
  for (int i = 0; i < output_lanes; i++) {
    LLVMValueRef lane_val = expr.lanes > 1 ? expr.vec[i] : expr.scalar;
    lane_val = ensure_float(lambda->data.AST_LAMBDA.body->type, lane_val,
                            frame_ctx.perform_builder);
    LLVMValueRef lane_idx = LLVMConstInt(i64_ty, (uint64_t)i, 0);
    LLVMValueRef lane_ptr =
        LLVMBuildGEP2(frame_ctx.perform_builder, f64_ty, frame_out_ptr,
                      &lane_idx, 1, "frame.out.ptr");
    LLVMBuildStore(frame_ctx.perform_builder, lane_val, lane_ptr);
  }
  if (!LLVMGetBasicBlockTerminator(
          LLVMGetInsertBlock(frame_ctx.perform_builder))) {
    LLVMBuildRetVoid(frame_ctx.perform_builder);
  }
  if (!LLVMGetBasicBlockTerminator(
          LLVMGetInsertBlock(frame_ctx.init_builder))) {
    LLVMBuildRetVoid(frame_ctx.init_builder);
  }

  LLVMTypeRef ptr_ptr_ty = LLVMPointerType(GENERIC_PTR, 0);
  LLVMValueRef inputs_ptr_cast =
      LLVMBuildPointerCast(dsp_ctx.perform_builder, dsp_ctx.inputs_ptr,
                           ptr_ptr_ty, "inputs_ptr.cast");
  LLVMTypeRef read_param_tys[] = {GENERIC_PTR, i64_ty};
  LLVMTypeRef read_fn_ty = LLVMFunctionType(f64_ty, read_param_tys, 2, 0);
  LLVMValueRef read_fn = LLVMGetNamedFunction(module, "ylc_read_inlet_node");
  if (!read_fn) {
    read_fn = LLVMAddFunction(module, "ylc_read_inlet_node", read_fn_ty);
    LLVMSetLinkage(read_fn, LLVMExternalLinkage);
  }
  LLVMValueRef frame_i64 = LLVMBuildSExt(
      dsp_ctx.perform_builder, dsp_ctx.frame_idx, i64_ty, "frame_idx.i64");
  // Use the actual frame_ty param count as ground truth, not num_inputs.
  // num_inputs may over-count when tuple params are expanded from AST, but
  // frame_ty already encodes the real function signature (e.g. tuple element
  // as a single struct param).
  unsigned frame_total_params = LLVMCountParamTypes(frame_ty);
  unsigned frame_user_params =
      frame_total_params > 3 ? frame_total_params - 3 : 0;

  LLVMTypeRef *frame_formal_tys =
      malloc(sizeof(LLVMTypeRef) * (size_t)frame_total_params);
  LLVMGetParamTypes(frame_ty, frame_formal_tys);

  LLVMValueRef *frame_call_args =
      malloc(sizeof(LLVMValueRef) * (size_t)frame_total_params);
  frame_call_args[0] = dsp_ctx.state_ptr;
  frame_call_args[1] = dsp_ctx.node_ptr;
  LLVMTypeRef get_output_buf_fn_ty =
      LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){GENERIC_PTR}, 1, 0);
  LLVMValueRef get_output_buf_fn =
      LLVMGetNamedFunction(module, "ylc_get_output_buf");
  if (!get_output_buf_fn) {
    get_output_buf_fn =
        LLVMAddFunction(module, "ylc_get_output_buf", get_output_buf_fn_ty);
    LLVMSetLinkage(get_output_buf_fn, LLVMExternalLinkage);
  }
  LLVMValueRef output_buf_i8 = LLVMBuildCall2(
      dsp_ctx.perform_builder, get_output_buf_fn_ty, get_output_buf_fn,
      (LLVMValueRef[]){dsp_ctx.node_ptr}, 1, "output.buf.i8");
  LLVMValueRef output_buf =
      LLVMBuildPointerCast(dsp_ctx.perform_builder, output_buf_i8,
                           LLVMPointerType(f64_ty, 0), "output.buf");
  LLVMValueRef output_lanes_i64 =
      LLVMConstInt(i64_ty, (uint64_t)output_lanes, 0);
  LLVMValueRef frame_offset = LLVMBuildMul(dsp_ctx.perform_builder, frame_i64,
                                           output_lanes_i64, "frame.out.off");
  LLVMValueRef frame_out_dest_ptr =
      LLVMBuildGEP2(dsp_ctx.perform_builder, f64_ty, output_buf, &frame_offset,
                    1, "frame.out.ptr");
  frame_call_args[2] = frame_out_dest_ptr;
  for (unsigned i = 0; i < frame_user_params; i++) {
    LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, (uint64_t)i, 0);
    LLVMValueRef inlet_slot =
        LLVMBuildGEP2(dsp_ctx.perform_builder, GENERIC_PTR, inputs_ptr_cast,
                      &idx_i64, 1, "inlet.slot");
    LLVMValueRef inlet_node = LLVMBuildLoad2(
        dsp_ctx.perform_builder, GENERIC_PTR, inlet_slot, "inlet.node");
    LLVMValueRef read_args[] = {inlet_node, frame_i64};
    LLVMValueRef sample = LLVMBuildCall2(dsp_ctx.perform_builder, read_fn_ty,
                                         read_fn, read_args, 2, "inlet.sample");
    LLVMTypeRef formal_ty = frame_formal_tys[i + 3];
    if (LLVMGetTypeKind(formal_ty) == LLVMIntegerTypeKind) {
      sample = LLVMBuildFPToSI(dsp_ctx.perform_builder, sample, formal_ty,
                               "inlet.sample.i");
    } else if (LLVMGetTypeKind(formal_ty) == LLVMStructTypeKind) {
      // Struct params can't be read from a scalar double inlet.
      // Use undef — the perform function for map/fold lambdas is never called
      // at runtime (only frame_fn is), so this is safe.
      sample = LLVMGetUndef(formal_ty);
    }
    frame_call_args[i + 3] = sample;
  }
  free(frame_formal_tys);
  LLVMBuildCall2(dsp_ctx.perform_builder, frame_ty, frame_fn, frame_call_args,
                 frame_total_params, "frame.call");
  free(frame_call_args);

  if (!LLVMGetBasicBlockTerminator(
          LLVMGetInsertBlock(dsp_ctx.perform_builder))) {
    LLVMTypeRef i32_ty = LLVMInt32TypeInContext(llvm_ctx);
    LLVMValueRef one = LLVMConstInt(i32_ty, 1, 0);
    LLVMValueRef next_idx = LLVMBuildAdd(
        dsp_ctx.perform_builder, dsp_ctx.frame_idx, one, "frame_idx.next");
    LLVMBuildStore(dsp_ctx.perform_builder, next_idx, dsp_ctx.frame_idx_ptr);
    LLVMBuildBr(dsp_ctx.perform_builder, dsp_ctx.frame_cond_bb);
  }
  LLVMPositionBuilderAtEnd(dsp_ctx.perform_builder, perf_exit_bb);

  if (!LLVMGetBasicBlockTerminator(
          LLVMGetInsertBlock(dsp_ctx.perform_builder))) {
    LLVMBuildRet(dsp_ctx.perform_builder, LLVMConstNull(GENERIC_PTR));
  }
  int state_bytes = 0;

  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(dsp_ctx.ctor_builder))) {
    state_bytes = (frame_ctx.state_offset + 7) & ~7;
    LLVMSetOperand(dsp_ctx.create_call, 3,
                   LLVMConstInt(i32_ty, (uint64_t)state_bytes, 0));
    LLVMSetOperand(dsp_ctx.create_call, 2,
                   LLVMConstInt(i32_ty, (uint64_t)output_lanes, 0));
    LLVMBuildCall2(dsp_ctx.ctor_builder, init_ty, init_fn,
                   (LLVMValueRef[]){ctor_state_ptr}, 1, "");

    if (num_inputs > 0) {
      LLVMTypeRef const_inlet_fn_ty =
          LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){f64_ty}, 1, 0);
      LLVMValueRef const_inlet_fn =
          LLVMGetNamedFunction(module, "ylc_const_inlet");
      if (!const_inlet_fn) {
        const_inlet_fn =
            LLVMAddFunction(module, "ylc_const_inlet", const_inlet_fn_ty);
        LLVMSetLinkage(const_inlet_fn, LLVMExternalLinkage);
      }

      LLVMTypeRef plug_input_tys[] = {i32_ty, GENERIC_PTR, GENERIC_PTR};
      LLVMTypeRef plug_input_fn_ty = LLVMFunctionType(
          LLVMVoidTypeInContext(llvm_ctx), plug_input_tys, 3, 0);
      LLVMValueRef plug_input_fn =
          LLVMGetNamedFunction(module, "plug_input_in_graph");
      if (!plug_input_fn) {
        plug_input_fn =
            LLVMAddFunction(module, "plug_input_in_graph", plug_input_fn_ty);
        LLVMSetLinkage(plug_input_fn, LLVMExternalLinkage);
      }

      for (int i = 0; i < num_inputs; i++) {
        LLVMValueRef idx_i32 = LLVMConstInt(i32_ty, (uint64_t)i, 0);
        LLVMValueRef param_val = LLVMGetParam(cons_fn, i);

        LLVMValueRef inlet_args[] = {param_val};
        LLVMValueRef const_node =
            LLVMBuildCall2(dsp_ctx.ctor_builder, const_inlet_fn_ty,
                           const_inlet_fn, inlet_args, 1, "const_inlet");
        LLVMValueRef attach_args[] = {idx_i32, node_val, const_node};
        LLVMBuildCall2(dsp_ctx.ctor_builder, plug_input_fn_ty, plug_input_fn,
                       attach_args, 3, "");
      }
    }

    LLVMBuildRet(dsp_ctx.ctor_builder, node_val);
  }
  if (cons_param_tys) {
    free(cons_param_tys);
  }

  LLVMDisposeBuilder(dsp_ctx.ctor_builder);
  LLVMDisposeBuilder(dsp_ctx.init_builder);
  LLVMDisposeBuilder(dsp_ctx.perform_builder);
  LLVMDisposeBuilder(frame_ctx.perform_builder);
  dsp_tmp_allocator_free(&tmp_alloc);
  destroy_ctx(&fn_ctx);

  return (SynthRecord){.name = name,
                       .ctor = cons_fn,
                       .init_fn = init_fn,
                       .frame_fn = frame_fn,
                       .perform_fn = perf_fn,
                       .arg_count = num_inputs,
                       .arg_kinds = arg_kinds,
                       .output_lanes = output_lanes,
                       .state_bytes = state_bytes};
}
void print_synth_record(SynthRecord rec) {

  printf("synth record %s bytes: %d\n", rec.name, rec.state_bytes);
  printf("init fn: \n");
  LLVMDumpValue(rec.init_fn);
  printf("\n");

  printf("frame_fn: \n");
  LLVMDumpValue(rec.frame_fn);
  printf("\n");
}

LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  Ast *source = ast->data.AST_APPLICATION.args;
  Ast *lambda = source->data.AST_LET.expr;
  Ast *binding = source->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;

  LLVMTypeRef frame_ty = synth_frame_fn_type(lambda, ctx, module, builder);

  // Determine synth_id before compiling so we can emit the registration call.
  JITSymbol *existing = lookup_id_ast(binding, ctx);
  int synth_id;
  if (existing && existing->type == (symbol_type)STYPE_AUDIO_JIT_SYM) {
    synth_id = audio_sym_synth_id(existing);
  } else {
    synth_id = synth_registry_len();
  }

  SynthRecord rec = compile_lambda_to_synth_record(
      lambda, name, frame_ty, NULL, ctx, module, builder);

  // Emit a runtime call to ylc_register_synth_ctor(synth_id, cons_fn) so that
  // synth_ctor_table is populated with the real compiled address when the
  // top-level function executes.
  LLVMTypeRef reg_fn_ty = LLVMFunctionType(
      LLVMVoidType(), (LLVMTypeRef[]){LLVMInt32Type(), GENERIC_PTR}, 2, 0);
  LLVMValueRef reg_fn = LLVMGetNamedFunction(module, "ylc_register_synth_ctor");
  if (!reg_fn) {
    reg_fn = LLVMAddFunction(module, "ylc_register_synth_ctor", reg_fn_ty);
    LLVMSetLinkage(reg_fn, LLVMExternalLinkage);
  }
  LLVMValueRef synth_id_val =
      LLVMConstInt(LLVMInt32Type(), (unsigned long long)synth_id, 0);
  LLVMBuildCall2(builder, reg_fn_ty, reg_fn,
                 (LLVMValueRef[]){synth_id_val, rec.ctor}, 2, "");

  if (existing && existing->type == (symbol_type)STYPE_AUDIO_JIT_SYM) {
    // Preserve the old ctor_ptr so the scheduler can keep calling the previous
    // constructor until ylc_register_synth_ctor fires with the new address.
    void *old_ctor_ptr = atomic_load_explicit(
        &synth_registry.records[synth_id].ctor_ptr, memory_order_relaxed);
    synth_registry.records[synth_id] = rec;
    atomic_store_explicit(&synth_registry.records[synth_id].ctor_ptr,
                          old_ctor_ptr, memory_order_relaxed);
    return rec.ctor;
  }

  extend_synth_registry(rec);

  JITSymbol *sym =
      new_symbol((symbol_type)STYPE_AUDIO_JIT_SYM, NULL, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = call_dsp_symbol;
  sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr = synth_id;
  ht_set_hash(ctx->frame->table, name, hash_string(name, strlen(name)), sym);

  return rec.ctor;
}

SynthRecord synth_registry_get(int synth_id) {
  return synth_registry.records[synth_id];
}
void *synth_registry_get_ctor_ptr(int synth_id) {
  return atomic_load_explicit(&synth_registry.records[synth_id].ctor_ptr,
                              memory_order_acquire);
}
void synth_registry_set_ctor_ptr(int synth_id, void *ctor_ptr) {
  atomic_store_explicit(&synth_registry.records[synth_id].ctor_ptr, ctor_ptr,
                        memory_order_release);
}
int synth_registry_len() { return synth_registry.length; }
