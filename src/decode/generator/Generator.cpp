/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/Generator.h"
#include "decode/generator/HeaderGen.h"
#include "decode/generator/SourceGen.h"
#include "decode/generator/DynArrayCollector.h"
#include "decode/generator/StatusEncoderGen.h"
#include "decode/generator/CmdDecoderGen.h"
#include "decode/generator/CmdEncoderGen.h"
#include "decode/generator/TypeNameGen.h"
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
#include "decode/core/HashMap.h"
#include "decode/core/HashSet.h"

#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>
#include <bmcl/Sha3.h>
#include <bmcl/FixedArrayView.h>

#include <iostream>
#include <deque>
#include <memory>

#if defined(__linux__)
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
#elif defined(_MSC_VER) || defined(__MINGW32__)
# include <windows.h>
#else
# error "Unsupported OS"
#endif

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
    _savePath = path.toStdString();
}

static bool makeDirectory(const char* path, Diagnostics* diag)
{
#if defined(__linux__)
    int rv = mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (rv == -1) {
        int rn = errno;
        if (rn == EEXIST) {
            return true;
        }
        diag->buildSystemFileErrorReport("failed to create directory", rn, path);
        return false;
    }
#elif defined(_MSC_VER) || defined(__MINGW32__)
    bool isOk = CreateDirectory(path, NULL);
    if (!isOk) {
        auto rn = GetLastError();
        if (rn != ERROR_ALREADY_EXISTS) {
            diag->buildSystemFileErrorReport("failed to create directory", rn, path);
            return false;
        }
    }
#endif
    return true;
}

static bool saveOutput(const char* path, SrcBuilder* output, Diagnostics* diag)
{
#if defined(__linux__)
    int fd;
    while (true) {
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd == -1) {
            int rn = errno;
            if (rn == EINTR) {
                continue;
            }
            diag->buildSystemFileErrorReport("failed to create file", rn, path);
            return false;
        }
        break;
    }

    std::size_t size = output->result().size();
    std::size_t total = 0;
    while(total < size) {
        ssize_t written = write(fd, output->result().c_str() + total, size - total);
        if (written == -1) {
            int rn = errno;
            if (rn == EINTR) {
                continue;
            }
            diag->buildSystemFileErrorReport("failed to write file", rn, path);
            close(fd);
            return false;
        }
        total += written;
    }

    close(fd);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    HANDLE handle = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        diag->buildSystemFileErrorReport("failed to create file", GetLastError(), path);
        return false;
    }
    DWORD bytesWritten;
    bool isOk = WriteFile(handle, output->result().c_str(), output->result().size(), &bytesWritten, NULL);
    if (!isOk) {
        diag->buildSystemFileErrorReport("failed to write file", GetLastError(), path);
        return false;
    }
    assert(output->result().size() == bytesWritten);
    CloseHandle(handle);
#endif
    return true;
}

static bool copyFile(const char* from, const char* to, Diagnostics* diag)
{
#if defined(__linux__)
    int fdFrom;
    while (true) {
        fdFrom = open(from, O_RDONLY);
        if (fdFrom == -1) {
            int rn = errno;
            if (rn == EINTR) {
                continue;
            }
            diag->buildSystemFileErrorReport("failed to open file", rn, from);
            return false;
        }
        break;
    }
    int fdTo;
    while (true) {
        fdTo = open(to, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fdTo == -1) {
            int rn = errno;
            if (rn == EINTR) {
                continue;
            }
            diag->buildSystemFileErrorReport("failed to open file", rn, to);
            return false;
        }
        break;
    }

    bool isOk = true;
    while (true) {
        char temp[4096];

        ssize_t size;
        while(true) {
            size = read(fdFrom, temp, sizeof(temp));
            if (size == 0) {
                goto end;
            }
            if (size == -1) {
                int rn = errno;
                if (rn == EINTR) {
                    continue;
                }
                diag->buildSystemFileErrorReport("failed to read file", rn, from);
                isOk = false;
                goto end;
            }
            break;
        }

        ssize_t total = 0;
        while(total < size) {
            ssize_t written = write(fdTo, &temp[total], size);
            if (written == -1) {
                int rn = errno;
                if (rn == EINTR) {
                    continue;
                }
                diag->buildSystemFileErrorReport("failed to write file", rn, to);
                isOk = false;
                goto end;
            }
            total += written;
        }
    }

end:
    close(fdFrom);
    close(fdTo);
    return isOk;
#elif defined(_MSC_VER) || defined(__MINGW32__)
    if (!CopyFile(from, to, false)) {
        //TODO: extend error message
        diag->buildSystemFileErrorReport("failed to copy file", GetLastError(), from);
        return false;
    }
    return true;
#endif
}

