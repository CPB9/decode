#include "decode/model/ValueNode.h"
#include "decode/model/Value.h"
#include "decode/core/Try.h"
#include "decode/core/Foreach.h"
#include "decode/generator/StringBuilder.h"
#include "decode/parser/Type.h"
#include "decode/model/ValueInfoCache.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>

namespace decode {

ValueNode::ValueNode(const ValueInfoCache* cache, Node* parent)
    : Node(parent)
    , _cache(cache)
{
}

ValueNode::~ValueNode()
{
}

bmcl::StringView ValueNode::typeName() const
{
    return _cache->nameForType(type());
}

bmcl::StringView ValueNode::fieldName() const
{
    return _fieldName;
}

static Rc<BuiltinValueNode> builtinNodeFromType(const BuiltinType* type, const ValueInfoCache* cache, Node* parent)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        //TODO: check target ptr size
        return new NumericValueNode<std::uint64_t>(type, cache, parent);
    case BuiltinTypeKind::ISize:
        //TODO: check target ptr size
        return new NumericValueNode<std::int64_t>(type, cache, parent);
    case BuiltinTypeKind::Varint:
        return new VarintValueNode(type, cache, parent);
    case BuiltinTypeKind::Varuint:
        return new VaruintValueNode(type, cache, parent);
    case BuiltinTypeKind::U8:
        return new NumericValueNode<std::uint8_t>(type, cache, parent);
    case BuiltinTypeKind::I8:
        return new NumericValueNode<std::int8_t>(type, cache, parent);
    case BuiltinTypeKind::U16:
        return new NumericValueNode<std::uint16_t>(type, cache, parent);
    case BuiltinTypeKind::I16:
        return new NumericValueNode<std::int16_t>(type, cache, parent);
    case BuiltinTypeKind::U32:
        return new NumericValueNode<std::uint32_t>(type, cache, parent);
    case BuiltinTypeKind::I32:
        return new NumericValueNode<std::int32_t>(type, cache, parent);
    case BuiltinTypeKind::U64:
        return new NumericValueNode<std::uint64_t>(type, cache, parent);
    case BuiltinTypeKind::I64:
        return new NumericValueNode<std::int64_t>(type, cache, parent);
    case BuiltinTypeKind::Bool:
        //TODO: make bool value
        return new NumericValueNode<std::uint8_t>(type, cache, parent);
    case BuiltinTypeKind::Void:
        assert(false);
        return nullptr;
    }
    assert(false);
    return nullptr;
}

static Rc<ValueNode> createNodefromType(const Type* type, const ValueInfoCache* cache, Node* parent)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        return builtinNodeFromType(type->asBuiltin(), cache, parent);
    case TypeKind::Reference:
        return new ReferenceValueNode(type->asReference(), cache, parent);
    case TypeKind::Array:
        return new ArrayValueNode(type->asArray(), cache, parent);
    case TypeKind::Slice:
        return new SliceValueNode(type->asSlice(), cache, parent);
    case TypeKind::Function:
        return new FunctionValueNode(type->asFunction(), cache, parent);
    case TypeKind::Enum:
        return new EnumValueNode(type->asEnum(), cache, parent);
    case TypeKind::Struct:
        return new StructValueNode(type->asStruct(), cache, parent);
    case TypeKind::Variant:
        return new VariantValueNode(type->asVariant(), cache, parent);
    case TypeKind::Imported:
        return createNodefromType(type->asImported()->link(), cache, parent);
    case TypeKind::Alias:
        return createNodefromType(type->asAlias()->alias(), cache, parent);
    }
    assert(false);
    return nullptr;
}

Rc<ValueNode> ValueNode::fromType(const Type* type, const ValueInfoCache* cache, Node* parent)
{
    assert(cache);
    return createNodefromType(type, cache, parent);
}

ContainerValueNode::ContainerValueNode(const ValueInfoCache* cache, Node* parent)
    : ValueNode(cache, parent)
{
}

