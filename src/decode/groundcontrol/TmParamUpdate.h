#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/groundcontrol/GcStructs.h"

#include <bmcl/Variant.h>

namespace decode {

enum class TmParamKind {
    None,
    Position,
    Orientation,
    Velocity,
    RoutesInfo,
    Route,
};

struct RouteTmParam {
    uint64_t id;
    Route route;
};

using TmParamUpdate =
    bmcl::Variant<TmParamKind, TmParamKind::None,
        bmcl::VariantElementDesc<TmParamKind, Position, TmParamKind::Position>,
        bmcl::VariantElementDesc<TmParamKind, Orientation, TmParamKind::Orientation>,
        bmcl::VariantElementDesc<TmParamKind, Velocity3, TmParamKind::Velocity>,
        bmcl::VariantElementDesc<TmParamKind, AllRoutesInfo, TmParamKind::RoutesInfo>,
        bmcl::VariantElementDesc<TmParamKind, RouteTmParam, TmParamKind::Route>
    >;
}
