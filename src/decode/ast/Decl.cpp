/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/Decl.h"
#include "decode/ast/Type.h"
#include "decode/ast/Function.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Field.h"

#include <bmcl/Result.h>
#include <bmcl/ArrayView.h>

namespace decode {

Decl::Decl(const ModuleInfo* info, Location start, Location end)
    : _moduleInfo(info)
    , _start(start)
    , _end(end)
{
}

Decl::Decl()
{
}

Decl::~Decl()
{
}

const ModuleInfo* Decl::moduleInfo() const
{
    return _moduleInfo.get();
}

void Decl::cloneDeclTo(Decl* dest)
{
    dest->_start = _start;
    dest->_end = _end;
    dest->_moduleInfo = _moduleInfo;
}


NamedDecl::NamedDecl()
{
}

NamedDecl::~NamedDecl()
{
}

bmcl::StringView NamedDecl::name() const
{
    return _name;
}

TypeDecl::TypeDecl()
{
}

TypeDecl::~TypeDecl()
{
}

const Type* TypeDecl::type() const
{
    return _type.get();
}

Type* TypeDecl::type()
{
    return _type.get();
}

ModuleDecl::ModuleDecl(const ModuleInfo* info, Location start, Location end)
    : Decl(info, start, end)
{
}

ModuleDecl::~ModuleDecl()
{
}

bmcl::StringView ModuleDecl::moduleName() const
{
    return moduleInfo()->moduleName();
}

ImportDecl::ImportDecl(const ModuleInfo* modInfo, bmcl::StringView path)
    : _importPath(path)
    , _modInfo(modInfo)
{
}

ImportDecl::~ImportDecl()
{
}

bmcl::StringView ImportDecl::path() const
{
    return _importPath;
}

ImportDecl::Types::ConstRange ImportDecl::typesRange() const
{
    return _types;
}

ImportDecl::Types::Range ImportDecl::typesRange()
{
    return _types;
}

bool ImportDecl::addType(ImportedType* type)
{
    auto it = std::find_if(_types.begin(), _types.end(), [type](const Rc<ImportedType>& current) {
        return current->name() == type->name();
    });
    if (it != _types.end()) {
        return false;
    }
    _types.emplace_back(type);
    return true;
}

ImplBlock::ImplBlock()
{
}

ImplBlock::~ImplBlock()
{
}

ImplBlock::Functions::ConstIterator ImplBlock::functionsBegin() const
{
    return _funcs.cbegin();
}

ImplBlock::Functions::ConstIterator ImplBlock::functionsEnd() const
{
    return _funcs.cend();
}

ImplBlock::Functions::ConstRange ImplBlock::functionsRange() const
{
    return _funcs;
}

void ImplBlock::addFunction(Function* func)
{
    _funcs.emplace_back(func);
}
}
