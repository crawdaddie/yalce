include "mlir/Dialect/LLVMIR/LLVMOpBase.td"
include "mlir/IR/CommonAttrConstraints.td"
include "mlir/IR/CommonTypeConstraints.td"
include "mlir/IR/OpBase.td"

def Dsp_Dialect : Dialect {
  let name = "dsp";
}

class Dsp_Op<string mnemonic, list<Trait> traits = []>
    : Op<Dsp_Dialect, mnemonic, traits>;

def Dsp_ArrayTypeConstraint : Type<
    CPred<"::llvm::isa<dsp::ArrayType>($_self)">,
    "dsp array type">;

def StateSlotOp : Dsp_Op<"state_slot"> {
  let arguments = (ins
    TypeAttr:$elem_type,
    F64Attr:$init_val,
    SI64Attr:$byte_offset
  );
  let results = (outs LLVM_AnyPointer:$result);
  let extraClassDeclaration = [{
    static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                      ::mlir::Type elem_type, double init_val = 0.0);
  }];
}

def StateCursorOp : Dsp_Op<"state_cursor"> {
  let arguments = (ins LLVM_AnyPointer:$base);
  let results = (outs LLVM_AnyPointer:$cursor);
}

def StateConsumeOp : Dsp_Op<"state_consume"> {
  let arguments = (ins
    LLVM_AnyPointer:$cursor,
    I32Attr:$size,
    I32Attr:$align
  );
  let results = (outs LLVM_AnyPointer:$ptr, LLVM_AnyPointer:$next_cursor);
  let assemblyFormat = "$cursor attr-dict `:` functional-type(operands, results)";
  let extraClassDeclaration = [{
    static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                      ::mlir::Value cursor, int32_t size, int32_t align);
  }];
}

def SubSynthOp : Dsp_Op<"subsynth"> {
  let arguments = (ins
    Variadic<F64>:$inputs,
    StrAttr:$synth_name,
    I32Attr:$state_bytes,
    I32Attr:$num_inputs
  );
  let results = (outs F64:$result);
  let extraClassDeclaration = [{
    static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                      ::llvm::StringRef synth_name, int32_t state_bytes,
                      int32_t num_inputs, ::mlir::ValueRange inputs);
  }];
}

def ArrayOp : Dsp_Op<"array"> {
  let arguments = (ins
    TypeAttr:$elem_type,
    Variadic<AnyType>:$values
  );
  let results = (outs Dsp_ArrayTypeConstraint:$result);
  let extraClassDeclaration = [{
    static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                      ::mlir::Type elem_type, ::mlir::ValueRange values);
  }];
}

def ArrayStatefulOp : Dsp_Op<"array_state"> {
  let arguments = (ins
    TypeAttr:$elem_type,
    Variadic<AnyType>:$values
  );
  let results = (outs Dsp_ArrayTypeConstraint:$result);
  let extraClassDeclaration = [{
    static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                      ::mlir::Type elem_type, ::mlir::ValueRange values);
  }];
}

def ArrayAtOp : Dsp_Op<"array_at"> {
  let arguments = (ins
    Dsp_ArrayTypeConstraint:$array,
    I32:$index
  );
  let results = (outs AnyType:$result);
  let extraClassDeclaration = [{
    static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                      ::mlir::Value array, ::mlir::Value index);
  }];
}

def ArraySetOp : Dsp_Op<"array_set"> {
  let arguments = (ins
    Dsp_ArrayTypeConstraint:$array,
    I32:$index,
    AnyType:$value
  );
}

def PhasorOp : Dsp_Op<"phasor"> {
  let arguments = (ins
    F64:$freq,
    F64Attr:$spf
  );
  let results = (outs F64:$result);
  let extraClassDeclaration = [{
    static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                      ::mlir::Value freq, double spf);
  }];
}

def PhasorStatefulOp : Dsp_Op<"phasor_stateful"> {
  let arguments = (ins
    LLVM_AnyPointer:$phase_slot,
    F64:$freq,
    F64Attr:$spf
  );
  let results = (outs F64:$result);
  let extraClassDeclaration = [{
    static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                      ::mlir::Value phase_slot, ::mlir::Value freq, double spf);
  }];
}

def Tabread2Op : Dsp_Op<"tabread2"> {
  let arguments = (ins
    F64:$phase,
    StrAttr:$table_symbol,
    I32Attr:$table_size
  );
  let results = (outs F64:$result);
  let extraClassDeclaration = [{
    static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                      ::mlir::Value phase, ::llvm::StringRef table_symbol,
                      int32_t table_size);
  }];
}

def TabreadOp : Dsp_Op<"tabread"> {
  let arguments = (ins
    F64:$phase,
    LLVM_AnyPointer:$table_ptr,
    I32Attr:$table_size
  );
  let results = (outs F64:$result);
  let extraClassDeclaration = [{
    static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                      ::mlir::Value phase, ::mlir::Value table_ptr,
                      int32_t table_size);
  }];
}
