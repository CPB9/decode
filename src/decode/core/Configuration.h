#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Hash.h"

#include <bmcl/StringView.h>
#include <bmcl/Option.h>

#include <unordered_map>

namespace decode {

class Configuration : public RefCountable {
public:
    Configuration();
    ~Configuration();

    void setCfgOption(bmcl::StringView key);
    void setCfgOption(bmcl::StringView key, bmcl::StringView value);

    bool isCfgOptionDefined(bmcl::StringView key) const;
    bool isCfgOptionSet(bmcl::StringView key, const bmcl::Option<bmcl::StringView>& value = bmcl::None) const;

private:
    std::unordered_map<bmcl::StringView, bmcl::Option<bmcl::StringView>> _values;
};
}
