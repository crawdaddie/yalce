/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Op Definitions                                                             *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: DspOps.td                                                            *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifdef GET_OP_LIST
#undef GET_OP_LIST

dsp::ArrayAtOp,
dsp::ArrayOp,
dsp::ArraySetOp,
dsp::ArrayStatefulOp,
dsp::PhasorOp,
dsp::PhasorStatefulOp,
dsp::PhasorSyncOp,
dsp::PhasorSyncStatefulOp,
dsp::StateConsumeOp,
dsp::StateCursorOp,
dsp::StateSlotOp,
dsp::SubSynthOp,
dsp::Tabread2Op,
dsp::TabreadOp
#endif  // GET_OP_LIST

#ifdef GET_OP_CLASSES
#undef GET_OP_CLASSES


//===----------------------------------------------------------------------===//
// Local Utility Method Definitions
//===----------------------------------------------------------------------===//

namespace dsp {

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_DspOps1(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((::llvm::isa<dsp::ArrayType>(type)))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be dsp array type, but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_DspOps2(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((type.isSignlessInteger(32)))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be 32-bit signless integer, but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_DspOps3(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((true))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be any type, but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_DspOps4(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((true))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be variadic of any type, but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_DspOps5(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((type.isF64()))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be 64-bit float, but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_DspOps6(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((::llvm::isa<::mlir::LLVM::LLVMPointerType>(type)))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be LLVM pointer type, but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_DspOps7(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((type.isF64()))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be variadic of 64-bit float, but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_DspOps1(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !(((::llvm::isa<::mlir::TypeAttr>(attr))) && ((::llvm::isa<::mlir::Type>(::llvm::cast<::mlir::TypeAttr>(attr).getValue()))) && ((true))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: any type attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_DspOps1(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_DspOps1(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_DspOps2(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !(((::llvm::isa<::mlir::FloatAttr>(attr))) && ((::llvm::cast<::mlir::FloatAttr>(attr).getType().isF64()))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: 64-bit float attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_DspOps2(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_DspOps2(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_DspOps3(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !(((::llvm::isa<::mlir::IntegerAttr>(attr))) && ((::llvm::cast<::mlir::IntegerAttr>(attr).getType().isSignlessInteger(32)))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: 32-bit signless integer attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_DspOps3(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_DspOps3(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_DspOps4(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !(((::llvm::isa<::mlir::IntegerAttr>(attr))) && ((::llvm::cast<::mlir::IntegerAttr>(attr).getType().isSignedInteger(64)))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: 64-bit signed integer attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_DspOps4(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_DspOps4(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_DspOps5(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !((::llvm::isa<::mlir::StringAttr>(attr))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: string attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_DspOps5(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_DspOps5(attr, attrName, [op]() {
    return op->emitOpError();
  });
}
} // namespace dsp
namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::ArrayAtOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
ArrayAtOpGenericAdaptorBase::ArrayAtOpGenericAdaptorBase(ArrayAtOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

} // namespace detail
ArrayAtOpAdaptor::ArrayAtOpAdaptor(ArrayAtOp op) : ArrayAtOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ArrayAtOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

::llvm::LogicalResult ArrayAtOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
      return ::mlir::success();
}

::mlir::Attribute ArrayAtOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code ArrayAtOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine();
}

std::optional<mlir::Attribute> ArrayAtOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
  return std::nullopt;
}

void ArrayAtOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
}

void ArrayAtOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
}

::llvm::LogicalResult ArrayAtOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    return ::mlir::success();
}

void ArrayAtOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value array, ::mlir::Value index) {
  odsState.addOperands(array);
  odsState.addOperands(index);
  odsState.addTypes(result);
}

ArrayAtOp ArrayAtOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value array, ::mlir::Value index) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, array, index);
  auto __res__ = ::llvm::dyn_cast<ArrayAtOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayAtOp ArrayAtOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value array, ::mlir::Value index) {
  return create(builder, builder.getLoc(), result, array, index);
}

void ArrayAtOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index) {
  odsState.addOperands(array);
  odsState.addOperands(index);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ArrayAtOp ArrayAtOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, array, index);
  auto __res__ = ::llvm::dyn_cast<ArrayAtOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayAtOp ArrayAtOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index) {
  return create(builder, builder.getLoc(), resultTypes, array, index);
}

void ArrayAtOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ArrayAtOp ArrayAtOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<ArrayAtOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayAtOp ArrayAtOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void ArrayAtOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ArrayAtOp ArrayAtOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<ArrayAtOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayAtOp ArrayAtOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult ArrayAtOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps2(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps3(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ArrayAtOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult ArrayAtOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::ArrayAtOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::ArrayOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
ArrayOpGenericAdaptorBase::ArrayOpGenericAdaptorBase(ArrayOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

std::pair<unsigned, unsigned> ArrayOpGenericAdaptorBase::getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (odsOperandsSize - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::Type ArrayOpGenericAdaptorBase::getElemType() {
  auto attr = getElemTypeAttr();
  return ::llvm::cast<::mlir::Type>(attr.getValue());
}

} // namespace detail
ArrayOpAdaptor::ArrayOpAdaptor(ArrayOp op) : ArrayOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ArrayOpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_elem_type = getProperties().elem_type; (void)tblgen_elem_type;
  if (!tblgen_elem_type) return emitError(loc, "'dsp.array' op ""requires attribute 'elem_type'");

  if (tblgen_elem_type && !(((::llvm::isa<::mlir::TypeAttr>(tblgen_elem_type))) && ((::llvm::isa<::mlir::Type>(::llvm::cast<::mlir::TypeAttr>(tblgen_elem_type).getValue()))) && ((true))))
    return emitError(loc, "'dsp.array' op ""attribute 'elem_type' failed to satisfy constraint: any type attribute");
  return ::mlir::success();
}

std::pair<unsigned, unsigned> ArrayOp::getODSOperandIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumOperands() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::MutableOperandRange ArrayOp::getValuesMutable() {
  auto range = getODSOperandIndexAndLength(0);
  auto mutableRange = ::mlir::MutableOperandRange(getOperation(), range.first, range.second);
  return mutableRange;
}

::llvm::LogicalResult ArrayOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.elem_type;
       auto attr = dict.get("elem_type");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `elem_type` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute ArrayOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.elem_type;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("elem_type",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code ArrayOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.elem_type.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> ArrayOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "elem_type")
      return prop.elem_type;
  return std::nullopt;
}

void ArrayOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "elem_type") {
       prop.elem_type = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.elem_type)>>(value);
       return;
    }
}

void ArrayOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.elem_type) attrs.append("elem_type", prop.elem_type);
}

::llvm::LogicalResult ArrayOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getElemTypeAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps1(attr, "elem_type", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult ArrayOp::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.elem_type)))
    return ::mlir::failure();
  return ::mlir::success();
}

void ArrayOp::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.elem_type);
}

::mlir::Type ArrayOp::getElemType() {
  auto attr = getElemTypeAttr();
  return ::llvm::cast<::mlir::Type>(attr.getValue());
}

void ArrayOp::setElemType(::mlir::Type attrValue) {
  getProperties().elem_type = ::mlir::TypeAttr::get(attrValue);
}

void ArrayOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  odsState.addOperands(values);
  odsState.getOrAddProperties<Properties>().elem_type = elem_type;
  odsState.addTypes(result);
}

ArrayOp ArrayOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, elem_type, values);
  auto __res__ = ::llvm::dyn_cast<ArrayOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayOp ArrayOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  return create(builder, builder.getLoc(), result, elem_type, values);
}

void ArrayOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  odsState.addOperands(values);
  odsState.getOrAddProperties<Properties>().elem_type = elem_type;
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ArrayOp ArrayOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, elem_type, values);
  auto __res__ = ::llvm::dyn_cast<ArrayOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayOp ArrayOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  return create(builder, builder.getLoc(), resultTypes, elem_type, values);
}

void ArrayOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  odsState.addOperands(values);
  odsState.getOrAddProperties<Properties>().elem_type = ::mlir::TypeAttr::get(elem_type);
  odsState.addTypes(result);
}

ArrayOp ArrayOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, elem_type, values);
  auto __res__ = ::llvm::dyn_cast<ArrayOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayOp ArrayOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  return create(builder, builder.getLoc(), result, elem_type, values);
}

void ArrayOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  odsState.addOperands(values);
  odsState.getOrAddProperties<Properties>().elem_type = ::mlir::TypeAttr::get(elem_type);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ArrayOp ArrayOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, elem_type, values);
  auto __res__ = ::llvm::dyn_cast<ArrayOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayOp ArrayOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  return create(builder, builder.getLoc(), resultTypes, elem_type, values);
}

void ArrayOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<ArrayOp::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

ArrayOp ArrayOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<ArrayOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayOp ArrayOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void ArrayOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ArrayOp ArrayOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<ArrayOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayOp ArrayOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult ArrayOp::verifyInvariantsImpl() {
  auto tblgen_elem_type = getProperties().elem_type; (void)tblgen_elem_type;
  if (!tblgen_elem_type) return emitOpError("requires attribute 'elem_type'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps1(*this, tblgen_elem_type, "elem_type")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps4(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ArrayOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult ArrayOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;
  {

    auto &propStorage = prop.elem_type;
    auto elem_typeAttrName = ::mlir::StringAttr::get(ctx, "elem_type");
    auto attr = dict.get(elem_typeAttrName);
    usedKeys.insert(elem_typeAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for elem_type in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `elem_type` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::ArrayOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::ArraySetOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
ArraySetOpGenericAdaptorBase::ArraySetOpGenericAdaptorBase(ArraySetOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

} // namespace detail
ArraySetOpAdaptor::ArraySetOpAdaptor(ArraySetOp op) : ArraySetOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ArraySetOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

::llvm::LogicalResult ArraySetOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
      return ::mlir::success();
}

::mlir::Attribute ArraySetOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code ArraySetOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine();
}

std::optional<mlir::Attribute> ArraySetOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
  return std::nullopt;
}

void ArraySetOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
}

void ArraySetOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
}

