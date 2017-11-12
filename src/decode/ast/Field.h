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
#include "decode/ast/DocBlockMixin.h"

#include <bmcl/StringView.h>

#include <vector>

namespace decode {

class Type;
class RangeAttr;

enum class VariantFieldKind {
    Constant,
    Tuple,
    Struct
};

class Field : public NamedRc, public DocBlockMixin {
public:
    using Pointer = Rc<Field>;
    using ConstPointer = Rc<const Field>;

    Field(bmcl::StringView name, Type* type);
    ~Field();

    const Type* type() const;
    Type* type();

    bmcl::OptionPtr<const RangeAttr> rangeAttribute() const;
    bmcl::OptionPtr<RangeAttr> rangeAttribute();
    void setRangeAttribute(RangeAttr* attr);

private:
    Rc<Type> _type;
    Rc<RangeAttr> _rangeAttr;
};

class ConstantVariantField;
class TupleVariantField;
class StructVariantField;

class VariantField : public NamedRc, public DocBlockMixin {
public:
    using Pointer = Rc<VariantField>;
    using ConstPointer = Rc<const VariantField>;

    VariantField(VariantFieldKind kind, std::uintmax_t id, bmcl::StringView name);
    ~VariantField();

    VariantFieldKind variantFieldKind() const;
    std::uintmax_t id() const;

    const ConstantVariantField* asConstantField() const;
    const TupleVariantField* asTupleField() const;
    const StructVariantField* asStructField() const;

    ConstantVariantField* asConstantField();
    TupleVariantField* asTupleField();
    StructVariantField* asStructField();

private:
    VariantFieldKind _variantFieldKind;
    std::uintmax_t _id;
};

class ConstantVariantField : public VariantField {
public:
    using Pointer = Rc<ConstantVariantField>;
    using ConstPointer = Rc<const ConstantVariantField>;

    ConstantVariantField(std::uintmax_t id, bmcl::StringView name);
    ~ConstantVariantField();
};

class StructVariantField : public VariantField {
public:
    using Pointer = Rc<StructVariantField>;
    using ConstPointer = Rc<const StructVariantField>;

    StructVariantField(std::uintmax_t id, bmcl::StringView name);
    ~StructVariantField();

    FieldVec::ConstIterator fieldsBegin() const;
    FieldVec::ConstIterator fieldsEnd() const;
    FieldVec::ConstRange fieldsRange() const;
    FieldVec::Iterator fieldsBegin();
    FieldVec::Iterator fieldsEnd();
    FieldVec::Range fieldsRange();

    void addField(Field* field);

    const Field* fieldAt(std::size_t index) const;

private:
    FieldVec _fields;
};

class TupleVariantField : public VariantField {
public:
    using Pointer = Rc<TupleVariantField>;
    using ConstPointer = Rc<const TupleVariantField>;

    TupleVariantField(std::uintmax_t id, bmcl::StringView name);
    ~TupleVariantField();

    TypeVec::ConstIterator typesBegin() const;
    TypeVec::ConstIterator typesEnd() const;
    TypeVec::ConstRange typesRange() const;
    TypeVec::Iterator typesBegin();
    TypeVec::Iterator typesEnd();
    TypeVec::Range typesRange();

    void addType(Type* type);

private:
    TypeVec _types;
};

template <typename T>
inline VariantFieldKind deferVariantFieldKind();

template <>
inline VariantFieldKind deferVariantFieldKind<ConstantVariantField>()
{
    return VariantFieldKind::Constant;
}

template <>
inline VariantFieldKind deferVariantFieldKind<TupleVariantField>()
{
    return VariantFieldKind::Tuple;
}

template <>
inline VariantFieldKind deferVariantFieldKind<StructVariantField>()
{
    return VariantFieldKind::Struct;
}
}
