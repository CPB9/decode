#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/parser/Type.h"

#include <bmcl/Option.h>

namespace bmcl { class MemReader; class MemWriter; }

namespace decode {

class Value : public RefCountable {
public:
    ~Value();

    static Rc<Value> fromType(const Rc<Type>& type);

    virtual bool encode(bmcl::MemWriter* dest) const = 0;
    virtual bool decode(bmcl::MemReader* src) = 0;

    virtual bool isContainer() const = 0;
    virtual bool isInitialized() const = 0;

protected:
    virtual Type* type() = 0;
};

class ContainerValue : public Value {
public:
    ContainerValue();
    ContainerValue(const Rc<Type>& type, std::size_t count);
    ~ContainerValue();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bool isContainer() const override;
    bool isInitialized() const override;

    virtual bmcl::Option<std::size_t> fixedSize() const = 0;

    const std::vector<Rc<Value>> values() const;

protected:
    std::vector<Rc<Value>> _values;
};

class ArrayValue : public ContainerValue {
public:
    ArrayValue(const Rc<ArrayType>& type);
    ~ArrayValue();

    bmcl::Option<std::size_t> fixedSize() const override;

protected:
    Type* type() override;

private:
    Rc<ArrayType> _type;
};

class SliceValue : public ContainerValue {
public:
    SliceValue(const Rc<SliceType>& type);
    ~SliceValue();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bmcl::Option<std::size_t> fixedSize() const override;

protected:
    Type* type() override;

private:
    Rc<SliceType> _type;
};

class StructValue : public ContainerValue {
public:
    StructValue(const Rc<StructType>& type);
    ~StructValue();

    bmcl::Option<std::size_t> fixedSize() const override;

protected:
    Type* type() override;

private:
    Rc<StructType> _type;
};

class VariantValue : public ContainerValue {
public:
    VariantValue(const Rc<VariantType>& type);
    ~VariantValue();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bmcl::Option<std::size_t> fixedSize() const override;

protected:
    Type* type() override;

private:
    Rc<VariantType> _type;
    std::size_t _currentId;
};

class NonContainerValue : public Value {
public:
    bool isContainer() const override;
};

class AddressValue : public NonContainerValue {
public:
    AddressValue();
    ~AddressValue();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bool isInitialized() const override;

protected:
    bmcl::Option<uint64_t> _address;
};

class ReferenceValue : public AddressValue {
public:
    ReferenceValue(const Rc<ReferenceType>& type);
    ~ReferenceValue();


protected:
    Type* type() override;

private:
    Rc<ReferenceType> _type;
};

class FunctionValue : public AddressValue {
public:
    FunctionValue(const Rc<FunctionType>& type);
    ~FunctionValue();


protected:
    Type* type() override;

private:
    Rc<FunctionType> _type;
};

class EnumValue : public NonContainerValue {
public:
    EnumValue(const Rc<EnumType>& type);
    ~EnumValue();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bool isInitialized() const override;

protected:
    Type* type() override;

private:
    Rc<EnumType> _type;
    bmcl::Option<std::size_t> _currentId;
};

class BuiltinValue : public NonContainerValue {
public:
    BuiltinValue(const Rc<BuiltinType>& type);
    ~BuiltinValue();

    static Rc<BuiltinValue> fromType(const Rc<BuiltinType>& type);

protected:
    Type* type() override;

    Rc<BuiltinType> _type;
};

template <typename T>
class NumericValue : public BuiltinValue {
public:
    NumericValue(const Rc<BuiltinType>& type);
    ~NumericValue();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bool isInitialized() const override;

private:
    bmcl::Option<T> _value;
};

extern template class NumericValue<std::uint8_t>;
extern template class NumericValue<std::int8_t>;
extern template class NumericValue<std::uint16_t>;
extern template class NumericValue<std::int16_t>;
extern template class NumericValue<std::uint32_t>;
extern template class NumericValue<std::int32_t>;
extern template class NumericValue<std::uint64_t>;
extern template class NumericValue<std::int64_t>;

class VarintValue : public BuiltinValue {
public:
    VarintValue(const Rc<BuiltinType>& type);
    ~VarintValue();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bool isInitialized() const override;

private:
    bmcl::Option<int64_t> _value;
};

class VaruintValue : public BuiltinValue {
public:
    VaruintValue(const Rc<BuiltinType>& type);
    ~VaruintValue();

    bool encode(bmcl::MemWriter* dest) const override;
    bool decode(bmcl::MemReader* src) override;

    bool isInitialized() const override;

private:
    bmcl::Option<uint64_t> _value;
};
}
