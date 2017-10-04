#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/groundcontrol/GcStructs.h"

#include <bmcl/Variant.h>
#include <bmcl/Option.h>

namespace decode {

enum class TmParamKind {
    None,
    Position,
    Orientation,
    Velocity,
    RoutesInfo,
    Route,
    GroupDeviceState,
    GroupState,
};

struct RouteTmParam {
    uint64_t id;
    Route route;
};

struct GroupDeviceStateTmParam {
    bmcl::Option<uintmax_t> groupId;
    bmcl::Option<uintmax_t> leaderId;
};

struct GroupStateTmParam {
    uintmax_t groupId;
    bmcl::Option<uintmax_t> leaderId;
    std::vector<uintmax_t> groupIds;
};

using TmParamUpdate =
    bmcl::Variant<TmParamKind, TmParamKind::None,
        bmcl::VariantElementDesc<TmParamKind, Position, TmParamKind::Position>,
        bmcl::VariantElementDesc<TmParamKind, Orientation, TmParamKind::Orientation>,
        bmcl::VariantElementDesc<TmParamKind, Velocity3, TmParamKind::Velocity>,
        bmcl::VariantElementDesc<TmParamKind, AllRoutesInfo, TmParamKind::RoutesInfo>,
        bmcl::VariantElementDesc<TmParamKind, RouteTmParam, TmParamKind::Route>,
        bmcl::VariantElementDesc<TmParamKind, GroupDeviceStateTmParam, TmParamKind::GroupDeviceState>,
        bmcl::VariantElementDesc<TmParamKind, GroupStateTmParam, TmParamKind::GroupState>
    >;
}
