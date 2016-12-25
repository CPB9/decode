#include "decode/generator/Generator.h"
#include "decode/generator/HeaderGen.h"
#include "decode/generator/SourceGen.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Package.h"
#include "decode/parser/Decl.h"
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

bool Generator::saveOutput(const char* path)
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

    std::size_t size = _output.result().size();
    std::size_t total = 0;
    while(total < size) {
        ssize_t written = write(fd, _output.result().c_str() + total, size);
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
    bool isOk = WriteFile(handle, _output.result().c_str(), _output.result().size(), &bytesWritten, NULL);
    if (!isOk) {
        BMCL_CRITICAL() << "error writing file";
        //TODO: report error
        return false;
    }
    assert(_output.result().size() == bytesWritten);
#endif
    return true;
}

bool Generator::generateFromPackage(const Rc<Package>& package)
{
    std::unique_ptr<HeaderGen> hgen(new HeaderGen(&_output)); //TODO: move up
    std::unique_ptr<SourceGen> sgen(new SourceGen(&_output));
    for (auto it : package->modules()) {
        if (!generateFromAst(it.second, hgen.get(), sgen.get())) {
            return false;
        }
    }

    return true;
}

bool Generator::dump(const Type* type, bmcl::StringView ext, StringBuilder* currentPath)
{
    if (!_output.result().empty()) {
        currentPath->append(type->name());
        currentPath->append(ext);
        TRY(saveOutput(currentPath->result().c_str()));
        currentPath->removeFromBack(type->name().size() + 2);
        _output.clear();
    }
    return true;
}

bool Generator::generateFromAst(const Rc<Ast>& ast, HeaderGen* hgen, SourceGen* sgen)
{
    _currentAst = ast;
    if (!ast->moduleInfo()->moduleName().equals("core")) {
        _output.setModName(ast->moduleInfo()->moduleName());
    } else {
        _output.setModName(bmcl::StringView::empty());
    }

    StringBuilder photonPath;
    photonPath.append(_savePath);
    photonPath.append("/photon");
    TRY(makeDirectory(photonPath.result().c_str()));
    photonPath.append('/');
    photonPath.append(_currentAst->moduleInfo()->moduleName());
    TRY(makeDirectory(photonPath.result().c_str()));
    photonPath.append('/');

    for (const auto& it : ast->typeMap()) {
        const Type* type = it.second.get();
        if (type->typeKind() == TypeKind::Imported) {
            return true;
        }

        hgen->genHeader(_currentAst.get(), type);
        TRY(dump(type, ".h", &photonPath));

        sgen->genSource(type);
        TRY(dump(type, ".c", &photonPath));
    }

    _currentAst = nullptr;
    return true;
}

}
