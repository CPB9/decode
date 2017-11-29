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
#include "decode/generator/GcStatusMsgGen.h"
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
    FuncPrototypeGen prototypeGen(_reprGen.get(), &_output);
    std::size_t statusesNum = 0;
    for (const ComponentAndMsg& msg : package->statusMsgs()) {
        _output.appendModIfdef(msg.component->moduleName());
        _output.appendIndent();
        _output.append("{");
        _output.append(".func = ");
        prototypeGen.appendStatusMessageGenFuncName(msg.component.get(), statusesNum);
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
        statusesNum++;
    }
    _output.append("};\n\n");

    _output.append("#define _PHOTON_TM_MSG_COUNT sizeof(_messageDesc) / sizeof(_messageDesc[0])\n\n");

    std::string tmDetailPath = _onboardPath + "/photon/Tm.Private.inc.c"; //FIXME: joinPath
    TRY(saveOutput(tmDetailPath.c_str(), _output.view(), _diag.get()));
    _output.clear();

    return true;
}

void Generator::generateSerializedPackage(const Project* project, SrcBuilder* dest)
{
    dest->clear();

    bmcl::Buffer encoded = project->encode();

    dest->appendNumericValueDefine(encoded.size(), "_PHOTON_PACKAGE_SIZE");
    dest->appendEol();
    dest->appendByteArrayDefinition("static const", "_package", encoded);
    dest->appendEol();

    Project::HashType ctx;
    ctx.update(encoded);
    auto hash = ctx.finalize();

    dest->appendNumericValueDefine(hash.size(), "_PHOTON_PACKAGE_HASH_SIZE");
    dest->appendEol();
    dest->appendByteArrayDefinition("static const", "_packageHash", hash);
    dest->appendEol();

    for (const Device* dev : project->devices()) {
        dest->appendDeviceIfDef(dev->name());
        dest->appendEol();
        bmcl::Bytes name = bmcl::StringView(dev->name()).asBytes();
        dest->appendNumericValueDefine(name.size(), "_PHOTON_DEVICE_NAME_SIZE");
        dest->appendEol();
        dest->appendByteArrayDefinition("static const", "_deviceName", name);
        dest->appendEol();
        dest->appendEndif();
        dest->appendEol();
    }

}

