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
#include "decode/parser/Ast.h"
#include "decode/parser/Component.h"
#include "decode/generator/Generator.h"

#include <bmcl/Result.h>
#include <bmcl/StringView.h>
#include <bmcl/Logging.h>
#include <bmcl/FileUtils.h>

#include <toml11/toml.hpp>

#include <unordered_map>
#include <set>

namespace decode {

template <typename C>
void removeDuplicates(C* container)
{
    std::sort(container->begin(), container->end());
    auto it = std::unique(container->begin(), container->end());
    container->erase(it, container->end());
}

struct DeviceDesc {
    std::vector<std::string> modules;
    std::vector<std::string> tmSources;
    std::vector<std::string> cmdTargets;
    std::string name;
    std::uint64_t id;
    Rc<Project::Device> device;
};

struct ModuleDesc {
    std::string name;
    std::uint64_t id;
    std::vector<std::string> sources;
    std::string relativeDest;
};

using DeviceDescMap = std::unordered_map<std::string, DeviceDesc>;
using ModuleDescMap = std::unordered_map<std::string, ModuleDesc>;

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


Project::Project(Configuration* cfg, Diagnostics* diag)
    : _cfg(cfg)
    , _diag(diag)
{
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

ProjectResult Project::fromFile(Configuration* cfg, Diagnostics* diag, const char* path)
{
    std::string projectFilePath(path);
    normalizePath(&projectFilePath);
    Rc<Project> proj = new Project(cfg, diag);

    BMCL_DEBUG() << "reading project file: " << projectFilePath;
    TableResult projectFile = readToml(projectFilePath);
    if (projectFile.isErr()) {
        return ProjectResult();
    }

    std::string master;
    DeviceDescMap deviceDescMap;
    std::vector<std::string> moduleDirs;
    std::vector<std::string> commonModuleNames;
    //read project file
    try {
        const toml::Table& projectTable = getValueFromTable<toml::Table>(projectFile.unwrap(), "project");
        proj->_name = getValueFromTable<std::string>(projectTable, "name");
        master = getValueFromTable<std::string>(projectTable, "master");
        int64_t id = getValueFromTable<std::int64_t>(projectTable, "mcc_id");
        if (id < 0) {
            BMCL_CRITICAL() << "mcc_id cannot be negative: " << id;
            return ProjectResult();
        }
        proj->_mccId = id;
        commonModuleNames = maybeGetArrayFromTable<std::string>(projectTable, "common_modules");
        moduleDirs = maybeGetArrayFromTable<std::string>(projectTable, "module_dirs");
        removeDuplicates(&moduleDirs);
        for (std::string& path : moduleDirs) {
            normalizePath(&path);
        }

        const toml::Array& devicesArray = getValueFromTable<toml::Array>(projectFile.unwrap(), "devices");
        std::set<uint64_t> deviceIds;
        for (const toml::value& value : devicesArray) {
            if (value.type() != toml::value_t::Table) {
                BMCL_CRITICAL() << "devices section must be a table";
                return ProjectResult();
            }
            const toml::Table& tab = value.cast<toml::value_t::Table>();
            DeviceDesc dev;
            dev.name = getValueFromTable<std::string>(tab, "name");
            int64_t id = getValueFromTable<int64_t>(tab, "id");
            if (id < 0) {
                BMCL_CRITICAL() << "device id cannot be negative: " << id;
                return ProjectResult();
            }
            if (id == proj->_mccId) {
                BMCL_CRITICAL() << "device id cannot be the same as mcc_id: " << id;
                return ProjectResult();
            }
            auto idsPair = deviceIds.insert(id);
            if (!idsPair.second) {
                BMCL_CRITICAL() << "found devices with conflicting ids";
                return ProjectResult();
            }
            dev.id = id;
            dev.modules = maybeGetArrayFromTable<std::string>(tab, "modules");
            dev.tmSources = maybeGetArrayFromTable<std::string>(tab, "tm_sources");
            dev.cmdTargets = maybeGetArrayFromTable<std::string>(tab, "cmd_targets");
            auto devPair = deviceDescMap.emplace(dev.name, std::move(dev));
            if (!devPair.second) {
                BMCL_CRITICAL() << "device with name " << dev.name << " already exists";
                return ProjectResult();
            }
        }
    } catch (...) {
        return ProjectResult();
    }

    std::vector<std::string> decodeFiles;
    ModuleDescMap moduleDescMap;

    std::string projectDir = projectFilePath;
    removeFilePart(&projectDir);
    for (const std::string& modDir : moduleDirs) {
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
            return ProjectResult();
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
                return ProjectResult();
            }
            int64_t id = getValueFromTable<int64_t>(modToml.unwrap(), "id");
            if (id < 0) {
                BMCL_CRITICAL() << "module id cannot be negative: " << id;
                return ProjectResult();
            }
            auto idsPair = moduleIds.insert(id);
            if (!idsPair.second) {
                BMCL_CRITICAL() << "found modules with conflicting ids";
                return ProjectResult();
            }
            mod.id = id;
            mod.relativeDest = getValueFromTable<std::string>(modToml.unwrap(), "dest");
            decodeFiles.push_back(joinPath(moduleDir, getValueFromTable<std::string>(modToml.unwrap(), "decode")));
            mod.sources = maybeGetArrayFromTable<std::string>(modToml.unwrap(), "sources");
            for (std::string& src : mod.sources) {
                src = joinPath(moduleDir, src);
            }
            auto modPair = moduleDescMap.emplace(moduleName, std::move(mod));
            if (!modPair.second) {
                BMCL_CRITICAL() << "module with name " << mod.name << " already exists";
                return ProjectResult();
            }
        }
    }

