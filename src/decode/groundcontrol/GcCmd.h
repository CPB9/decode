#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/groundcontrol/GcStructs.h"

#include <bmcl/Variant.h>

namespace decode {

enum class GcCmdKind {
    None,
    UploadRoute,
    Test,
};

struct Route {
    std::vector<Waypoint> waypoints;
};

using GcCmd =
    bmcl::Variant<GcCmdKind, GcCmdKind::None,
        bmcl::VariantElementDesc<GcCmdKind, Route, GcCmdKind::UploadRoute>,
        bmcl::VariantElementDesc<GcCmdKind, int, GcCmdKind::Test>
    >;
}
