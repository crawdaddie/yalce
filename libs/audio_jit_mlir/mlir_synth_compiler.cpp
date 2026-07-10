#include "mlir_synth_compiler.h"

#undef PI

#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Export.h"

#include "llvm-c/Core.h"
#include "llvm/IR/Module.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdio>
#include <memory>
#include <vector>

using namespace mlir;

namespace ylc::audio_mlir {

static LLVMTypeRef i8_type(LLVMModuleRef module) {
  return LLVMInt8TypeInContext(LLVMGetModuleContext(module));
}

static LLVMTypeRef i32_type(LLVMModuleRef module) {
  return LLVMInt32TypeInContext(LLVMGetModuleContext(module));
}

static LLVMTypeRef i64_type(LLVMModuleRef module) {
  return LLVMInt64TypeInContext(LLVMGetModuleContext(module));
}

static LLVMTypeRef f64_type(LLVMModuleRef module) {
  return LLVMDoubleTypeInContext(LLVMGetModuleContext(module));
}

static LLVMTypeRef void_type(LLVMModuleRef module) {
  return LLVMVoidTypeInContext(LLVMGetModuleContext(module));
}

static LLVMTypeRef ptr_type(LLVMModuleRef module) {
  return LLVMPointerType(i8_type(module), 0);
}

static unsigned count_lambda_inputs(Ast *lambda) {
  if (!lambda || lambda->tag != AST_LAMBDA) {
    return 0;
  }

  unsigned count = 0;
  for (AstList *p = lambda->data.AST_LAMBDA.params; p; p = p->next) {
    Ast *param = p->ast;
    if (!param) {
      continue;
    }
    if (param->tag == AST_IDENTIFIER) {
      count++;
    } else if (param->tag == AST_TUPLE) {
      count += (unsigned)param->data.AST_LIST.len;
    }
  }
  return count;
}

static void dump_mlir_func(ModuleOp mod, const std::string &name) {
  fprintf(stderr, "\n=== audio_jit_mlir MLIR func: %s ===\n", name.c_str());
  if (auto fn = mod.lookupSymbol<LLVM::LLVMFuncOp>(name)) {
    fn.print(llvm::errs());
    llvm::errs() << "\n";
  } else {
    fprintf(stderr, "<missing MLIR function>\n");
  }
  fprintf(stderr, "=== end audio_jit_mlir MLIR func: %s ===\n", name.c_str());
}

static void dump_llvm_value(const char *label, LLVMValueRef value) {
  fprintf(stderr, "\n=== audio_jit_mlir LLVM IR: %s ===\n", label);
  if (value) {
    LLVMDumpValue(value);
    fprintf(stderr, "\n");
  } else {
    fprintf(stderr, "<missing LLVM value>\n");
  }
  fprintf(stderr, "=== end audio_jit_mlir LLVM IR: %s ===\n", label);
}

static bool emit_mlir_init_and_frame(const MlirSynthNames &names,
                                     unsigned arg_count,
                                     LLVMModuleRef target_module,
                                     LLVMValueRef *out_init,
                                     LLVMValueRef *out_frame) {
  llvm::Module *llvm_target = reinterpret_cast<llvm::Module *>(target_module);
  MLIRContext mlir_ctx;
  mlir_ctx.getOrLoadDialect<LLVM::LLVMDialect>();
  registerBuiltinDialectTranslation(mlir_ctx);
  registerLLVMDialectTranslation(mlir_ctx);

  auto loc = UnknownLoc::get(&mlir_ctx);
  OwningOpRef<ModuleOp> module_ref = ModuleOp::create(loc);
  ModuleOp mod = *module_ref;
  OpBuilder b(&mlir_ctx);
  b.setInsertionPointToEnd(mod.getBody());

  mlir::Type mlir_ptr_ty = LLVM::LLVMPointerType::get(&mlir_ctx);
  mlir::Type mlir_f64_ty = b.getF64Type();
  mlir::Type mlir_void_ty = LLVM::LLVMVoidType::get(&mlir_ctx);

  {
    b.setInsertionPointToEnd(mod.getBody());
    auto init_ty =
        LLVM::LLVMFunctionType::get(mlir_void_ty, {mlir_ptr_ty}, false);
    auto init_fn = b.create<LLVM::LLVMFuncOp>(loc, names.init, init_ty);
    Block *entry = init_fn.addEntryBlock(b);
    b.setInsertionPointToStart(entry);
    b.create<LLVM::ReturnOp>(loc, ValueRange{});
  }

  {
    b.setInsertionPointToEnd(mod.getBody());
    std::vector<mlir::Type> frame_params;
    frame_params.push_back(mlir_ptr_ty); // state
    frame_params.push_back(mlir_ptr_ty); // node
    frame_params.push_back(mlir_ptr_ty); // out pointer
    for (unsigned i = 0; i < arg_count; i++) {
      frame_params.push_back(mlir_f64_ty);
    }

    auto frame_ty =
        LLVM::LLVMFunctionType::get(mlir_void_ty, frame_params, false);
    auto frame_fn = b.create<LLVM::LLVMFuncOp>(loc, names.frame, frame_ty);
    Block *entry = frame_fn.addEntryBlock(b);
    b.setInsertionPointToStart(entry);

    // Placeholder DSP body. Replace this block with high-level audio MLIR
    // emission, then lower to LLVM dialect before translation.
    Value out_ptr = frame_fn.getArgument(2);
    Value zero_idx = b.create<LLVM::ConstantOp>(
        loc, b.getI64Type(), b.getI64IntegerAttr(0));
    Value sample_ptr = b.create<LLVM::GEPOp>(
        loc, mlir_ptr_ty, mlir_f64_ty, out_ptr, ValueRange{zero_idx});
    Value zero_sample = b.create<LLVM::ConstantOp>(
        loc, mlir_f64_ty, b.getF64FloatAttr(0.0));
    b.create<LLVM::StoreOp>(loc, zero_sample, sample_ptr);
    b.create<LLVM::ReturnOp>(loc, ValueRange{});
  }

  dump_mlir_func(mod, names.init);
  dump_mlir_func(mod, names.frame);

  auto translated = translateModuleToLLVMIR(mod, llvm_target->getContext());
  if (!translated) {
    fprintf(stderr, "audio_jit_mlir: failed to translate MLIR module\n");
    return false;
  }

  fprintf(stderr, "\n=== audio_jit_mlir translated LLVM module: %s ===\n",
          names.prefix.c_str());
  translated->print(llvm::errs(), nullptr);
  fprintf(stderr, "=== end audio_jit_mlir translated LLVM module: %s ===\n",
          names.prefix.c_str());

  if (llvm::Linker::linkModules(*llvm_target, std::move(translated))) {
    fprintf(stderr, "audio_jit_mlir: failed to link translated MLIR module\n");
    return false;
  }

  *out_init = LLVMGetNamedFunction(target_module, names.init.c_str());
  *out_frame = LLVMGetNamedFunction(target_module, names.frame.c_str());
  return *out_init && *out_frame;
}

static LLVMValueRef declare_or_get(LLVMModuleRef module, const char *name,
                                   LLVMTypeRef fn_ty) {
  LLVMValueRef fn = LLVMGetNamedFunction(module, name);
  if (!fn) {
    fn = LLVMAddFunction(module, name, fn_ty);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
  }
  return fn;
}

static LLVMValueRef build_perform_fn(const MlirSynthNames &names,
                                     unsigned arg_count,
                                     LLVMValueRef frame_fn,
                                     LLVMModuleRef module) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef ptr_ty = ptr_type(module);
  LLVMTypeRef i32_ty = i32_type(module);
  LLVMTypeRef i64_ty = i64_type(module);
  LLVMTypeRef f64_ty = f64_type(module);