::llvm::LogicalResult ArraySetOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    return ::mlir::success();
}

void ArraySetOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value) {
  odsState.addOperands(array);
  odsState.addOperands(index);
  odsState.addOperands(value);
  odsState.addTypes(result);
}

ArraySetOp ArraySetOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, array, index, value);
  auto __res__ = ::llvm::dyn_cast<ArraySetOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArraySetOp ArraySetOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value) {
  return create(builder, builder.getLoc(), result, array, index, value);
}

void ArraySetOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value) {
  odsState.addOperands(array);
  odsState.addOperands(index);
  odsState.addOperands(value);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ArraySetOp ArraySetOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, array, index, value);
  auto __res__ = ::llvm::dyn_cast<ArraySetOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArraySetOp ArraySetOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value) {
  return create(builder, builder.getLoc(), resultTypes, array, index, value);
}

void ArraySetOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 3u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ArraySetOp ArraySetOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<ArraySetOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArraySetOp ArraySetOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void ArraySetOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 3u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ArraySetOp ArraySetOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<ArraySetOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArraySetOp ArraySetOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult ArraySetOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps2(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup2 = getODSOperands(2);

    for (auto v : valueGroup2) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps3(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ArraySetOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult ArraySetOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::ArraySetOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::ArrayStatefulOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
ArrayStatefulOpGenericAdaptorBase::ArrayStatefulOpGenericAdaptorBase(ArrayStatefulOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

std::pair<unsigned, unsigned> ArrayStatefulOpGenericAdaptorBase::getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (odsOperandsSize - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::Type ArrayStatefulOpGenericAdaptorBase::getElemType() {
  auto attr = getElemTypeAttr();
  return ::llvm::cast<::mlir::Type>(attr.getValue());
}

} // namespace detail
ArrayStatefulOpAdaptor::ArrayStatefulOpAdaptor(ArrayStatefulOp op) : ArrayStatefulOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ArrayStatefulOpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_elem_type = getProperties().elem_type; (void)tblgen_elem_type;
  if (!tblgen_elem_type) return emitError(loc, "'dsp.array_state' op ""requires attribute 'elem_type'");

  if (tblgen_elem_type && !(((::llvm::isa<::mlir::TypeAttr>(tblgen_elem_type))) && ((::llvm::isa<::mlir::Type>(::llvm::cast<::mlir::TypeAttr>(tblgen_elem_type).getValue()))) && ((true))))
    return emitError(loc, "'dsp.array_state' op ""attribute 'elem_type' failed to satisfy constraint: any type attribute");
  return ::mlir::success();
}

std::pair<unsigned, unsigned> ArrayStatefulOp::getODSOperandIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumOperands() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::MutableOperandRange ArrayStatefulOp::getValuesMutable() {
  auto range = getODSOperandIndexAndLength(0);
  auto mutableRange = ::mlir::MutableOperandRange(getOperation(), range.first, range.second);
  return mutableRange;
}

::llvm::LogicalResult ArrayStatefulOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.elem_type;
       auto attr = dict.get("elem_type");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `elem_type` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute ArrayStatefulOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.elem_type;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("elem_type",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code ArrayStatefulOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.elem_type.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> ArrayStatefulOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "elem_type")
      return prop.elem_type;
  return std::nullopt;
}

void ArrayStatefulOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "elem_type") {
       prop.elem_type = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.elem_type)>>(value);
       return;
    }
}

void ArrayStatefulOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.elem_type) attrs.append("elem_type", prop.elem_type);
}

::llvm::LogicalResult ArrayStatefulOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getElemTypeAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps1(attr, "elem_type", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult ArrayStatefulOp::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.elem_type)))
    return ::mlir::failure();
  return ::mlir::success();
}

void ArrayStatefulOp::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.elem_type);
}

::mlir::Type ArrayStatefulOp::getElemType() {
  auto attr = getElemTypeAttr();
  return ::llvm::cast<::mlir::Type>(attr.getValue());
}

void ArrayStatefulOp::setElemType(::mlir::Type attrValue) {
  getProperties().elem_type = ::mlir::TypeAttr::get(attrValue);
}

void ArrayStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  odsState.addOperands(values);
  odsState.getOrAddProperties<Properties>().elem_type = elem_type;
  odsState.addTypes(result);
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, elem_type, values);
  auto __res__ = ::llvm::dyn_cast<ArrayStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  return create(builder, builder.getLoc(), result, elem_type, values);
}

void ArrayStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  odsState.addOperands(values);
  odsState.getOrAddProperties<Properties>().elem_type = elem_type;
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, elem_type, values);
  auto __res__ = ::llvm::dyn_cast<ArrayStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values) {
  return create(builder, builder.getLoc(), resultTypes, elem_type, values);
}

void ArrayStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  odsState.addOperands(values);
  odsState.getOrAddProperties<Properties>().elem_type = ::mlir::TypeAttr::get(elem_type);
  odsState.addTypes(result);
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, elem_type, values);
  auto __res__ = ::llvm::dyn_cast<ArrayStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  return create(builder, builder.getLoc(), result, elem_type, values);
}

void ArrayStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  odsState.addOperands(values);
  odsState.getOrAddProperties<Properties>().elem_type = ::mlir::TypeAttr::get(elem_type);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, elem_type, values);
  auto __res__ = ::llvm::dyn_cast<ArrayStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values) {
  return create(builder, builder.getLoc(), resultTypes, elem_type, values);
}

void ArrayStatefulOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<ArrayStatefulOp::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<ArrayStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void ArrayStatefulOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<ArrayStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ArrayStatefulOp ArrayStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult ArrayStatefulOp::verifyInvariantsImpl() {
  auto tblgen_elem_type = getProperties().elem_type; (void)tblgen_elem_type;
  if (!tblgen_elem_type) return emitOpError("requires attribute 'elem_type'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps1(*this, tblgen_elem_type, "elem_type")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps4(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ArrayStatefulOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult ArrayStatefulOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;
  {

    auto &propStorage = prop.elem_type;
    auto elem_typeAttrName = ::mlir::StringAttr::get(ctx, "elem_type");
    auto attr = dict.get(elem_typeAttrName);
    usedKeys.insert(elem_typeAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for elem_type in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `elem_type` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::ArrayStatefulOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::PhasorOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
PhasorOpGenericAdaptorBase::PhasorOpGenericAdaptorBase(PhasorOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

::llvm::APFloat PhasorOpGenericAdaptorBase::getSpf() {
  auto attr = getSpfAttr();
  return attr.getValue();
}

} // namespace detail
PhasorOpAdaptor::PhasorOpAdaptor(PhasorOp op) : PhasorOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult PhasorOpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_spf = getProperties().spf; (void)tblgen_spf;
  if (!tblgen_spf) return emitError(loc, "'dsp.phasor' op ""requires attribute 'spf'");

  if (tblgen_spf && !(((::llvm::isa<::mlir::FloatAttr>(tblgen_spf))) && ((::llvm::cast<::mlir::FloatAttr>(tblgen_spf).getType().isF64()))))
    return emitError(loc, "'dsp.phasor' op ""attribute 'spf' failed to satisfy constraint: 64-bit float attribute");
  return ::mlir::success();
}

::llvm::LogicalResult PhasorOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.spf;
       auto attr = dict.get("spf");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `spf` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute PhasorOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.spf;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("spf",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code PhasorOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.spf.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> PhasorOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "spf")
      return prop.spf;
  return std::nullopt;
}

void PhasorOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "spf") {
       prop.spf = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.spf)>>(value);
       return;
    }
}

void PhasorOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.spf) attrs.append("spf", prop.spf);
}

::llvm::LogicalResult PhasorOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getSpfAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps2(attr, "spf", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult PhasorOp::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.spf)))
    return ::mlir::failure();
  return ::mlir::success();
}

void PhasorOp::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.spf);
}

::llvm::APFloat PhasorOp::getSpf() {
  auto attr = getSpfAttr();
  return attr.getValue();
}

void PhasorOp::setSpf(::llvm::APFloat attrValue) {
  getProperties().spf = ::mlir::Builder((*this)->getContext()).getFloatAttr(::mlir::Builder((*this)->getContext()).getF64Type(), attrValue);
}

void PhasorOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  odsState.addOperands(freq);
  odsState.getOrAddProperties<Properties>().spf = spf;
  odsState.addTypes(result);
}

PhasorOp PhasorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, freq, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorOp PhasorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  return create(builder, builder.getLoc(), result, freq, spf);
}

void PhasorOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  odsState.addOperands(freq);
  odsState.getOrAddProperties<Properties>().spf = spf;
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

PhasorOp PhasorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, freq, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorOp PhasorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  return create(builder, builder.getLoc(), resultTypes, freq, spf);
}

void PhasorOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value freq, ::llvm::APFloat spf) {
  odsState.addOperands(freq);
  odsState.getOrAddProperties<Properties>().spf = odsBuilder.getFloatAttr(odsBuilder.getF64Type(), spf);
  odsState.addTypes(result);
}

PhasorOp PhasorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value freq, ::llvm::APFloat spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, freq, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorOp PhasorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value freq, ::llvm::APFloat spf) {
  return create(builder, builder.getLoc(), result, freq, spf);
}

void PhasorOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::llvm::APFloat spf) {
  odsState.addOperands(freq);
  odsState.getOrAddProperties<Properties>().spf = odsBuilder.getFloatAttr(odsBuilder.getF64Type(), spf);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

PhasorOp PhasorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::llvm::APFloat spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, freq, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorOp PhasorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::llvm::APFloat spf) {
  return create(builder, builder.getLoc(), resultTypes, freq, spf);
}

void PhasorOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<PhasorOp::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

PhasorOp PhasorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<PhasorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorOp PhasorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void PhasorOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

