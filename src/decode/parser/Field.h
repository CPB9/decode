#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/NamedRc.h"

#include <bmcl/StringView.h>
#include <bmcl/Option.h>

#include <vector>

namespace decode {

class Type;

enum class VariantFieldKind {
    Constant,
    Tuple,
    Struct
};

class Field : public NamedRc {
public:
    Field(bmcl::StringView name, Type* type)
        : NamedRc(name)
        , _type(type)
    {
    }

    const Type* type() const
    {
        return _type.get();
    }

    Type* type()
    {
        return _type.get();
    }

private:

    Rc<Type> _type;
};

template<typename T>
class RefCountableVector : public std::vector<T>, public RefCountable { //TODO: remove Rc
};

class FieldList : public RefCountableVector<Rc<Field>> {
public:
    bmcl::Option<const Rc<Field>&> fieldWithName(bmcl::StringView name)
    {
        for (const Rc<Field>& value : *this) {
            if (value->name() == name) {
                return value;
            }
        }
        return bmcl::None;
    }
};

class VariantField : public NamedRc {
public:
    VariantField(VariantFieldKind kind, bmcl::StringView name)
        : NamedRc(name)
        , _variantFieldKind(kind)
    {
    }

    VariantFieldKind variantFieldKind() const
    {
        return _variantFieldKind;
    }

private:
    VariantFieldKind _variantFieldKind;
};

class ConstantVariantField : public VariantField {
public:
    ConstantVariantField(bmcl::StringView name)
        : VariantField(VariantFieldKind::Constant, name)
    {
    }
};

class StructVariantField : public VariantField {
public:
    StructVariantField(bmcl::StringView name)
        : VariantField(VariantFieldKind::Struct, name)
        , _fields(new FieldList)
    {
    }

    const FieldList* fields() const
    {
        return _fields.get();
    }

    FieldList* fields()
    {
        return _fields.get();
    }

private:
    Rc<FieldList> _fields;
};

class TupleVariantField : public VariantField {
public:
    TupleVariantField(bmcl::StringView name)
        : VariantField(VariantFieldKind::Tuple, name)
    {
    }

    const std::vector<Rc<Type>>& types() const
    {
        return _types;
    }

    void addType(Type* type)
    {
        _types.emplace_back(type);
    }

private:
    std::vector<Rc<Type>> _types;
};

}
