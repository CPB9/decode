/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/parser/Containers.h"
#include "decode/ast/Field.h"
#include "decode/ast/Type.h"

#include <bmcl/OptionPtr.h>
#include <bmcl/StringView.h>

namespace decode {

bmcl::OptionPtr<Field> FieldVec::fieldWithName(bmcl::StringView name)
{
    for (const Rc<Field>& value : *this) {
        if (value->name() == name) {
            return value.get();
        }
    }
    return bmcl::None;
}

bmcl::OptionPtr<const Field> FieldVec::fieldWithName(bmcl::StringView name) const
{
    for (const Rc<Field>& value : *this) {
        if (value->name() == name) {
            return value.get();
        }
    }
    return bmcl::None;
}
}
