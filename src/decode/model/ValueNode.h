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
#include "decode/parser/Type.h"
#include "decode/model/Node.h"

#include <bmcl/Option.h>
#include <bmcl/OptionPtr.h>

#include <string>

namespace bmcl { class MemReader; class MemWriter; }

namespace decode {

class ValueInfoCache;
class ModelEventHandler;
class Function;


class ValueNode : public Node {
public:
    ~ValueNode();

    static Rc<ValueNode> fromType(const Type* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    virtual bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const = 0;
    virtual bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) = 0;

    virtual bool isContainerValue() const = 0;
    virtual bool isInitialized() const = 0;
    virtual const Type* type() const = 0;

    bmcl::StringView typeName() const override;
    bmcl::StringView fieldName() const override;

    void setFieldName(bmcl::StringView name)
    {
        _fieldName = name;
    }

protected:
    explicit ValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    Rc<const ValueInfoCache> _cache;

private:
    bmcl::StringView _fieldName;
};

class ContainerValueNode : public ValueNode {
public:
    ~ContainerValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;

    bool isContainerValue() const override;
    bool isInitialized() const override;

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;

    ValueNode* nodeAt(std::size_t index)
    {
        return _values[index].get();
    }

    const std::vector<Rc<ValueNode>> values() const;

protected:
    explicit ContainerValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    std::vector<Rc<ValueNode>> _values;
};

class ArrayValueNode : public ContainerValueNode {
public:
    ArrayValueNode(const ArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~ArrayValueNode();

    const Type* type() const override;

private:
    Rc<const ArrayType> _type;
};

class SliceValueNode : public ContainerValueNode {
public:
    SliceValueNode(const SliceType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~SliceValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;

    const Type* type() const override;

    void resize(std::size_t size, std::size_t nodeIndex, ModelEventHandler* handler);

private:
    Rc<const SliceType> _type;
};

class StructValueNode : public ContainerValueNode {
public:
    StructValueNode(const StructType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~StructValueNode();

    const Type* type() const override;
    bmcl::OptionPtr<ValueNode> nodeWithName(bmcl::StringView name);

private:
    Rc<const StructType> _type;
};

class VariantValueNode : public ContainerValueNode {
public:
    VariantValueNode(const VariantType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~VariantValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;

    const Type* type() const override;
    Value value() const override;

private:
    Rc<const VariantType> _type;
    bmcl::Option<std::uint64_t> _currentId;
};

class NonContainerValueNode : public ValueNode {
public:
    bool isContainerValue() const override;
    bool canHaveChildren() const override;

    bool canSetValue() const override;

protected:
    NonContainerValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
};

class AddressValueNode : public NonContainerValueNode {
public:
    AddressValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~AddressValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;

    bool isInitialized() const override;
    Value value() const override;

    ValueKind valueKind() const override;
    bool setValue(const Value& value) override;

protected:
    bmcl::Option<uint64_t> _address;
};

class ReferenceValueNode : public AddressValueNode {
public:
    ReferenceValueNode(const ReferenceType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~ReferenceValueNode();
    const Type* type() const override;

private:
    Rc<const ReferenceType> _type;
};

class FunctionValueNode : public AddressValueNode {
public:
    FunctionValueNode(const FunctionType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~FunctionValueNode();
    const Type* type() const override;

private:
    Rc<const FunctionType> _type;
};

class EnumValueNode : public NonContainerValueNode {
public:
    EnumValueNode(const EnumType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~EnumValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;
    decode::Value value() const override;

    bool isInitialized() const override;
    const Type* type() const override;

    //TODO: implement enum editing
    //ValueKind valueKind() const override;
    //bool setValue(const Value& value) override;

private:
    Rc<const EnumType> _type;
    bmcl::Option<int64_t> _currentId;
};

class BuiltinValueNode : public NonContainerValueNode {
public:
    ~BuiltinValueNode();

    static Rc<BuiltinValueNode> fromType(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    const Type* type() const override;

protected:
    BuiltinValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    Rc<const BuiltinType> _type;
};

template <typename T>
class NumericValueNode : public BuiltinValueNode {
public:
    NumericValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~NumericValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;
    decode::Value value() const override;

    bool isInitialized() const override;

    ValueKind valueKind() const override;
    bool setValue(const Value& value) override;

private:
    bool emplace(int64_t value);
    bool emplace(uint64_t value);

    bmcl::Option<T> _value;
};

extern template class NumericValueNode<std::uint8_t>;
extern template class NumericValueNode<std::int8_t>;
extern template class NumericValueNode<std::uint16_t>;
extern template class NumericValueNode<std::int16_t>;
extern template class NumericValueNode<std::uint32_t>;
extern template class NumericValueNode<std::int32_t>;
extern template class NumericValueNode<std::uint64_t>;
extern template class NumericValueNode<std::int64_t>;

class VarintValueNode : public BuiltinValueNode {
public:
    VarintValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~VarintValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;
    Value value() const override;

    bool isInitialized() const override;

    ValueKind valueKind() const override;
    bool setValue(const Value& value) override;

private:
    bmcl::Option<int64_t> _value;
};

class VaruintValueNode : public BuiltinValueNode {
public:
    VaruintValueNode(const BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~VaruintValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;
    Value value() const override;

    bool isInitialized() const override;

    ValueKind valueKind() const override;
    bool setValue(const Value& value) override;

private:
    bmcl::Option<uint64_t> _value;
};

}