ContainerValueNode::~ContainerValueNode()
{
    for (const Rc<ValueNode>& v : _values) {
        v->setParent(nullptr);
    }
}

bool ContainerValueNode::isContainerValue() const
{
    return true;
}

bool ContainerValueNode::isInitialized() const
{
    for (const Rc<ValueNode>& v : _values) {
        if (!v->isInitialized()) {
            return false;
        }
    }
    return true;
}

bool ContainerValueNode::encode(bmcl::MemWriter* dest) const
{
    for (const Rc<ValueNode>& value : _values) {
        TRY(value->encode(dest));
    }
    return true;
}

bool ContainerValueNode::decode(bmcl::MemReader* src)
{
    for (const Rc<ValueNode>& value : _values) {
        TRY(value->decode(src));
    }
    return true;
}

const std::vector<Rc<ValueNode>> ContainerValueNode::values() const
{
    return _values;
}

std::size_t ContainerValueNode::numChildren() const
{
    return _values.size();
}

bmcl::Option<std::size_t> ContainerValueNode::childIndex(const Node* node) const
{
    return childIndexGeneric(_values, node);
}

Node* ContainerValueNode::childAt(std::size_t idx)
{
    return childAtGeneric(_values, idx);
}

ArrayValueNode::ArrayValueNode(const ArrayType* type, const ValueInfoCache* cache, Node* parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
{
    std::size_t count = type->elementCount();
    _values.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        Rc<ValueNode> node = ValueNode::fromType(type->elementType(), _cache.get(), this);
        node->setFieldName(_cache->arrayIndex(i));
        _values.push_back(node);
    }
}

ArrayValueNode::~ArrayValueNode()
{
}

const Type* ArrayValueNode::type() const
{
    return _type.get();
}

bmcl::Option<std::size_t> ArrayValueNode::fixedSize() const
{
    return _type->elementCount();
}

SliceValueNode::SliceValueNode(const SliceType* type, const ValueInfoCache* cache, Node* parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
{
}

SliceValueNode::~SliceValueNode()
{
}

bool SliceValueNode::encode(bmcl::MemWriter* dest) const
{
    TRY(dest->writeVarUint(_values.size()));
    return ContainerValueNode::encode(dest);
}

bool SliceValueNode::decode(bmcl::MemReader* src)
{
    uint64_t size;
    TRY(src->readVarUint(&size));
    resize(size);
    return ContainerValueNode::decode(src);
}

const Type* SliceValueNode::type() const
{
    return _type.get();
}

bmcl::Option<std::size_t> SliceValueNode::fixedSize() const
{
    return bmcl::None;
}

void SliceValueNode::resize(std::size_t size)
{
    std::size_t currentSize = _values.size();
    if (size > currentSize) {
        _values.reserve(size);
        for (std::size_t i = 0; i < (size - _values.size()); i++) {
            Rc<ValueNode> node = ValueNode::fromType(_type->elementType(), _cache.get(), this);
            node->setFieldName(_cache->arrayIndex(currentSize + i));
            _values.push_back(node);
        }
    } else {
        _values.resize(size);
    }
}

StructValueNode::StructValueNode(const StructType* type, const ValueInfoCache* cache, Node* parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
{
    _values.reserve(type->fields()->size());
    for (const Rc<Field>& field : *type->fields()) {
        Rc<ValueNode> node = ValueNode::fromType(field->type(), _cache.get(), this);
        node->setFieldName(field->name());
        _values.push_back(node);
    }
}

StructValueNode::~StructValueNode()
{
}

const Type* StructValueNode::type() const
{
    return _type.get();
}

bmcl::Option<std::size_t> StructValueNode::fixedSize() const
{
    return _type->fields()->size();
}

bmcl::OptionPtr<ValueNode> StructValueNode::nodeWithName(bmcl::StringView name)
{
    bmcl::OptionPtr<const Field> field = _type->fieldWithName(name);
    if (field.isNone()) {
        return bmcl::None;
    }
    for (const Rc<ValueNode>& node : _values) {
        if (node->type() == field->type()) {
            return node.get();
        }
    }
    return bmcl::None;
}

VariantValueNode::VariantValueNode(const VariantType* type, const ValueInfoCache* cache, Node* parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
    , _currentId(0)
{
}

VariantValueNode::~VariantValueNode()
{
}

bool VariantValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_values.empty()) {
        //TODO: report error
        return false;
    }
    TRY(dest->writeVarUint(_currentId));
    return ContainerValueNode::encode(dest);
}

bool VariantValueNode::decode(bmcl::MemReader* src)
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
            _values[i] = ValueNode::fromType(tField->types()[i].get(), _cache.get(), this);
        }
        TRY(ContainerValueNode::decode(src));
        break;
    }
    case VariantFieldKind::Struct: {
        const StructVariantField* sField = static_cast<const StructVariantField*>(field.get());
        std::size_t size = sField->fields()->size();
        _values.resize(size);
        for (std::size_t i = 0; i < size; i++) {
            _values[i] = ValueNode::fromType(sField->fields()->at(i)->type(), _cache.get(), this);
        }
        TRY(ContainerValueNode::decode(src));
        break;
    }
    default:
        assert(false);
    }

    _currentId = id;
    return true;
}

