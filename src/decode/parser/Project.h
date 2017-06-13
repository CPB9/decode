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

#include <bmcl/Fwd.h>
#include <bmcl/StringView.h>

#include <string>
#include <vector>
#include <unordered_set>

namespace decode {

class Diagnostics;
class Configuration;
class Project;
class Package;
class Ast;

using ProjectResult = bmcl::Result<Rc<Project>, void>;

class Project : public RefCountable {
public:
    struct Device : public RefCountable {
        std::vector<Rc<Ast>> modules;
        std::vector<Rc<Device>> tmSources;
        std::vector<Rc<Device>> cmdTargets;
        std::string name;
        uint64_t id;
    };

    static ProjectResult fromFile(Configuration* cfg, Diagnostics* diag, const char* projectFilePath);
    ~Project();

    bool generate(const char* destDir);

    const std::string& name() const;
    std::uint64_t mccId() const;
    const Package* package() const;

    bmcl::Buffer encode() const;

private:
    Project(Configuration* cfg, Diagnostics* diag);

    Rc<Configuration> _cfg;
    Rc<Diagnostics> _diag;
    Rc<Package> _package;
    std::vector<Rc<Device>> _devices;
    std::string _name;
    std::uint64_t _mccId;
};
}
