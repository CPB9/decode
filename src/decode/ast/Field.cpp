/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/Field.h"
#include "decode/ast/Type.h"
#include "decode/core/RangeAttr.h"

namespace decode {

Field::Field(bmcl::StringView name, Type* type)
    : NamedRc(name)
    , _type(type)
{
}

Field::~Field()
{
}

const Type* Field::type() const
{
    return _type.get();
}

Type* Field::type()
{
    return _type.get();
}

bmcl::OptionPtr<const RangeAttr> Field::rangeAttribute() const
{
    return _rangeAttr.get();
}

bmcl::OptionPtr<RangeAttr> Field::rangeAttribute()
{
    return _rangeAttr.get();
}

void Field::setRangeAttribute(RangeAttr* attr)
{
    _rangeAttr.reset(attr);
}

VariantField::VariantField(VariantFieldKind kind, std::uintmax_t id, bmcl::StringView name)
    : NamedRc(name)
    , _variantFieldKind(kind)
    , _id(id)
{
}

VariantField::~VariantField()
{
}

VariantFieldKind VariantField::variantFieldKind() const
{
    return _variantFieldKind;
}

std::uintmax_t VariantField::id() const
{
    return _id;
}

const ConstantVariantField* VariantField::asConstantField() const
{
    assert(_variantFieldKind == VariantFieldKind::Constant);
    return static_cast<const ConstantVariantField*>(this);
}

const TupleVariantField* VariantField::asTupleField() const
{
    assert(_variantFieldKind == VariantFieldKind::Tuple);
    return static_cast<const TupleVariantField*>(this);
}

const StructVariantField* VariantField::asStructField() const
{
    assert(_variantFieldKind == VariantFieldKind::Struct);
    return static_cast<const StructVariantField*>(this);
}

ConstantVariantField* VariantField::asConstantField()
{
    assert(_variantFieldKind == VariantFieldKind::Constant);
    return static_cast<ConstantVariantField*>(this);
}

TupleVariantField* VariantField::asTupleField()
{
    assert(_variantFieldKind == VariantFieldKind::Tuple);
    return static_cast<TupleVariantField*>(this);
}

StructVariantField* VariantField::asStructField()
{
    assert(_variantFieldKind == VariantFieldKind::Struct);
    return static_cast<StructVariantField*>(this);
}

ConstantVariantField::ConstantVariantField(std::uintmax_t id, bmcl::StringView name)
    : VariantField(VariantFieldKind::Constant, id, name)
{
}

ConstantVariantField::~ConstantVariantField()
{
}

StructVariantField::StructVariantField(std::uintmax_t id, bmcl::StringView name)
    : VariantField(VariantFieldKind::Struct, id, name)
{
}

StructVariantField::~StructVariantField()
{
}

FieldVec::ConstIterator StructVariantField::fieldsBegin() const
{
    return _fields.cbegin();
}

FieldVec::ConstIterator StructVariantField::fieldsEnd() const
{
    return _fields.cend();
}

FieldVec::ConstRange StructVariantField::fieldsRange() const
{
    return _fields;
}

FieldVec::Iterator StructVariantField::fieldsBegin()
{
    return _fields.begin();
}

FieldVec::Iterator StructVariantField::fieldsEnd()
{
    return _fields.end();
}

FieldVec::Range StructVariantField::fieldsRange()
{
    return _fields;
}

void StructVariantField::addField(Field* field)
{
    _fields.emplace_back(field);
}

const Field* StructVariantField::fieldAt(std::size_t index) const
{
    return _fields[index].get();
}

TupleVariantField::TupleVariantField(std::uintmax_t id, bmcl::StringView name)
    : VariantField(VariantFieldKind::Tuple, id, name)
{
}

TupleVariantField::~TupleVariantField()
{
}

TypeVec::ConstIterator TupleVariantField::typesBegin() const
{
    return _types.cbegin();
}

TypeVec::ConstIterator TupleVariantField::typesEnd() const
{
    return _types.cend();
}

TypeVec::ConstRange TupleVariantField::typesRange() const
{
    return _types;
}

TypeVec::Iterator TupleVariantField::typesBegin()
{
    return _types.begin();
}

TypeVec::Iterator TupleVariantField::typesEnd()
{
    return _types.end();
}

TypeVec::Range TupleVariantField::typesRange()
{
    return _types;
}

void TupleVariantField::addType(Type* type)
{
    _types.emplace_back(type);
}
}
