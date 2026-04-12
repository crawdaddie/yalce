#ifndef _LIBS_AUDIO_JIT_COMPILE_SYNTH_H
#define _LIBS_AUDIO_JIT_COMPILE_SYNTH_H

#include "../../engine/common.h"
#include "../../lang/backend_llvm/common.h"
#include "../../lang/common.h"
#include "../../lang/parse.h"
#include <llvm-c/Core.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdint.h>

typedef struct {
  int32_t comptime_size;
} DspArrayAttributes;

typedef struct {
  size_t total_bytes;
  size_t current_bytes;
  unsigned char *bytes;
} DspTmpAllocator;

typedef struct {
  // state_ptr
  LLVMValueRef node_ptr;  // !llvm.ptr  — the Node* itself
  LLVMValueRef state_ptr; // !llvm.ptr  — opaque state block
  LLVMValueRef init_state_ptr;
  LLVMValueRef state_base_ptr;
  LLVMValueRef init_state_base_ptr;
  LLVMValueRef state_cursor_ptr;      // alloca(i8*)
  LLVMValueRef init_state_cursor_ptr; // alloca(i8*)
  LLVMValueRef inputs_ptr;            // !llvm.ptr  — Node** inputs array
  LLVMValueRef spf;                   // frame seconds value
  LLVMValueRef frame_idx; // index      — loop induction variable (eg 0 .. 256)
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
  DspArrayAttributes array_attrs;
  DspTmpAllocator *tmp_alloc;
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
  _Atomic(void *) ctor_ptr; // compiled machine address, populated at runtime
  int output_lanes;
  int state_bytes;
} SynthRecord;

typedef struct SynthRegistry {
  int length;
  int capacity;
  SynthRecord *records;
} SynthRegistry;

SynthRecord synth_registry_get(int synth_id);
void *synth_registry_get_ctor_ptr(int synth_id);
void synth_registry_set_ctor_ptr(int synth_id, void *ctor_ptr);
int synth_registry_len();

SynthRecord compile_lambda_to_synth_record(Ast *lambda, const char *name,
                                           LLVMTypeRef frame_ty,
                                           JITLangCtx *ctx,
                                           LLVMModuleRef module,

                                           LLVMBuilderRef builder);

void print_synth_record(SynthRecord rec);

void *dsp_tmp_alloc(DspBuildCtx *dsp_ctx, size_t size, size_t align);
void dsp_tmp_allocator_init(DspTmpAllocator *alloc, size_t initial_bytes);
void dsp_tmp_allocator_free(DspTmpAllocator *alloc);
#endif
