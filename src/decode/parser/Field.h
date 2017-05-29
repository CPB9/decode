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
#include "decode/core/Iterator.h"
#include "decode/core/NamedRc.h"
#include "decode/parser/Containers.h"

#include <bmcl/StringView.h>

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
    {
    }

    FieldVec::ConstIterator fieldsBegin() const
    {
        return _fields.cbegin();
    }

    FieldVec::ConstIterator fieldsEnd() const
    {
        return _fields.cend();
    }

    FieldVec::ConstRange fieldsRange() const
    {
        return _fields;
    }

    FieldVec::Iterator fieldsBegin()
    {
        return _fields.begin();
    }

    FieldVec::Iterator fieldsEnd()
    {
        return _fields.end();
    }

    FieldVec::Range fieldsRange()
    {
        return _fields;
    }

    void addField(Field* field)
    {
        _fields.emplace_back(field);
    }

private:
    FieldVec _fields;
};

class TupleVariantField : public VariantField {
public:
    TupleVariantField(bmcl::StringView name)
        : VariantField(VariantFieldKind::Tuple, name)
    {
    }

    TypeVec::ConstIterator typesBegin() const
    {
        return _types.cbegin();
    }

    TypeVec::ConstIterator typesEnd() const
    {
        return _types.cend();
    }

    TypeVec::ConstRange typesRange() const
    {
        return _types;
    }

    TypeVec::Iterator typesBegin()
    {
        return _types.begin();
    }

    TypeVec::Iterator typesEnd()
    {
        return _types.end();
    }

    TypeVec::Range typesRange()
    {
        return _types;
    }

    void addType(Type* type)
    {
        _types.emplace_back(type);
    }

private:
    TypeVec _types;
};
}
