#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Hash.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Type.h"
#include "decode/parser/Constant.h"

#include <bmcl/StringView.h>
#include <bmcl/Option.h>

#include <string>
#include <unordered_map>
#include <functional>

namespace decode {

class ModuleInfo;
class Module;
class TypeImport;
class ImportedType;
class Type;
class TypeDecl;
class Component;
class Constant;

class Ast : public RefCountable {
public:
    Ast();
    ~Ast();

    const std::vector<Rc<TypeImport>>& imports() const
    {
        return _importDecls;
    }

    const Rc<ModuleInfo>& moduleInfo() const
    {
        return _moduleInfo;
    }

    const std::unordered_map<bmcl::StringView, Rc<NamedType>>& typeMap() const
    {
        return _typeNameToType;
    }

    const std::unordered_map<bmcl::StringView, Rc<Constant>>& constants() const
    {
        return _constants;
    }

    bmcl::Option<const Rc<NamedType>&> findTypeWithName(bmcl::StringView name) const
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

    const bmcl::Option<Rc<Component>>& component() const
    {
        return _component;
    }

private:
    friend class Parser;
    friend class Package;

    void addTopLevelType(const Rc<NamedType>& type)
    {
        auto it = _typeNameToType.emplace(type->name(), type);
        assert(it.second); //TODO: check for top level type name conflicts
    }

    void addImplBlock(const Rc<ImplBlock>& block)
    {
        _typeNameToImplBlock.emplace(block->name(), block);
    }

    std::vector<Rc<TypeImport>> _importDecls;

    bmcl::Option<Rc<Component>> _component;

    std::unordered_map<bmcl::StringView, Rc<NamedType>> _typeNameToType;
    std::unordered_map<bmcl::StringView, Rc<ImplBlock>> _typeNameToImplBlock;
    std::unordered_map<bmcl::StringView, Rc<Constant>> _constants;
    std::unordered_map<Rc<Type>, Rc<TypeDecl>> _typeToDecl;
    Rc<Module> _moduleDecl;
    Rc<ModuleInfo> _moduleInfo;
};

}
