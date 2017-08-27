#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/groundcontrol/GcStructs.h"

#include <bmcl/Variant.h>
#include <bmcl/Option.h>

namespace decode {

enum class GcCmdKind {
    None,
    UploadRoute,
    SetActiveRoute,
    SetRouteActivePoint,
    SetRouteInverted,
    SetRouteClosed,
};

struct Route {
    std::vector<Waypoint> waypoints;
};

struct SetActiveRouteGcCmd {
    bmcl::Option<uint64_t> id;
};

struct SetRouteActivePointGcCmd {
    uint64_t id;
    bmcl::Option<uint64_t> index;
};

struct SetRouteInvertedGcCmd {
    uint64_t id;
    bool isInverted;
};

struct SetRouteClosedGcCmd {
    uint64_t id;
    bool isClosed;
};

using GcCmd =
    bmcl::Variant<GcCmdKind, GcCmdKind::None,
        bmcl::VariantElementDesc<GcCmdKind, Route, GcCmdKind::UploadRoute>,
        bmcl::VariantElementDesc<GcCmdKind, SetActiveRouteGcCmd, GcCmdKind::SetActiveRoute>,
        bmcl::VariantElementDesc<GcCmdKind, SetRouteActivePointGcCmd, GcCmdKind::SetRouteActivePoint>,
        bmcl::VariantElementDesc<GcCmdKind, SetRouteInvertedGcCmd, GcCmdKind::SetRouteInverted>,
        bmcl::VariantElementDesc<GcCmdKind, SetRouteClosedGcCmd, GcCmdKind::SetRouteClosed>
    >;
}
