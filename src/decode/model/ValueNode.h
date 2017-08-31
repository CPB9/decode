/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/model/Node.h"
#include "decode/model/ValueInfoCache.h"

#include <bmcl/Option.h>
#include <bmcl/OptionPtr.h>

#include <string>

namespace bmcl { class MemReader; class MemWriter; }

namespace decode {

class ValueInfoCache;
class Function;
class NodeViewUpdater;
class ArrayType;
class DynArrayType;
class StructType;
class FunctionType;
class BuiltinType;
class AliasType;
class ImportedType;
class VariantType;
class EnumType;
class ReferenceType;
class RangeAttr;
class Field;

class ValueNode : public Node {
public:
    using Pointer = Rc<ValueNode>;
    using ConstPointer = Rc<const ValueNode>;

    ~ValueNode();

    static Rc<ValueNode> fromType(const Type* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    static Rc<ValueNode> fromField(const Field* field, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    virtual bool encode(bmcl::MemWriter* dest) const = 0;
    virtual bool decode(bmcl::MemReader* src) = 0;

    virtual bool isContainerValue() const = 0;
    virtual bool isInitialized() const = 0;
    virtual const Type* type() const = 0;

    bmcl::StringView typeName() const override;
    bmcl::StringView fieldName() const override;
    bmcl::StringView shortDescription() const override;

    void setFieldName(bmcl::StringView name);
    void setShortDesc(bmcl::StringView desc);

    const ValueInfoCache* cache() const
    {
        return _cache.get();
    }

protected:

    explicit ValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    Rc<const ValueInfoCache> _cache;

private:
    bmcl::StringView _fieldName;
    bmcl::StringView _shortDesc;
};

class ContainerValueNode : public ValueNode {
public:
    using Pointer = Rc<ContainerValueNode>;
    using ConstPointer = Rc<const ContainerValueNode>;

    ~ContainerValueNode();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bool isContainerValue() const override;
    bool isInitialized() const override;

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;

    const ValueNode* nodeAt(std::size_t index) const;
    ValueNode* nodeAt(std::size_t index);

    const std::vector<Rc<ValueNode>> values();

protected:
    explicit ContainerValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    std::vector<Rc<ValueNode>> _values;
};

class ArrayValueNode : public ContainerValueNode {
public:
    using Pointer = Rc<ArrayValueNode>;
    using ConstPointer = Rc<const ArrayValueNode>;

    ArrayValueNode(const ArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~ArrayValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;
    bool decode(bmcl::MemReader* src) override;
    const Type* type() const override;

private:
    Rc<const ArrayType> _type;
    StrIndexCache _indexCache; //TODO: share
    bool _changedSinceUpdate;
};

class DynArrayValueNode : public ContainerValueNode {
public:
    using Pointer = Rc<DynArrayValueNode>;
    using ConstPointer = Rc<const DynArrayValueNode>;

    DynArrayValueNode(const DynArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~DynArrayValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    const Type* type() const override;

    std::size_t maxSize() const;
    bmcl::Option<std::size_t> canBeResized() const override;
    bool resizeNode(std::size_t size) override;
    void resizeDynArray(std::size_t size);

private:
    Rc<const DynArrayType> _type;
    StrIndexCache _indexCache; //TODO: share
    std::size_t _minSizeSinceUpdate;
    std::size_t _lastUpdateSize;
};

class StructValueNode : public ContainerValueNode {
public:
    using Pointer = Rc<StructValueNode>;
    using ConstPointer = Rc<const StructValueNode>;

    StructValueNode(const StructType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~StructValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;
    bool decode(bmcl::MemReader* src) override;

    const Type* type() const override;
    bmcl::OptionPtr<ValueNode> nodeWithName(bmcl::StringView name);

private:
    Rc<const StructType> _type;
    bool _changedSinceUpdate;
};

template <typename T>
class ValuePair {
public:
    ValuePair(T value)
        : _current(value)
        , _previous(value)
        , _neverCollected(true)
    {
    }

    void updateState()
    {
        _neverCollected = false;
        _previous = _current;
    }

    bool hasChanged() const
    {
        return _neverCollected || (_previous != _current);
    }

    void setValue(T value)
    {
        _current = value;
    }

    T value() const
    {
        return _current;
    }

private:
    T _current;
    T _previous;
    bool _neverCollected;
};

class VariantValueNode : public ContainerValueNode {
public:
    using Pointer = Rc<VariantValueNode>;
    using ConstPointer = Rc<const VariantValueNode>;

    VariantValueNode(const VariantType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~VariantValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    const Type* type() const override;
    Value value() const override;

private:
    Rc<const VariantType> _type;
    bmcl::Option<ValuePair<std::uint64_t>> _currentId;
};

class NonContainerValueNode : public ValueNode {
public:
    using Pointer = Rc<NonContainerValueNode>;
    using ConstPointer = Rc<const NonContainerValueNode>;

    bool isContainerValue() const override;
    bool canHaveChildren() const override;

    bool canSetValue() const override;

protected:
    NonContainerValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
};

class StringValueNode : public NonContainerValueNode {
public:
    StringValueNode(const DynArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~StringValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bool isInitialized() const override;
    Value value() const override;

    ValueKind valueKind() const override;
    bool setValue(const Value& value) override;

    const Type * type() const override;

private:
    Rc<const DynArrayType> _type;
    bmcl::Option<std::string> _value;
    bool _hasChanged;
};

class AddressValueNode : public NonContainerValueNode {
public:
    using Pointer = Rc<AddressValueNode>;
    using ConstPointer = Rc<const AddressValueNode>;

    AddressValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~AddressValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bool isInitialized() const override;
    Value value() const override;

    ValueKind valueKind() const override;
    bool setValue(const Value& value) override;

protected:
    bmcl::Option<ValuePair<uint64_t>> _address;
};

class ReferenceValueNode : public AddressValueNode {
public:
    using Pointer = Rc<ReferenceValueNode>;
    using ConstPointer = Rc<const ReferenceValueNode>;

    ReferenceValueNode(const ReferenceType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~ReferenceValueNode();
    const Type* type() const override;

private:
    Rc<const ReferenceType> _type;
};

class FunctionValueNode : public AddressValueNode {
public:
    using Pointer = Rc<FunctionValueNode>;
    using ConstPointer = Rc<const FunctionValueNode>;

    FunctionValueNode(const FunctionType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~FunctionValueNode();
    const Type* type() const override;

private:
    Rc<const FunctionType> _type;
};

class EnumValueNode : public NonContainerValueNode {
public:
    using Pointer = Rc<EnumValueNode>;
    using ConstPointer = Rc<const EnumValueNode>;

    EnumValueNode(const EnumType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~EnumValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;
    decode::Value value() const override;

    bool isInitialized() const override;
    const Type* type() const override;

    //TODO: implement enum editing
    //ValueKind valueKind() const override;
    //bool setValue(const Value& value) override;

private:
    Rc<const EnumType> _type;
    bmcl::Option<ValuePair<int64_t>> _currentId;
};

class BuiltinValueNode : public NonContainerValueNode {
public:
    using Pointer = Rc<BuiltinValueNode>;
    using ConstPointer = Rc<const BuiltinValueNode>;

    ~BuiltinValueNode();

    static Rc<BuiltinValueNode> fromType(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    const Type* type() const override;

    void setRangeAttribute(const RangeAttr* attr);

protected:
    BuiltinValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    Rc<const RangeAttr> _rangeAttr;
    Rc<const BuiltinType> _type;
};

template <typename T>
class NumericValueNode : public BuiltinValueNode {
public:
    using Pointer = Rc<NumericValueNode<T>>;
    using ConstPointer = Rc<const NumericValueNode<T>>;

    NumericValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~NumericValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;
    decode::Value value() const override;

    bool isInitialized() const override;

    ValueKind valueKind() const override;
    bool setValue(const Value& value) override;

    bool isDefault() const override;
    bool isInRange() const override;

    bmcl::Option<T> rawValue() const;
    void setRawValue(T value);

protected:
    bool emplace(int64_t value);
    bool emplace(uint64_t value);
    bool emplace(double value);

    bmcl::Option<ValuePair<T>> _value;
};

extern template class NumericValueNode<std::uint8_t>;
extern template class NumericValueNode<std::int8_t>;
extern template class NumericValueNode<std::uint16_t>;
extern template class NumericValueNode<std::int16_t>;
extern template class NumericValueNode<std::uint32_t>;
extern template class NumericValueNode<std::int32_t>;
extern template class NumericValueNode<std::uint64_t>;
extern template class NumericValueNode<std::int64_t>;
extern template class NumericValueNode<float>;
extern template class NumericValueNode<double>;

class VarintValueNode : public NumericValueNode<std::int64_t> {
public:
    using Pointer = Rc<VarintValueNode>;
    using ConstPointer = Rc<const VarintValueNode>;

    VarintValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~VarintValueNode();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;
};

class VaruintValueNode : public NumericValueNode<std::uint64_t> {
public:
    using Pointer = Rc<VaruintValueNode>;
    using ConstPointer = Rc<const VaruintValueNode>;

    VaruintValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~VaruintValueNode();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;
};
}
