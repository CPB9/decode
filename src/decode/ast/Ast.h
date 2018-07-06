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
#include "decode/parser/Containers.h"

#include <bmcl/Fwd.h>
#include <bmcl/StringView.h>
#include <bmcl/StringViewHash.h>

#include <string>
#include <functional>

namespace decode {

class BuiltinType;
class ModuleInfo;
class ModuleDecl;
class ImportDecl;
class ImplBlock;
class ImportedType;
class Type;
class NamedType;
class GenericInstantiationType;
class TypeDecl;
class Component;
class Constant;
class AllBuiltinTypes;

class Ast : public RefCountable {
public:
    using Pointer = Rc<Ast>;
    using ConstPointer = Rc<const Ast>;
    using Types = RcVec<Type>;
    using NamedTypes = RcSecondUnorderedMap<bmcl::StringView, NamedType>;
    using GenericInstantiations = RcVec<GenericInstantiationType>;
    using Constants = RcSecondUnorderedMap<bmcl::StringView, Constant>;
    using Imports = RcVec<ImportDecl>;
    using ImplBlocks = RcSecondUnorderedMap<Rc<Type>, ImplBlock>;

    Ast(AllBuiltinTypes* builtinTypes);
    ~Ast();

    Types::ConstIterator typesBegin() const;
    Types::ConstIterator typesEnd() const;
    Types::ConstRange typesRange() const;
    NamedTypes::ConstIterator namedTypesBegin() const;
    NamedTypes::ConstIterator namedTypesEnd() const;
    NamedTypes::ConstRange namedTypesRange() const;
    Imports::ConstIterator importsBegin() const;
    Imports::ConstIterator importsEnd() const;
    Imports::ConstRange importsRange() const;
    Imports::Range importsRange();
    Constants::ConstIterator constantsBegin() const;
    Constants::ConstIterator constantsEnd() const;
    Constants::ConstRange constantsRange() const;
    GenericInstantiations::ConstRange genericInstantiationsRange() const;
    GenericInstantiations::Range genericInstantiationsRange();

    bool hasConstants() const;
    const std::string& fileName() const;
    const ModuleInfo* moduleInfo() const;
    bmcl::StringView moduleName() const;
    bmcl::OptionPtr<const Component> component() const;
    bmcl::OptionPtr<Component> component();
    bmcl::OptionPtr<const NamedType> findTypeWithName(bmcl::StringView name) const;
    bmcl::OptionPtr<NamedType> findTypeWithName(bmcl::StringView name);
    bmcl::OptionPtr<const ImplBlock> findImplBlock(const Type* type) const;

    void setModuleDecl(ModuleDecl* decl);
    void addType(Type* type);
    void addTopLevelType(NamedType* type);
    void addImplBlock(Type* type, ImplBlock* block);
    void addTypeImport(ImportDecl* decl);
    void addConstant(Constant* constant);
    void addGenericInstantiation(GenericInstantiationType* type);
    void setComponent(Component* comp);

    const AllBuiltinTypes* builtinTypes() const;
    AllBuiltinTypes* builtinTypes();

private:
    Imports _importDecls;
    Rc<Component> _component;
    NamedTypes _typeNameToType;
    Types _types;
    ImplBlocks _typeToImplBlock;
    Constants _constants;
    GenericInstantiations _genericInstantiations;
    Rc<const ModuleInfo> _moduleInfo;
    Rc<ModuleDecl> _moduleDecl;
    Rc<AllBuiltinTypes> _allBuiltins;
};

}
