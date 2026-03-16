#include "./compile_synth.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/serde.h"
#include "./audio_jit.h"

#include "../../engine/audio_graph.h"
#include "../../engine/common.h"
#include "../../engine/ctx.h"
#include "../../engine/node.h"
#include "../../lang/backend_llvm/application.h"
#include "../../lang/backend_llvm/array.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/function_extern.h"
#include "../../lang/backend_llvm/lib_registry.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/backend_llvm/types.h"
#include "../../lang/common.h"
#include "../../lang/ht.h"
#include "../../lang/serde.h"
#include "../../lang/types/builtins.h"
#include "../../lang/types/inference.h"
#include "../../lang/types/type_ser.h"
#include "../../lang/ylc_datatypes.h"
#include <string.h>

static SynthRegistry synth_registry;

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

  LLVMValueRef node = LLVMBuildCall2(builder, ctor_fn_ty, ctor_fn, ctor_args,
                                     formal_count, "audio_jit.node");
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
LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  Ast *source = ast->data.AST_APPLICATION.args;
  Ast *lambda = source->data.AST_LET.expr;
  Ast *binding = source->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;
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

  fprintf(stderr, "compile audio function for %s\n", name);
  // print_ast(lambda);
  // print_type(lambda->type);
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  // Build Synth Perform func scaffold
  LLVMTypeRef perf_ty =
      LLVMFunctionType(GENERIC_PTR,
                       (LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR, GENERIC_PTR,
                                       LLVMInt32Type(), LLVMDoubleType()},
                       5, 0);
  LLVMTypeRef *frame_param_tys =
      malloc(sizeof(LLVMTypeRef) * (size_t)(num_inputs + 2));
  frame_param_tys[0] = GENERIC_PTR; // state ptr (already offset by caller)
  for (int i = 0; i < num_inputs; i++) {
    frame_param_tys[i + 1] = LLVMDoubleType();
  }
  frame_param_tys[num_inputs + 1] = GENERIC_PTR; // enclosing node ptr
  LLVMTypeRef frame_ty =
      LLVMFunctionType(LLVMDoubleType(), frame_param_tys, num_inputs + 2, 0);

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

  DspBuildCtx dsp_ctx = {
      .ctor_builder = ctor_b,
      .init_builder = init_b,
      .perform_builder = LLVMCreateBuilderInContext(llvm_ctx),
      .perf_fn = perf_fn,
  };
  DspBuildCtx frame_ctx = {
      .ctor_builder = ctor_b,
      .init_builder = init_b,
      .perform_builder = LLVMCreateBuilderInContext(llvm_ctx),
      .perf_fn = frame_fn,
  };

  LLVMTypeRef create_param_tys[] = {GENERIC_PTR, i32_ty, i32_ty};
  LLVMTypeRef create_fn_ty =
      LLVMFunctionType(GENERIC_PTR, create_param_tys, 3, 0);
  LLVMValueRef create_fn =
      LLVMGetNamedFunction(module, "ylc_create_audio_node");
  if (!create_fn) {
    create_fn = LLVMAddFunction(module, "ylc_create_audio_node", create_fn_ty);
    LLVMSetLinkage(create_fn, LLVMExternalLinkage);
  }

  LLVMValueRef create_args[] = {
      perf_fn,
      LLVMConstInt(i32_ty, (uint64_t)num_inputs, 0),
      LLVMConstInt(i32_ty, 0, 0),
  };
  LLVMValueRef node_val =
      LLVMBuildCall2(ctor_b, create_fn_ty, create_fn, create_args, 3, "node");
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
  frame_ctx.init_state_ptr = init_state_ptr;

  LLVMPositionBuilderAtEnd(dsp_ctx.perform_builder, perf_bb);
  LLVMBasicBlockRef perf_exit_bb =
      dsp_build_perform_loop(perf_fn, &dsp_ctx, llvm_ctx);

  LLVMPositionBuilderAtEnd(frame_ctx.perform_builder, frame_bb);
  frame_ctx.state_ptr = LLVMGetParam(frame_fn, 0);
  frame_ctx.node_ptr = LLVMGetParam(frame_fn, num_inputs + 1);
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
      LLVMValueRef arg_val = LLVMGetParam(frame_fn, idx + 1);
      JITSymbol *sym = new_symbol(STYPE_LOCAL_VAR, param_type, arg_val, f64_ty);

      const char *id_chars = param_ast->data.AST_IDENTIFIER.value;
      int id_len = param_ast->data.AST_IDENTIFIER.length;
      ht_set_hash(fn_ctx.frame->table, id_chars, hash_string(id_chars, id_len),
                  sym);

      fn_type = fn_type->data.T_FN.to;
    }
  }
  LLVMValueRef expr =
      dsp_build_expr(lambda->data.AST_LAMBDA.body, &frame_ctx, &fn_ctx, module,
                     frame_ctx.perform_builder);
  fprintf(stderr, "compile_audio_fn: body built for %s\n", name);

  if (expr) {
    expr = ensure_float(lambda->data.AST_LAMBDA.body->type, expr,
                        frame_ctx.perform_builder);
    LLVMBuildRet(frame_ctx.perform_builder, expr);
  }
  if (!LLVMGetBasicBlockTerminator(
          LLVMGetInsertBlock(frame_ctx.perform_builder))) {
    LLVMBuildRet(frame_ctx.perform_builder, LLVMConstReal(f64_ty, 0.0));
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
  LLVMValueRef *frame_call_args =
      malloc(sizeof(LLVMValueRef) * (size_t)(num_inputs + 2));
  frame_call_args[0] = dsp_ctx.state_ptr;
  for (int i = 0; i < num_inputs; i++) {
    LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, (uint64_t)i, 0);
    LLVMValueRef inlet_slot =
        LLVMBuildGEP2(dsp_ctx.perform_builder, GENERIC_PTR, inputs_ptr_cast,
                      &idx_i64, 1, "inlet.slot");
    LLVMValueRef inlet_node = LLVMBuildLoad2(
        dsp_ctx.perform_builder, GENERIC_PTR, inlet_slot, "inlet.node");
    LLVMValueRef read_args[] = {inlet_node, frame_i64};
    frame_call_args[i + 1] =
        LLVMBuildCall2(dsp_ctx.perform_builder, read_fn_ty, read_fn, read_args,
                       2, "inlet.sample");
  }
  frame_call_args[num_inputs + 1] = dsp_ctx.node_ptr;
  LLVMValueRef frame_sample =
      LLVMBuildCall2(dsp_ctx.perform_builder, frame_ty, frame_fn,
                     frame_call_args, num_inputs + 2, "frame.call");
  free(frame_call_args);

  LLVMTypeRef void_ty = LLVMVoidType();
  LLVMTypeRef write_param_tys[] = {GENERIC_PTR, i64_ty, f64_ty};
  LLVMTypeRef write_fn_ty = LLVMFunctionType(void_ty, write_param_tys, 3, 0);
  LLVMValueRef write_fn = LLVMGetNamedFunction(module, "dsp_write_output");
  if (!write_fn) {
    write_fn = LLVMAddFunction(module, "dsp_write_output", write_fn_ty);
    LLVMSetLinkage(write_fn, LLVMExternalLinkage);
  }
  LLVMValueRef write_args[] = {dsp_ctx.node_ptr, frame_i64, frame_sample};
  LLVMBuildCall2(dsp_ctx.perform_builder, write_fn_ty, write_fn, write_args, 3,
                 "");

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
    LLVMSetOperand(dsp_ctx.create_call, 2,
                   LLVMConstInt(i32_ty, (uint64_t)state_bytes, 0));
    LLVMBuildCall2(dsp_ctx.ctor_builder, init_ty, init_fn,
                   (LLVMValueRef[]){ctor_state_ptr}, 1, "node.init");

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
  free(frame_param_tys);

  JITSymbol *sym =
      new_symbol((symbol_type)STYPE_AUDIO_JIT_SYM, NULL, NULL, NULL);

  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = call_dsp_symbol;
  int synth_id =
      extend_synth_registry((SynthRecord){.name = name,
                                          .ctor = cons_fn,
                                          .init_fn = init_fn,
                                          .frame_fn = frame_fn,
                                          .perform_fn = perf_fn,
                                          .state_bytes = state_bytes});
  sym->symbol_data.STYPE_GENERIC_FUNCTION.stack_ptr = synth_id;

  ht_set_hash(ctx->frame->table, name, hash_string(name, strlen(name)), sym);
  fprintf(stderr, "libaudio_jit: audio fn symbol %s\n", name);

  LLVMDisposeBuilder(dsp_ctx.ctor_builder);
  LLVMDisposeBuilder(dsp_ctx.init_builder);
  LLVMDisposeBuilder(dsp_ctx.perform_builder);
  LLVMDisposeBuilder(frame_ctx.perform_builder);
  destroy_ctx(&fn_ctx);

  printf("compiled for %s: \n", name);
  LLVMDumpValue(perf_fn);
  printf("\n");
  LLVMDumpValue(frame_fn);
  printf("\n");

  LLVMDumpValue(cons_fn);
  printf("\n");
  return cons_fn;
}

SynthRecord synth_registry_get(int synth_id) {
  return synth_registry.records[synth_id];
}
int synth_registry_len() { return synth_registry.length; }
