#include "decode/model/Value.h"
#include "decode/core/Try.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>

namespace decode {

Value::~Value()
{
}

Rc<Value> Value::fromType(const Rc<Type>& type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        return BuiltinValue::fromType(type->asBuiltin());
    case TypeKind::Reference:
        return new ReferenceValue(type->asReference());
    case TypeKind::Array:
        return new ArrayValue(type->asArray());
    case TypeKind::Slice:
        return new SliceValue(type->asSlice());
    case TypeKind::Function:
        return new FunctionValue(type->asFunction());
    case TypeKind::Enum:
        return new EnumValue(type->asEnum());
    case TypeKind::Struct:
        return new StructValue(type->asStruct());
    case TypeKind::Variant:
        return new VariantValue(type->asVariant());
    case TypeKind::Imported:
        return Value::fromType(type->asImported()->link());
    case TypeKind::Alias:
        return Value::fromType(type->asAlias()->alias());
    }
    assert(false);
    return nullptr;
}

ContainerValue::ContainerValue()
{
}

ContainerValue::ContainerValue(const Rc<Type>& type, std::size_t count)
{
    _values.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        _values.push_back(Value::fromType(type));
    }
}

ContainerValue::~ContainerValue()
{
}

bool ContainerValue::isContainer() const
{
    return true;
}

bool ContainerValue::encode(bmcl::MemWriter* dest) const
{
    for (const Rc<Value>& value : _values) {
        TRY(value->encode(dest));
    }
    return true;
}

bool ContainerValue::decode(bmcl::MemReader* src)
{
    for (const Rc<Value>& value : _values) {
        TRY(value->decode(src));
    }
    return true;
}

const std::vector<Rc<Value>> ContainerValue::values() const
{
    return _values;
}

ArrayValue::ArrayValue(const Rc<ArrayType>& type)
    : ContainerValue(type, type->elementCount())
    , _type(type)
{
}

ArrayValue::~ArrayValue()
{
}

Type* ArrayValue::type()
{
    return _type.get();
}

SliceValue::SliceValue(const Rc<SliceType>& type)
    : _type(type)
{
}

SliceValue::~SliceValue()
{
}

bool SliceValue::encode(bmcl::MemWriter* dest) const
{
    TRY(dest->writeVarUint(_values.size()));
    return ContainerValue::encode(dest);
}

bool SliceValue::decode(bmcl::MemReader* src)
{
    uint64_t size;
    TRY(src->readVarUint(&size));
    if (size > _values.size()) {
        _values.reserve(size);
        for (std::size_t i = 0; i < (size - _values.size()); i++) {
            _values.push_back(Value::fromType(_type->elementType()));
        }
    } else {
        _values.resize(size);
    }
    return ContainerValue::decode(src);
}

Type* SliceValue::type()
{
    return _type.get();
}

StructValue::StructValue(const Rc<StructType>& type)
{
    _values.reserve(type->fields()->size());
    for (const Rc<Field>& field : *type->fields()) {
        _values.push_back(Value::fromType(field->type()));
    }
}

StructValue::~StructValue()
{
}

Type* StructValue::type()
{
    return _type.get();
}

VariantValue::VariantValue(const Rc<VariantType>& type)
    : _type(type)
    , _currentId(0)
{
}

VariantValue::~VariantValue()
{
}

bool VariantValue::encode(bmcl::MemWriter* dest) const
{
    if (_values.empty()) {
        //TODO: report error
        return false;
    }
    TRY(dest->writeVarUint(_currentId));
    return ContainerValue::encode(dest);
}

bool VariantValue::decode(bmcl::MemReader* src)
{
    uint64_t id;
    TRY(src->readVarUint(&id));
    if (id >= _type->fields().size()) {
        //TODO: report error
        return false;
    }
    const Rc<VariantField>& field = _type->fields()[id];
    switch (field->variantFieldKind()) {
    case VariantFieldKind::Constant:
        break;
    case VariantFieldKind::Tuple: {
        const TupleVariantField* tField = static_cast<const TupleVariantField*>(field.get());
        std::size_t size = tField->types().size();
        _values.resize(size);
        for (std::size_t i = 0; i < size; i++) {
            _values[i] = Value::fromType(tField->types()[i]);
        }
        TRY(ContainerValue::decode(src));
        break;
    }
    case VariantFieldKind::Struct: {
        const StructVariantField* sField = static_cast<const StructVariantField*>(field.get());
        std::size_t size = sField->fields()->size();
        _values.resize(size);
        for (std::size_t i = 0; i < size; i++) {
            _values[i] = Value::fromType(sField->fields()->at(i)->type());
        }
        TRY(ContainerValue::decode(src));
        break;
    }
    default:
        assert(false);
    }

    _currentId = id;
    return true;
}

Type* VariantValue::type()
{
    return _type.get();
}

bool NonContainerValue::isContainer() const
{
    return false;
}

AddressValue::AddressValue()
    : _address(0)
{
}

AddressValue::~AddressValue()
{
}

