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

    const Rc<Type>& type() const
    {
        return _type;
    }

protected:
    Field() = default;

private:
    friend class Parser;

    Rc<Type> _type;
};

template<typename T>
class RefCountableVector : public std::vector<T>, public RefCountable {
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

    VariantFieldKind variantFieldKind() const
    {
        return _variantFieldKind;
    }

protected:
    VariantField(VariantFieldKind kind)
        : _variantFieldKind(kind)
    {
    }

    VariantFieldKind _variantFieldKind;

private:
    friend class Parser;
};

class ConstantVariantField : public VariantField {
protected:
    ConstantVariantField()
        : VariantField(VariantFieldKind::Constant)
    {
    }

private:
    friend class Parser;
};

class StructVariantField : public VariantField {
public:
    const Rc<FieldList>& fields() const
    {
        return _fields;
    }

protected:
    StructVariantField()
        : VariantField(VariantFieldKind::Struct)
    {
    }

private:
    friend class Parser;

    Rc<FieldList> _fields;
};

class TupleVariantField : public VariantField {
public:

    const std::vector<Rc<Type>>& types() const
    {
        return _types;
    }

protected:
    TupleVariantField()
        : VariantField(VariantFieldKind::Tuple)
    {
    }

private:
    friend class Parser;

    std::vector<Rc<Type>> _types;
};

}
