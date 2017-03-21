#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Hash.h"

#include <bmcl/ResultFwd.h>

#include <unordered_map>
#include <map>

namespace bmcl {
class Buffer;
}

namespace decode {

class Ast;
class Diagnostics;
class Parser;
class Package;
class Component;
class StatusRegexp;
struct ComponentAndMsg;

typedef bmcl::Result<Rc<Package>, void> PackageResult;

inline bool operator<(const bmcl::StringView left, const bmcl::StringView right)
{
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}

class Package : public RefCountable {
public:
    struct StringViewComparator
    {
        inline bool operator()(const bmcl::StringView& left, const bmcl::StringView& right) const
        {
            return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
        }
    };
    using AstMap = std::map<bmcl::StringView, Rc<Ast>, StringViewComparator>;

    static PackageResult readFromDirectory(const Rc<Diagnostics>& diag, const char* path);
    static PackageResult decodeFromMemory(const Rc<Diagnostics>& diag, const void* src, std::size_t size);

    ~Package();

    bmcl::Buffer encode() const;

    const AstMap& modules() const;
    const std::map<std::size_t, Rc<Component>>& components() const;
    const Rc<Diagnostics>& diagnostics() const;
    const std::vector<ComponentAndMsg>& statusMsgs() const;

private:
    Package(const Rc<Diagnostics>& diag);
    bool addFile(const char* path, Parser* p);
    void addAst(const Rc<Ast>& ast);
    bool resolveAll();
    bool resolveTypes(const Rc<Ast>& ast);
    bool resolveStatuses(const Rc<Ast>& ast);
    bool mapComponent(const Rc<Ast>& ast);

    Rc<Diagnostics> _diag;
    AstMap _modNameToAstMap;
    std::map<std::size_t, Rc<Component>> _components;
    std::vector<ComponentAndMsg> _statusMsgs;
};

inline const std::map<std::size_t, Rc<Component>>& Package::components() const
{
    return _components;
}

inline const Package::AstMap& Package::modules() const
{
    return _modNameToAstMap;
}

inline const Rc<Diagnostics>& Package::diagnostics() const
{
    return _diag;
}

inline const std::vector<ComponentAndMsg>& Package::statusMsgs() const
{
    return _statusMsgs;
}
}
