/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/parser/Project.h"

#include "decode/core/Configuration.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/Try.h"
#include "decode/parser/Package.h"
#include "decode/generator/Generator.h"

#include <bmcl/Result.h>
#include <bmcl/StringView.h>
#include <bmcl/Logging.h>
#include <bmcl/FileUtils.h>

#include <toml11/toml.hpp>

#include <set>

namespace decode {

#if defined(__linux__)
static constexpr char sep = '/';
#elif defined(_MSC_VER) || defined(__MINGW32__)
static constexpr char sep = '\\';
static constexpr char altsep = '/';
#endif

static void normalizePath(std::string* dest)
{
#if defined(__linux__)
    (void)dest;
#elif defined(_MSC_VER) || defined(__MINGW32__)
    for (char& c : *dest) {
        if (c == altsep) {
            c = sep;
        }
    }
#endif
}

static void joinPath(std::string* left, const std::string& right)
{
    if (left->empty()) {
        *left = right;
        return;
    }
    if (left->back() == sep) {
        *left += right;
        return;
    }
    left->push_back(sep);
    left->append(right);
}

static std::string joinPath(const std::string& left, const std::string& right)
{
    if (left.empty()) {
        return right;
    }
    if (left.back() == sep) {
        return left + right;
    }
    return left + sep + right;
}

static void removeFilePart(std::string* dest)
{
    if (dest->empty()) {
        return;
    }
    if (dest->back() == sep) {
        return;
    }

    std::size_t n = dest->rfind(sep);
    if (n == std::string::npos) {
        dest->resize(0);
    } else {
        dest->resize(n + 1);
    }
}

static bool isAbsPath(const std::string& path)
{
#if defined(__linux__)
    if (!path.empty() && path.front() == sep) {
        return true;
    }
#elif defined(_MSC_VER) || defined(__MINGW32__)
    if (path.size() < 2) {
        return false;
    }
    if (path[1] == ':') {
        return true;
    }
    if (path[0] == sep && path[1] == sep) {
        return true;
    }
#endif
    return false;
}


Project::Project(Configuration* cfg, Diagnostics* diag, const char* projectFilePath)
    : _cfg(cfg)
    , _diag(diag)
    , _projectFilePath(projectFilePath)
{
    normalizePath(&_projectFilePath);
}

Project::~Project()
{
}

class ParseException {
};

template <typename T>
const T& getValueFromTable(const toml::Table& table, const char* key)
{
    try {
        const toml::value& value = table.at(key);
        return value.cast<toml::detail::check_type<T>()>();
    } catch (const std::out_of_range& exc) {
        BMCL_CRITICAL() << "invalid table key: " << key;
    } catch (const toml::type_error& exc) {
        BMCL_CRITICAL() << "invalid value type: " << key;
    } catch (...) {
        BMCL_CRITICAL() << "unknown toml error: " << key;
    }
    throw ParseException();
}

template <typename T>
std::vector<T> maybeGetArrayFromTable(const toml::Table& table, const char* key)
{
    try {
        return toml::get<std::vector<T>>(table.at(key));
    } catch (const std::out_of_range& exc) {
        return std::vector<T>();
    } catch (const toml::type_error& exc) {
        BMCL_CRITICAL() << "invalid value type: " << key;
    } catch (...) {
        BMCL_CRITICAL() << "unknown toml error: " << key;
    }
    return std::vector<T>();
}

using TableResult = bmcl::Result<toml::Table, void>;

static TableResult readToml(const std::string& path)
{
    auto file = bmcl::readFileIntoString(path.c_str());
    if (file.isErr()) {
        BMCL_CRITICAL() << "error reading file: " << path;
        return TableResult();
    }
    try {
        return toml::parse_data::invoke(file.unwrap().begin(), file.unwrap().end());
    } catch (const toml::syntax_error& exc) {
        BMCL_CRITICAL() << "error parsing " << path << ": " << exc.what();
        return TableResult();
    }
    return TableResult();
}

bool Project::readModuleDescriptions(std::vector<std::string>* decodeFiles, std::vector<SourcesToCopy>* sources)
{
    std::string projectDir = _projectFilePath;
    removeFilePart(&projectDir);
    for (const std::string& modDir : _moduleDirs) {
        std::string dirPath;
        if (!isAbsPath(modDir)) {
            dirPath = projectDir;
            joinPath(&dirPath, modDir);
        } else {
            dirPath = modDir;
        }
        std::string dirTomlPath = dirPath;
        joinPath(&dirTomlPath, "dir.toml");
        BMCL_DEBUG() << "reading mod dir: " << dirTomlPath;
        TableResult dirToml = readToml(dirTomlPath);
        if (dirToml.isErr()) {
            return false;
        }
        std::vector<std::string> modules = maybeGetArrayFromTable<std::string>(dirToml.unwrap(), "modules");
        std::set<uint64_t> moduleIds;
        for (const std::string& moduleName : modules) {
            ModuleDesc mod;
            mod.name = moduleName;
            std::string modTomlPath = dirPath;
            joinPath(&modTomlPath, moduleName);
            std::string moduleDir = modTomlPath;
            joinPath(&modTomlPath, "mod.toml");
            BMCL_DEBUG() << "reading module root: " << modTomlPath;
            TableResult modToml = readToml(modTomlPath);
            if (modToml.isErr()) {
                return false;
            }
            int64_t id = getValueFromTable<int64_t>(modToml.unwrap(), "id");
            if (id < 0) {
                BMCL_CRITICAL() << "module id cannot be negative: " << id;
                return false;
            }
            auto idsPair = moduleIds.insert(id);
            if (!idsPair.second) {
                BMCL_CRITICAL() << "found modules with conflicting ids";
                return false;
            }
            mod.id = id;
            SourcesToCopy src;
            src.relativeDest = getValueFromTable<std::string>(modToml.unwrap(), "dest");
            decodeFiles->push_back(joinPath(moduleDir, getValueFromTable<std::string>(modToml.unwrap(), "decode")));
            src.sources = maybeGetArrayFromTable<std::string>(modToml.unwrap(), "sources");
            for (std::string& src : src.sources) {
                src = joinPath(moduleDir, src);
            }
            sources->push_back(std::move(src));
            auto modPair = _modules.emplace(moduleName, std::move(mod));
            if (!modPair.second) {
                BMCL_CRITICAL() << "module with name " << mod.name << " already exists";
                return false;
            }
        }
    }
    return true;
}

bool Project::readProjectFile()
{
    BMCL_DEBUG() << "reading project file: " << _projectFilePath;
    TableResult projectFile = readToml(_projectFilePath);
    if (projectFile.isErr()) {
        return false;
    }

    try {
        const toml::Table& projectTable = getValueFromTable<toml::Table>(projectFile.unwrap(), "project");
        _name = getValueFromTable<std::string>(projectTable, "name");
        _master = getValueFromTable<std::string>(projectTable, "master");
        int64_t id = getValueFromTable<std::int64_t>(projectTable, "mcc_id");
        if (id < 0) {
            BMCL_CRITICAL() << "mcc_id cannot be negative: " << id;
            return false;
        }
        _mccId = id;
        _commonModuleNames = maybeGetArrayFromTable<std::string>(projectTable, "common_modules");
        _moduleDirs = maybeGetArrayFromTable<std::string>(projectTable, "module_dirs");
        for (std::string& path : _moduleDirs) {
            normalizePath(&path);
        }

        const toml::Array& devicesArray = getValueFromTable<toml::Array>(projectFile.unwrap(), "devices");
        std::set<uint64_t> deviceIds;
        for (const toml::value& value : devicesArray) {
            if (value.type() != toml::value_t::Table) {
                BMCL_CRITICAL() << "devices section must be a table";
                return false;
            }
            const toml::Table& tab = value.cast<toml::value_t::Table>();
            DeviceDesc dev;
            dev.name = getValueFromTable<std::string>(tab, "name");
            int64_t id = getValueFromTable<int64_t>(tab, "id");
            if (id < 0) {
                BMCL_CRITICAL() << "device id cannot be negative: " << id;
                return false;
            }
            if (id == _mccId) {
                BMCL_CRITICAL() << "device id cannot be the same as mcc_id: " << id;
                return false;
            }
            auto idsPair = deviceIds.insert(id);
            if (!idsPair.second) {
                BMCL_CRITICAL() << "found devices with conflicting ids";
                return false;
            }
            dev.id = id;
            dev.modules = maybeGetArrayFromTable<std::string>(tab, "modules");
            dev.tmSources = maybeGetArrayFromTable<std::string>(tab, "tm_sources");
            dev.cmdTargets = maybeGetArrayFromTable<std::string>(tab, "cmd_targets");
            auto devPair = _devices.emplace(dev.name, std::move(dev));
            if (!devPair.second) {
                BMCL_CRITICAL() << "device with name " << dev.name << " already exists";
                return false;
            }
        }
    } catch (...) {
        return false;
    }
    return true;
}

bool Project::checkProject()
{
    if (_devices.find(_master) == _devices.end()) {
        BMCL_CRITICAL() << "device with name " << _master << " marked as master does not exist";
        return false;
    }
    for (const std::string& modName : _commonModuleNames) {
        if (_modules.find(modName) == _modules.end()) {
            BMCL_CRITICAL() << "common module " << modName << " does not exist";
            return false;
        }
    }

    for (auto it : _devices) {
        for (const std::string& modName : it.second.modules) {
            if (_modules.find(modName) == _modules.end()) {
                BMCL_CRITICAL() << "module " << modName << " does not exist";
                return false;
            }
        }
        for (const std::string& deviceName : it.second.tmSources) {
            //TODO: deviceName == it->second.name
            if (_devices.find(deviceName) == _devices.end()) {
                BMCL_CRITICAL() << "unknown tm source: " << deviceName;
                return false;
            }
        }
        for (const std::string& deviceName : it.second.cmdTargets) {
            //TODO: deviceName == it->second.name
            if (_devices.find(deviceName) == _devices.end()) {
                BMCL_CRITICAL() << "unknown cmd target: " << deviceName;
                return false;
            }
        }
    }
    return true;
}

bool Project::parsePackage(const std::vector<std::string>& decodeFiles)
{
    PackageResult package = Package::readFromFiles(_cfg.get(), _diag.get(), decodeFiles);
    if (package.isErr()) {
        return false;
    }
    _package = package.take();
    return true;
}

bool Project::copySources(const std::vector<Project::SourcesToCopy>& sources)
{
    return true;
}

ProjectResult Project::fromFile(Configuration* cfg, Diagnostics* diag, const char* projectFilePath)
{
    Rc<Project> proj = new Project(cfg, diag, projectFilePath);
    if (!proj->readProjectFile()) {
        return ProjectResult();
    }
    std::vector<std::string> decodeFiles;
    std::vector<SourcesToCopy> sources;
    if (!proj->readModuleDescriptions(&decodeFiles, &sources)) {
        return ProjectResult();
    }
    if (!proj->checkProject()) {
        return ProjectResult();
    }
    if (!proj->parsePackage(decodeFiles)) {
        return ProjectResult();
    }

    return proj;
}

bool Project::generate(const char* destDir)
{
    BMCL_DEBUG() << "generating";

    Rc<Generator> gen = new Generator(_diag.get());
    gen->setOutPath(destDir);
    bool genOk = gen->generateProject(this);
    if (genOk) {
        BMCL_DEBUG() << "generating complete";
    } else {
        BMCL_DEBUG() << "generating failed";
    }
    return genOk;
}

const Project::ModuleDescMap& Project::modules() const
{
    return _modules;
}

const Project::DeviceDescMap& Project::devices() const
{
    return _devices;
}

const std::string& Project::masterDeviceName() const
{
    return _master;
}

const std::string& Project::name() const
{
    return _name;
}

const std::vector<std::string>& Project::commonModuleNames() const
{
    return _commonModuleNames;
}

std::uint64_t Project::mccId() const
{
    return _mccId;
}

const Package* Project::package() const
{
    return _package.get();
}

bmcl::Buffer Project::encode() const
{
    return bmcl::Buffer();
}
}