PhasorOp PhasorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<PhasorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorOp PhasorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult PhasorOp::verifyInvariantsImpl() {
  auto tblgen_spf = getProperties().spf; (void)tblgen_spf;
  if (!tblgen_spf) return emitOpError("requires attribute 'spf'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps2(*this, tblgen_spf, "spf")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult PhasorOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult PhasorOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;
  {

    auto &propStorage = prop.spf;
    auto spfAttrName = ::mlir::StringAttr::get(ctx, "spf");
    auto attr = dict.get(spfAttrName);
    usedKeys.insert(spfAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for spf in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `spf` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::PhasorOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::PhasorStatefulOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
PhasorStatefulOpGenericAdaptorBase::PhasorStatefulOpGenericAdaptorBase(PhasorStatefulOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

::llvm::APFloat PhasorStatefulOpGenericAdaptorBase::getSpf() {
  auto attr = getSpfAttr();
  return attr.getValue();
}

} // namespace detail
PhasorStatefulOpAdaptor::PhasorStatefulOpAdaptor(PhasorStatefulOp op) : PhasorStatefulOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult PhasorStatefulOpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_spf = getProperties().spf; (void)tblgen_spf;
  if (!tblgen_spf) return emitError(loc, "'dsp.phasor_stateful' op ""requires attribute 'spf'");

  if (tblgen_spf && !(((::llvm::isa<::mlir::FloatAttr>(tblgen_spf))) && ((::llvm::cast<::mlir::FloatAttr>(tblgen_spf).getType().isF64()))))
    return emitError(loc, "'dsp.phasor_stateful' op ""attribute 'spf' failed to satisfy constraint: 64-bit float attribute");
  return ::mlir::success();
}

::llvm::LogicalResult PhasorStatefulOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.spf;
       auto attr = dict.get("spf");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `spf` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute PhasorStatefulOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.spf;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("spf",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code PhasorStatefulOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.spf.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> PhasorStatefulOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "spf")
      return prop.spf;
  return std::nullopt;
}

void PhasorStatefulOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "spf") {
       prop.spf = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.spf)>>(value);
       return;
    }
}

void PhasorStatefulOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.spf) attrs.append("spf", prop.spf);
}

::llvm::LogicalResult PhasorStatefulOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getSpfAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps2(attr, "spf", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult PhasorStatefulOp::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.spf)))
    return ::mlir::failure();
  return ::mlir::success();
}

void PhasorStatefulOp::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.spf);
}

::llvm::APFloat PhasorStatefulOp::getSpf() {
  auto attr = getSpfAttr();
  return attr.getValue();
}

void PhasorStatefulOp::setSpf(::llvm::APFloat attrValue) {
  getProperties().spf = ::mlir::Builder((*this)->getContext()).getFloatAttr(::mlir::Builder((*this)->getContext()).getF64Type(), attrValue);
}

void PhasorStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  odsState.addOperands(phase_slot);
  odsState.addOperands(freq);
  odsState.getOrAddProperties<Properties>().spf = spf;
  odsState.addTypes(result);
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, phase_slot, freq, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  return create(builder, builder.getLoc(), result, phase_slot, freq, spf);
}

void PhasorStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  odsState.addOperands(phase_slot);
  odsState.addOperands(freq);
  odsState.getOrAddProperties<Properties>().spf = spf;
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, phase_slot, freq, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf) {
  return create(builder, builder.getLoc(), resultTypes, phase_slot, freq, spf);
}

void PhasorStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf) {
  odsState.addOperands(phase_slot);
  odsState.addOperands(freq);
  odsState.getOrAddProperties<Properties>().spf = odsBuilder.getFloatAttr(odsBuilder.getF64Type(), spf);
  odsState.addTypes(result);
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, phase_slot, freq, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf) {
  return create(builder, builder.getLoc(), result, phase_slot, freq, spf);
}

void PhasorStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf) {
  odsState.addOperands(phase_slot);
  odsState.addOperands(freq);
  odsState.getOrAddProperties<Properties>().spf = odsBuilder.getFloatAttr(odsBuilder.getF64Type(), spf);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, phase_slot, freq, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf) {
  return create(builder, builder.getLoc(), resultTypes, phase_slot, freq, spf);
}

void PhasorStatefulOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<PhasorStatefulOp::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<PhasorStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void PhasorStatefulOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<PhasorStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorStatefulOp PhasorStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult PhasorStatefulOp::verifyInvariantsImpl() {
  auto tblgen_spf = getProperties().spf; (void)tblgen_spf;
  if (!tblgen_spf) return emitOpError("requires attribute 'spf'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps2(*this, tblgen_spf, "spf")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps6(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult PhasorStatefulOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult PhasorStatefulOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;
  {

    auto &propStorage = prop.spf;
    auto spfAttrName = ::mlir::StringAttr::get(ctx, "spf");
    auto attr = dict.get(spfAttrName);
    usedKeys.insert(spfAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for spf in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `spf` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::PhasorStatefulOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::PhasorSyncOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
PhasorSyncOpGenericAdaptorBase::PhasorSyncOpGenericAdaptorBase(PhasorSyncOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

::llvm::APFloat PhasorSyncOpGenericAdaptorBase::getSpf() {
  auto attr = getSpfAttr();
  return attr.getValue();
}

} // namespace detail
PhasorSyncOpAdaptor::PhasorSyncOpAdaptor(PhasorSyncOp op) : PhasorSyncOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult PhasorSyncOpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_spf = getProperties().spf; (void)tblgen_spf;
  if (!tblgen_spf) return emitError(loc, "'dsp.phasor_sync' op ""requires attribute 'spf'");

  if (tblgen_spf && !(((::llvm::isa<::mlir::FloatAttr>(tblgen_spf))) && ((::llvm::cast<::mlir::FloatAttr>(tblgen_spf).getType().isF64()))))
    return emitError(loc, "'dsp.phasor_sync' op ""attribute 'spf' failed to satisfy constraint: 64-bit float attribute");
  return ::mlir::success();
}

::llvm::LogicalResult PhasorSyncOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.spf;
       auto attr = dict.get("spf");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `spf` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute PhasorSyncOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.spf;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("spf",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code PhasorSyncOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.spf.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> PhasorSyncOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "spf")
      return prop.spf;
  return std::nullopt;
}

void PhasorSyncOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "spf") {
       prop.spf = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.spf)>>(value);
       return;
    }
}

void PhasorSyncOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.spf) attrs.append("spf", prop.spf);
}

::llvm::LogicalResult PhasorSyncOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getSpfAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps2(attr, "spf", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult PhasorSyncOp::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.spf)))
    return ::mlir::failure();
  return ::mlir::success();
}

void PhasorSyncOp::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.spf);
}

::llvm::APFloat PhasorSyncOp::getSpf() {
  auto attr = getSpfAttr();
  return attr.getValue();
}

void PhasorSyncOp::setSpf(::llvm::APFloat attrValue) {
  getProperties().spf = ::mlir::Builder((*this)->getContext()).getFloatAttr(::mlir::Builder((*this)->getContext()).getF64Type(), attrValue);
}

void PhasorSyncOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  odsState.addOperands(freq);
  odsState.addOperands(trig);
  odsState.getOrAddProperties<Properties>().spf = spf;
  odsState.addTypes(result);
}

PhasorSyncOp PhasorSyncOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, freq, trig, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncOp PhasorSyncOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  return create(builder, builder.getLoc(), result, freq, trig, spf);
}

void PhasorSyncOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  odsState.addOperands(freq);
  odsState.addOperands(trig);
  odsState.getOrAddProperties<Properties>().spf = spf;
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

PhasorSyncOp PhasorSyncOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, freq, trig, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncOp PhasorSyncOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  return create(builder, builder.getLoc(), resultTypes, freq, trig, spf);
}

void PhasorSyncOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  odsState.addOperands(freq);
  odsState.addOperands(trig);
  odsState.getOrAddProperties<Properties>().spf = odsBuilder.getFloatAttr(odsBuilder.getF64Type(), spf);
  odsState.addTypes(result);
}

PhasorSyncOp PhasorSyncOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, freq, trig, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncOp PhasorSyncOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  return create(builder, builder.getLoc(), result, freq, trig, spf);
}

void PhasorSyncOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  odsState.addOperands(freq);
  odsState.addOperands(trig);
  odsState.getOrAddProperties<Properties>().spf = odsBuilder.getFloatAttr(odsBuilder.getF64Type(), spf);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

PhasorSyncOp PhasorSyncOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, freq, trig, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncOp PhasorSyncOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  return create(builder, builder.getLoc(), resultTypes, freq, trig, spf);
}

void PhasorSyncOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<PhasorSyncOp::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

PhasorSyncOp PhasorSyncOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncOp PhasorSyncOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void PhasorSyncOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

PhasorSyncOp PhasorSyncOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncOp PhasorSyncOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult PhasorSyncOp::verifyInvariantsImpl() {
  auto tblgen_spf = getProperties().spf; (void)tblgen_spf;
  if (!tblgen_spf) return emitOpError("requires attribute 'spf'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps2(*this, tblgen_spf, "spf")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult PhasorSyncOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult PhasorSyncOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;
  {

    auto &propStorage = prop.spf;
    auto spfAttrName = ::mlir::StringAttr::get(ctx, "spf");
    auto attr = dict.get(spfAttrName);
    usedKeys.insert(spfAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for spf in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `spf` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::PhasorSyncOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::PhasorSyncStatefulOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
PhasorSyncStatefulOpGenericAdaptorBase::PhasorSyncStatefulOpGenericAdaptorBase(PhasorSyncStatefulOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

::llvm::APFloat PhasorSyncStatefulOpGenericAdaptorBase::getSpf() {
  auto attr = getSpfAttr();
  return attr.getValue();
}

} // namespace detail
PhasorSyncStatefulOpAdaptor::PhasorSyncStatefulOpAdaptor(PhasorSyncStatefulOp op) : PhasorSyncStatefulOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult PhasorSyncStatefulOpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_spf = getProperties().spf; (void)tblgen_spf;
  if (!tblgen_spf) return emitError(loc, "'dsp.phasor_sync_stateful' op ""requires attribute 'spf'");

  if (tblgen_spf && !(((::llvm::isa<::mlir::FloatAttr>(tblgen_spf))) && ((::llvm::cast<::mlir::FloatAttr>(tblgen_spf).getType().isF64()))))
    return emitError(loc, "'dsp.phasor_sync_stateful' op ""attribute 'spf' failed to satisfy constraint: 64-bit float attribute");
  return ::mlir::success();
}

::llvm::LogicalResult PhasorSyncStatefulOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.spf;
       auto attr = dict.get("spf");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `spf` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute PhasorSyncStatefulOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.spf;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("spf",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code PhasorSyncStatefulOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.spf.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> PhasorSyncStatefulOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "spf")
      return prop.spf;
  return std::nullopt;
}

void PhasorSyncStatefulOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "spf") {
       prop.spf = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.spf)>>(value);
       return;
    }
}

void PhasorSyncStatefulOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.spf) attrs.append("spf", prop.spf);
}

::llvm::LogicalResult PhasorSyncStatefulOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getSpfAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps2(attr, "spf", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult PhasorSyncStatefulOp::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.spf)))
    return ::mlir::failure();
  return ::mlir::success();
}

void PhasorSyncStatefulOp::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.spf);
}

::llvm::APFloat PhasorSyncStatefulOp::getSpf() {
  auto attr = getSpfAttr();
  return attr.getValue();
}

void PhasorSyncStatefulOp::setSpf(::llvm::APFloat attrValue) {
  getProperties().spf = ::mlir::Builder((*this)->getContext()).getFloatAttr(::mlir::Builder((*this)->getContext()).getF64Type(), attrValue);
}

void PhasorSyncStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  odsState.addOperands(phase_slot);
  odsState.addOperands(freq);
  odsState.addOperands(trig);
  odsState.getOrAddProperties<Properties>().spf = spf;
  odsState.addTypes(result);
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, phase_slot, freq, trig, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  return create(builder, builder.getLoc(), result, phase_slot, freq, trig, spf);
}

void PhasorSyncStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  odsState.addOperands(phase_slot);
  odsState.addOperands(freq);
  odsState.addOperands(trig);
  odsState.getOrAddProperties<Properties>().spf = spf;
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, phase_slot, freq, trig, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::mlir::FloatAttr spf) {
  return create(builder, builder.getLoc(), resultTypes, phase_slot, freq, trig, spf);
}

void PhasorSyncStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  odsState.addOperands(phase_slot);
  odsState.addOperands(freq);
  odsState.addOperands(trig);
  odsState.getOrAddProperties<Properties>().spf = odsBuilder.getFloatAttr(odsBuilder.getF64Type(), spf);
  odsState.addTypes(result);
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, phase_slot, freq, trig, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  return create(builder, builder.getLoc(), result, phase_slot, freq, trig, spf);
}

void PhasorSyncStatefulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  odsState.addOperands(phase_slot);
  odsState.addOperands(freq);
  odsState.addOperands(trig);
  odsState.getOrAddProperties<Properties>().spf = odsBuilder.getFloatAttr(odsBuilder.getF64Type(), spf);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, phase_slot, freq, trig, spf);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::Value trig, ::llvm::APFloat spf) {
  return create(builder, builder.getLoc(), resultTypes, phase_slot, freq, trig, spf);
}

void PhasorSyncStatefulOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 3u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<PhasorSyncStatefulOp::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void PhasorSyncStatefulOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 3u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<PhasorSyncStatefulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PhasorSyncStatefulOp PhasorSyncStatefulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult PhasorSyncStatefulOp::verifyInvariantsImpl() {
  auto tblgen_spf = getProperties().spf; (void)tblgen_spf;
  if (!tblgen_spf) return emitOpError("requires attribute 'spf'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps2(*this, tblgen_spf, "spf")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps6(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup2 = getODSOperands(2);

    for (auto v : valueGroup2) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult PhasorSyncStatefulOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult PhasorSyncStatefulOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;
  {

    auto &propStorage = prop.spf;
    auto spfAttrName = ::mlir::StringAttr::get(ctx, "spf");
    auto attr = dict.get(spfAttrName);
    usedKeys.insert(spfAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for spf in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `spf` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::PhasorSyncStatefulOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::StateConsumeOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
StateConsumeOpGenericAdaptorBase::StateConsumeOpGenericAdaptorBase(StateConsumeOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

uint32_t StateConsumeOpGenericAdaptorBase::getSize() {
  auto attr = getSizeAttr();
  return attr.getValue().getZExtValue();
}

uint32_t StateConsumeOpGenericAdaptorBase::getAlign() {
  auto attr = getAlignAttr();
  return attr.getValue().getZExtValue();
}

} // namespace detail
StateConsumeOpAdaptor::StateConsumeOpAdaptor(StateConsumeOp op) : StateConsumeOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult StateConsumeOpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_align = getProperties().align; (void)tblgen_align;
  if (!tblgen_align) return emitError(loc, "'dsp.state_consume' op ""requires attribute 'align'");
  auto tblgen_size = getProperties().size; (void)tblgen_size;
  if (!tblgen_size) return emitError(loc, "'dsp.state_consume' op ""requires attribute 'size'");

  if (tblgen_size && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_size))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_size).getType().isSignlessInteger(32)))))
    return emitError(loc, "'dsp.state_consume' op ""attribute 'size' failed to satisfy constraint: 32-bit signless integer attribute");

  if (tblgen_align && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_align))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_align).getType().isSignlessInteger(32)))))
    return emitError(loc, "'dsp.state_consume' op ""attribute 'align' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

void StateConsumeOp::getAsmResultNames(::mlir::OpAsmSetValueNameFn setNameFn) {
  auto resultGroup0 = getODSResults(0);
  if (!resultGroup0.empty())
    setNameFn(*resultGroup0.begin(), "ptr");
  auto resultGroup1 = getODSResults(1);
  if (!resultGroup1.empty())
    setNameFn(*resultGroup1.begin(), "next_cursor");
}

::llvm::LogicalResult StateConsumeOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.align;
       auto attr = dict.get("align");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `align` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  {
    auto &propStorage = prop.size;
       auto attr = dict.get("size");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `size` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute StateConsumeOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.align;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("align",
                                       propStorage));
    }

    {
      const auto &propStorage = prop.size;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("size",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code StateConsumeOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.align.getAsOpaquePointer()), 
    llvm::hash_value(prop.size.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> StateConsumeOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "align")
      return prop.align;

    if (name == "size")
      return prop.size;
  return std::nullopt;
}

void StateConsumeOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "align") {
       prop.align = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.align)>>(value);
       return;
    }

    if (name == "size") {
       prop.size = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.size)>>(value);
       return;
    }
}

void StateConsumeOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.align) attrs.append("align", prop.align);

    if (prop.size) attrs.append("size", prop.size);
}

::llvm::LogicalResult StateConsumeOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getAlignAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(attr, "align", emitError)))
        return ::mlir::failure();
    }

    {
      ::mlir::Attribute attr = attrs.get(getSizeAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(attr, "size", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult StateConsumeOp::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.align)))
    return ::mlir::failure();

  if (::mlir::failed(reader.readAttribute(prop.size)))
    return ::mlir::failure();
  return ::mlir::success();
}

void StateConsumeOp::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.align);
  writer.writeAttribute(prop.size);
}

uint32_t StateConsumeOp::getSize() {
  auto attr = getSizeAttr();
  return attr.getValue().getZExtValue();
}

uint32_t StateConsumeOp::getAlign() {
  auto attr = getAlignAttr();
  return attr.getValue().getZExtValue();
}

void StateConsumeOp::setSize(uint32_t attrValue) {
  getProperties().size = ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue);
}

void StateConsumeOp::setAlign(uint32_t attrValue) {
  getProperties().align = ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue);
}

void StateConsumeOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align) {
  odsState.addOperands(cursor);
  odsState.getOrAddProperties<Properties>().size = size;
  odsState.getOrAddProperties<Properties>().align = align;
  odsState.addTypes(ptr);
  odsState.addTypes(next_cursor);
}

StateConsumeOp StateConsumeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, ptr, next_cursor, cursor, size, align);
  auto __res__ = ::llvm::dyn_cast<StateConsumeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateConsumeOp StateConsumeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align) {
  return create(builder, builder.getLoc(), ptr, next_cursor, cursor, size, align);
}

void StateConsumeOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align) {
  odsState.addOperands(cursor);
  odsState.getOrAddProperties<Properties>().size = size;
  odsState.getOrAddProperties<Properties>().align = align;
  assert(resultTypes.size() == 2u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

StateConsumeOp StateConsumeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, cursor, size, align);
  auto __res__ = ::llvm::dyn_cast<StateConsumeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateConsumeOp StateConsumeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align) {
  return create(builder, builder.getLoc(), resultTypes, cursor, size, align);
}

void StateConsumeOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, uint32_t size, uint32_t align) {
  odsState.addOperands(cursor);
  odsState.getOrAddProperties<Properties>().size = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), size);
  odsState.getOrAddProperties<Properties>().align = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), align);
  odsState.addTypes(ptr);
  odsState.addTypes(next_cursor);
}

StateConsumeOp StateConsumeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, uint32_t size, uint32_t align) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, ptr, next_cursor, cursor, size, align);
  auto __res__ = ::llvm::dyn_cast<StateConsumeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateConsumeOp StateConsumeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, uint32_t size, uint32_t align) {
  return create(builder, builder.getLoc(), ptr, next_cursor, cursor, size, align);
}

void StateConsumeOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, uint32_t size, uint32_t align) {
  odsState.addOperands(cursor);
  odsState.getOrAddProperties<Properties>().size = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), size);
  odsState.getOrAddProperties<Properties>().align = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), align);
  assert(resultTypes.size() == 2u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

StateConsumeOp StateConsumeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, uint32_t size, uint32_t align) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, cursor, size, align);
  auto __res__ = ::llvm::dyn_cast<StateConsumeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateConsumeOp StateConsumeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, uint32_t size, uint32_t align) {
  return create(builder, builder.getLoc(), resultTypes, cursor, size, align);
}

void StateConsumeOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 2u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<StateConsumeOp::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

