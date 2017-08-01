/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/Field.h"
#include "decode/ast/Type.h"

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

VariantField::VariantField(VariantFieldKind kind, bmcl::StringView name)
    : NamedRc(name)
    , _variantFieldKind(kind)
{
}

VariantField::~VariantField()
{
}

VariantFieldKind VariantField::variantFieldKind() const
{
    return _variantFieldKind;
}

ConstantVariantField::ConstantVariantField(bmcl::StringView name)
    : VariantField(VariantFieldKind::Constant, name)
{
}

ConstantVariantField::~ConstantVariantField()
{
}

StructVariantField::StructVariantField(bmcl::StringView name)
    : VariantField(VariantFieldKind::Struct, name)
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

TupleVariantField::TupleVariantField(bmcl::StringView name)
    : VariantField(VariantFieldKind::Tuple, name)
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
