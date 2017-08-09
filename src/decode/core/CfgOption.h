/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

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
    using Pointer = Rc<CfgOption>;
    using ConstPointer = Rc<const CfgOption>;

    virtual bool matchesConfiguration(const Configuration* cfg) const = 0;
};

class SingleCfgOption : public CfgOption {
public:
    using Pointer = Rc<SingleCfgOption>;
    using ConstPointer = Rc<const SingleCfgOption>;

    SingleCfgOption(bmcl::StringView key);
    SingleCfgOption(bmcl::StringView key, bmcl::StringView value);
    ~SingleCfgOption();

    bool matchesConfiguration(const Configuration* cfg) const override;

protected:
    bmcl::StringView _key;
    bmcl::Option<bmcl::StringView> _value;
};

class NotCfgOption : public SingleCfgOption {
public:
    using Pointer = Rc<NotCfgOption>;
    using ConstPointer = Rc<const NotCfgOption>;

    using SingleCfgOption::SingleCfgOption;

    bool matchesConfiguration(const Configuration* cfg) const override;
};

class AnyCfgOption : public CfgOption {
public:
    using Pointer = Rc<AnyCfgOption>;
    using ConstPointer = Rc<const AnyCfgOption>;

    AnyCfgOption();
    ~AnyCfgOption();

    void addOption(const CfgOption* attr);
    bool matchesConfiguration(const Configuration* cfg) const override;

protected:
    std::vector<Rc<const CfgOption>> _attrs;
};

class AllCfgOption : public AnyCfgOption {
public:
    using Pointer = Rc<AllCfgOption>;
    using ConstPointer = Rc<const AllCfgOption>;

    using AnyCfgOption::AnyCfgOption;

    bool matchesConfiguration(const Configuration* cfg) const override;
};

}
