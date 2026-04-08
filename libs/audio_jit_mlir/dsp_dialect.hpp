#pragma once

#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/LLVMIR/LLVMTypes.h>
#include <mlir/IR/Dialect.h>
#include <mlir/IR/DialectImplementation.h>
#include <mlir/IR/OpDefinition.h>
#include <mlir/IR/OpImplementation.h>
#include <mlir/IR/Types.h>
#include <mlir/IR/Value.h>

// ---------------------------------------------------------------------------
// DspDialect
// ---------------------------------------------------------------------------

class DspDialect : public mlir::Dialect {
  explicit DspDialect(mlir::MLIRContext *ctx);
  void initialize();
  friend class mlir::MLIRContext;

public:
  ~DspDialect() override = default;
  static constexpr llvm::StringLiteral getDialectNamespace() { return "dsp"; }
  mlir::Type parseType(mlir::DialectAsmParser &parser) const override;
  void printType(mlir::Type type,
                 mlir::DialectAsmPrinter &printer) const override;
};

MLIR_DECLARE_EXPLICIT_TYPE_ID(DspDialect)

namespace dsp {

namespace detail {
struct ArrayTypeStorage;
}

class ArrayType : public mlir::Type::TypeBase<ArrayType, mlir::Type,
                                              detail::ArrayTypeStorage> {
public:
  using Base::Base;
  static constexpr llvm::StringLiteral name = "dsp.array";
  static constexpr llvm::StringLiteral dialectName = "dsp";
  static constexpr llvm::StringLiteral getMnemonic() { return "array"; }

  static ArrayType get(mlir::MLIRContext *ctx, mlir::Type elementType);
  static ArrayType get(mlir::Type elementType);
  static mlir::Type parse(mlir::AsmParser &parser);
  void print(mlir::AsmPrinter &printer) const;

  mlir::Type getElementType() const;
};

} // namespace dsp

MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::ArrayType)

#define GET_OP_CLASSES
#include "generated/DspOps.h.inc"

// ---------------------------------------------------------------------------
// Pre-pass: assign byte offsets to all dsp.state_slot ops in a function.
// Returns total state bytes needed.
// ---------------------------------------------------------------------------

namespace dsp {
int64_t assign_dsp_state_offsets(mlir::func::FuncOp fn);
int64_t getArrayElemSize(mlir::Type type);
int32_t getArrayElemAlign(mlir::Type type);

bool is_literal_const_value(mlir::Value value);
bool is_defined_in_frame_function(mlir::Value value);
bool is_frame_varying_value(mlir::Value value);
bool any_frame_varying_values(mlir::ValueRange values);
} // namespace dsp