bool Generator::generateTmPrivate(const Package* package)
{
    _output.clear();

    _output.append("static PhotonTmMessageDesc _messageDesc[] = {\n");
    FuncPrototypeGen prototypeGen(_reprGen.get(), &_output);
    std::size_t statusesNum = 0;
    for (const ComponentAndMsg& msg : package->statusMsgs()) {
        _output.appendModIfdef(msg.component->moduleName());
        _output.appendIndent(1);
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

    std::string tmDetailPath = _savePath + "/photon/Tm.Private.inc.c";
    TRY(saveOutput(tmDetailPath.c_str(), &_output, _diag.get()));
    _output.clear();

    return true;
}

bool Generator::generateSerializedPackage(const Project* project)
{
    _output.clear();

    bmcl::Buffer encoded = project->encode();

    _output.appendNumericValueDefine("_PHOTON_PACKAGE_SIZE", encoded.size());
    _output.appendEol();
    _output.appendByteArrayDefinition("static const", "_package", encoded);
    _output.appendEol();

    Project::HashType ctx;
    ctx.update(encoded);
    auto hash = ctx.finalize();

    _output.appendNumericValueDefine("_PHOTON_PACKAGE_HASH_SIZE", hash.size());
    _output.appendEol();
    _output.appendByteArrayDefinition("static const", "_packageHash", hash);
    _output.appendEol();

    for (const Device* dev : project->devices()) {
        _output.appendDeviceIfDef(dev->name);
        _output.appendEol();
        bmcl::Bytes name = bmcl::StringView(dev->name).asBytes();
        _output.appendNumericValueDefine("_PHOTON_DEVICE_NAME_SIZE", name.size());
        _output.appendEol();
        _output.appendByteArrayDefinition("static const", "_deviceName", name);
        _output.appendEol();
        _output.appendEndif();
        _output.appendEol();
    }

    std::string packageDetailPath = _savePath + "/photon/Package.Private.inc.c";
    TRY(saveOutput(packageDetailPath.c_str(), &_output, _diag.get()));
    _output.clear();

    return true;
}

void Generator::appendBuiltinSources(bmcl::StringView ext)
{
    std::initializer_list<bmcl::StringView> builtin = {"CmdDecoder.Private", "CmdEncoder.Private", "StatusEncoder.Private"};
    for (bmcl::StringView str : builtin) {
        _output.append("#include \"photon/");
        _output.append(str);
        _output.append(ext);
        _output.append("\"\n");
    }
}

bool Generator::generateDeviceFiles(const Project* project)
{
    HashMap<Rc<const Ast>, std::vector<std::string>> srcsPaths;
    for (const Ast* mod : project->package()->modules()) {
        auto src = project->sourcesForModule(mod);
        if (src.isNone()) {
            continue;
        }

        std::string dest = _savePath;
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

    auto appendBundledSources = [this, &srcsPaths](const Device* dev, bmcl::StringView ext) {
        for (const Rc<Ast>& module : dev->modules) {
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

    IncludeCollector coll;
    for (const Device* dev : project->devices()) {
        HashSet<std::string> types;
        types.insert("core/Reader");
        types.insert("core/Writer");
        types.insert("core/Error");

        for (const Rc<Ast>& module : dev->modules) {
            coll.collect(module.get(), &types);
        }
        HashSet<Rc<Ast>> targetMods;

        for (const Rc<Device>& dep : dev->cmdTargets) {
            for (const Rc<Ast>& module : dep->modules) {
                targetMods.emplace(module);
                if (module->component().isNone()) {
                    continue;
                }
                coll.collectCmds(module->component()->cmdsRange(), &types);
            }
        }

        for (const Rc<Device>& dep : dev->tmSources) {
            for (const Rc<Ast>& module : dep->modules) {
                if (module->component().isNone()) {
                    continue;
                }
                coll.collectParams(module->component()->paramsRange(), &types);
            }
        }

        //header
        if (dev == project->master()) {
            _output.append("#define PHOTON_IS_MASTER\n\n");
        }
        _output.append("#define PHOTON_DEVICE_");
        _output.appendUpper(dev->name);
        _output.append("\n\n");
        for (const Rc<Ast>& module : dev->modules) {
            _output.append("#define PHOTON_HAS_MODULE_");
            _output.appendUpper(module->moduleInfo()->moduleName());
            _output.appendEol();
        }
        for (const Rc<Ast>& module : targetMods) {
            _output.append("#define PHOTON_HAS_CMD_TARGET_");
            _output.appendUpper(module->moduleInfo()->moduleName());
            _output.appendEol();
        }
        _output.appendEol();

        _output.append("#include \"photon/Config.h\"\n\n");

        for (const std::string& inc : types) {
            _output.appendTypeInclude(inc, ".h");
        }
        _output.appendEol();

        for (const Rc<Ast>& module : dev->modules) {
            if (module->component().isSome()) {
                _output.appendComponentInclude(module->moduleInfo()->moduleName(), ".h");
            }
        }
        _output.appendEol();

        appendBuiltinSources(".h");
        _output.appendEol();

        appendBundledSources(dev, ".h");

        SrcBuilder path;
        path.append(_savePath);
        path.append("/Photon");
        path.appendWithFirstUpper(dev->name);
        path.appendWithFirstUpper(".h");
        TRY(saveOutput(path.result().c_str(), &_output, _diag.get()));
        _output.clear();

        //src
        _output.append("#include \"Photon");
        _output.appendWithFirstUpper(dev->name);
        _output.append(".h\"\n\n");
        for (const std::string& inc : types) {
            _output.appendTypeInclude(inc, ".gen.c");
        }
        _output.appendEol();

        for (const Rc<Ast>& module : dev->modules) {
            if (module->component().isSome()) {
                _output.appendComponentInclude(module->moduleInfo()->moduleName(), ".c");
            }
        }
        _output.appendEol();

        appendBuiltinSources(".c");
        _output.appendEol();

        appendBundledSources(dev, ".c");

        path.result().back() = 'c';
        TRY(saveOutput(path.result().c_str(), &_output, _diag.get()));
        _output.clear();
    }
    return true;
}

bool Generator::generateConfig(const Project* project)
{
    _photonPath.append('/');

    _output.append("#include \"photon/core/Config.h\"");

    TRY(dump("Config", ".h", &_photonPath));
    _output.clear();

    _photonPath.removeFromBack(1);
    return true;
}

bool Generator::generateProject(const Project* project)
{
    _photonPath.append(_savePath);
    _photonPath.append("/photon");
    TRY(makeDirectory(_photonPath.result().c_str(), _diag.get()));
    _photonPath.append('/');
    const Package* package = project->package();

    _reprGen = new TypeReprGen(&_output);
    _hgen.reset(new HeaderGen(_reprGen.get(), &_output));
    _sgen.reset(new SourceGen(_reprGen.get(), &_output));
    for (const Ast* it : package->modules()) {
        if (!generateTypesAndComponents(it)) {
            return false;
        }
    }

    TRY(generateGenerics(package));
    TRY(generateConfig(project));
    TRY(generateDynArrays());
    TRY(generateTmPrivate(package));
    TRY(generateSerializedPackage(project));
    TRY(generateStatusMessages(package));
    TRY(generateCommands(package));
    TRY(generateDeviceFiles(project));

    _output.resize(0);
    _hgen.reset();
    _sgen.reset();
    _reprGen.reset();
    _dynArrays.clear();
    _photonPath.resize(0);
    return true;
}

#define GEN_PREFIX ".gen"

bool Generator::generateDynArrays()
{
    _photonPath.append("_dynarray_");
    TRY(makeDirectory(_photonPath.result().c_str(), _diag.get()));
    _photonPath.append('/');

    for (const auto& it : _dynArrays) {
        _hgen->genDynArrayHeader(it.second.get());
        TRY(dumpIfNotEmpty(it.first, ".h", &_photonPath));
        _output.clear();

        _sgen->genTypeSource(it.second.get());
        TRY(dumpIfNotEmpty(it.first, GEN_PREFIX ".c", &_photonPath));
        _output.clear();
    }

    _photonPath.removeFromBack(std::strlen("_dynarray_/"));
    return true;
}

bool Generator::generateStatusMessages(const Package* package)
{
    StatusEncoderGen gen(_reprGen.get(), &_output);
    gen.generateHeader(package->statusMsgs());
    TRY(dumpIfNotEmpty("StatusEncoder.Private", ".h", &_photonPath));
    _output.clear();

    gen.generateSource(package->statusMsgs());
    TRY(dumpIfNotEmpty("StatusEncoder.Private", ".c", &_photonPath));
    _output.clear();

    return true;
}

bool Generator::generateCommands(const Package* package)
{
    CmdDecoderGen decGen(_reprGen.get(), &_output);
    decGen.generateHeader(package->components());
    TRY(dumpIfNotEmpty("CmdDecoder.Private", ".h", &_photonPath));
    _output.clear();

    decGen.generateSource(package->components());
    TRY(dumpIfNotEmpty("CmdDecoder.Private", ".c", &_photonPath));
    _output.clear();

    CmdEncoderGen encGen(_reprGen.get(), &_output);
    encGen.generateHeader(package->components());
    TRY(dumpIfNotEmpty("CmdEncoder.Private", ".h", &_photonPath));
    _output.clear();

    encGen.generateSource(package->components());
    TRY(dumpIfNotEmpty("CmdEncoder.Private", ".c", &_photonPath));
    _output.clear();

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
    TRY(saveOutput(currentPath->result().c_str(), &_output, _diag.get()));
    currentPath->removeFromBack(name.size() + ext.size());
    return true;
}

bool Generator::generateGenerics(const Package* package)
{
    _photonPath.append("_generic_");
    TRY(makeDirectory(_photonPath.result().c_str(), _diag.get()));
    _photonPath.append('/');
    SrcBuilder typeNameBuilder;
    TypeNameGen typeNameGen(&typeNameBuilder);
    for (const Ast* ast : package->modules()) {
        for (const GenericInstantiationType* type : ast->genericInstantiationsRange()) {
            typeNameGen.genTypeName(type);

            _hgen->genTypeHeader(ast, type, typeNameBuilder.result());
            TRY(dump(typeNameBuilder.result(), ".h", &_photonPath));
            _output.clear();

            _sgen->genTypeSource(type, typeNameBuilder.result());
            TRY(dump(typeNameBuilder.result(), GEN_PREFIX ".c", &_photonPath));

            _output.clear();
            typeNameBuilder.clear();
        }
    }
    _photonPath.removeFromBack(10);
    return true;
}

bool Generator::generateTypesAndComponents(const Ast* ast)
{
    _photonPath.append(ast->moduleInfo()->moduleName());
    TRY(makeDirectory(_photonPath.result().c_str(), _diag.get()));
    _photonPath.append('/');

    DynArrayCollector coll;
    SrcBuilder typeNameBuilder;
    TypeNameGen typeNameGen(&typeNameBuilder);
    for (const NamedType* type : ast->namedTypesRange()) {
        coll.collectUniqueDynArrays(type, &_dynArrays);
        if (type->typeKind() == TypeKind::Imported) {
            continue;
        }
        if (type->typeKind() == TypeKind::Generic) {
            continue;
        }
        typeNameGen.genTypeName(type);

        _hgen->genTypeHeader(ast, type, typeNameBuilder.result());
        TRY(dump(type->name(), ".h", &_photonPath));
        _output.clear();

        _sgen->genTypeSource(type, typeNameBuilder.result());
        TRY(dump(type->name(), GEN_PREFIX ".c", &_photonPath));

        _output.clear();
        typeNameBuilder.clear();
    }

    if (ast->component().isSome()) {
        bmcl::OptionPtr<const Component> comp = ast->component();
        coll.collectUniqueDynArrays(comp.unwrap(), &_dynArrays);

        _hgen->genComponentHeader(ast, comp.unwrap());
        TRY(dumpIfNotEmpty(comp->moduleName(), ".Component.h", &_photonPath));
        _output.clear();

        _output.append("#include \"photon/");
        _output.append(comp->moduleName());
        _output.append("/");
        _output.appendWithFirstUpper(comp->moduleName());
        _output.append(".Component.h\"\n\n");
        if (comp->hasParams()) {
            _output.append("Photon");
            _output.appendWithFirstUpper(comp->moduleName());
            _output.append(" _photon");
            _output.appendWithFirstUpper(comp->moduleName());
            _output.append(';');
        }
        TRY(dumpIfNotEmpty(comp->moduleName(), ".Component.c", &_photonPath));
        _output.clear();

        //sgen->genTypeSource(type);
        //TRY(dump(type->name(), GEN_PREFIX ".c", &photonPath));
        _output.clear();
    }

    if (ast->hasConstants()) {
        _hgen->startIncludeGuard(ast->moduleInfo()->moduleName(), "CONSTANTS");
        for (const Constant* c : ast->constantsRange()) {
            _output.append("#define PHOTON_");
            _output.appendUpper(ast->moduleInfo()->moduleName());
            _output.append('_');
            _output.append(c->name());
            _output.append(" ");
            _output.appendNumericValue(c->value());
            _output.appendEol();
        }
        _output.appendEol();
        _hgen->endIncludeGuard();
        TRY(dumpIfNotEmpty(ast->moduleInfo()->moduleName(), ".Constants.h", &_photonPath));
        _output.clear();
    }

    _photonPath.removeFromBack(ast->moduleInfo()->moduleName().size() + 1);
    return true;
}
}
