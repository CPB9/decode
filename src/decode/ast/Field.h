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

enum class VariantFieldKind {
    Constant,
    Tuple,
    Struct
};

class Field : public NamedRc, public DocBlockMixin {
public:
    Field(bmcl::StringView name, Type* type);
    ~Field();

    const Type* type() const;
    Type* type();

private:
    Rc<Type> _type;
};

class VariantField : public NamedRc, public DocBlockMixin {
public:
    VariantField(VariantFieldKind kind, bmcl::StringView name);
    ~VariantField();

    VariantFieldKind variantFieldKind() const;

private:
    VariantFieldKind _variantFieldKind;
};

class ConstantVariantField : public VariantField {
public:
    ConstantVariantField(bmcl::StringView name);
    ~ConstantVariantField();
};

class StructVariantField : public VariantField {
public:
    StructVariantField(bmcl::StringView name);
    ~StructVariantField();

    FieldVec::ConstIterator fieldsBegin() const;
    FieldVec::ConstIterator fieldsEnd() const;
    FieldVec::ConstRange fieldsRange() const;
    FieldVec::Iterator fieldsBegin();
    FieldVec::Iterator fieldsEnd();
    FieldVec::Range fieldsRange();

    void addField(Field* field);

private:
    FieldVec _fields;
};

class TupleVariantField : public VariantField {
public:
    TupleVariantField(bmcl::StringView name);
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
}