StateConsumeOp StateConsumeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<StateConsumeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateConsumeOp StateConsumeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void StateConsumeOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 2u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

StateConsumeOp StateConsumeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<StateConsumeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateConsumeOp StateConsumeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult StateConsumeOp::verifyInvariantsImpl() {
  auto tblgen_align = getProperties().align; (void)tblgen_align;
  if (!tblgen_align) return emitOpError("requires attribute 'align'");
  auto tblgen_size = getProperties().size; (void)tblgen_size;
  if (!tblgen_size) return emitOpError("requires attribute 'size'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(*this, tblgen_size, "size")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(*this, tblgen_align, "align")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps6(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps6(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSResults(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps6(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult StateConsumeOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult StateConsumeOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand cursorRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> cursorOperands(&cursorRawOperand, 1);  ::llvm::SMLoc cursorOperandsLoc;
  (void)cursorOperandsLoc;
  ::llvm::ArrayRef<::mlir::Type> allOperandTypes;
  ::llvm::ArrayRef<::mlir::Type> allResultTypes;

  cursorOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(cursorRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
    if (failed(verifyInherentAttrs(result.name, result.attributes, [&]() {
        return parser.emitError(loc) << "'" << result.name.getStringRef() << "' op ";
      })))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  ::mlir::FunctionType allOperand__allResult_functionType;
  if (parser.parseType(allOperand__allResult_functionType))
    return ::mlir::failure();
  allOperandTypes = allOperand__allResult_functionType.getInputs();
  allResultTypes = allOperand__allResult_functionType.getResults();
  result.addTypes(allResultTypes);
  if (parser.resolveOperands(cursorOperands, allOperandTypes, parser.getNameLoc(), result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void StateConsumeOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getCursor();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  _odsPrinter.printFunctionalType(getOperation()->getOperandTypes(), getOperation()->getResultTypes());
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::StateConsumeOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::StateCursorOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
StateCursorOpGenericAdaptorBase::StateCursorOpGenericAdaptorBase(StateCursorOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

} // namespace detail
StateCursorOpAdaptor::StateCursorOpAdaptor(StateCursorOp op) : StateCursorOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult StateCursorOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

::llvm::LogicalResult StateCursorOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
      return ::mlir::success();
}

::mlir::Attribute StateCursorOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code StateCursorOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine();
}

std::optional<mlir::Attribute> StateCursorOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
  return std::nullopt;
}

void StateCursorOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
}

void StateCursorOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
}

::llvm::LogicalResult StateCursorOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    return ::mlir::success();
}

void StateCursorOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type cursor, ::mlir::Value base) {
  odsState.addOperands(base);
  odsState.addTypes(cursor);
}

StateCursorOp StateCursorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type cursor, ::mlir::Value base) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, cursor, base);
  auto __res__ = ::llvm::dyn_cast<StateCursorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateCursorOp StateCursorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type cursor, ::mlir::Value base) {
  return create(builder, builder.getLoc(), cursor, base);
}

void StateCursorOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value base) {
  odsState.addOperands(base);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

StateCursorOp StateCursorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value base) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, base);
  auto __res__ = ::llvm::dyn_cast<StateCursorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateCursorOp StateCursorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value base) {
  return create(builder, builder.getLoc(), resultTypes, base);
}

void StateCursorOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

StateCursorOp StateCursorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<StateCursorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateCursorOp StateCursorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void StateCursorOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

StateCursorOp StateCursorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<StateCursorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateCursorOp StateCursorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult StateCursorOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps6(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps6(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult StateCursorOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult StateCursorOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::StateCursorOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::StateSlotOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
StateSlotOpGenericAdaptorBase::StateSlotOpGenericAdaptorBase(StateSlotOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

::mlir::Type StateSlotOpGenericAdaptorBase::getElemType() {
  auto attr = getElemTypeAttr();
  return ::llvm::cast<::mlir::Type>(attr.getValue());
}

::llvm::APFloat StateSlotOpGenericAdaptorBase::getInitVal() {
  auto attr = getInitValAttr();
  return attr.getValue();
}

int64_t StateSlotOpGenericAdaptorBase::getByteOffset() {
  auto attr = getByteOffsetAttr();
  return attr.getValue().getSExtValue();
}

} // namespace detail
StateSlotOpAdaptor::StateSlotOpAdaptor(StateSlotOp op) : StateSlotOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult StateSlotOpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_byte_offset = getProperties().byte_offset; (void)tblgen_byte_offset;
  if (!tblgen_byte_offset) return emitError(loc, "'dsp.state_slot' op ""requires attribute 'byte_offset'");
  auto tblgen_elem_type = getProperties().elem_type; (void)tblgen_elem_type;
  if (!tblgen_elem_type) return emitError(loc, "'dsp.state_slot' op ""requires attribute 'elem_type'");
  auto tblgen_init_val = getProperties().init_val; (void)tblgen_init_val;
  if (!tblgen_init_val) return emitError(loc, "'dsp.state_slot' op ""requires attribute 'init_val'");

  if (tblgen_elem_type && !(((::llvm::isa<::mlir::TypeAttr>(tblgen_elem_type))) && ((::llvm::isa<::mlir::Type>(::llvm::cast<::mlir::TypeAttr>(tblgen_elem_type).getValue()))) && ((true))))
    return emitError(loc, "'dsp.state_slot' op ""attribute 'elem_type' failed to satisfy constraint: any type attribute");

  if (tblgen_init_val && !(((::llvm::isa<::mlir::FloatAttr>(tblgen_init_val))) && ((::llvm::cast<::mlir::FloatAttr>(tblgen_init_val).getType().isF64()))))
    return emitError(loc, "'dsp.state_slot' op ""attribute 'init_val' failed to satisfy constraint: 64-bit float attribute");

  if (tblgen_byte_offset && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_byte_offset))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_byte_offset).getType().isSignedInteger(64)))))
    return emitError(loc, "'dsp.state_slot' op ""attribute 'byte_offset' failed to satisfy constraint: 64-bit signed integer attribute");
  return ::mlir::success();
}

::llvm::LogicalResult StateSlotOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.byte_offset;
       auto attr = dict.get("byte_offset");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `byte_offset` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  {
    auto &propStorage = prop.elem_type;
       auto attr = dict.get("elem_type");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `elem_type` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  {
    auto &propStorage = prop.init_val;
       auto attr = dict.get("init_val");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `init_val` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute StateSlotOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.byte_offset;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("byte_offset",
                                       propStorage));
    }

    {
      const auto &propStorage = prop.elem_type;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("elem_type",
                                       propStorage));
    }

    {
      const auto &propStorage = prop.init_val;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("init_val",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code StateSlotOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.byte_offset.getAsOpaquePointer()), 
    llvm::hash_value(prop.elem_type.getAsOpaquePointer()), 
    llvm::hash_value(prop.init_val.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> StateSlotOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "byte_offset")
      return prop.byte_offset;

    if (name == "elem_type")
      return prop.elem_type;

    if (name == "init_val")
      return prop.init_val;
  return std::nullopt;
}

void StateSlotOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "byte_offset") {
       prop.byte_offset = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.byte_offset)>>(value);
       return;
    }

    if (name == "elem_type") {
       prop.elem_type = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.elem_type)>>(value);
       return;
    }

    if (name == "init_val") {
       prop.init_val = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.init_val)>>(value);
       return;
    }
}

void StateSlotOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.byte_offset) attrs.append("byte_offset", prop.byte_offset);

    if (prop.elem_type) attrs.append("elem_type", prop.elem_type);

    if (prop.init_val) attrs.append("init_val", prop.init_val);
}

::llvm::LogicalResult StateSlotOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getByteOffsetAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps4(attr, "byte_offset", emitError)))
        return ::mlir::failure();
    }

    {
      ::mlir::Attribute attr = attrs.get(getElemTypeAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps1(attr, "elem_type", emitError)))
        return ::mlir::failure();
    }

    {
      ::mlir::Attribute attr = attrs.get(getInitValAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps2(attr, "init_val", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult StateSlotOp::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.byte_offset)))
    return ::mlir::failure();

  if (::mlir::failed(reader.readAttribute(prop.elem_type)))
    return ::mlir::failure();

  if (::mlir::failed(reader.readAttribute(prop.init_val)))
    return ::mlir::failure();
  return ::mlir::success();
}

void StateSlotOp::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.byte_offset);
  writer.writeAttribute(prop.elem_type);
  writer.writeAttribute(prop.init_val);
}

::mlir::Type StateSlotOp::getElemType() {
  auto attr = getElemTypeAttr();
  return ::llvm::cast<::mlir::Type>(attr.getValue());
}

::llvm::APFloat StateSlotOp::getInitVal() {
  auto attr = getInitValAttr();
  return attr.getValue();
}

int64_t StateSlotOp::getByteOffset() {
  auto attr = getByteOffsetAttr();
  return attr.getValue().getSExtValue();
}

void StateSlotOp::setElemType(::mlir::Type attrValue) {
  getProperties().elem_type = ::mlir::TypeAttr::get(attrValue);
}

void StateSlotOp::setInitVal(::llvm::APFloat attrValue) {
  getProperties().init_val = ::mlir::Builder((*this)->getContext()).getFloatAttr(::mlir::Builder((*this)->getContext()).getF64Type(), attrValue);
}

void StateSlotOp::setByteOffset(int64_t attrValue) {
  getProperties().byte_offset = ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(64, /*isSigned=*/true), attrValue);
}

void StateSlotOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset) {
  odsState.getOrAddProperties<Properties>().elem_type = elem_type;
  odsState.getOrAddProperties<Properties>().init_val = init_val;
  odsState.getOrAddProperties<Properties>().byte_offset = byte_offset;
  odsState.addTypes(result);
}

StateSlotOp StateSlotOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, elem_type, init_val, byte_offset);
  auto __res__ = ::llvm::dyn_cast<StateSlotOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateSlotOp StateSlotOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset) {
  return create(builder, builder.getLoc(), result, elem_type, init_val, byte_offset);
}

void StateSlotOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset) {
  odsState.getOrAddProperties<Properties>().elem_type = elem_type;
  odsState.getOrAddProperties<Properties>().init_val = init_val;
  odsState.getOrAddProperties<Properties>().byte_offset = byte_offset;
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

StateSlotOp StateSlotOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, elem_type, init_val, byte_offset);
  auto __res__ = ::llvm::dyn_cast<StateSlotOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateSlotOp StateSlotOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset) {
  return create(builder, builder.getLoc(), resultTypes, elem_type, init_val, byte_offset);
}

void StateSlotOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset) {
  odsState.getOrAddProperties<Properties>().elem_type = ::mlir::TypeAttr::get(elem_type);
  odsState.getOrAddProperties<Properties>().init_val = odsBuilder.getFloatAttr(odsBuilder.getF64Type(), init_val);
  odsState.getOrAddProperties<Properties>().byte_offset = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(64, /*isSigned=*/true), byte_offset);
  odsState.addTypes(result);
}

StateSlotOp StateSlotOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, elem_type, init_val, byte_offset);
  auto __res__ = ::llvm::dyn_cast<StateSlotOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateSlotOp StateSlotOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset) {
  return create(builder, builder.getLoc(), result, elem_type, init_val, byte_offset);
}

void StateSlotOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset) {
  odsState.getOrAddProperties<Properties>().elem_type = ::mlir::TypeAttr::get(elem_type);
  odsState.getOrAddProperties<Properties>().init_val = odsBuilder.getFloatAttr(odsBuilder.getF64Type(), init_val);
  odsState.getOrAddProperties<Properties>().byte_offset = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(64, /*isSigned=*/true), byte_offset);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

StateSlotOp StateSlotOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, elem_type, init_val, byte_offset);
  auto __res__ = ::llvm::dyn_cast<StateSlotOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateSlotOp StateSlotOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset) {
  return create(builder, builder.getLoc(), resultTypes, elem_type, init_val, byte_offset);
}

void StateSlotOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 0u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<StateSlotOp::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

StateSlotOp StateSlotOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<StateSlotOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateSlotOp StateSlotOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void StateSlotOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 0u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

StateSlotOp StateSlotOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<StateSlotOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StateSlotOp StateSlotOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult StateSlotOp::verifyInvariantsImpl() {
  auto tblgen_byte_offset = getProperties().byte_offset; (void)tblgen_byte_offset;
  if (!tblgen_byte_offset) return emitOpError("requires attribute 'byte_offset'");
  auto tblgen_elem_type = getProperties().elem_type; (void)tblgen_elem_type;
  if (!tblgen_elem_type) return emitOpError("requires attribute 'elem_type'");
  auto tblgen_init_val = getProperties().init_val; (void)tblgen_init_val;
  if (!tblgen_init_val) return emitOpError("requires attribute 'init_val'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps1(*this, tblgen_elem_type, "elem_type")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps2(*this, tblgen_init_val, "init_val")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps4(*this, tblgen_byte_offset, "byte_offset")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps6(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult StateSlotOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult StateSlotOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;
  {

    auto &propStorage = prop.elem_type;
    auto elem_typeAttrName = ::mlir::StringAttr::get(ctx, "elem_type");
    auto attr = dict.get(elem_typeAttrName);
    usedKeys.insert(elem_typeAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for elem_type in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `elem_type` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  {

    auto &propStorage = prop.init_val;
    auto init_valAttrName = ::mlir::StringAttr::get(ctx, "init_val");
    auto attr = dict.get(init_valAttrName);
    usedKeys.insert(init_valAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for init_val in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `init_val` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  {

    auto &propStorage = prop.byte_offset;
    auto byte_offsetAttrName = ::mlir::StringAttr::get(ctx, "byte_offset");
    auto attr = dict.get(byte_offsetAttrName);
    usedKeys.insert(byte_offsetAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for byte_offset in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `byte_offset` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::StateSlotOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::SubSynthOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
SubSynthOpGenericAdaptorBase::SubSynthOpGenericAdaptorBase(SubSynthOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

std::pair<unsigned, unsigned> SubSynthOpGenericAdaptorBase::getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (odsOperandsSize - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::llvm::StringRef SubSynthOpGenericAdaptorBase::getSynthName() {
  auto attr = getSynthNameAttr();
  return attr.getValue();
}

uint32_t SubSynthOpGenericAdaptorBase::getStateBytes() {
  auto attr = getStateBytesAttr();
  return attr.getValue().getZExtValue();
}

uint32_t SubSynthOpGenericAdaptorBase::getNumInputs() {
  auto attr = getNumInputsAttr();
  return attr.getValue().getZExtValue();
}

} // namespace detail
SubSynthOpAdaptor::SubSynthOpAdaptor(SubSynthOp op) : SubSynthOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult SubSynthOpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_num_inputs = getProperties().num_inputs; (void)tblgen_num_inputs;
  if (!tblgen_num_inputs) return emitError(loc, "'dsp.subsynth' op ""requires attribute 'num_inputs'");
  auto tblgen_state_bytes = getProperties().state_bytes; (void)tblgen_state_bytes;
  if (!tblgen_state_bytes) return emitError(loc, "'dsp.subsynth' op ""requires attribute 'state_bytes'");
  auto tblgen_synth_name = getProperties().synth_name; (void)tblgen_synth_name;
  if (!tblgen_synth_name) return emitError(loc, "'dsp.subsynth' op ""requires attribute 'synth_name'");

  if (tblgen_synth_name && !((::llvm::isa<::mlir::StringAttr>(tblgen_synth_name))))
    return emitError(loc, "'dsp.subsynth' op ""attribute 'synth_name' failed to satisfy constraint: string attribute");

  if (tblgen_state_bytes && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_state_bytes))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_state_bytes).getType().isSignlessInteger(32)))))
    return emitError(loc, "'dsp.subsynth' op ""attribute 'state_bytes' failed to satisfy constraint: 32-bit signless integer attribute");

  if (tblgen_num_inputs && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_num_inputs))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_num_inputs).getType().isSignlessInteger(32)))))
    return emitError(loc, "'dsp.subsynth' op ""attribute 'num_inputs' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

std::pair<unsigned, unsigned> SubSynthOp::getODSOperandIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumOperands() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::MutableOperandRange SubSynthOp::getInputsMutable() {
  auto range = getODSOperandIndexAndLength(0);
  auto mutableRange = ::mlir::MutableOperandRange(getOperation(), range.first, range.second);
  return mutableRange;
}

::llvm::LogicalResult SubSynthOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.num_inputs;
       auto attr = dict.get("num_inputs");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `num_inputs` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  {
    auto &propStorage = prop.state_bytes;
       auto attr = dict.get("state_bytes");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `state_bytes` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  {
    auto &propStorage = prop.synth_name;
       auto attr = dict.get("synth_name");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `synth_name` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute SubSynthOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.num_inputs;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("num_inputs",
                                       propStorage));
    }

    {
      const auto &propStorage = prop.state_bytes;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("state_bytes",
                                       propStorage));
    }

    {
      const auto &propStorage = prop.synth_name;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("synth_name",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code SubSynthOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.num_inputs.getAsOpaquePointer()), 
    llvm::hash_value(prop.state_bytes.getAsOpaquePointer()), 
    llvm::hash_value(prop.synth_name.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> SubSynthOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "num_inputs")
      return prop.num_inputs;

    if (name == "state_bytes")
      return prop.state_bytes;

    if (name == "synth_name")
      return prop.synth_name;
  return std::nullopt;
}

void SubSynthOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "num_inputs") {
       prop.num_inputs = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.num_inputs)>>(value);
       return;
    }

    if (name == "state_bytes") {
       prop.state_bytes = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.state_bytes)>>(value);
       return;
    }

    if (name == "synth_name") {
       prop.synth_name = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.synth_name)>>(value);
       return;
    }
}

void SubSynthOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.num_inputs) attrs.append("num_inputs", prop.num_inputs);

    if (prop.state_bytes) attrs.append("state_bytes", prop.state_bytes);

    if (prop.synth_name) attrs.append("synth_name", prop.synth_name);
}

::llvm::LogicalResult SubSynthOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getNumInputsAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(attr, "num_inputs", emitError)))
        return ::mlir::failure();
    }

    {
      ::mlir::Attribute attr = attrs.get(getStateBytesAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(attr, "state_bytes", emitError)))
        return ::mlir::failure();
    }

    {
      ::mlir::Attribute attr = attrs.get(getSynthNameAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps5(attr, "synth_name", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult SubSynthOp::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.num_inputs)))
    return ::mlir::failure();

  if (::mlir::failed(reader.readAttribute(prop.state_bytes)))
    return ::mlir::failure();

  if (::mlir::failed(reader.readAttribute(prop.synth_name)))
    return ::mlir::failure();
  return ::mlir::success();
}

void SubSynthOp::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.num_inputs);
  writer.writeAttribute(prop.state_bytes);
  writer.writeAttribute(prop.synth_name);
}

::llvm::StringRef SubSynthOp::getSynthName() {
  auto attr = getSynthNameAttr();
  return attr.getValue();
}

uint32_t SubSynthOp::getStateBytes() {
  auto attr = getStateBytesAttr();
  return attr.getValue().getZExtValue();
}

uint32_t SubSynthOp::getNumInputs() {
  auto attr = getNumInputsAttr();
  return attr.getValue().getZExtValue();
}

void SubSynthOp::setSynthName(::llvm::StringRef attrValue) {
  getProperties().synth_name = ::mlir::Builder((*this)->getContext()).getStringAttr(attrValue);
}

void SubSynthOp::setStateBytes(uint32_t attrValue) {
  getProperties().state_bytes = ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue);
}

void SubSynthOp::setNumInputs(uint32_t attrValue) {
  getProperties().num_inputs = ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue);
}

void SubSynthOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs) {
  odsState.addOperands(inputs);
  odsState.getOrAddProperties<Properties>().synth_name = synth_name;
  odsState.getOrAddProperties<Properties>().state_bytes = state_bytes;
  odsState.getOrAddProperties<Properties>().num_inputs = num_inputs;
  odsState.addTypes(result);
}

