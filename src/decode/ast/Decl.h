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
#include "decode/core/Location.h"
#include "decode/core/NamedRc.h"
#include "decode/parser/Containers.h"

#include <bmcl/Option.h>
#include <bmcl/Either.h>

#include <vector>

namespace decode {

class ImportedType;
class ModuleInfo;
class NamedType;
class Type;

//TODO: refact

class Decl : public RefCountable {
public:
    using Pointer = Rc<Decl>;
    using ConstPointer = Rc<const Decl>;

    Decl(const ModuleInfo* info, Location start, Location end);
    ~Decl();

    const ModuleInfo* moduleInfo() const;

protected:
    friend class Parser;
    void cloneDeclTo(Decl* dest);

    Decl();

private:

    Rc<const ModuleInfo> _moduleInfo;
    Location _start;
    Location _end;
};

class NamedDecl : public Decl {
public:
    using Pointer = Rc<NamedDecl>;
    using ConstPointer = Rc<const NamedDecl>;

    NamedDecl();
    ~NamedDecl();

    bmcl::StringView name() const;

private:
    friend class Parser;
    bmcl::StringView _name;
};

class TypeDecl : public Decl {
public:
    using Pointer = Rc<TypeDecl>;
    using ConstPointer = Rc<const TypeDecl>;

    ~TypeDecl();

    const Type* type() const;
    Type* type();

protected:
    TypeDecl();

private:
    friend class Parser;

    Rc<Type> _type;
};

class ModuleDecl : public Decl {
public:
    using Pointer = Rc<ModuleDecl>;
    using ConstPointer = Rc<const ModuleDecl>;

    ModuleDecl(const ModuleInfo* info, Location start, Location end);
    ~ModuleDecl();

    bmcl::StringView moduleName() const;
};

class ImportedType;

class ImportDecl : public RefCountable {
public:
    using Pointer = Rc<ImportDecl>;
    using ConstPointer = Rc<const ImportDecl>;
    using Types = RcVec<ImportedType>;

    ImportDecl(const ModuleInfo* modInfo, bmcl::StringView path);
    ~ImportDecl();

    bmcl::StringView path() const;
    Types::ConstRange typesRange() const;
    Types::Range typesRange();

    bool addType(ImportedType* type);

private:
    bmcl::StringView _importPath;
    Rc<const ModuleInfo> _modInfo;
    std::vector<Rc<ImportedType>> _types;
};

class FunctionType;

class Function;

class ImplBlock : public NamedDecl {
public:
    using Pointer = Rc<ImplBlock>;
    using ConstPointer = Rc<const ImplBlock>;
    using Functions = RcVec<Function>;

    ~ImplBlock();

    Functions::ConstIterator functionsBegin() const;
    Functions::ConstIterator functionsEnd() const;
    Functions::ConstRange functionsRange() const;
    void addFunction(Function* func);

private:
    friend class Parser;
    ImplBlock();

    Functions _funcs;
};
}
