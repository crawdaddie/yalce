/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Op Declarations                                                            *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: DspOps.td                                                            *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

namespace dsp {
class ArrayAtOp;
} // namespace dsp
namespace dsp {
class ArrayOp;
} // namespace dsp
namespace dsp {
class ArraySetOp;
} // namespace dsp
namespace dsp {
class ArrayStatefulOp;
} // namespace dsp
namespace dsp {
class PhasorOp;
} // namespace dsp
namespace dsp {
class PhasorStatefulOp;
} // namespace dsp
namespace dsp {
class StateConsumeOp;
} // namespace dsp
namespace dsp {
class StateCursorOp;
} // namespace dsp
namespace dsp {
class StateSlotOp;
} // namespace dsp
namespace dsp {
class SubSynthOp;
} // namespace dsp
namespace dsp {
class Tabread2Op;
} // namespace dsp
namespace dsp {
class TabreadOp;
} // namespace dsp
#ifdef GET_OP_CLASSES
#undef GET_OP_CLASSES

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::ArrayAtOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class ArrayAtOpGenericAdaptorBase {
public:
  using Properties = ::mlir::EmptyProperties;
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  ArrayAtOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.array_at", odsAttrs.getContext());
  }

  ArrayAtOpGenericAdaptorBase(ArrayAtOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
    return {index, 1};
  }

  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

};
} // namespace detail
template <typename RangeT>
class ArrayAtOpGenericAdaptor : public detail::ArrayAtOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::ArrayAtOpGenericAdaptorBase;
  static constexpr int odsIndex_array = 0;
  static constexpr int odsIndex_index = 1;
public:
  ArrayAtOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  ArrayAtOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : ArrayAtOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  ArrayAtOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : ArrayAtOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  ArrayAtOpGenericAdaptor(RangeT values, const ArrayAtOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = ArrayAtOp, typename = std::enable_if_t<std::is_same_v<LateInst, ArrayAtOp>>>
  ArrayAtOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  ValueT getArray() {
    return (*getODSOperands(0).begin());
  }

  ValueT getIndex() {
    return (*getODSOperands(1).begin());
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class ArrayAtOpAdaptor : public ArrayAtOpGenericAdaptor<::mlir::ValueRange> {
public:
  using ArrayAtOpGenericAdaptor::ArrayAtOpGenericAdaptor;
  ArrayAtOpAdaptor(ArrayAtOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class ArrayAtOp : public ::mlir::Op<ArrayAtOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::Type>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::NOperands<2>::Impl, ::mlir::OpTrait::OpInvariants> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = ArrayAtOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = ArrayAtOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_array = 0;
  static constexpr int odsIndex_index = 1;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    return {};
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.array_at");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::Type> getArray() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::Type>>(*getODSOperands(0).begin());
  }

  ::mlir::TypedValue<::mlir::IntegerType> getIndex() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::IntegerType>>(*getODSOperands(1).begin());
  }

  ::mlir::OpOperand &getArrayMutable() {
    auto range = getODSOperandIndexAndLength(0);
    return getOperation()->getOpOperand(range.first);
  }

  ::mlir::OpOperand &getIndexMutable() {
    auto range = getODSOperandIndexAndLength(1);
    return getOperation()->getOpOperand(range.first);
  }

  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::Type> getResult() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::Type>>(*getODSResults(0).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value array, ::mlir::Value index);
  static ArrayAtOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value array, ::mlir::Value index);
  static ArrayAtOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value array, ::mlir::Value index);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index);
  static ArrayAtOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index);
  static ArrayAtOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static ArrayAtOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static ArrayAtOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static ArrayAtOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static ArrayAtOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
public:
  static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                    ::mlir::Value array, ::mlir::Value index);
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::ArrayAtOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::ArrayOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class ArrayOpGenericAdaptorBase {
public:
  struct Properties {
    using elem_typeTy = ::mlir::TypeAttr;
    elem_typeTy elem_type;

    auto getElemType() const {
      auto &propStorage = this->elem_type;
      return ::llvm::cast<::mlir::TypeAttr>(propStorage);
    }
    void setElemType(const ::mlir::TypeAttr &propValue) {
      this->elem_type = propValue;
    }
    bool operator==(const Properties &rhs) const {
      return 
        rhs.elem_type == this->elem_type &&
        true;
    }
    bool operator!=(const Properties &rhs) const {
      return !(*this == rhs);
    }
  };
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  ArrayOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.array", odsAttrs.getContext());
  }

  ArrayOpGenericAdaptorBase(ArrayOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize);
  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

  ::mlir::TypeAttr getElemTypeAttr() {
    auto attr = ::llvm::cast<::mlir::TypeAttr>(getProperties().elem_type);
    return attr;
  }

  ::mlir::Type getElemType();
};
} // namespace detail
template <typename RangeT>
class ArrayOpGenericAdaptor : public detail::ArrayOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::ArrayOpGenericAdaptorBase;
  static constexpr int odsIndex_values = 0;
public:
  ArrayOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  ArrayOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : ArrayOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  ArrayOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : ArrayOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  ArrayOpGenericAdaptor(RangeT values, const ArrayOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = ArrayOp, typename = std::enable_if_t<std::is_same_v<LateInst, ArrayOp>>>
  ArrayOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  RangeT getValues() {
    return getODSOperands(0);
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class ArrayOpAdaptor : public ArrayOpGenericAdaptor<::mlir::ValueRange> {
public:
  using ArrayOpGenericAdaptor::ArrayOpGenericAdaptor;
  ArrayOpAdaptor(ArrayOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class ArrayOp : public ::mlir::Op<ArrayOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::Type>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = ArrayOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = ArrayOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_values = 0;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    static ::llvm::StringRef attrNames[] = {::llvm::StringRef("elem_type")};
    return ::llvm::ArrayRef(attrNames);
  }

  ::mlir::StringAttr getElemTypeAttrName() {
    return getAttributeNameForIndex(0);
  }

  static ::mlir::StringAttr getElemTypeAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 0);
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.array");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index);
  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::Operation::operand_range getValues() {
    return getODSOperands(0);
  }

  ::mlir::MutableOperandRange getValuesMutable();
  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::Type> getResult() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::Type>>(*getODSResults(0).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::llvm::LogicalResult readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state);
  void writeProperties(::mlir::DialectBytecodeWriter &writer);
  ::mlir::TypeAttr getElemTypeAttr() {
    return ::llvm::cast<::mlir::TypeAttr>(getProperties().elem_type);
  }

  ::mlir::Type getElemType();
  void setElemTypeAttr(::mlir::TypeAttr attr) {
    getProperties().elem_type = attr;
  }

  void setElemType(::mlir::Type attrValue);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static ArrayOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static ArrayOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static ArrayOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static ArrayOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static ArrayOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static ArrayOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static ArrayOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static ArrayOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static ArrayOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static ArrayOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static ArrayOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static ArrayOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
private:
  ::mlir::StringAttr getAttributeNameForIndex(unsigned index) {
    return getAttributeNameForIndex((*this)->getName(), index);
  }

  static ::mlir::StringAttr getAttributeNameForIndex(::mlir::OperationName name, unsigned index) {
    assert(index < 1 && "invalid attribute index");
    assert(name.getStringRef() == getOperationName() && "invalid operation name");
    assert(name.isRegistered() && "Operation isn't registered, missing a "
          "dependent dialect loading?");
    return name.getAttributeNames()[index];
  }

public:
  static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                    ::mlir::Type elem_type, ::mlir::ValueRange values);
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::ArrayOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::ArraySetOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class ArraySetOpGenericAdaptorBase {
public:
  using Properties = ::mlir::EmptyProperties;
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  ArraySetOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.array_set", odsAttrs.getContext());
  }

  ArraySetOpGenericAdaptorBase(ArraySetOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
    return {index, 1};
  }

  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

};
} // namespace detail
template <typename RangeT>
class ArraySetOpGenericAdaptor : public detail::ArraySetOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::ArraySetOpGenericAdaptorBase;
  static constexpr int odsIndex_array = 0;
  static constexpr int odsIndex_index = 1;
  static constexpr int odsIndex_value = 2;