void Generator::appendBuiltinSources(bmcl::StringView ext)
{
    std::initializer_list<bmcl::StringView> builtin = {"CmdDecoder.Private", "CmdEncoder.Private",
                                                       "StatusEncoder.Private", "StatusDecoder.Private"};
    for (bmcl::StringView str : builtin) {
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
    for (const Device* dev : project->devices()) {
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
        for (const Device* dep : dev->cmdTargets()) {
            appendTargetMods(dep);
        }
        if (dev->hasSelfCmdTarget()) {
            appendTargetMods(dev);
        }


        auto appendSourceMods = [&](const Device* dep) {
            for (const Ast* module : dep->modules()) {
                sourceMods.emplace(module);
                if (module->component().isNone()) {
                    continue;
                }
                coll.collectParams(module->component()->paramsRange(), &types);
            }
        };
        for (const Device* dep : dev->tmSources()) {
            appendSourceMods(dep);
        }
        if (dev->hasSelfTmSource()) {
            appendSourceMods(dev);
        }

        //header
        if (dev == project->master()) {
            _output.append("#define PHOTON_IS_MASTER\n\n");
        }
        _output.append("#define PHOTON_DEVICE_");
        _output.appendUpper(dev->name());
        _output.append("\n\n");

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
        for (const Device* dep : dev->cmdTargets()) {
            appendDevTarget(dep);
        }
        if (dev->hasSelfCmdTarget()) {
            appendDevTarget(dev);
        }
        auto appendDevSource = [this](const Device* dep) {
            _output.append("#define PHOTON_HAS_DEVICE_SOURCE_");
            _output.appendUpper(dep->name());
            _output.appendEol();
        };
        for (const Device* dep : dev->tmSources()) {
            appendDevSource(dep);
        }
        if (dev->hasSelfTmSource()) {
            appendDevSource(dev);
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

        appendBuiltinSources(".h");
        _output.appendEol();

        appendBundledSources(dev, ".h");

        SrcBuilder path;
        path.append(_onboardPath);
        path.append("/Photon"); //FIXME: joinPath
        path.appendWithFirstUpper(dev->name());
        path.append(".h");
        TRY(saveOutput(path.result().c_str(), _output.view(), _diag.get()));
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

        appendBuiltinSources(".c");
        _output.appendEol();

        appendBundledSources(dev, ".c");

        path.result().back() = 'c';
        TRY(saveOutput(path.result().c_str(), _output.view(), _diag.get()));
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
    _onboardPath = joinPath(_savePath, "onboard");
    TRY(makeDirectory(_onboardPath.c_str(), _diag.get()));
    _gcPath = joinPath(_savePath, "groundcontrol");
    TRY(makeDirectory(_gcPath.c_str(), _diag.get()));
    _onboardPhotonPath.result() = joinPath(_onboardPath, "photon");
    TRY(makeDirectory(_onboardPhotonPath.result().c_str(), _diag.get()));
    _gcPhotonPath.result() = joinPath(_gcPath, "photon");
    TRY(makeDirectory(_gcPhotonPath.result().c_str(), _diag.get()));

    SrcBuilder dest;
    dest.result().reserve(32 * 1024);
    _output.result().reserve(32 * 1024);
    auto future = std::async(std::launch::async, &Generator::generateSerializedPackage, project, &dest);

    _onboardPhotonPath.append('/'); //FIXME: remove
    _gcPhotonPath.append('/'); //FIXME: remove

    const Package* package = project->package();

    _reprGen = new TypeReprGen(&_output);
    _onboardHgen.reset(new OnboardTypeHeaderGen(_reprGen.get(), &_output));
    _onboardSgen.reset(new OnboardTypeSourceGen(_reprGen.get(), &_output));
    for (const Ast* it : package->modules()) {
        if (!generateTypesAndComponents(it)) {
            return false;
        }
    }

    TRY(generateGenerics(package));
    TRY(generateConfig(project));
    TRY(generateDynArrays());
    TRY(generateTmPrivate(package));
    TRY(generateStatusMessages(project));
    TRY(generateCommands(package));
    TRY(generateDeviceFiles(project));

    GcInterfaceGen igen(&_output);
    igen.generateHeader(package);
    TRY(dumpIfNotEmpty("Interface", ".hpp", &_gcPhotonPath));

    future.wait();
    std::string packageDetailPath = _onboardPath + "/photon/Package.Private.inc.c"; //FIXME: joinPath
    TRY(saveOutput(packageDetailPath.c_str(), dest.view(), _diag.get()));

    _output.resize(0);
    _onboardHgen.reset();
    _onboardSgen.reset();
    _reprGen.reset();
    _dynArrays.clear();
    _onboardPhotonPath.resize(0);
    _onboardPath.clear();
    _gcPath.clear();
    return true;
}

#define GEN_PREFIX ".gen"

bool Generator::generateDynArrays()
{
    _onboardPhotonPath.append("_dynarray_");
    TRY(makeDirectory(_onboardPhotonPath.result().c_str(), _diag.get()));
    _onboardPhotonPath.append('/');

    for (const auto& it : _dynArrays) {
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
    StatusEncoderGen gen(_reprGen.get(), &_output);
    gen.generateEncoderHeader(project);
    TRY(dumpIfNotEmpty("StatusEncoder.Private", ".h", &_onboardPhotonPath));

    gen.generateEncoderSource(project);
    TRY(dumpIfNotEmpty("StatusEncoder.Private", ".c", &_onboardPhotonPath));

    gen.generateDecoderHeader(project);
    TRY(dumpIfNotEmpty("StatusDecoder.Private", ".h", &_onboardPhotonPath));

    gen.generateDecoderSource(project);
    TRY(dumpIfNotEmpty("StatusDecoder.Private", ".c", &_onboardPhotonPath));

    std::size_t pathSize = _gcPhotonPath.result().size();
    _gcPhotonPath.append("_msgs_");
    TRY(makeDirectory(_gcPhotonPath.result().c_str(), _diag.get()));
    _gcPhotonPath.append('/');

    GcStatusMsgGen msgGen(&_output);
    SrcBuilder msgName;
    for (const Component* comp : project->package()->components()) {
        for (const StatusMsg* msg : comp->statusesRange()) {
            msgName.appendWithFirstUpper(comp->name());
            msgName.append("Msg");
            msgName.appendWithFirstUpper(msg->name());
            msgGen.generateHeader(comp, msg);
            TRY(dumpIfNotEmpty(msgName.view(), ".hpp", &_gcPhotonPath));
            msgName.clear();
        }
    }
    _gcPhotonPath.resize(pathSize);

    return true;
}

bool Generator::generateCommands(const Package* package)
{
    CmdDecoderGen decGen(_reprGen.get(), &_output);
    decGen.generateHeader(package->components());
    TRY(dumpIfNotEmpty("CmdDecoder.Private", ".h", &_onboardPhotonPath));

    decGen.generateSource(package->components());
    TRY(dumpIfNotEmpty("CmdDecoder.Private", ".c", &_onboardPhotonPath));

    CmdEncoderGen encGen(_reprGen.get(), &_output);
    encGen.generateHeader(package->components());
    TRY(dumpIfNotEmpty("CmdEncoder.Private", ".h", &_onboardPhotonPath));

    encGen.generateSource(package->components());
    TRY(dumpIfNotEmpty("CmdEncoder.Private", ".c", &_onboardPhotonPath));

    return true;
}

bool Generator::dumpIfNotEmpty(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath)
{
    if (!_output.result().empty()) {
        TRY(dump(name, ext, currentPath));
    }
    return true;
}

bool Generator::dump(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath)
{
    currentPath->appendWithFirstUpper(name);
    currentPath->append(ext);
    TRY(saveOutput(currentPath->result().c_str(), _output.view(), _diag.get()));
    currentPath->removeFromBack(name.size() + ext.size());
    _output.clear();
    return true;
}

bool Generator::generateGenerics(const Package* package)
{
    _onboardPhotonPath.append("_generic_");
    TRY(makeDirectory(_onboardPhotonPath.result().c_str(), _diag.get()));
    _onboardPhotonPath.append('/');

    _gcPhotonPath.append("_generic_");
    TRY(makeDirectory(_gcPhotonPath.result().c_str(), _diag.get()));
    _gcPhotonPath.append('/');

    SrcBuilder typeNameBuilder;
    TypeNameGen typeNameGen(&typeNameBuilder);
    GcTypeGen gcTypeGen(&_output);
    for (const Ast* ast : package->modules()) {
        for (const GenericInstantiationType* type : ast->genericInstantiationsRange()) {
            typeNameGen.genTypeName(type);

            _onboardHgen->genTypeHeader(ast, type, typeNameBuilder.result());
            TRY(dump(typeNameBuilder.result(), ".h", &_onboardPhotonPath));

            _onboardSgen->genTypeSource(type, typeNameBuilder.result());
            TRY(dump(typeNameBuilder.result(), GEN_PREFIX ".c", &_onboardPhotonPath));

            gcTypeGen.generateHeader(type);
            TRY(dump(typeNameBuilder.result(), ".hpp", &_gcPhotonPath));

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
    TRY(makeDirectory(_onboardPhotonPath.result().c_str(), _diag.get()));
    _onboardPhotonPath.append('/'); //FIXME: joinPath

    _gcPhotonPath.append(ast->moduleName());
    TRY(makeDirectory(_gcPhotonPath.result().c_str(), _diag.get()));
    _gcPhotonPath.append('/'); //FIXME: joinPath

    DynArrayCollector coll;
    SrcBuilder typeNameBuilder;
    TypeNameGen typeNameGen(&typeNameBuilder);
    GcTypeGen gcTypeGen(&_output);
    for (const NamedType* type : ast->namedTypesRange()) {
        coll.collectUniqueDynArrays(type, &_dynArrays);
        if (type->typeKind() == TypeKind::Imported) {
            continue;
        }
        if (type->typeKind() != TypeKind::Generic) {
            typeNameGen.genTypeName(type);

            _onboardHgen->genTypeHeader(ast, type, typeNameBuilder.result());
            TRY(dump(type->name(), ".h", &_onboardPhotonPath));

            _onboardSgen->genTypeSource(type, typeNameBuilder.result());
            TRY(dump(type->name(), GEN_PREFIX ".c", &_onboardPhotonPath));

            typeNameBuilder.clear();
        }
        gcTypeGen.generateHeader(type);
        TRY(dump(type->name(), ".hpp", &_gcPhotonPath));

    }

    if (ast->component().isSome()) {
        bmcl::OptionPtr<const Component> comp = ast->component();
        coll.collectUniqueDynArrays(comp.unwrap(), &_dynArrays);

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
