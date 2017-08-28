/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/Function.h"
#include "decode/ast/Type.h"

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

Command::Command(bmcl::StringView name, FunctionType* type)
    : Function(name, type)
    , _number(0)
{
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
}
