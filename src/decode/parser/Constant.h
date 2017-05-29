/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/NamedRc.h"

#include <bmcl/StringView.h>

#include <cstdint>

namespace decode {

class Type;

class Constant : public NamedRc {
public:
    Constant(bmcl::StringView name, std::uintmax_t value, Type* type)
        : NamedRc(name)
        , _value(value)
        , _type(type)
    {
    }

    std::uintmax_t value() const
    {
        return _value;
    }

    const Rc<Type>& type() const
    {
        return _type;
    }

private:
    std::uintmax_t _value;
    Rc<Type> _type;
};
}