  LLVMTypeRef perform_ty = LLVMFunctionType(
      ptr_ty, (LLVMTypeRef[]){ptr_ty, ptr_ty, ptr_ty, i32_ty, f64_ty}, 5, 0);
  LLVMValueRef perform_fn = LLVMAddFunction(module, names.perform.c_str(),
                                            perform_ty);
  LLVMSetLinkage(perform_fn, LLVMExternalLinkage);

  LLVMBuilderRef b = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(llvm_ctx, perform_fn, "entry");
  LLVMBasicBlockRef cond =
      LLVMAppendBasicBlockInContext(llvm_ctx, perform_fn, "cond");
  LLVMBasicBlockRef body =
      LLVMAppendBasicBlockInContext(llvm_ctx, perform_fn, "body");
  LLVMBasicBlockRef exit =
      LLVMAppendBasicBlockInContext(llvm_ctx, perform_fn, "exit");

  LLVMPositionBuilderAtEnd(b, entry);
  LLVMValueRef frame_idx_ptr = LLVMBuildAlloca(b, i32_ty, "frame.idx.ptr");
  LLVMBuildStore(b, LLVMConstInt(i32_ty, 0, 0), frame_idx_ptr);
  LLVMBuildBr(b, cond);

  LLVMPositionBuilderAtEnd(b, cond);
  LLVMValueRef frame_idx = LLVMBuildLoad2(b, i32_ty, frame_idx_ptr, "frame.idx");
  LLVMValueRef nframes = LLVMGetParam(perform_fn, 3);
  LLVMValueRef more = LLVMBuildICmp(b, LLVMIntSLT, frame_idx, nframes, "more");
  LLVMBuildCondBr(b, more, body, exit);

