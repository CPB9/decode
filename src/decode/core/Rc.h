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