const Type* VariantValueNode::type() const
{
    return _type.get();
}

bmcl::Option<std::size_t> VariantValueNode::fixedSize() const
{
    return bmcl::None;
}

NonContainerValueNode::NonContainerValueNode(const ValueInfoCache* cache, Node* parent)
    : ValueNode(cache, parent)
{
}

bool NonContainerValueNode::isContainerValue() const
{
    return false;
}

AddressValueNode::AddressValueNode(const ValueInfoCache* cache, Node* parent)
    : NonContainerValueNode(cache, parent)
{
}

AddressValueNode::~AddressValueNode()
{
}

bool AddressValueNode::encode(bmcl::MemWriter* dest) const
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

bool AddressValueNode::decode(bmcl::MemReader* src)
{
    //TODO: get target word size
    if (src->readableSize() < 8) {
        return false;
    }
    _address.unwrap() = src->readUint64Le();
    return true;
}

bool AddressValueNode::isInitialized() const
{
    return _address.isSome();
}

Value AddressValueNode::value() const
{
    if (_address.isSome()) {
        return Value::makeUnsigned(_address.unwrap());
    }
    return Value::makeUninitialized();
}

ReferenceValueNode::ReferenceValueNode(const ReferenceType* type, const ValueInfoCache* cache, Node* parent)
    : AddressValueNode(cache, parent)
    , _type(type)
{
}

ReferenceValueNode::~ReferenceValueNode()
{
}

const Type* ReferenceValueNode::type() const
{
    return _type.get();
}

FunctionValueNode::FunctionValueNode(const FunctionType* type, const ValueInfoCache* cache, Node* parent)
    : AddressValueNode(cache, parent)
    , _type(type)
{
}

FunctionValueNode::~FunctionValueNode()
{
}

const Type* FunctionValueNode::type() const
{
    return _type.get();
}

EnumValueNode::EnumValueNode(const EnumType* type, const ValueInfoCache* cache, Node* parent)
    : NonContainerValueNode(cache, parent)
    , _type(type)
{
}

EnumValueNode::~EnumValueNode()
{
}

bool EnumValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_currentId.isSome()) {
        TRY(dest->writeVarUint(_currentId.unwrap()));
        return true;
    }
    //TODO: report error
    return false;
}

bool EnumValueNode::decode(bmcl::MemReader* src)
{
    uint64_t value;
    TRY(src->readVarUint(&value));
    auto it = _type->constants().find(value);
    if (it == _type->constants().end()) {
        //TODO: report error
        return false;
    }
    _currentId.emplace(value);
    return true;
}

