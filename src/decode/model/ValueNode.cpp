/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/ValueNode.h"
#include "decode/model/Value.h"
#include "decode/ast/Field.h"
#include "decode/core/Try.h"
#include "decode/core/Foreach.h"
#include "decode/ast/Type.h"
#include "decode/ast/Component.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/NodeViewUpdate.h"
#include "decode/model/NodeView.h"
#include "decode/model/NodeViewUpdater.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>

namespace bmcl {
#ifdef BMCL_LITTLE_ENDIAN
template <>
inline float htole(float value)
{
    return value;
}

template <>
inline float letoh(float value)
{
    return value;
}

template <>
inline double htole(double value)
{
    return value;
}

template <>
inline double letoh(double value)
{
    return value;
}
#else
# error TODO: implement for big endian
#endif
template <>
inline char htole(char value)
{
    return value;
}

template <>
inline char letoh(char value)
{
    return value;
}
}

namespace decode {

template <typename T>
inline bool updateOptionalValue(bmcl::Option<T>* value, T newValue)
{
    if (value->isSome()) {
        if (value->unwrap() != newValue) {
            value->unwrap() = newValue;
            return true;
        }
        return false;
    }
    value->emplace(newValue);
    return true;
}

template <typename T>
inline bool updateOptionalValuePair(bmcl::Option<ValuePair<T>>* value, T newValue)
{
    if (value->isSome()) {
        if (value->unwrap().value() != newValue) {
            value->unwrap().setValue(newValue);
            return true;
        }
        return false;
    }
    value->emplace(newValue);
    return true;
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

bmcl::StringView ValueNode::shortDescription() const
{
    return _shortDesc;
}

void ValueNode::setFieldName(bmcl::StringView name)
{
    _fieldName = name;
}

void ValueNode::setShortDesc(bmcl::StringView desc)
{
    _shortDesc = desc;
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
    case BuiltinTypeKind::F32:
        return new NumericValueNode<float>(type, cache, parent);
    case BuiltinTypeKind::F64:
        return new NumericValueNode<double>(type, cache, parent);
    case BuiltinTypeKind::Bool:
        //TODO: make bool value
        return new NumericValueNode<std::uint8_t>(type, cache, parent);
    case BuiltinTypeKind::Void:
        assert(false);
        return nullptr;
    case BuiltinTypeKind::Char:
        return new NumericValueNode<char>(type, cache, parent);
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
    case TypeKind::DynArray: {
        const DynArrayType* dynArray = type->asDynArray();
        if (dynArray->elementType()->isBuiltin()) {
            if (dynArray->elementType()->asBuiltin()->builtinTypeKind() == BuiltinTypeKind::Char) {
                return new StringValueNode(dynArray, cache, parent);
            }
        }
        return new DynArrayValueNode(dynArray, cache, parent);
    }
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

bool ContainerValueNode::encode(bmcl::MemWriter* dest) const
{
    for (std::size_t i = 0; i < _values.size(); i++) {
        TRY(_values[i]->encode(dest));
    }
    return true;
}

bool ContainerValueNode::decode(bmcl::MemReader* src)
{
    for (std::size_t i = 0; i < _values.size(); i++) {
        TRY(_values[i]->decode(src));
    }
    return true;
}

const std::vector<Rc<ValueNode>> ContainerValueNode::values()
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

ValueNode* ContainerValueNode::nodeAt(std::size_t index)
{
    return _values[index].get();
}

const ValueNode* ContainerValueNode::nodeAt(std::size_t index) const
{
    return _values[index].get();
}

ArrayValueNode::ArrayValueNode(const ArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
    , _changedSinceUpdate(false)
{
    std::size_t count = type->elementCount();
    _values.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        Rc<ValueNode> node = ValueNode::fromType(type->elementType(), _cache.get(), this);
        node->setFieldName(_indexCache.arrayIndex(i));
        _values.push_back(node);
    }
}

ArrayValueNode::~ArrayValueNode()
{
}

void ArrayValueNode::collectUpdates(NodeViewUpdater* dest)
{
    for (const Rc<ValueNode>& node : _values) {
        node->collectUpdates(dest);
    }
    _changedSinceUpdate = false;
}

bool ArrayValueNode::decode(bmcl::MemReader* src)
{
    _changedSinceUpdate = true;
    return ContainerValueNode::decode(src);
}

const Type* ArrayValueNode::type() const
{
    return _type.get();
}

DynArrayValueNode::DynArrayValueNode(const DynArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
    , _minSizeSinceUpdate(0)
    , _lastUpdateSize(0)
{
}

DynArrayValueNode::~DynArrayValueNode()
{
}

void DynArrayValueNode::collectUpdates(NodeViewUpdater* dest)
{
    for (std::size_t i = 0; i < _minSizeSinceUpdate; i++) {
        _values[0]->collectUpdates(dest);
    }
    if (_minSizeSinceUpdate < _lastUpdateSize) {
        //shrink
        dest->addShrinkUpdate(_minSizeSinceUpdate, this);
    }
    if (_values.size() > _minSizeSinceUpdate) {
        //extend
        NodeViewVec vec;
        vec.reserve(_values.size() - _minSizeSinceUpdate);
        for (std::size_t i = _minSizeSinceUpdate; i < _values.size(); i++) {
            vec.emplace_back(new NodeView(_values[i].get()));
        }
        dest->addExtendUpdate(std::move(vec), this);
    }

    _minSizeSinceUpdate = _values.size();
    _lastUpdateSize = _minSizeSinceUpdate;
}

bool DynArrayValueNode::encode(bmcl::MemWriter* dest) const
{
    if (!dest->writeVarUint(_values.size())) {
        //TODO: report error
        return false;
    }
    return ContainerValueNode::encode(dest);
}

bool DynArrayValueNode::decode(bmcl::MemReader* src)
{
    uint64_t size;
    if (!src->readVarUint(&size)) {
        //TODO: report error
        return false;
    }
    if (size > _type->maxSize()) {
        //TODO: report error
        return false;
    }
    resizeDynArray(size);
    return ContainerValueNode::decode(src);
}

const Type* DynArrayValueNode::type() const
{
    return _type.get();
}

std::size_t DynArrayValueNode::maxSize() const
{
    return _type->maxSize();
}

bmcl::Option<std::size_t> DynArrayValueNode::canBeResized() const
{
    return bmcl::Option<std::size_t>(_type->maxSize());
}

bool DynArrayValueNode::resizeNode(std::size_t size)
{
    if (size > _type->maxSize()) {
        return false;
    }
    resizeDynArray(size);
    return true;
}

void DynArrayValueNode::resizeDynArray(std::size_t size)
{
    _minSizeSinceUpdate = std::min(_minSizeSinceUpdate, size);
    std::size_t currentSize = _values.size();
    if (size > currentSize) {
        _values.reserve(size);
        for (std::size_t i = 0; i < (size - currentSize); i++) {
            Rc<ValueNode> node = ValueNode::fromType(_type->elementType(), _cache.get(), this);
            node->setFieldName(_indexCache.arrayIndex(currentSize + i));
            _values.push_back(node);
        }
    } else if (size < currentSize) {
        _values.resize(size);
    }
    assert(values().size() == size);
}

StructValueNode::StructValueNode(const StructType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
    , _changedSinceUpdate(false)
{
    _values.reserve(type->fieldsRange().size());
    for (const Field* field : type->fieldsRange()) {
        Rc<ValueNode> node = ValueNode::fromType(field->type(), _cache.get(), this);
        node->setFieldName(field->name());
        node->setShortDesc(field->shortDescription());
        _values.push_back(node);
    }
}

StructValueNode::~StructValueNode()
{
}

void StructValueNode::collectUpdates(NodeViewUpdater* dest)
{
    for (const Rc<ValueNode>& node : _values) {
        node->collectUpdates(dest);
    }
    _changedSinceUpdate = false;
}

bool StructValueNode::decode(bmcl::MemReader* src)
{
    _changedSinceUpdate = true;
    return ContainerValueNode::decode(src);
}

const Type* StructValueNode::type() const
{
    return _type.get();
}

bmcl::OptionPtr<ValueNode> StructValueNode::nodeWithName(bmcl::StringView name)
{
    for (const Rc<ValueNode>& node : _values) {
        if (node->fieldName() == name) {
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
        return Value::makeStringView(_type->fieldsBegin()[_currentId.unwrap().value()]->name());
    }
    return Value::makeUninitialized();
}

void VariantValueNode::collectUpdates(NodeViewUpdater* dest)
{
    if (_currentId.isNone()) {
        return;
    }

    if (!_currentId.unwrap().hasChanged()) {
        return;
    }

    dest->addShrinkUpdate(std::size_t(0), this);
    NodeViewVec vec;
    vec.reserve(_values.size());
    for (const Rc<ValueNode>& node : _values) {
        vec.emplace_back(new NodeView(node.get()));
    }
    dest->addExtendUpdate(std::move(vec), this);
    _currentId.unwrap().updateState();
}

bool VariantValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_currentId.isNone()) {
        //TODO: report error
        return false;
    }
    TRY(dest->writeVarUint(_currentId.unwrap().value()));
    return ContainerValueNode::encode(dest);
}

bool VariantValueNode::decode(bmcl::MemReader* src)
{
    uint64_t id;
    TRY(src->readVarUint(&id));
    if (id >= _type->fieldsRange().size()) {
        //TODO: report error
        return false;
    }
    updateOptionalValuePair(&_currentId, id);
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
        } else if (size > currentSize) {
            _values.resize(size);
        }
        for (std::size_t i = 0; i < size; i++) {
            _values[i] = ValueNode::fromType(tField->typesBegin()[i], _cache.get(), this);
        }
        TRY(ContainerValueNode::decode(src));
        break;
    }
    case VariantFieldKind::Struct: {
        const StructVariantField* sField = static_cast<const StructVariantField*>(field);
        std::size_t currentSize = _values.size();
        std::size_t size = sField->fieldsRange().size();
        if (size < currentSize) {
            _values.resize(size);
        } else if (size > currentSize) {
            _values.resize(size);
        }
        for (std::size_t i = 0; i < size; i++) {
            const Field* field = sField->fieldsBegin()[i];
            _values[i] = ValueNode::fromType(field->type(), _cache.get(), this);
            _values[i]->setFieldName(field->name());
        }
        TRY(ContainerValueNode::decode(src));
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

StringValueNode::StringValueNode(const DynArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NonContainerValueNode(cache, parent)
    , _type(type)
    , _hasChanged(false)
{
    assert(type->elementType()->isBuiltin());
    assert(type->elementType()->asBuiltin()->builtinTypeKind() == BuiltinTypeKind::Char);
}

StringValueNode::~StringValueNode()
{
}

const Type* StringValueNode::type() const
{
    return _type.get();
}

void StringValueNode::collectUpdates(NodeViewUpdater* dest)
{
    if (_hasChanged) {
        dest->addValueUpdate(value(), this);
        _hasChanged = false;
    }
}

bool StringValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    if (!dest->writeVarUint(_value->size())) {
        //TODO: report error
        return false;
    }
    if (dest->sizeLeft() < _value->size()) {
        //TODO: report error
        return false;
    }
    dest->write(_value->data(), _value->size());
    return true;
}

bool StringValueNode::decode(bmcl::MemReader* src)
{
    uint64_t size;
    if (!src->readVarUint(&size)) {
        //TODO: report error
        return false;
    }
    if (src->sizeLeft() < size) {
        //TODO: report error
        return false;
    }
    if (_value.isNone()) {
        _value.emplace();
    }
    _value->assign((const char*)src->current(), size);
    src->skip(size);
    return true;
}

bool StringValueNode::isInitialized() const
{
    return _value.isSome();
}

Value StringValueNode::value() const
{
    if (_value.isSome()) {
        return Value::makeString(_value.unwrap());
    }
    return Value::makeUninitialized();
}

ValueKind StringValueNode::valueKind() const
{
    return ValueKind::String;
}

bool StringValueNode::setValue(const Value& value)
{
    if (value.kind() == ValueKind::String) {
        _hasChanged = true;
        if (value.asString().size() > _type->maxSize()) {
            //TODO: report error
            return false;
        }
        _value.emplace(value.asString());
        return true;
    } else if (value.kind() == ValueKind::StringView) {
        _hasChanged = true;
        if (value.asStringView().size() > _type->maxSize()) {
            //TODO: report error
            return false;
        }
        _value.emplace(value.asStringView().toStdString());
        return true;
    }
    return false;
}

AddressValueNode::AddressValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NonContainerValueNode(cache, parent)
{
}

AddressValueNode::~AddressValueNode()
{
}

void AddressValueNode::collectUpdates(NodeViewUpdater* dest)
{
    if (_address.isNone()) {
        return;
    }

    if (!_address.unwrap().hasChanged()) {
        return;
    }

    dest->addValueUpdate(Value::makeUnsigned(_address.unwrap().value()), this);

    _address.unwrap().updateState();
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
    dest->writeUint64Le(_address.unwrap().value());
    return true;
}

bool AddressValueNode::decode(bmcl::MemReader* src)
{
    //TODO: get target word size
    if (src->readableSize() < 8) {
        //TODO: report error
        return false;
    }
    uint64_t value = src->readUint64Le();
    updateOptionalValuePair(&_address, value);
    return true;
}

bool AddressValueNode::isInitialized() const
{
    return _address.isSome();
}

Value AddressValueNode::value() const
{
    if (_address.isSome()) {
        return Value::makeUnsigned(_address.unwrap().value());
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
void EnumValueNode::collectUpdates(NodeViewUpdater* dest)
{
    if (_currentId.isNone()) {
        return;
    }

    if (!_currentId.unwrap().hasChanged()) {
        return;
    }

    dest->addValueUpdate(Value::makeSigned(_currentId.unwrap().value()), this);

    _currentId.unwrap().updateState();
}

bool EnumValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_currentId.isSome()) {
        TRY(dest->writeVarUint(_currentId.unwrap().value()));
        return true;
    }
    //TODO: report error
    return false;
}

bool EnumValueNode::decode(bmcl::MemReader* src)
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
    updateOptionalValuePair(&_currentId, value);
    return true;
}

decode::Value EnumValueNode::value() const
{
    if (_currentId.isSome()) {
            auto it = _type->constantsRange().findIf([this](const EnumConstant* c) {
                return c->value() == _currentId.unwrap().value();
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
void NumericValueNode<T>::collectUpdates(NodeViewUpdater* dest)
{
    if (_value.isNone()) {
        return;
    }

    if (!_value.unwrap().hasChanged()) {
        return;
    }

    if (std::is_floating_point<T>::value) {
        dest->addValueUpdate(Value::makeDouble(_value.unwrap().value()), this);
    } else if (std::is_signed<T>::value) {
        dest->addValueUpdate(Value::makeSigned(_value.unwrap().value()), this);
    } else {
        dest->addValueUpdate(Value::makeUnsigned(_value.unwrap().value()), this);
    }

    _value.unwrap().updateState();
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
    dest->writeType<T>(bmcl::htole<T>(_value.unwrap().value()));
    return true;
}

template <typename T>
bool NumericValueNode<T>::decode(bmcl::MemReader* src)
{
    if (src->readableSize() < sizeof(T)) {
        //TODO: report error
        return false;
    }
    T value = bmcl::letoh<T>(src->readType<T>());
    updateOptionalValuePair(&_value, value);
    return true;
}

template <typename T>
decode::Value NumericValueNode<T>::value() const
{
    if (_value.isNone()) {
        return Value::makeUninitialized();
    }
    if (std::is_floating_point<T>::value) {
        return Value::makeDouble(_value.unwrap().value());
    } else if (std::is_signed<T>::value) {
        return Value::makeSigned(_value.unwrap().value());
    } else {
        return Value::makeUnsigned(_value.unwrap().value());
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
    if (std::is_floating_point<T>::value) {
        return ValueKind::Double;
    } else if (std::is_signed<T>::value) {
        return ValueKind::Signed;
    }
    return ValueKind::Unsigned;
}

template <typename T>
bool NumericValueNode<T>::emplace(int64_t value)
{
    if (value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max()) {
        _value.emplace(value);
        return true;
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
bool NumericValueNode<T>::emplace(double value)
{
    if (value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max()) {
        _value.emplace(value);
        return true;
    }
    return false;
}

template <typename T>
bool NumericValueNode<T>::setValue(const Value& value)
{
    if (value.isA(ValueKind::Double)) {
        return emplace(value.asDouble());
    } else if (value.isA(ValueKind::Signed)) {
        return emplace(value.asSigned());
    } else if (value.isA(ValueKind::Unsigned)) {
        return emplace(value.asUnsigned());
    }
    return false;
}

template <typename T>
bmcl::Option<T> NumericValueNode<T>::rawValue() const
{
    if (_value.isSome()) {
        return _value.unwrap().value();
    }
    return bmcl::None;
}

template <typename T>
void NumericValueNode<T>::setRawValue(T value)
{
    updateOptionalValuePair(&_value, value);
}

template class NumericValueNode<std::uint8_t>;
template class NumericValueNode<std::int8_t>;
template class NumericValueNode<std::uint16_t>;
template class NumericValueNode<std::int16_t>;
template class NumericValueNode<std::uint32_t>;
template class NumericValueNode<std::int32_t>;
template class NumericValueNode<std::uint64_t>;
template class NumericValueNode<std::int64_t>;
template class NumericValueNode<float>;
template class NumericValueNode<double>;

VarintValueNode::VarintValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NumericValueNode<int64_t>(type, cache, parent)
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
    return dest->writeVarInt(_value.unwrap().value());
}

bool VarintValueNode::decode(bmcl::MemReader* src)
{
    int64_t value;
    //TODO: report error
    TRY(src->readVarInt(&value));
    updateOptionalValuePair(&_value, value);
    return true;
}

VaruintValueNode::VaruintValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NumericValueNode<uint64_t>(type, cache, parent)
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
    return dest->writeVarUint(_value.unwrap().value());
}

bool VaruintValueNode::decode(bmcl::MemReader* src)
{
    uint64_t value;
    //TODO: report error
    TRY(src->readVarUint(&value));
    updateOptionalValuePair(&_value, value);
    return true;
}
}
