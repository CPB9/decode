/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"

#include <bmcl/Rc.h>
#include <bmcl/ThreadSafeRefCountable.h>

#include <utility>

namespace decode {

template <typename T>
using Rc = bmcl::Rc<T>;

using RefCountable = bmcl::ThreadSafeRefCountable<std::size_t>;

template <typename T, typename... A>
Rc<T> makeRc(A&&... args)
{
    return new T(std::forward<A>(args)...);
}
}
