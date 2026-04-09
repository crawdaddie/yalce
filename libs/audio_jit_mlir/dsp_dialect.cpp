#include "./dsp_dialect.hpp"

#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/IR/Builders.h>
#include <mlir/IR/BuiltinAttributes.h>
#include <mlir/IR/DialectImplementation.h>
#include <mlir/IR/Matchers.h>
#include <mlir/Target/LLVMIR/LLVMTranslationInterface.h>
#include <mlir/Target/LLVMIR/ModuleTranslation.h>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/TypeSwitch.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>

#define GET_OP_CLASSES
#include "generated/DspOps.cpp.inc"

#ifndef SIN_TABSIZE
#define SIN_TABSIZE (1 << 11)
#endif

#ifndef SQ_TABSIZE
#define SQ_TABSIZE (1 << 11)
#endif

#ifndef SAW_TABSIZE
#define SAW_TABSIZE (1 << 11)
#endif

namespace {

static llvm::Value *getOrCreateStaticTablePtr(llvm::Module &module,
                                              llvm::IRBuilderBase &builder,
                                              llvm::StringRef symbol,
                                              int32_t table_size) {
  int32_t expected_size = 0;
  if (symbol == "ylc_sin_table")
    expected_size = SIN_TABSIZE;
  else if (symbol == "ylc_sq_table")
    expected_size = SQ_TABSIZE;
  else if (symbol == "ylc_saw_table")
    expected_size = SAW_TABSIZE;
  else
    return nullptr;

  if (expected_size != table_size)
    return nullptr;

  llvm::Type *f64_ty = builder.getDoubleTy();
  auto *arr_ty = llvm::ArrayType::get(f64_ty, table_size);
  llvm::GlobalVariable *global = module.getNamedGlobal(symbol);
  if (!global) {
    global = new llvm::GlobalVariable(module, arr_ty, /*isConstant=*/true,
                                      llvm::GlobalValue::ExternalLinkage,
                                      /*Initializer=*/nullptr, symbol);
    global->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
    global->setAlignment(llvm::Align(sizeof(double)));
  }

  return builder.CreateConstGEP2_32(arr_ty, global, 0, 0, "tab.base");
}

static llvm::Value *buildPow2Tabread(llvm::Value *phase, llvm::Value *table_ptr,
                                     int32_t table_size,
                                     llvm::IRBuilderBase &builder) {
  llvm::Type *f64_ty = builder.getDoubleTy();
  llvm::Type *i32_ty = builder.getInt32Ty();
  llvm::Type *i64_ty = builder.getInt64Ty();

  llvm::Value *scaled = builder.CreateFMul(
      phase, llvm::ConstantFP::get(f64_ty, (double)table_size),
      "tab.scaled_idx");
  llvm::Value *index = builder.CreateFPToSI(scaled, i32_ty, "tab.index");
  llvm::Value *index_f = builder.CreateSIToFP(index, f64_ty, "tab.index_f");
  llvm::Value *frac = builder.CreateFSub(scaled, index_f, "tab.frac");

  llvm::Value *mask = llvm::ConstantInt::get(i32_ty, table_size - 1);
  llvm::Value *one_i32 = llvm::ConstantInt::get(i32_ty, 1);
  llvm::Value *idx0 = builder.CreateAnd(index, mask, "tab.idx0");
  llvm::Value *idx1 = builder.CreateAnd(
      builder.CreateAdd(index, one_i32, "tab.idx1_raw"), mask, "tab.idx1");

  llvm::Value *idx0_i64 = builder.CreateZExt(idx0, i64_ty, "tab.idx0.i64");
  llvm::Value *idx1_i64 = builder.CreateZExt(idx1, i64_ty, "tab.idx1.i64");

  llvm::Value *a_ptr =
      builder.CreateGEP(f64_ty, table_ptr, idx0_i64, "tab.a.ptr");
  llvm::Value *b_ptr =
      builder.CreateGEP(f64_ty, table_ptr, idx1_i64, "tab.b.ptr");
  llvm::Value *a = builder.CreateLoad(f64_ty, a_ptr, "tab.a");
  llvm::Value *b = builder.CreateLoad(f64_ty, b_ptr, "tab.b");

  llvm::Value *inv_frac = builder.CreateFSub(llvm::ConstantFP::get(f64_ty, 1.0),
                                             frac, "tab.inv_frac");
  llvm::Value *a_term = builder.CreateFMul(inv_frac, a, "tab.a_term");
  llvm::Value *b_term = builder.CreateFMul(frac, b, "tab.b_term");
  return builder.CreateFAdd(a_term, b_term, "tab.sample");
}

static llvm::Value *buildTabread(llvm::Value *phase, llvm::Value *table_ptr,
                                 int32_t table_size,
                                 llvm::IRBuilderBase &builder) {
  llvm::Type *f64_ty = builder.getDoubleTy();
  llvm::Type *i64_ty = builder.getInt64Ty();
  llvm::Module *module = builder.GetInsertBlock()->getModule();

  llvm::Value *scaled = builder.CreateFMul(
      phase, llvm::ConstantFP::get(f64_ty, (double)table_size),
      "tab.scaled_idx");

  llvm::Function *floor_fn = llvm::Intrinsic::getOrInsertDeclaration(
      module, llvm::Intrinsic::floor, {f64_ty});
  llvm::Value *i0_f = builder.CreateCall(floor_fn, {scaled}, "tab.i0f");
  llvm::Value *frac = builder.CreateFSub(scaled, i0_f, "tab.frac");

  llvm::Value *i0 = builder.CreateFPToSI(i0_f, i64_ty, "tab.i0");
  llvm::Value *i1_raw =
      builder.CreateAdd(i0, llvm::ConstantInt::get(i64_ty, 1), "tab.i1_raw");

  llvm::Value *len_i64 = llvm::ConstantInt::get(i64_ty, table_size);
  llvm::Value *i1_ge_len =
      builder.CreateICmpSGE(i1_raw, len_i64, "tab.i1_ge_len");
  llvm::Value *i1 = builder.CreateSelect(
      i1_ge_len, llvm::ConstantInt::get(i64_ty, 0), i1_raw, "tab.i1");

  llvm::Value *a_ptr = builder.CreateGEP(f64_ty, table_ptr, i0, "tab.a.ptr");
  llvm::Value *b_ptr = builder.CreateGEP(f64_ty, table_ptr, i1, "tab.b.ptr");
  llvm::Value *a = builder.CreateLoad(f64_ty, a_ptr, "tab.a");
  llvm::Value *b = builder.CreateLoad(f64_ty, b_ptr, "tab.b");

  llvm::Value *inv_frac = builder.CreateFSub(llvm::ConstantFP::get(f64_ty, 1.0),
                                             frac, "tab.inv_frac");
  llvm::Value *a_term = builder.CreateFMul(inv_frac, a, "tab.a_term");
  llvm::Value *b_term = builder.CreateFMul(frac, b, "tab.b_term");
  return builder.CreateFAdd(a_term, b_term, "tab.sample");
}

static bool isPowerOfTwo(int32_t n) { return n > 0 && (n & (n - 1)) == 0; }

} // namespace

