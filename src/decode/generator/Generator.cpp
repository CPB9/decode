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
#include "decode/generator/SliceCollector.h"
#include "decode/generator/StatusEncoderGen.h"
#include "decode/generator/CmdDecoderGen.h"
#include "decode/generator/CmdEncoderGen.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Package.h"
#include "decode/parser/Project.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Component.h"
#include "decode/parser/Constant.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/Try.h"
#include "decode/core/PathUtils.h"

#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>
#include <bmcl/Sha3.h>
#include <bmcl/FixedArrayView.h>

#include <unordered_set>
#include <iostream>
#include <deque>
#include <memory>

#if defined(__linux__)
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
#elif defined(_MSC_VER) || defined(__MINGW32__)
# include <windows.h>
#endif

//TODO: use joinPath

namespace decode {

Generator::Generator(Diagnostics* diag)
    : _diag(diag)
{
}

void Generator::setOutPath(bmcl::StringView path)
{
    _savePath = path.toStdString();
}

bool Generator::makeDirectory(const char* path)
{
#if defined(__linux__)
    int rv = mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (rv == -1) {
        int rn = errno;
        if (rn == EEXIST) {
            return true;
        }
        //TODO: report error
        BMCL_CRITICAL() << "unable to create dir: " << path;
        return false;
    }
#elif defined(_MSC_VER) || defined(__MINGW32__)
    bool isOk = CreateDirectory(path, NULL);
    if (!isOk) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            BMCL_CRITICAL() << "error creating dir";
            return false;
        }
    }
#endif
    return true;
}

bool Generator::saveOutput(const char* path, SrcBuilder* output)
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
            //TODO: handle error
            BMCL_CRITICAL() << "unable to open file: " << path;
            return false;
        }
        break;
    }

    std::size_t size = output->result().size();
    std::size_t total = 0;
    while(total < size) {
        ssize_t written = write(fd, output->result().c_str() + total, size - total);
        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            BMCL_CRITICAL() << "unable to write file: " << path;
            //TODO: handle error
            close(fd);
            return false;
        }
        total += written;
    }

    close(fd);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    HANDLE handle = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        BMCL_CRITICAL() << "error creating file";
        //TODO: report error
        return false;
    }
    DWORD bytesWritten;
    bool isOk = WriteFile(handle, output->result().c_str(), output->result().size(), &bytesWritten, NULL);
    if (!isOk) {
        BMCL_CRITICAL() << "error writing file";
        //TODO: report error
        return false;
    }
    assert(output->result().size() == bytesWritten);
#endif
    return true;
}

static bool copyFile(const char* from, const char* to)
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
            //TODO: handle error
            BMCL_CRITICAL() << "unable to open file for reading: " << from;
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
            //TODO: handle error
            BMCL_CRITICAL() << "unable to open file for writing: " << to;
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
                if (errno == EINTR) {
                    continue;
                }
                BMCL_CRITICAL() << "unable to read file: " << to;
                //TODO: handle error
                isOk = false;
                goto end;
            }
            break;
        }

        std::size_t total = 0;
        while(total < size) {
            ssize_t written = write(fdTo, &temp[total], size);
            if (written == -1) {
                if (errno == EINTR) {
                    continue;
                }
                BMCL_CRITICAL() << "unable to write file: " << to;
                //TODO: handle error
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
        BMCL_CRITICAL() << "failed to copy file " << from << " to " << to;
        return false;
    }
    return true;
#endif
}

