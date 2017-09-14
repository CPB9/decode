/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/Type.h"
#include "decode/ast/Ast.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Decl.h"
#include "decode/ast/Type.h"
#include "decode/ast/Decl.h"
#include "decode/ast/Constant.h"
#include "decode/ast/Component.h"
#include "decode/core/Hash.h"

#include <bmcl/Option.h>

namespace decode {

Ast::Ast()
{
}

Ast::~Ast()
{
}


Ast::Types::ConstIterator Ast::typesBegin() const
{
    return _types.cbegin();
}

Ast::Types::ConstIterator Ast::typesEnd() const
{
    return _types.cend();
}

Ast::Types::ConstRange Ast::typesRange() const
{
    return _types;
}

Ast::NamedTypes::ConstIterator Ast::namedTypesBegin() const
{
    return _typeNameToType.cbegin();
}

Ast::NamedTypes::ConstIterator Ast::namedTypesEnd() const
{
    return _typeNameToType.cend();
}

Ast::NamedTypes::ConstRange Ast::namedTypesRange() const
{
    return _typeNameToType;
}

Ast::Imports::ConstIterator Ast::importsBegin() const
{
    return _importDecls.cbegin();
}

Ast::Imports::ConstIterator Ast::importsEnd() const
{
    return _importDecls.cend();
}

Ast::Imports::ConstRange Ast::importsRange() const
{
    return _importDecls;
}

Ast::Imports::Range Ast::importsRange()
{
    return _importDecls;
}

bool Ast::hasConstants() const
{
    return !_constants.empty();
}

Ast::Constants::ConstIterator Ast::constantsBegin() const
{
    return _constants.cbegin();
}

Ast::Constants::ConstIterator Ast::constantsEnd() const
{
    return _constants.cend();
}

Ast::Constants::ConstRange Ast::constantsRange() const
{
    return _constants;
}

Ast::GenericDecls::ConstRange Ast::genericTypeDeclsRange() const
{
    return _genericDecls;
}

Ast::GenericDecls::Range Ast::genericTypeDeclsRange()
{
    return _genericDecls;
}

const ModuleInfo* Ast::moduleInfo() const
{
    return _moduleInfo.get();
}

bmcl::OptionPtr<const Component> Ast::component() const
{
    return _component.get();
}

bmcl::OptionPtr<Component> Ast::component()
{
    return _component.get();
}

bmcl::OptionPtr<const NamedType> Ast::findTypeWithName(bmcl::StringView name) const
{
    return _typeNameToType.findValueWithKey(name);
}

bmcl::OptionPtr<NamedType> Ast::findTypeWithName(bmcl::StringView name)
{
    return _typeNameToType.findValueWithKey(name);
}

bmcl::OptionPtr<const ImplBlock> Ast::findImplBlockWithName(bmcl::StringView name) const
{
    return _typeNameToImplBlock.findValueWithKey(name);
}

bmcl::OptionPtr<GenericTypeDecl> Ast::findGenericTypeDeclWithName(bmcl::StringView name)
{
    return _genericDecls.findValueWithKey(name);
}

void Ast::setModuleDecl(ModuleDecl* decl)
{
    _moduleDecl.reset(decl);
    _moduleInfo.reset(decl->moduleInfo());
}

void Ast::addType(Type* type)
{
    _types.emplace_back(type);
}

void Ast::addTopLevelType(NamedType* type)
{
    auto it = _typeNameToType.emplace(type->name(), type);
    assert(it.second); //TODO: check for top level type name conflicts
    _types.emplace_back(type);
}

void Ast::addImplBlock(ImplBlock* block)
{
    _typeNameToImplBlock.emplace(block->name(), block);
}

void Ast::addTypeImport(ImportDecl* decl)
{
    _importDecls.emplace_back(decl);
    for (ImportedType* type : decl->typesRange()) {
        addTopLevelType(type);
    }
}

void Ast::addConstant(Constant* constant)
{
    auto it = _constants.emplace(constant->name(), constant);
    assert(it.second); //TODO: check for top level type name conflicts
}

void Ast::addGenericTypeDecl(GenericTypeDecl* decl)
{
    auto it = _genericDecls.emplace(decl->name(), decl);
    assert(it.second); //TODO: check for top level type name conflicts
}

void Ast::setComponent(Component* comp)
{
    _component.reset(comp);
}

const std::string& Ast::fileName() const
{
    return _moduleInfo->fileName();
}
}
