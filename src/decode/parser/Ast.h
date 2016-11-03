#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Hash.h"

#include <bmcl/StringView.h>

#include <string>
#include <unordered_map>
#include <functional>

namespace decode {

class ModuleInfo;
class Module;
class Import;
class ImportedType;
class Type;

class Ast : public bmcl::RefCountable<unsigned int> {
public:
    Ast();
    ~Ast();

    const std::vector<Rc<Import>>& imports() const
    {
        return _importDecls;
    }

    const Rc<ModuleInfo>& moduleInfo() const
    {
        return _moduleInfo;
    }

    const std::unordered_map<bmcl::StringView, Rc<Type>>& typeMap() const
    {
        return _typeNameToType;
    }

private:
    friend class Parser;

    void addType(const Rc<Type>& type)
    {
    }

    std::vector<Rc<Import>> _importDecls;

    std::unordered_map<bmcl::StringView, Rc<Type>> _typeNameToType;
    Rc<Module> _moduleDecl;
    Rc<ModuleInfo> _moduleInfo;
};
}
