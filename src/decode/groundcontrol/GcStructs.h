#pragma once

#include "decode/Config.h"

#include <bmcl/Variant.h>

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

struct Position {
    LatLon latLon;
    double altitude;
};

struct FormationEntry {
    Vec3 pos;
    uint64_t id;
};

enum class WaypointActionKind {
    None,
    Sleep,
    Formation,
    Reynolds,
    Snake,
    Loop,
};

using WaypointAction =
    bmcl::Variant<WaypointActionKind, WaypointActionKind::None,
        bmcl::VariantElementDesc<WaypointActionKind, uint64_t, WaypointActionKind::Sleep>,
        bmcl::VariantElementDesc<WaypointActionKind, std::vector<FormationEntry>, WaypointActionKind::Formation>,
        bmcl::VariantElementDesc<WaypointActionKind, void, WaypointActionKind::Reynolds>,
        bmcl::VariantElementDesc<WaypointActionKind, void, WaypointActionKind::Snake>,
        bmcl::VariantElementDesc<WaypointActionKind, void, WaypointActionKind::Loop>
    >;

struct Waypoint {
    Position position;
    WaypointAction action;
};
};