namespace dsp {

// ---------------------------------------------------------------------------
// Semantic array ops
// ---------------------------------------------------------------------------

namespace detail {
struct ArrayTypeStorage : public mlir::TypeStorage {
  using KeyTy = mlir::Type;

  explicit ArrayTypeStorage(mlir::Type elementType)
      : elementType(elementType) {}

  bool operator==(const KeyTy &key) const { return key == elementType; }
  static llvm::hash_code hashKey(const KeyTy &key) {
    return llvm::hash_combine(key);
  }
  static ArrayTypeStorage *construct(mlir::TypeStorageAllocator &allocator,
                                     const KeyTy &key) {
    return new (allocator.allocate<ArrayTypeStorage>()) ArrayTypeStorage(key);
  }

  mlir::Type elementType;
};
} // namespace detail

ArrayType ArrayType::get(mlir::MLIRContext *ctx, mlir::Type elementType) {
  return Base::get(ctx, elementType);
}

ArrayType ArrayType::get(mlir::Type elementType) {
  return Base::get(elementType.getContext(), elementType);
}

mlir::Type ArrayType::parse(mlir::AsmParser &parser) {
  if (parser.parseLess())
    return {};
  mlir::Type elementType;
  if (parser.parseType(elementType) || parser.parseGreater())
    return {};
  return get(parser.getContext(), elementType);
}

void ArrayType::print(mlir::AsmPrinter &printer) const {
  printer << "<";
  printer.printType(getElementType());
  printer << ">";
}

mlir::Type ArrayType::getElementType() const { return getImpl()->elementType; }

void dsp::ArrayOp::build(mlir::OpBuilder &b, mlir::OperationState &state,
                         mlir::Type elem_type, mlir::ValueRange values) {
  state.addOperands(values);
  state.addAttribute("elem_type", mlir::TypeAttr::get(elem_type));
  state.addTypes(dsp::ArrayType::get(b.getContext(), elem_type));
}

void dsp::ArrayStatefulOp::build(mlir::OpBuilder &b,
                                 mlir::OperationState &state,
                                 mlir::Type elem_type,
                                 mlir::ValueRange values) {
  state.addOperands(values);
  state.addAttribute("elem_type", mlir::TypeAttr::get(elem_type));
  state.addTypes(dsp::ArrayType::get(b.getContext(), elem_type));
}

void dsp::ArrayAtOp::build(mlir::OpBuilder &b, mlir::OperationState &state,
                           mlir::Value array, mlir::Value index) {
  auto arrType = llvm::cast<dsp::ArrayType>(array.getType());
  state.addOperands({array, index});
  state.addTypes(arrType.getElementType());
}

void dsp::ArraySetOp::build(mlir::OpBuilder &b, mlir::OperationState &state,
                            mlir::Value array, mlir::Value index,
                            mlir::Value value) {
  state.addOperands({array, index, value});
  state.addTypes(array.getType());
}

static llvm::Type *lower_array_elem_type(mlir::Type ty,
                                         llvm::LLVMContext &ctx) {
  if (ty.isF64())
    return llvm::Type::getDoubleTy(ctx);
  if (ty.isF32())
    return llvm::Type::getFloatTy(ctx);
  if (ty.isInteger(1))
    return llvm::Type::getInt1Ty(ctx);
  if (ty.isInteger(8))
    return llvm::Type::getInt8Ty(ctx);
  if (ty.isInteger(16))
    return llvm::Type::getInt16Ty(ctx);
  if (ty.isInteger(32))
    return llvm::Type::getInt32Ty(ctx);
  if (ty.isInteger(64))
    return llvm::Type::getInt64Ty(ctx);
  if (llvm::isa<mlir::LLVM::LLVMPointerType>(ty))
    return llvm::PointerType::get(ctx, 0);
  return nullptr;
}

int64_t getArrayElemSize(mlir::Type ty) {
  if (ty.isF64() || ty.isInteger(64) ||
      llvm::isa<mlir::LLVM::LLVMPointerType>(ty))
    return 8;
  if (ty.isF32() || ty.isInteger(32))
    return 4;
  if (ty.isInteger(16))
    return 2;
  if (ty.isInteger(8) || ty.isInteger(1))
    return 1;
  return 0;
}

int32_t getArrayElemAlign(mlir::Type ty) {
  return static_cast<int32_t>(getArrayElemSize(ty));
}

// ---------------------------------------------------------------------------
// StateCursorOp / StateConsumeOp
// ---------------------------------------------------------------------------

void dsp::StateConsumeOp::build(mlir::OpBuilder &b, mlir::OperationState &state,
                                mlir::Value cursor, int32_t size,
                                int32_t align) {
  auto ptr_ty = mlir::LLVM::LLVMPointerType::get(b.getContext());
  state.addOperands({cursor});
  state.addTypes({ptr_ty, ptr_ty});
  state.addAttribute("size", b.getIntegerAttr(b.getIntegerType(32), size));
  state.addAttribute("align", b.getIntegerAttr(b.getIntegerType(32), align));
}

namespace {

mlir::LogicalResult lower_array_op(mlir::Operation *op,
                                   llvm::IRBuilderBase &builder,
                                   mlir::LLVM::ModuleTranslation &mt) {
  auto array_op = mlir::cast<dsp::ArrayOp>(op);
  auto arr_type = llvm::cast<dsp::ArrayType>(array_op.getResult().getType());
  mlir::Type elem_ty = arr_type.getElementType();
  llvm::Type *llvm_elem_ty =
      lower_array_elem_type(elem_ty, builder.getContext());
  if (!llvm_elem_ty) {
    op->emitError("dsp.array: unsupported element type for LLVM lowering");
    return mlir::failure();
  }

  auto *array_size =
      llvm::ConstantInt::get(builder.getInt64Ty(), array_op.getValues().size());
  llvm::Value *base_ptr =
      builder.CreateAlloca(llvm_elem_ty, array_size, "array.base");

  for (auto [i, value] : llvm::enumerate(array_op.getValues())) {
    llvm::Value *elem_ptr = builder.CreateGEP(
        llvm_elem_ty, base_ptr, llvm::ConstantInt::get(builder.getInt64Ty(), i),
        "array.elem.ptr");
    builder.CreateStore(mt.lookupValue(value), elem_ptr);
  }

  mt.mapValue(array_op.getResult(), base_ptr);
  return mlir::success();
}

mlir::LogicalResult lower_array_stateful_op(mlir::Operation *op,
                                            llvm::IRBuilderBase &builder,
                                            mlir::LLVM::ModuleTranslation &mt) {
  auto array_op = mlir::cast<dsp::ArrayStatefulOp>(op);
  auto byte_offset_attr = op->getAttrOfType<mlir::IntegerAttr>("byte_offset");
  if (!byte_offset_attr) {
    op->emitError("dsp.array_state missing byte_offset for LLVM lowering");
    return mlir::failure();
  }

  auto fn =
      mlir::cast<mlir::LLVM::LLVMFuncOp>(op->getParentRegion()->getParentOp());
  llvm::Value *state_ptr = mt.lookupValue(fn.getArgument(0));
  llvm::Value *array_ptr = builder.CreateConstGEP1_64(
      builder.getInt8Ty(), state_ptr,
      static_cast<uint64_t>(byte_offset_attr.getInt()), "array.state");
  mt.mapValue(array_op.getResult(), array_ptr);
  return mlir::success();
}

mlir::LogicalResult lower_array_at_op(mlir::Operation *op,
                                      llvm::IRBuilderBase &builder,
                                      mlir::LLVM::ModuleTranslation &mt) {
  auto get_op = mlir::cast<dsp::ArrayAtOp>(op);
  auto arr_type = llvm::cast<dsp::ArrayType>(get_op.getArray().getType());
  llvm::Type *llvm_elem_ty =
      lower_array_elem_type(arr_type.getElementType(), builder.getContext());
  if (!llvm_elem_ty) {
    op->emitError("dsp.array_get: unsupported element type for LLVM lowering");
    return mlir::failure();
  }

  llvm::Value *base_ptr = mt.lookupValue(get_op.getArray());
  llvm::Value *index = mt.lookupValue(get_op.getIndex());
  if (index->getType() != builder.getInt64Ty())
    index = builder.CreateSExtOrTrunc(index, builder.getInt64Ty(), "array.idx");
  llvm::Value *elem_ptr =
      builder.CreateGEP(llvm_elem_ty, base_ptr, index, "array.get.ptr");
  llvm::Value *elem = builder.CreateLoad(llvm_elem_ty, elem_ptr, "array.get");
  mt.mapValue(get_op.getResult(), elem);
  return mlir::success();
}

mlir::LogicalResult lower_array_set_op(mlir::Operation *op,
                                       llvm::IRBuilderBase &builder,
                                       mlir::LLVM::ModuleTranslation &mt) {
  auto set_op = mlir::cast<dsp::ArraySetOp>(op);
  auto arr_type = llvm::cast<dsp::ArrayType>(set_op.getArray().getType());
  llvm::Type *llvm_elem_ty =
      lower_array_elem_type(arr_type.getElementType(), builder.getContext());
  if (!llvm_elem_ty) {
    op->emitError("dsp.array_set: unsupported element type for LLVM lowering");
    return mlir::failure();
  }

  llvm::Value *base_ptr = mt.lookupValue(set_op.getArray());
  llvm::Value *index = mt.lookupValue(set_op.getIndex());
  if (index->getType() != builder.getInt64Ty())
    index = builder.CreateSExtOrTrunc(index, builder.getInt64Ty(), "array.idx");
  llvm::Value *elem_ptr =
      builder.CreateGEP(llvm_elem_ty, base_ptr, index, "array.set.ptr");
  builder.CreateStore(mt.lookupValue(set_op.getValue()), elem_ptr);
  mt.mapValue(set_op.getResult(), base_ptr);
  return mlir::success();
}

mlir::LogicalResult lower_state_cursor_op(mlir::Operation *op,
                                          llvm::IRBuilderBase &builder,
                                          mlir::LLVM::ModuleTranslation &mt) {
  auto cursor_op = mlir::cast<dsp::StateCursorOp>(op);
  mt.mapValue(cursor_op.getCursor(), mt.lookupValue(cursor_op.getBase()));
  return mlir::success();
}

mlir::LogicalResult lower_state_consume_op(mlir::Operation *op,
                                           llvm::IRBuilderBase &builder,
                                           mlir::LLVM::ModuleTranslation &mt) {
  auto consume = mlir::cast<dsp::StateConsumeOp>(op);
  llvm::Value *cursor = mt.lookupValue(consume.getCursor());
  llvm::Type *i8_ptr_ty = builder.getPtrTy();
  llvm::Type *i64_ty = builder.getInt64Ty();

  int32_t align = static_cast<int32_t>(consume.getAlign());
  int32_t size = static_cast<int32_t>(consume.getSize());

  llvm::Value *cursor_i64 =
      builder.CreatePtrToInt(cursor, i64_ty, "state.cursor_i64");
  llvm::Value *align_mask =
      llvm::ConstantInt::get(i64_ty, static_cast<uint64_t>(align - 1));
  llvm::Value *biased =
      builder.CreateAdd(cursor_i64, align_mask, "state.align_biased");
  llvm::Value *aligned_i64 = builder.CreateAnd(
      biased, llvm::ConstantInt::get(i64_ty, ~static_cast<uint64_t>(align - 1)),
      "state.aligned_i64");
  llvm::Value *aligned =
      builder.CreateIntToPtr(aligned_i64, i8_ptr_ty, "state.aligned");
  llvm::Value *next =
      builder.CreateGEP(builder.getInt8Ty(), aligned,
                        llvm::ConstantInt::get(i64_ty, size), "state.next");

  mt.mapValue(consume.getPtr(), aligned);
  mt.mapValue(consume.getNextCursor(), next);
  return mlir::success();
}

} // namespace

// ---------------------------------------------------------------------------
// StateSlotOp
// ---------------------------------------------------------------------------

void dsp::StateSlotOp::build(mlir::OpBuilder &b, mlir::OperationState &state,
                             mlir::Type elem_type, double init_val) {
  state.addTypes(mlir::LLVM::LLVMPointerType::get(b.getContext()));
  state.addAttribute("elem_type", mlir::TypeAttr::get(elem_type));
  state.addAttribute("init_val", b.getF64FloatAttr(init_val));
  state.addAttribute("byte_offset", b.getIntegerAttr(b.getIntegerType(64), -1));
}

void dsp::SubSynthOp::build(mlir::OpBuilder &b, mlir::OperationState &state,
                            llvm::StringRef synth_name, int32_t state_bytes,
                            int32_t num_inputs, mlir::ValueRange inputs) {
  state.addOperands(inputs);
  state.addTypes(b.getF64Type());
  state.addAttribute("synth_name", b.getStringAttr(synth_name));
  state.addAttribute("state_bytes", b.getI32IntegerAttr(state_bytes));
  state.addAttribute("num_inputs", b.getI32IntegerAttr(num_inputs));
}

namespace {

// Lowering lives with the op so definition and translation stay in sync.
mlir::LogicalResult lower_state_slot_op(mlir::Operation *op,
                                        llvm::IRBuilderBase &builder,
                                        mlir::LLVM::ModuleTranslation &mt) {
  auto slot = mlir::cast<dsp::StateSlotOp>(op);
  int64_t byte_offset = slot.getByteOffset();
  if (byte_offset < 0) {
    op->emitError("dsp.state_slot: no byte_offset — call "
                  "assign_dsp_state_offsets() before translation");
    return mlir::failure();
  }

  auto fn =
      mlir::cast<mlir::LLVM::LLVMFuncOp>(op->getParentRegion()->getParentOp());
  llvm::Value *state_ptr = mt.lookupValue(fn.getArgument(0));
  llvm::Value *slot_ptr = builder.CreateConstGEP1_64(
      builder.getInt8Ty(), state_ptr, (uint64_t)byte_offset, "state.slot");
  mt.mapValue(slot->getResult(0), slot_ptr);
  return mlir::success();
}

} // namespace
//
// ---------------------------------------------------------------------------
// PhasorStatefulOp
// ---------------------------------------------------------------------------

void dsp::PhasorOp::build(mlir::OpBuilder &b, mlir::OperationState &state,
                          mlir::Value freq, double spf) {
  state.addOperands({freq});
  state.addTypes(b.getF64Type());
  state.addAttribute("spf", b.getF64FloatAttr(spf));
}

void dsp::PhasorStatefulOp::build(mlir::OpBuilder &b,
                                  mlir::OperationState &state,
                                  mlir::Value phase_slot, mlir::Value freq,
                                  double spf) {
  state.addOperands({phase_slot, freq});
  state.addTypes(b.getF64Type());
  state.addAttribute("spf", b.getF64FloatAttr(spf));
}

void dsp::PhasorSyncOp::build(mlir::OpBuilder &b, mlir::OperationState &state,
                              mlir::Value freq, mlir::Value trig, double spf) {
  state.addOperands({freq, trig});
  state.addTypes(b.getF64Type());
  state.addAttribute("spf", b.getF64FloatAttr(spf));
}

void dsp::PhasorSyncStatefulOp::build(mlir::OpBuilder &b,
                                      mlir::OperationState &state,
                                      mlir::Value phase_slot, mlir::Value freq,
                                      mlir::Value trig, double spf) {
  state.addOperands({phase_slot, freq, trig});
  state.addTypes(b.getF64Type());
  state.addAttribute("spf", b.getF64FloatAttr(spf));
}

namespace {

// Lowering lives with the op so definition and translation stay in sync.

mlir::LogicalResult
lower_phasor_stateful_op(mlir::Operation *op, llvm::IRBuilderBase &builder,
                         mlir::LLVM::ModuleTranslation &mt) {
  auto phasor = mlir::cast<dsp::PhasorStatefulOp>(op);

  llvm::Type *f64_ty = builder.getDoubleTy();
  llvm::Type *ptr_ty = builder.getPtrTy();

  llvm::Value *phase_ptr = mt.lookupValue(phasor.getPhaseSlot());
  phase_ptr = builder.CreateBitCast(phase_ptr, ptr_ty, "phasor.sync_phase_ptr");

  llvm::Value *phase =
      builder.CreateLoad(f64_ty, phase_ptr, "phasor.sync_phase");

  double spf_attr = phasor.getSpf().convertToDouble();
  llvm::Value *spf = llvm::ConstantFP::get(f64_ty, spf_attr);
  llvm::Value *freq = mt.lookupValue(phasor.getFreq());

  llvm::Value *step = builder.CreateFMul(freq, spf, "phasor.step");
  llvm::Value *advanced = builder.CreateFAdd(phase, step, "phasor.advanced");

  llvm::Value *zero = llvm::ConstantFP::get(f64_ty, 0.0);
  llvm::Value *one = llvm::ConstantFP::get(f64_ty, 1.0);

  llvm::Value *ovf = builder.CreateFCmpOGE(advanced, one, "phasor.ovf");
  llvm::Value *udf = builder.CreateFCmpOLT(advanced, zero, "phasor.udf");

  llvm::Value *wrappedOvf =
      builder.CreateFSub(advanced, one, "phasor.wrap_ovf_val");
  llvm::Value *next =
      builder.CreateSelect(ovf, wrappedOvf, advanced, "phasor.wrap_ovf");

  llvm::Value *wrappedUdf =
      builder.CreateFAdd(next, one, "phasor.wrap_udf_val");
  next = builder.CreateSelect(udf, wrappedUdf, next, "phasor.wrap_udf");

  builder.CreateStore(next, phase_ptr);
  mt.mapValue(phasor.getResult(), phase);
  return mlir::success();
}

mlir::LogicalResult
lower_phasor_sync_stateful_op(mlir::Operation *op, llvm::IRBuilderBase &builder,
                              mlir::LLVM::ModuleTranslation &mt) {
  auto phasor = mlir::cast<dsp::PhasorSyncStatefulOp>(op);

  llvm::Type *f64_ty = builder.getDoubleTy();
  llvm::Type *ptr_ty = builder.getPtrTy();

  llvm::Value *phase_ptr = mt.lookupValue(phasor.getPhaseSlot());
  phase_ptr = builder.CreateBitCast(phase_ptr, ptr_ty, "phasor.phase_ptr");

  llvm::Value *phase = builder.CreateLoad(f64_ty, phase_ptr, "phasor.phase");

  double spf_attr = phasor.getSpf().convertToDouble();
  llvm::Value *spf = llvm::ConstantFP::get(f64_ty, spf_attr);
  llvm::Value *freq = mt.lookupValue(phasor.getFreq());
  llvm::Value *trig = mt.lookupValue(phasor.getTrig());
  llvm::Value *zero = llvm::ConstantFP::get(f64_ty, 0.0);
  llvm::Value *one = llvm::ConstantFP::get(f64_ty, 1.0);
  assert(trig && "phasor_sync trig was not translated");

  trig = builder.CreateFAdd(trig, zero, "phasor.sync_trig");

  llvm::Value *reset = builder.CreateFCmpOGE(trig, one, "phasor.sync_reset");
  llvm::Value *base_phase =
      builder.CreateSelect(reset, zero, phase, "phasor.sync_base_phase");

  llvm::Value *step = builder.CreateFMul(freq, spf, "phasor.sync_step");
  llvm::Value *advanced =
      builder.CreateFAdd(base_phase, step, "phasor.sync_advanced");

  llvm::Value *ovf = builder.CreateFCmpOGE(advanced, one, "phasor.sync_ovf");
  llvm::Value *udf = builder.CreateFCmpOLT(advanced, zero, "phasor.sync_udf");

  llvm::Value *wrappedOvf =
      builder.CreateFSub(advanced, one, "phasor.sync_wrap_ovf_val");
  llvm::Value *next =
      builder.CreateSelect(ovf, wrappedOvf, advanced, "phasor.sync_wrap_ovf");

  llvm::Value *wrappedUdf =
      builder.CreateFAdd(next, one, "phasor.sync_wrap_udf_val");
  next = builder.CreateSelect(udf, wrappedUdf, next, "phasor.sync_wrap_udf");

  builder.CreateStore(next, phase_ptr);
  mt.mapValue(phasor.getResult(), base_phase);
  return mlir::success();
}

mlir::LogicalResult lower_phasor_op(mlir::Operation *op,
                                    llvm::IRBuilderBase &builder,
                                    mlir::LLVM::ModuleTranslation &mt) {
  op->emitError(
      "dsp.phasor reached LLVM lowering before state materialization; "
      "rewrite it to dsp.phasor_stateful first");
  return mlir::failure();
}

mlir::LogicalResult lower_phasor_sync_op(mlir::Operation *op,
                                         llvm::IRBuilderBase &builder,
                                         mlir::LLVM::ModuleTranslation &mt) {
  op->emitError(
      "dsp.phasor_sync reached LLVM lowering before state materialization; "
      "rewrite it to dsp.phasor_sync_stateful first");
  return mlir::failure();
}

} // namespace
//
void dsp::TabreadOp::build(mlir::OpBuilder &b, mlir::OperationState &state,
                           mlir::Value phase, mlir::Value table_ptr,
                           int32_t table_size) {
  state.addOperands({phase, table_ptr});
  state.addTypes(b.getF64Type());
  state.addAttribute("table_size",
                     b.getIntegerAttr(b.getIntegerType(32), table_size));
}

namespace {

mlir::LogicalResult lower_tabread_op(mlir::Operation *op,
                                     llvm::IRBuilderBase &builder,
                                     mlir::LLVM::ModuleTranslation &mt) {
  auto tabread_op = mlir::cast<dsp::TabreadOp>(op);
  int32_t table_size = static_cast<int32_t>(tabread_op.getTableSize());
  llvm::Value *phase = mt.lookupValue(tabread_op.getPhase());
  llvm::Value *table_ptr = mt.lookupValue(tabread_op.getTablePtr());
  llvm::Value *sample =
      isPowerOfTwo(table_size)
          ? buildPow2Tabread(phase, table_ptr, table_size, builder)
          : buildTabread(phase, table_ptr, table_size, builder);
  mt.mapValue(tabread_op.getResult(), sample);
  return mlir::success();
}

} // namespace

//
void dsp::Tabread2Op::build(mlir::OpBuilder &b, mlir::OperationState &state,
                            mlir::Value phase, llvm::StringRef table_symbol,
                            int32_t table_size) {
  state.addOperands({phase});
  state.addTypes(b.getF64Type());
  state.addAttribute("table_symbol", b.getStringAttr(table_symbol));
  state.addAttribute("table_size",
                     b.getIntegerAttr(b.getIntegerType(32), table_size));
}

namespace {

// Lowering lives with the op so definition and translation stay in sync.
mlir::LogicalResult lower_tabread2_op(mlir::Operation *op,
                                      llvm::IRBuilderBase &builder,
                                      mlir::LLVM::ModuleTranslation &mt) {
  auto tabread_op = mlir::cast<dsp::Tabread2Op>(op);
  auto table_symbol =
      op->getAttrOfType<mlir::StringAttr>("table_symbol").getValue();
  int32_t table_size =
      (int32_t)op->getAttrOfType<mlir::IntegerAttr>("table_size").getInt();

  auto *module = builder.GetInsertBlock()->getModule();
  llvm::Value *table_ptr =
      getOrCreateStaticTablePtr(*module, builder, table_symbol, table_size);
  if (!table_ptr) {
    op->emitError("dsp.tabread2: unknown static table '")
        << table_symbol << "' with size " << table_size;
    return mlir::failure();
  }

  llvm::Value *phase = mt.lookupValue(tabread_op->getOperand(0));
  llvm::Value *sample =
      isPowerOfTwo(table_size)
          ? buildPow2Tabread(phase, table_ptr, table_size, builder)
          : buildTabread(phase, table_ptr, table_size, builder);
  mt.mapValue(tabread_op->getResult(0), sample);
  return mlir::success();
}

} // namespace

// ---------------------------------------------------------------------------
// Pre-pass: walk all dsp.state_slot ops in program order and assign offsets.
// Returns total state bytes needed.
// ---------------------------------------------------------------------------

int64_t assign_dsp_state_offsets(mlir::func::FuncOp fn) {
  int64_t offset = 0;
  fn.walk([&](dsp::StateSlotOp slot) {
    offset = (offset + 7) & ~7LL; // align to 8 bytes
    slot.setByteOffset(offset);
    offset += 8; // all current slot types are f64 (8 bytes)
  });
  return offset;
}

bool is_literal_const_value(mlir::Value value) {
  return mlir::matchPattern(value, mlir::m_Constant());
}

bool is_defined_in_frame_function(mlir::Value value) {
  if (auto arg = llvm::dyn_cast<mlir::BlockArgument>(value)) {
    if (auto fn =
            llvm::dyn_cast<mlir::func::FuncOp>(arg.getOwner()->getParentOp())) {
      return fn.getSymName().ends_with(".frame");
    }
    return false;
  }

  mlir::Operation *def = value.getDefiningOp();
  if (!def)
    return false;

  if (auto fn = def->getParentOfType<mlir::func::FuncOp>())
    return fn.getSymName().ends_with(".frame");
  return false;
}

bool is_frame_varying_value(mlir::Value value) {
  return is_defined_in_frame_function(value) && !is_literal_const_value(value);
}

bool any_frame_varying_values(mlir::ValueRange values) {
  return llvm::any_of(
      values, [](mlir::Value value) { return is_frame_varying_value(value); });
}

// ---------------------------------------------------------------------------
// LLVMTranslationDialectInterface
// ---------------------------------------------------------------------------

namespace {

struct DspToLLVMTranslation : public mlir::LLVMTranslationDialectInterface {
  using mlir::LLVMTranslationDialectInterface::LLVMTranslationDialectInterface;

