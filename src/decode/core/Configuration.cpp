#include "decode/core/Configuration.h"

namespace decode {

Configuration::Configuration()
{
    setCfgOption("target_pointer_width", "32");
}

Configuration::~Configuration()
{
}

void Configuration::setCfgOption(bmcl::StringView key)
{
    _values.emplace(key, bmcl::None);
}

void Configuration::setCfgOption(bmcl::StringView key, bmcl::StringView value)
{
    _values.emplace(key, value);
}

bool Configuration::isCfgOptionDefined(bmcl::StringView key) const
{
    return _values.find(key) != _values.end();
}

bool Configuration::isCfgOptionSet(bmcl::StringView key, const bmcl::Option<bmcl::StringView>& value) const
{
    auto it = _values.find(key);
    if (it == _values.end()) {
        return false;
    }
    return it->second == value;
}
}
