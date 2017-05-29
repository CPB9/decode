/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/ValueNode.h"
#include "decode/model/Value.h"
#include "decode/core/Try.h"
#include "decode/core/Foreach.h"
#include "decode/generator/StringBuilder.h"
#include "decode/parser/Type.h"
#include "decode/parser/Component.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/ModelEventHandler.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>

namespace decode {

template <typename T>
inline void updateOptionalValue(bmcl::Option<T>* value, T newValue, ModelEventHandler* handler, Node* node, std::size_t nodeIndex)
{
    if (value->isSome()) {
        if (value->unwrap() != newValue) {
            value->unwrap() = newValue;
            handler->nodeValueUpdatedEvent(node, nodeIndex);
        }
    } else {
        value->emplace(newValue);
    }
}

ValueNode::ValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
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

static Rc<BuiltinValueNode> builtinNodeFromType(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
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

static Rc<ValueNode> createNodefromType(const Type* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
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

Rc<ValueNode> ValueNode::fromType(const Type* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
{
    assert(cache);
    return createNodefromType(type, cache, parent);
}

ContainerValueNode::ContainerValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
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

bool ContainerValueNode::encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    (void)nodeIndex;
    for (std::size_t i = 0; i < _values.size(); i++) {
        TRY(_values[i]->encode(i, handler, dest));
    }
    return true;
}

bool ContainerValueNode::decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src)
{
    (void)nodeIndex;
    for (std::size_t i = 0; i < _values.size(); i++) {
        TRY(_values[i]->decode(i, handler, src));
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

bmcl::OptionPtr<Node> ContainerValueNode::childAt(std::size_t idx)
{
    return childAtGeneric(_values, idx);
}

ArrayValueNode::ArrayValueNode(const ArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
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

SliceValueNode::SliceValueNode(const SliceType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
{
}

SliceValueNode::~SliceValueNode()
{
}

bool SliceValueNode::encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    if (!dest->writeVarUint(_values.size())) {
        //TODO: report error
        return false;
    }
    return ContainerValueNode::encode(nodeIndex, handler, dest);
}

bool SliceValueNode::decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src)
{
    uint64_t size;
    if (!src->readVarUint(&size)) {
        //TODO: report error
        return false;
    }
    resize(size, nodeIndex, handler);
    return ContainerValueNode::decode(nodeIndex, handler, src);
}

const Type* SliceValueNode::type() const
{
    return _type.get();
}

void SliceValueNode::resize(std::size_t size, std::size_t nodeIndex, ModelEventHandler* handler)
{
    std::size_t currentSize = _values.size();
    if (size > currentSize) {
        _values.reserve(size);
        for (std::size_t i = 0; i < (size - currentSize); i++) {
            Rc<ValueNode> node = ValueNode::fromType(_type->elementType(), _cache.get(), this);
            node->setFieldName(_cache->arrayIndex(currentSize + i));
            _values.push_back(node);
        }
        handler->nodesInsertedEvent(this, nodeIndex, currentSize, size - 1);
    } else if (size < currentSize) {
        _values.resize(size);
        handler->nodesRemovedEvent(this, nodeIndex, size, currentSize - 1);
    }
    assert(values().size() == size);
}

StructValueNode::StructValueNode(const StructType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
{
    _values.reserve(type->fieldsRange().size());
    for (const Field* field : type->fieldsRange()) {
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

VariantValueNode::VariantValueNode(const VariantType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
    , _currentId(0)
{
}

VariantValueNode::~VariantValueNode()
{
}

Value VariantValueNode::value() const
{
    if (_currentId.isSome()) {
        return Value::makeStringView(_type->fieldsBegin()[_currentId.unwrap()]->name());
    }
    return Value::makeUninitialized();
}

bool VariantValueNode::encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    if (_currentId.isNone()) {
        //TODO: report error
        return false;
    }
    TRY(dest->writeVarUint(_currentId.unwrap()));
    return ContainerValueNode::encode(nodeIndex, handler, dest);
}

bool VariantValueNode::decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src)
{
    uint64_t id;
    TRY(src->readVarUint(&id));
    if (id >= _type->fieldsRange().size()) {
        //TODO: report error
        return false;
    }
    updateOptionalValue(&_currentId, id, handler, this, nodeIndex);
    //TODO: do not resize if type doesn't change
    const VariantField* field = _type->fieldsBegin()[id];
    switch (field->variantFieldKind()) {
    case VariantFieldKind::Constant:
        break;
    case VariantFieldKind::Tuple: {
        const TupleVariantField* tField = static_cast<const TupleVariantField*>(field);
        std::size_t currentSize = _values.size();
        std::size_t size = tField->typesRange().size();
        if (size < currentSize) {
            _values.resize(size);
            handler->nodesRemovedEvent(this, nodeIndex, size, currentSize - 1);
        } else if (size > currentSize) {
            _values.resize(size);
            handler->nodesInsertedEvent(this, nodeIndex, currentSize, size - 1);
        }
        for (std::size_t i = 0; i < size; i++) {
            _values[i] = ValueNode::fromType(tField->typesBegin()[i], _cache.get(), this);
        }
        TRY(ContainerValueNode::decode(nodeIndex, handler, src));
        break;
    }
    case VariantFieldKind::Struct: {
        const StructVariantField* sField = static_cast<const StructVariantField*>(field);
        std::size_t currentSize = _values.size();
        std::size_t size = sField->fieldsRange().size();
        if (size < currentSize) {
            _values.resize(size);
            handler->nodesRemovedEvent(this, nodeIndex, size, currentSize - 1);
        } else if (size > currentSize) {
            _values.resize(size);
            handler->nodesInsertedEvent(this, nodeIndex, currentSize, size - 1);
        }
        for (std::size_t i = 0; i < size; i++) {
            _values[i] = ValueNode::fromType(sField->fieldsBegin()[i]->type(), _cache.get(), this);
        }
        TRY(ContainerValueNode::decode(nodeIndex, handler, src));
        break;
    }
    default:
        assert(false);
    }

    return true;
}

const Type* VariantValueNode::type() const
{
    return _type.get();
}

NonContainerValueNode::NonContainerValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ValueNode(cache, parent)
{
}

bool NonContainerValueNode::isContainerValue() const
{
    return false;
}

bool NonContainerValueNode::canHaveChildren() const
{
    return false;
}

bool NonContainerValueNode::canSetValue() const
{
    return true;
}

AddressValueNode::AddressValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NonContainerValueNode(cache, parent)
{
}

AddressValueNode::~AddressValueNode()
{
}

bool AddressValueNode::encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    (void)nodeIndex;
    (void)handler;
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

bool AddressValueNode::decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src)
{
    //TODO: get target word size
    if (src->readableSize() < 8) {
        //TODO: report error
        return false;
    }
    uint64_t value = src->readUint64Le();
    updateOptionalValue(&_address, value, handler, this, nodeIndex);
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

ValueKind AddressValueNode::valueKind() const
{
    return ValueKind::Unsigned;
}

bool AddressValueNode::setValue(const Value& value)
{
    if (value.isA(ValueKind::Unsigned)) {
        //TODO: check word size
        _address.emplace(value.asUnsigned());
        return true;
    }
    return false;
}

ReferenceValueNode::ReferenceValueNode(const ReferenceType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
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

FunctionValueNode::FunctionValueNode(const FunctionType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
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

EnumValueNode::EnumValueNode(const EnumType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NonContainerValueNode(cache, parent)
    , _type(type)
{
}

EnumValueNode::~EnumValueNode()
{
}

bool EnumValueNode::encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    (void)nodeIndex;
    (void)handler;
    if (_currentId.isSome()) {
        TRY(dest->writeVarUint(_currentId.unwrap()));
        return true;
    }
    //TODO: report error
    return false;
}

bool EnumValueNode::decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src)
{
    int64_t value;
    if (!src->readVarInt(&value)) {
        //TODO: report error
        return false;
    }
    auto it = _type->constantsRange().findIf([value](const EnumConstant* c) {
        return c->value() == value;
    });
    if (it == _type->constantsEnd()) {
        //TODO: report error
        return false;
    }
    updateOptionalValue(&_currentId, value, handler, this, nodeIndex);
    return true;
}

decode::Value EnumValueNode::value() const
{
    if (_currentId.isSome()) {
            auto it = _type->constantsRange().findIf([this](const EnumConstant* c) {
                return c->value() == _currentId.unwrap();
            });
        assert(it != _type->constantsEnd());
        return Value::makeStringView(it->name());
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

BuiltinValueNode::BuiltinValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NonContainerValueNode(cache, parent)
    , _type(type)
{
}

BuiltinValueNode::~BuiltinValueNode()
{
}


Rc<BuiltinValueNode> BuiltinValueNode::fromType(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
{
    assert(cache);
    return builtinNodeFromType(type, cache, parent);
}

const Type* BuiltinValueNode::type() const
{
    return _type.get();
}

template <typename T>
NumericValueNode<T>::NumericValueNode(const BuiltinType* type,const ValueInfoCache* cache,  bmcl::OptionPtr<Node> parent)
    : BuiltinValueNode(type, cache, parent)
{
}

template <typename T>
NumericValueNode<T>::~NumericValueNode()
{
}

template <typename T>
bool NumericValueNode<T>::encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    (void)nodeIndex;
    (void)handler;
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
bool NumericValueNode<T>::decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src)
{
    if (src->readableSize() < sizeof(T)) {
        //TODO: report error
        return false;
    }
    T value = bmcl::letoh<T>(src->readType<T>());
    updateOptionalValue(&_value, value, handler, this, nodeIndex);
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

template <typename T>
ValueKind NumericValueNode<T>::valueKind() const
{
    if (std::is_signed<T>::value) {
        return ValueKind::Signed;
    }
    return ValueKind::Unsigned;
}

template <typename T>
bool NumericValueNode<T>::emplace(int64_t value)
{
    if (std::is_signed<T>::value) {
        if (value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max()) {
            _value.emplace(value);
            return true;
        }
        return false;
    } else {
        if (value >= 0 && uint64_t(value) <= std::numeric_limits<T>::max()) {
            _value.emplace(value);
            return true;
        }
        return false;
    }
    return false;
}

template <typename T>
bool NumericValueNode<T>::emplace(uint64_t value)
{
    if (value <= std::numeric_limits<T>::max()) {
        _value.emplace(value);
        return true;
    }
    return false;
}

template <typename T>
bool NumericValueNode<T>::setValue(const Value& value)
{
    if (value.isA(ValueKind::Signed)) {
        return emplace(value.asSigned());
    } else if (value.isA(ValueKind::Unsigned)) {
        return emplace(value.asUnsigned());
    }
    return false;
}

template class NumericValueNode<std::uint8_t>;
template class NumericValueNode<std::int8_t>;
template class NumericValueNode<std::uint16_t>;
template class NumericValueNode<std::int16_t>;
template class NumericValueNode<std::uint32_t>;
template class NumericValueNode<std::int32_t>;
template class NumericValueNode<std::uint64_t>;
template class NumericValueNode<std::int64_t>;

VarintValueNode::VarintValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : BuiltinValueNode(type, cache, parent)
{
}

VarintValueNode::~VarintValueNode()
{
}

bool VarintValueNode::encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    (void)nodeIndex;
    (void)handler;
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    return dest->writeVarInt(_value.unwrap());
}

bool VarintValueNode::decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src)
{
    int64_t value;
    //TODO: report error
    TRY(src->readVarInt(&value));
    updateOptionalValue(&_value, value, handler, this, nodeIndex);
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

ValueKind VarintValueNode::valueKind() const
{
    return ValueKind::Signed;
}

bool VarintValueNode::setValue(const Value& value)
{
    if (value.isA(ValueKind::Signed)) {
        _value.emplace(value.asSigned());
        return true;
    }
    return false;
}

VaruintValueNode::VaruintValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : BuiltinValueNode(type, cache, parent)
{
}

VaruintValueNode::~VaruintValueNode()
{
}

bool VaruintValueNode::encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    (void)handler;
    (void)nodeIndex;
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    return dest->writeVarUint(_value.unwrap());
}

bool VaruintValueNode::decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src)
{
    uint64_t value;
    //TODO: report error
    TRY(src->readVarUint(&value));
    updateOptionalValue(&_value, value, handler, this, nodeIndex);
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

ValueKind VaruintValueNode::valueKind() const
{
    return ValueKind::Unsigned;
}

bool VaruintValueNode::setValue(const Value& value)
{
    if (value.isA(ValueKind::Unsigned)) {
        _value.emplace(value.asUnsigned());
        return true;
    }
    return false;
}
}
