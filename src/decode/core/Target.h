#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <cstddef>

namespace decode {

class Target : public RefCountable {
public:
    Target(std::size_t pointerSize)
        : _pointerSize(pointerSize)
    {
    }

    std::size_t pointerSize() const
    {
        return _pointerSize;
    }

private:
    std::size_t _pointerSize;
};
}
