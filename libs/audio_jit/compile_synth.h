#ifndef _LIBS_AUDIO_JIT_COMPILE_SYNTH_H
#define _LIBS_AUDIO_JIT_COMPILE_SYNTH_H

#include "../../engine/common.h"
#include "../../lang/backend_llvm/common.h"
#include "../../lang/common.h"
#include "../../lang/parse.h"
#include <llvm-c/Core.h>
#include <stdint.h>

typedef struct {
  // state_ptr
  LLVMValueRef node_ptr;  // !llvm.ptr  — the Node* itself
  LLVMValueRef state_ptr; // !llvm.ptr  — opaque state block
  LLVMValueRef init_state_ptr;
  LLVMValueRef inputs_ptr; // !llvm.ptr  — Node** inputs array
  LLVMValueRef spf;        // frame seconds value
  LLVMValueRef frame_idx;  // index      — loop induction variable (eg 0 .. 256)
  LLVMValueRef frame_idx_ptr;
  LLVMValueRef perf_fn;
  LLVMValueRef create_call;
  LLVMBasicBlockRef frame_cond_bb;
  int sample_rate;
  double spf_scalar;
  int state_offset;
  LLVMBuilderRef ctor_builder;
  LLVMBuilderRef init_builder;
  LLVMBuilderRef perform_builder;
} DspBuildCtx;
LLVMValueRef CompileAudioFnHandler(Ast *ast, JITLangCtx *ctx,
                                   LLVMModuleRef module,
                                   LLVMBuilderRef builder);

void init_synth_registry();

int audio_sym_synth_id(JITSymbol *sym);

typedef struct SynthRecord {
  const char *name;
  LLVMValueRef ctor;
  LLVMValueRef init_fn;
  LLVMValueRef frame_fn;
  LLVMValueRef perform_fn;
  int state_bytes;
} SynthRecord;

typedef struct SynthRegistry {
  int length;
  int capacity;
  SynthRecord *records;
} SynthRegistry;

SynthRecord synth_registry_get(int synth_id);
int synth_registry_len();

LLVMValueRef dsp_build_expr(Ast *ast, DspBuildCtx *dsp_ctx, JITLangCtx *ctx,
                            LLVMModuleRef module, LLVMBuilderRef builder);

SynthRecord compile_lambda_to_synth_record(Ast *lambda, const char *name,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,

                                           LLVMBuilderRef builder);

void print_synth_record(SynthRecord rec);
#endif