public:
  ArraySetOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  ArraySetOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : ArraySetOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  ArraySetOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : ArraySetOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  ArraySetOpGenericAdaptor(RangeT values, const ArraySetOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = ArraySetOp, typename = std::enable_if_t<std::is_same_v<LateInst, ArraySetOp>>>
  ArraySetOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  ValueT getArray() {
    return (*getODSOperands(0).begin());
  }

  ValueT getIndex() {
    return (*getODSOperands(1).begin());
  }

  ValueT getValue() {
    return (*getODSOperands(2).begin());
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class ArraySetOpAdaptor : public ArraySetOpGenericAdaptor<::mlir::ValueRange> {
public:
  using ArraySetOpGenericAdaptor::ArraySetOpGenericAdaptor;
  ArraySetOpAdaptor(ArraySetOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class ArraySetOp : public ::mlir::Op<ArraySetOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::ZeroResults, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::NOperands<3>::Impl, ::mlir::OpTrait::OpInvariants> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = ArraySetOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = ArraySetOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_array = 0;
  static constexpr int odsIndex_index = 1;
  static constexpr int odsIndex_value = 2;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    return {};
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.array_set");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::Type> getArray() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::Type>>(*getODSOperands(0).begin());
  }

  ::mlir::TypedValue<::mlir::IntegerType> getIndex() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::IntegerType>>(*getODSOperands(1).begin());
  }

  ::mlir::TypedValue<::mlir::Type> getValue() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::Type>>(*getODSOperands(2).begin());
  }

  ::mlir::OpOperand &getArrayMutable() {
    auto range = getODSOperandIndexAndLength(0);
    return getOperation()->getOpOperand(range.first);
  }

  ::mlir::OpOperand &getIndexMutable() {
    auto range = getODSOperandIndexAndLength(1);
    return getOperation()->getOpOperand(range.first);
  }

  ::mlir::OpOperand &getValueMutable() {
    auto range = getODSOperandIndexAndLength(2);
    return getOperation()->getOpOperand(range.first);
  }

  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value);
  static ArraySetOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value);
  static ArraySetOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value);
  static ArraySetOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value);
  static ArraySetOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value array, ::mlir::Value index, ::mlir::Value value);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static ArraySetOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static ArraySetOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static ArraySetOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static ArraySetOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
public:
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::ArraySetOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::ArrayStatefulOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class ArrayStatefulOpGenericAdaptorBase {
public:
  struct Properties {
    using elem_typeTy = ::mlir::TypeAttr;
    elem_typeTy elem_type;

    auto getElemType() const {
      auto &propStorage = this->elem_type;
      return ::llvm::cast<::mlir::TypeAttr>(propStorage);
    }
    void setElemType(const ::mlir::TypeAttr &propValue) {
      this->elem_type = propValue;
    }
    bool operator==(const Properties &rhs) const {
      return 
        rhs.elem_type == this->elem_type &&
        true;
    }
    bool operator!=(const Properties &rhs) const {
      return !(*this == rhs);
    }
  };
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  ArrayStatefulOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.array_state", odsAttrs.getContext());
  }

  ArrayStatefulOpGenericAdaptorBase(ArrayStatefulOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize);
  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

  ::mlir::TypeAttr getElemTypeAttr() {
    auto attr = ::llvm::cast<::mlir::TypeAttr>(getProperties().elem_type);
    return attr;
  }

  ::mlir::Type getElemType();
};
} // namespace detail
template <typename RangeT>
class ArrayStatefulOpGenericAdaptor : public detail::ArrayStatefulOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::ArrayStatefulOpGenericAdaptorBase;
  static constexpr int odsIndex_values = 0;
public:
  ArrayStatefulOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  ArrayStatefulOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : ArrayStatefulOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  ArrayStatefulOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : ArrayStatefulOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  ArrayStatefulOpGenericAdaptor(RangeT values, const ArrayStatefulOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = ArrayStatefulOp, typename = std::enable_if_t<std::is_same_v<LateInst, ArrayStatefulOp>>>
  ArrayStatefulOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  RangeT getValues() {
    return getODSOperands(0);
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class ArrayStatefulOpAdaptor : public ArrayStatefulOpGenericAdaptor<::mlir::ValueRange> {
public:
  using ArrayStatefulOpGenericAdaptor::ArrayStatefulOpGenericAdaptor;
  ArrayStatefulOpAdaptor(ArrayStatefulOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class ArrayStatefulOp : public ::mlir::Op<ArrayStatefulOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::Type>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = ArrayStatefulOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = ArrayStatefulOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_values = 0;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    static ::llvm::StringRef attrNames[] = {::llvm::StringRef("elem_type")};
    return ::llvm::ArrayRef(attrNames);
  }

  ::mlir::StringAttr getElemTypeAttrName() {
    return getAttributeNameForIndex(0);
  }

  static ::mlir::StringAttr getElemTypeAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 0);
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.array_state");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index);
  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::Operation::operand_range getValues() {
    return getODSOperands(0);
  }

  ::mlir::MutableOperandRange getValuesMutable();
  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::Type> getResult() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::Type>>(*getODSResults(0).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::llvm::LogicalResult readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state);
  void writeProperties(::mlir::DialectBytecodeWriter &writer);
  ::mlir::TypeAttr getElemTypeAttr() {
    return ::llvm::cast<::mlir::TypeAttr>(getProperties().elem_type);
  }

  ::mlir::Type getElemType();
  void setElemTypeAttr(::mlir::TypeAttr attr) {
    getProperties().elem_type = attr;
  }

  void setElemType(::mlir::Type attrValue);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static ArrayStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static ArrayStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static ArrayStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static ArrayStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::ValueRange values);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static ArrayStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static ArrayStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static ArrayStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static ArrayStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::mlir::ValueRange values);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static ArrayStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static ArrayStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static ArrayStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static ArrayStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
private:
  ::mlir::StringAttr getAttributeNameForIndex(unsigned index) {
    return getAttributeNameForIndex((*this)->getName(), index);
  }

  static ::mlir::StringAttr getAttributeNameForIndex(::mlir::OperationName name, unsigned index) {
    assert(index < 1 && "invalid attribute index");
    assert(name.getStringRef() == getOperationName() && "invalid operation name");
    assert(name.isRegistered() && "Operation isn't registered, missing a "
          "dependent dialect loading?");
    return name.getAttributeNames()[index];
  }

public:
  static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                    ::mlir::Type elem_type, ::mlir::ValueRange values);
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::ArrayStatefulOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::PhasorOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class PhasorOpGenericAdaptorBase {
public:
  struct Properties {
    using spfTy = ::mlir::FloatAttr;
    spfTy spf;

    auto getSpf() const {
      auto &propStorage = this->spf;
      return ::llvm::cast<::mlir::FloatAttr>(propStorage);
    }
    void setSpf(const ::mlir::FloatAttr &propValue) {
      this->spf = propValue;
    }
    bool operator==(const Properties &rhs) const {
      return 
        rhs.spf == this->spf &&
        true;
    }
    bool operator!=(const Properties &rhs) const {
      return !(*this == rhs);
    }
  };
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  PhasorOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.phasor", odsAttrs.getContext());
  }

  PhasorOpGenericAdaptorBase(PhasorOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
    return {index, 1};
  }

  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

  ::mlir::FloatAttr getSpfAttr() {
    auto attr = ::llvm::cast<::mlir::FloatAttr>(getProperties().spf);
    return attr;
  }

  ::llvm::APFloat getSpf();
};
} // namespace detail
template <typename RangeT>
class PhasorOpGenericAdaptor : public detail::PhasorOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::PhasorOpGenericAdaptorBase;
  static constexpr int odsIndex_freq = 0;
