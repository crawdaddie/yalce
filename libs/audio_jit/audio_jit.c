#include "./audio_jit.h"

#include "../../engine/common.h"
#include "../../engine/node.h"
#include "../../lang/backend_llvm/codegen.h"
#include "../../lang/backend_llvm/lib_registry.h"
#include "../../lang/backend_llvm/symbols.h"
#include "../../lang/common.h"
#include "../../lang/ht.h"
#include "../../lang/serde.h"
#include "../../lang/types/type_ser.h"

#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int STYPE_AUDIO_JIT_SYM;
int STYPE_AUDIO_JIT_INLINE_SYM;
int STYPE_AUDIO_JIT_BUILTIN_HANDLER;

Node *ylc_create_audio_node(perform_func_t perform, int num_inputs,
                            int state_bytes) {
  size_t total =
      sizeof(Node) + (size_t)state_bytes + ((size_t)BUF_SIZE * sizeof(double));
  Node *node = (Node *)calloc(1, total);
  if (!node) {
    return NULL;
  }

  node->perform = perform;
  node->num_inputs = num_inputs;
  node->state_size = state_bytes;
  node->meta = (char *)"audio_jit_synth";
  node->output = (Signal){
      .layout = 1,
      .size = BUF_SIZE,
      .buf = (double *)((char *)node + sizeof(Node) + state_bytes),
  };
  node->next = NULL;

  return node;
}

void ylc_write_output(void *node_raw, int64_t frame, double val) {
  ((Node *)node_raw)->output.buf[frame] = val;
}

static LLVMValueRef call_dsp_symbol(Ast *ast, JITLangCtx *ctx,
                                    LLVMModuleRef module_ref,
                                    LLVMBuilderRef builder) {
  Ast *sym_id = ast->data.AST_APPLICATION.function;
  JITSymbol *sym = lookup_id_ast(sym_id, ctx);
  if (!sym || !sym->val) {
    fprintf(stderr, "audio_jit: unresolved compiled synth symbol\n");
    print_ast(sym_id);
    return LLVMConstNull(GENERIC_PTR);
  }

  LLVMValueRef ctor_fn = sym->val;
  printf("call dsp symbol %s\n");
  print_ast(ast);
  LLVMTypeRef ctor_fn_ty = LLVMGlobalGetValueType(ctor_fn);
  return LLVMBuildCall2(builder, ctor_fn_ty, ctor_fn, NULL, 0,
                        "audio_jit.node");
}

static LLVMValueRef SinOscHandler(Ast *ast, JITLangCtx *ctx,
                                  LLVMModuleRef module_ref,
                                  LLVMBuilderRef builder) {

  Ast *sym_id = ast->data.AST_APPLICATION.function;
  printf("sin osc raw node\n");
  print_ast(sym_id);
  print_type(sym_id->type);
  return LLVMConstInt(LLVMInt32Type(), 0, 0);
}

typedef struct {
  // state_ptr
  LLVMValueRef node_ptr;   // !llvm.ptr  — the Node* itself
  LLVMValueRef state_ptr;  // !llvm.ptr  — opaque state block
  LLVMValueRef inputs_ptr; // !llvm.ptr  — Node** inputs array
  LLVMValueRef frame_idx;  // index      — loop induction variable (eg 0 .. 256)
  LLVMValueRef frame_idx_ptr;
  LLVMValueRef perf_fn;
  LLVMBasicBlockRef frame_cond_bb;
  int state_offset;
  LLVMBuilderRef ctor_builder;
  LLVMBuilderRef perform_builder;
} DspBuildCtx;

LLVMValueRef dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder);

