/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/Function.h"
#include "decode/ast/Type.h"
#include "decode/ast/Field.h"
#include "decode/ast/Component.h"
#include "decode/core/EncodedSizes.h"

namespace decode {

Function::Function(bmcl::StringView name, FunctionType* type)
    : NamedRc(name)
    , _type(type)
{
}

const FunctionType* Function::type() const
{
    return _type.get();
}

FunctionType* Function::type()
{
    return _type.get();
}

FieldVec::ConstRange Function::fieldsRange() const
{
    return _type->argumentsRange();
}

const Field* Function::fieldAt(std::size_t index) const
{
    return _type->argumentsBegin()[index]; //HACK
}

Function::~Function()
{
}

CmdArgument::CmdArgument(Field* field, CmdArgPassKind kind)
    : _field(field)
    , _argPassKind(kind)
{
}

CmdArgument::~CmdArgument()
{
}

const Field* CmdArgument::field() const
{
    return _field.get();
}

Field* CmdArgument::field()
{
    return _field.get();
}

CmdArgPassKind CmdArgument::argPassKind() const
{
    return _argPassKind;
}

void CmdArgument::setArgPassKind(CmdArgPassKind kind)
{
    _argPassKind = kind;
}

const Type* CmdArgument::type() const
{
    return _field->type();
}

Type* CmdArgument::type()
{
    return _field->type();
}

bmcl::StringView CmdArgument::name() const
{
    return _field->name();
}

Command::Command(bmcl::StringView name, FunctionType* type)
    : Function(name, type)
    , _number(0)
{
    _args.reserve(type->argumentsRange().size());
    for (Field* field : type->argumentsRange()) {
        _args.emplace_back(field, CmdArgPassKind::Default);
    }
}

Command::~Command()
{
}

std::uintmax_t Command::number() const
{
    return _number;
}

void Command::setNumber(std::uintmax_t num)
{
    _number = num;
}

Command::ArgsRange Command::argumentsRange()
{
    return _args;
}

Command::ArgsConstRange Command::argumentsRange() const
{
    return _args;
}

EncodedSizes Command::encodedSizes() const
{
    EncodedSizes sizes(2);
    for (const CmdArgument &arg : argumentsRange()) {
        sizes += arg.type()->encodedSizes();
    }
    return sizes;
}
}