public:
  PhasorOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  PhasorOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : PhasorOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  PhasorOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : PhasorOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  PhasorOpGenericAdaptor(RangeT values, const PhasorOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = PhasorOp, typename = std::enable_if_t<std::is_same_v<LateInst, PhasorOp>>>
  PhasorOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  ValueT getFreq() {
    return (*getODSOperands(0).begin());
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class PhasorOpAdaptor : public PhasorOpGenericAdaptor<::mlir::ValueRange> {
public:
  using PhasorOpGenericAdaptor::PhasorOpGenericAdaptor;
  PhasorOpAdaptor(PhasorOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class PhasorOp : public ::mlir::Op<PhasorOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::FloatType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::OneOperand, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = PhasorOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = PhasorOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_freq = 0;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    static ::llvm::StringRef attrNames[] = {::llvm::StringRef("spf")};
    return ::llvm::ArrayRef(attrNames);
  }

  ::mlir::StringAttr getSpfAttrName() {
    return getAttributeNameForIndex(0);
  }

  static ::mlir::StringAttr getSpfAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 0);
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.phasor");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::FloatType> getFreq() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::FloatType>>(*getODSOperands(0).begin());
  }

  ::mlir::OpOperand &getFreqMutable() {
    auto range = getODSOperandIndexAndLength(0);
    return getOperation()->getOpOperand(range.first);
  }

  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::FloatType> getResult() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::FloatType>>(*getODSResults(0).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::llvm::LogicalResult readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state);
  void writeProperties(::mlir::DialectBytecodeWriter &writer);
  ::mlir::FloatAttr getSpfAttr() {
    return ::llvm::cast<::mlir::FloatAttr>(getProperties().spf);
  }

  ::llvm::APFloat getSpf();
  void setSpfAttr(::mlir::FloatAttr attr) {
    getProperties().spf = attr;
  }

  void setSpf(::llvm::APFloat attrValue);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static PhasorOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static PhasorOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static PhasorOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static PhasorOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value freq, ::llvm::APFloat spf);
  static PhasorOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value freq, ::llvm::APFloat spf);
  static PhasorOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value freq, ::llvm::APFloat spf);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::llvm::APFloat spf);
  static PhasorOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::llvm::APFloat spf);
  static PhasorOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value freq, ::llvm::APFloat spf);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static PhasorOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static PhasorOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static PhasorOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static PhasorOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
private:
  ::mlir::StringAttr getAttributeNameForIndex(unsigned index) {
    return getAttributeNameForIndex((*this)->getName(), index);
  }

  static ::mlir::StringAttr getAttributeNameForIndex(::mlir::OperationName name, unsigned index) {
    assert(index < 1 && "invalid attribute index");
    assert(name.getStringRef() == getOperationName() && "invalid operation name");
    assert(name.isRegistered() && "Operation isn't registered, missing a "
          "dependent dialect loading?");
    return name.getAttributeNames()[index];
  }

public:
  static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                    ::mlir::Value freq, double spf);
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::PhasorOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::PhasorStatefulOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class PhasorStatefulOpGenericAdaptorBase {
public:
  struct Properties {
    using spfTy = ::mlir::FloatAttr;
    spfTy spf;

    auto getSpf() const {
      auto &propStorage = this->spf;
      return ::llvm::cast<::mlir::FloatAttr>(propStorage);
    }
    void setSpf(const ::mlir::FloatAttr &propValue) {
      this->spf = propValue;
    }
    bool operator==(const Properties &rhs) const {
      return 
        rhs.spf == this->spf &&
        true;
    }
    bool operator!=(const Properties &rhs) const {
      return !(*this == rhs);
    }
  };
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  PhasorStatefulOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.phasor_stateful", odsAttrs.getContext());
  }

  PhasorStatefulOpGenericAdaptorBase(PhasorStatefulOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
    return {index, 1};
  }

  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

  ::mlir::FloatAttr getSpfAttr() {
    auto attr = ::llvm::cast<::mlir::FloatAttr>(getProperties().spf);
    return attr;
  }

  ::llvm::APFloat getSpf();
};
} // namespace detail
template <typename RangeT>
class PhasorStatefulOpGenericAdaptor : public detail::PhasorStatefulOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::PhasorStatefulOpGenericAdaptorBase;
  static constexpr int odsIndex_phase_slot = 0;
  static constexpr int odsIndex_freq = 1;
public:
  PhasorStatefulOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  PhasorStatefulOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : PhasorStatefulOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  PhasorStatefulOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : PhasorStatefulOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  PhasorStatefulOpGenericAdaptor(RangeT values, const PhasorStatefulOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = PhasorStatefulOp, typename = std::enable_if_t<std::is_same_v<LateInst, PhasorStatefulOp>>>
  PhasorStatefulOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  ValueT getPhaseSlot() {
    return (*getODSOperands(0).begin());
  }

  ValueT getFreq() {
    return (*getODSOperands(1).begin());
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class PhasorStatefulOpAdaptor : public PhasorStatefulOpGenericAdaptor<::mlir::ValueRange> {
public:
  using PhasorStatefulOpGenericAdaptor::PhasorStatefulOpGenericAdaptor;
  PhasorStatefulOpAdaptor(PhasorStatefulOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class PhasorStatefulOp : public ::mlir::Op<PhasorStatefulOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::FloatType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::NOperands<2>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = PhasorStatefulOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = PhasorStatefulOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_phase_slot = 0;
  static constexpr int odsIndex_freq = 1;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    static ::llvm::StringRef attrNames[] = {::llvm::StringRef("spf")};
    return ::llvm::ArrayRef(attrNames);
  }

  ::mlir::StringAttr getSpfAttrName() {
    return getAttributeNameForIndex(0);
  }

  static ::mlir::StringAttr getSpfAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 0);
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.phasor_stateful");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::LLVM::LLVMPointerType> getPhaseSlot() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::LLVM::LLVMPointerType>>(*getODSOperands(0).begin());
  }

  ::mlir::TypedValue<::mlir::FloatType> getFreq() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::FloatType>>(*getODSOperands(1).begin());
  }

  ::mlir::OpOperand &getPhaseSlotMutable() {
    auto range = getODSOperandIndexAndLength(0);
    return getOperation()->getOpOperand(range.first);
  }

  ::mlir::OpOperand &getFreqMutable() {
    auto range = getODSOperandIndexAndLength(1);
    return getOperation()->getOpOperand(range.first);
  }

  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::FloatType> getResult() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::FloatType>>(*getODSResults(0).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::llvm::LogicalResult readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state);
  void writeProperties(::mlir::DialectBytecodeWriter &writer);
  ::mlir::FloatAttr getSpfAttr() {
    return ::llvm::cast<::mlir::FloatAttr>(getProperties().spf);
  }

  ::llvm::APFloat getSpf();
  void setSpfAttr(::mlir::FloatAttr attr) {
    getProperties().spf = attr;
  }

  void setSpf(::llvm::APFloat attrValue);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static PhasorStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static PhasorStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static PhasorStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static PhasorStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::mlir::FloatAttr spf);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf);
  static PhasorStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf);
  static PhasorStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf);
  static PhasorStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf);
  static PhasorStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase_slot, ::mlir::Value freq, ::llvm::APFloat spf);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static PhasorStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static PhasorStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static PhasorStatefulOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static PhasorStatefulOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
