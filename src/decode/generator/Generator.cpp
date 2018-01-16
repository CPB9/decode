/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/Generator.h"
#include "decode/generator/OnboardTypeHeaderGen.h"
#include "decode/generator/OnboardTypeSourceGen.h"
#include "decode/generator/DynArrayCollector.h"
#include "decode/generator/StatusEncoderGen.h"
#include "decode/generator/CmdDecoderGen.h"
#include "decode/generator/CmdEncoderGen.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/generator/GcTypeGen.h"
#include "decode/generator/IncludeGen.h"
#include "decode/generator/GcInterfaceGen.h"
#include "decode/generator/GcMsgGen.h"
#include "decode/ast/Ast.h"
#include "decode/ast/Function.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/parser/Package.h"
#include "decode/parser/Project.h"
#include "decode/ast/Decl.h"
#include "decode/ast/Component.h"
#include "decode/ast/Constant.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/Try.h"
#include "decode/core/PathUtils.h"
#include "decode/core/Utils.h"
#include "decode/core/HashMap.h"
#include "decode/core/HashSet.h"

#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>
#include <bmcl/Sha3.h>
#include <bmcl/FixedArrayView.h>

#include <iostream>
#include <deque>
#include <memory>
#include <future>

//TODO: use joinPath

namespace decode {

Generator::Generator(Diagnostics* diag)
    : _diag(diag)
{
}

Generator::~Generator()
{
}

void Generator::setOutPath(bmcl::StringView path)
{
    _savePath.assign(path.begin(), path.end());
}

bool Generator::generateTmPrivate(const Package* package)
{
    _output.clear();

    _output.append("static PhotonTmMessageDesc _messageDesc[] = {\n");
    FuncPrototypeGen prototypeGen(&_output);
    for (const ComponentAndMsg& msg : package->statusMsgs()) {
        _output.appendModIfdef(msg.component->moduleName());
        _output.appendIndent();
        _output.append("{");
        _output.append(".func = ");
        prototypeGen.appendStatusEncoderFunctionName(msg.component.get(), msg.msg.get());
        _output.append(", .compNum = ");
        _output.appendNumericValue(msg.component->number());
        _output.append(", .msgNum = ");
        _output.appendNumericValue(msg.msg->number());
        _output.append(", .interest = ");
        _output.appendNumericValue(0);
        _output.append(", .priority = ");
        _output.appendNumericValue(msg.msg->priority());
        _output.append(", .isEnabled = ");
        _output.appendBoolValue(msg.msg->isEnabled());
        _output.append("},\n");
        _output.appendEndif();
    }
    _output.append("};\n\n");

    _output.append("#define _PHOTON_TM_MSG_COUNT sizeof(_messageDesc) / sizeof(_messageDesc[0])\n\n");

    std::string tmDetailPath = _onboardPath + "/photon/StatusTable.inc.c"; //FIXME: joinPath
    TRY(saveOutput(tmDetailPath.c_str(), _output.view(), _diag.get()));
    _output.clear();

    return true;
}

void Generator::generateSerializedPackage(const Project* project, bmcl::Buffer* serialized, SrcBuilder* sourceCode)
{
    sourceCode->clear();

    *serialized = project->encode();

    sourceCode->appendNumericValueDefine(serialized->size(), "_PHOTON_PACKAGE_SIZE");
    sourceCode->appendEol();
    sourceCode->appendByteArrayDefinition("static const", "_package", *serialized);
    sourceCode->appendEol();

    Project::HashType ctx;
    ctx.update(*serialized);
    auto hash = ctx.finalize();

    sourceCode->appendNumericValueDefine(hash.size(), "_PHOTON_PACKAGE_HASH_SIZE");
    sourceCode->appendEol();
    sourceCode->appendByteArrayDefinition("static const", "_packageHash", hash);
    sourceCode->appendEol();

    for (const Device* dev : project->devices()) {
        sourceCode->appendDeviceIfDef(dev->name());
        sourceCode->appendEol();
        bmcl::Bytes name = bmcl::StringView(dev->name()).asBytes();
        sourceCode->appendNumericValueDefine(name.size(), "_PHOTON_DEVICE_NAME_SIZE");
        sourceCode->appendEol();
        sourceCode->appendByteArrayDefinition("static const", "_deviceName", name);
        sourceCode->appendEol();
        sourceCode->appendEndif();
        sourceCode->appendEol();
    }

}

void Generator::appendBuiltinHeaders()
{
    std::initializer_list<bmcl::StringView> builtin = {"CmdDecoder", "StatusDecoder"};
    appendBuiltins(builtin, ".h");
}

void Generator::appendBuiltinSources()
{
    std::initializer_list<bmcl::StringView> builtin = {"CmdDecoder", "CmdEncoder",
                                                       "StatusEncoder", "StatusDecoder", "EventEncoder"};
    appendBuiltins(builtin, ".c");
}

void Generator::appendBuiltins(bmcl::ArrayView<bmcl::StringView> names, bmcl::StringView ext)
{
    for (bmcl::StringView str : names) {
        _output.append("#include \"photon/");
        _output.append(str);
        _output.append(ext);
        _output.append("\"\n");
    }
}

//TODO: refact
bool Generator::generateDeviceFiles(const Project* project)
{
    HashMap<Rc<const Ast>, std::vector<std::string>> srcsPaths;
    for (const Ast* mod : project->package()->modules()) {
        auto src = project->sourcesForModule(mod);
        if (src.isNone()) {
            continue;
        }

        if (_config.useAbsolutePathsForBundledSources) {
            std::vector<std::string> paths;
            for (const std::string& file : src->sources) {
                paths.emplace_back(absolutePath(file.c_str()));
            }
            srcsPaths.emplace(mod, paths);
        } else {
            std::string dest = _onboardPath;
            joinPath(&dest, src->relativeDest);
            std::size_t destSize = dest.size();

            std::vector<std::string> paths;
            for (const std::string& file : src->sources) {
                bmcl::StringView fname = getFilePart(file);
                joinPath(&dest, fname);
                TRY(copyFile(file.c_str(), dest.c_str(), _diag.get()));
                dest.resize(destSize);
                paths.push_back(joinPath(src->relativeDest, fname));
            }
            srcsPaths.emplace(mod, std::move(paths));
        }
    }

    auto appendBundledSources = [this, &srcsPaths](const Device* dev, bmcl::StringView ext) {
        for (const Ast* module : dev->modules()) {
            auto it = srcsPaths.find(module);
            if (it == srcsPaths.end()) {
                continue;
            }
            for (const std::string& path : it->second) {
                if (!bmcl::StringView(path).endsWith(ext)) {
                    continue;
                }
                _output.append("#include \"");
                _output.append(path);
                _output.append("\"\n");
            }
        }
    };

    TypeDependsCollector coll;
    for (const DeviceConnection* conn : project->deviceConnections()) {
        const Device* dev = conn->device();
        TypeDependsCollector::Depends types;
//         types.insert("core/Reader");
//         types.insert("core/Writer");
//         types.insert("core/Error");

        for (const Ast* module : dev->modules()) {
            coll.collect(module, &types);
        }
        HashSet<Rc<const Ast>> targetMods;
        HashSet<Rc<const Ast>> sourceMods;

        auto appendTargetMods = [&](const Device* dep) {
            for (const Ast* module : dep->modules()) {
                targetMods.emplace(module);
                if (module->component().isNone()) {
                    continue;
                }
                coll.collectCmds(module->component()->cmdsRange(), &types);
            }
        };
        for (const Device* dep : conn->cmdTargets()) {
            appendTargetMods(dep);
        }

        auto appendSourceMods = [&](const Device* dep) {
            for (const Ast* module : dep->modules()) {
                sourceMods.emplace(module);
                if (module->component().isNone()) {
                    continue;
                }
                coll.collectEvents(module->component()->eventsRange(), &types);
                coll.collectStatuses(module->component()->statusesRange(), &types);
            }
        };
        for (const Device* dep : conn->tmSources()) {
            appendSourceMods(dep);
        }

        //header
        if (dev == project->master()) {
            _output.append("#define PHOTON_IS_MASTER\n\n");
        }

        _output.appendNumericValueDefine(dev->id(), "PHOTON_DEVICE_ID");
        for (const Device* d : project->devices()) {
            _output.append("#define PHOTON_DEVICE_ID_");
            _output.appendUpper(d->name());
            _output.appendSpace();
            _output.appendNumericValue(d->id());
            _output.append("\n");
        }
        _output.appendEol();

        auto appendDevTarget = [this](const Device* dep) {
            _output.append("#define PHOTON_HAS_DEVICE_TARGET_");
            _output.appendUpper(dep->name());
            _output.appendEol();
        };
        for (const Device* dep : conn->cmdTargets()) {
            appendDevTarget(dep);
        }
        auto appendDevSource = [this](const Device* dep) {
            _output.append("#define PHOTON_HAS_DEVICE_SOURCE_");
            _output.appendUpper(dep->name());
            _output.appendEol();
        };
        for (const Device* dep : conn->tmSources()) {
            appendDevSource(dep);
        }
        for (const Ast* module : dev->modules()) {
            _output.append("#define PHOTON_HAS_MODULE_");
            _output.appendUpper(module->moduleInfo()->moduleName());
            _output.appendEol();
        }
        for (const Rc<const Ast>& module : targetMods) {
            _output.append("#define PHOTON_HAS_CMD_TARGET_");
            _output.appendUpper(module->moduleInfo()->moduleName());
            _output.appendEol();
        }
        for (const Rc<const Ast>& module : sourceMods) {
            _output.append("#define PHOTON_HAS_TM_SOURCE_");
            _output.appendUpper(module->moduleInfo()->moduleName());
            _output.appendEol();
        }
        _output.appendEol();

        _output.append("#include \"photon/Config.h\"\n\n");

        IncludeGen includeGen(&_output);
        includeGen.genOnboardIncludePaths(&types, ".h");
        _output.appendEol();

        for (const Ast* module : dev->modules()) {
            if (module->component().isSome()) {
                _output.appendComponentInclude(module->moduleInfo()->moduleName(), ".h");
            }
        }
        _output.appendEol();

        appendBuiltinHeaders();
        _output.appendEol();

        appendBundledSources(dev, ".h");

        SrcBuilder path;
        path.append(_onboardPath);
        path.append("/Photon"); //FIXME: joinPath
        path.appendWithFirstUpper(dev->name());
        path.append(".h");
        TRY(saveOutput(path.c_str(), _output.view(), _diag.get()));
        _output.clear();

        //src
        _output.append("#include \"Photon");
        _output.appendWithFirstUpper(dev->name());
        _output.append(".h\"\n\n");
        includeGen.genOnboardIncludePaths(&types, ".gen.c");
        _output.appendEol();

        for (const Ast* module : dev->modules()) {
            if (module->component().isSome()) {
                _output.appendComponentInclude(module->moduleInfo()->moduleName(), ".c");
            }
        }
        _output.appendEol();

        appendBuiltinSources();
        _output.appendEol();

        appendBundledSources(dev, ".c");

        path.back() = 'c';
        TRY(saveOutput(path.c_str(), _output.view(), _diag.get()));
        _output.clear();
    }
    return true;
}

bool Generator::generateConfig(const Project* project)
{
    _onboardPhotonPath.append('/');

    _output.append("#include \"photon/core/Config.h\"");

    TRY(dump("Config", ".h", &_onboardPhotonPath));

    _onboardPhotonPath.removeFromBack(1);
    return true;
}

bool Generator::generateProject(const Project* project, const GeneratorConfig& cfg)
{
    _config = cfg;

    TRY(makeDirectory(_savePath.c_str(), _diag.get()));

    std::string dummyPath = _savePath + "/Photon.dummy.h"; //FIXME: joinPath
    TRY(saveOutput(dummyPath.c_str(), bmcl::StringView::empty(), _diag.get()));

    bmcl::StringView exts[2] = {".c", ".h"};
    for (bmcl::StringView ext : exts) {
        for (const Device* dev : project->devices()) {
            _output.append("#ifdef PHOTON_DEVICE_");
            _output.appendUpper(dev->name());
            _output.appendEol();
            _output.append("#include \"Photon");
            _output.appendWithFirstUpper(dev->name());
            _output.append(ext);
            _output.append("\"\n");
            _output.appendEndif();
        }

        std::string photoncPath = _savePath + "/Photon" + ext.toStdString();
        TRY(saveOutput(photoncPath.c_str(), _output.view(), _diag.get()));
        _output.clear();
    }

    _onboardPath = _savePath;
    TRY(makeDirectory(_onboardPath.c_str(), _diag.get()));
    _gcPath = _savePath;
    TRY(makeDirectory(_gcPath.c_str(), _diag.get()));
    _onboardPhotonPath.assign(joinPath(_onboardPath, "photon"));
    TRY(makeDirectory(_onboardPhotonPath.c_str(), _diag.get()));
    _gcPhotonPath.assign(joinPath(_gcPath, "photon"));
    TRY(makeDirectory(_gcPhotonPath.c_str(), _diag.get()));

    SrcBuilder packageSourceCode;
    bmcl::Buffer serializedProject;
    packageSourceCode.reserve(1024 * 1024);
    _output.reserve(1024 * 1024);
    auto future = std::async(std::launch::async, &Generator::generateSerializedPackage, project, &serializedProject, &packageSourceCode);

    _onboardPhotonPath.append('/'); //FIXME: remove
    _gcPhotonPath.append('/'); //FIXME: remove

    const Package* package = project->package();

    _onboardHgen.reset(new OnboardTypeHeaderGen(&_output));
    _onboardSgen.reset(new OnboardTypeSourceGen(&_output));
    for (const Ast* it : package->modules()) {
        if (!generateTypesAndComponents(it)) {
            return false;
        }
    }

    TRY(generateGenerics(package));
    TRY(generateConfig(project));
    TRY(generateDynArrays(package));
    TRY(generateTmPrivate(package));
    TRY(generateStatusMessages(project));
    TRY(generateCommands(package));
    TRY(generateDeviceFiles(project));

    GcInterfaceGen igen(&_output);
    igen.generateHeader(package);
    std::string interfacePath = _savePath + "/Photon.hpp";
    TRY(saveOutput(interfacePath.c_str(), _output.view(), _diag.get()));
    _output.clear();

    future.wait();
    std::string packageDetailPath = _onboardPath + "/photon/Package.inc.c"; //FIXME: joinPath
    TRY(saveOutput(packageDetailPath.c_str(), packageSourceCode.view(), _diag.get()));
    std::string packageBlobPath = _onboardPath + "/photon/Package.bin"; //FIXME: joinPath
    TRY(saveOutput(packageBlobPath.c_str(), serializedProject, _diag.get()));

    _output.clear();
    _onboardHgen.reset();
    _onboardSgen.reset();
    _onboardPhotonPath.clear();
    _onboardPath.clear();
    _gcPath.clear();
    return true;
}

#define GEN_PREFIX ".gen"

bool Generator::generateDynArrays(const Package* package)
{
    RcSecondUnorderedMap<std::string, const DynArrayType> dynArrays;
    DynArrayCollector coll;
    for (const Ast* ast : package->modules()) {
        for (const Type* type : ast->typesRange()) {
            coll.collectUniqueDynArrays(type, &dynArrays);
        }
    }

    _onboardPhotonPath.append("_dynarray_");
    TRY(makeDirectory(_onboardPhotonPath.c_str(), _diag.get()));
    _onboardPhotonPath.append('/');

    for (const auto& it : dynArrays) {
        _onboardHgen->genDynArrayHeader(it.second.get());
        TRY(dumpIfNotEmpty(it.first, ".h", &_onboardPhotonPath));

        _onboardSgen->genTypeSource(it.second.get());
        TRY(dumpIfNotEmpty(it.first, GEN_PREFIX ".c", &_onboardPhotonPath));
    }

    _onboardPhotonPath.removeFromBack(std::strlen("_dynarray_/"));
    return true;
}

bool Generator::generateStatusMessages(const Project* project)
{
    StatusEncoderGen gen(&_output);
    gen.generateStatusEncoderSource(project);
    TRY(dump("StatusEncoder", ".c", &_onboardPhotonPath));

    gen.generateStatusDecoderHeader(project);
    TRY(dump("StatusDecoder", ".h", &_onboardPhotonPath));

    gen.generateStatusDecoderSource(project);
    TRY(dump("StatusDecoder", ".c", &_onboardPhotonPath));

    gen.generateEventEncoderSource(project);
    TRY(dump("EventEncoder", ".c", &_onboardPhotonPath));

    std::size_t pathSize = _gcPhotonPath.size();
    _gcPhotonPath.append("_statuses_");
    TRY(makeDirectory(_gcPhotonPath.c_str(), _diag.get()));
    _gcPhotonPath.append('/');

    //refact
    GcMsgGen msgGen(&_output);
    SrcBuilder msgName;
    for (const Component* comp : project->package()->components()) {
        for (const StatusMsg* msg : comp->statusesRange()) {
            msgName.appendWithFirstUpper(comp->name());
            msgName.append("_");
            msgName.appendWithFirstUpper(msg->name());
            msgGen.generateStatusHeader(comp, msg);
            TRY(dumpIfNotEmpty(msgName.view(), ".hpp", &_gcPhotonPath));
            msgName.clear();
        }
    }
    _gcPhotonPath.resize(pathSize);

    _gcPhotonPath.append("_events_");
    TRY(makeDirectory(_gcPhotonPath.c_str(), _diag.get()));
    _gcPhotonPath.append('/');

    for (const Component* comp : project->package()->components()) {
        for (const EventMsg* msg : comp->eventsRange()) {
            msgName.appendWithFirstUpper(comp->name());
            msgName.append("_");
            msgName.appendWithFirstUpper(msg->name());
            msgGen.generateEventHeader(comp, msg);
            TRY(dumpIfNotEmpty(msgName.view(), ".hpp", &_gcPhotonPath));
            msgName.clear();
        }
    }

    _gcPhotonPath.resize(pathSize);
    return true;
}

bool Generator::generateCommands(const Package* package)
{
    CmdDecoderGen decGen(&_output);
    decGen.generateHeader(package->components());
    TRY(dump("CmdDecoder", ".h", &_onboardPhotonPath));

    decGen.generateSource(package->components());
    TRY(dump("CmdDecoder", ".c", &_onboardPhotonPath));

    CmdEncoderGen encGen(&_output);
    encGen.generateSource(package->components());
    TRY(dump("CmdEncoder", ".c", &_onboardPhotonPath));

    return true;
}

bool Generator::dumpIfNotEmpty(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath)
{
    if (!_output.empty()) {
        TRY(dump(name, ext, currentPath));
    }
    return true;
}

bool Generator::dump(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath)
{
    currentPath->appendWithFirstUpper(name);
    currentPath->append(ext);
    TRY(saveOutput(currentPath->c_str(), _output.view(), _diag.get()));
    currentPath->removeFromBack(name.size() + ext.size());
    _output.clear();
    return true;
}

bool Generator::generateGenerics(const Package* package)
{
    _onboardPhotonPath.append("_generic_");
    TRY(makeDirectory(_onboardPhotonPath.c_str(), _diag.get()));
    _onboardPhotonPath.append('/');

    _gcPhotonPath.append("_generic_");
    TRY(makeDirectory(_gcPhotonPath.c_str(), _diag.get()));
    _gcPhotonPath.append('/');

    SrcBuilder typeNameBuilder;
    TypeNameGen typeNameGen(&typeNameBuilder);
    GcTypeGen gcTypeGen(&_output);
    for (const Ast* ast : package->modules()) {
        for (const GenericInstantiationType* type : ast->genericInstantiationsRange()) {
            typeNameGen.genTypeName(type);

            _onboardHgen->genTypeHeader(ast, type, typeNameBuilder.view());
            TRY(dump(typeNameBuilder.view(), ".h", &_onboardPhotonPath));

            _onboardSgen->genTypeSource(type, typeNameBuilder.view());
            TRY(dump(typeNameBuilder.view(), GEN_PREFIX ".c", &_onboardPhotonPath));

            gcTypeGen.generateHeader(type);
            TRY(dump(typeNameBuilder.view(), ".hpp", &_gcPhotonPath));

            typeNameBuilder.clear();
        }
    }
    _onboardPhotonPath.removeFromBack(10);
    _gcPhotonPath.removeFromBack(10);
    return true;
}

bool Generator::generateTypesAndComponents(const Ast* ast)
{
    _onboardPhotonPath.append(ast->moduleName());
    TRY(makeDirectory(_onboardPhotonPath.c_str(), _diag.get()));
    _onboardPhotonPath.append('/'); //FIXME: joinPath

    _gcPhotonPath.append(ast->moduleName());
    TRY(makeDirectory(_gcPhotonPath.c_str(), _diag.get()));
    _gcPhotonPath.append('/'); //FIXME: joinPath

    SrcBuilder typeNameBuilder;
    TypeNameGen typeNameGen(&typeNameBuilder);
    GcTypeGen gcTypeGen(&_output);
    for (const NamedType* type : ast->namedTypesRange()) {
        if (type->typeKind() == TypeKind::Imported) {
            continue;
        }
        if (type->typeKind() != TypeKind::Generic) {
            typeNameGen.genTypeName(type);

            _onboardHgen->genTypeHeader(ast, type, typeNameBuilder.view());
            TRY(dump(type->name(), ".h", &_onboardPhotonPath));

            _onboardSgen->genTypeSource(type, typeNameBuilder.view());
            TRY(dump(type->name(), GEN_PREFIX ".c", &_onboardPhotonPath));

            typeNameBuilder.clear();
        }
        gcTypeGen.generateHeader(type);
        TRY(dump(type->name(), ".hpp", &_gcPhotonPath));

    }

    if (ast->component().isSome()) {
        bmcl::OptionPtr<const Component> comp = ast->component();

        _onboardHgen->genComponentHeader(ast, comp.unwrap());
        TRY(dumpIfNotEmpty(comp->moduleName(), ".Component.h", &_onboardPhotonPath));
        _output.appendComponentInclude(comp->moduleName(), ".h");
        _output.appendEol();
        if (comp->hasParams()) {
            _output.append("Photon");
            _output.appendWithFirstUpper(comp->moduleName());
            _output.append(" _photon");
            _output.appendWithFirstUpper(comp->moduleName());
            _output.append(';');
        }
        TRY(dumpIfNotEmpty(comp->moduleName(), ".Component.c", &_onboardPhotonPath));

        //sgen->genTypeSource(type);
        //TRY(dump(type->name(), GEN_PREFIX ".c", &photonPath));
    }

    if (ast->hasConstants()) {
        _onboardHgen->startIncludeGuard(ast->moduleName(), "CONSTANTS");
        for (const Constant* c : ast->constantsRange()) {
            _output.append("#define PHOTON_");
            _output.appendUpper(ast->moduleName());
            _output.append('_');
            _output.append(c->name());
            _output.append(" ");
            _output.appendNumericValue(c->value());
            _output.appendEol();
        }
        _output.appendEol();
        _onboardHgen->endIncludeGuard();
        TRY(dumpIfNotEmpty(ast->moduleName(), ".Constants.h", &_onboardPhotonPath));
    }

    _onboardPhotonPath.removeFromBack(ast->moduleName().size() + 1);
    _gcPhotonPath.removeFromBack(ast->moduleName().size() + 1);
    return true;
}
}