  mlir::LogicalResult
  convertOperation(mlir::Operation *op, llvm::IRBuilderBase &builder,
                   mlir::LLVM::ModuleTranslation &mt) const override {
    if (mlir::isa<dsp::ArrayOp>(op)) {
      return lower_array_op(op, builder, mt);
    }

    if (mlir::isa<dsp::ArrayStatefulOp>(op)) {
      return lower_array_stateful_op(op, builder, mt);
    }

    if (mlir::isa<dsp::ArrayAtOp>(op)) {
      return lower_array_at_op(op, builder, mt);
    }

    if (mlir::isa<dsp::ArraySetOp>(op)) {
      return lower_array_set_op(op, builder, mt);
    }

    if (mlir::isa<dsp::StateCursorOp>(op)) {
      return lower_state_cursor_op(op, builder, mt);
    }

    if (mlir::isa<dsp::StateConsumeOp>(op)) {
      return lower_state_consume_op(op, builder, mt);
    }

    if (mlir::isa<dsp::StateSlotOp>(op)) {
      return lower_state_slot_op(op, builder, mt);
    }

    if (mlir::isa<dsp::PhasorOp>(op)) {
      return lower_phasor_op(op, builder, mt);
    }

    if (mlir::isa<dsp::PhasorStatefulOp>(op)) {
      return lower_phasor_stateful_op(op, builder, mt);
    }

    if (mlir::isa<dsp::PhasorSyncOp>(op)) {
      return lower_phasor_sync_op(op, builder, mt);
    }

    if (mlir::isa<dsp::PhasorSyncStatefulOp>(op)) {
      return lower_phasor_sync_stateful_op(op, builder, mt);
    }

    if (mlir::isa<dsp::Tabread2Op>(op)) {
      return lower_tabread2_op(op, builder, mt);
    }

    if (mlir::isa<dsp::TabreadOp>(op)) {
      return lower_tabread_op(op, builder, mt);
    }

    return mlir::failure();
  }
};

} // namespace

// ---------------------------------------------------------------------------
// DspDialect
// ---------------------------------------------------------------------------

} // namespace dsp

