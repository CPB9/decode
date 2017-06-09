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
#include "decode/parser/ModuleDesc.h"
#include "decode/parser/DeviceDesc.h"

#include <bmcl/Fwd.h>
#include <bmcl/StringView.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace decode {

class Diagnostics;
class Configuration;
class Project;
class Package;

using ProjectResult = bmcl::Result<Rc<Project>, void>;

class Project : public RefCountable {
public:
    using DeviceDescMap = std::unordered_map<std::string, DeviceDesc>;
    using ModuleDescMap = std::unordered_map<std::string, ModuleDesc>;

    struct SourcesToCopy {
        std::vector<std::string> sources;
        std::string relativeDest;
    };

    static ProjectResult fromFile(Configuration* cfg, Diagnostics* diag, const char* projectFilePath);
    ~Project();

    bool generate(const char* destDir);

    const ModuleDescMap& modules() const;
    const DeviceDescMap& devices() const;
    const std::string& masterDeviceName() const;
    const std::string& name() const;
    const std::vector<std::string>& commonModuleNames() const;
    std::uint64_t mccId() const;
    const Package* package() const;

    bmcl::Buffer encode() const;

private:
    Project(Configuration* cfg, Diagnostics* diag, const char* projectFilePath);

    bool readProjectFile();
    bool readModuleDescriptions(std::vector<std::string>* decodeFiles, std::vector<SourcesToCopy>* sources);
    bool checkProject();
    bool parsePackage(const std::vector<std::string>& decodeFiles);
    bool copySources(const std::vector<SourcesToCopy>& sources);

    Rc<Configuration> _cfg;
    Rc<Diagnostics> _diag;
    Rc<Package> _package;
    std::string _projectFilePath;
    std::string _name;
    std::string _master;
    std::vector<std::string> _commonModuleNames;
    std::vector<std::string> _moduleDirs;
    DeviceDescMap _devices;
    ModuleDescMap _modules;
    std::uint64_t _mccId;
};
}