private:
  ::mlir::StringAttr getAttributeNameForIndex(unsigned index) {
    return getAttributeNameForIndex((*this)->getName(), index);
  }

  static ::mlir::StringAttr getAttributeNameForIndex(::mlir::OperationName name, unsigned index) {
    assert(index < 1 && "invalid attribute index");
    assert(name.getStringRef() == getOperationName() && "invalid operation name");
    assert(name.isRegistered() && "Operation isn't registered, missing a "
          "dependent dialect loading?");
    return name.getAttributeNames()[index];
  }

public:
  static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                    ::mlir::Value phase_slot, ::mlir::Value freq, double spf);
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::PhasorStatefulOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::StateConsumeOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class StateConsumeOpGenericAdaptorBase {
public:
  struct Properties {
    using alignTy = ::mlir::IntegerAttr;
    alignTy align;

    auto getAlign() const {
      auto &propStorage = this->align;
      return ::llvm::cast<::mlir::IntegerAttr>(propStorage);
    }
    void setAlign(const ::mlir::IntegerAttr &propValue) {
      this->align = propValue;
    }
    using sizeTy = ::mlir::IntegerAttr;
    sizeTy size;

    auto getSize() const {
      auto &propStorage = this->size;
      return ::llvm::cast<::mlir::IntegerAttr>(propStorage);
    }
    void setSize(const ::mlir::IntegerAttr &propValue) {
      this->size = propValue;
    }
    bool operator==(const Properties &rhs) const {
      return 
        rhs.align == this->align &&
        rhs.size == this->size &&
        true;
    }
    bool operator!=(const Properties &rhs) const {
      return !(*this == rhs);
    }
  };
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  StateConsumeOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.state_consume", odsAttrs.getContext());
  }

  StateConsumeOpGenericAdaptorBase(StateConsumeOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
    return {index, 1};
  }

  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

  ::mlir::IntegerAttr getSizeAttr() {
    auto attr = ::llvm::cast<::mlir::IntegerAttr>(getProperties().size);
    return attr;
  }

  uint32_t getSize();
  ::mlir::IntegerAttr getAlignAttr() {
    auto attr = ::llvm::cast<::mlir::IntegerAttr>(getProperties().align);
    return attr;
  }

  uint32_t getAlign();
};
} // namespace detail
template <typename RangeT>
class StateConsumeOpGenericAdaptor : public detail::StateConsumeOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::StateConsumeOpGenericAdaptorBase;
  static constexpr int odsIndex_cursor = 0;
public:
  StateConsumeOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  StateConsumeOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : StateConsumeOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  StateConsumeOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : StateConsumeOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  StateConsumeOpGenericAdaptor(RangeT values, const StateConsumeOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = StateConsumeOp, typename = std::enable_if_t<std::is_same_v<LateInst, StateConsumeOp>>>
  StateConsumeOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  ValueT getCursor() {
    return (*getODSOperands(0).begin());
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class StateConsumeOpAdaptor : public StateConsumeOpGenericAdaptor<::mlir::ValueRange> {
public:
  using StateConsumeOpGenericAdaptor::StateConsumeOpGenericAdaptor;
  StateConsumeOpAdaptor(StateConsumeOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class StateConsumeOp : public ::mlir::Op<StateConsumeOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::NResults<2>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::OneOperand, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait, ::mlir::OpAsmOpInterface::Trait> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = StateConsumeOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = StateConsumeOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_cursor = 0;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    static ::llvm::StringRef attrNames[] = {::llvm::StringRef("align"), ::llvm::StringRef("size")};
    return ::llvm::ArrayRef(attrNames);
  }

  ::mlir::StringAttr getAlignAttrName() {
    return getAttributeNameForIndex(0);
  }

  static ::mlir::StringAttr getAlignAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 0);
  }

  ::mlir::StringAttr getSizeAttrName() {
    return getAttributeNameForIndex(1);
  }

  static ::mlir::StringAttr getSizeAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 1);
  }

  void getAsmResultNames(::mlir::OpAsmSetValueNameFn setNameFn);
  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.state_consume");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::LLVM::LLVMPointerType> getCursor() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::LLVM::LLVMPointerType>>(*getODSOperands(0).begin());
  }

  ::mlir::OpOperand &getCursorMutable() {
    auto range = getODSOperandIndexAndLength(0);
    return getOperation()->getOpOperand(range.first);
  }

  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::LLVM::LLVMPointerType> getPtr() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::LLVM::LLVMPointerType>>(*getODSResults(0).begin());
  }

  ::mlir::TypedValue<::mlir::LLVM::LLVMPointerType> getNextCursor() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::LLVM::LLVMPointerType>>(*getODSResults(1).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::llvm::LogicalResult readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state);
  void writeProperties(::mlir::DialectBytecodeWriter &writer);
  ::mlir::IntegerAttr getSizeAttr() {
    return ::llvm::cast<::mlir::IntegerAttr>(getProperties().size);
  }

  uint32_t getSize();
  ::mlir::IntegerAttr getAlignAttr() {
    return ::llvm::cast<::mlir::IntegerAttr>(getProperties().align);
  }

  uint32_t getAlign();
  void setSizeAttr(::mlir::IntegerAttr attr) {
    getProperties().size = attr;
  }

  void setSize(uint32_t attrValue);
  void setAlignAttr(::mlir::IntegerAttr attr) {
    getProperties().align = attr;
  }

  void setAlign(uint32_t attrValue);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align);
  static StateConsumeOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align);
  static StateConsumeOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align);
  static StateConsumeOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align);
  static StateConsumeOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, ::mlir::IntegerAttr size, ::mlir::IntegerAttr align);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, uint32_t size, uint32_t align);
  static StateConsumeOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, uint32_t size, uint32_t align);
  static StateConsumeOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type ptr, ::mlir::Type next_cursor, ::mlir::Value cursor, uint32_t size, uint32_t align);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, uint32_t size, uint32_t align);
  static StateConsumeOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, uint32_t size, uint32_t align);
  static StateConsumeOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value cursor, uint32_t size, uint32_t align);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static StateConsumeOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static StateConsumeOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static StateConsumeOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static StateConsumeOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::mlir::ParseResult parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result);
  void print(::mlir::OpAsmPrinter &_odsPrinter);
private:
  ::mlir::StringAttr getAttributeNameForIndex(unsigned index) {
    return getAttributeNameForIndex((*this)->getName(), index);
  }

  static ::mlir::StringAttr getAttributeNameForIndex(::mlir::OperationName name, unsigned index) {
    assert(index < 2 && "invalid attribute index");
    assert(name.getStringRef() == getOperationName() && "invalid operation name");
    assert(name.isRegistered() && "Operation isn't registered, missing a "
          "dependent dialect loading?");
    return name.getAttributeNames()[index];
  }

public:
  static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                    ::mlir::Value cursor, int32_t size, int32_t align);
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::StateConsumeOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::StateCursorOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class StateCursorOpGenericAdaptorBase {
public:
  using Properties = ::mlir::EmptyProperties;
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  StateCursorOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.state_cursor", odsAttrs.getContext());
  }

  StateCursorOpGenericAdaptorBase(StateCursorOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
    return {index, 1};
  }

  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

};
} // namespace detail
template <typename RangeT>
class StateCursorOpGenericAdaptor : public detail::StateCursorOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::StateCursorOpGenericAdaptorBase;
  static constexpr int odsIndex_base = 0;
