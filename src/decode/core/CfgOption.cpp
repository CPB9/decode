/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

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

SingleCfgOption::~SingleCfgOption()
{
}

bool SingleCfgOption::matchesConfiguration(const Configuration* cfg) const
{
    return cfg->isCfgOptionSet(_key, _value);
}

bool NotCfgOption::matchesConfiguration(const Configuration* cfg) const
{
    return !cfg->isCfgOptionSet(_key, _value);
}

AnyCfgOption::AnyCfgOption()
{
}

AnyCfgOption::~AnyCfgOption()
{
}

void AnyCfgOption::addOption(const CfgOption* attr)
{
    _attrs.emplace_back(attr);
}

bool AnyCfgOption::matchesConfiguration(const Configuration* cfg) const
{
    for (const Rc<const CfgOption>& attr : _attrs) {
        if (attr->matchesConfiguration(cfg)) {
            return true;
        }
    }
    return false;
}

bool AllCfgOption::matchesConfiguration(const Configuration* cfg) const
{
    for (const Rc<const CfgOption>& attr : _attrs) {
        if (!attr->matchesConfiguration(cfg)) {
            return false;
        }
    }
    return true;
}
}
