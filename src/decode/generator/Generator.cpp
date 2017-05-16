#include "decode/generator/Generator.h"
#include "decode/generator/HeaderGen.h"
#include "decode/generator/SourceGen.h"
#include "decode/generator/SliceCollector.h"
#include "decode/generator/StatusEncoderGen.h"
#include "decode/generator/CmdDecoderGen.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Package.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Component.h"
#include "decode/parser/Constant.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/Try.h"

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

//TODO: refact

namespace decode {

Generator::Generator(Diagnostics* diag)
    : _diag(diag)
{
}

void Generator::setOutPath(bmcl::StringView path)
{
    _savePath = path.toStdString();
    _savePath.push_back('/');
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
        ssize_t written = write(fd, output->result().c_str() + total, size);
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

bool Generator::generateTmPrivate(const Package* package)
{
    _output.clear();

    _output.append("#define _PHOTON_TM_MSG_COUNT ");
    _output.appendNumericValue(package->statusMsgs().size());
    _output.append("\n\n");

    _output.append("static PhotonTmMessageDesc _messageDesc[");
    _output.appendNumericValue(package->statusMsgs().size());
    _output.append("] = {\n");
    std::size_t statusesNum = 0;
    for (const ComponentAndMsg& msg : package->statusMsgs()) {
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
        statusesNum++;
    }
    _output.append("};\n");

    std::string tmDetailPath = _savePath + "/photon/Tm.Private.inc.c";
    TRY(saveOutput(tmDetailPath.c_str(), &_output));
    _output.clear();

    return true;
}

bool Generator::generateSerializedPackage(const Package* package)
{
    _output.clear();

    bmcl::Buffer encoded = package->encode();

    _output.appendNumericValueDefine("_PHOTON_PACKAGE_SIZE", encoded.size());
    _output.appendEol();
    _output.appendByteArrayDefinition("_package", encoded);
    _output.appendEol();

    bmcl::Sha3<512> ctx;
    ctx.update(encoded);
    auto hash = ctx.finalize();

    _output.appendNumericValueDefine("_PHOTON_PACKAGE_HASH_SIZE", hash.size());
    _output.appendEol();
    _output.appendByteArrayDefinition("_packageHash", hash);
    _output.appendEol();

    std::string packageDetailPath = _savePath + "/photon/Package.Private.inc.c";
    TRY(saveOutput(packageDetailPath.c_str(), &_output));
    _output.clear();

    return true;
}

bool Generator::generateFromPackage(const Package* package)
{
    _photonPath.append(_savePath);
    _photonPath.append("/photon");
    TRY(makeDirectory(_photonPath.result().c_str()));
    _photonPath.append('/');

    _reprGen = new TypeReprGen(&_output);
    _hgen.reset(new HeaderGen(_reprGen.get(), &_output));
    _sgen.reset(new SourceGen(_reprGen.get(), &_output));
    for (const Ast* it : package->modules()) {
        if (!generateTypesAndComponents(it)) {
            return false;
        }
    }

    TRY(generateSlices());

    TRY(generateTmPrivate(package));
    TRY(generateSerializedPackage(package));
    TRY(generateStatusMessages(package));
    TRY(generateCommands(package));

    std::string mainPath = _savePath + "/photon/Photon.c";
    TRY(saveOutput(mainPath.c_str(), &_main));


    _main.resize(0);
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

    for (auto it : _slices) {
        _hgen->genSliceHeader(it.second.get());
        TRY(dump(it.first, ".h", &_photonPath));
        _output.clear();

        _sgen->genTypeSource(it.second.get());
        TRY(dump(it.first, GEN_PREFIX ".c", &_photonPath));

        if (!_output.result().empty()) {
            _main.append("#include \"photon/_slices_/");
            _main.appendWithFirstUpper(it.first);
            _main.append(GEN_PREFIX ".c\"\n");
        }
        _output.clear();
    }

    _photonPath.removeFromBack(std::strlen("_slices_/"));
    return true;
}

bool Generator::generateStatusMessages(const Package* package)
{
    StatusEncoderGen gen(_reprGen.get(), &_output);
    gen.generateHeader(package->statusMsgs());
    TRY(dump("StatusEncoder.Private", ".h", &_photonPath));
    _output.clear();

    gen.generateSource(package->statusMsgs());
    TRY(dump("StatusEncoder.Private", ".c", &_photonPath));
    _output.clear();

    _main.append("#include \"photon/StatusEncoder.Private.c\"\n");
    return true;
}

bool Generator::generateCommands(const Package* package)
{
    CmdDecoderGen gen(_reprGen.get(), &_output);
    gen.generateHeader(package->components());
    TRY(dump("CmdDecoder.Private", ".h", &_photonPath));
    _output.clear();

    gen.generateSource(package->components());
    TRY(dump("CmdDecoder.Private", ".c", &_photonPath));
    _output.clear();

    _main.append("#include \"photon/CmdDecoder.Private.c\"\n");
    return true;
}

bool Generator::dump(bmcl::StringView name, bmcl::StringView ext, StringBuilder* currentPath)
{
    if (!_output.result().empty()) {
        currentPath->appendWithFirstUpper(name);
        currentPath->append(ext);
        TRY(saveOutput(currentPath->result().c_str(), &_output));
        currentPath->removeFromBack(name.size() + ext.size());
    }
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

        if (!_output.result().empty()) {
            _main.append("#include \"photon/");
            _main.append(type->moduleName());
            _main.append("/");
            _main.appendWithFirstUpper(type->name());
            _main.append(GEN_PREFIX ".c\"\n");
        }

        _output.clear();
    }

    if (ast->component().isSome()) {
        bmcl::OptionPtr<const Component> comp = ast->component();
        coll.collectUniqueSlices(comp.unwrap(), &_slices);

        _hgen->genComponentHeader(_currentAst.get(), comp.unwrap());
        TRY(dump(comp->moduleName(), ".Component.h", &_photonPath));
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
        TRY(dump(comp->moduleName(), ".Component.c", &_photonPath));
        _main.append("#include \"photon/");
        _main.append(comp->moduleName());
        _main.append("/");
        _main.appendWithFirstUpper(comp->moduleName());
        _main.append(".Component.c\"\n");
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
        TRY(dump(ast->moduleInfo()->moduleName(), ".Constants.h", &_photonPath));
        _output.clear();
    }

    _photonPath.removeFromBack(_currentAst->moduleInfo()->moduleName().size() + 1);

    _currentAst = nullptr;
    return true;
}
}
