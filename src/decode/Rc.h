#pragma once

#include "decode/Config.h"

#include <bmcl/Rc.h>
#include <bmcl/RefCountable.h>

namespace decode {

template <typename T>
using Rc = bmcl::Rc<T>;

typedef bmcl::RefCountable<unsigned int> RefCountable;
}