bool AddressValue::encode(bmcl::MemWriter* dest) const
{
    //TODO: get target word size
    if (_address.isNone()) {
        //TODO: report error
        return false;
    }
    if (dest->writableSize() < 8) {
        return false;
    }
    dest->writeUint64Le(_address.unwrap());
    return true;
}

bool AddressValue::decode(bmcl::MemReader* src)
{
    //TODO: get target word size
    if (src->readableSize() < 8) {
        return false;
    }
    _address.unwrap() = src->readUint64Le();
    return true;
}

ReferenceValue::ReferenceValue(const Rc<ReferenceType>& type)
    : _type(type)
{
}

ReferenceValue::~ReferenceValue()
{
}

Type* ReferenceValue::type()
{
    return _type.get();
}

FunctionValue::FunctionValue(const Rc<FunctionType>& type)
    : _type(type)
{
}

FunctionValue::~FunctionValue()
{
}

Type* FunctionValue::type()
{
    return _type.get();
}

EnumValue::EnumValue(const Rc<EnumType>& type)
    : _type(type)
{
}

EnumValue::~EnumValue()
{
}

bool EnumValue::encode(bmcl::MemWriter* dest) const
{
    if (_currentId.isSome()) {
        TRY(dest->writeVarUint(_currentId.unwrap()));
        return true;
    }
    //TODO: report error
    return false;
}

bool EnumValue::decode(bmcl::MemReader* src)
{
    uint64_t value;
    TRY(src->readVarUint(&value));
    _currentId.unwrap() = value;
    return true;
}

Type* EnumValue::type()
{
    return _type.get();
}

BuiltinValue::BuiltinValue(const Rc<BuiltinType>& type)
    : _type(type)
{
}

BuiltinValue::~BuiltinValue()
{
}

Rc<BuiltinValue> BuiltinValue::fromType(const Rc<BuiltinType>& type)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        //TODO: check target ptr size
        return new NumericValue<std::uint64_t>(type);
    case BuiltinTypeKind::ISize:
        //TODO: check target ptr size
        return new NumericValue<std::int64_t>(type);
    case BuiltinTypeKind::Varint:
        return new VarintValue(type);
    case BuiltinTypeKind::Varuint:
        return new VaruintValue(type);
    case BuiltinTypeKind::U8:
        return new NumericValue<std::uint8_t>(type);
    case BuiltinTypeKind::I8:
        return new NumericValue<std::int8_t>(type);
    case BuiltinTypeKind::U16:
        return new NumericValue<std::uint16_t>(type);
    case BuiltinTypeKind::I16:
        return new NumericValue<std::int16_t>(type);
    case BuiltinTypeKind::U32:
        return new NumericValue<std::uint32_t>(type);
    case BuiltinTypeKind::I32:
        return new NumericValue<std::int32_t>(type);
    case BuiltinTypeKind::U64:
        return new NumericValue<std::uint64_t>(type);
    case BuiltinTypeKind::I64:
        return new NumericValue<std::int64_t>(type);
    case BuiltinTypeKind::Bool:
        //TODO: make bool value
        return new NumericValue<std::uint8_t>(type);
    case BuiltinTypeKind::Void:
        assert(false);
        return nullptr;
    }
    assert(false);
    return nullptr;
}

Type* BuiltinValue::type()
{
    return _type.get();
}

template <typename T>
NumericValue<T>::NumericValue(const Rc<BuiltinType>& type)
    : BuiltinValue(type)
{
}

template <typename T>
NumericValue<T>::~NumericValue()
{
}

template <typename T>
bool NumericValue<T>::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    if (dest->writableSize() < sizeof(T)) {
        //TODO: report error
        return false;
    }
    dest->writeType<T>(bmcl::htole<T>(_value.unwrap()));
    return true;
}

template <typename T>
bool NumericValue<T>::decode(bmcl::MemReader* src)
{
    if (src->readableSize() < sizeof(T)) {
        //TODO: report error
        return false;
    }
    _value.emplace(bmcl::letoh<T>(src->readType<T>()));
    return true;
}

template class NumericValue<std::uint8_t>;
template class NumericValue<std::int8_t>;
template class NumericValue<std::uint16_t>;
template class NumericValue<std::int16_t>;
template class NumericValue<std::uint32_t>;
template class NumericValue<std::int32_t>;
template class NumericValue<std::uint64_t>;
template class NumericValue<std::int64_t>;

VarintValue::VarintValue(const Rc<BuiltinType>& type)
    : BuiltinValue(type)
{
}

VarintValue::~VarintValue()
{
}

bool VarintValue::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    return dest->writeVarInt(_value.unwrap());
}

bool VarintValue::decode(bmcl::MemReader* src)
{
    int64_t value;
    TRY(src->readVarInt(&value));
    _value.emplace(value);
    return true;
}

VaruintValue::VaruintValue(const Rc<BuiltinType>& type)
    : BuiltinValue(type)
{
}

VaruintValue::~VaruintValue()
{
}

bool VaruintValue::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    return dest->writeVarUint(_value.unwrap());
}

bool VaruintValue::decode(bmcl::MemReader* src)
{
    uint64_t value;
    TRY(src->readVarUint(&value));
    _value.emplace(value);
    return true;
}
}
