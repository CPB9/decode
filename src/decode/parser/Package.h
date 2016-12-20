#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Hash.h"

#include <bmcl/ResultFwd.h>

#include <unordered_map>

namespace bmcl {
class Buffer;
}

namespace decode {

class Ast;
class Diagnostics;

class Package;

typedef bmcl::Result<Rc<Package>, void> PackageResult;

class Package : public RefCountable {
public:
    static PackageResult readFromDirectory(const Rc<Diagnostics>& diag, const char* path);
    static PackageResult decodeFromMemory(const Rc<Diagnostics>& diag, const void* src, std::size_t size);

    bmcl::Buffer encode() const;

private:
    Package(const Rc<Diagnostics>& diag);
    bool addFile(const char* path);
    void addAst(const Rc<Ast>& ast);
    bool resolveAll();
    bool resolveTypes(const Rc<Ast>& ast);
    bool resolveStatuses(const Rc<Ast>& ast);

    Rc<Diagnostics> _diag;
    std::unordered_map<bmcl::StringView, Rc<Ast>> _modNameToAstMap;
};
}
