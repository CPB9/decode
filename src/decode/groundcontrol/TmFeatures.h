#pragma once

#include "decode/Config.h"

namespace decode {

struct TmFeatures {
public:
    TmFeatures()
        : hasLatLon(false)
        , hasOrientation(false)
    {
    }

    bool hasLatLon;
    bool hasOrientation;
};

}
