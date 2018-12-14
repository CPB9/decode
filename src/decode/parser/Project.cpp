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
#include "decode/core/PathUtils.h"
#include "decode/parser/Package.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Ast.h"
#include "decode/ast/Component.h"
#include "decode/generator/Generator.h"
#include "decode/core/Zpaq.h"
#include "decode/core/Utils.h"
#include "decode/core/ProgressPrinter.h"
#include "decode/core/HashMap.h"

#include <bmcl/Result.h>
#include <bmcl/StringView.h>
#include <bmcl/Logging.h>
#include <bmcl/MemReader.h>
#include <bmcl/FileUtils.h>
#include <bmcl/Sha3.h>
#include <bmcl/FixedArrayView.h>

#include <toml11/toml.hpp>

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
    Rc<Device> device;
    Rc<DeviceConnection> connection;
};

struct ModuleDesc {
    std::string name;
    std::uint64_t id;
    Project::SourcesToCopy sources;
};

using DeviceDescMap = HashMap<std::string, DeviceDesc>;
using ModuleDescMap = HashMap<std::string, ModuleDesc>;

Project::Project(Configuration* cfg, Diagnostics* diag)
    : _cfg(cfg)
    , _diag(diag)
{
}

Project::~Project()
{
}

class ParseException {
public:
    ParseException()
    {
    }

    explicit ParseException(const std::string& err)
        : _error(err)
    {
    }

    explicit ParseException(std::string&& err)
        : _error(std::move(err))
    {
    }

    const std::string& what() const
    {
        return _error;
    }

private:
    std::string _error;
};

template <typename T>
const T& getValueFromTable(const toml::Table& table, const char* key)
{
    try {
        const toml::value& value = table.at(key);
        return value.cast<toml::detail::check_type<T>()>();
    } catch (const std::out_of_range& exc) {
        throw ParseException(std::string("invalid table key (") + key + ")");
    } catch (const toml::type_error& exc) {
        throw ParseException(std::string("invalid value type (") + key + ")");
    } catch (...) {
        throw ParseException(std::string("unknown toml error (") + key + ")");
    }
}

template <typename T>
std::vector<T> maybeGetArrayFromTable(const toml::Table& table, const char* key)
{
    try {
        return toml::get<std::vector<T>>(table.at(key));
    } catch (const std::out_of_range& exc) {
        return std::vector<T>();
    } catch (const toml::type_error& exc) {
        throw ParseException(std::string("invalid value type (") + key + ")");
    } catch (...) {
        throw ParseException(std::string("unknown toml error (") + key + ")");
    }
}

using TableResult = bmcl::Result<toml::Table, void>;

static void addError(bmcl::StringView msg, bmcl::StringView cause, Diagnostics* diag)
{
    diag->buildSystemErrorReport(msg, cause);
}

static void addParseError(bmcl::StringView path, bmcl::StringView cause, Diagnostics* diag)
{
    std::string msg = "failed to parse file ";
    msg.append(path.begin(), path.end());
    addError(msg, cause, diag);
}

static TableResult readToml(const std::string& path, Diagnostics* diag)
{
    auto file = bmcl::readFileIntoString(path.c_str());
    if (file.isErr()) {
        diag->buildSystemFileErrorReport("failed to read file", file.unwrapErr(), path);
        return TableResult();
    }
    std::string::iterator begin = file.unwrap().begin();
    std::string::iterator end = file.unwrap().end();
    try {
        return toml::parse_data::invoke(begin, end);
    } catch (const std::pair<std::string::iterator, toml::syntax_error>& exc) {
        addParseError(path, exc.second.what(), diag);
        //BMCL_DEBUG() << std::string(exc.first, end);
        return TableResult();
    }
    return TableResult();
}