static LLVMBasicBlockRef dsp_build_perform_loop(LLVMValueRef perform_fn,
                                                DspBuildCtx *dsp_ctx,
                                                LLVMContextRef llvm_ctx) {
  LLVMBuilderRef builder = dsp_ctx->perform_builder;
  LLVMTypeRef i32_ty = LLVMInt32TypeInContext(llvm_ctx);

  dsp_ctx->node_ptr = LLVMGetParam(perform_fn, 0);
  dsp_ctx->state_ptr = LLVMGetParam(perform_fn, 1);
  dsp_ctx->inputs_ptr = LLVMGetParam(perform_fn, 2);

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

LLVMValueRef builtin_phasor(LLVMValueRef freq, DspBuildCtx *dsp_ctx,
                            JITLangCtx *ctx, LLVMModuleRef module,
                            LLVMBuilderRef builder) {
  int off = dsp_ctx->state_offset;
  dsp_ctx->state_offset += 8;

  LLVMTypeRef i8_ty = LLVMInt8Type();
  LLVMTypeRef i32_ty = LLVMInt32Type();
  LLVMTypeRef f64_ty = LLVMDoubleType();

  // ctor allocation is zeroed, so the initial phase is already 0.0.
  LLVMValueRef off_val = LLVMConstInt(i32_ty, (uint64_t)off, 0);
  LLVMValueRef phase_ptr = LLVMBuildGEP2(builder, i8_ty, dsp_ctx->state_ptr,
                                         &off_val, 1, "phasor.phase_ptr");
  LLVMValueRef phase =
      LLVMBuildLoad2(builder, f64_ty, phase_ptr, "phasor.phase");
  LLVMValueRef spf = LLVMGetParam(dsp_ctx->perf_fn, 4);
  LLVMValueRef step = LLVMBuildFMul(builder, freq, spf, "phasor.step");
  LLVMValueRef advanced =
      LLVMBuildFAdd(builder, phase, step, "phasor.advanced");

  LLVMValueRef zero = LLVMConstReal(f64_ty, 0.0);
  LLVMValueRef one = LLVMConstReal(f64_ty, 1.0);
  LLVMValueRef ovf =
      LLVMBuildFCmp(builder, LLVMRealOGE, advanced, one, "phasor.ovf");
  LLVMValueRef udf =
      LLVMBuildFCmp(builder, LLVMRealOLT, advanced, zero, "phasor.udf");
  LLVMValueRef next =
      LLVMBuildSelect(builder, ovf, zero, advanced, "phasor.wrap_ovf");
  next = LLVMBuildSelect(builder, udf, one, next, "phasor.wrap_udf");

  LLVMBuildStore(builder, next, phase_ptr);
  return phase;
}

LLVMValueRef builtin_sin_osc(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                             LLVMModuleRef module, LLVMBuilderRef builder) {
  printf("builtin sin osc\n");
  print_ast(ast);

  LLVMValueRef freq = dsp_build_expr(ast->data.AST_APPLICATION.args, dsp_ctx,
                                     ctx, module, builder);

  LLVMDumpValue(freq);
  LLVMValueRef phasor = builtin_phasor(freq, dsp_ctx, ctx, module, builder);
  return phasor;
}

LLVMValueRef dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder) {
  switch (ast->tag) {
  case AST_BODY: {
  }
  case AST_IDENTIFIER: {
    return NULL;
  }
  case AST_DOUBLE: {
    return codegen(ast, ctx, module, builder);
  }
  case AST_INT: {
    return codegen(ast, ctx, module, builder);
  }
  case AST_APPLICATION: {
    Ast *f = ast->data.AST_APPLICATION.function;
    if (strcmp(f->data.AST_IDENTIFIER.value, "sin_osc") == 0) {
      return builtin_sin_osc(ast, dsp_ctx, ctx, module, builder);
    }
    return NULL;
  }
  default: {
    return NULL;
  }
  }
}
// LLVMValueRef dsp_build_perform_fn(LLVMValueRef samp_fn, DspBuildCtx *dsp_ctx,
//                                   JITLangCtx *ctx, LLVMModuleRef module_ref,
//                                   LLVMBuilderRef builder) {}
// LLVMValueRef dsp_build_cons_fn(const char *name, Ast *lambda,
//                                LLVMValueRef perform_fn, DspBuildCtx *dsp_ctx,
//                                JITLangCtx *ctx, LLVMModuleRef module_ref,
//                                LLVMBuilderRef builder) {}

LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder) {
  Ast *source = ast->data.AST_APPLICATION.args;
  Ast *lambda = source->data.AST_LET.expr;
  Ast *binding = source->data.AST_LET.binding;
  const char *name = binding->data.AST_IDENTIFIER.value;
  int num_inputs = 0;

  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
    if (p->ast->tag == AST_IDENTIFIER) {
      num_inputs++;
    } else if (p->ast->tag == AST_TUPLE) {
      num_inputs += p->ast->data.AST_LIST.len;
    }
  }

  fprintf(stderr, "compile audio function for %s\n", name);
  // print_ast(lambda);
  // print_type(lambda->type);
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  // Build Synth Perform func scaffold
  LLVMTypeRef perf_ty =
      LLVMFunctionType(LLVMVoidType(),
                       (LLVMTypeRef[]){GENERIC_PTR, GENERIC_PTR, GENERIC_PTR,
                                       LLVMInt32Type(), LLVMDoubleType()},
                       5, 0);

  char perf_name[32];
  sprintf(perf_name, "%s.perform", name);
  LLVMValueRef perf_fn = LLVMAddFunction(module, perf_name, perf_ty);

  // Build synth cons func scaffold
  LLVMTypeRef cons_ty = LLVMFunctionType(GENERIC_PTR, (LLVMTypeRef[]){}, 0, 0);
  char cons_name[32];
  sprintf(cons_name, "%s.cons", name);
  LLVMValueRef cons_fn = LLVMAddFunction(module, cons_name, cons_ty);
  LLVMSetLinkage(cons_fn, LLVMExternalLinkage);
  LLVMBasicBlockRef ctor_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, cons_fn, "entry");

  LLVMBuilderRef ctor_b = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMPositionBuilderAtEnd(ctor_b, ctor_bb);

  LLVMBasicBlockRef perf_bb =
      LLVMAppendBasicBlockInContext(llvm_ctx, perf_fn, "entry");

  DspBuildCtx dsp_ctx = {
      .ctor_builder = ctor_b,
      .perform_builder = LLVMCreateBuilderInContext(llvm_ctx),
      .perf_fn = perf_fn,
  };
  LLVMPositionBuilderAtEnd(dsp_ctx.perform_builder, perf_bb);
  LLVMBasicBlockRef perf_exit_bb =
      dsp_build_perform_loop(perf_fn, &dsp_ctx, llvm_ctx);

  LLVMValueRef expr = dsp_build_expr(lambda->data.AST_LAMBDA.body, &dsp_ctx,
                                     ctx, module, dsp_ctx.perform_builder);

  if (expr) {
    LLVMTypeRef void_ty = LLVMVoidType();
    LLVMTypeRef i64_ty = LLVMInt64Type();
    LLVMTypeRef f64_ty = LLVMDoubleType();
    LLVMTypeRef write_param_tys[] = {GENERIC_PTR, i64_ty, f64_ty};
    LLVMTypeRef write_fn_ty = LLVMFunctionType(void_ty, write_param_tys, 3, 0);
    LLVMValueRef write_fn = LLVMGetNamedFunction(module, "ylc_write_output");
    if (!write_fn) {
      write_fn = LLVMAddFunction(module, "ylc_write_output", write_fn_ty);
      LLVMSetLinkage(write_fn, LLVMExternalLinkage);
    }

    LLVMValueRef frame_i64 = LLVMBuildSExt(
        dsp_ctx.perform_builder, dsp_ctx.frame_idx, i64_ty, "frame_idx.i64");
    LLVMValueRef write_args[] = {dsp_ctx.node_ptr, frame_i64, expr};
    LLVMBuildCall2(dsp_ctx.perform_builder, write_fn_ty, write_fn, write_args,
                   3, "");
  }

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
    LLVMBuildRetVoid(dsp_ctx.perform_builder);
  }

  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(dsp_ctx.ctor_builder))) {
    int state_bytes = (dsp_ctx.state_offset + 7) & ~7;
    LLVMTypeRef i32_ty = LLVMInt32TypeInContext(llvm_ctx);
    LLVMTypeRef create_param_tys[] = {GENERIC_PTR, i32_ty, i32_ty};
    LLVMTypeRef create_fn_ty =
        LLVMFunctionType(GENERIC_PTR, create_param_tys, 3, 0);
    LLVMValueRef create_fn =
        LLVMGetNamedFunction(module, "ylc_create_audio_node");
    if (!create_fn) {
      create_fn =
          LLVMAddFunction(module, "ylc_create_audio_node", create_fn_ty);
      LLVMSetLinkage(create_fn, LLVMExternalLinkage);
    }

    LLVMValueRef create_args[] = {
        perf_fn,
        LLVMConstInt(i32_ty, (uint64_t)num_inputs, 0),
        LLVMConstInt(i32_ty, (uint64_t)state_bytes, 0),
    };
    LLVMValueRef node_val = LLVMBuildCall2(dsp_ctx.ctor_builder, create_fn_ty,
                                           create_fn, create_args, 3, "node");
    LLVMBuildRet(dsp_ctx.ctor_builder, node_val);
  }

  JITSymbol *sym =
      new_symbol((symbol_type)STYPE_AUDIO_JIT_SYM, NULL, cons_fn, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = call_dsp_symbol;
  ht_set_hash(ctx->frame->table, name, hash_string(name, strlen(name)), sym);
  fprintf(stderr, "libaudio_jit: audio fn symbol %s\n", name);

  LLVMDisposeBuilder(dsp_ctx.ctor_builder);
  LLVMDisposeBuilder(dsp_ctx.perform_builder);

  printf("compiled for %s: \n", name);
  LLVMDumpValue(perf_fn);
  printf("\n");

  LLVMDumpValue(cons_fn);
  printf("\n");
  return cons_fn;
}
LLVMValueRef PlayHandler(Ast *ast, JITLangCtx *ctx, LLVMModuleRef module,
                         LLVMBuilderRef builder) {
  LLVMValueRef node =
      codegen(ast->data.AST_APPLICATION.args, ctx, module, builder);
  LLVMTypeRef play_param_tys[] = {GENERIC_PTR};
  LLVMTypeRef play_fn_ty = LLVMFunctionType(GENERIC_PTR, play_param_tys, 1, 0);
  LLVMValueRef play_fn = LLVMGetNamedFunction(module, "play_node");
  if (!play_fn) {
    play_fn = LLVMAddFunction(module, "play_node", play_fn_ty);
    LLVMSetLinkage(play_fn, LLVMExternalLinkage);
  }

  return LLVMBuildCall2(builder, play_fn_ty, play_fn, &node, 1, "play_node");
}

