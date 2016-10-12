#pragma once

#include "decode/Config.h"
#include "decode/Rc.h"

#include <bmcl/StringView.h>

#include <string>
#include <unordered_map>
#include <functional>

namespace std
{
    template<>
    struct hash<bmcl::StringView>
    {
        std::size_t operator()(bmcl::StringView view) const
        {
            return std::size_t(view.begin());
        }
    };
}

namespace decode {
namespace parser {

class ModuleInfo;
class ModuleDecl;
class ImportDecl;
class Type;

class Ast : public bmcl::RefCountable<unsigned int> {
public:
    Ast();
    ~Ast();

private:
    friend class Parser;

    std::vector<Rc<ImportDecl>> _importDecls;

    std::unordered_map<bmcl::StringView, Rc<Type>> _typeNameToTypeMap;
    Rc<ModuleDecl> _moduleDecl;
    Rc<ModuleInfo> _moduleInfo;
};
}
}