  LLVMPositionBuilderAtEnd(b, body);
  LLVMValueRef node_arg = LLVMGetParam(perform_fn, 0);
  LLVMValueRef state_arg = LLVMGetParam(perform_fn, 1);
  LLVMValueRef inputs_arg = LLVMGetParam(perform_fn, 2);
  LLVMValueRef frame_i64 = LLVMBuildSExt(b, frame_idx, i64_ty, "frame.i64");

  LLVMTypeRef get_buf_ty = LLVMFunctionType(ptr_ty, (LLVMTypeRef[]){ptr_ty}, 1, 0);
  LLVMValueRef get_buf_fn =
      declare_or_get(module, "ylc_audio_mlir_get_output_buf", get_buf_ty);
  LLVMValueRef out_raw =
      LLVMBuildCall2(b, get_buf_ty, get_buf_fn, &node_arg, 1, "out.raw");
  LLVMValueRef out_f64 =
      LLVMBuildPointerCast(b, out_raw, LLVMPointerType(f64_ty, 0), "out.f64");
  LLVMValueRef out_ptr =
      LLVMBuildGEP2(b, f64_ty, out_f64, &frame_i64, 1, "frame.out");

  std::vector<LLVMTypeRef> frame_param_tys;
  frame_param_tys.push_back(ptr_ty);
  frame_param_tys.push_back(ptr_ty);
  frame_param_tys.push_back(ptr_ty);
  for (unsigned i = 0; i < arg_count; i++) {
    frame_param_tys.push_back(f64_ty);
  }
  LLVMTypeRef frame_ty = LLVMFunctionType(
      void_type(module), frame_param_tys.data(), (unsigned)frame_param_tys.size(),
      0);

  std::vector<LLVMValueRef> frame_args;
  frame_args.push_back(state_arg);
  frame_args.push_back(node_arg);
  frame_args.push_back(out_ptr);

  LLVMTypeRef read_ty =
      LLVMFunctionType(f64_ty, (LLVMTypeRef[]){ptr_ty, i64_ty}, 2, 0);
  LLVMValueRef read_fn =
      declare_or_get(module, "ylc_audio_mlir_read_inlet_node", read_ty);
  for (unsigned i = 0; i < arg_count; i++) {
    LLVMValueRef idx_i64 = LLVMConstInt(i64_ty, i, 0);
    LLVMValueRef slot =
        LLVMBuildGEP2(b, ptr_ty, inputs_arg, &idx_i64, 1, "inlet.slot");
    LLVMValueRef inlet_node = LLVMBuildLoad2(b, ptr_ty, slot, "inlet.node");
    LLVMValueRef read_args[] = {inlet_node, frame_i64};
    LLVMValueRef sample =
        LLVMBuildCall2(b, read_ty, read_fn, read_args, 2, "inlet.sample");
    frame_args.push_back(sample);
  }

  LLVMBuildCall2(b, frame_ty, frame_fn, frame_args.data(),
                 (unsigned)frame_args.size(), "");
  LLVMValueRef next =
      LLVMBuildAdd(b, frame_idx, LLVMConstInt(i32_ty, 1, 0), "frame.next");
  LLVMBuildStore(b, next, frame_idx_ptr);
  LLVMBuildBr(b, cond);

  LLVMPositionBuilderAtEnd(b, exit);
  LLVMBuildRet(b, LLVMConstNull(ptr_ty));
  LLVMDisposeBuilder(b);
  return perform_fn;
}

