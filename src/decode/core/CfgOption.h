#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/StringView.h>
#include <bmcl/Option.h>

#include <map>

namespace decode {

class Configuration;

class CfgOption : public RefCountable {
public:
    virtual bool matchesConfiguration(const Configuration* cfg) const = 0;
};

class SingleCfgOption : public CfgOption {
public:
    SingleCfgOption(bmcl::StringView key);
    SingleCfgOption(bmcl::StringView key, bmcl::StringView value);

    bool matchesConfiguration(const Configuration* cfg) const override;

protected:
    bmcl::StringView _key;
    bmcl::Option<bmcl::StringView> _value;
};

class NotCfgOption : public SingleCfgOption {
public:
    using SingleCfgOption::SingleCfgOption;

    bool matchesConfiguration(const Configuration* cfg) const override;
};

class AnyCfgOption : public CfgOption {
public:
    AnyCfgOption();
    ~AnyCfgOption();

    void addOption(const CfgOption* attr);
    bool matchesConfiguration(const Configuration* cfg) const override;

protected:
    std::vector<Rc<const CfgOption>> _attrs;
};

class AllCfgOption : public AnyCfgOption {
public:
    using AnyCfgOption::AnyCfgOption;

    bool matchesConfiguration(const Configuration* cfg) const override;
};

}
