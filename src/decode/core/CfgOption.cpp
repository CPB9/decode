#include "decode/core/CfgOption.h"
#include "decode/core/Configuration.h"

namespace decode {

SingleCfgOption::SingleCfgOption(bmcl::StringView key)
    : _key(key)
{
}

SingleCfgOption::SingleCfgOption(bmcl::StringView key, bmcl::StringView value)
    : _key(key)
    , _value(value)
{
}

bool SingleCfgOption::matchesConfiguration(const Rc<Configuration>& cfg) const
{
    return cfg->isCfgOptionSet(_key, _value);
}

bool NotCfgOption::matchesConfiguration(const Rc<Configuration>& cfg) const
{
    return !cfg->isCfgOptionSet(_key, _value);
}

AnyCfgOption::AnyCfgOption()
{
}

AnyCfgOption::~AnyCfgOption()
{
}

void AnyCfgOption::addOption(const Rc<CfgOption>& attr)
{
    _attrs.emplace_back(attr);
}

bool AnyCfgOption::matchesConfiguration(const Rc<Configuration>& cfg) const
{
    for (const Rc<CfgOption>& attr : _attrs) {
        if (attr->matchesConfiguration(cfg)) {
            return true;
        }
    }
    return false;
}

bool AllCfgOption::matchesConfiguration(const Rc<Configuration>& cfg) const
{
    for (const Rc<CfgOption>& attr : _attrs) {
        if (!attr->matchesConfiguration(cfg)) {
            return false;
        }
    }
    return true;
}
}