public:
  StateCursorOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  StateCursorOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : StateCursorOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  StateCursorOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : StateCursorOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  StateCursorOpGenericAdaptor(RangeT values, const StateCursorOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = StateCursorOp, typename = std::enable_if_t<std::is_same_v<LateInst, StateCursorOp>>>
  StateCursorOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  ValueT getBase() {
    return (*getODSOperands(0).begin());
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class StateCursorOpAdaptor : public StateCursorOpGenericAdaptor<::mlir::ValueRange> {
public:
  using StateCursorOpGenericAdaptor::StateCursorOpGenericAdaptor;
  StateCursorOpAdaptor(StateCursorOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class StateCursorOp : public ::mlir::Op<StateCursorOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::LLVM::LLVMPointerType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::OneOperand, ::mlir::OpTrait::OpInvariants> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = StateCursorOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = StateCursorOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_base = 0;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    return {};
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.state_cursor");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::LLVM::LLVMPointerType> getBase() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::LLVM::LLVMPointerType>>(*getODSOperands(0).begin());
  }

  ::mlir::OpOperand &getBaseMutable() {
    auto range = getODSOperandIndexAndLength(0);
    return getOperation()->getOpOperand(range.first);
  }

  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::LLVM::LLVMPointerType> getCursor() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::LLVM::LLVMPointerType>>(*getODSResults(0).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type cursor, ::mlir::Value base);
  static StateCursorOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type cursor, ::mlir::Value base);
  static StateCursorOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type cursor, ::mlir::Value base);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value base);
  static StateCursorOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value base);
  static StateCursorOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value base);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static StateCursorOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static StateCursorOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static StateCursorOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static StateCursorOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
public:
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::StateCursorOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::StateSlotOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class StateSlotOpGenericAdaptorBase {
public:
  struct Properties {
    using byte_offsetTy = ::mlir::IntegerAttr;
    byte_offsetTy byte_offset;

    auto getByteOffset() const {
      auto &propStorage = this->byte_offset;
      return ::llvm::cast<::mlir::IntegerAttr>(propStorage);
    }
    void setByteOffset(const ::mlir::IntegerAttr &propValue) {
      this->byte_offset = propValue;
    }
    using elem_typeTy = ::mlir::TypeAttr;
    elem_typeTy elem_type;

    auto getElemType() const {
      auto &propStorage = this->elem_type;
      return ::llvm::cast<::mlir::TypeAttr>(propStorage);
    }
    void setElemType(const ::mlir::TypeAttr &propValue) {
      this->elem_type = propValue;
    }
    using init_valTy = ::mlir::FloatAttr;
    init_valTy init_val;

    auto getInitVal() const {
      auto &propStorage = this->init_val;
      return ::llvm::cast<::mlir::FloatAttr>(propStorage);
    }
    void setInitVal(const ::mlir::FloatAttr &propValue) {
      this->init_val = propValue;
    }
    bool operator==(const Properties &rhs) const {
      return 
        rhs.byte_offset == this->byte_offset &&
        rhs.elem_type == this->elem_type &&
        rhs.init_val == this->init_val &&
        true;
    }
    bool operator!=(const Properties &rhs) const {
      return !(*this == rhs);
    }
  };
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  StateSlotOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.state_slot", odsAttrs.getContext());
  }

  StateSlotOpGenericAdaptorBase(StateSlotOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
    return {index, 1};
  }

  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

  ::mlir::TypeAttr getElemTypeAttr() {
    auto attr = ::llvm::cast<::mlir::TypeAttr>(getProperties().elem_type);
    return attr;
  }

  ::mlir::Type getElemType();
  ::mlir::FloatAttr getInitValAttr() {
    auto attr = ::llvm::cast<::mlir::FloatAttr>(getProperties().init_val);
    return attr;
  }

  ::llvm::APFloat getInitVal();
  ::mlir::IntegerAttr getByteOffsetAttr() {
    auto attr = ::llvm::cast<::mlir::IntegerAttr>(getProperties().byte_offset);
    return attr;
  }

  int64_t getByteOffset();
};
} // namespace detail
template <typename RangeT>
class StateSlotOpGenericAdaptor : public detail::StateSlotOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::StateSlotOpGenericAdaptorBase;
public:
  StateSlotOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  StateSlotOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : StateSlotOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  StateSlotOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : StateSlotOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  StateSlotOpGenericAdaptor(RangeT values, const StateSlotOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = StateSlotOp, typename = std::enable_if_t<std::is_same_v<LateInst, StateSlotOp>>>
  StateSlotOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class StateSlotOpAdaptor : public StateSlotOpGenericAdaptor<::mlir::ValueRange> {
public:
  using StateSlotOpGenericAdaptor::StateSlotOpGenericAdaptor;
  StateSlotOpAdaptor(StateSlotOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class StateSlotOp : public ::mlir::Op<StateSlotOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::LLVM::LLVMPointerType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::ZeroOperands, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = StateSlotOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = StateSlotOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    static ::llvm::StringRef attrNames[] = {::llvm::StringRef("byte_offset"), ::llvm::StringRef("elem_type"), ::llvm::StringRef("init_val")};
    return ::llvm::ArrayRef(attrNames);
  }

  ::mlir::StringAttr getByteOffsetAttrName() {
    return getAttributeNameForIndex(0);
  }

  static ::mlir::StringAttr getByteOffsetAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 0);
  }

  ::mlir::StringAttr getElemTypeAttrName() {
    return getAttributeNameForIndex(1);
  }

  static ::mlir::StringAttr getElemTypeAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 1);
  }

  ::mlir::StringAttr getInitValAttrName() {
    return getAttributeNameForIndex(2);
  }

  static ::mlir::StringAttr getInitValAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 2);
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.state_slot");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::LLVM::LLVMPointerType> getResult() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::LLVM::LLVMPointerType>>(*getODSResults(0).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::llvm::LogicalResult readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state);
  void writeProperties(::mlir::DialectBytecodeWriter &writer);
  ::mlir::TypeAttr getElemTypeAttr() {
    return ::llvm::cast<::mlir::TypeAttr>(getProperties().elem_type);
  }

  ::mlir::Type getElemType();
  ::mlir::FloatAttr getInitValAttr() {
    return ::llvm::cast<::mlir::FloatAttr>(getProperties().init_val);
  }

  ::llvm::APFloat getInitVal();
  ::mlir::IntegerAttr getByteOffsetAttr() {
    return ::llvm::cast<::mlir::IntegerAttr>(getProperties().byte_offset);
  }

  int64_t getByteOffset();
  void setElemTypeAttr(::mlir::TypeAttr attr) {
    getProperties().elem_type = attr;
  }

  void setElemType(::mlir::Type attrValue);
  void setInitValAttr(::mlir::FloatAttr attr) {
    getProperties().init_val = attr;
  }

  void setInitVal(::llvm::APFloat attrValue);
  void setByteOffsetAttr(::mlir::IntegerAttr attr) {
    getProperties().byte_offset = attr;
  }

  void setByteOffset(int64_t attrValue);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset);
  static StateSlotOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset);
  static StateSlotOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset);
  static StateSlotOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset);
  static StateSlotOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr elem_type, ::mlir::FloatAttr init_val, ::mlir::IntegerAttr byte_offset);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset);
  static StateSlotOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset);
  static StateSlotOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset);
  static StateSlotOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset);
  static StateSlotOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Type elem_type, ::llvm::APFloat init_val, int64_t byte_offset);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static StateSlotOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static StateSlotOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static StateSlotOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static StateSlotOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
private:
  ::mlir::StringAttr getAttributeNameForIndex(unsigned index) {
    return getAttributeNameForIndex((*this)->getName(), index);
  }

  static ::mlir::StringAttr getAttributeNameForIndex(::mlir::OperationName name, unsigned index) {
    assert(index < 3 && "invalid attribute index");
    assert(name.getStringRef() == getOperationName() && "invalid operation name");
    assert(name.isRegistered() && "Operation isn't registered, missing a "
          "dependent dialect loading?");
    return name.getAttributeNames()[index];
  }