SubSynthOp SubSynthOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, inputs, synth_name, state_bytes, num_inputs);
  auto __res__ = ::llvm::dyn_cast<SubSynthOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SubSynthOp SubSynthOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs) {
  return create(builder, builder.getLoc(), result, inputs, synth_name, state_bytes, num_inputs);
}

void SubSynthOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs) {
  odsState.addOperands(inputs);
  odsState.getOrAddProperties<Properties>().synth_name = synth_name;
  odsState.getOrAddProperties<Properties>().state_bytes = state_bytes;
  odsState.getOrAddProperties<Properties>().num_inputs = num_inputs;
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

SubSynthOp SubSynthOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, inputs, synth_name, state_bytes, num_inputs);
  auto __res__ = ::llvm::dyn_cast<SubSynthOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SubSynthOp SubSynthOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs) {
  return create(builder, builder.getLoc(), resultTypes, inputs, synth_name, state_bytes, num_inputs);
}

void SubSynthOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs) {
  odsState.addOperands(inputs);
  odsState.getOrAddProperties<Properties>().synth_name = odsBuilder.getStringAttr(synth_name);
  odsState.getOrAddProperties<Properties>().state_bytes = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), state_bytes);
  odsState.getOrAddProperties<Properties>().num_inputs = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), num_inputs);
  odsState.addTypes(result);
}

SubSynthOp SubSynthOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, inputs, synth_name, state_bytes, num_inputs);
  auto __res__ = ::llvm::dyn_cast<SubSynthOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SubSynthOp SubSynthOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs) {
  return create(builder, builder.getLoc(), result, inputs, synth_name, state_bytes, num_inputs);
}

void SubSynthOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs) {
  odsState.addOperands(inputs);
  odsState.getOrAddProperties<Properties>().synth_name = odsBuilder.getStringAttr(synth_name);
  odsState.getOrAddProperties<Properties>().state_bytes = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), state_bytes);
  odsState.getOrAddProperties<Properties>().num_inputs = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), num_inputs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

SubSynthOp SubSynthOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, inputs, synth_name, state_bytes, num_inputs);
  auto __res__ = ::llvm::dyn_cast<SubSynthOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SubSynthOp SubSynthOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs) {
  return create(builder, builder.getLoc(), resultTypes, inputs, synth_name, state_bytes, num_inputs);
}

void SubSynthOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<SubSynthOp::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

SubSynthOp SubSynthOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<SubSynthOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SubSynthOp SubSynthOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void SubSynthOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

SubSynthOp SubSynthOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<SubSynthOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SubSynthOp SubSynthOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult SubSynthOp::verifyInvariantsImpl() {
  auto tblgen_num_inputs = getProperties().num_inputs; (void)tblgen_num_inputs;
  if (!tblgen_num_inputs) return emitOpError("requires attribute 'num_inputs'");
  auto tblgen_state_bytes = getProperties().state_bytes; (void)tblgen_state_bytes;
  if (!tblgen_state_bytes) return emitOpError("requires attribute 'state_bytes'");
  auto tblgen_synth_name = getProperties().synth_name; (void)tblgen_synth_name;
  if (!tblgen_synth_name) return emitOpError("requires attribute 'synth_name'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps5(*this, tblgen_synth_name, "synth_name")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(*this, tblgen_state_bytes, "state_bytes")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(*this, tblgen_num_inputs, "num_inputs")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps7(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult SubSynthOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult SubSynthOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;
  {

    auto &propStorage = prop.synth_name;
    auto synth_nameAttrName = ::mlir::StringAttr::get(ctx, "synth_name");
    auto attr = dict.get(synth_nameAttrName);
    usedKeys.insert(synth_nameAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for synth_name in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `synth_name` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  {

    auto &propStorage = prop.state_bytes;
    auto state_bytesAttrName = ::mlir::StringAttr::get(ctx, "state_bytes");
    auto attr = dict.get(state_bytesAttrName);
    usedKeys.insert(state_bytesAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for state_bytes in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `state_bytes` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  {

    auto &propStorage = prop.num_inputs;
    auto num_inputsAttrName = ::mlir::StringAttr::get(ctx, "num_inputs");
    auto attr = dict.get(num_inputsAttrName);
    usedKeys.insert(num_inputsAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for num_inputs in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `num_inputs` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::SubSynthOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::Tabread2Op definitions
//===----------------------------------------------------------------------===//

namespace detail {
Tabread2OpGenericAdaptorBase::Tabread2OpGenericAdaptorBase(Tabread2Op op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

::llvm::StringRef Tabread2OpGenericAdaptorBase::getTableSymbol() {
  auto attr = getTableSymbolAttr();
  return attr.getValue();
}

uint32_t Tabread2OpGenericAdaptorBase::getTableSize() {
  auto attr = getTableSizeAttr();
  return attr.getValue().getZExtValue();
}

} // namespace detail
Tabread2OpAdaptor::Tabread2OpAdaptor(Tabread2Op op) : Tabread2OpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult Tabread2OpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_table_size = getProperties().table_size; (void)tblgen_table_size;
  if (!tblgen_table_size) return emitError(loc, "'dsp.tabread2' op ""requires attribute 'table_size'");
  auto tblgen_table_symbol = getProperties().table_symbol; (void)tblgen_table_symbol;
  if (!tblgen_table_symbol) return emitError(loc, "'dsp.tabread2' op ""requires attribute 'table_symbol'");

  if (tblgen_table_symbol && !((::llvm::isa<::mlir::StringAttr>(tblgen_table_symbol))))
    return emitError(loc, "'dsp.tabread2' op ""attribute 'table_symbol' failed to satisfy constraint: string attribute");

  if (tblgen_table_size && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_table_size))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_table_size).getType().isSignlessInteger(32)))))
    return emitError(loc, "'dsp.tabread2' op ""attribute 'table_size' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

::llvm::LogicalResult Tabread2Op::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.table_size;
       auto attr = dict.get("table_size");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `table_size` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  {
    auto &propStorage = prop.table_symbol;
       auto attr = dict.get("table_symbol");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `table_symbol` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute Tabread2Op::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.table_size;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("table_size",
                                       propStorage));
    }

    {
      const auto &propStorage = prop.table_symbol;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("table_symbol",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code Tabread2Op::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.table_size.getAsOpaquePointer()), 
    llvm::hash_value(prop.table_symbol.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> Tabread2Op::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "table_size")
      return prop.table_size;

    if (name == "table_symbol")
      return prop.table_symbol;
  return std::nullopt;
}

void Tabread2Op::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "table_size") {
       prop.table_size = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.table_size)>>(value);
       return;
    }

    if (name == "table_symbol") {
       prop.table_symbol = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.table_symbol)>>(value);
       return;
    }
}

void Tabread2Op::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.table_size) attrs.append("table_size", prop.table_size);

    if (prop.table_symbol) attrs.append("table_symbol", prop.table_symbol);
}

::llvm::LogicalResult Tabread2Op::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getTableSizeAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(attr, "table_size", emitError)))
        return ::mlir::failure();
    }

    {
      ::mlir::Attribute attr = attrs.get(getTableSymbolAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps5(attr, "table_symbol", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult Tabread2Op::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.table_size)))
    return ::mlir::failure();

  if (::mlir::failed(reader.readAttribute(prop.table_symbol)))
    return ::mlir::failure();
  return ::mlir::success();
}

void Tabread2Op::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.table_size);
  writer.writeAttribute(prop.table_symbol);
}

::llvm::StringRef Tabread2Op::getTableSymbol() {
  auto attr = getTableSymbolAttr();
  return attr.getValue();
}

uint32_t Tabread2Op::getTableSize() {
  auto attr = getTableSizeAttr();
  return attr.getValue().getZExtValue();
}

void Tabread2Op::setTableSymbol(::llvm::StringRef attrValue) {
  getProperties().table_symbol = ::mlir::Builder((*this)->getContext()).getStringAttr(attrValue);
}

void Tabread2Op::setTableSize(uint32_t attrValue) {
  getProperties().table_size = ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue);
}

void Tabread2Op::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size) {
  odsState.addOperands(phase);
  odsState.getOrAddProperties<Properties>().table_symbol = table_symbol;
  odsState.getOrAddProperties<Properties>().table_size = table_size;
  odsState.addTypes(result);
}

Tabread2Op Tabread2Op::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, phase, table_symbol, table_size);
  auto __res__ = ::llvm::dyn_cast<Tabread2Op>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

Tabread2Op Tabread2Op::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size) {
  return create(builder, builder.getLoc(), result, phase, table_symbol, table_size);
}

void Tabread2Op::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size) {
  odsState.addOperands(phase);
  odsState.getOrAddProperties<Properties>().table_symbol = table_symbol;
  odsState.getOrAddProperties<Properties>().table_size = table_size;
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

Tabread2Op Tabread2Op::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, phase, table_symbol, table_size);
  auto __res__ = ::llvm::dyn_cast<Tabread2Op>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

Tabread2Op Tabread2Op::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size) {
  return create(builder, builder.getLoc(), resultTypes, phase, table_symbol, table_size);
}

void Tabread2Op::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size) {
  odsState.addOperands(phase);
  odsState.getOrAddProperties<Properties>().table_symbol = odsBuilder.getStringAttr(table_symbol);
  odsState.getOrAddProperties<Properties>().table_size = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), table_size);
  odsState.addTypes(result);
}

Tabread2Op Tabread2Op::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, phase, table_symbol, table_size);
  auto __res__ = ::llvm::dyn_cast<Tabread2Op>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

Tabread2Op Tabread2Op::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size) {
  return create(builder, builder.getLoc(), result, phase, table_symbol, table_size);
}

void Tabread2Op::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size) {
  odsState.addOperands(phase);
  odsState.getOrAddProperties<Properties>().table_symbol = odsBuilder.getStringAttr(table_symbol);
  odsState.getOrAddProperties<Properties>().table_size = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), table_size);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

Tabread2Op Tabread2Op::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, phase, table_symbol, table_size);
  auto __res__ = ::llvm::dyn_cast<Tabread2Op>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

