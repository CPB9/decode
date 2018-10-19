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
#include "decode/core/HashMap.h"
#include "decode/parser/Containers.h"

#include <bmcl/Fwd.h>
#include <bmcl/StringView.h>
#include <bmcl/RcHash.h>

#include <string>
#include <vector>

namespace decode {

class Diagnostics;
class Configuration;
class Project;
class Package;
class Ast;
class Device;
struct GeneratorConfig;

using ProjectResult = bmcl::Result<Rc<Project>, void>;

using DeviceVec = RcVec<Device>;

class Device : public RefCountable {
public:
    using Pointer = Rc<Device>;
    using ConstPointer = Rc<const Device>;

    const Package* package() const;
    uint64_t id() const;
    const std::string& name() const;
    RcVec<Ast>::ConstRange modules() const;

private:
    friend class Project;
    Device();
    ~Device();

    RcVec<Ast> _modules;
    std::string _name;
    uint64_t _id;
    Rc<Package> _package;
};

class DeviceConnection : public RefCountable {
public:
    using Pointer = Rc<DeviceConnection>;
    using ConstPointer = Rc<const DeviceConnection>;

    ~DeviceConnection();

    const Device* device() const;
    DeviceVec::ConstRange tmSources() const;
    DeviceVec::ConstRange cmdTargets() const;

private:
    friend class Project;

    DeviceConnection(const Device* dev);

    Rc<const Device> _device;
    DeviceVec _tmSources;
    DeviceVec _cmdTargets;
};

class Project : public RefCountable {
public:
    using Pointer = Rc<Project>;
    using ConstPointer = Rc<const Project>;
    using HashType = bmcl::Sha3<512>;

    struct SourcesToCopy {
        std::vector<std::string> sources;
        std::string relativeDest;
    };

    static ProjectResult fromFile(Configuration* cfg, Diagnostics* diag, const char* projectFilePath);
    static ProjectResult decodeFromMemory(Diagnostics* diag, const void* src, std::size_t size);
    ~Project();

    static std::array<std::uint8_t, 512 / 8> hash(bmcl::Bytes data);

    bool generate(const char* destDir, const GeneratorConfig& cfg);

    const std::string& name() const;
    std::uint64_t mccId() const;
    const Package* package() const;
    const Device* master() const;
    DeviceVec::ConstIterator devicesBegin() const;
    DeviceVec::ConstIterator devicesEnd() const;
    DeviceVec::ConstRange devices() const;
    RcVec<DeviceConnection>::ConstRange deviceConnections() const;

    bmcl::Buffer encode() const;
    void encode(bmcl::Buffer* dest) const;
    bmcl::Option<const SourcesToCopy&> sourcesForModule(const Ast* module) const;
    bmcl::OptionPtr<const Device> deviceWithName(bmcl::StringView name) const;
    bmcl::OptionPtr<Device> deviceWithName(bmcl::StringView name);

private:
    Project(Configuration* cfg, Diagnostics* diag);

    Rc<Configuration> _cfg;
    Rc<Diagnostics> _diag;
    Rc<Package> _package;
    Rc<Device> _master;
    std::vector<Rc<Device>> _devices;
    std::vector<Rc<DeviceConnection>> _connections;
    HashMap<Rc<const Ast>, SourcesToCopy> _sourcesMap;
    std::string _name;
    std::uint64_t _mccId;
};
}