bool Generator::generateTmPrivate(const Package* package)
{
    _output.clear();

    _output.append("static PhotonTmMessageDesc _messageDesc[] = {\n");
    std::size_t statusesNum = 0;
    for (const ComponentAndMsg& msg : package->statusMsgs()) {
        _output.appendModIfdef(msg.component->moduleName());
        _output.appendIndent(1);
        _output.append("{");
        _output.append(".func = ");
        _hgen->appendStatusMessageGenFuncName(msg.component.get(), statusesNum);
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
    TRY(saveOutput(tmDetailPath.c_str(), &_output));
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

    bmcl::Sha3<512> ctx;
    ctx.update(encoded);
    auto hash = ctx.finalize();

    _output.appendNumericValueDefine("_PHOTON_PACKAGE_HASH_SIZE", hash.size());
    _output.appendEol();
    _output.appendByteArrayDefinition("static const", "_packageHash", hash);
    _output.appendEol();

    std::string packageDetailPath = _savePath + "/photon/Package.Private.inc.c";
    TRY(saveOutput(packageDetailPath.c_str(), &_output));
    _output.clear();

    return true;
}

void Generator::appendBuiltinSources(bmcl::StringView ext)
{
    std::initializer_list<bmcl::StringView> builtin = {"CmdDecoder.Private", "CmdEncoder.Private", "StatusEncoder.Private", "Init"};
    for (bmcl::StringView str : builtin) {
        _output.append("#include \"photon/");
        _output.append(str);
        _output.append(ext);
        _output.append("\"\n");
    }
}

bool Generator::generateDeviceFiles(const Project* project)
{
    std::unordered_map<Rc<const Ast>, std::vector<std::string>> srcsPaths;
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
            TRY(copyFile(file.c_str(), dest.c_str()));
            dest.resize(destSize);
            paths.push_back(joinPath(src->relativeDest, fname));
        }
        srcsPaths.emplace(mod, std::move(paths));
    }

    auto appendBundledSources = [this, &srcsPaths](const Project::Device* dev, bmcl::StringView ext) {
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
    for (const Project::Device* dev : project->devices()) {
        std::unordered_set<std::string> types;
        types.insert("core/Reader");
        types.insert("core/Writer");
        types.insert("core/Error");

        for (const Rc<Ast>& module : dev->modules) {
            coll.collect(module.get(), &types);
        }
        std::unordered_set<Rc<Ast>> targetMods;

        for (const Rc<Project::Device>& dep : dev->cmdTargets) {
            for (const Rc<Ast>& module : dep->modules) {
                targetMods.emplace(module);
                if (module->component().isNone()) {
                    continue;
                }
                coll.collectCmds(module->component()->cmdsRange(), &types);
            }
        }

        for (const Rc<Project::Device>& dep : dev->tmSources) {
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
        TRY(saveOutput(path.result().c_str(), &_output));
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
        TRY(saveOutput(path.result().c_str(), &_output));
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

bool Generator::generateInit(const Project* project)
{
    _output.append("#ifndef __PHOTON_INIT_H__\n");
    _output.append("#define __PHOTON_INIT_H__\n\n");
    _output.startCppGuard();
    _output.append("void Photon_Init();\n\n");
    _output.endCppGuard();
    _output.append("#endif\n");
    TRY(dumpIfNotEmpty("Init", ".h", &_photonPath));
    _output.clear();

    _output.append("#include \"photon/Init.h\"\n\n");
    _output.append("void Photon_Init()\n{\n");
    for (const Ast* module : project->package()->modules()) {
        if (module->component().isNone()) {
            continue;
        }
        bmcl::OptionPtr<const ImplBlock> impl = module->component().unwrap()->implBlock();
        if (impl.isNone()) {
            continue;
        }
        auto initFunc = std::find_if(impl->functionsBegin(), impl->functionsEnd(), [](const Function* f) {
            return f->name() == "init";
        });
        if (initFunc == impl->functionsEnd()) {
            continue;
        }
        if (initFunc->type()->hasArguments()) {
            BMCL_WARNING() << "init function from component <" << module->moduleInfo()->moduleName().toStdString() << " has arguments";
            continue;
        }
        bmcl::StringView modName = module->moduleInfo()->moduleName();
        _output.appendModIfdef(modName);
        _output.append("    Photon");
        _output.appendWithFirstUpper(modName);
        _output.append("_Init();\n");
        _output.appendEndif();
    }
    _output.append("}\n");
    TRY(dumpIfNotEmpty("Init", ".c", &_photonPath));
    _output.clear();

    return true;
}

bool Generator::generateProject(const Project* project)
{
    _photonPath.append(_savePath);
    _photonPath.append("/photon");
    TRY(makeDirectory(_photonPath.result().c_str()));
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

    TRY(generateConfig(project));
    TRY(generateInit(project));
    TRY(generateSlices());
    TRY(generateTmPrivate(package));
    TRY(generateSerializedPackage(project));
    TRY(generateStatusMessages(package));
    TRY(generateCommands(package));
    TRY(generateDeviceFiles(project));

    _output.resize(0);
    _hgen.reset();
    _sgen.reset();
    _reprGen.reset();
    _slices.clear();
    _photonPath.resize(0);
    return true;
}

#define GEN_PREFIX ".gen"

bool Generator::generateSlices()
{
    _photonPath.append("_slices_");
    TRY(makeDirectory(_photonPath.result().c_str()));
    _photonPath.append('/');

    for (const auto& it : _slices) {
        _hgen->genSliceHeader(it.second.get());
        TRY(dumpIfNotEmpty(it.first, ".h", &_photonPath));
        _output.clear();

        _sgen->genTypeSource(it.second.get());
        TRY(dumpIfNotEmpty(it.first, GEN_PREFIX ".c", &_photonPath));
        _output.clear();
    }

    _photonPath.removeFromBack(std::strlen("_slices_/"));
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
    TRY(saveOutput(currentPath->result().c_str(), &_output));
    currentPath->removeFromBack(name.size() + ext.size());
    return true;
}

bool Generator::generateTypesAndComponents(const Ast* ast)
{
    _currentAst = ast;
    if (ast->moduleInfo()->moduleName() != "core") {
        _output.setModName(ast->moduleInfo()->moduleName());
    } else {
        _output.setModName(bmcl::StringView::empty());
    }

    _photonPath.append(_currentAst->moduleInfo()->moduleName());
    TRY(makeDirectory(_photonPath.result().c_str()));
    _photonPath.append('/');

    SliceCollector coll;
    for (const NamedType* type : ast->typesRange()) {
        coll.collectUniqueSlices(type, &_slices);
        if (type->typeKind() == TypeKind::Imported) {
            continue;
        }

        _hgen->genTypeHeader(_currentAst.get(), type);
        TRY(dump(type->name(), ".h", &_photonPath));
        _output.clear();

        _sgen->genTypeSource(type);
        TRY(dump(type->name(), GEN_PREFIX ".c", &_photonPath));

        _output.clear();
    }

    if (ast->component().isSome()) {
        bmcl::OptionPtr<const Component> comp = ast->component();
        coll.collectUniqueSlices(comp.unwrap(), &_slices);

        _hgen->genComponentHeader(_currentAst.get(), comp.unwrap());
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

    _photonPath.removeFromBack(_currentAst->moduleInfo()->moduleName().size() + 1);

    _currentAst = nullptr;
    return true;
}
}