MLIR_DEFINE_EXPLICIT_TYPE_ID(DspDialect)
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::ArrayType)

DspDialect::DspDialect(mlir::MLIRContext *ctx)
    : mlir::Dialect(getDialectNamespace(), ctx,
                    mlir::TypeID::get<DspDialect>()) {
  initialize();
}

void DspDialect::initialize() {
  addTypes<dsp::ArrayType>();
  addOperations<
#define GET_OP_LIST
#include "generated/DspOps.cpp.inc"
      >();
  addInterface<dsp::DspToLLVMTranslation>();
}

mlir::Type DspDialect::parseType(mlir::DialectAsmParser &parser) const {
  llvm::StringRef mnemonic;
  if (failed(parser.parseKeyword(&mnemonic)))
    return {};
  if (mnemonic == dsp::ArrayType::getMnemonic())
    return dsp::ArrayType::parse(parser);
  parser.emitError(parser.getNameLoc(), "unknown dsp type `")
      << mnemonic << "`";
  return {};
}

void DspDialect::printType(mlir::Type type,
                           mlir::DialectAsmPrinter &printer) const {
  if (auto arr = llvm::dyn_cast<dsp::ArrayType>(type)) {
    printer << dsp::ArrayType::getMnemonic();
    arr.print(printer);
    return;
  }
  llvm_unreachable("unknown dsp type");
}