public:
  static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                    ::mlir::Type elem_type, double init_val = 0.0);
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::StateSlotOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::SubSynthOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class SubSynthOpGenericAdaptorBase {
public:
  struct Properties {
    using num_inputsTy = ::mlir::IntegerAttr;
    num_inputsTy num_inputs;

    auto getNumInputs() const {
      auto &propStorage = this->num_inputs;
      return ::llvm::cast<::mlir::IntegerAttr>(propStorage);
    }
    void setNumInputs(const ::mlir::IntegerAttr &propValue) {
      this->num_inputs = propValue;
    }
    using state_bytesTy = ::mlir::IntegerAttr;
    state_bytesTy state_bytes;

    auto getStateBytes() const {
      auto &propStorage = this->state_bytes;
      return ::llvm::cast<::mlir::IntegerAttr>(propStorage);
    }
    void setStateBytes(const ::mlir::IntegerAttr &propValue) {
      this->state_bytes = propValue;
    }
    using synth_nameTy = ::mlir::StringAttr;
    synth_nameTy synth_name;

    auto getSynthName() const {
      auto &propStorage = this->synth_name;
      return ::llvm::cast<::mlir::StringAttr>(propStorage);
    }
    void setSynthName(const ::mlir::StringAttr &propValue) {
      this->synth_name = propValue;
    }
    bool operator==(const Properties &rhs) const {
      return 
        rhs.num_inputs == this->num_inputs &&
        rhs.state_bytes == this->state_bytes &&
        rhs.synth_name == this->synth_name &&
        true;
    }
    bool operator!=(const Properties &rhs) const {
      return !(*this == rhs);
    }
  };
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  SubSynthOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.subsynth", odsAttrs.getContext());
  }

  SubSynthOpGenericAdaptorBase(SubSynthOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize);
  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

  ::mlir::StringAttr getSynthNameAttr() {
    auto attr = ::llvm::cast<::mlir::StringAttr>(getProperties().synth_name);
    return attr;
  }

  ::llvm::StringRef getSynthName();
  ::mlir::IntegerAttr getStateBytesAttr() {
    auto attr = ::llvm::cast<::mlir::IntegerAttr>(getProperties().state_bytes);
    return attr;
  }

  uint32_t getStateBytes();
  ::mlir::IntegerAttr getNumInputsAttr() {
    auto attr = ::llvm::cast<::mlir::IntegerAttr>(getProperties().num_inputs);
    return attr;
  }

  uint32_t getNumInputs();
};
} // namespace detail
template <typename RangeT>
class SubSynthOpGenericAdaptor : public detail::SubSynthOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::SubSynthOpGenericAdaptorBase;
  static constexpr int odsIndex_inputs = 0;
public:
  SubSynthOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  SubSynthOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : SubSynthOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  SubSynthOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : SubSynthOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  SubSynthOpGenericAdaptor(RangeT values, const SubSynthOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = SubSynthOp, typename = std::enable_if_t<std::is_same_v<LateInst, SubSynthOp>>>
  SubSynthOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  RangeT getInputs() {
    return getODSOperands(0);
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class SubSynthOpAdaptor : public SubSynthOpGenericAdaptor<::mlir::ValueRange> {
public:
  using SubSynthOpGenericAdaptor::SubSynthOpGenericAdaptor;
  SubSynthOpAdaptor(SubSynthOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class SubSynthOp : public ::mlir::Op<SubSynthOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::FloatType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::VariadicOperands, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = SubSynthOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = SubSynthOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_inputs = 0;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    static ::llvm::StringRef attrNames[] = {::llvm::StringRef("num_inputs"), ::llvm::StringRef("state_bytes"), ::llvm::StringRef("synth_name")};
    return ::llvm::ArrayRef(attrNames);
  }

  ::mlir::StringAttr getNumInputsAttrName() {
    return getAttributeNameForIndex(0);
  }

  static ::mlir::StringAttr getNumInputsAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 0);
  }

  ::mlir::StringAttr getStateBytesAttrName() {
    return getAttributeNameForIndex(1);
  }

  static ::mlir::StringAttr getStateBytesAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 1);
  }

  ::mlir::StringAttr getSynthNameAttrName() {
    return getAttributeNameForIndex(2);
  }

  static ::mlir::StringAttr getSynthNameAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 2);
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.subsynth");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index);
  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::Operation::operand_range getInputs() {
    return getODSOperands(0);
  }

  ::mlir::MutableOperandRange getInputsMutable();
  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::FloatType> getResult() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::FloatType>>(*getODSResults(0).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::llvm::LogicalResult readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state);
  void writeProperties(::mlir::DialectBytecodeWriter &writer);
  ::mlir::StringAttr getSynthNameAttr() {
    return ::llvm::cast<::mlir::StringAttr>(getProperties().synth_name);
  }

  ::llvm::StringRef getSynthName();
  ::mlir::IntegerAttr getStateBytesAttr() {
    return ::llvm::cast<::mlir::IntegerAttr>(getProperties().state_bytes);
  }

  uint32_t getStateBytes();
  ::mlir::IntegerAttr getNumInputsAttr() {
    return ::llvm::cast<::mlir::IntegerAttr>(getProperties().num_inputs);
  }

  uint32_t getNumInputs();
  void setSynthNameAttr(::mlir::StringAttr attr) {
    getProperties().synth_name = attr;
  }

  void setSynthName(::llvm::StringRef attrValue);
  void setStateBytesAttr(::mlir::IntegerAttr attr) {
    getProperties().state_bytes = attr;
  }

  void setStateBytes(uint32_t attrValue);
  void setNumInputsAttr(::mlir::IntegerAttr attr) {
    getProperties().num_inputs = attr;
  }

  void setNumInputs(uint32_t attrValue);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs);
  static SubSynthOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs);
  static SubSynthOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs);
  static SubSynthOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs);
  static SubSynthOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::mlir::StringAttr synth_name, ::mlir::IntegerAttr state_bytes, ::mlir::IntegerAttr num_inputs);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs);
  static SubSynthOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs);
  static SubSynthOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs);
  static SubSynthOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs);
  static SubSynthOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange inputs, ::llvm::StringRef synth_name, uint32_t state_bytes, uint32_t num_inputs);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static SubSynthOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static SubSynthOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static SubSynthOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static SubSynthOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
private:
  ::mlir::StringAttr getAttributeNameForIndex(unsigned index) {
    return getAttributeNameForIndex((*this)->getName(), index);
  }

  static ::mlir::StringAttr getAttributeNameForIndex(::mlir::OperationName name, unsigned index) {
    assert(index < 3 && "invalid attribute index");
    assert(name.getStringRef() == getOperationName() && "invalid operation name");
    assert(name.isRegistered() && "Operation isn't registered, missing a "
          "dependent dialect loading?");
    return name.getAttributeNames()[index];
  }

public:
  static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                    ::llvm::StringRef synth_name, int32_t state_bytes,
                    int32_t num_inputs, ::mlir::ValueRange inputs);
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::SubSynthOp)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::Tabread2Op declarations
//===----------------------------------------------------------------------===//

namespace detail {
class Tabread2OpGenericAdaptorBase {
public:
  struct Properties {
    using table_sizeTy = ::mlir::IntegerAttr;
    table_sizeTy table_size;

    auto getTableSize() const {
      auto &propStorage = this->table_size;
      return ::llvm::cast<::mlir::IntegerAttr>(propStorage);
    }
    void setTableSize(const ::mlir::IntegerAttr &propValue) {
      this->table_size = propValue;
    }
    using table_symbolTy = ::mlir::StringAttr;
    table_symbolTy table_symbol;

