/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

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
class ModuleDecl;
class ImportDecl;
class ImportedType;
class Type;
class TypeDecl;
class Component;
class Constant;

class Ast : public RefCountable {
public:
    using Types = RcVec<Type>;
    using NamedTypes = RcSecondUnorderedMap<bmcl::StringView, NamedType>;
    using Constants = RcSecondUnorderedMap<bmcl::StringView, Constant>;
    using Imports = RcVec<ImportDecl>;
    using ImplBlocks = RcSecondUnorderedMap<bmcl::StringView, ImplBlock>;

    Ast();
    ~Ast();

    Types::ConstIterator typesBegin() const
    {
        return _types.cbegin();
    }

    Types::ConstIterator typesEnd() const
    {
        return _types.cend();
    }

    Types::ConstRange typesRange() const
    {
        return _types;
    }

    NamedTypes::ConstIterator namedTypesBegin() const
    {
        return _typeNameToType.cbegin();
    }

    NamedTypes::ConstIterator namedTypesEnd() const
    {
        return _typeNameToType.cend();
    }

    NamedTypes::ConstRange namedTypesRange() const
    {
        return _typeNameToType;
    }

    Imports::ConstIterator importsBegin() const
    {
        return _importDecls.cbegin();
    }

    Imports::ConstIterator importsEnd() const
    {
        return _importDecls.cend();
    }

    Imports::ConstRange importsRange() const
    {
        return _importDecls;
    }

    Imports::Range importsRange()
    {
        return _importDecls;
    }

    bool hasConstants() const
    {
        return !_constants.empty();
    }

    Constants::ConstIterator constantsBegin() const
    {
        return _constants.cbegin();
    }

    Constants::ConstIterator constantsEnd() const
    {
        return _constants.cend();
    }

    Constants::ConstRange constantsRange() const
    {
        return _constants;
    }

    const ModuleInfo* moduleInfo() const
    {
        return _moduleInfo.get();
    }

    bmcl::OptionPtr<const Component> component() const
    {
        return _component.get();
    }

    bmcl::OptionPtr<Component> component()
    {
        return _component.get();
    }

    bmcl::OptionPtr<const NamedType> findTypeWithName(bmcl::StringView name) const
    {
        return _typeNameToType.findValueWithKey(name);
    }

    bmcl::OptionPtr<NamedType> findTypeWithName(bmcl::StringView name)
    {
        return _typeNameToType.findValueWithKey(name);
    }

    bmcl::OptionPtr<const ImplBlock> findImplBlockWithName(bmcl::StringView name) const
    {
        return _typeNameToImplBlock.findValueWithKey(name);
    }

    void setModuleDecl(ModuleDecl* decl)
    {
        _moduleDecl.reset(decl);
        _moduleInfo.reset(decl->moduleInfo());
    }

    void addType(Type* type)
    {
        _types.emplace_back(type);
    }

    void addTopLevelType(NamedType* type)
    {
        auto it = _typeNameToType.emplace(type->name(), type);
        assert(it.second); //TODO: check for top level type name conflicts
        _types.emplace_back(type);
    }

    void addImplBlock(ImplBlock* block)
    {
        _typeNameToImplBlock.emplace(block->name(), block);
    }

    void addTypeImport(ImportDecl* decl)
    {
        _importDecls.emplace_back(decl);
        for (ImportedType* type : decl->typesRange()) {
            addTopLevelType(type);
        }
    }

    void addConstant(Constant* constant)
    {
        auto it = _constants.emplace(constant->name(), constant);
        assert(it.second); //TODO: check for top level type name conflicts
    }

    void setComponent(Component* comp)
    {
        _component.reset(comp);
    }

    const std::string& fileName() const
    {
        return _moduleInfo->fileName();
    }

private:
    Imports _importDecls;

    Rc<Component> _component;

    NamedTypes _typeNameToType;
    Types _types;
    ImplBlocks _typeNameToImplBlock;
    Constants _constants;
    //std::unordered_map<Rc<Type>, Rc<TypeDecl>> _typeToDecl;
    Rc<const ModuleInfo> _moduleInfo;
    Rc<ModuleDecl> _moduleDecl;
};

}