static void register_builtin(ht *stack, const char *name,
                             BuiltinHandler handler) {
  JITSymbol *sym = new_symbol(STYPE_GENERIC_FUNCTION, NULL, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = handler;
  ht_set_hash(stack, name, hash_string(name, strlen(name)), sym);
}

__attribute__((constructor)) static void ylc_audio_jit_init(void) {
  if (!ylc_jit_ctx) {
    fprintf(stderr, "libaudio_jit: no JIT context at load time\n");
    return;
  }

  STYPE_AUDIO_JIT_SYM = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_BUILTIN_HANDLER = REGISTERED_JIT_SYMBOL_TYPE++;
  STYPE_AUDIO_JIT_INLINE_SYM = REGISTERED_JIT_SYMBOL_TYPE++;

  ht *stack = ylc_jit_ctx->frame->table;
  register_builtin(stack, "compile_audio_fn", CompileAudioFnHandler);
  fprintf(stderr, "libaudio_jit: registered compile_audio_fn\n");

  // register_builtin(stack, "sin_osc", SinOscHandler);
  JITSymbol *sym =
      new_symbol((symbol_type)STYPE_AUDIO_JIT_SYM, NULL, NULL, NULL);
  sym->symbol_data.STYPE_GENERIC_FUNCTION.builtin_handler = SinOscHandler;
  ht_set_hash(stack, "sin_osc", hash_string("sin_osc", 7), sym);

  register_builtin(stack, "play", PlayHandler);
}