    auto getTableSymbol() const {
      auto &propStorage = this->table_symbol;
      return ::llvm::cast<::mlir::StringAttr>(propStorage);
    }
    void setTableSymbol(const ::mlir::StringAttr &propValue) {
      this->table_symbol = propValue;
    }
    bool operator==(const Properties &rhs) const {
      return 
        rhs.table_size == this->table_size &&
        rhs.table_symbol == this->table_symbol &&
        true;
    }
    bool operator!=(const Properties &rhs) const {
      return !(*this == rhs);
    }
  };
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  Tabread2OpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.tabread2", odsAttrs.getContext());
  }

  Tabread2OpGenericAdaptorBase(Tabread2Op op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
    return {index, 1};
  }

  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

  ::mlir::StringAttr getTableSymbolAttr() {
    auto attr = ::llvm::cast<::mlir::StringAttr>(getProperties().table_symbol);
    return attr;
  }

  ::llvm::StringRef getTableSymbol();
  ::mlir::IntegerAttr getTableSizeAttr() {
    auto attr = ::llvm::cast<::mlir::IntegerAttr>(getProperties().table_size);
    return attr;
  }

  uint32_t getTableSize();
};
} // namespace detail
template <typename RangeT>
class Tabread2OpGenericAdaptor : public detail::Tabread2OpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::Tabread2OpGenericAdaptorBase;
  static constexpr int odsIndex_phase = 0;
public:
  Tabread2OpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  Tabread2OpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : Tabread2OpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  Tabread2OpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : Tabread2OpGenericAdaptor(values, attrs, Properties{}, {}) {}

  Tabread2OpGenericAdaptor(RangeT values, const Tabread2OpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = Tabread2Op, typename = std::enable_if_t<std::is_same_v<LateInst, Tabread2Op>>>
  Tabread2OpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  ValueT getPhase() {
    return (*getODSOperands(0).begin());
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class Tabread2OpAdaptor : public Tabread2OpGenericAdaptor<::mlir::ValueRange> {
public:
  using Tabread2OpGenericAdaptor::Tabread2OpGenericAdaptor;
  Tabread2OpAdaptor(Tabread2Op op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class Tabread2Op : public ::mlir::Op<Tabread2Op, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::FloatType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::OneOperand, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = Tabread2OpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = Tabread2OpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_phase = 0;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    static ::llvm::StringRef attrNames[] = {::llvm::StringRef("table_size"), ::llvm::StringRef("table_symbol")};
    return ::llvm::ArrayRef(attrNames);
  }

  ::mlir::StringAttr getTableSizeAttrName() {
    return getAttributeNameForIndex(0);
  }

  static ::mlir::StringAttr getTableSizeAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 0);
  }

  ::mlir::StringAttr getTableSymbolAttrName() {
    return getAttributeNameForIndex(1);
  }

  static ::mlir::StringAttr getTableSymbolAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 1);
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.tabread2");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::FloatType> getPhase() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::FloatType>>(*getODSOperands(0).begin());
  }

  ::mlir::OpOperand &getPhaseMutable() {
    auto range = getODSOperandIndexAndLength(0);
    return getOperation()->getOpOperand(range.first);
  }

  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::FloatType> getResult() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::FloatType>>(*getODSResults(0).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::llvm::LogicalResult readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state);
  void writeProperties(::mlir::DialectBytecodeWriter &writer);
  ::mlir::StringAttr getTableSymbolAttr() {
    return ::llvm::cast<::mlir::StringAttr>(getProperties().table_symbol);
  }

  ::llvm::StringRef getTableSymbol();
  ::mlir::IntegerAttr getTableSizeAttr() {
    return ::llvm::cast<::mlir::IntegerAttr>(getProperties().table_size);
  }

  uint32_t getTableSize();
  void setTableSymbolAttr(::mlir::StringAttr attr) {
    getProperties().table_symbol = attr;
  }

  void setTableSymbol(::llvm::StringRef attrValue);
  void setTableSizeAttr(::mlir::IntegerAttr attr) {
    getProperties().table_size = attr;
  }

  void setTableSize(uint32_t attrValue);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size);
  static Tabread2Op create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size);
  static Tabread2Op create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size);
  static Tabread2Op create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size);
  static Tabread2Op create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::StringAttr table_symbol, ::mlir::IntegerAttr table_size);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size);
  static Tabread2Op create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size);
  static Tabread2Op create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size);
  static Tabread2Op create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size);
  static Tabread2Op create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::llvm::StringRef table_symbol, uint32_t table_size);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static Tabread2Op create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static Tabread2Op create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static Tabread2Op create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static Tabread2Op create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
private:
  ::mlir::StringAttr getAttributeNameForIndex(unsigned index) {
    return getAttributeNameForIndex((*this)->getName(), index);
  }

  static ::mlir::StringAttr getAttributeNameForIndex(::mlir::OperationName name, unsigned index) {
    assert(index < 2 && "invalid attribute index");
    assert(name.getStringRef() == getOperationName() && "invalid operation name");
    assert(name.isRegistered() && "Operation isn't registered, missing a "
          "dependent dialect loading?");
    return name.getAttributeNames()[index];
  }

public:
  static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                    ::mlir::Value phase, ::llvm::StringRef table_symbol,
                    int32_t table_size);
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::Tabread2Op)

namespace dsp {

//===----------------------------------------------------------------------===//
// dsp::TabreadOp declarations
//===----------------------------------------------------------------------===//

namespace detail {
class TabreadOpGenericAdaptorBase {
public:
  struct Properties {
    using table_sizeTy = ::mlir::IntegerAttr;
    table_sizeTy table_size;

    auto getTableSize() const {
      auto &propStorage = this->table_size;
      return ::llvm::cast<::mlir::IntegerAttr>(propStorage);
    }
    void setTableSize(const ::mlir::IntegerAttr &propValue) {
      this->table_size = propValue;
    }
    bool operator==(const Properties &rhs) const {
      return 
        rhs.table_size == this->table_size &&
        true;
    }
    bool operator!=(const Properties &rhs) const {
      return !(*this == rhs);
    }
  };
protected:
  ::mlir::DictionaryAttr odsAttrs;
  ::std::optional<::mlir::OperationName> odsOpName;
  Properties properties;
  ::mlir::RegionRange odsRegions;
public:
  TabreadOpGenericAdaptorBase(::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : odsAttrs(attrs), properties(properties), odsRegions(regions) {  if (odsAttrs)
      odsOpName.emplace("dsp.tabread", odsAttrs.getContext());
  }

  TabreadOpGenericAdaptorBase(TabreadOp op);

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
    return {index, 1};
  }

  const Properties &getProperties() {
    return properties;
  }

  ::mlir::DictionaryAttr getAttributes() {
    return odsAttrs;
  }

  ::mlir::IntegerAttr getTableSizeAttr() {
    auto attr = ::llvm::cast<::mlir::IntegerAttr>(getProperties().table_size);
    return attr;
  }

  uint32_t getTableSize();
};
} // namespace detail
template <typename RangeT>
class TabreadOpGenericAdaptor : public detail::TabreadOpGenericAdaptorBase {
  using ValueT = ::llvm::detail::ValueOfRange<RangeT>;
  using Base = detail::TabreadOpGenericAdaptorBase;
  static constexpr int odsIndex_phase = 0;
  static constexpr int odsIndex_table_ptr = 1;
public:
  TabreadOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, const Properties &properties, ::mlir::RegionRange regions = {}) : Base(attrs, properties, regions), odsOperands(values) {}

  TabreadOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs, ::mlir::OpaqueProperties properties, ::mlir::RegionRange regions = {}) : TabreadOpGenericAdaptor(values, attrs, (properties ? *properties.as<Properties *>() : Properties{}), regions) {}

  TabreadOpGenericAdaptor(RangeT values, ::mlir::DictionaryAttr attrs = nullptr) : TabreadOpGenericAdaptor(values, attrs, Properties{}, {}) {}

  TabreadOpGenericAdaptor(RangeT values, const TabreadOpGenericAdaptorBase &base) : Base(base), odsOperands(values) {}

  template <typename LateInst = TabreadOp, typename = std::enable_if_t<std::is_same_v<LateInst, TabreadOp>>>
  TabreadOpGenericAdaptor(RangeT values, LateInst op) : Base(op), odsOperands(values) {}

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return Base::getODSOperandIndexAndLength(index, odsOperands.size());
  }

  RangeT getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(odsOperands.begin(), valueRange.first),
             std::next(odsOperands.begin(), valueRange.first + valueRange.second)};
  }

  ValueT getPhase() {
    return (*getODSOperands(0).begin());
  }

  ValueT getTablePtr() {
    return (*getODSOperands(1).begin());
  }

  RangeT getOperands() {
    return odsOperands;
  }

private:
  RangeT odsOperands;
};
class TabreadOpAdaptor : public TabreadOpGenericAdaptor<::mlir::ValueRange> {
public:
  using TabreadOpGenericAdaptor::TabreadOpGenericAdaptor;
  TabreadOpAdaptor(TabreadOp op);

  ::llvm::LogicalResult verify(::mlir::Location loc);
};
class TabreadOp : public ::mlir::Op<TabreadOp, ::mlir::OpTrait::ZeroRegions, ::mlir::OpTrait::OneResult, ::mlir::OpTrait::OneTypedResult<::mlir::FloatType>::Impl, ::mlir::OpTrait::ZeroSuccessors, ::mlir::OpTrait::NOperands<2>::Impl, ::mlir::OpTrait::OpInvariants, ::mlir::BytecodeOpInterface::Trait> {
public:
  using Op::Op;
  using Op::print;
  using Adaptor = TabreadOpAdaptor;
  template <typename RangeT>
  using GenericAdaptor = TabreadOpGenericAdaptor<RangeT>;
  using FoldAdaptor = GenericAdaptor<::llvm::ArrayRef<::mlir::Attribute>>;
  static constexpr int odsIndex_phase = 0;
  static constexpr int odsIndex_table_ptr = 1;
  using Properties = FoldAdaptor::Properties;
  static ::llvm::ArrayRef<::llvm::StringRef> getAttributeNames() {
    static ::llvm::StringRef attrNames[] = {::llvm::StringRef("table_size")};
    return ::llvm::ArrayRef(attrNames);
  }

  ::mlir::StringAttr getTableSizeAttrName() {
    return getAttributeNameForIndex(0);
  }

  static ::mlir::StringAttr getTableSizeAttrName(::mlir::OperationName name) {
    return getAttributeNameForIndex(name, 0);
  }

  static constexpr ::llvm::StringLiteral getOperationName() {
    return ::llvm::StringLiteral("dsp.tabread");
  }

  std::pair<unsigned, unsigned> getODSOperandIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::operand_range getODSOperands(unsigned index) {
    auto valueRange = getODSOperandIndexAndLength(index);
    return {std::next(getOperation()->operand_begin(), valueRange.first),
             std::next(getOperation()->operand_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::FloatType> getPhase() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::FloatType>>(*getODSOperands(0).begin());
  }

  ::mlir::TypedValue<::mlir::LLVM::LLVMPointerType> getTablePtr() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::LLVM::LLVMPointerType>>(*getODSOperands(1).begin());
  }

  ::mlir::OpOperand &getPhaseMutable() {
    auto range = getODSOperandIndexAndLength(0);
    return getOperation()->getOpOperand(range.first);
  }

  ::mlir::OpOperand &getTablePtrMutable() {
    auto range = getODSOperandIndexAndLength(1);
    return getOperation()->getOpOperand(range.first);
  }

  std::pair<unsigned, unsigned> getODSResultIndexAndLength(unsigned index) {
    return {index, 1};
  }

  ::mlir::Operation::result_range getODSResults(unsigned index) {
    auto valueRange = getODSResultIndexAndLength(index);
    return {std::next(getOperation()->result_begin(), valueRange.first),
             std::next(getOperation()->result_begin(), valueRange.first + valueRange.second)};
  }

  ::mlir::TypedValue<::mlir::FloatType> getResult() {
    return ::llvm::cast<::mlir::TypedValue<::mlir::FloatType>>(*getODSResults(0).begin());
  }

  static ::llvm::LogicalResult setPropertiesFromAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::mlir::Attribute getPropertiesAsAttr(::mlir::MLIRContext *ctx, const Properties &prop);
  static llvm::hash_code computePropertiesHash(const Properties &prop);
  static std::optional<mlir::Attribute> getInherentAttr(::mlir::MLIRContext *ctx, const Properties &prop, llvm::StringRef name);
  static void setInherentAttr(Properties &prop, llvm::StringRef name, mlir::Attribute value);
  static void populateInherentAttrs(::mlir::MLIRContext *ctx, const Properties &prop, ::mlir::NamedAttrList &attrs);
  static ::llvm::LogicalResult verifyInherentAttrs(::mlir::OperationName opName, ::mlir::NamedAttrList &attrs, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
  static ::llvm::LogicalResult readProperties(::mlir::DialectBytecodeReader &reader, ::mlir::OperationState &state);
  void writeProperties(::mlir::DialectBytecodeWriter &writer);
  ::mlir::IntegerAttr getTableSizeAttr() {
    return ::llvm::cast<::mlir::IntegerAttr>(getProperties().table_size);
  }

  uint32_t getTableSize();
  void setTableSizeAttr(::mlir::IntegerAttr attr) {
    getProperties().table_size = attr;
  }

  void setTableSize(uint32_t attrValue);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size);
  static TabreadOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size);
  static TabreadOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size);
  static TabreadOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size);
  static TabreadOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, ::mlir::IntegerAttr table_size);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size);
  static TabreadOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size);
  static TabreadOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size);
  static void build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size);
  static TabreadOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size);
  static TabreadOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value phase, ::mlir::Value table_ptr, uint32_t table_size);
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static TabreadOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static TabreadOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes = {});
  static void build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static TabreadOp create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  static TabreadOp create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, const Properties &properties, ::llvm::ArrayRef<::mlir::NamedAttribute> discardableAttributes = {});
  ::llvm::LogicalResult verifyInvariantsImpl();
  ::llvm::LogicalResult verifyInvariants();
  static ::llvm::LogicalResult setPropertiesFromParsedAttr(Properties &prop, ::mlir::Attribute attr, ::llvm::function_ref<::mlir::InFlightDiagnostic()> emitError);
private:
  ::mlir::StringAttr getAttributeNameForIndex(unsigned index) {
    return getAttributeNameForIndex((*this)->getName(), index);
  }

  static ::mlir::StringAttr getAttributeNameForIndex(::mlir::OperationName name, unsigned index) {
    assert(index < 1 && "invalid attribute index");
    assert(name.getStringRef() == getOperationName() && "invalid operation name");
    assert(name.isRegistered() && "Operation isn't registered, missing a "
          "dependent dialect loading?");
    return name.getAttributeNames()[index];
  }

public:
  static void build(::mlir::OpBuilder &b, ::mlir::OperationState &state,
                    ::mlir::Value phase, ::mlir::Value table_ptr,
                    int32_t table_size);
};
} // namespace dsp
MLIR_DECLARE_EXPLICIT_TYPE_ID(dsp::TabreadOp)


#endif  // GET_OP_CLASSES

