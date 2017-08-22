#pragma once

#include "decode/Config.h"

namespace decode {

struct LatLon {
    double latitude;
    double longitude;
};

struct Orientation {
    double heading;
    double pitch;
    double roll;
};

struct Vec3 {
    double x;
    double y;
    double z;
};
};
