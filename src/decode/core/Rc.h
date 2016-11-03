#pragma once

#include "decode/Config.h"

#include <bmcl/Rc.h>
#include <bmcl/RefCountable.h>

#include <utility>

namespace decode {

template <typename T>
using Rc = bmcl::Rc<T>;

typedef bmcl::RefCountable<unsigned int> RefCountable;

template <typename T, typename... A>
Rc<T> makeRc(A&&... args)
{
    return new T(std::forward<A>(args)...);
}
}
