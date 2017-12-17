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

#include <bmcl/Fwd.h>

namespace decode {

class Type;
class StringBuilder;

Rc<Type> wrapPassedTypeIntoPointerIfRequired(Type* type);
void derefPassedVarNameIfRequired(const Type* type, bmcl::StringView name, StringBuilder* dest);
}
