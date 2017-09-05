#pragma once

#include "decode/Config.h"

#include <bmcl/Variant.h>
#include <bmcl/Option.h>

#include <vector>

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

struct Velocity3 {
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

struct RouteInfo {
    RouteInfo()
        : id(0)
        , size(0)
        , maxSize(0)
        , isClosed(false)
        , isInverted(false)
        , isEditing(false)
    {
    }

    uintmax_t id;
    uint64_t size;
    uint64_t maxSize;
    bmcl::Option<uintmax_t> activePoint;
    bool isClosed;
    bool isInverted;
    bool isEditing;
};

struct AllRoutesInfo {
    std::vector<RouteInfo> info;
    bmcl::Option<uintmax_t> activeRoute;
};

enum class WaypointActionKind {
    None,
    Sleep,
    Formation,
    Reynolds,
    Snake,
    Loop,
};

struct SleepWaypointAction {
    uint64_t timeout;
};

struct FormationWaypointAction {
    std::vector<FormationEntry> entries;
};

struct ReynoldsWaypointAction {
};

struct SnakeWaypointAction {
};

struct LoopWaypointAction {
};

using WaypointAction =
    bmcl::Variant<WaypointActionKind, WaypointActionKind::None,
        bmcl::VariantElementDesc<WaypointActionKind, SleepWaypointAction, WaypointActionKind::Sleep>,
        bmcl::VariantElementDesc<WaypointActionKind, FormationWaypointAction, WaypointActionKind::Formation>,
        bmcl::VariantElementDesc<WaypointActionKind, ReynoldsWaypointAction, WaypointActionKind::Reynolds>,
        bmcl::VariantElementDesc<WaypointActionKind, SnakeWaypointAction, WaypointActionKind::Snake>,
        bmcl::VariantElementDesc<WaypointActionKind, LoopWaypointAction, WaypointActionKind::Loop>
    >;

struct Waypoint {
    Position position;
    bmcl::Option<double> speed;
    WaypointAction action;
};

struct Route {
    std::vector<Waypoint> waypoints;
    RouteInfo info;
};
};