    //parse package
    PackageResult package = Package::readFromFiles(cfg, diag, decodeFiles);
    if (package.isErr()) {
        return ProjectResult();
    }
    proj->_package = package.take();

    //check project
    if (deviceDescMap.find(master) == deviceDescMap.end()) {
        BMCL_CRITICAL() << "device with name " << master << " marked as master does not exist";
        return ProjectResult();
    }
    std::vector<Rc<Ast>> commonModules;
    for (const std::string& modName : commonModuleNames) {
        bmcl::OptionPtr<Ast> mod = proj->_package->moduleWithName((modName));
        commonModules.emplace_back(mod.unwrap());
        if (mod.isNone()) {
            BMCL_CRITICAL() << "common module " << modName << " does not exist";
            return ProjectResult();
        }
    }

    for (auto& it : deviceDescMap) {
        Rc<Device> dev = new Device;
        dev->id = it.second.id;
        dev->name = std::move(it.second.name);
        dev->modules = commonModules;

        for (const std::string& modName : it.second.modules) {
            bmcl::OptionPtr<Ast> mod = proj->_package->moduleWithName((modName));
            if (mod.isNone()) {
                BMCL_CRITICAL() << "module " << modName << " does not exist";
                return ProjectResult();
            }
            dev->modules.emplace_back(mod.unwrap());
        }

        it.second.device = dev;
        proj->_devices.push_back(dev);
    }

    for (auto& it : deviceDescMap) {
        Rc<Device> dev = it.second.device;

        for (const std::string& deviceName : it.second.tmSources) {
            if (deviceName == dev->name) {
                //TODO: do not ignore
                continue;
            }
            auto jt = std::find_if(proj->_devices.begin(), proj->_devices.end(), [&deviceName](const Rc<Device>& d) {
                return d->name == deviceName;
            });
            if (jt == proj->_devices.end()) {
                BMCL_CRITICAL() << "unknown tm source: " << deviceName;
                return ProjectResult();
            }
            dev->tmSources.push_back(*jt);
        }

        for (const std::string& deviceName : it.second.cmdTargets) {
            if (deviceName == dev->name) {
                //TODO: do not ignore
                continue;
            }
            auto jt = std::find_if(proj->_devices.begin(), proj->_devices.end(), [&deviceName](const Rc<Device>& d) {
                return d->name == deviceName;
            });
            if (jt == proj->_devices.end()) {
                BMCL_CRITICAL() << "unknown cmd target: " << deviceName;
                return ProjectResult();
            }
            dev->cmdTargets.push_back(*jt);
        }
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

const std::string& Project::name() const
{
    return _name;
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