ProjectResult Project::fromFile(Configuration* cfg, Diagnostics* diag, const char* path)
{
    std::string projectFilePath(path);
    normalizePath(&projectFilePath);
    Rc<Project> proj = new Project(cfg, diag);

    ProgressPrinter printer(cfg->verboseOutput());
    printer.printActionProgress("Reading", "project file `" + projectFilePath + "`");
    TableResult projectFile = readToml(projectFilePath, diag);
    if (projectFile.isErr()) {
        return ProjectResult();
    }

    std::string master;
    DeviceDescMap deviceDescMap;
    std::vector<std::string> moduleDirs;
    std::vector<std::string> commonModuleNames;
    std::map<int64_t, bmcl::StringView> componentNumToName;
    std::map<bmcl::StringView, int64_t, Package::StringViewComparator> componentNameToNum;
    //read project file
    try {
        const toml::Table& projectTable = getValueFromTable<toml::Table>(projectFile.unwrap(), "project");
        proj->_name = getValueFromTable<std::string>(projectTable, "name");
        master = getValueFromTable<std::string>(projectTable, "master");
        int64_t id = getValueFromTable<std::int64_t>(projectTable, "mcc_id");
        if (id < 0) {
            addParseError(path, "mcc_id cannot be negative (" + std::to_string(id) + ")", diag);
            return ProjectResult();
        }
        proj->_mccId = id;
        commonModuleNames = maybeGetArrayFromTable<std::string>(projectTable, "common_modules");
        moduleDirs = maybeGetArrayFromTable<std::string>(projectTable, "module_dirs");
        removeDuplicates(&moduleDirs);
        for (std::string& path : moduleDirs) {
            normalizePath(&path);
        }

        auto numsIt = projectFile.unwrap().find("component_numbers");
        if (numsIt != projectFile.unwrap().end()) {
            if (numsIt->second.type() != toml::value_t::Table) {
                addParseError(path, "component_numbers must be a table", diag);
                return ProjectResult();
            }
            const toml::Table& nums = numsIt->second.cast<toml::value_t::Table>();
            for (const auto& it : nums) {
                if (it.second.type() != toml::value_t::Integer) {
                    addParseError(path, "component \"" + it.first + "\" number must be an integer", diag);
                    return ProjectResult();
                }
                const std::string& name = it.first;
                int64_t num = it.second.cast<toml::value_t::Integer>();
                componentNumToName.emplace(num, name);
                componentNameToNum.emplace(name, num);
            }
        }

        const toml::Array& devicesArray = getValueFromTable<toml::Array>(projectFile.unwrap(), "devices");
        std::set<uint64_t> deviceIds;
        for (const toml::value& value : devicesArray) {
            if (value.type() != toml::value_t::Table) {
                addParseError(path, "devices section must be a table", diag);
                return ProjectResult();
            }
            const toml::Table& tab = value.cast<toml::value_t::Table>();
            DeviceDesc dev;
            dev.name = getValueFromTable<std::string>(tab, "name");
            int64_t id = getValueFromTable<int64_t>(tab, "id");
            if (id < 0) {
                addParseError(path, "device id cannot be negative (" + std::to_string(id) + ")", diag);
                return ProjectResult();
            }
            if (uint64_t(id) == proj->_mccId) {
                addParseError(path, "device id cannot be the same as mcc_id (" + std::to_string(id) + ")", diag);
                return ProjectResult();
            }
            auto idsPair = deviceIds.insert(id);
            if (!idsPair.second) {
                addParseError(path, "devices cannot have the same id (" + std::to_string(id) + ")", diag);
                return ProjectResult();
            }
            dev.id = id;
            dev.modules = maybeGetArrayFromTable<std::string>(tab, "modules");
            dev.tmSources = maybeGetArrayFromTable<std::string>(tab, "tm_sources");
            dev.cmdTargets = maybeGetArrayFromTable<std::string>(tab, "cmd_targets");
            auto devPair = deviceDescMap.emplace(dev.name, std::move(dev));
            if (!devPair.second) {
                addParseError(path, "device with name '" + dev.name + "' already exists", diag);
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
    for (const std::string& relativeModDir : moduleDirs) {
        std::string dirPath;
        if (!isAbsPath(relativeModDir)) {
            dirPath = projectDir;
            joinPath(&dirPath, relativeModDir);
        } else {
            dirPath = relativeModDir;
        }
        std::set<uint64_t> moduleIds;
        ModuleDesc mod;
        std::string modTomlPath = dirPath;
        joinPath(&modTomlPath, "mod.toml");
        printer.printActionProgress("Reading", "module `" + modTomlPath + "`");
        TableResult modToml = readToml(modTomlPath, diag);
        if (modToml.isErr()) {
            return ProjectResult();
        }
        int64_t id = getValueFromTable<int64_t>(modToml.unwrap(), "id");
        if (id < 0) {
            addParseError(modTomlPath, "module id cannot be negative (" + std::to_string(id) + ")", diag);
            return ProjectResult();
        }
        auto idsPair = moduleIds.insert(id);
        if (!idsPair.second) {
            addParseError(modTomlPath, "module with id '" + std::to_string(id) + "' already exists", diag);
            return ProjectResult();
        }
        mod.name = getValueFromTable<std::string>(modToml.unwrap(), "name");
        mod.id = id;
        mod.sources.relativeDest = getValueFromTable<std::string>(modToml.unwrap(), "dest");
        std::string decodePath = getValueFromTable<std::string>(modToml.unwrap(), "decode");
        decodeFiles.push_back(joinPath(dirPath, decodePath));
        mod.sources.sources = maybeGetArrayFromTable<std::string>(modToml.unwrap(), "sources");
        for (std::string& src : mod.sources.sources) {
            src = joinPath(dirPath, src);
        }
        auto modPair = moduleDescMap.emplace(mod.name, std::move(mod));
        if (!modPair.second) {
            addParseError(modTomlPath, "module with name '" + mod.name + "' already exists", diag);
            return ProjectResult();
        }
    }

    PackageResult package = Package::readFromFiles(cfg, diag, decodeFiles);
    if (package.isErr()) {
        return ProjectResult();
    }
    proj->_package = package.take();

    std::size_t currentCompNum = 0;
    for (Component* comp : proj->_package->components()) {
        auto nameToNumIt = componentNameToNum.find(comp->name());
        if (nameToNumIt != componentNameToNum.end()) {
            comp->setNumber(nameToNumIt->second);
        } else {
            while (true) {
                auto numToNameIt = componentNumToName.find(currentCompNum);
                if (numToNameIt == componentNumToName.end()) {
                    comp->setNumber(currentCompNum);
                    currentCompNum++;
                    break;
                }
                currentCompNum++;
            }
        }
    }
    proj->_package->sortComponentsByNumber();

    //check project
    if (deviceDescMap.find(master) == deviceDescMap.end()) {
        addParseError(path, "device with name '" + master + "' marked as master does not exist", diag);
        return ProjectResult();
    }
    RcVec<Ast> commonModules;
    for (const std::string& modName : commonModuleNames) {
        bmcl::OptionPtr<Ast> mod = proj->_package->moduleWithName((modName));
        commonModules.emplace_back(mod.unwrap());
        if (mod.isNone()) {
            addParseError(path, "common module '" + modName + "' does not exist", diag);
            return ProjectResult();
        }
    }

    for (auto& it : moduleDescMap) {
        bmcl::OptionPtr<Ast> mod = proj->_package->moduleWithName(it.second.name);
        if (mod.isNone()) {
            addParseError(path, "module '" + it.second.name + "' does not exist", diag);
            return ProjectResult();
        }
        proj->_sourcesMap.emplace(mod.unwrap(), std::move(it.second.sources));
    }

    for (auto& it : deviceDescMap) {
        Rc<Device> dev = new Device;
        dev->_package = proj->_package;
        dev->_id = it.second.id;
        dev->_name = std::move(it.second.name);
        dev->_modules = commonModules;

        for (const std::string& modName : it.second.modules) {
            bmcl::OptionPtr<Ast> mod = proj->_package->moduleWithName(modName);
            if (mod.isNone()) {
                addParseError(path, "module '" + it.second.name + "' does not exist", diag);
                return ProjectResult();
            }
            dev->_modules.emplace_back(mod.unwrap());
        }

        it.second.device = dev;
        it.second.connection = new DeviceConnection(dev.get());
        proj->_devices.push_back(dev);
        proj->_connections.push_back(it.second.connection);
        if (it.first == master) {
            proj->_master = dev;
        }
    }

    for (auto& it : deviceDescMap) {
        Rc<Device> dev = it.second.device;

        for (const std::string& deviceName : it.second.tmSources) {
            auto jt = std::find_if(proj->_devices.begin(), proj->_devices.end(), [&deviceName](const Rc<Device>& d) {
                return d->_name == deviceName;
            });
            if (jt == proj->_devices.end()) {
                addParseError(path, "unknown tm source (" + deviceName + ")", diag);
                return ProjectResult();
            }
            it.second.connection->_tmSources.push_back(*jt);
        }

        for (const std::string& deviceName : it.second.cmdTargets) {
            auto jt = std::find_if(proj->_devices.begin(), proj->_devices.end(), [&deviceName](const Rc<Device>& d) {
                return d->_name == deviceName;
            });
            if (jt == proj->_devices.end()) {
                addParseError(path, "unknown cmd target (" + deviceName + ")", diag);
                return ProjectResult();
            }
            it.second.connection->_cmdTargets.push_back(*jt);
        }
    }
    return proj;
}

bool Project::generate(const char* destDir, const GeneratorConfig& cfg)
{
    ProgressPrinter printer(_cfg->verboseOutput());
    printer.printActionProgress("Generating", "sources");

    Rc<Generator> gen = new Generator(_diag.get());
    gen->setOutPath(destDir);
    bool genOk = gen->generateProject(this, cfg);
    //if (genOk) {
    //    BMCL_DEBUG() << "generating complete";
    //} else {
    //    BMCL_DEBUG() << "generating failed";
    //}
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

typedef std::array<std::uint8_t, 4> MagicType;
const MagicType magic = {{0x7a, 0x70, 0x61, 0x71}};

ProjectResult Project::decodeFromMemory(Diagnostics* diag, const void* src, std::size_t size)
{
    ZpaqResult rv = zpaqDecompress(src, size);
    if (rv.isErr()) {
        addError("error decompressing project from memory", rv.unwrapErr(), diag);
        return ProjectResult();
    }

    auto addReadErr = [diag](bmcl::StringView cause) {
        addError("error parsing project from memory", cause, diag);
    };

    auto addReadStrErr = [&addReadErr](bmcl::StringView cause, bmcl::StringView strCause) {
        addReadErr(cause.toStdString() + "(" + strCause.toStdString() + ")");
    };

    bmcl::MemReader reader(rv.unwrap().data(), rv.unwrap().size());
    if (reader.readableSize() < (magic.size() + 2)) {
        addReadErr("Unexpected EOF reading magic");
        return ProjectResult();
    }

    MagicType m;
    reader.read(m.data(), m.size());

    if (m != magic) {
        //TODO: print hex magic
        addReadErr("Invalid magic");
        return ProjectResult();
    }

    Rc<Configuration> cfg = new Configuration;

    cfg->setGeneratedCodeDebugLevel(reader.readUint8());
    cfg->setCompressionLevel(reader.readUint8());

    uint64_t numOptions;
    if (!reader.readVarUint(&numOptions)) {
        addReadErr("Error reading option number");
        return ProjectResult();
    }
    for (uint64_t i = 0; i < numOptions; i++) {
        auto key = deserializeString(&reader);
        if (key.isErr()) {
            addReadStrErr("Error reading option key", key.unwrapErr());
            return ProjectResult();
        }

        if (reader.readableSize() < 1) {
            addReadErr("Unexpected EOF reading project option");
            return ProjectResult();
        }

        bool hasValue = reader.readUint8();
        if (hasValue) {
            auto value = deserializeString(&reader);
            if (value.isErr()) {
                addReadStrErr("Error reading option value", value.unwrapErr());
                return ProjectResult();
            }

            cfg->setCfgOption(key.unwrap(), value.unwrap());
        } else {
            cfg->setCfgOption(key.unwrap());
        }
    }

    if (reader.readableSize() < 4) {
        addReadErr("Unexpected EOF reading mcc_id");
        return ProjectResult();
    }

    uint64_t mccId;
    if (!reader.readVarUint(&mccId)) {
        addReadErr("Error reading mcc_id");
        return ProjectResult();
    }

    auto name = deserializeString(&reader);
    if (name.isErr()) {
        addReadStrErr("Error reading project name", name.unwrapErr());
        return ProjectResult();
    }

    uint32_t packageSize = reader.readUint32Le();

    PackageResult package = Package::decodeFromMemory(cfg.get(), diag, reader.current(), packageSize);

    if (package.isErr()) {
        return ProjectResult();
    }
    reader.skip(packageSize);

    uint64_t devNum;
    if (!reader.readVarUint(&devNum)) {
        addReadErr("Error reading device number");
        return ProjectResult();
    }

    uint64_t masterIndex;
    if (!reader.readVarUint(&masterIndex)) {
        addReadErr("Error reading master index");
        return ProjectResult();
    }
    if (masterIndex >= devNum) {
        addReadErr("Invalid master index");
        return ProjectResult();
    }

    std::vector<Rc<Device>> devices;
    std::vector<Rc<DeviceConnection>> connections;
    for (uint64_t i = 0; i < devNum; i++) {
        Rc<Device> dev = new Device;
        Rc<DeviceConnection> conn = new DeviceConnection(dev.get());
        connections.push_back(conn);
        dev->_package = package.unwrap();
        if (!reader.readVarUint(&dev->_id)) {
            addReadErr("Error reading device id");
            return ProjectResult();
        }

        auto name = deserializeString(&reader);
        if (name.isErr()) {
            addReadStrErr("Error reading device name", name.unwrapErr());
            return ProjectResult();
        }
        dev->_name = name.unwrap().toStdString();

        uint64_t modNum;
        if (!reader.readVarUint(&modNum)) {
            addReadErr("Error reading module number");
            return ProjectResult();
        }

        for (uint64_t j = 0; j < modNum; j++) {
            auto modName = deserializeString(&reader);
            if (name.isErr()) {
                addReadStrErr("Error reading module name", modName.unwrapErr());
                return ProjectResult();
            }
            auto mod = package.unwrap()->moduleWithName(modName.unwrap());
            if (mod.isNone()) {
                addReadErr("Invalid module name reference");
                return ProjectResult();
            }
            dev->_modules.emplace_back(mod.unwrap());
        }
        devices.push_back(std::move(dev));
    }

    for (uint64_t i = 0; i < devNum; i++) {
        uint64_t deviceIndex;
        if (!reader.readVarUint(&deviceIndex)) {
            addReadErr("Error reading device index");
            return ProjectResult();
        }
        if (deviceIndex >= devices.size()) {
            addReadErr("Device index too big");
            return ProjectResult();
        }
        Rc<Device> current = devices[deviceIndex];
        Rc<DeviceConnection> conn = connections[deviceIndex];

        auto updateRefs = [&reader, &devices, &addReadErr](std::vector<Rc<Device>>* target) -> bool {
            uint64_t num;
            if (!reader.readVarUint(&num)) {
                addReadErr("Error reading device number");
                return false;
            }

            for (uint64_t j = 0; j < num; j++) {
                uint64_t index;
                if (!reader.readVarUint(&index)) {
                    addReadErr("Error reading device index");
                    return false;
                }
                if (index >= devices.size()) {
                    addReadErr("Invalid device index");
                    return false;
                }
                target->push_back(devices[index]);
            }
            return true;
        };
        if (!updateRefs(&conn->_tmSources)) {
            return ProjectResult();
        }

        if (!updateRefs(&conn->_cmdTargets)) {
            return ProjectResult();
        }
    }

    for (uint64_t i = 0; i < package.unwrap()->components().size(); i++) {
        auto compName = deserializeString(&reader);
        if (name.isErr()) {
            addReadStrErr("Error reading component name", compName.unwrapErr());
            return ProjectResult();
        }
        uint64_t num;
        if (!reader.readVarUint(&num)) {
            addReadErr("Error reading component number");
            return ProjectResult();
        }
        auto mod = package.unwrap()->moduleWithName(compName.unwrap());
        if (mod.isNone()) {
            addReadErr("Invalid component name reference");
            return ProjectResult();
        }
        if (mod.unwrap()->component().isNone()) {
            addReadErr("Invalid component name reference");
            return ProjectResult();
        }
        mod.unwrap()->component()->setNumber(num);
    }
    package.unwrap()->sortComponentsByNumber();

    if (reader.current() != reader.end()) {
        addReadErr("Expected EOF");
        return ProjectResult();
    }

    Rc<Project> proj = new Project(cfg.get(), diag);
    proj->_master = devices[masterIndex];
    proj->_devices = std::move(devices);
    proj->_connections = std::move(connections);
    proj->_package = package.take();
    proj->_mccId = mccId;
    proj->_name = name.unwrap().toStdString();
    return proj;
}

bmcl::Buffer Project::encode() const
{
    bmcl::Buffer dest;
    dest.write(magic.data(), magic.size());

    dest.writeUint8(_cfg->generatedCodeDebugLevel());
    dest.writeUint8(_cfg->compressionLevel());

    dest.writeVarUint(_cfg->numOptions());
    for (const auto& it : _cfg->optionsRange()) {
        dest.writeVarUint(it.first.size());
        dest.write(it.first.data(), it.first.size());
        if (it.second.isSome()) {
            dest.writeUint8(1);
            dest.writeVarUint(it.second->size());
            dest.write(it.second->data(), it.second->size());
        } else {
            dest.writeUint8(0);
        }
    }

    dest.writeVarUint(_mccId);
    serializeString(_name, &dest);

    std::size_t sizeOffset = dest.size();
    dest.writeUint32(0);
    _package->encode(&dest);

    std::size_t packageSize = dest.size() - sizeOffset - 4;
    le32enc(dest.data() + sizeOffset, packageSize);

    dest.writeVarUint(_devices.size());

    auto mt = std::find(_devices.begin(), _devices.end(), _master);
    assert(mt != _devices.end());
    dest.writeVarUint(mt - _devices.begin());

    for (const Rc<Device>& dev : _devices) {
        dest.writeVarUint(dev->_id);
        serializeString(dev->_name, &dest);
        dest.writeVarUint(dev->_modules.size());
        for (const Rc<Ast>& module : dev->_modules) {
            serializeString(module->moduleInfo()->moduleName(), &dest);
        }
    }

    for (std::size_t i = 0; i < _devices.size(); i++) {
        const Rc<DeviceConnection>& conn = _connections[i];
        dest.writeVarUint(i);
        dest.writeVarUint(conn->_tmSources.size());
        //TODO: refact
        for (const Rc<Device>& tmSrc : conn->_tmSources) {
            auto it = std::find(_devices.begin(), _devices.end(), tmSrc);
            assert(it != _devices.end());
            dest.writeVarUint(it - _devices.begin());
        }

        dest.writeVarUint(conn->_cmdTargets.size());
        for (const Rc<Device>& tmSrc : conn->_cmdTargets) {
            auto it = std::find(_devices.begin(), _devices.end(), tmSrc);
            assert(it != _devices.end());
            dest.writeVarUint(it - _devices.begin());
        }
    }

    for (const Component* comp : _package->components()) {
        serializeString(comp->name(), &dest);
        dest.writeVarUint(comp->number());
    }

    //BMCL_DEBUG() << "uncompressed project size: " << dest.size();

    ZpaqResult compressed = zpaqCompress(dest.data(), dest.size(), _cfg->compressionLevel());
    assert(compressed.isOk());

    //BMCL_DEBUG() << "compressed project size: " << compressed.unwrap().size();

    return compressed.take();
}

DeviceVec::ConstIterator Project::devicesBegin() const
{
    return _devices.begin();
}

DeviceVec::ConstIterator Project::devicesEnd() const
{
    return _devices.end();
}

DeviceVec::ConstRange Project::devices() const
{
    return _devices;
}

RcVec<DeviceConnection>::ConstRange Project::deviceConnections() const
{
    return _connections;
}

const Device* Project::master() const
{
    return _master.get();
}

bmcl::Option<const Project::SourcesToCopy&> Project::sourcesForModule(const Ast* module) const
{
    auto it = _sourcesMap.find(module);
    if (it == _sourcesMap.end()) {
        return bmcl::None;
    }
    return it->second;
}

bmcl::OptionPtr<const Device> Project::deviceWithName(bmcl::StringView name) const
{
    auto it = std::find_if(_devices.begin(), _devices.end(), [&name](const Rc<Device>& dev) {
        return dev->_name == name;
    });
    if (it == _devices.end()) {
        return bmcl::None;
    }
    return it->get();
}

bmcl::OptionPtr<Device> Project::deviceWithName(bmcl::StringView name)
{
    auto it = std::find_if(_devices.begin(), _devices.end(), [&name](const Rc<Device>& dev) {
        return dev->_name == name;
    });
    if (it == _devices.end()) {
        return bmcl::None;
    }
    return it->get();
}

std::array<std::uint8_t, 512 / 8> Project::hash(bmcl::Bytes data)
{
    return Project::HashType::calcInOneStep(data);
}

Device::Device()
    : _id(0)
{
}

Device::~Device()
{
}

const Package* Device::package() const
{
    return _package.get();
}

uint64_t Device::id() const
{
    return _id;
}

const std::string& Device::name() const
{
    return _name;
}

RcVec<Ast>::ConstRange Device::modules() const
{
    return _modules;
}

DeviceConnection::DeviceConnection(const Device* dev)
    : _device(dev)
{
}

DeviceConnection::~DeviceConnection()
{
}

const Device* DeviceConnection::device() const
{
    return _device.get();
}

DeviceVec::ConstRange DeviceConnection::tmSources() const
{
    return _tmSources;
}

DeviceVec::ConstRange DeviceConnection::cmdTargets() const
{
    return _cmdTargets;
}
}
