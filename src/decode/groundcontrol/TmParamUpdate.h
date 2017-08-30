#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/groundcontrol/GcStructs.h"

#include <bmcl/Variant.h>

namespace decode {

enum class TmParamKind {
    None,
    LatLon,
    Orientation,
    RoutesInfo,
    Route,
};

struct RouteTmParam {
    uint64_t id;
    Route route;
};

using TmParamUpdate =
    bmcl::Variant<TmParamKind, TmParamKind::None,
        bmcl::VariantElementDesc<TmParamKind, LatLon, TmParamKind::LatLon>,
        bmcl::VariantElementDesc<TmParamKind, Orientation, TmParamKind::Orientation>,
        bmcl::VariantElementDesc<TmParamKind, AllRoutesInfo, TmParamKind::RoutesInfo>,
        bmcl::VariantElementDesc<TmParamKind, RouteTmParam, TmParamKind::Route>
    >;
}