Tabread2Op Tabread2Op::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size) {
  return create(builder, builder.getLoc(), resultTypes, phase, table_symbol, table_size);
}

void Tabread2Op::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<Tabread2Op::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

Tabread2Op Tabread2Op::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<Tabread2Op>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

Tabread2Op Tabread2Op::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void Tabread2Op::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

Tabread2Op Tabread2Op::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<Tabread2Op>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

Tabread2Op Tabread2Op::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult Tabread2Op::verifyInvariantsImpl() {
  auto tblgen_table_size = getProperties().table_size; (void)tblgen_table_size;
  if (!tblgen_table_size) return emitOpError("requires attribute 'table_size'");
  auto tblgen_table_symbol = getProperties().table_symbol; (void)tblgen_table_symbol;
  if (!tblgen_table_symbol) return emitOpError("requires attribute 'table_symbol'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps5(*this, tblgen_table_symbol, "table_symbol")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(*this, tblgen_table_size, "table_size")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult Tabread2Op::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult Tabread2Op::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;
  {

    auto &propStorage = prop.table_symbol;
    auto table_symbolAttrName = ::mlir::StringAttr::get(ctx, "table_symbol");
    auto attr = dict.get(table_symbolAttrName);
    usedKeys.insert(table_symbolAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for table_symbol in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `table_symbol` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  {

    auto &propStorage = prop.table_size;
    auto table_sizeAttrName = ::mlir::StringAttr::get(ctx, "table_size");
    auto attr = dict.get(table_sizeAttrName);
    usedKeys.insert(table_sizeAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for table_size in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `table_size` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::Tabread2Op)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::TabreadOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
TabreadOpGenericAdaptorBase::TabreadOpGenericAdaptorBase(TabreadOp op) : odsAttrs(op->getRawDictionaryAttrs()), odsOpName(op->getName()), properties(op.getProperties()), odsRegions(op->getRegions()) {}

uint32_t TabreadOpGenericAdaptorBase::getTableSize() {
  auto attr = getTableSizeAttr();
  return attr.getValue().getZExtValue();
}

} // namespace detail
TabreadOpAdaptor::TabreadOpAdaptor(TabreadOp op) : TabreadOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult TabreadOpAdaptor::verify(::mlir::Location loc) {
  auto tblgen_table_size = getProperties().table_size; (void)tblgen_table_size;
  if (!tblgen_table_size) return emitError(loc, "'dsp.tabread' op ""requires attribute 'table_size'");

  if (tblgen_table_size && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_table_size))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_table_size).getType().isSignlessInteger(32)))))
    return emitError(loc, "'dsp.tabread' op ""attribute 'table_size' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

::llvm::LogicalResult TabreadOp::setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }

  {
    auto &propStorage = prop.table_size;
       auto attr = dict.get("table_size");
    if (attr) {
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `table_size` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }
  return ::mlir::success();
}

::mlir::Attribute TabreadOp::getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop) {
    ::mlir::SmallVector<::mlir::NamedAttribute> attrs;
    ::mlir::Builder odsBuilder{ctx};

    {
      const auto &propStorage = prop.table_size;
      if (propStorage)
        attrs.push_back(odsBuilder.getNamedAttr("table_size",
                                       propStorage));
    }

  if (!attrs.empty())
    return odsBuilder.getDictionaryAttr(attrs);
  return {};
}

llvm::hash_code TabreadOp::computePropertiesHash(const Properties &prop) {
  using llvm::hash_value;
  return llvm::hash_combine(
    llvm::hash_value(prop.table_size.getAsOpaquePointer()));
}

std::optional<mlir::Attribute> TabreadOp::getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name) {
    if (name == "table_size")
      return prop.table_size;
  return std::nullopt;
}

void TabreadOp::setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value) {
    if (name == "table_size") {
       prop.table_size = ::llvm::dyn_cast_or_null<std::remove_reference_t<decltype(prop.table_size)>>(value);
       return;
    }
}

void TabreadOp::populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs) {
    if (prop.table_size) attrs.append("table_size", prop.table_size);
}

::llvm::LogicalResult TabreadOp::verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
    {
      ::mlir::Attribute attr = attrs.get(getTableSizeAttrName(opName));
      if (attr && ::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(attr, "table_size", emitError)))
        return ::mlir::failure();
    }
    return ::mlir::success();
}

::llvm::LogicalResult TabreadOp::readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state) {
  auto &prop = state.getOrAddProperties<Properties>(); (void)prop;
  if (::mlir::failed(reader.readAttribute(prop.table_size)))
    return ::mlir::failure();
  return ::mlir::success();
}

void TabreadOp::writeProperties(::mlir::DialectBytecodeWriter &writer) {
  auto &prop = getProperties(); (void)prop;
  writer.writeAttribute(prop.table_size);
}

uint32_t TabreadOp::getTableSize() {
  auto attr = getTableSizeAttr();
  return attr.getValue().getZExtValue();
}

void TabreadOp::setTableSize(uint32_t attrValue) {
  getProperties().table_size = ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue);
}

void TabreadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size) {
  odsState.addOperands(phase);
  odsState.addOperands(table_ptr);
  odsState.getOrAddProperties<Properties>().table_size = table_size;
  odsState.addTypes(result);
}

TabreadOp TabreadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, phase, table_ptr, table_size);
  auto __res__ = ::llvm::dyn_cast<TabreadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

TabreadOp TabreadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size) {
  return create(builder, builder.getLoc(), result, phase, table_ptr, table_size);
}

void TabreadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size) {
  odsState.addOperands(phase);
  odsState.addOperands(table_ptr);
  odsState.getOrAddProperties<Properties>().table_size = table_size;
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

TabreadOp TabreadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, phase, table_ptr, table_size);
  auto __res__ = ::llvm::dyn_cast<TabreadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

TabreadOp TabreadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size) {
  return create(builder, builder.getLoc(), resultTypes, phase, table_ptr, table_size);
}

void TabreadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size) {
  odsState.addOperands(phase);
  odsState.addOperands(table_ptr);
  odsState.getOrAddProperties<Properties>().table_size = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), table_size);
  odsState.addTypes(result);
}

TabreadOp TabreadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, result, phase, table_ptr, table_size);
  auto __res__ = ::llvm::dyn_cast<TabreadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

TabreadOp TabreadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size) {
  return create(builder, builder.getLoc(), result, phase, table_ptr, table_size);
}

void TabreadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size) {
  odsState.addOperands(phase);
  odsState.addOperands(table_ptr);
  odsState.getOrAddProperties<Properties>().table_size = odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), table_size);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

TabreadOp TabreadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, phase, table_ptr, table_size);
  auto __res__ = ::llvm::dyn_cast<TabreadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

TabreadOp TabreadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size) {
  return create(builder, builder.getLoc(), resultTypes, phase, table_ptr, table_size);
}

void TabreadOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);

  if (!attributes.empty()) {
    ::mlir::OpaqueProperties properties =
      &odsState.getOrAddProperties<TabreadOp::Properties>();
    std::optional<::mlir::RegisteredOperationName> info =
      odsState.name.getRegisteredInfo();
    if (failed(info->setOpPropertiesFromAttribute(odsState.name, properties,
        odsState.attributes.getDictionary(odsState.getContext()), nullptr)))
      ::llvm::report_fatal_error("Property conversion failed.");
  }
}

TabreadOp TabreadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, attributes);
  auto __res__ = ::llvm::dyn_cast<TabreadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

TabreadOp TabreadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, attributes);
}

void TabreadOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.useProperties(const_cast<Properties&>(properties));
  odsState.addAttributes(discardableAttributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

TabreadOp TabreadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, resultTypes, operands, properties, discardableAttributes);
  auto __res__ = ::llvm::dyn_cast<TabreadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

TabreadOp TabreadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes) {
  return create(builder, builder.getLoc(), resultTypes, operands, properties, discardableAttributes);
}

::llvm::LogicalResult TabreadOp::verifyInvariantsImpl() {
  auto tblgen_table_size = getProperties().table_size; (void)tblgen_table_size;
  if (!tblgen_table_size) return emitOpError("requires attribute 'table_size'");

  if (::mlir::failed(__mlir_ods_local_attr_constraint_DspOps3(*this, tblgen_table_size, "table_size")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps6(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_DspOps5(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult TabreadOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::llvm::LogicalResult TabreadOp::setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  ::mlir::DictionaryAttr dict = ::llvm::dyn_cast<::mlir::DictionaryAttr>(attr);
  if (!dict) {
    emitError() << "expected DictionaryAttr to set properties";
    return ::mlir::failure();
  }
  // keep track of used keys in the input dictionary to be able to error out
  // if there are some unknown ones.
  ::mlir::DenseSet<::mlir::StringAttr> usedKeys;
  ::mlir::MLIRContext *ctx = dict.getContext();
  (void)ctx;
  {

    auto &propStorage = prop.table_size;
    auto table_sizeAttrName = ::mlir::StringAttr::get(ctx, "table_size");
    auto attr = dict.get(table_sizeAttrName);
    usedKeys.insert(table_sizeAttrName);
    if (attr || /*isRequired=*/true) {
      if (!attr) {
        emitError() << "expected key entry for table_size in DictionaryAttr to set "
                   "Properties.";
        return ::mlir::failure();
      }
      auto convertedAttr = ::llvm::dyn_cast<std::remove_reference_t<decltype(propStorage)>>(attr);
      if (convertedAttr) {
        propStorage = convertedAttr;
      } else {
        emitError() << "Invalid attribute `table_size` in property conversion: " << attr;
        return ::mlir::failure();
      }
    }
  }

  for (::mlir::NamedAttribute attr : dict) {
    if (!usedKeys.contains(attr.getName()))
      return emitError() << "unknown key '" << attr.getName() <<
          "' when parsing properties dictionary";
  }
  return ::mlir::success();
}

} // namespace dsp
MLIR_DEFINE_EXPLICIT_TYPE_ID(dsp::TabreadOp)


#endif  // GET_OP_CLASSES

