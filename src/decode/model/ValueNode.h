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

class ValueNode : public Node {
public:
    ~ValueNode();

    static Rc<ValueNode> fromType(const Type* type, const ValueInfoCache* cache, Node* parent);

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
    explicit ValueNode(const ValueInfoCache* cache, Node* parent);

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
    Node* childAt(std::size_t idx) override;

    ValueNode* nodeAt(std::size_t index)
    {
        return _values[index].get();
    }

    virtual bmcl::Option<std::size_t> fixedSize() const = 0;

    const std::vector<Rc<ValueNode>> values() const;

protected:
    explicit ContainerValueNode(const ValueInfoCache* cache, Node* parent);

    std::vector<Rc<ValueNode>> _values;
};

class ArrayValueNode : public ContainerValueNode {
public:
    ArrayValueNode(const ArrayType* type, const ValueInfoCache* cache, Node* parent);
    ~ArrayValueNode();

    bmcl::Option<std::size_t> fixedSize() const override;
    const Type* type() const override;

private:
    Rc<const ArrayType> _type;
};

class SliceValueNode : public ContainerValueNode {
public:
    SliceValueNode(const SliceType* type, const ValueInfoCache* cache, Node* parent);
    ~SliceValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;

    bmcl::Option<std::size_t> fixedSize() const override;
    const Type* type() const override;

    void resize(std::size_t size, std::size_t nodeIndex, ModelEventHandler* handler);

private:
    Rc<const SliceType> _type;
};

class StructValueNode : public ContainerValueNode {
public:
    StructValueNode(const StructType* type, const ValueInfoCache* cache, Node* parent);
    ~StructValueNode();

    bmcl::Option<std::size_t> fixedSize() const override;
    const Type* type() const override;
    bmcl::OptionPtr<ValueNode> nodeWithName(bmcl::StringView name);

private:
    Rc<const StructType> _type;
};

class VariantValueNode : public ContainerValueNode {
public:
    VariantValueNode(const VariantType* type, const ValueInfoCache* cache, Node* parent);
    ~VariantValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;

    bmcl::Option<std::size_t> fixedSize() const override;
    const Type* type() const override;
    Value value() const override;

private:
    Rc<const VariantType> _type;
    bmcl::Option<std::size_t> _currentId;
};

class NonContainerValueNode : public ValueNode {
public:
    NonContainerValueNode(const ValueInfoCache* cache, Node* parent);
    bool isContainerValue() const override;
    bool canHaveChildren() const override;
};

class AddressValueNode : public NonContainerValueNode {
public:
    AddressValueNode(const ValueInfoCache* cache, Node* parent);
    ~AddressValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;

    bool isInitialized() const override;
    Value value() const override;

protected:
    bmcl::Option<uint64_t> _address;
};

class ReferenceValueNode : public AddressValueNode {
public:
    ReferenceValueNode(const ReferenceType* type, const ValueInfoCache* cache, Node* parent);
    ~ReferenceValueNode();
    const Type* type() const override;

private:
    Rc<const ReferenceType> _type;
};

class FunctionValueNode : public AddressValueNode {
public:
    FunctionValueNode(const FunctionType* type, const ValueInfoCache* cache, Node* parent);
    ~FunctionValueNode();
    const Type* type() const override;

private:
    Rc<const FunctionType> _type;
};

class EnumValueNode : public NonContainerValueNode {
public:
    EnumValueNode(const EnumType* type, const ValueInfoCache* cache, Node* parent);
    ~EnumValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;
    decode::Value value() const override;

    bool isInitialized() const override;
    const Type* type() const override;

private:
    Rc<const EnumType> _type;
    bmcl::Option<std::size_t> _currentId;
};

class BuiltinValueNode : public NonContainerValueNode {
public:
    ~BuiltinValueNode();

    static Rc<BuiltinValueNode> fromType(const BuiltinType* type, const ValueInfoCache* cache, Node* parent);
    const Type* type() const override;

protected:
    BuiltinValueNode(const BuiltinType* type, const ValueInfoCache* cache, Node* parent);

    Rc<const BuiltinType> _type;
};

template <typename T>
class NumericValueNode : public BuiltinValueNode {
public:
    NumericValueNode(const BuiltinType* type, const ValueInfoCache* cache, Node* parent);
    ~NumericValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;
    decode::Value value() const override;

    bool isInitialized() const override;

private:
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
    VarintValueNode(const BuiltinType* type, const ValueInfoCache* cache, Node* parent);
    ~VarintValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;
    Value value() const override;

    bool isInitialized() const override;

private:
    bmcl::Option<int64_t> _value;
};

class VaruintValueNode : public BuiltinValueNode {
public:
    VaruintValueNode(const BuiltinType* type, const ValueInfoCache* cache, Node* parent);
    ~VaruintValueNode();

    bool encode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemWriter* dest) const override;
    bool decode(std::size_t nodeIndex, ModelEventHandler* handler, bmcl::MemReader* src) override;
    Value value() const override;

    bool isInitialized() const override;

private:
    bmcl::Option<uint64_t> _value;
};
}
