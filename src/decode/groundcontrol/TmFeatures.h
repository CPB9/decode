#pragma once

#include "decode/Config.h"

namespace decode {

struct TmFeatures {
public:
    TmFeatures()
        : hasPosition(false)
        , hasOrientation(false)
        , hasVelocity(false)
    {
    }

    bool hasPosition;
    bool hasOrientation;
    bool hasVelocity;
};

}