decode::Value EnumValueNode::value() const
{
    if (_currentId.isSome()) {
        auto it = _type->constants().find(_currentId.unwrap());
        assert(it != _type->constants().end());
        return Value::makeStringView(it->second->name());
    }
    return Value::makeUninitialized();
}

bool EnumValueNode::isInitialized() const
{
    return _currentId.isSome();
}

const Type* EnumValueNode::type() const
{
    return _type.get();
}

BuiltinValueNode::BuiltinValueNode(const BuiltinType* type, const ValueInfoCache* cache, Node* parent)
    : NonContainerValueNode(cache, parent)
    , _type(type)
{
}

BuiltinValueNode::~BuiltinValueNode()
{
}


Rc<BuiltinValueNode> BuiltinValueNode::fromType(const BuiltinType* type, const ValueInfoCache* cache, Node* parent)
{
    assert(cache);
    return builtinNodeFromType(type, cache, parent);
}

const Type* BuiltinValueNode::type() const
{
    return _type.get();
}

template <typename T>
NumericValueNode<T>::NumericValueNode(const BuiltinType* type,const ValueInfoCache* cache,  Node* parent)
    : BuiltinValueNode(type, cache, parent)
{
}

template <typename T>
NumericValueNode<T>::~NumericValueNode()
{
}

template <typename T>
bool NumericValueNode<T>::encode(bmcl::MemWriter* dest) const
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
bool NumericValueNode<T>::decode(bmcl::MemReader* src)
{
    if (src->readableSize() < sizeof(T)) {
        //TODO: report error
        return false;
    }
    _value.emplace(bmcl::letoh<T>(src->readType<T>()));
    return true;
}

template <typename T>
decode::Value NumericValueNode<T>::value() const
{
    if (_value.isNone()) {
        return Value::makeUninitialized();
    }
    if (std::is_signed<T>::value) {
        return Value::makeSigned(_value.unwrap());
    } else {
        return Value::makeUnsigned(_value.unwrap());
    }
}

template <typename T>
bool NumericValueNode<T>::isInitialized() const
{
    return _value.isSome();
}

template class NumericValueNode<std::uint8_t>;
template class NumericValueNode<std::int8_t>;
template class NumericValueNode<std::uint16_t>;
template class NumericValueNode<std::int16_t>;
template class NumericValueNode<std::uint32_t>;
template class NumericValueNode<std::int32_t>;
template class NumericValueNode<std::uint64_t>;
template class NumericValueNode<std::int64_t>;

VarintValueNode::VarintValueNode(const BuiltinType* type, const ValueInfoCache* cache, Node* parent)
    : BuiltinValueNode(type, cache, parent)
{
}

VarintValueNode::~VarintValueNode()
{
}

bool VarintValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    return dest->writeVarInt(_value.unwrap());
}

bool VarintValueNode::decode(bmcl::MemReader* src)
{
    int64_t value;
    TRY(src->readVarInt(&value));
    _value.emplace(value);
    return true;
}

Value VarintValueNode::value() const
{
    if (_value.isSome()) {
        return Value::makeSigned(_value.unwrap());
    }
    return Value::makeUninitialized();
}

bool VarintValueNode::isInitialized() const
{
    return _value.isSome();
}

VaruintValueNode::VaruintValueNode(const BuiltinType* type, const ValueInfoCache* cache, Node* parent)
    : BuiltinValueNode(type, cache, parent)
{
}

VaruintValueNode::~VaruintValueNode()
{
}

bool VaruintValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    return dest->writeVarUint(_value.unwrap());
}

bool VaruintValueNode::decode(bmcl::MemReader* src)
{
    uint64_t value;
    TRY(src->readVarUint(&value));
    _value.emplace(value);
    return true;
}

Value VaruintValueNode::value() const
{
    if (_value.isSome()) {
        return Value::makeUnsigned(_value.unwrap());
    }
    return Value::makeUninitialized();
}

bool VaruintValueNode::isInitialized() const
{
    return _value.isSome();
}
}
