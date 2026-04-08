#ifndef AUDIO_JIT_MLIR_SYNTH_RECORD_H
#define AUDIO_JIT_MLIR_SYNTH_RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

// Mirrors audio_jit's SynthRecord but owns MLIR values instead of LLVM ones.
// The void* fields hold mlir::func::FuncOp* (heap-allocated, or nullptr).
//
// ctor_ptr is the compiled machine address populated at runtime.
// Callers must read/write it via __atomic_load/__atomic_store (or C11
// atomic_load/atomic_store with an explicit _Atomic cast) because it is
// written by the JIT thread and read by the audio thread.
typedef struct MlirSynthRecord {
  const char *name;
  void *ctor;        // mlir::func::FuncOp*
  void *init_fn;     // mlir::func::FuncOp*
  void *frame_fn;    // mlir::func::FuncOp*
  void *perform_fn;  // mlir::func::FuncOp*
  void *ctor_ptr;    // compiled machine address — access atomically
  void *frame_ptr;   // compiled machine address — for standalone use
  void *mlir_module; // mlir::OwningOpRef<mlir::ModuleOp>* — preserved for
                     // inlining into dependents
  int state_bytes;
  int num_inputs;
} MlirSynthRecord;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // AUDIO_JIT_MLIR_SYNTH_RECORD_H
