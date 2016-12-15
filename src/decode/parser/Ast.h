#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Hash.h"
#include "decode/parser/Decl.h"

#include <bmcl/StringView.h>
#include <bmcl/Option.h>

#include <string>
#include <unordered_map>
#include <functional>

namespace decode {

class ModuleInfo;
class Module;
class Import;
class ImportedType;
class Type;
class TypeDecl;

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

    bmcl::Option<const Rc<Type>&> findTypeWithName(bmcl::StringView name) const
    {
        auto it = _typeNameToType.find(name);
        if (it == _typeNameToType.end()) {
            return bmcl::None;
        }
        return it->second;
    }

    bmcl::Option<const Rc<ImplBlock>&> findImplBlockWithName(bmcl::StringView name) const
    {
        auto it = _typeNameToImplBlock.find(name);
        if (it == _typeNameToImplBlock.end()) {
            return bmcl::None;
        }
        return it->second;
    }

private:
    friend class Parser;

    void addTopLevelType(const Rc<Type>& type)
    {
        _typeNameToType.emplace(type->name(), type);
    }

    void addImplBlock(const Rc<ImplBlock>& block)
    {
        _typeNameToImplBlock.emplace(block->name(), block);
    }

    std::vector<Rc<Import>> _importDecls;

    std::unordered_map<bmcl::StringView, Rc<Type>> _typeNameToType;
    std::unordered_map<bmcl::StringView, Rc<ImplBlock>> _typeNameToImplBlock;
    std::unordered_map<Rc<Type>, Rc<TypeDecl>> _typeToDecl;
    Rc<Module> _moduleDecl;
    Rc<ModuleInfo> _moduleInfo;
};

}
