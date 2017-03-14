#include "decode/model/Value.h"
#include "decode/core/Try.h"
#include "decode/core/Foreach.h"
#include "decode/generator/StringBuilder.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>

namespace decode {

static void buildNamedTypeName(const NamedType* type, StringBuilder* dest)
{
    dest->append(type->moduleName());
    dest->append("::");
    dest->append(type->name());
}

static void buildTypeName(const Type* type, StringBuilder* dest)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin: {
        const BuiltinType* builtin = type->asBuiltin();
        switch (builtin->builtinTypeKind()) {
        case BuiltinTypeKind::USize:
            dest->append("usize");
            return;
        case BuiltinTypeKind::ISize:
            dest->append("isize");
            return;
        case BuiltinTypeKind::Varuint:
            dest->append("varuint");
            return;
        case BuiltinTypeKind::Varint:
            dest->append( "varint");
            return;
        case BuiltinTypeKind::U8:
            dest->append( "u8");
            return;
        case BuiltinTypeKind::I8:
            dest->append( "i8");
            return;
        case BuiltinTypeKind::U16:
            dest->append( "u16");
            return;
        case BuiltinTypeKind::I16:
            dest->append( "i16");
            return;
        case BuiltinTypeKind::U32:
            dest->append( "u32");
            return;
        case BuiltinTypeKind::I32:
            dest->append( "i32");
            return;
        case BuiltinTypeKind::U64:
            dest->append( "u64");
            return;
        case BuiltinTypeKind::I64:
            dest->append( "i64");
            return;
        case BuiltinTypeKind::Bool:
            dest->append( "bool");
            return;
        case BuiltinTypeKind::Void:
            dest->append("void");
            return;
        }
        assert(false);
        break;
    }
    case TypeKind::Reference: {
        const ReferenceType* ref = type->asReference();
        if (ref->referenceKind() == ReferenceKind::Pointer) {
            if (ref->isMutable()) {
                dest->append("*mut ");
            } else {
                dest->append("*const ");
            }
        } else {
            if (ref->isMutable()) {
                dest->append("&mut ");
            } else {
                dest->append('&');
            }
        }
        buildTypeName(ref->pointee(), dest);
        return;
    }
    case TypeKind::Array: {
        const ArrayType* array = type->asArray();
        dest->append('[');
        buildTypeName(array->elementType().get(), dest);
        dest->append(", ");
        dest->appendNumericValue(array->elementCount());
        dest->append(']');
        return;
    }
    case TypeKind::Slice: {
        const SliceType* slice = type->asSlice();
        dest->append("&[");
        buildTypeName(slice->elementType().get(), dest);
        dest->append(']');
        return;
    }
    case TypeKind::Function: {
        const FunctionType* func = type->asFunction();
        dest->append("&Fn(");
        foreachList(func->arguments(), [dest](const Rc<Field>& arg){
            dest->append(arg->name());
            dest->append(": ");
            buildTypeName(arg->type(), dest);
        }, [dest](const Rc<Field>& arg){
            dest->append(", ");
        });
        if (func->returnValue().isSome()) {
            dest->append(") -> ");
            buildTypeName(func->returnValue().unwrap().get(), dest);
        } else {
            dest->append(')');
        }
        return;
    }
    case TypeKind::Enum:
        buildNamedTypeName(type->asEnum(), dest);
        return;
    case TypeKind::Struct:
        buildNamedTypeName(type->asStruct(), dest);
        return;
    case TypeKind::Variant:
        buildNamedTypeName(type->asVariant(), dest);
        return;
    case TypeKind::Imported:
        buildNamedTypeName(type->asImported(), dest);
        return;
    case TypeKind::Alias:
        buildNamedTypeName(type->asAlias(), dest);
        return;
    }
    assert(false);
}

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

bool ContainerValue::isInitialized() const
{
    for (const Rc<Value>& v : _values) {
        if (!v->isInitialized()) {
            return false;
        }
    }
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

bmcl::Option<std::size_t> ArrayValue::fixedSize() const
{
    return _type->elementCount();
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

bmcl::Option<std::size_t> SliceValue::fixedSize() const
{
    return bmcl::None;
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

bmcl::Option<std::size_t> StructValue::fixedSize() const
{
    return _type->fields()->size();
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

bmcl::Option<std::size_t> VariantValue::fixedSize() const
{
    return bmcl::None;
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

bool AddressValue::isInitialized() const
{
    return _address.isSome();
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

bool EnumValue::isInitialized() const
{
    return _currentId.isSome();
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

template <typename T>
bool NumericValue<T>::isInitialized() const
{
    return _value.isSome();
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

bool VarintValue::isInitialized() const
{
    return _value.isSome();
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

bool VaruintValue::isInitialized() const
{
    return _value.isSome();
}
}
