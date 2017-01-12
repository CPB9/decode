#include "decode/generator/Generator.h"
#include "decode/generator/HeaderGen.h"
#include "decode/generator/SourceGen.h"
#include "decode/generator/SliceCollector.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Package.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Component.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/Try.h"

#include <bmcl/Logging.h>

#include <unordered_set>
#include <iostream>
#include <deque>
#include <memory>

#if defined(__linux__)
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
#elif defined(_MSC_VER)
# include <windows.h>
#endif

//TODO: refact

namespace decode {

Generator::Generator(const Rc<Diagnostics>& diag)
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
#elif defined(_MSC_VER)
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
#elif defined(_MSC_VER)
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

bool Generator::generateTmPrivate(const Rc<Package>& package)
{
    _output.clear();
    _output.append("static PhotonTmMessageDesc messageDesc[] = {\n");
    std::size_t statusesNum = 0;
    for (auto it : package->components()) {
        const bmcl::Option<Rc<Statuses>>& statuses =  it.second->statuses();
        if (statuses.isNone()) {
            continue;
        }
        for (auto jt : statuses.unwrap()->statusMap()) {
            statusesNum++;
            _output.appendIndent(1);
            _output.append("{");
            _output.append(".compNum = ");
            _output.appendNumericValue(it.first);
            _output.append(", .msgNum = ");
            _output.appendNumericValue(jt.first);
            _output.append(", .interest = ");
            _output.appendNumericValue(0);
            _output.append(", .priority = ");
            _output.appendNumericValue(0);
            _output.append(", .isAllowed = true");
            _output.append("},\n");
        }
    }
    _output.append("};\n\n");
    _output.append("#define _PHOTON_TM_MSG_COUNT ");
    _output.appendNumericValue(statusesNum);
    _output.append("\n");

    std::string tmDetailPath = _savePath + "/photon/TmPrivate.inc.c";
    TRY(saveOutput(tmDetailPath.c_str(), &_output));

    return true;
}

bool Generator::generateFromPackage(const Rc<Package>& package)
{
    _photonPath.append(_savePath);
    _photonPath.append("/photon");
    TRY(makeDirectory(_photonPath.result().c_str()));
    _photonPath.append('/');

    Rc<TypeReprGen> reprGen = new TypeReprGen(&_output);
    _hgen.reset(new HeaderGen(reprGen, &_output));
    _sgen.reset(new SourceGen(reprGen, &_output));
    for (auto it : package->modules()) {
        if (!generateTypesAndComponents(it.second)) {
            return false;
        }
    }

    TRY(generateSlices());

    std::string mainPath = _savePath + "/photon/Photon.c";
    TRY(saveOutput(mainPath.c_str(), &_main));

    TRY(generateTmPrivate(package));

    _main.resize(0);
    _output.resize(0);
    _hgen.reset();
    _sgen.reset();
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

bool Generator::generateTypesAndComponents(const Rc<Ast>& ast)
{
    _currentAst = ast;
    if (!ast->moduleInfo()->moduleName().equals("core")) {
        _output.setModName(ast->moduleInfo()->moduleName());
    } else {
        _output.setModName(bmcl::StringView::empty());
    }

    _photonPath.append(_currentAst->moduleInfo()->moduleName());
    TRY(makeDirectory(_photonPath.result().c_str()));
    _photonPath.append('/');

    SliceCollector coll;
    for (const auto& it : ast->typeMap()) {
        coll.collectUniqueSlices(it.second.get(), &_slices);
        const NamedType* type = it.second.get();
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
        const Rc<Component>& comp = ast->component().unwrap();
        coll.collectUniqueSlices(comp.get(), &_slices);

        _hgen->genComponentHeader(_currentAst.get(), comp.get());
        TRY(dump(comp->moduleName(), ".Component.h", &_photonPath));
        _output.clear();

        _output.append("static Photon");
        _output.appendWithFirstUpper(comp->moduleName());
        _output.append(" _");
        _output.appendWithFirstLower(comp->moduleName());
        _output.append(';');
        TRY(dump(comp->moduleName(), ".Component.inc.c", &_photonPath));
        _output.clear();

        //sgen->genTypeSource(type);
        //TRY(dump(type->name(), GEN_PREFIX ".c", &photonPath));
        _output.clear();
    }
    _photonPath.removeFromBack(_currentAst->moduleInfo()->moduleName().size() + 1);

    _currentAst = nullptr;
    return true;
}
}
