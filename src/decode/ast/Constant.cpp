/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */



#include "decode/ast/Constant.h"
#include "decode/ast/Type.h"

namespace decode {

Constant::Constant(bmcl::StringView name, std::uintmax_t value, Type* type)
    : NamedRc(name)
    , _value(value)
    , _type(type)
{
}

Constant::~Constant()
{
}

std::uintmax_t Constant::value() const
{
    return _value;
}

const Type* Constant::type() const
{
    return _type.get();
}
}
