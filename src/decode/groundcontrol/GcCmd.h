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
    DownloadRouteInfo,
    DownloadRoute,
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

struct DownloadRouteInfoGcCmd {
};

struct DownloadRouteGcCmd {
    uint64_t id;
};

using GcCmd =
    bmcl::Variant<GcCmdKind, GcCmdKind::None,
        bmcl::VariantElementDesc<GcCmdKind, Route, GcCmdKind::UploadRoute>,
        bmcl::VariantElementDesc<GcCmdKind, SetActiveRouteGcCmd, GcCmdKind::SetActiveRoute>,
        bmcl::VariantElementDesc<GcCmdKind, SetRouteActivePointGcCmd, GcCmdKind::SetRouteActivePoint>,
        bmcl::VariantElementDesc<GcCmdKind, SetRouteInvertedGcCmd, GcCmdKind::SetRouteInverted>,
        bmcl::VariantElementDesc<GcCmdKind, SetRouteClosedGcCmd, GcCmdKind::SetRouteClosed>,
        bmcl::VariantElementDesc<GcCmdKind, DownloadRouteInfoGcCmd, GcCmdKind::DownloadRouteInfo>,
        bmcl::VariantElementDesc<GcCmdKind, DownloadRouteGcCmd, GcCmdKind::DownloadRoute>
    >;
}
