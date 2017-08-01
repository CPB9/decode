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
#include "decode/core/NamedRc.h"
#include "decode/ast/DocBlockMixin.h"

#include <bmcl/Fwd.h>

namespace decode {

class ImplBlock;
class FunctionType;
class Field;
class Type;
class ModuleInfo;

class Function : public NamedRc, public DocBlockMixin {
public:
    Function(bmcl::StringView name, FunctionType* type);
    ~Function();

    const FunctionType* type() const;

private:
    Rc<FunctionType> _type;
};
}
