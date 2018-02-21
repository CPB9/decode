#include "decode/core/CmdCallAttr.h"

#include <bmcl/Option.h>

namespace decode {

CmdCallAttr::CmdCallAttr()
{
}

CmdCallAttr::~CmdCallAttr()
{
}

void CmdCallAttr::addParam(bmcl::StringView name, CmdArgPassKind kind)
{
    _params.emplace_back(name, kind);
}

CmdCallAttr::ParamVecConstRange CmdCallAttr::params() const
{
    return ParamVecConstRange(_params);
}

CmdArgPassKind CmdCallAttr::findArgPassKind(bmcl::StringView name) const
{
    auto it = std::find_if(_params.begin(), _params.end(), [&name](const CmdParamPass& value) {
        return value.name == name;
    });
    if (it == _params.end()) {
        return CmdArgPassKind::Default;
    }
    return it->kind;
}
}