static LLVMValueRef build_constructor_fn(const MlirSynthNames &names,
                                         unsigned arg_count,
                                         int output_lanes, int state_bytes,
                                         LLVMValueRef perform_fn,
                                         LLVMValueRef init_fn,
                                         LLVMModuleRef module) {
  LLVMContextRef llvm_ctx = LLVMGetModuleContext(module);
  LLVMTypeRef ptr_ty = ptr_type(module);
  LLVMTypeRef i32_ty = i32_type(module);
  LLVMTypeRef f64_ty = f64_type(module);
  LLVMTypeRef void_ty = void_type(module);

  std::vector<LLVMTypeRef> cons_param_tys(arg_count, f64_ty);
  LLVMTypeRef cons_ty = LLVMFunctionType(
      ptr_ty, cons_param_tys.data(), (unsigned)cons_param_tys.size(), 0);
  LLVMValueRef cons_fn = LLVMAddFunction(module, names.cons.c_str(), cons_ty);
  LLVMSetLinkage(cons_fn, LLVMExternalLinkage);

  LLVMBuilderRef b = LLVMCreateBuilderInContext(llvm_ctx);
  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(llvm_ctx, cons_fn, "entry");
  LLVMPositionBuilderAtEnd(b, entry);

  LLVMTypeRef create_ty = LLVMFunctionType(
      ptr_ty, (LLVMTypeRef[]){ptr_ty, i32_ty, i32_ty, i32_ty, ptr_ty}, 5, 0);
  LLVMValueRef create_fn =
      declare_or_get(module, "ylc_audio_mlir_create_audio_node", create_ty);
  LLVMValueRef meta = LLVMBuildGlobalStringPtr(
      b, names.public_name.c_str(), "audio.mlir.synth.name");
  LLVMValueRef create_args[] = {
      perform_fn,
      LLVMConstInt(i32_ty, arg_count, 0),
      LLVMConstInt(i32_ty, output_lanes, 0),
      LLVMConstInt(i32_ty, state_bytes, 0),
      meta,
  };
  LLVMValueRef node =
      LLVMBuildCall2(b, create_ty, create_fn, create_args, 5, "node");

  LLVMTypeRef state_ty = LLVMFunctionType(ptr_ty, (LLVMTypeRef[]){ptr_ty}, 1, 0);
  LLVMValueRef state_fn =
      declare_or_get(module, "ylc_audio_mlir_node_state", state_ty);
  LLVMValueRef state =
      LLVMBuildCall2(b, state_ty, state_fn, &node, 1, "state");

  LLVMTypeRef init_ty = LLVMFunctionType(void_ty, (LLVMTypeRef[]){ptr_ty}, 1, 0);
  LLVMBuildCall2(b, init_ty, init_fn, &state, 1, "");

  if (arg_count > 0) {
    LLVMTypeRef const_inlet_ty =
        LLVMFunctionType(ptr_ty, (LLVMTypeRef[]){f64_ty}, 1, 0);
    LLVMValueRef const_inlet_fn =
        declare_or_get(module, "ylc_audio_mlir_const_inlet", const_inlet_ty);
    LLVMTypeRef plug_ty =
        LLVMFunctionType(void_ty, (LLVMTypeRef[]){i32_ty, ptr_ty, ptr_ty}, 3, 0);
    LLVMValueRef plug_fn = declare_or_get(module, "plug_input_in_graph", plug_ty);

    for (unsigned i = 0; i < arg_count; i++) {
      LLVMValueRef param = LLVMGetParam(cons_fn, i);
      LLVMValueRef inlet =
          LLVMBuildCall2(b, const_inlet_ty, const_inlet_fn, &param, 1, "inlet");
      LLVMValueRef plug_args[] = {LLVMConstInt(i32_ty, i, 0), node, inlet};
      LLVMBuildCall2(b, plug_ty, plug_fn, plug_args, 3, "");
    }
  }

  LLVMBuildRet(b, node);
  LLVMDisposeBuilder(b);
  return cons_fn;
}

MlirSynthCompileResult compile_mlir_synth_stub(Ast *lambda,
                                               const MlirSynthNames &names,
                                               JITLangCtx *ctx,
                                               LLVMModuleRef module,
                                               LLVMBuilderRef builder) {
  MlirSynthCompileResult result;
  result.names = names;
  result.arg_count = count_lambda_inputs(lambda);
  result.output_lanes = 1;
  result.state_bytes = 0;

  if (!emit_mlir_init_and_frame(names, result.arg_count, module,
                                &result.init_fn, &result.frame_fn)) {
    return result;
  }

  result.perform_fn =
      build_perform_fn(names, result.arg_count, result.frame_fn, module);
  result.cons_fn = build_constructor_fn(names, result.arg_count,
                                        result.output_lanes, result.state_bytes,
                                        result.perform_fn, result.init_fn,
                                        module);
  result.ok = result.cons_fn && result.init_fn && result.perform_fn &&
              result.frame_fn;
  if (result.ok) {
    dump_llvm_value("init", result.init_fn);
    dump_llvm_value("frame", result.frame_fn);
    dump_llvm_value("perform", result.perform_fn);
    dump_llvm_value("cons", result.cons_fn);
  }
  return result;
}

} // namespace ylc::audio_mlir
